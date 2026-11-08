#include "main.h"
#include "global.h"
#include "mygpio.h"
#include "mytimer.h"
/////////////////////////////////////////////////////////////////////////////////////////////
// import function prototype


/////////////////////////////////////////////////////////////////////////////////////////////
// local function prototype
void _mygpio_buzzer_proc( void );

/////////////////////////////////////////////////////////////////////////////////////////////
// global variable


/////////////////////////////////////////////////////////////////////////////////////////////
// public and export function implement
void MYGPIO_Init( void )
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();
	__HAL_RCC_GPIOC_CLK_ENABLE();
	__HAL_RCC_GPIOF_CLK_ENABLE();

	/* RS485 Enable/Disable GPIO */
  	HAL_GPIO_WritePin( MYGPIO_RS485_ENABLE_GPIO_PORT, MYGPIO_RS485_ENABLE_GPIO_NUM, GPIO_PIN_RESET );

  	/*Configure GPIO pins : MYGPIO_RS485_ENABLE_GPIO_NUM */
	GPIO_InitStruct.Pin = MYGPIO_RS485_ENABLE_GPIO_NUM;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(MYGPIO_RS485_ENABLE_GPIO_PORT, &GPIO_InitStruct);
	HAL_GPIO_WritePin( MYGPIO_RS485_ENABLE_GPIO_PORT, MYGPIO_RS485_ENABLE_GPIO_NUM, GPIO_PIN_RESET );


  	/* Led GPIO */
	HAL_GPIO_WritePin( MYGPIO_LED_GPIO_PORT, MYGPIO_LED_POWER_GPIO_NUM | MYGPIO_LED_GIDONG_GPIO_NUM, GPIO_PIN_SET );
	GPIO_InitStruct.Pin = MYGPIO_LED_POWER_GPIO_NUM | MYGPIO_LED_GIDONG_GPIO_NUM;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(MYGPIO_LED_GPIO_PORT, &GPIO_InitStruct);

	HAL_GPIO_WritePin( MYGPIO_LED_M_GPIO_PORT, MYGPIO_LED_MANUAL_GPIO_NUM, GPIO_PIN_SET );
	GPIO_InitStruct.Pin = MYGPIO_LED_MANUAL_GPIO_NUM;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(MYGPIO_LED_M_GPIO_PORT, &GPIO_InitStruct);

	/* 7Segment GPIO */
	// digit
	HAL_GPIO_WritePin( MYGPIO_7SEG_GPIO_PORT, MYGPIO_7SEG_DIGIT1_GPIO_NUM | MYGPIO_7SEG_DIGIT2_GPIO_NUM | MYGPIO_7SEG_DIGIT3_GPIO_NUM, GPIO_PIN_RESET );
	GPIO_InitStruct.Pin = MYGPIO_7SEG_DIGIT1_GPIO_NUM | MYGPIO_7SEG_DIGIT2_GPIO_NUM | MYGPIO_7SEG_DIGIT3_GPIO_NUM;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(MYGPIO_7SEG_GPIO_PORT, &GPIO_InitStruct);

	// data
	HAL_GPIO_WritePin( MYGPIO_7SEG_DATA_GPIO_PORT, MYGPIO_7SEG_A_GPIO_NUM | MYGPIO_7SEG_B_GPIO_NUM | MYGPIO_7SEG_C_GPIO_NUM
						| MYGPIO_7SEG_D_GPIO_NUM | MYGPIO_7SEG_E_GPIO_NUM | MYGPIO_7SEG_F_GPIO_NUM 
						| MYGPIO_7SEG_G_GPIO_NUM | MYGPIO_7SEG_DP_GPIO_NUM, GPIO_PIN_SET );
	GPIO_InitStruct.Pin = MYGPIO_7SEG_A_GPIO_NUM | MYGPIO_7SEG_B_GPIO_NUM | MYGPIO_7SEG_C_GPIO_NUM
						| MYGPIO_7SEG_D_GPIO_NUM | MYGPIO_7SEG_E_GPIO_NUM | MYGPIO_7SEG_F_GPIO_NUM 
						| MYGPIO_7SEG_G_GPIO_NUM | MYGPIO_7SEG_DP_GPIO_NUM;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(MYGPIO_7SEG_DATA_GPIO_PORT, &GPIO_InitStruct);

	/* Differnetial Pressure Sensor */
	GPIO_InitStruct.Pin = MYGPIO_DP_SENSOR_SCL_GPIO_NUM | MYGPIO_DP_SENSOR_SDA_GPIO_NUM;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(MYGPIO_DP_SENSOR_GPIO_PORT, &GPIO_InitStruct);

	// 버스 아이들 상태: 둘 다 High(릴리즈)
    HAL_GPIO_WritePin(MYGPIO_DP_SENSOR_GPIO_PORT, MYGPIO_DP_SENSOR_SCL_GPIO_NUM, GPIO_PIN_SET);
    HAL_GPIO_WritePin(MYGPIO_DP_SENSOR_GPIO_PORT, MYGPIO_DP_SENSOR_SDA_GPIO_NUM, GPIO_PIN_SET);

    /* Button */
    GPIO_InitStruct.Pin = MYGPIO_BTN_UP_GPIO_NUM | MYGPIO_BTN_DOWN_GPIO_NUM | MYGPIO_BTN_SEL_GPIO_NUM;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(MYGPIO_BTN_GPIO_PORT, &GPIO_InitStruct);

	/* Input siginal */
	// Gidong
	GPIO_InitStruct.Pin = MYGPIO_IN_GIDING_GPIO_NUM;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(MYGPIO_IN_GPIO_PORT, &GPIO_InitStruct);

	// Manual
	GPIO_InitStruct.Pin = MYGPIO_IN_MANUAL_GPIO_NUM;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(MYGPIO_IN_M_GPIO_PORT, &GPIO_InitStruct);

	/* Motor limit inputs.  The board uses external contacts/pull-ups. */
	GPIO_InitStruct.Pin = MYGPIO_MOTOR_LIMIT_SW0_GPIO_NUM | MYGPIO_MOTOR_LIMIT_SW1_GPIO_NUM;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(MYGPIO_MOTOR_GPIO_PORT, &GPIO_InitStruct);

