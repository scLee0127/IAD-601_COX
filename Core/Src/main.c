/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include "global.h"
#include "myuart.h"
#include "mygpio.h"
#include "mytimer.h"
#include "build_info.h"
#include "my7segment.h"
#include "mygui.h"
#include "mydpsensor.h"
#include "myevent.h"
#include "mymodbus.h"
#include "mymotor.h"
#include "myadc.h"
#include "enviroment.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
IWDG_HandleTypeDef hiwdg;
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
// UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */
ADC_HandleTypeDef hadc;

#ifdef __GNUC__
    #define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
    #define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif /* __GNUC__ */

PUTCHAR_PROTOTYPE
{
    const HAL_StatusTypeDef status = HAL_UART_Transmit(&UartHandler_Debug,
                                                       (uint8_t *)&ch, 1, 1000);
    return (HAL_OK == status) ? ch : EOF;
}
// int _write( int file, char *p, int len )
// {
//     MYGPIO_Rs485_Eanble( MYGPIO_RS485_ENABLE );
//     HAL_UART_Transmit(&UartHandler_Debug, (uint8_t*)p, len, 1000);
//     MYGPIO_Rs485_Eanble( MYGPIO_RS485_DISABLE );
//     return len;
// }
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_IWDG_Init(void);
static void IWDG_Redfresh( void );

/* USER CODE BEGIN PFP */
void Print_Banner( void )
{
    MYUTIL_Printf("====================================================\r\n");
    MYUTIL_Printf("= GENP IAD200 by Simba\r\n");
    MYUTIL_Printf("= Version : %01d.%02d(%s)\r\n", FIRMWARE_MAJOR_NUM, FIRMWARE_MINOR_NUM, BUILD_DATETIME );
    MYUTIL_Printf("= FW Date : %s %s\r\n", __DATE__, __TIME__ );
    MYUTIL_Printf("====================================================\r\n");
}
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */

// static void uart_sanity_check(UART_HandleTypeDef *huart)
// {
//     uint32_t f_usart = HAL_RCCEx_GetPeriphCLKFreq(RCC_PERIPHCLK_USART1); // HAL이 인식한 USART1 클럭
//     uint32_t baud    = huart->Init.BaudRate;
//     uint32_t over8   = (huart->Init.OverSampling == UART_OVERSAMPLING_8) ? 1U : 0U;

//     // 기대 BRR 계산(F0 규칙): OVER8=0(16x) → round(f/baud), OVER8=1(8x) → round(2*f/baud)
//     uint32_t expect_brr = over8 ? ((2U*f_usart + baud/2U) / baud)
//                                 : ((    f_usart + baud/2U) / baud);

//     uint32_t actual_brr = huart->Instance->BRR;

//     // 상대 오차(‰) 계산
//     int32_t err_ppm = 0;
//     if (actual_brr != 0) {
//         int32_t diff = (int32_t)actual_brr - (int32_t)expect_brr;
//         err_ppm = (int32_t)((1000000LL * diff) / (int32_t)expect_brr); // ppm 단위
//     }

//     printf("[USART1] Fclk=%lu Hz, Baud=%lu, OVER8=%lu\r\n", (unsigned long)f_usart, (unsigned long)baud, (unsigned long)over8);
//     printf("  expect BRR=0x%lX (%lu), actual BRR=0x%lX (%lu), err=%ld ppm\r\n",
//            (unsigned long)expect_brr, (unsigned long)expect_brr,
//            (unsigned long)actual_brr, (unsigned long)actual_brr,
//            (long)err_ppm);
// }

// static void uart_dump_regs(USART_TypeDef *U)
// {
//     uint32_t cr1 = U->CR1, cr2 = U->CR2, cr3 = U->CR3, brr = U->BRR;

//     printf("CR1=0x%08lX  CR2=0x%08lX  CR3=0x%08lX  BRR=0x%08lX\r\n",
//            (unsigned long)cr1, (unsigned long)cr2, (unsigned long)cr3, (unsigned long)brr);

//     // 관심 비트 해석
//     printf("  OVER8=%lu M[12|M0]=%lu%lu PCE=%lu PS=%lu TE=%lu RE=%lu UE=%lu\r\n",
//            (unsigned long)((cr1>>15)&1),                     // OVER8
//            (unsigned long)((cr1>>12)&1), (unsigned long)((cr1>>28)&1), // M, M0 (시리즈별)
//            (unsigned long)((cr1>>10)&1), (unsigned long)((cr1>>9)&1),  // Parity
//            (unsigned long)((cr1>>3)&1),  (unsigned long)((cr1>>2)&1),  // TE/RE
//            (unsigned long)(cr1 & 1));                                   // UE
//     printf("  STOP=%lu  SWAP=%lu  TXINV=%lu RXINV=%lu  DATAINV=%lu MSBFIRST=%lu\r\n",
//            (unsigned long)((cr2>>12)&3),                     // STOP
//            (unsigned long)((cr2>>15)&1),                     // SWAP
//            (unsigned long)((cr2>>22)&1), (unsigned long)((cr2>>16)&1), // TXINV/RXINV
//            (unsigned long)((cr2>>27)&1), (unsigned long)((cr2>>19)&1));// DATAINV/MSB
// }

