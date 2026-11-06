#include "global.h"
#include "mygpio.h"

/////////////////////////////////////////////////////////////////////////////////////////////
// import function prototype


/////////////////////////////////////////////////////////////////////////////////////////////
// local function prototype


/////////////////////////////////////////////////////////////////////////////////////////////
// global variable

// Timer
int gnTestTimer = 0;
int gnBootTimer = BOOT_LOGO_TIME_MS;
int gnGuiTimer = 0;
int gnGuiReturnTimer = 0;
int gnDpSensorTimer = 0;
int gnBtnTimer = 0;
int gnFndBlinkTimer = 0;
int gnBuzzerTimer = 0;
int gnMotorTimer = 0;
int gnGuiSensorTimer = 0;
int gnMotorHoldTimer = 0;
int gnMotorFixedSpeedTimer = 0;
int gnSaveEnvDelayTimer = 0;
int gnModbusTxTimer = 0;
int gnMotorDeadbandTick = 0;
int gnMotorLimitSwStablTimer = 0;
int gnUartCheckTimer = 0;
// Display
uint8_t gcnError_msg = 0;

// BUtton
uint8_t gcnBtnState = 0;

// Modbus
uint8_t gcnDeviceId = 0;
uint8_t gcnRs485_Baudrate = 0;

// Flash
uint8_t gcnSaveEnv_f = 0;

// DP Sensor
float gnfZeroOffset = 0.0f;
uint8_t gcnSensor_Offset_Sign = 0;
int gnSensor_Offset = 11;
int gnSensor_xTimes = 22;
// int32_t gnCorrectValue = 0;
float gnfLpfPaValue = 0.0f;
int32_t gnDispSensorValue = 0;
float gnfTargetDpValue = 50.0f;

// Current Sensor
float gnfMotorCurretValue = 0.0f;

// Buzzer
int gnBuzzer_Run_Time = 1000;
uint8_t gcnBuzzer_Act_f = 0;

// Motor state
uint16_t gnMotorNonc = MOTOR_STATE_NORMAL_CLOSE;
uint8_t gcnMotorOverCurrent_f = 0;
float gnfNormalUpperRamp = 0.0f;
float gnfNormalDwonRamp = 0.0f;
float gnfSettledBand_Pa = 0.0f;
float gnfSettleBandUpperRamp = 0.0f;
float gnfSettleBandDownRamp = 0.0f;
float gnfDeadBand_Pa = 0.0f;
uint16_t gnDeadBand_Period = 0;
uint16_t gnDeadBand_UpperTick = 0;
uint16_t gnDeadBand_DownTick = 0;
float gnfMotorMinUpperSpeed = 0.0f;
float gnfMotorMinDownSpeed = 0.0f;
float gnfMotorMaxUpperSpeed = 0.0f;
float gnfMotorMaxDownSpeed = 0.0f;
float gnfMotorPid_P = 0.0f;
float gnfMotorPid_I = 0.0f;
float gnfMotorPid_D = 0.0f;
uint16_t gnLimitBlankTime_ms = 0;
float gnfDeadBand_Tick_Min_Pa = 0.0f;
// Event
uint8_t gcnFireEvent_f = 0;

// Uart
uint8_t gabRS485RxBuff[RS485_BUFF_SIZE] = {0,};
uint16_t gnRS485RxOffset = 0;
uint16_t gnRS485RxBuffSize = 0;
uint8_t gabRS485TxBuff[RS485_BUFF_SIZE] = {0,};
uint16_t gnRS485TxBuffSize = 0;
uint8_t gnRS485Rx_f = 0;

uint8_t gabRS485TxDmaBuf[RS485_TX_DMA_BUFF_SIZE] = {0,};
uint8_t gabRS485TxRingBuf[RS485_TX_RING_BUFF_SIZE] = {0,};


/////////////////////////////////////////////////////////////////////////////////////////////
// public and export function implement
void DBG_Hex_Print( char* _pszTag, uint8_t* _abData, uint16_t _nSize )
{
	int i = 0;
	MYUTIL_Printf("[%s] - %dbyte\r\n", _pszTag, _nSize );
	for( i = 1; i < ( _nSize + 1 ); i++ )
	{
		if( 0 == ( _nSize % 16 ) ){
			MYUTIL_Printf("\r\n");
		}
		MYUTIL_Printf("%02x ", _abData[i-1] );
	}
	MYUTIL_Printf("\r\n");
}
/////////////////////////////////////////////////////////////////////////////////////////////
// local function implement