#if 0
	/* Motor Ctrl */
	HAL_GPIO_WritePin(MYGPIO_MOTOR_GPIO_PORT, MYGPIO_MOTOR_CTRL_S0_GPIO_NUM | MYGPIO_MOTOR_CTRL_S1_GPIO_NUM, GPIO_PIN_RESET);
	GPIO_InitStruct.Pin = MYGPIO_MOTOR_CTRL_S0_GPIO_NUM | MYGPIO_MOTOR_CTRL_S1_GPIO_NUM;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(MYGPIO_MOTOR_GPIO_PORT, &GPIO_InitStruct);

	HAL_GPIO_WritePin(MYGPIO_MOTOR_GPIO_PORT, MYGPIO_MOTOR_CTRL_S0_GPIO_NUM | MYGPIO_MOTOR_CTRL_S1_GPIO_NUM, GPIO_PIN_RESET);

	/* Motor Limit Switch */
	GPIO_InitStruct.Pin = MYGPIO_MOTOR_LIMIT_SW0_GPIO_NUM | MYGPIO_MOTOR_LIMIT_SW1_GPIO_NUM;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(MYGPIO_MOTOR_GPIO_PORT, &GPIO_InitStruct);

	/* Buzzer */
	HAL_GPIO_WritePin(MYGPIO_BUZZER_GPIO_PORT, MYGPIO_BUZZER_GPIO_NUM, GPIO_PIN_RESET);
	GPIO_InitStruct.Pin = MYGPIO_BUZZER_GPIO_NUM;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(MYGPIO_BUZZER_GPIO_PORT, &GPIO_InitStruct);

	HAL_GPIO_WritePin(MYGPIO_BUZZER_GPIO_PORT, MYGPIO_BUZZER_GPIO_NUM, GPIO_PIN_RESET);
