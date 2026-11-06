#if 1
#include <string.h>

#include "global.h"
#include "mymodbus.h"
#include "myuart.h"
#include "mymotor.h"
#include "myadc.h"

/////////////////////////////////////////////////////////////////////////////////////////////
// import function prototype


/////////////////////////////////////////////////////////////////////////////////////////////
// local function prototype
void _mymodbus_buffer_clear( void );
uint16_t _modbus_crc16( uint8_t* _pabIn, uint16_t _nLen );
uint16_t _modbus_delta_crc16( uint8_t* _pabIn, uint16_t _nLen );
void _create_packet( void );
uint8_t _modbus_check_packet( uint8_t* _pbData, uint16_t nLen );
uint8_t _modbus_get_hold_register( uint8_t* _pbData, uint16_t nLen );
uint8_t _modbus_set_hold_register( uint8_t* _pbData, uint16_t nLen );
void _modbus_update_data( void );
/////////////////////////////////////////////////////////////////////////////////////////////
// global variable
#define MODBUS_HOLDING_FUNC_CODE_READ 		0x03
#define MODBUS_INPUT_FUNC_CODE_READ 		0x04
#define MODBUS_HOLDING_FUNC_CODE_WRITE 		0x06

#define DATA_RATIO_LIMIT_MOTOR_CURRENT  		(float)10.0f
#define DATA_RATIO_PID  				 		(float)100.0f
#define DATA_RATIO_CURRENT  			 		(float)100.0f
#define DATA_RATIO_MOTOR_PWM			 		(float)1000.0f

typedef struct stMODBUS_DATA{
	/* uint16_t nDeviceId; */
	uint16_t nCfgOverCurrent;
	uint16_t nCfgTaget_Pa;
	uint16_t nCfgPid_P;
	uint16_t nCfgPid_I;
	uint16_t nCfgPid_D;
	uint16_t nCfgPid_Ext_P;
	uint16_t nCfgLimitChkTime_ms;
	uint16_t nCfgDiffPa_Cali_Offset;
	uint16_t nCfgStopTime;
	uint16_t nCfgWrongOpenClose;
	uint16_t nCommand;
	uint16_t nStDiff_Pa;
	uint16_t nStCurrnet;
	uint16_t nStDeviceStatus;
	uint16_t nStMotorPwm;
	uint16_t nSafety_Pa;
	uint16_t nSafety_Up_Ramp;
	uint16_t nSafety_Down_Ramp;
	uint16_t nDeadband_Pa;
	uint16_t nDeadband_Min_Pa;
	uint16_t nDeadband_Peiod;
	uint16_t nDeadband_Upper_Tick;
	uint16_t nDeadband_Down_Tick;
	uint16_t nMotor_Min_Up_Spd;
	uint16_t nMotor_Min_Down_Spd;
	uint16_t nMotor_Max_Up_Spd;
	uint16_t nMotor_Max_Down_Spd;
	uint16_t nNormal_Up_Ramp;
	uint16_t nNormal_Down_Ramp;
	uint16_t nMotor_NoNc;
}MODBUS_DATA;

enum eMODBUS_REG{
	MB_CFG_OVERCURRENT = 0,
	MB_CFG_TARGET_PA,
	MB_CFG_PID_P,
	MB_CFG_PID_I,
	MB_CFG_PID_D,
	MB_CFG_PID_EXT_P,
	MB_CFG_LIMIT_CHK_TIME_MS,
	MB_CFG_DIFF_PA_CALIBRATION_OFFSET,
	MB_CFG_STOP_TIME_MS,
	MB_CFG_WRONG_OPEN_CLOSE,
	MB_COMMAND,
	MB_ST_DIFF_PA_NONE,
	MB_ST_CURRENT_NONE,
	MB_ST_DEVICE_STATUS_NONE,
	MB_ST_MOTOR_PWM_NONE,
	MB_CFG_SAFETY_PA,
	MB_CFG_SAFETY_UP_RAMP,
	MB_CFG_SAFETY_DOWM_RAMP,
	MB_CFG_DEADBAND_PA,
	MB_CFG_DEADBAND_MIN_PA,
	MB_CFG_DEADBAND_PERIOD,
	MB_CFG_DEADBAND_UPPER_TICK,
	MB_CFG_DEADBAND_DOWN_TICK,
	MB_CFG_MOTOR_MIN_UP_SPEED,
	MB_CFG_MOTOR_MIN_DOWN_SPEED,
	MB_CFG_MOTOR_MAX_UP_SPEED,
	MB_CFG_MOTOR_MAX_DOWN_SPEED,
	MB_CFG_NORMAL_UP_RAMP,
	MB_CFG_NORMAL_DOWM_RAMP,
	MB_CFG_MOTOR_NONC,
	MB_IDX_MAX
};

MODBUS_DATA gstModbusData;
/////////////////////////////////////////////////////////////////////////////////////////////
// public and export function implement
void MYMODBUS_Proc( void )
{
	uint8_t cncode = 0;
	uint8_t cnres = 0;

	if( 1 == gnRS485Rx_f ){
		gnRS485Rx_f = 0;
		// MYUTIL_Printf("size %d %d\r\n", gnRS485RxBuffSize, gnRS485RxOffset );
		if( 0 == _modbus_check_packet( gabRS485RxBuff, gnRS485RxBuffSize ) ){
			_modbus_update_data();
			cncode = gabRS485RxBuff[1];
			// printf("----------------------------------------\r\n");
			// DBG_Hex_Print((char*)"MODBUS[R]", gabRS485RxBuff, gnRS485RxBuffSize );
			// printf("----------------------------------------\r\n");
			if( ( MODBUS_HOLDING_FUNC_CODE_READ == cncode ) || ( MODBUS_INPUT_FUNC_CODE_READ == cncode ) ){
				cnres = _modbus_get_hold_register( gabRS485RxBuff, gnRS485RxBuffSize );
			}
			else
			if( MODBUS_HOLDING_FUNC_CODE_WRITE == cncode ){
				cnres = _modbus_set_hold_register( gabRS485RxBuff, gnRS485RxBuffSize );
			}
			
			// _create_packet();
			if( 0 == cnres ){
				// DBG_Hex_Print((char*)"MODBUS[S1]", gabRS485TxBuff, gnRS485TxBuffSize );
				MYUART_Send_Data_DMA( &UartHandler_Rs486, gabRS485TxBuff, gnRS485TxBuffSize );
			}
		}
		
		_mymodbus_buffer_clear();
	}
}

void MYMODBUS_Update_Cfg( DEVICEENV* _pstDeviceEnv )
{
	// gstModbusData.nDeviceId = (uint8_t)_pstDeviceEnv->nDeviceID;
	
	gstModbusData.nCfgOverCurrent = _pstDeviceEnv->nOverCurrent;
	gstModbusData.nCfgTaget_Pa = _pstDeviceEnv->nTargetPa;
	gstModbusData.nCfgPid_P = _pstDeviceEnv->nPid_P;
	gstModbusData.nCfgPid_I = _pstDeviceEnv->nPid_I;
	gstModbusData.nCfgPid_D = _pstDeviceEnv->nPid_D;
	gstModbusData.nCfgPid_Ext_P = 0;
	gstModbusData.nCfgLimitChkTime_ms = 800;
	gstModbusData.nCfgDiffPa_Cali_Offset = 900;
	gstModbusData.nCfgStopTime = 20;
	gstModbusData.nCfgWrongOpenClose = 0;
	gstModbusData.nCommand = 0;
	gstModbusData.nSafety_Pa = _pstDeviceEnv->nSafetyPa;
	gstModbusData.nSafety_Up_Ramp = _pstDeviceEnv->nSafetyUpperRamp;
	gstModbusData.nSafety_Down_Ramp = _pstDeviceEnv->nSafetyDownRamp;
	gstModbusData.nDeadband_Pa = _pstDeviceEnv->nDeadbandPa;
	gstModbusData.nDeadband_Min_Pa = _pstDeviceEnv->nDeadBandTickMinPa;
	gstModbusData.nDeadband_Peiod = _pstDeviceEnv->nDeadbandPeriod;
	gstModbusData.nDeadband_Upper_Tick = _pstDeviceEnv->nDeadbandUpperTick;
	gstModbusData.nDeadband_Down_Tick = _pstDeviceEnv->nDeadbandDownTick;
	gstModbusData.nMotor_Min_Up_Spd = _pstDeviceEnv->nMinUpperSpeed;
	gstModbusData.nMotor_Min_Down_Spd = _pstDeviceEnv->nMinDownSpeed;
	gstModbusData.nMotor_Max_Up_Spd = _pstDeviceEnv->nMaxUpperSpeed;
	gstModbusData.nMotor_Max_Down_Spd = _pstDeviceEnv->nMaxDownSpeed;
	gstModbusData.nNormal_Up_Ramp = _pstDeviceEnv->nNormalUpperRamp;
	gstModbusData.nNormal_Down_Ramp = _pstDeviceEnv->nNormalDownRamp;
	gstModbusData.nMotor_NoNc = _pstDeviceEnv->nMotorNoNc;
}

