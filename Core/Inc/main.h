/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f0xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define FND_DIGIT_3_Pin GPIO_PIN_13
#define FND_DIGIT_3_GPIO_Port GPIOC
#define FND_DIGIT_2_Pin GPIO_PIN_14
#define FND_DIGIT_2_GPIO_Port GPIOC
#define FND_DIGIT_1_Pin GPIO_PIN_15
#define FND_DIGIT_1_GPIO_Port GPIOC
#define DOWN_KEY_Pin GPIO_PIN_0
#define DOWN_KEY_GPIO_Port GPIOA
#define GIDONG_IN_Pin GPIO_PIN_1
#define GIDONG_IN_GPIO_Port GPIOA
#define DEBUG_UART_TX_Pin GPIO_PIN_2
#define DEBUG_UART_TX_GPIO_Port GPIOA
#define DEBUG_UART_RX_Pin GPIO_PIN_3
#define DEBUG_UART_RX_GPIO_Port GPIOA
#define UP_KEY_Pin GPIO_PIN_4
#define UP_KEY_GPIO_Port GPIOA
#define LED_GIDONG_Pin GPIO_PIN_5
#define LED_GIDONG_GPIO_Port GPIOA
#define LED_POWER_Pin GPIO_PIN_7
#define LED_POWER_GPIO_Port GPIOA
#define FND_A_Pin GPIO_PIN_0
#define FND_A_GPIO_Port GPIOB
#define FND_B_Pin GPIO_PIN_1
#define FND_B_GPIO_Port GPIOB
#define FND_C_Pin GPIO_PIN_2
#define FND_C_GPIO_Port GPIOB
#define LED_MANUAL_Pin GPIO_PIN_10
#define LED_MANUAL_GPIO_Port GPIOB
#define MANUAL_IN_Pin GPIO_PIN_11
#define MANUAL_IN_GPIO_Port GPIOB
#define INB_Pin GPIO_PIN_12
#define INB_GPIO_Port GPIOB
#define INA_Pin GPIO_PIN_14
#define INA_GPIO_Port GPIOB
#define SEL0_Pin GPIO_PIN_15
#define SEL0_GPIO_Port GPIOB
#define RS485_EN_Pin GPIO_PIN_8
#define RS485_EN_GPIO_Port GPIOA
#define RS485_TX_Pin GPIO_PIN_9
#define RS485_TX_GPIO_Port GPIOA
#define RS485_RX_Pin GPIO_PIN_10
#define RS485_RX_GPIO_Port GPIOA
#define EXT_FAULT_ON_RELAY_Pin GPIO_PIN_11
#define EXT_FAULT_ON_RELAY_GPIO_Port GPIOA
#define EXT_ON_CHECK_RELAY_Pin GPIO_PIN_12
#define EXT_ON_CHECK_RELAY_GPIO_Port GPIOA
#define M_SW2_Pin GPIO_PIN_6
#define M_SW2_GPIO_Port GPIOF
#define M_SW1_Pin GPIO_PIN_7
#define M_SW1_GPIO_Port GPIOF
#define MODE_KEY_Pin GPIO_PIN_15
#define MODE_KEY_GPIO_Port GPIOA
#define FND_D_Pin GPIO_PIN_3
#define FND_D_GPIO_Port GPIOB
#define FND_E_Pin GPIO_PIN_4
#define FND_E_GPIO_Port GPIOB
#define FND_F_Pin GPIO_PIN_5
#define FND_F_GPIO_Port GPIOB
#define FND_G_Pin GPIO_PIN_6
#define FND_G_GPIO_Port GPIOB
#define GND_DP_Pin GPIO_PIN_7
#define GND_DP_GPIO_Port GPIOB
#define SENSOR_I2C_SCL_Pin GPIO_PIN_8
#define SENSOR_I2C_SCL_GPIO_Port GPIOB
#define SENSOR_I2C_SDA_Pin GPIO_PIN_9
#define SENSOR_I2C_SDA_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