#endif
	/* Relay outputs: PA12 is the PA1/gidong confirmation output and PA11 is
	 * the existing manual/fault relay output.  PB11 remains the manual input. */
	HAL_GPIO_WritePin(MYGPIO_OUT_RELAY_GPIO_PORT,
		MYGPIO_OUT_RELAY_EXT_CHECK_GPIO_NUM | MYGPIO_OUT_RELAY_EXT_FAULT_GPIO_NUM,
		GPIO_PIN_RESET);
	GPIO_InitStruct.Pin = MYGPIO_OUT_RELAY_EXT_CHECK_GPIO_NUM | MYGPIO_OUT_RELAY_EXT_FAULT_GPIO_NUM;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(MYGPIO_OUT_RELAY_GPIO_PORT, &GPIO_InitStruct);

	HAL_GPIO_WritePin(MYGPIO_OUT_RELAY_GPIO_PORT,
		MYGPIO_OUT_RELAY_EXT_CHECK_GPIO_NUM | MYGPIO_OUT_RELAY_EXT_FAULT_GPIO_NUM,
		GPIO_PIN_RESET);

#if BOARD_MOTOR_ENABLE
	/* Keep the VNH7070 control inputs in the no-drive state until TIM1 is
	 * configured.  PB13 is initialized as a GPIO low by MYTIMER_Init(). */
	HAL_GPIO_WritePin(MYGPIO_MOTOR_INA_GPIO_PORT,
		MYGPIO_MOTOR_INA_GPIO_NUM | MYGPIO_MOTOR_INB_GPIO_NUM | MYGPIO_MOTOR_SEL0_GPIO_NUM,
		GPIO_PIN_RESET);
	GPIO_InitStruct.Pin = MYGPIO_MOTOR_INA_GPIO_NUM | MYGPIO_MOTOR_INB_GPIO_NUM |
		MYGPIO_MOTOR_SEL0_GPIO_NUM;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(MYGPIO_MOTOR_INA_GPIO_PORT, &GPIO_InitStruct);
	HAL_GPIO_WritePin(MYGPIO_MOTOR_INA_GPIO_PORT,
		MYGPIO_MOTOR_INA_GPIO_NUM | MYGPIO_MOTOR_INB_GPIO_NUM | MYGPIO_MOTOR_SEL0_GPIO_NUM,
		GPIO_PIN_RESET);
#endif

}

void MYGPIO_Rs485_Eanble( uint8_t _cnEn )
{
	if( MYGPIO_RS485_DISABLE == _cnEn ){
		HAL_GPIO_WritePin( MYGPIO_RS485_ENABLE_GPIO_PORT, MYGPIO_RS485_ENABLE_GPIO_NUM, GPIO_PIN_RESET );
	}
	else{
		HAL_GPIO_WritePin( MYGPIO_RS485_ENABLE_GPIO_PORT, MYGPIO_RS485_ENABLE_GPIO_NUM, GPIO_PIN_SET );
	}
}

void MYGPIO_Led_Control( uint8_t _cnLed, uint8_t _cnCtrl )
{
	GPIO_TypeDef *port = MYGPIO_LED_GPIO_PORT;
	uint16_t npinnum = 0;
	if( LED_SELECT_GIDONG == _cnLed ){
		npinnum = MYGPIO_LED_GIDONG_GPIO_NUM;
	}
	else
	if( LED_SELECT_MANUAL == _cnLed ){
		port = MYGPIO_LED_M_GPIO_PORT;
		npinnum = MYGPIO_LED_MANUAL_GPIO_NUM;
	}
	else{
		// LED_SELECT_POWER
		npinnum = MYGPIO_LED_POWER_GPIO_NUM;
	}
	HAL_GPIO_WritePin( port, npinnum, (GPIO_PinState)_cnCtrl );
}

void MYGPIO_Relay_Control( uint8_t _cnRelay, uint8_t _cnCtrl )
{
	
	if( RELAY_SEL_GIDONG == _cnRelay ){
		HAL_GPIO_WritePin( MYGPIO_OUT_RELAY_GPIO_PORT, MYGPIO_OUT_RELAY_GIDONG_GPIO_NUM, (GPIO_PinState)_cnCtrl );
	}
	else
	if( RELAY_SEL_MANUAL == _cnRelay ){
		HAL_GPIO_WritePin( MYGPIO_OUT_RELAY_GPIO_PORT, MYGPIO_OUT_RELAY_MANUAL_GPIO_NUM, (GPIO_PinState)_cnCtrl );
	}
	else{
		// SKIP
	}
	
}