void _modbus_update_data( void )
{
	float nftmp = 0.0f;
	uint16_t ntmp = 0;
	gstModbusData.nStDiff_Pa = (int16_t)gnfLpfPaValue;
	nftmp = ADC0_Get_Current();
	gstModbusData.nStCurrnet = (uint16_t)( nftmp * DATA_RATIO_CURRENT );
	ntmp = (MYMOTOR_Get_Open_LimitSwtich_St() << 8);
	ntmp |= (MYMOTOR_Get_Close_LimitSwtich_St() << 0);
	gstModbusData.nStDeviceStatus = ntmp;
	nftmp = MYMOTOR_Get_Motor_Pwm();
	gstModbusData.nStMotorPwm = (uint16_t)( nftmp * DATA_RATIO_MOTOR_PWM );
}

/////////////////////////////////////////////////////////////////////////////////////////////
// local function implement
uint8_t _modbus_check_packet( uint8_t* _pbData, uint16_t nLen )
{
	uint8_t cnret = 0;
	uint16_t cncrc = 0;
	const uint8_t cnAddr = gcnDeviceId;

	do{
		if( 4 > nLen ){
			MYUTIL_Printf("Wrong data size %d\r\n", nLen );
			cnret = 1;
			break;
		}

		// Check Modbus Addr
		if( cnAddr != _pbData[0] ){
			MYUTIL_Printf("Wrong Addr %02x\r\n", _pbData[0] );
			cnret = 2;
			break;
		}

		// cncrc = _modbus_crc16( _pbData, nLen - 2 );
		cncrc = ( (uint16_t)( _pbData[nLen-2] << 8 ) & 0xff00 ) | ( (uint16_t)_pbData[nLen-1] & 0x00ff );
		if( cncrc != _modbus_crc16( _pbData, nLen - 2 ) ){
			DBG_Hex_Print((char*)"_modbus_check_packet", _pbData, nLen );
			printf("Wrong CRC check: %04x %04x\r\n", cncrc, _modbus_crc16( _pbData, nLen - 2 ) );
			cnret = 3;
			break;
		}
	}while( 0 );

	return cnret;
}

uint8_t _modbus_get_hold_register( uint8_t* _pbData, uint16_t nLen )
{
	uint8_t cnret = 0;
	uint16_t ncrc = 0;
	int16_t noffset = 0;
	uint16_t nval = 0;
	int i;
	uint16_t* ptr = (uint16_t*)&gstModbusData;
	uint8_t nlimitcount = sizeof( MODBUS_DATA ) / sizeof( uint16_t );
	uint16_t nstart_addr = ( (uint16_t)_pbData[2] << 8 ) | _pbData[3];
	uint16_t ndata_count = ( (uint16_t)_pbData[4] << 8 ) | _pbData[5];

	if( ( 0 == ndata_count ) || ( MB_IDX_MAX  < ndata_count ) ){
		MYUTIL_Printf("Wrong 0x03 data count [%d]\r\n", ndata_count );
		return 1;
	}

	if( ( nstart_addr + ndata_count ) > nlimitcount ){
		MYUTIL_Printf("0x03 out of range start:%d, cnt:%d, limit:%d\r\n", nstart_addr, ndata_count, nlimitcount );
		return 1;
	}

	memset( gabRS485TxBuff, 0, sizeof( gabRS485TxBuff ) );
	gabRS485TxBuff[noffset] = _pbData[0];  				noffset += 1;
	gabRS485TxBuff[noffset] = _pbData[1];  				noffset += 1;
	gabRS485TxBuff[noffset] = ndata_count * 2; 			noffset += 1;

	for( i = 0; i < ndata_count; i ++ )
	{
		nval = ptr[nstart_addr + i];
		gabRS485TxBuff[noffset] = (uint8_t)( ( nval >> 8 ) & 0xff);  	noffset += 1;
		gabRS485TxBuff[noffset] = (uint8_t)( ( nval >> 0 ) & 0xff);  	noffset += 1;
	}
	ncrc = _modbus_crc16( gabRS485TxBuff, noffset );
	gabRS485TxBuff[noffset] = (uint8_t)( ( ncrc >> 8 ) & 0xff);  		noffset += 1;
	gabRS485TxBuff[noffset] = (uint8_t)( ( ncrc >> 0 ) & 0xff);  		noffset += 1;
	gnRS485TxBuffSize = noffset;
	
	// DBG_Hex_Print((char*)"MODBUS[S:0x03]", gabRS485TxBuff, gnRS485TxBuffSize );
	return cnret;
}

