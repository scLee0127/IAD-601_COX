/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    stm32f0xx_it.c
  * @brief   Interrupt Service Routines.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stm32f0xx_it.h"
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "global.h"
#include "myuart.h"
#include "mytimer.h"
#include "mygpio.h"
#include "my7segment.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN TD */

/* USER CODE END TD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/* External variables --------------------------------------------------------*/

/* USER CODE BEGIN EV */

/* USER CODE END EV */

/******************************************************************************/
/*           Cortex-M0 Processor Interruption and Exception Handlers          */
/******************************************************************************/
/**
  * @brief This function handles Non maskable interrupt.
  */
void NMI_Handler(void)
{
  /* USER CODE BEGIN NonMaskableInt_IRQn 0 */

  /* USER CODE END NonMaskableInt_IRQn 0 */
  /* USER CODE BEGIN NonMaskableInt_IRQn 1 */
   while (1)
  {
  }
  /* USER CODE END NonMaskableInt_IRQn 1 */
}

/**
  * @brief This function handles Hard fault interrupt.
  */
void HardFault_Handler(void)
{
  /* USER CODE BEGIN HardFault_IRQn 0 */

  /* USER CODE END HardFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_HardFault_IRQn 0 */
    /* USER CODE END W1_HardFault_IRQn 0 */
  }
}

/**
  * @brief This function handles System service call via SWI instruction.
  */
void SVC_Handler(void)
{
  /* USER CODE BEGIN SVC_IRQn 0 */

  /* USER CODE END SVC_IRQn 0 */
  /* USER CODE BEGIN SVC_IRQn 1 */

  /* USER CODE END SVC_IRQn 1 */
}

/**
  * @brief This function handles Pendable request for system service.
  */
void PendSV_Handler(void)
{
  /* USER CODE BEGIN PendSV_IRQn 0 */

  /* USER CODE END PendSV_IRQn 0 */
  /* USER CODE BEGIN PendSV_IRQn 1 */

  /* USER CODE END PendSV_IRQn 1 */
}

/**
  * @brief This function handles System tick timer.
  */
void SysTick_Handler(void)
{
  /* USER CODE BEGIN SysTick_IRQn 0 */

  /* USER CODE END SysTick_IRQn 0 */
  HAL_IncTick();
  /* USER CODE BEGIN SysTick_IRQn 1 */
  MYTIMER_Proc();  // Update application timers every 1 ms.
  /* USER CODE END SysTick_IRQn 1 */
}

/******************************************************************************/
/* STM32F0xx Peripheral Interrupt Handlers                                    */
/* Add here the Interrupt Handlers for the used peripherals.                  */
/* For the available peripheral interrupt handler names,                      */
/* please refer to the startup file (startup_stm32f0xx.s).                    */
/******************************************************************************/

/**
  * @brief This function handles USART2 global interrupt.
  */
void USART2_IRQHandler(void)
{
  /* USER CODE BEGIN USART2_IRQn 0 */

  /* USER CODE END USART2_IRQn 0 */
  // HAL_UART_IRQHandler(&huart1);
  /* USER CODE BEGIN USART2_IRQn 1 */
  HAL_UART_IRQHandler( &UartHandler_Debug );
  /* USER CODE END USART2_IRQn 1 */
}

/**
  * @brief This function handles USART1 global interrupt.
  */