uint8_t MYGPIO_Get_Gidong_State( void )
{
	uint8_t cnret = 0;

	if( 0 == HAL_GPIO_ReadPin( MYGPIO_IN_GPIO_PORT, MYGPIO_IN_GIDING_GPIO_NUM ) ){
		cnret = 1;
	}
	return cnret;
}

uint8_t MYGPIO_Get_Manual_State( void )
{
	uint8_t cnret = 0;

	if( 0 == HAL_GPIO_ReadPin( MYGPIO_IN_M_GPIO_PORT, MYGPIO_IN_MANUAL_GPIO_NUM ) ){
		cnret = 1;
	}
	return cnret;
}

uint8_t MYGPIO_Get_LimitSwitch_State( uint8_t _cnIdx )
{
	uint8_t cnret = 0;
	uint16_t cnpinnum = 0;

	if( LIMIT_SWITCH_SEL_0 == _cnIdx ){
		cnpinnum = MYGPIO_MOTOR_LIMIT_SW0_GPIO_NUM;
	}
	else{
		cnpinnum = MYGPIO_MOTOR_LIMIT_SW1_GPIO_NUM;
	}

	if( 0 == HAL_GPIO_ReadPin( MYGPIO_MOTOR_GPIO_PORT, cnpinnum ) ){
		cnret = 1;
	}
	return cnret; // 	HAL_GPIO_ReadPin( MYGPIO_MOTOR_GPIO_PORT, cnpinnum );
}

void MYGPIO_Run_Buzzer( uint8_t _cnBuzzerType )
{
	if( BUZZER_PLAY_TYPE_BEEP == _cnBuzzerType ){
		gnBuzzerTimer = 50;
	}
	else{
		if( 0 == gcnBuzzer_Act_f ){
			gcnBuzzer_Act_f = 1;
			gnBuzzerTimer = gnBuzzer_Run_Time;
		}
	}
}

void MYGPIO_Motor_Control( uint8_t _cnDirection, uint8_t _cnSpeed )
{
	// if( MOTOR_DIRECTION_VCC_GND_FORWARD == _cnDirection ){
	// 	HAL_GPIO_WritePin( MYGPIO_MOTOR_GPIO_PORT, MYGPIO_MOTOR_CTRL_S0_GPIO_NUM, GPIO_PIN_RESET );
	// 	MYPWM_Set_Duty( MOTOR_PWM_CHANNEL_M1, MOTOR_PWM_SPEED_STOP );
	// 	MYPWM_Set_Duty( MOTOR_PWM_CHANNEL_M0, _cnSpeed );
	// 	HAL_Delay(1);
	// 	HAL_GPIO_WritePin( MYGPIO_MOTOR_GPIO_PORT, MYGPIO_MOTOR_CTRL_S1_GPIO_NUM, GPIO_PIN_SET );
	// }
	// else
	// if( MOTRO_DIRECTION_GND_VCC_REVERSE == _cnDirection ){
	// 	HAL_GPIO_WritePin( MYGPIO_MOTOR_GPIO_PORT, MYGPIO_MOTOR_CTRL_S1_GPIO_NUM, GPIO_PIN_RESET );
	// 	MYPWM_Set_Duty( MOTOR_PWM_CHANNEL_M0, MOTOR_PWM_SPEED_STOP );
	// 	MYPWM_Set_Duty( MOTOR_PWM_CHANNEL_M1, _cnSpeed );
	// 	HAL_Delay(1);
	// 	HAL_GPIO_WritePin( MYGPIO_MOTOR_GPIO_PORT, MYGPIO_MOTOR_CTRL_S0_GPIO_NUM, GPIO_PIN_SET );
	// }
	// else{
	// 	// MOTOR_DIRECTION_GND_GND_OFF
	// 	HAL_GPIO_WritePin( MYGPIO_MOTOR_GPIO_PORT, MYGPIO_MOTOR_CTRL_S0_GPIO_NUM, GPIO_PIN_RESET );
	// 	HAL_GPIO_WritePin( MYGPIO_MOTOR_GPIO_PORT, MYGPIO_MOTOR_CTRL_S1_GPIO_NUM, GPIO_PIN_RESET );
	// 	MYPWM_Set_Duty( MOTOR_PWM_CHANNEL_M0, MOTOR_PWM_SPEED_STOP );
	// 	MYPWM_Set_Duty( MOTOR_PWM_CHANNEL_M1, MOTOR_PWM_SPEED_STOP );
	// }
}