// void _modbus_update_data( void )
// {
// 	float nftmp = 0.0f;
// 	// gstModbusData.nDeviceId = 2;
// 	gstModbusData.nCfgOverCurrent = (uint16_t)( gnfMotorCurretValue * DATA_RATIO_LIMIT_MOTOR_CURRENT );
// 	gstModbusData.nCfgTaget_Pa = (uint16_t)gnfTargetDpValue;
// 	gstModbusData.nCfgPid_P = (uint16_t)( gnfMotorPid_P * DATA_RATIO_PID );
// 	gstModbusData.nCfgPid_I = (uint16_t)( gnfMotorPid_I * DATA_RATIO_PID );
// 	gstModbusData.nCfgPid_D = (uint16_t)( gnfMotorPid_D * DATA_RATIO_PID );
// 	gstModbusData.nCfgPid_Ext_P = 0;
// 	gstModbusData.nCfgLimitChkTime_ms = 800;
// 	gstModbusData.nCfgDiffPa_Cali_Offset = 900;
// 	gstModbusData.nCfgStopTime = 20;
// 	gstModbusData.nCfgWrongOpenClose = 0;
// 	gstModbusData.nCommand = 0;
// 	gstModbusData.nStDiff_Pa = (int16_t)gnCorrectValue;
// 	nftmp = ADC0_Get_Current();
// 	gstModbusData.nStCurrnet = (uint16_t)( nftmp * DATA_RATIO_CURRENT );
// 	gstModbusData.nStDeviceStatus = 0;
// 	nftmp = MYMOTOR_Get_Motor_Pwm();
// 	gstModbusData.nStMotorPwm = (uint16_t)( nftmp * DATA_RATIO_MOTOR_PWM );
// }
uint8_t _modbus_set_hold_register( uint8_t* _pbData, uint16_t nLen )
{
	uint8_t cnret = 0;
	uint16_t ncrc = 0;
	int16_t noffset = 0;
	uint8_t cnchange_f = 0;
	// float nftmp = 0;
	uint16_t nstart_addr = ( (uint16_t)_pbData[2] << 8 ) | _pbData[3];
	uint16_t ndata_val = ( (uint16_t)_pbData[4] << 8 ) | _pbData[5];
	uint16_t* ptr = (uint16_t*)&gstModbusData;
	uint8_t nlimitcount = sizeof( MODBUS_DATA ) / sizeof( uint16_t );

	if( nstart_addr > MB_IDX_MAX ){
		MYUTIL_Printf("0x06 out of range addr:%d, limit:%d\r\n", nstart_addr, nlimitcount );
		return 1;
	}
	ptr[nstart_addr] = ndata_val;

	ENV_Backup_To_Temperary();
	DEVICEENV* pstDevEnv_Tmp = Env_Get_Enviroment_Temperary();

	if( MB_CFG_OVERCURRENT == nstart_addr ){
		// nftmp = (float)ndata_val / DATA_RATIO_LIMIT_MOTOR_CURRENT;
		if( ndata_val != pstDevEnv_Tmp->nOverCurrent ){
			pstDevEnv_Tmp->nOverCurrent = ndata_val;
			cnchange_f = 1;
		}
	}
	else
	if( MB_CFG_TARGET_PA == nstart_addr ){
		// nftmp = (float)ndata_val;
		if( ndata_val != pstDevEnv_Tmp->nTargetPa ){
			pstDevEnv_Tmp->nTargetPa = ndata_val;
			cnchange_f = 1;
		}
	}
	else
	if( MB_CFG_PID_P == nstart_addr ){
		// nftmp = (float)ndata_val / DATA_RATIO_PID;
		if( ndata_val != pstDevEnv_Tmp->nPid_P ){
			pstDevEnv_Tmp->nPid_P = ndata_val;
			cnchange_f = 1;
		}
	}
	else
	if( MB_CFG_PID_I == nstart_addr ){
		// nftmp = (float)ndata_val / DATA_RATIO_PID;
		if( ndata_val != pstDevEnv_Tmp->nPid_I ){
			pstDevEnv_Tmp->nPid_I = ndata_val;
			cnchange_f = 1;
		}
	}
	else
	if( MB_CFG_PID_D == nstart_addr ){
		// nftmp = (float)ndata_val / DATA_RATIO_PID;
		if( ndata_val != pstDevEnv_Tmp->nPid_D ){
			pstDevEnv_Tmp->nPid_D = ndata_val;
			cnchange_f = 1;
		}
	}
	else
	if( MB_CFG_PID_EXT_P == nstart_addr ){
		// SKIP
	}
	else
	if( MB_CFG_LIMIT_CHK_TIME_MS == nstart_addr ){
		// SKIP
	}
	else
	if( MB_CFG_DIFF_PA_CALIBRATION_OFFSET == nstart_addr ){
		// SKIP
	}
	else
	if( MB_CFG_STOP_TIME_MS == nstart_addr ){
		// SKIP
	}
	else
	if( MB_CFG_WRONG_OPEN_CLOSE == nstart_addr ){
		// SKIP
	}
	else
	if( MB_COMMAND == nstart_addr ){
		// SKIP
	}
	else
	if( MB_CFG_SAFETY_PA == nstart_addr ){
		if( ndata_val != pstDevEnv_Tmp->nSafetyPa ){
			pstDevEnv_Tmp->nSafetyPa = ndata_val;
			cnchange_f = 1;
		}
	}
	else
	if( MB_CFG_SAFETY_UP_RAMP == nstart_addr ){
		if( ndata_val != pstDevEnv_Tmp->nSafetyUpperRamp ){
			pstDevEnv_Tmp->nSafetyUpperRamp = ndata_val;
			cnchange_f = 1;
		}
	}
	else
	if( MB_CFG_SAFETY_DOWM_RAMP == nstart_addr ){
		if( ndata_val != pstDevEnv_Tmp->nSafetyDownRamp ){
			pstDevEnv_Tmp->nSafetyDownRamp = ndata_val;
			cnchange_f = 1;
		}
	}
	else
	if( MB_CFG_DEADBAND_PA == nstart_addr ){
		if( ndata_val != pstDevEnv_Tmp->nDeadbandPa ){
			pstDevEnv_Tmp->nDeadbandPa = ndata_val;
			cnchange_f = 1;
		}
	}
	else
	if( MB_CFG_DEADBAND_MIN_PA == nstart_addr ){
		if( ndata_val != pstDevEnv_Tmp->nDeadBandTickMinPa ){
			pstDevEnv_Tmp->nDeadBandTickMinPa = ndata_val;
			cnchange_f = 1;
		}
	}
	else
	if( MB_CFG_DEADBAND_PERIOD == nstart_addr ){
		if( ndata_val != pstDevEnv_Tmp->nDeadbandPeriod ){
			pstDevEnv_Tmp->nDeadbandPeriod = ndata_val;
			cnchange_f = 1;
		}
	}
	else
	if( MB_CFG_DEADBAND_UPPER_TICK == nstart_addr ){
		if( ndata_val != pstDevEnv_Tmp->nDeadbandUpperTick ){
			pstDevEnv_Tmp->nDeadbandUpperTick = ndata_val;
			cnchange_f = 1;
		}
	}
	else
	if( MB_CFG_DEADBAND_DOWN_TICK == nstart_addr ){
		if( ndata_val != pstDevEnv_Tmp->nDeadbandDownTick ){
			pstDevEnv_Tmp->nDeadbandDownTick = ndata_val;
			cnchange_f = 1;
		}
	}
	else
	if( MB_CFG_MOTOR_MIN_UP_SPEED == nstart_addr ){
		if( ndata_val != pstDevEnv_Tmp->nMinUpperSpeed ){
			pstDevEnv_Tmp->nMinUpperSpeed = ndata_val;
			cnchange_f = 1;
		}
	}
	else
	if( MB_CFG_MOTOR_MIN_DOWN_SPEED == nstart_addr ){
		if( ndata_val != pstDevEnv_Tmp->nMinDownSpeed ){
			pstDevEnv_Tmp->nMinDownSpeed = ndata_val;
			cnchange_f = 1;
		}
	}
	else
	if( MB_CFG_MOTOR_MAX_UP_SPEED == nstart_addr ){
		if( ndata_val != pstDevEnv_Tmp->nMaxUpperSpeed ){
			pstDevEnv_Tmp->nMaxUpperSpeed = ndata_val;
			cnchange_f = 1;
		}
	}
	else
	if( MB_CFG_MOTOR_MAX_DOWN_SPEED == nstart_addr ){
		if( ndata_val != pstDevEnv_Tmp->nMaxDownSpeed ){
			pstDevEnv_Tmp->nMaxDownSpeed = ndata_val;
			cnchange_f = 1;
		}
	}
	else
	if( MB_CFG_NORMAL_UP_RAMP == nstart_addr ){
		if( ndata_val != pstDevEnv_Tmp->nNormalUpperRamp ){
			pstDevEnv_Tmp->nNormalUpperRamp = ndata_val;
			cnchange_f = 1;
		}
	}
	else
	if( MB_CFG_NORMAL_DOWM_RAMP == nstart_addr ){
		if( ndata_val != pstDevEnv_Tmp->nNormalDownRamp ){
			pstDevEnv_Tmp->nNormalDownRamp = ndata_val;
			cnchange_f = 1;
		}
	}
	else
	if( MB_CFG_MOTOR_NONC == nstart_addr ){
		if( ndata_val != pstDevEnv_Tmp->nMotorNoNc ){
			pstDevEnv_Tmp->nMotorNoNc = ndata_val;
			cnchange_f = 1;
		}
	}
	else{
		// SKIP
	}

	

	// response frame
	memset( gabRS485TxBuff, 0, sizeof( gabRS485TxBuff ) );
	gabRS485TxBuff[noffset] = _pbData[0];  				noffset += 1;
	gabRS485TxBuff[noffset] = _pbData[1];  				noffset += 1;
	gabRS485TxBuff[noffset] = _pbData[2];  				noffset += 1;
	gabRS485TxBuff[noffset] = _pbData[3];  				noffset += 1;
	gabRS485TxBuff[noffset] = _pbData[4];  				noffset += 1;
	gabRS485TxBuff[noffset] = _pbData[5];  				noffset += 1;


	ncrc = _modbus_crc16( gabRS485TxBuff, noffset );
	gabRS485TxBuff[noffset] = (uint8_t)( ( ncrc >> 8 ) & 0xff);  		noffset += 1;
	gabRS485TxBuff[noffset] = (uint8_t)( ( ncrc >> 0 ) & 0xff);  		noffset += 1;
	gnRS485TxBuffSize = noffset;

	if( 1 == cnchange_f ){
		Env_Restore_From_Temperary();
		ENV_Reload_Enviroment();
		Env_Save_Flash();
	}
	// printf("CRC %04x\r\n", ncrc);
	// DBG_Hex_Print((char*)"MODBUS[Sx06]", gabRS485TxBuff, gnRS485TxBuffSize );

	return cnret;
}

void _mymodbus_buffer_clear( void )
{
	memset( gabRS485RxBuff, 0, sizeof( gabRS485RxBuff ) );
	gnRS485RxOffset = 0;
	gnRS485RxBuffSize = 0;
}

/* Table of CRC values for high–order byte */
static unsigned char abModbus_CRCHi[256] = {
	0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81,
	0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
	0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01,
	0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
	0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81,
	0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
	0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01,
	0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
	0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81,
	0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
	0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01,
	0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
	0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81,
	0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
	0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01,
	0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
	0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81,
	0x40
}; 