void USART1_IRQHandler(void)
{
    /* USER CODE BEGIN USART1_IRQn 0 */
    uint8_t cntmp = 0;
    uint32_t cnisr = 0;
    uint32_t cnicr = 0;

    // --- IDLE: 라인 유휴(프레임 경계) 감지 ---
    if (__HAL_UART_GET_FLAG(&UartHandler_Rs486, UART_FLAG_IDLE) != RESET &&
        __HAL_UART_GET_IT_SOURCE(&UartHandler_Rs486, UART_IT_IDLE) != RESET)
    {
        __HAL_UART_CLEAR_IDLEFLAG(&UartHandler_Rs486);  // ICR.IDLECF

        // 끊김 알림 → 상위에서 gnConsoleRxOffset 만큼 처리
        gnRS485RxBuffSize = gnRS485RxOffset;
        gnRS485Rx_f = 1;
    }

    cnisr = UartHandler_Rs486.Instance->ISR;  // Read, Flag snapshot
    if( cnisr & USART_ISR_ORE ){ cnicr |= USART_ICR_ORECF; }
    if( cnisr & USART_ISR_FE ){  cnicr |= USART_ICR_FECF; }
    if( cnisr & USART_ISR_NE ){  cnicr |= USART_ICR_NCF; }
    if( cnisr & USART_ISR_PE ){  cnicr |= USART_ICR_PECF; }
    if( cnicr) {
        UartHandler_Rs486.Instance->ICR = cnicr;
        // 필요시 더미리드로 라인 비우기 (옵션)
        volatile uint8_t dummy = (uint8_t)UartHandler_Rs486.Instance->RDR;
        (void)dummy;
    }

    // --- TC: TX DMA 전송 후 송신 완료 감지 ---
    // HAL_UART_Transmit_DMA는 DMA 전송이 끝나면 TC(Transmission Complete) 인터럽트를 발생시킵니다.
    // HAL 기본 핸들러를 사용하지 않으므로, 여기서 TC 플래그를 처리하고 상태를 복구해야 무한 루프에 빠지지 않습니다.
    if ((cnisr & USART_ISR_TC) && (UartHandler_Rs486.Instance->CR1 & USART_CR1_TCIE))
    {
      UartHandler_Rs486.Instance->ICR = USART_ICR_TCCF;   // TC 플래그 물리적 클리어
      UartHandler_Rs486.Instance->CR1 &= ~USART_CR1_TCIE; // TC 인터럽트 비활성화
      UartHandler_Rs486.gState = HAL_UART_STATE_READY;    // UART 송신 상태를 READY로 강제 복구
      HAL_UART_TxCpltCallback(&UartHandler_Rs486);        // 송신 완료 콜백 호출 (RS485 DE 핀 제어 로직 실행)
    }
    
    if( ( RESET != __HAL_UART_GET_FLAG( &UartHandler_Rs486, UART_FLAG_RXNE ) ) && 
        ( RESET != __HAL_UART_GET_IT_SOURCE( &UartHandler_Rs486, UART_IT_RXNE ) ) ){
        cntmp = (uint8_t)( UartHandler_Rs486.Instance->RDR & (uint8_t)0x00FF );

        if( gnRS485RxOffset >= ( RS485_BUFF_SIZE - 1 ) ){
            gnRS485RxOffset = 0;
        }

        gabRS485RxBuff[gnRS485RxOffset++] = cntmp;
        gabRS485RxBuff[gnRS485RxOffset] = '\0';
    }
    // __HAL_UART_CLEAR_PEFLAG( &UartHandler_Rs486 );
    /* USER CODE END USART2_IRQn 0 */
    /* USER CODE BEGIN USART2_IRQn 1 */
    // HAL_UART_IRQHandler( &UartHandler_Rs486 );
    /* USER CODE END USART2_IRQn 1 */
}

/* USER CODE BEGIN 1 */
void TIM6_IRQHandler(void)
{
    if (__HAL_TIM_GET_FLAG(&htim6, TIM_FLAG_UPDATE) != RESET &&
        __HAL_TIM_GET_IT_SOURCE(&htim6, TIM_IT_UPDATE) != RESET)
    {
        __HAL_TIM_CLEAR_IT(&htim6, TIM_IT_UPDATE);
        MY7SEGMENT_Proc();
    }
}

/**
  * @brief This function handles DMA1 channel 2 and 3 interrupts.
  */
void DMA1_Channel2_3_IRQHandler(void)
{
  /* USER CODE BEGIN DMA1_Channel2_3_IRQn 0 */

  /* USER CODE END DMA1_Channel2_3_IRQn 0 */
  HAL_DMA_IRQHandler(&hdma_usart1_tx);
  /* USER CODE BEGIN DMA1_Channel2_3_IRQn 1 */

  /* USER CODE END DMA1_Channel2_3_IRQn 1 */
}
/* USER CODE END 1 */
