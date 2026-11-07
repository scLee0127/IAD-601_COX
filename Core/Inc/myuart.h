#ifndef __MYUART_H__
#define __MYUART_H__

// define
#define UART_BAUD_1200 				0
#define UART_BAUD_2400 				1
#define UART_BAUD_4800 				2
#define UART_BAUD_9600 				3
#define UART_BAUD_14400 			4
#define UART_BAUD_19200 			5
#define UART_BAUD_38400 			6
#define UART_BAUD_57600 			7
#define UART_BAUD_115200 			8
#define UART_BAUD_230400 			9

#define UART_DBBUG_COM_NUM  		USART2
#define UART_RS485_COM_NUM  		USART1

#define MYUART_INSTANCE_DEBUG  		UART_DBBUG_COM_NUM
#define MYUART_INSTANCE_RS485  		UART_RS485_COM_NUM

#define UART_TX_TIMEOUT_MS  		1000
// variable
extern UART_HandleTypeDef UartHandler_Debug;      // debug UART2
extern UART_HandleTypeDef UartHandler_Rs486;      // RS485 UART1
extern DMA_HandleTypeDef hdma_usart1_tx;

// Funciton
void MYUART_Debug_Init( void );
void MYUART_Debug_Probe_Proc(void);
void MYUART_Init( void );
void MYUART_RS485_ReInit( void );
uint8_t MYUART_Send_Data( UART_HandleTypeDef *_phuart, uint8_t* _pbData, uint16_t _nSize );
uint8_t MYUART_Send_Data_DMA( UART_HandleTypeDef *_phuart, const uint8_t* _pbData, uint16_t _nSize );
void MYUART_Check_Tx_Timeout( void );
void MYUART_Check_Uart_Status( UART_HandleTypeDef *_phuart );
void MYUART_IRQ_DMA_Recovery( UART_HandleTypeDef *huart );
#endif // __MYUART_H__
