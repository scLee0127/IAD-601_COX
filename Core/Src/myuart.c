#include "main.h"
#include "global.h"
#include "myuart.h"
#include "mygpio.h"
#include "enviroment.h"
#include "myringbuffer.h"
/////////////////////////////////////////////////////////////////////////////////////////////
// import function prototype


/////////////////////////////////////////////////////////////////////////////////////////////
// local function prototype
static int _myuart_convert_baudrate( int _nbaud );
static void _myuart_uart2_debug_init( uint8_t _cnbaudrate );
static void _myuart_uart1_rs485_init( const uint8_t _cnbaudrate );
static void _myuart_tx_ring_buffer_init( void );
static void _myuart_rs485_rx_reset( void );
static float _myuart_1byte_send_time_ms( const int _nbaud );
static void _myuart_set_tx_timeout( uint16_t _nSize );

/////////////////////////////////////////////////////////////////////////////////////////////
// global variable
UART_HandleTypeDef UartHandler_Debug;      // debug UART2
UART_HandleTypeDef UartHandler_Rs486;      // RS485 UART1
DMA_HandleTypeDef hdma_usart1_tx;
ring_buffer_t gstRing_RS485_Tx;
static volatile uint8_t gcnUartIt_RS485_Tx_Busy_f = 0;
static uint32_t gnUartTxTime_RS485 = 0;

#define RS485_DEFAULT_TIMEOUT_MS            100
#define RS485_SEND_TIME_RATIO               1.5f
uint32_t gnRS485_Tx_Timeout = 0;
uint8_t gcnRS485_Baudrate = 0;
/////////////////////////////////////////////////////////////////////////////////////////////
// public and export function implement
void MYUART_Debug_Init( void )
{
    if( UartHandler_Debug.gState == HAL_UART_STATE_RESET ){
        _myuart_uart2_debug_init( UART_BAUD_230400 );
        /* Send boot logs immediately, including messages without a newline. */
        setvbuf( stdout, NULL, _IONBF, 0 );
    }
}

void MYUART_Init( void )
{
    const DEVICEENV* pstenv = Env_Get_Enviroment();
    gcnRS485_Baudrate = (uint8_t)pstenv->nBaudrate;
    _myuart_tx_ring_buffer_init();

    MYUART_Debug_Init();
    _myuart_uart1_rs485_init( gcnRS485_Baudrate );
}

void MYUART_RS485_ReInit( void )
{
    const DEVICEENV* pstenv = Env_Get_Enviroment();
    gcnRS485_Baudrate = (uint8_t)pstenv->nBaudrate;
    HAL_NVIC_DisableIRQ(USART1_IRQn);
    HAL_UART_DeInit( &UartHandler_Rs486 );
    _myuart_uart1_rs485_init( gcnRS485_Baudrate );
}

uint8_t MYUART_Send_Data( UART_HandleTypeDef *_phuart, uint8_t* _pbData, uint16_t _nSize )
{
    int cnret = 0;
    
    if( MYUART_INSTANCE_RS485 == _phuart->Instance ){
        MYGPIO_Rs485_Eanble( MYGPIO_RS485_ENABLE );
    }

    cnret = HAL_UART_Transmit( _phuart, _pbData, _nSize, UART_TX_TIMEOUT_MS );
    // DBG_Hex_Print((char*)"MODBUS[S]", _pbData, _nSize );
    if( MYUART_INSTANCE_RS485 == _phuart->Instance ){
    	HAL_Delay(1);
        MYGPIO_Rs485_Eanble( MYGPIO_RS485_DISABLE );
    }
    
    if( HAL_OK != cnret ){
        MYUTIL_Printf("%s %d\r\n", __func__, cnret );
    }
    return cnret;
}

