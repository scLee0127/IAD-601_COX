#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#include <stdio.h>
#include "main.h"

#define FIRMWARE_MAJOR_NUM  			1
#define FIRMWARE_MINOR_NUM  			0

// #define BOARD_SELECT_OLD_VERSION  		1
#define BOARD_DEBUG_MSG_ENABLE 			1


#ifdef BOARD_DEBUG_MSG_ENABLE 
#define MYUTIL_Printf( fmt, args... ) 		printf( fmt, ##args )
#else
#define MYUTIL_Printf( fmt, args... )
#endif

/* Set to 1 to restore motor control and PWM output. */
#ifndef BOARD_MOTOR_ENABLE
#define BOARD_MOTOR_ENABLE 0
#endif

// Timer
#define BOOT_LOGO_TIME_MS  				2000
#define GUI_REFRESH_TIME_MS  			500
extern int gnTestTimer;
extern int gnBootTimer;
extern int gnGuiTimer;
extern int gnGuiReturnTimer;
extern int gnDpSensorTimer;
extern int gnBtnTimer;
extern int gnFndBlinkTimer;
extern int gnBuzzerTimer;
extern int gnMotorTimer;
extern int gnGuiSensorTimer;
extern int gnMotorHoldTimer;
extern int gnMotorFixedSpeedTimer;
#define ENV_SAVE_FLASH_TIME_MS  		5000
extern int gnSaveEnvDelayTimer;
#define MODBUS_TX_DELAY_TIME_MS 		5
extern int gnModbusTxTimer;
extern int gnMotorDeadbandTick;
extern int gnMotorLimitSwStablTimer;
extern int gnUartCheckTimer;

// Display
extern uint8_t gcnError_msg;

// Button
extern uint8_t gcnBtnState;

// Modbus
extern uint8_t gcnDeviceId;
extern uint8_t gcnRs485_Baudrate;

// Flash
extern uint8_t gcnSaveEnv_f;

// DP Sensor
extern float gnfZeroOffset;
extern int gnSensor_Offset;
extern uint8_t gcnSensor_Offset_Sign;
extern int gnSensor_xTimes;
// extern int32_t gnCorrectValue;
extern float gnfLpfPaValue;
extern int32_t gnDispSensorValue;
extern float gnfTargetDpValue;

// Motor Current sensor
extern float gnfMotorCurretValue;
extern float gnfNormalUpperRamp;
extern float gnfNormalDwonRamp;
extern float gnfSettledBand_Pa;
extern float gnfSettleBandUpperRamp;
extern float gnfSettleBandDownRamp;
extern float gnfDeadBand_Pa;
extern uint16_t gnDeadBand_Period;
extern uint16_t gnDeadBand_UpperTick;
extern uint16_t gnDeadBand_DownTick;
extern float gnfMotorMinUpperSpeed;
extern float gnfMotorMinDownSpeed;
extern float gnfMotorMaxUpperSpeed;
extern float gnfMotorMaxDownSpeed;
extern float gnfMotorPid_P;
extern float gnfMotorPid_I;
extern float gnfMotorPid_D;
extern uint16_t gnLimitBlankTime_ms;
extern float gnfDeadBand_Tick_Min_Pa;

// Buzzer
extern int gnBuzzer_Run_Time;
extern uint8_t gcnBuzzer_Act_f;

// Event
extern uint8_t gcnFireEvent_f;

// LED State
#define LED_SELECT_POWER  		0
#define LED_SELECT_GIDONG  		1
#define LED_SELECT_MANUAL  		2

#define LED_CTRL_STATE_OFF  	0
#define LED_CTRL_STATE_ON   	1

// Motor Limit Switch
#define LIMIT_SWITCH_SEL_0 		0
#define LIMIT_SWITCH_SEL_1 		1

// Motor GND Ctrl
#define MOTOR_DIRECTION_GND_GND_OFF		0
#define MOTOR_DIRECTION_VCC_GND_FORWARD 1
#define MOTRO_DIRECTION_GND_VCC_REVERSE 2
#define MOTOR_PWM_CHANNEL_M0  			0
#define MOTOR_PWM_CHANNEL_M1  			1
#define MOTOR_PWM_SPEED_STOP 			0
#define MOTOR_PWM_SPEED_LOW  			50
#define MOTOR_PWM_SPEED_MIDDLE 			75
#define MOTOR_PWM_SPEED_HIGH 			100

// Motor state
#define MOTOR_STATE_NORMAL_CLOSE  		0
#define MOTOR_STATE_NORMAL_OPEN  		1
extern uint8_t gcnMotorOverCurrent_f;
extern uint16_t gnMotorNonc;

// Cotnrol State
#define PIN_STATE_ON 			1
#define PIN_STATE_OFF 			0

// Buzzer
#define BUZZER_PLAY_TYPE_BEEP  	0
#define BUZZER_PLAY_TYPE_SIGNAL 1

// RELAY
#define RELAY_SEL_GIDONG 		0
#define RELAY_SEL_MANUAL 		1

// RS485
#define RS485_BUFF_SIZE 		128
extern uint8_t gabRS485RxBuff[RS485_BUFF_SIZE];
extern uint16_t gnRS485RxOffset;
extern uint16_t gnRS485RxBuffSize;
extern uint8_t gnRS485Rx_f;
extern uint8_t gabRS485TxBuff[RS485_BUFF_SIZE];
extern uint16_t gnRS485TxBuffSize;

#define RS485_TX_DMA_BUFF_SIZE            512
extern uint8_t gabRS485TxDmaBuf[RS485_TX_DMA_BUFF_SIZE];
#define RS485_TX_RING_BUFF_SIZE           1024
extern uint8_t gabRS485TxRingBuf[RS485_TX_RING_BUFF_SIZE];

void DBG_Hex_Print( char* _pszTag, uint8_t* _abData, uint16_t _nSize );
#endif // __GLOBAL_H__