void MYGPIO_Motor_Apply( int _nDirection, float _nDuty )
{
#if BOARD_MOTOR_ENABLE
	static int last_hw_direction = 0;
	int hw_direction = _nDirection;

	if ((0 == _nDirection) || (_nDuty <= 0.0f)) {
		/* PWM must be low before changing INA/INB. */
		MYPWM_Set_Duty(MOTOR_PWM_CHANNEL_M0, 0.0f);
		HAL_GPIO_WritePin(MYGPIO_MOTOR_INA_GPIO_PORT, MYGPIO_MOTOR_INA_GPIO_NUM, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(MYGPIO_MOTOR_INB_GPIO_PORT, MYGPIO_MOTOR_INB_GPIO_NUM, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(MYGPIO_MOTOR_SEL0_GPIO_PORT, MYGPIO_MOTOR_SEL0_GPIO_NUM, GPIO_PIN_RESET);
		last_hw_direction = 0;
		return;
	}

	/* Preserve the existing NO/NC electrical inversion exactly once. */
	if (MOTOR_STATE_NORMAL_CLOSE != gnMotorNonc) {
		hw_direction = -hw_direction;
	}

	if (hw_direction != last_hw_direction) {
		MYPWM_Set_Duty(MOTOR_PWM_CHANNEL_M0, 0.0f);
		HAL_GPIO_WritePin(MYGPIO_MOTOR_INA_GPIO_PORT, MYGPIO_MOTOR_INA_GPIO_NUM,
			(hw_direction > 0) ? GPIO_PIN_SET : GPIO_PIN_RESET);
		HAL_GPIO_WritePin(MYGPIO_MOTOR_INB_GPIO_PORT, MYGPIO_MOTOR_INB_GPIO_NUM,
			(hw_direction > 0) ? GPIO_PIN_RESET : GPIO_PIN_SET);
		/* Select CS feedback for the high-side switch actually driving. */
		HAL_GPIO_WritePin(MYGPIO_MOTOR_SEL0_GPIO_PORT,
			MYGPIO_MOTOR_SEL0_GPIO_NUM,
			(hw_direction > 0) ? GPIO_PIN_SET : GPIO_PIN_RESET);
		/* VNH7070BAS input-to-PWM setup guard.  At 48 MHz this conservative
		 * NOP loop is longer than the specified 20 us; it runs only on a
		 * direction transition, never in the normal ramp update. */
		for (volatile uint32_t delay = 0U; delay < 300U; ++delay) {
			__NOP();
		}
		last_hw_direction = hw_direction;
	}

	MYPWM_Set_Duty(MOTOR_PWM_CHANNEL_M0, _nDuty);
#else
	(void)_nDirection;
	(void)_nDuty;
#endif
}

void MYGPIO_Motor_Stop( void )
{
#if BOARD_MOTOR_ENABLE
	MYGPIO_Motor_Apply(0, 0.0f);
#endif
}

void MYGPIO_Stop_Buzzer( void )
{
	gcnBuzzer_Act_f = 0;
}

void MYGPIO_Proc( void )
{
	_mygpio_buzzer_proc();
}

/////////////////////////////////////////////////////////////////////////////////////////////
// local function implement
void _mygpio_buzzer_proc( void )
{
#if 0
	if( 0 == gnBuzzerTimer ){
		// Stop Buzzer
		if( 1 == HAL_GPIO_ReadPin( MYGPIO_BUZZER_GPIO_PORT, MYGPIO_BUZZER_GPIO_NUM ) ){
			HAL_GPIO_WritePin( MYGPIO_BUZZER_GPIO_PORT, MYGPIO_BUZZER_GPIO_NUM, PIN_STATE_OFF );
		}
	}
	else{
		// Play Buzzer
		if( 0 == HAL_GPIO_ReadPin( MYGPIO_BUZZER_GPIO_PORT, MYGPIO_BUZZER_GPIO_NUM ) ){
			HAL_GPIO_WritePin( MYGPIO_BUZZER_GPIO_PORT, MYGPIO_BUZZER_GPIO_NUM, PIN_STATE_ON );
		}

	}
#endif
}