uint8_t MYUART_Send_Data_DMA( UART_HandleTypeDef *_phuart, const uint8_t* _pbData, uint16_t _nSize )
{
    int cnret = 0;
    HAL_StatusTypeDef nhal_st;
    if( MYUART_INSTANCE_RS485 == _phuart->Instance ){
        HAL_NVIC_DisableIRQ(USART1_IRQn);
        // DBG_Hex_Print((char*)"MODBUS[S]", _pbData, _nSize );
        cnret = MYRING_Buffer_Write_Partial( &gstRing_RS485_Tx, _pbData, _nSize  );

        if( !gcnUartIt_RS485_Tx_Busy_f && !MYRING_Buffer_is_Empty( &gstRing_RS485_Tx ) ){
            uint16_t ntx_len = MYRING_Buffer_Read_Block( &gstRing_RS485_Tx, gabRS485TxDmaBuf, RS485_TX_DMA_BUFF_SIZE );
            if( ntx_len > 0 ){
                gcnUartIt_RS485_Tx_Busy_f = 1; // 전송 시작 전 Busy 플래그를 먼저 설정합니다.
                gnUartTxTime_RS485 = HAL_GetTick();
                _myuart_set_tx_timeout( _nSize );
                MYGPIO_Rs485_Eanble( MYGPIO_RS485_ENABLE );
                
                // 트랜시버(MAX485 등)가 송신 모드로 완전히 전환될 수 있도록 아주 짧은 딜레이 추가 (약 1~2us)
                for(volatile int i = 0; i < 20; i++) {
                    __NOP();
                }

                nhal_st = HAL_UART_Transmit_DMA( _phuart, gabRS485TxDmaBuf, ntx_len );
                if( HAL_OK != nhal_st ){
                    // 전송 시작에 실패하면, 상태를 원상 복구합니다.
                    MYGPIO_Rs485_Eanble( MYGPIO_RS485_DISABLE );
                    gcnUartIt_RS485_Tx_Busy_f = 0;
                }
            }
        }
        HAL_NVIC_EnableIRQ(USART1_IRQn);
    }
    return cnret;
}

void MYUART_Check_Tx_Timeout( void )
{
    uint32_t ncurr_tick = HAL_GetTick();

    if( gcnUartIt_RS485_Tx_Busy_f ){
        if( ( ncurr_tick - gnUartTxTime_RS485 ) > gnRS485_Tx_Timeout ){
            MYUTIL_Printf("RS485 TX Timeout Recovery[%ld]\r\n", gnRS485_Tx_Timeout);
            HAL_UART_AbortTransmit( &UartHandler_Rs486 );
            MYGPIO_Rs485_Eanble( MYGPIO_RS485_DISABLE );
            gcnUartIt_RS485_Tx_Busy_f = 0;
        }
    }
}

void MYUART_Check_Uart_Status( UART_HandleTypeDef *_phuart )
{
    // 1. 상태(State)와 에러(Error)를 각각 가져옵니다.
    uint32_t tx_dma_error = HAL_DMA_ERROR_NONE;
    if (_phuart->hdmatx != NULL) {
        tx_dma_error = HAL_DMA_GetError(_phuart->hdmatx);
    }

    // 2. Check ISR register
    uint32_t nuart_err = _phuart->Instance->ISR & ( UART_FLAG_ORE | UART_FLAG_FE | UART_FLAG_NE );

    // 3. Check RX Interrupts (RXNE, IDLE) are alive
    // 하드웨어 에러나 노이즈로 인해 RX 인터럽트가 꺼졌는지 확인합니다.
    uint32_t rx_it_err = 0;
    if (__HAL_UART_GET_IT_SOURCE(_phuart, UART_IT_RXNE) == RESET ||
        __HAL_UART_GET_IT_SOURCE(_phuart, UART_IT_IDLE) == RESET) {
        rx_it_err = 1;
    }

    // 4. 복구 조건: TX DMA 에러 코드가 있거나, UART 하드웨어 에러, 또는 RX 인터럽트 비정상 감지 시
    if (tx_dma_error != HAL_DMA_ERROR_NONE || nuart_err != 0 || rx_it_err != 0){
        MYUTIL_Printf("\r\n!!! [RS485] ERR (TX_D:%lx, U:%lx, RX_IT:%lx) !!!\r\n", tx_dma_error, nuart_err, rx_it_err);
        
        // [복구 단계]
        // 에러 플래그 클리어
        _phuart->Instance->ICR = (UART_CLEAR_OREF | UART_CLEAR_FEF | UART_CLEAR_NEF);
        
        // 더미 리드로 멈춰있는 수신 레지스터(RDR) 비우기 (ORE 방지)
        volatile uint32_t dummy = _phuart->Instance->RDR;
        (void)dummy;

        // 전송/수신 모두 리셋 및 상태 초기화
        MYUART_IRQ_DMA_Recovery(_phuart);

        MYUTIL_Printf("[RS485] Recovery Completed.\r\n");
    }
}