/* Table of CRC values for low–order byte */
static char abModbus_CRCLo[256] = {
	0x00, 0xC0, 0xC1, 0x01, 0xC3, 0x03, 0x02, 0xC2, 0xC6, 0x06, 0x07, 0xC7, 0x05, 0xC5, 0xC4,
	0x04, 0xCC, 0x0C, 0x0D, 0xCD, 0x0F, 0xCF, 0xCE, 0x0E, 0x0A, 0xCA, 0xCB, 0x0B, 0xC9, 0x09,
	0x08, 0xC8, 0xD8, 0x18, 0x19, 0xD9, 0x1B, 0xDB, 0xDA, 0x1A, 0x1E, 0xDE, 0xDF, 0x1F, 0xDD,
	0x1D, 0x1C, 0xDC, 0x14, 0xD4, 0xD5, 0x15, 0xD7, 0x17, 0x16, 0xD6, 0xD2, 0x12, 0x13, 0xD3,
	0x11, 0xD1, 0xD0, 0x10, 0xF0, 0x30, 0x31, 0xF1, 0x33, 0xF3, 0xF2, 0x32, 0x36, 0xF6, 0xF7,
	0x37, 0xF5, 0x35, 0x34, 0xF4, 0x3C, 0xFC, 0xFD, 0x3D, 0xFF, 0x3F, 0x3E, 0xFE, 0xFA, 0x3A,
	0x3B, 0xFB, 0x39, 0xF9, 0xF8, 0x38, 0x28, 0xE8, 0xE9, 0x29, 0xEB, 0x2B, 0x2A, 0xEA, 0xEE,
	0x2E, 0x2F, 0xEF, 0x2D, 0xED, 0xEC, 0x2C, 0xE4, 0x24, 0x25, 0xE5, 0x27, 0xE7, 0xE6, 0x26,
	0x22, 0xE2, 0xE3, 0x23, 0xE1, 0x21, 0x20, 0xE0, 0xA0, 0x60, 0x61, 0xA1, 0x63, 0xA3, 0xA2,
	0x62, 0x66, 0xA6, 0xA7, 0x67, 0xA5, 0x65, 0x64, 0xA4, 0x6C, 0xAC, 0xAD, 0x6D, 0xAF, 0x6F,
	0x6E, 0xAE, 0xAA, 0x6A, 0x6B, 0xAB, 0x69, 0xA9, 0xA8, 0x68, 0x78, 0xB8, 0xB9, 0x79, 0xBB,
	0x7B, 0x7A, 0xBA, 0xBE, 0x7E, 0x7F, 0xBF, 0x7D, 0xBD, 0xBC, 0x7C, 0xB4, 0x74, 0x75, 0xB5,
	0x77, 0xB7, 0xB6, 0x76, 0x72, 0xB2, 0xB3, 0x73, 0xB1, 0x71, 0x70, 0xB0, 0x50, 0x90, 0x91,
	0x51, 0x93, 0x53, 0x52, 0x92, 0x96, 0x56, 0x57, 0x97, 0x55, 0x95, 0x94, 0x54, 0x9C, 0x5C,
	0x5D, 0x9D, 0x5F, 0x9F, 0x9E, 0x5E, 0x5A, 0x9A, 0x9B, 0x5B, 0x99, 0x59, 0x58, 0x98, 0x88,
	0x48, 0x49, 0x89, 0x4B, 0x8B, 0x8A, 0x4A, 0x4E, 0x8E, 0x8F, 0x4F, 0x8D, 0x4D, 0x4C, 0x8C,
	0x44, 0x84, 0x85, 0x45, 0x87, 0x47, 0x46, 0x86, 0x82, 0x42, 0x43, 0x83, 0x41, 0x81, 0x80,
	0x40
};

// _pabIn ; /* message to calculate CRC upon */
// _nLen ; /* quantity of bytes in message */
uint16_t _modbus_crc16( uint8_t* _pabIn, uint16_t _nLen )
{
	int i;
	uint8_t uchCRCHi = 0xFF ; /* high byte of CRC initialized */
	uint8_t uchCRCLo = 0xFF ; /* low byte of CRC initialized */
	uint16_t nIndex ; /* will index into CRC lookup table */


	// while( _nLen-- ) 
	for( i = 0; i < _nLen; i++ )
	{
		nIndex = uchCRCHi ^ *_pabIn++ ; /* calculate the CRC */
		uchCRCHi = uchCRCLo ^ abModbus_CRCHi[nIndex] ;
		uchCRCLo = abModbus_CRCLo[nIndex] ;
	}
	
	return (uchCRCHi << 8 | uchCRCLo) ;
}

// uint16_t _modbus_delta_crc16( uint8_t* _pabIn, uint16_t _nLen )
// {
// 	int j;
// 	uint16_t reg_crc = 0xffff;
// 	while( _nLen-- )
// 	{
// 		reg_crc ^= *_pabIn++;
// 		for( j = 0; j < 8; j++ )
// 		{
// 			if( reg_crc & 0x01 ){
// 				reg_crc = ( reg_crc >> 1 ) ^ 0xa001;
// 			}
// 			else{
// 				reg_crc = reg_crc >> 1;
// 			}
// 		}
// 	}
// 	return reg_crc;
// }


/*
void _create_packet( void )
{
	int16_t noffset = 0; 
	float nftmp = 0;
	int16_t ntmp16 = 0;
	uint16_t ncrc = 0;
	memset( gabRS485TxBuff, 0, sizeof( gabRS485TxBuff ) );
	gabRS485TxBuff[noffset] = 0x02;  		noffset += 1;
	gabRS485TxBuff[noffset] = gabRS485RxBuff[1];  		noffset += 1;
	gabRS485TxBuff[noffset] = 0x20;  		noffset += 1;
	ntmp16 = (int16_t)gnCorrectValue;
	gabRS485TxBuff[noffset] = (uint8_t)( ( ntmp16 >> 8 ) & 0xff );  	noffset += 1;
	gabRS485TxBuff[noffset] = (uint8_t)( ( ntmp16 >> 0 ) & 0xff );  	noffset += 1;
	printf("ntmp16 : %d\r\n", ntmp16);
	// memcpy( &gabRS485TxBuff[noffset], &ntmp16, 2 );
	// noffset += 2;
	nftmp =  ADC0_Get_Current();
	printf("Current %f\r\n", nftmp );
	ntmp16 = 100;
	gabRS485TxBuff[noffset] = (uint8_t)( ( ntmp16 >> 8 ) & 0xff );  	noffset += 1;
	gabRS485TxBuff[noffset] = (uint8_t)( ( ntmp16 >> 0 ) & 0xff );  	noffset += 1;
	// memcpy( &gabRS485TxBuff[noffset], &ntmp16, 2 );
	// noffset += 2;

	ntmp16 = -200;
	gabRS485TxBuff[noffset] = (uint8_t)( ( ntmp16 >> 8 ) & 0xff );  	noffset += 1;
	gabRS485TxBuff[noffset] = (uint8_t)( ( ntmp16 >> 0 ) & 0xff );  	noffset += 1;
	// memcpy( &gabRS485TxBuff[noffset], &ntmp16, 2 );
	// noffset += 2;

	nftmp =  MYMOTOR_Get_Motor_Pwm();
	printf("PWM %f\r\n", nftmp);
	ntmp16 = 300;
	gabRS485TxBuff[noffset] = (uint8_t)( ( ntmp16 >> 8 ) & 0xff );  	noffset += 1;
	gabRS485TxBuff[noffset] = (uint8_t)( ( ntmp16 >> 0 ) & 0xff );  	noffset += 1;
	// memcpy( &gabRS485TxBuff[noffset], &ntmp16, 2 );
	// noffset += 2;

	ntmp16 = 10;
	int i;
	for( i = 0;  i < 12; i++ )
	{
		ntmp16 = 10+i;
		gabRS485TxBuff[noffset] = (uint8_t)( ( ntmp16 >> 8 ) & 0xff );  	noffset += 1;
		gabRS485TxBuff[noffset] = (uint8_t)( ( ntmp16 >> 0 ) & 0xff );  	noffset += 1;
		// memcpy( &gabRS485TxBuff[noffset], &ntmp16, 2 );
		// noffset += 2;
	}
	// noffset += 20;

	ncrc = _modbus_crc16( gabRS485TxBuff, noffset );
	gabRS485TxBuff[noffset] = (uint8_t)( ( ncrc >> 8 ) & 0xff);  		noffset += 1;
	gabRS485TxBuff[noffset] = (uint8_t)( ( ncrc >> 0 ) & 0xff);  		noffset += 1;
	gnRS485TxBuffSize = noffset+1;
	printf("CRC %04x\r\n", ncrc);
	DBG_Hex_Print((char*)"MODBUS[S]", gabRS485TxBuff, gnRS485TxBuffSize );
}
*/
#else
#include <string.h>

#include "global.h"
#include "mymodbus.h"
#include "myuart.h"
#include "mymotor.h"
#include "myadc.h"