int main(void)
{

  /* USER CODE BEGIN 1 */
  uint8_t cnLedToggle_f = 0;
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */
  // MX_GPIO_Init();
  MX_IWDG_Init();     // watch dog
  MYGPIO_Init();
  MYUART_Debug_Init();
  /* The debug UART is ready here; emit exactly one boot banner before any
   * environment-load log can obscure whether USART2 itself is alive. */
  Print_Banner();
  ENV_Load_Enviroment_Info();
  MYUART_Init();

  ENV_Print_Load_Data();
  MYTIMER_Init();
  MYADC_Init();
  MYMOTOR_Init();
  // uart_sanity_check( &UartHandler_Debug );
  // uart_dump_regs( UartHandler_Debug.Instance );
  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  /* USER CODE BEGIN 2 */

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    IWDG_Redfresh();
    MYUART_Debug_Probe_Proc();
    if( 0 != gnBootTimer ) {
      MY7SEGMENT_Set_Value( FND_E, FND_n, FND_P_DOT, SEVEN_SEG_BLINK_OFF );
      // MYGPIO_Buzzer_Control( CONTROL_STATE_ON );
    }
    else{
      
    
      MYGUI_Proc();
      MYGPIO_Proc();
      MYEVENT_Proc();
      MYADC_Proc();  // Refresh the completed ADC sample before motor protection.
      MYMOTOR_Proc();
//      MYMODBUS_Proc();
      MYUART_Check_Tx_Timeout();
      if( 0 == gnUartCheckTimer ){
        gnUartCheckTimer = 1000;
        MYUART_Check_Uart_Status( &UartHandler_Rs486 );
      }

      if( ( 1 == gcnSaveEnv_f ) && ( 0 == gnSaveEnvDelayTimer ) ){
          gcnSaveEnv_f = 0;
          ENV_Save_Enviroment_Info();
      }
    }
    
    
    if( 0 == gnTestTimer  ){

      // MYUTIL_Printf("................ event %d / %ldPA %fA\r\n", gcnFireEvent_f, gnCorrectValue, ADC0_Get_Current());
      
      gnTestTimer = 1000;
      if( 0 == cnLedToggle_f ){
        cnLedToggle_f = 1;
        // HAL_GPIO_WritePin( MYGPIO_RS485_RX_LED_GPIO_PORT, MYGPIO_RS485_RX_LED_GPIO_NUM, GPIO_PIN_RESET );
        // HAL_GPIO_WritePin( MYGPIO_RS485_RX_LED_GPIO_PORT, MYGPIO_RS485_TX_LED_GPIO_NUM, GPIO_PIN_SET );
      }
      else{
        cnLedToggle_f = 0;
        // HAL_GPIO_WritePin( MYGPIO_RS485_RX_LED_GPIO_PORT, MYGPIO_RS485_RX_LED_GPIO_NUM, GPIO_PIN_SET );
        // HAL_GPIO_WritePin( MYGPIO_RS485_RX_LED_GPIO_PORT, MYGPIO_RS485_TX_LED_GPIO_NUM, GPIO_PIN_RESET );
      }
      
    }
    SM7291_Sensor_Proc();
    // MYBTN_Proc();

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL12;
  RCC_OscInitStruct.PLL.PREDIV = RCC_PREDIV_DIV1;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART1;
  PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_SYSCLK;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
/**
  * @brief IWDG Initialization Function
  * @param None
  * @retval None
  */
static void MX_IWDG_Init(void)
{

  /* USER CODE BEGIN IWDG_Init 0 */

  /* USER CODE END IWDG_Init 0 */

  /* USER CODE BEGIN IWDG_Init 1 */
  // 32x1,250/40,000 ~= 1sec
  /* USER CODE END IWDG_Init 1 */
  hiwdg.Instance = IWDG;
  hiwdg.Init.Prescaler = IWDG_PRESCALER_32;
  hiwdg.Init.Window = 4095;
  hiwdg.Init.Reload = 2500;             // watch dog 2seconds
  if (HAL_IWDG_Init(&hiwdg) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN IWDG_Init 2 */

  /* USER CODE END IWDG_Init 2 */

}

static void IWDG_Redfresh( void )
{
  if( HAL_OK != HAL_IWDG_Refresh(&hiwdg) ){
    Error_Handler();
  }
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
    MYUTIL_Printf("E");
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