void MYUART_IRQ_DMA_Recovery( UART_HandleTypeDef *huart )
{
    // 1. 하드웨어 즉시 중지
    // 수신(RX)에 영향을 주지 않도록 진행 중인 송신(TX) DMA만 멈추고 gState를 READY로 복구
    HAL_UART_AbortTransmit( huart );

    // 2. HAL 라이브러리 상태 변수 강제 초기화(한번더 확인)
    huart->gState = HAL_UART_STATE_READY;
    huart->RxState = HAL_UART_STATE_READY;
    huart->ErrorCode = HAL_UART_ERROR_NONE;

    // 3. 소프트웨어 플래그 초기화
    if( MYUART_INSTANCE_RS485 == huart->Instance ){
        gcnUartIt_RS485_Tx_Busy_f = 0;
        
        /* Reset flags, software state and RXNE/IDLE/NVIC as one operation. */
        _myuart_rs485_rx_reset();
    }
}

void HAL_UART_TxCpltCallback( UART_HandleTypeDef *_phuart )
{
    HAL_StatusTypeDef nhal_st;
    if( MYUART_INSTANCE_RS485 == _phuart->Instance ){
        // 링 버퍼에 남은 데이터가 있는지 확인합니다.
        if( !MYRING_Buffer_is_Empty( &gstRing_RS485_Tx ) ){
            uint16_t ntx_len = MYRING_Buffer_Read_Block( &gstRing_RS485_Tx, gabRS485TxDmaBuf, RS485_TX_DMA_BUFF_SIZE );
            if( ntx_len > 0 ){
                // 보낼 데이터가 더 있다면, 다음 DMA 전송을 시작합니다.
                // DE 핀은 이미 활성화 상태이므로 다시 설정할 필요가 없습니다.
                gnUartTxTime_RS485 = HAL_GetTick();
                nhal_st = HAL_UART_Transmit_DMA( _phuart, gabRS485TxDmaBuf, ntx_len );
                if( HAL_OK != nhal_st ){
                    // 만약 다음 전송 시작에 실패하면, 모든 전송을 중단하고 에러 상태를 처리합니다.
                    MYGPIO_Rs485_Eanble( MYGPIO_RS485_DISABLE );
                    gcnUartIt_RS485_Tx_Busy_f = 0;
                }
                // 다음 TxCpltCallback을 기다립니다.
                return;
            }
        }
        // 링 버퍼가 비어있으면 모든 데이터 전송이 완료된 것입니다.
        MYGPIO_Rs485_Eanble( MYGPIO_RS485_DISABLE ); // DE 핀을 수신 모드로 전환합니다.
        gcnUartIt_RS485_Tx_Busy_f = 0;               // Busy 플래그를 해제합니다.
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////
// local function implement
static void _myuart_uart2_debug_init( uint8_t _cnbaudrate )
{
	UartHandler_Debug.Instance = MYUART_INSTANCE_DEBUG;
	UartHandler_Debug.Init.BaudRate = _myuart_convert_baudrate( _cnbaudrate );
	UartHandler_Debug.Init.WordLength = UART_WORDLENGTH_8B;
	UartHandler_Debug.Init.StopBits = UART_STOPBITS_1;
	UartHandler_Debug.Init.Parity = UART_PARITY_NONE;
	UartHandler_Debug.Init.Mode = UART_MODE_TX_RX;
	UartHandler_Debug.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	UartHandler_Debug.Init.OverSampling = UART_OVERSAMPLING_16;
	UartHandler_Debug.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
	UartHandler_Debug.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
	if (HAL_UART_Init(&UartHandler_Debug) != HAL_OK)
	{
		Error_Handler();
	}

    // uart_sanity_check(&UartHandler_Debug);
}

static void _myuart_uart1_rs485_init( const uint8_t _cnbaudrate )
{
    MYUTIL_Printf("RS485 Baud %d/%d\r\n",_cnbaudrate, _myuart_convert_baudrate( _cnbaudrate ) );

    /* DMA1 하드웨어 클럭 활성화 (가장 중요) */
    __HAL_RCC_DMA1_CLK_ENABLE();

    HAL_NVIC_SetPriority( DMA1_Channel2_3_IRQn, 3, 0);
	HAL_NVIC_EnableIRQ( DMA1_Channel2_3_IRQn);
    /* USART1 DMA Init */
    /* USART1_TX Init */
    hdma_usart1_tx.Instance = DMA1_Channel2;
    hdma_usart1_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
    hdma_usart1_tx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_usart1_tx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_usart1_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_usart1_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma_usart1_tx.Init.Mode = DMA_NORMAL;
    hdma_usart1_tx.Init.Priority = DMA_PRIORITY_LOW;
    if (HAL_DMA_Init(&hdma_usart1_tx) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_LINKDMA( &UartHandler_Rs486, hdmatx, hdma_usart1_tx );

	UartHandler_Rs486.Instance = MYUART_INSTANCE_RS485;
	UartHandler_Rs486.Init.BaudRate = _myuart_convert_baudrate( _cnbaudrate );
	UartHandler_Rs486.Init.WordLength = UART_WORDLENGTH_8B;
	UartHandler_Rs486.Init.StopBits = UART_STOPBITS_1;
	UartHandler_Rs486.Init.Parity = UART_PARITY_NONE;
	UartHandler_Rs486.Init.Mode = UART_MODE_TX_RX;
	UartHandler_Rs486.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	UartHandler_Rs486.Init.OverSampling = UART_OVERSAMPLING_16;
	UartHandler_Rs486.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
	UartHandler_Rs486.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
	if (HAL_UART_Init(&UartHandler_Rs486) != HAL_OK)
	{
		Error_Handler();
	}

	/* HAL init leaves application RX interrupts disabled.  Clear stale UART
	 * state first, then enable RXNE, IDLE and the USART1 vector together. */
	_myuart_rs485_rx_reset();
	MYGPIO_Rs485_Eanble(MYGPIO_RS485_DISABLE);
}

static void _myuart_rs485_rx_reset( void )
{
	volatile uint32_t dummy;

	HAL_NVIC_DisableIRQ(USART1_IRQn);
	__HAL_UART_DISABLE_IT(&UartHandler_Rs486, UART_IT_RXNE);
	__HAL_UART_DISABLE_IT(&UartHandler_Rs486, UART_IT_IDLE);

	/* Drain RDR before enabling the new receive session. */
	while ((UartHandler_Rs486.Instance->ISR & USART_ISR_RXNE) != 0U) {
		dummy = UartHandler_Rs486.Instance->RDR;
		(void)dummy;
	}
	UartHandler_Rs486.Instance->ICR = UART_CLEAR_PEF | UART_CLEAR_FEF |
		UART_CLEAR_NEF | UART_CLEAR_OREF | UART_CLEAR_IDLEF | UART_CLEAR_TCF;

	gnRS485RxOffset = 0U;
	gnRS485RxBuffSize = 0U;
	gnRS485Rx_f = 0U;
	gabRS485RxBuff[0] = '\0';

	__HAL_UART_ENABLE_IT(&UartHandler_Rs486, UART_IT_RXNE);
	__HAL_UART_ENABLE_IT(&UartHandler_Rs486, UART_IT_IDLE);
	HAL_NVIC_ClearPendingIRQ(USART1_IRQn);
	HAL_NVIC_EnableIRQ(USART1_IRQn);
}

static void _myuart_tx_ring_buffer_init( void )
{
    MYRING_Buffer_Init( &gstRing_RS485_Tx, gabRS485TxRingBuf, RS485_TX_RING_BUFF_SIZE );
}

static void _myuart_set_tx_timeout( uint16_t _nSize )
{
    float nfonebytetime = _myuart_1byte_send_time_ms( gcnRS485_Baudrate );
    gnRS485_Tx_Timeout = (int)( nfonebytetime * (float)_nSize * RS485_SEND_TIME_RATIO );
    if( 0 == gnRS485_Tx_Timeout ){
        gnRS485_Tx_Timeout = RS485_DEFAULT_TIMEOUT_MS;
    }
    // MYUTIL_Printf("%ld\r\n", gnRS485_Tx_Timeout);
}

static float _myuart_1byte_send_time_ms( const int _nbaud )
{
    float nfret = 0.0f;
    // 1000.0f / ( baudrate / 8 :: 1000ms per 1byte

    if( UART_BAUD_1200 == _nbaud ){
        nfret = 6.67f; 
    }
    else
    if( UART_BAUD_2400 == _nbaud ){
        nfret = 3.33f; 
    }
    else
    if( UART_BAUD_4800 == _nbaud ){
        nfret = 1.67f; 
    }
    else
    if( UART_BAUD_9600 == _nbaud ){
        nfret = 0.83f; 
    }
    else
    if( UART_BAUD_14400 == _nbaud ){
        nfret = 0.56f; 
    }
    else
    if( UART_BAUD_19200 == _nbaud ){
        nfret = 0.42f;
    }
    else
    if( UART_BAUD_38400 == _nbaud ){
        nfret = 0.21f;
    }
    else
    if( UART_BAUD_57600 == _nbaud ){
        nfret = 0.14f;
    }
    else
    if( UART_BAUD_115200 == _nbaud ){
        nfret = 0.07f;
    }
    else
    if( UART_BAUD_230400 == _nbaud ){
        nfret = 0.4f;
    }
    else{
        nfret = 0.83f;
    }

    return nfret;
}

static int _myuart_convert_baudrate( const int _nbaud )
{
    int nbuadrate = 0;

    if( UART_BAUD_1200 == _nbaud ){
        nbuadrate = 1200; 
    }
    else
    if( UART_BAUD_2400 == _nbaud ){
        nbuadrate = 2400; 
    }
    else
    if( UART_BAUD_4800 == _nbaud ){
        nbuadrate = 4800; 
    }
    else
    if( UART_BAUD_9600 == _nbaud ){
        nbuadrate = 9600; 
    }
    else
    if( UART_BAUD_14400 == _nbaud ){
        nbuadrate = 14400; 
    }
    else
    if( UART_BAUD_19200 == _nbaud ){
        nbuadrate = 19200;
    }
    else
    if( UART_BAUD_38400 == _nbaud ){
        nbuadrate = 38400;
    }
    else
    if( UART_BAUD_57600 == _nbaud ){
        nbuadrate = 57600;
    }
    else
    if( UART_BAUD_115200 == _nbaud ){
        nbuadrate = 115200;
    }
    else
    if( UART_BAUD_230400 == _nbaud ){
        nbuadrate = 230400;
    }
    else{
        nbuadrate = 9600;
    }

    return nbuadrate;
}