/////////////////////////////////////////////////////////////////////////////////////////////
// import function prototype


/////////////////////////////////////////////////////////////////////////////////////////////
// local function prototype
void _mymodbus_buffer_clear( void );
uint16_t _modbus_crc16( uint8_t* _pabIn, uint16_t _nLen );
uint16_t _modbus_delta_crc16( uint8_t* _pabIn, uint16_t _nLen );
void _create_packet( void );
uint8_t _modbus_check_packet( uint8_t* _pbData, uint16_t nLen );
uint8_t _modbus_get_hold_register( uint8_t* _pbData, uint16_t nLen );
uint8_t _modbus_set_hold_register( uint8_t* _pbData, uint16_t nLen );
void _modbus_update_data( void );
/////////////////////////////////////////////////////////////////////////////////////////////
// global variable
#define MODBUS_HOLDING_FUNC_CODE_READ 		0x03
#define MODBUS_INPUT_FUNC_CODE_READ 		0x04
#define MODBUS_HOLDING_FUNC_CODE_WRITE 		0x06

#define DATA_RATIO_LIMIT_MOTOR_CURRENT  		(float)10.0f
#define DATA_RATIO_PID  				 		(float)100.0f
#define DATA_RATIO_CURRENT  			 		(float)100.0f
#define DATA_RATIO_MOTOR_PWM			 		(float)1000.0f

typedef struct stMODBUS_DATA{
	/* uint16_t nDeviceId; */
	uint16_t nCfgOverCurrent;
	uint16_t nCfgTaget_Pa;
	uint16_t nCfgPid_P;
	uint16_t nCfgPid_I;
	uint16_t nCfgPid_D;
	uint16_t nCfgPid_Ext_P;
	uint16_t nCfgLimitChkTime_ms;
	uint16_t nCfgDiffPa_Cali_Offset;
	uint16_t nCfgStopTime;
	uint16_t nCfgWrongOpenClose;
	uint16_t nCommand;
	uint16_t nStDiff_Pa;
	uint16_t nStCurrnet;
	uint16_t nStDeviceStatus;
	uint16_t nStMotorPwm;
	uint16_t nSafety_Pa;
	uint16_t nSafety_Up_Ramp;
	uint16_t nSafety_Down_Ramp;
	uint16_t nDeadband_Pa;
	uint16_t nDeadband_Min_Pa;
	uint16_t nDeadband_Peiod;
	uint16_t nDeadband_Tick;
	uint16_t nMotor_Min_Spd;
	uint16_t nMotor_Max_Up_Spd;
	uint16_t nMotor_Max_Down_Spd;
	uint16_t nNormal_Up_Ramp;
	uint16_t nNormal_Down_Ramp;
	uint16_t nMotor_NoNc;
	uint16_t abDiff_Pa_Rpt_Test[16];
}MODBUS_DATA;

enum eMODBUS_REG{
	MB_CFG_OVERCURRENT = 0,
	MB_CFG_TARGET_PA,
	MB_CFG_PID_P,
	MB_CFG_PID_I,
	MB_CFG_PID_D,
	MB_CFG_PID_EXT_P,
	MB_CFG_LIMIT_CHK_TIME_MS,
	MB_CFG_DIFF_PA_CALIBRATION_OFFSET,
	MB_CFG_STOP_TIME_MS,
	MB_CFG_WRONG_OPEN_CLOSE,
	MB_COMMAND,
	MB_ST_DIFF_PA_NONE,
	MB_ST_CURRENT_NONE,
	MB_ST_DEVICE_STATUS_NONE,
	MB_ST_MOTOR_PWM_NONE,
	MB_CFG_SAFETY_PA,
	MB_CFG_SAFETY_UP_RAMP,
	MB_CFG_SAFETY_DOWM_RAMP,
	MB_CFG_DEADBAND_PA,
	MB_CFG_DEADBAND_MIN_PA,
	MB_CFG_DEADBAND_PERIOD,
	MB_CFG_DEADBAND_TICK,
	MB_CFG_MOTOR_MIN_SPEED,
	MB_CFG_MOTOR_MAX_UP_SPEED,
	MB_CFG_MOTOR_MAX_DOWN_SPEED,
	MB_CFG_NORMAL_UP_RAMP,
	MB_CFG_NORMAL_DOWM_RAMP,
	MB_CFG_MOTOR_NONC,
	MB_IDX_MAX
};

MODBUS_DATA gstModbusData;
uint8_t gcnSend_f = 0;
/////////////////////////////////////////////////////////////////////////////////////////////
// public and export function implement
void MYMODBUS_Proc( void )
{
	uint8_t cncode = 0;
	uint8_t cnres = 0;

	if( 1 == gnRS485Rx_f ){
		gnRS485Rx_f = 0;
		// MYUTIL_Printf("size %d %d\r\n", gnRS485RxBuffSize, gnRS485RxOffset );
		if( 0 == _modbus_check_packet( gabRS485RxBuff, gnRS485RxBuffSize ) ){
			_modbus_update_data();
			cncode = gabRS485RxBuff[1];
			// printf("----------------------------------------\r\n");
			 // DBG_Hex_Print((char*)"MODBUS[R]", gabRS485RxBuff, gnRS485RxBuffSize );
			// printf("----------------------------------------\r\n");
			if( ( MODBUS_HOLDING_FUNC_CODE_READ == cncode ) || ( MODBUS_INPUT_FUNC_CODE_READ == cncode ) ){
				cnres = _modbus_get_hold_register( gabRS485RxBuff, gnRS485RxBuffSize );
			}
			else
			if( MODBUS_HOLDING_FUNC_CODE_WRITE == cncode ){
				cnres = _modbus_set_hold_register( gabRS485RxBuff, gnRS485RxBuffSize );
			}

			// _create_packet();
			if( 0 == cnres ){
				gnModbusTxTimer = MODBUS_TX_DELAY_TIME_MS;
				gcnSend_f = 1;

			}
		}

		_mymodbus_buffer_clear();
	}

	if( ( 1 == gcnSend_f ) && ( 0 == gnModbusTxTimer ) ){
		gcnSend_f = 0;
		MYUART_Send_Data( &UartHandler_Rs486, gabRS485TxBuff, gnRS485TxBuffSize );
	}
}

void MYMODBUS_Update_Cfg( DEVICEENV* _pstDeviceEnv )
{
	// gstModbusData.nDeviceId = (uint8_t)_pstDeviceEnv->nDeviceID;

	gstModbusData.nCfgOverCurrent = _pstDeviceEnv->nOverCurrent;
	gstModbusData.nCfgTaget_Pa = _pstDeviceEnv->nTargetPa;
	gstModbusData.nCfgPid_P = _pstDeviceEnv->nPid_P;
	gstModbusData.nCfgPid_I = _pstDeviceEnv->nPid_I;
	gstModbusData.nCfgPid_D = _pstDeviceEnv->nPid_D;
	gstModbusData.nCfgPid_Ext_P = 0;
	gstModbusData.nCfgLimitChkTime_ms = 800;
	gstModbusData.nCfgDiffPa_Cali_Offset = 900;
	gstModbusData.nCfgStopTime = 20;
	gstModbusData.nCfgWrongOpenClose = 0;
	gstModbusData.nCommand = 0;
	gstModbusData.nSafety_Pa = _pstDeviceEnv->nSafetyPa;
	gstModbusData.nSafety_Up_Ramp = _pstDeviceEnv->nSafetyUpperRamp;
	gstModbusData.nSafety_Down_Ramp = _pstDeviceEnv->nSafetyDownRamp;
	gstModbusData.nDeadband_Pa = _pstDeviceEnv->nDeadbandPa;
	gstModbusData.nDeadband_Min_Pa = _pstDeviceEnv->nDeadBandTickMinPa;
	gstModbusData.nDeadband_Peiod = _pstDeviceEnv->nDeadbandPeriod;
	gstModbusData.nDeadband_Tick = _pstDeviceEnv->nDeadbandTick;
	gstModbusData.nMotor_Min_Spd = _pstDeviceEnv->nMinSpeed;
	gstModbusData.nMotor_Max_Up_Spd = _pstDeviceEnv->nMaxUpperSpeed;
	gstModbusData.nMotor_Max_Down_Spd = _pstDeviceEnv->nMaxDownSpeed;
	gstModbusData.nNormal_Up_Ramp = _pstDeviceEnv->nNormalUpperRamp;
	gstModbusData.nNormal_Down_Ramp = _pstDeviceEnv->nNormalDownRamp;
	gstModbusData.nMotor_NoNc = _pstDeviceEnv->nMotorNoNc;
}

void _modbus_update_data( void )
{
	float nftmp = 0.0f;
	uint16_t ntmp = 0;
	gstModbusData.nStDiff_Pa = (int16_t)gnfLpfPaValue;
	nftmp = ADC0_Get_Current();
	gstModbusData.nStCurrnet = (uint16_t)( nftmp * DATA_RATIO_CURRENT );
	ntmp = (MYMOTOR_Get_Open_LimitSwtich_St() << 8);
	ntmp |= (MYMOTOR_Get_Close_LimitSwtich_St() << 0);
	gstModbusData.nStDeviceStatus = ntmp;
	nftmp = MYMOTOR_Get_Motor_Pwm();
	gstModbusData.nStMotorPwm = (uint16_t)( nftmp * DATA_RATIO_MOTOR_PWM );

	int i;
	for( i = 0; i < 16; i++ )
	{
		gstModbusData.abDiff_Pa_Rpt_Test[i] = (int16_t)gnfLpfPaValue;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////
// local function implement
uint8_t _modbus_check_packet( uint8_t* _pbData, uint16_t nLen )
{
	uint8_t cnret = 0;
	uint16_t cncrc = 0;
	const uint8_t cnAddr = gcnDeviceId;

	do{
		// Check Modbus Addr
//		if( cnAddr != _pbData[0] ){
//			MYUTIL_Printf("Wrong Addr %02x\r\n", _pbData[0] );
//			cnret = 1;
//			break;
//		}

		if( 4 > nLen ){
			MYUTIL_Printf("Wrong data size %d\r\n", nLen );
			cnret = 2;
			break;
		}
		// cncrc = _modbus_crc16( _pbData, nLen - 2 );
		cncrc = ( (uint16_t)( _pbData[nLen-2] << 8 ) & 0xff00 ) | ( (uint16_t)_pbData[nLen-1] & 0x00ff );
		if( cncrc != _modbus_crc16( _pbData, nLen - 2 ) ){
			printf("Wrong CRC check: %04x %04x\r\n", cncrc, _modbus_crc16( _pbData, nLen - 2 ) );
			cnret = 3;
			break;
		}
	}while( 0 );

	return cnret;
}

uint8_t _modbus_get_hold_register( uint8_t* _pbData, uint16_t nLen )
{
	uint8_t cnret = 0;
	uint16_t ncrc = 0;
	int16_t noffset = 0;
	uint16_t nval = 0;
	int i;
	uint16_t* ptr = (uint16_t*)&gstModbusData;
	uint8_t nlimitcount = sizeof( MODBUS_DATA ) / sizeof( uint16_t );
	uint16_t nstart_addr = ( (uint16_t)_pbData[2] << 8 ) | _pbData[3];
	uint16_t ndata_count = ( (uint16_t)_pbData[4] << 8 ) | _pbData[5];

	if( ( 0 == ndata_count ) || ( MB_IDX_MAX  < ndata_count ) ){
		MYUTIL_Printf("Wrong 0x03 data count [%d]\r\n", ndata_count );
		return 1;
	}

	if( 480 != nstart_addr ){
		if( ( nstart_addr + ndata_count ) > nlimitcount ){
			MYUTIL_Printf("0x03 out of range start:%d, cnt:%d, limit:%d\r\n", nstart_addr, ndata_count, nlimitcount );
			return 1;
		}
	}

	// Check TX Buffer Overflow
	if( ( ndata_count * 2 + 5 ) > RS485_BUFF_SIZE ){
		MYUTIL_Printf("0x03 TX Buffer Overflow req:%d\r\n", ndata_count * 2 + 5 );
		return 1;
	}

	memset( gabRS485TxBuff, 0, sizeof( gabRS485TxBuff ) );
	gabRS485TxBuff[noffset] = _pbData[0];  				noffset += 1;
	gabRS485TxBuff[noffset] = _pbData[1];  				noffset += 1;
	gabRS485TxBuff[noffset] = ndata_count * 2; 			noffset += 1;

	if( 480 != nstart_addr ){
		if( ( 11 == nstart_addr ) && ( 1 == ndata_count ) ){
			nval = _pbData[0] + 2;
			gabRS485TxBuff[noffset] = (uint8_t)( ( nval >> 8 ) & 0xff);  	noffset += 1;
			gabRS485TxBuff[noffset] = (uint8_t)( ( nval >> 0 ) & 0xff);  	noffset += 1;
		}
		else{
			for( i = 0; i < ndata_count; i ++ )
			{
				nval = ptr[nstart_addr + i];
				gabRS485TxBuff[noffset] = (uint8_t)( ( nval >> 8 ) & 0xff);  	noffset += 1;
				gabRS485TxBuff[noffset] = (uint8_t)( ( nval >> 0 ) & 0xff);  	noffset += 1;
			}
		}
	}
	else{
		for( i = 0; i < ndata_count; i ++ )
		{
			if( 31 < _pbData[0] ){
				nval = ( _pbData[0] - 1) + ( ( _pbData[0] - 31 ) * 10 ) + i;
			}
			else{
				nval = (  _pbData[0] * 10 ) + i + 10;
			}
			gabRS485TxBuff[noffset] = (uint8_t)( ( nval >> 8 ) & 0xff);  	noffset += 1;
			gabRS485TxBuff[noffset] = (uint8_t)( ( nval >> 0 ) & 0xff);  	noffset += 1;
		}
	}
	ncrc = _modbus_crc16( gabRS485TxBuff, noffset );
	gabRS485TxBuff[noffset] = (uint8_t)( ( ncrc >> 8 ) & 0xff);  		noffset += 1;
	gabRS485TxBuff[noffset] = (uint8_t)( ( ncrc >> 0 ) & 0xff);  		noffset += 1;
	gnRS485TxBuffSize = noffset;

	 // DBG_Hex_Print((char*)"MODBUS[S:0x03]", gabRS485TxBuff, gnRS485TxBuffSize );
	return cnret;
}

// void _modbus_update_data( void )
// {
// 	float nftmp = 0.0f;
// 	// gstModbusData.nDeviceId = 2;
// 	gstModbusData.nCfgOverCurrent = (uint16_t)( gnfMotorCurretValue * DATA_RATIO_LIMIT_MOTOR_CURRENT );
// 	gstModbusData.nCfgTaget_Pa = (uint16_t)gnfTargetDpValue;
// 	gstModbusData.nCfgPid_P = (uint16_t)( gnfMotorPid_P * DATA_RATIO_PID );
// 	gstModbusData.nCfgPid_I = (uint16_t)( gnfMotorPid_I * DATA_RATIO_PID );
// 	gstModbusData.nCfgPid_D = (uint16_t)( gnfMotorPid_D * DATA_RATIO_PID );
// 	gstModbusData.nCfgPid_Ext_P = 0;
// 	gstModbusData.nCfgLimitChkTime_ms = 800;
// 	gstModbusData.nCfgDiffPa_Cali_Offset = 900;
// 	gstModbusData.nCfgStopTime = 20;
// 	gstModbusData.nCfgWrongOpenClose = 0;
// 	gstModbusData.nCommand = 0;
// 	gstModbusData.nStDiff_Pa = (int16_t)gnCorrectValue;
// 	nftmp = ADC0_Get_Current();
// 	gstModbusData.nStCurrnet = (uint16_t)( nftmp * DATA_RATIO_CURRENT );
// 	gstModbusData.nStDeviceStatus = 0;
// 	nftmp = MYMOTOR_Get_Motor_Pwm();
// 	gstModbusData.nStMotorPwm = (uint16_t)( nftmp * DATA_RATIO_MOTOR_PWM );
// }
uint8_t _modbus_set_hold_register( uint8_t* _pbData, uint16_t nLen )
{
	uint8_t cnret = 0;
	uint16_t ncrc = 0;
	int16_t noffset = 0;
	uint8_t cnchange_f = 0;
	float nftmp = 0;
	uint16_t nstart_addr = ( (uint16_t)_pbData[2] << 8 ) | _pbData[3];
	uint16_t ndata_val = ( (uint16_t)_pbData[4] << 8 ) | _pbData[5];
	uint16_t* ptr = (uint16_t*)&gstModbusData;
	uint8_t nlimitcount = sizeof( MODBUS_DATA ) / sizeof( uint16_t );

	if( nstart_addr > MB_IDX_MAX ){
		MYUTIL_Printf("0x06 out of range addr:%d, limit:%d\r\n", nstart_addr, nlimitcount );
		return 1;
	}
	ptr[nstart_addr] = ndata_val;

	ENV_Backup_To_Temperary();
	DEVICEENV* pstDevEnv_Tmp = Env_Get_Enviroment_Temperary();

	if( MB_CFG_OVERCURRENT == nstart_addr ){
		// nftmp = (float)ndata_val / DATA_RATIO_LIMIT_MOTOR_CURRENT;
		if( ndata_val != pstDevEnv_Tmp->nOverCurrent ){
			pstDevEnv_Tmp->nOverCurrent = ndata_val;
			cnchange_f = 1;
		}
	}
	else
	if( MB_CFG_TARGET_PA == nstart_addr ){
		// nftmp = (float)ndata_val;
		if( ndata_val != pstDevEnv_Tmp->nTargetPa ){
			pstDevEnv_Tmp->nTargetPa = ndata_val;
			cnchange_f = 1;
		}
	}
	else
	if( MB_CFG_PID_P == nstart_addr ){
		// nftmp = (float)ndata_val / DATA_RATIO_PID;
		if( ndata_val != pstDevEnv_Tmp->nPid_P ){
			pstDevEnv_Tmp->nPid_P = ndata_val;
			cnchange_f = 1;
		}
	}
	else
	if( MB_CFG_PID_I == nstart_addr ){
		// nftmp = (float)ndata_val / DATA_RATIO_PID;
		if( ndata_val != pstDevEnv_Tmp->nPid_I ){
			pstDevEnv_Tmp->nPid_I = ndata_val;
			cnchange_f = 1;
		}
	}
	else
	if( MB_CFG_PID_D == nstart_addr ){
		// nftmp = (float)ndata_val / DATA_RATIO_PID;
		if( ndata_val != pstDevEnv_Tmp->nPid_D ){
			pstDevEnv_Tmp->nPid_D = ndata_val;
			cnchange_f = 1;
		}
	}
	else
	if( MB_CFG_PID_EXT_P == nstart_addr ){
		// SKIP
	}
	else
	if( MB_CFG_LIMIT_CHK_TIME_MS == nstart_addr ){
		// SKIP
	}
	else
	if( MB_CFG_DIFF_PA_CALIBRATION_OFFSET == nstart_addr ){
		// SKIP
	}
	else
	if( MB_CFG_STOP_TIME_MS == nstart_addr ){
		// SKIP
	}
	else
	if( MB_CFG_WRONG_OPEN_CLOSE == nstart_addr ){
		// SKIP
	}
	else
	if( MB_COMMAND == nstart_addr ){
		// SKIP
	}
	else
	if( MB_CFG_SAFETY_PA == nstart_addr ){
		if( ndata_val != pstDevEnv_Tmp->nSafetyPa ){
			pstDevEnv_Tmp->nSafetyPa = ndata_val;
			cnchange_f = 1;
		}
	}
	else
	if( MB_CFG_SAFETY_UP_RAMP == nstart_addr ){
		if( ndata_val != pstDevEnv_Tmp->nSafetyUpperRamp ){
			pstDevEnv_Tmp->nSafetyUpperRamp = ndata_val;
			cnchange_f = 1;
		}
	}
	else
	if( MB_CFG_SAFETY_DOWM_RAMP == nstart_addr ){
		if( ndata_val != pstDevEnv_Tmp->nSafetyDownRamp ){
			pstDevEnv_Tmp->nSafetyDownRamp = ndata_val;
			cnchange_f = 1;
		}
	}
	else
	if( MB_CFG_DEADBAND_PA == nstart_addr ){
		if( ndata_val != pstDevEnv_Tmp->nDeadbandPa ){
			pstDevEnv_Tmp->nDeadbandPa = ndata_val;
			cnchange_f = 1;
		}
	}
	else
	if( MB_CFG_DEADBAND_MIN_PA == nstart_addr ){
		if( ndata_val != pstDevEnv_Tmp->nDeadBandTickMinPa ){
			pstDevEnv_Tmp->nDeadBandTickMinPa = ndata_val;
			cnchange_f = 1;
		}
	}
	else
	if( MB_CFG_DEADBAND_PERIOD == nstart_addr ){
		if( ndata_val != pstDevEnv_Tmp->nDeadbandPeriod ){
			pstDevEnv_Tmp->nDeadbandPeriod = ndata_val;
			cnchange_f = 1;
		}
	}
	else
	if( MB_CFG_DEADBAND_TICK == nstart_addr ){
		if( ndata_val != pstDevEnv_Tmp->nDeadbandTick ){
			pstDevEnv_Tmp->nDeadbandTick = ndata_val;
			cnchange_f = 1;
		}
	}
	else
	if( MB_CFG_MOTOR_MIN_SPEED == nstart_addr ){
		if( ndata_val != pstDevEnv_Tmp->nMinSpeed ){
			pstDevEnv_Tmp->nMinSpeed = ndata_val;
			cnchange_f = 1;
		}
	}
	else
	if( MB_CFG_MOTOR_MAX_UP_SPEED == nstart_addr ){
		if( ndata_val != pstDevEnv_Tmp->nMaxUpperSpeed ){
			pstDevEnv_Tmp->nMaxUpperSpeed = ndata_val;
			cnchange_f = 1;
		}
	}
	else
	if( MB_CFG_MOTOR_MAX_DOWN_SPEED == nstart_addr ){
		if( ndata_val != pstDevEnv_Tmp->nMaxDownSpeed ){
			pstDevEnv_Tmp->nMaxDownSpeed = ndata_val;
			cnchange_f = 1;
		}
	}
	else
	if( MB_CFG_NORMAL_UP_RAMP == nstart_addr ){
		if( ndata_val != pstDevEnv_Tmp->nNormalUpperRamp ){
			pstDevEnv_Tmp->nNormalUpperRamp = ndata_val;
			cnchange_f = 1;
		}
	}
	else
	if( MB_CFG_NORMAL_DOWM_RAMP == nstart_addr ){
		if( ndata_val != pstDevEnv_Tmp->nNormalDownRamp ){
			pstDevEnv_Tmp->nNormalDownRamp = ndata_val;
			cnchange_f = 1;
		}
	}
	else
	if( MB_CFG_MOTOR_NONC == nstart_addr ){
		if( ndata_val != pstDevEnv_Tmp->nMotorNoNc ){
			pstDevEnv_Tmp->nMotorNoNc = ndata_val;
			cnchange_f = 1;
		}
	}
	else{
		// SKIP
	}



	// response frame
	memset( gabRS485TxBuff, 0, sizeof( gabRS485TxBuff ) );
	gabRS485TxBuff[noffset] = _pbData[0];  				noffset += 1;
	gabRS485TxBuff[noffset] = _pbData[1];  				noffset += 1;
	gabRS485TxBuff[noffset] = _pbData[2];  				noffset += 1;
	gabRS485TxBuff[noffset] = _pbData[3];  				noffset += 1;
	gabRS485TxBuff[noffset] = _pbData[4];  				noffset += 1;
	gabRS485TxBuff[noffset] = _pbData[5];  				noffset += 1;


	ncrc = _modbus_crc16( gabRS485TxBuff, noffset );
	gabRS485TxBuff[noffset] = (uint8_t)( ( ncrc >> 8 ) & 0xff);  		noffset += 1;
	gabRS485TxBuff[noffset] = (uint8_t)( ( ncrc >> 0 ) & 0xff);  		noffset += 1;
	gnRS485TxBuffSize = noffset;

	if( 1 == cnchange_f ){
		Env_Restore_From_Temperary();
		ENV_Reload_Enviroment();
		Env_Save_Flash();
	}
	// printf("CRC %04x\r\n", ncrc);
	// DBG_Hex_Print((char*)"MODBUS[Sx06]", gabRS485TxBuff, gnRS485TxBuffSize );

	return cnret;
}

void _mymodbus_buffer_clear( void )
{
	memset( gabRS485RxBuff, 0, sizeof( gabRS485RxBuff ) );
	gnRS485RxOffset = 0;
	gnRS485RxBuffSize = 0;
}

/* Table of CRC values for high–order byte */
static unsigned char abModbus_CRCHi[256] = {
	0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81,
	0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
	0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01,
	0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
	0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81,
	0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
	0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01,
	0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
	0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81,
	0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
	0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01,
	0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
	0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81,
	0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
	0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01,
	0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
	0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81,
	0x40
};

/* Table of CRC values for low–order byte */
static char abModbus_CRCLo[256] = {
	0x00, 0xC0, 0xC1, 0x01, 0xC3, 0x03, 0x02, 0xC2, 0xC6, 0x06, 0x07, 0xC7, 0x05, 0xC5, 0xC4,
	0x04, 0xCC, 0x0C, 0x0D, 0xCD, 0x0F, 0xCF, 0xCE, 0x0E, 0x0A, 0xCA, 0xCB, 0x0B, 0xC9, 0x09,
	0x08, 0xC8, 0xD8, 0x18, 0x19, 0xD9, 0x1B, 0xDB, 0xDA, 0x1A, 0x1E, 0xDE, 0xDF, 0x1F, 0xDD,
	0x1D, 0x1C, 0xDC, 0x14, 0xD4, 0xD5, 0x15, 0xD7, 0x17, 0x16, 0xD6, 0xD2, 0x12, 0x13, 0xD3,
	0x11, 0xD1, 0xD0, 0x10, 0xF0, 0x30, 0x31, 0xF1, 0x33, 0xF3, 0xF2, 0x32, 0x36, 0xF6, 0xF7,
	0x37, 0xF5, 0x35, 0x34, 0xF4, 0x3C, 0xFC, 0xFD, 0x3D, 0xFF, 0x3F, 0x3E, 0xFE, 0xFA, 0x3A,
	0x3B, 0xFB, 0x39, 0xF9, 0xF8, 0x38, 0x28, 0xE8, 0xE9, 0x29, 0xEB, 0x2B, 0x2A, 0xEA, 0xEE,
	0x2E, 0x2F, 0xEF, 0x2D, 0xED, 0xEC, 0x2C, 0xE4, 0x24, 0x25, 0xE5, 0x27, 0xE7, 0xE6, 0x26,
	0x22, 0xE2, 0xE3, 0x23, 0xE1, 0x21, 0x20, 0xE0, 0xA0, 0x60, 0x61, 0xA1, 0x63, 0xA3, 0xA2,
	0x62, 0x66, 0xA6, 0xA7, 0x67, 0xA5, 0x65, 0x64, 0xA4, 0x6C, 0xAC, 0xAD, 0x6D, 0xAF, 0x6F,
	0x6E, 0xAE, 0xAA, 0x6A, 0x6B, 0xAB, 0x69, 0xA9, 0xA8, 0x68, 0x78, 0xB8, 0xB9, 0x79, 0xBB,
	0x7B, 0x7A, 0xBA, 0xBE, 0x7E, 0x7F, 0xBF, 0x7D, 0xBD, 0xBC, 0x7C, 0xB4, 0x74, 0x75, 0xB5,
	0x77, 0xB7, 0xB6, 0x76, 0x72, 0xB2, 0xB3, 0x73, 0xB1, 0x71, 0x70, 0xB0, 0x50, 0x90, 0x91,
	0x51, 0x93, 0x53, 0x52, 0x92, 0x96, 0x56, 0x57, 0x97, 0x55, 0x95, 0x94, 0x54, 0x9C, 0x5C,
	0x5D, 0x9D, 0x5F, 0x9F, 0x9E, 0x5E, 0x5A, 0x9A, 0x9B, 0x5B, 0x99, 0x59, 0x58, 0x98, 0x88,
	0x48, 0x49, 0x89, 0x4B, 0x8B, 0x8A, 0x4A, 0x4E, 0x8E, 0x8F, 0x4F, 0x8D, 0x4D, 0x4C, 0x8C,
	0x44, 0x84, 0x85, 0x45, 0x87, 0x47, 0x46, 0x86, 0x82, 0x42, 0x43, 0x83, 0x41, 0x81, 0x80,
	0x40
};

// _pabIn ; /* message to calculate CRC upon */
// _nLen ; /* quantity of bytes in message */
uint16_t _modbus_crc16( uint8_t* _pabIn, uint16_t _nLen )
{
	int i;
	uint8_t uchCRCHi = 0xFF ; /* high byte of CRC initialized */
	uint8_t uchCRCLo = 0xFF ; /* low byte of CRC initialized */
	uint16_t nIndex ; /* will index into CRC lookup table */


	// while( _nLen-- )
	for( i = 0; i < _nLen; i++ )
	{
		nIndex = uchCRCHi ^ *_pabIn++ ; /* calculate the CRC */
		uchCRCHi = uchCRCLo ^ abModbus_CRCHi[nIndex] ;
		uchCRCLo = abModbus_CRCLo[nIndex] ;
	}

	return (uchCRCHi << 8 | uchCRCLo) ;
}

// uint16_t _modbus_delta_crc16( uint8_t* _pabIn, uint16_t _nLen )
// {
// 	int j;
// 	uint16_t reg_crc = 0xffff;
// 	while( _nLen-- )
// 	{
// 		reg_crc ^= *_pabIn++;
// 		for( j = 0; j < 8; j++ )
// 		{
// 			if( reg_crc & 0x01 ){
// 				reg_crc = ( reg_crc >> 1 ) ^ 0xa001;
// 			}
// 			else{
// 				reg_crc = reg_crc >> 1;
// 			}
// 		}
// 	}
// 	return reg_crc;
// }


/*
void _create_packet( void )
{
	int16_t noffset = 0;
	float nftmp = 0;
	int16_t ntmp16 = 0;
	uint16_t ncrc = 0;
	memset( gabRS485TxBuff, 0, sizeof( gabRS485TxBuff ) );
	gabRS485TxBuff[noffset] = 0x02;  		noffset += 1;
	gabRS485TxBuff[noffset] = gabRS485RxBuff[1];  		noffset += 1;
	gabRS485TxBuff[noffset] = 0x20;  		noffset += 1;
	ntmp16 = (int16_t)gnCorrectValue;
	gabRS485TxBuff[noffset] = (uint8_t)( ( ntmp16 >> 8 ) & 0xff );  	noffset += 1;
	gabRS485TxBuff[noffset] = (uint8_t)( ( ntmp16 >> 0 ) & 0xff );  	noffset += 1;
	printf("ntmp16 : %d\r\n", ntmp16);
	// memcpy( &gabRS485TxBuff[noffset], &ntmp16, 2 );
	// noffset += 2;
	nftmp =  ADC0_Get_Current();
	printf("Current %f\r\n", nftmp );
	ntmp16 = 100;
	gabRS485TxBuff[noffset] = (uint8_t)( ( ntmp16 >> 8 ) & 0xff );  	noffset += 1;
	gabRS485TxBuff[noffset] = (uint8_t)( ( ntmp16 >> 0 ) & 0xff );  	noffset += 1;
	// memcpy( &gabRS485TxBuff[noffset], &ntmp16, 2 );
	// noffset += 2;

	ntmp16 = -200;
	gabRS485TxBuff[noffset] = (uint8_t)( ( ntmp16 >> 8 ) & 0xff );  	noffset += 1;
	gabRS485TxBuff[noffset] = (uint8_t)( ( ntmp16 >> 0 ) & 0xff );  	noffset += 1;
	// memcpy( &gabRS485TxBuff[noffset], &ntmp16, 2 );
	// noffset += 2;

	nftmp =  MYMOTOR_Get_Motor_Pwm();
	printf("PWM %f\r\n", nftmp);
	ntmp16 = 300;
	gabRS485TxBuff[noffset] = (uint8_t)( ( ntmp16 >> 8 ) & 0xff );  	noffset += 1;
	gabRS485TxBuff[noffset] = (uint8_t)( ( ntmp16 >> 0 ) & 0xff );  	noffset += 1;
	// memcpy( &gabRS485TxBuff[noffset], &ntmp16, 2 );
	// noffset += 2;

	ntmp16 = 10;
	int i;
	for( i = 0;  i < 12; i++ )
	{
		ntmp16 = 10+i;
		gabRS485TxBuff[noffset] = (uint8_t)( ( ntmp16 >> 8 ) & 0xff );  	noffset += 1;
		gabRS485TxBuff[noffset] = (uint8_t)( ( ntmp16 >> 0 ) & 0xff );  	noffset += 1;
		// memcpy( &gabRS485TxBuff[noffset], &ntmp16, 2 );
		// noffset += 2;
	}
	// noffset += 20;

	ncrc = _modbus_crc16( gabRS485TxBuff, noffset );
	gabRS485TxBuff[noffset] = (uint8_t)( ( ncrc >> 8 ) & 0xff);  		noffset += 1;
	gabRS485TxBuff[noffset] = (uint8_t)( ( ncrc >> 0 ) & 0xff);  		noffset += 1;
	gnRS485TxBuffSize = noffset+1;
	printf("CRC %04x\r\n", ncrc);
	DBG_Hex_Print((char*)"MODBUS[S]", gabRS485TxBuff, gnRS485TxBuffSize );
}
*/
#endif
