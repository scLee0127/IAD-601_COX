#include <string.h>

#include "global.h"
#include "enviroment.h"
#include "myflash.h"
#include "mymotor.h"
#include "mymodbus.h"
#include "myuart.h"

/////////////////////////////////////////////////////////////////////////////////////////////
// import function prototype


/////////////////////////////////////////////////////////////////////////////////////////////
// local function prototype
void _env_init_factory_default( void );
void _env_apply_load_data( void );
void _env_print_load_data( void );
/////////////////////////////////////////////////////////////////////////////////////////////
// global variable
DEVICEENV gstDeviceEnv;
DEVICEENV gstDeviceEnv_Tmp;
#define ENV_DATA_HALF_SIZE 		( sizeof( DEVICEENV ) / sizeof( uint16_t ) )
uint16_t gabFlashBuff[ENV_DATA_HALF_SIZE] = {0,};
static uint8_t gcnRs485_Baud_Chg_f = 0;
/////////////////////////////////////////////////////////////////////////////////////////////
// public and export function implement
void ENV_Load_Enviroment_Info( void )
{
	int nres = 0;
	
	memset( gabFlashBuff, 0, sizeof( gabFlashBuff ) );
	Flash_ReadHalfWords( FLASH_USER_START_ADDR, gabFlashBuff, ENV_DATA_HALF_SIZE );
	memcpy( (uint8_t*)&gstDeviceEnv, gabFlashBuff, sizeof( DEVICEENV ) );
	if( ENV_MAGIC_CODE != gstDeviceEnv.nMagicNum ){
		MYUTIL_Printf("Reset Enviroment Data\r\n");
		nres = Flash_EraseUserPage();
		if( HAL_OK == nres ){
			MYUTIL_Printf("Erase flash success\r\n");
		}

		_env_init_factory_default();
		MYUTIL_Printf("Write Flash%d\r\n", sizeof( DEVICEENV ));
		memset( gabFlashBuff, 0, sizeof( gabFlashBuff ) );
		memcpy( gabFlashBuff, (uint8_t*)&gstDeviceEnv, sizeof( DEVICEENV ) );
		nres = Flash_WriteHalfWords( FLASH_USER_START_ADDR, gabFlashBuff, ENV_DATA_HALF_SIZE );
		if( 0 == nres ){
			MYUTIL_Printf("Write data in flash success[load]\r\n");
		}
	}
	else{
		MYUTIL_Printf("Magic code OK\r\n");
	}
	_env_apply_load_data();
}

int ENV_Save_Enviroment_Info( void )
{
	int nres = 0;
	MYUTIL_Printf("[S-F] Magic Code : %08x %08x\r\n", (unsigned int)ENV_MAGIC_CODE, (unsigned int)gstDeviceEnv.nMagicNum );
	nres = Flash_EraseUserPage();
	if( 0 == nres ){
		MYUTIL_Printf("Success erase flash\r\n");
	}

	MYUTIL_Printf("Write data in flash success[save]\r\n");
	memset( gabFlashBuff, 0, sizeof( gabFlashBuff ) );
	memcpy( gabFlashBuff, (uint8_t*)&gstDeviceEnv, sizeof( DEVICEENV ) );
	Flash_WriteHalfWords( FLASH_USER_START_ADDR, gabFlashBuff, ENV_DATA_HALF_SIZE );

	if( 1 == gcnRs485_Baud_Chg_f ){
		gcnRs485_Baud_Chg_f = 0;
		MYUART_RS485_ReInit();
	}
	_env_apply_load_data();

	return nres;
}


void ENV_Init_Enviroment_MFG( void )
{
	int nres = 0;
	nres = Flash_EraseUserPage();
	if( 0 == nres ){
		MYUTIL_Printf("Success erase flash\r\n");
	}

	_env_init_factory_default();
	MYUTIL_Printf("Initialize flash data\r\n");
	memset( gabFlashBuff, 0, sizeof( gabFlashBuff ) );
	memcpy( gabFlashBuff, (uint8_t*)&gstDeviceEnv, sizeof( DEVICEENV ) );
	Flash_WriteHalfWords( FLASH_USER_START_ADDR, gabFlashBuff, ENV_DATA_HALF_SIZE );

	_env_apply_load_data();
}

void ENV_Reload_Enviroment( void )
{
	MYUTIL_Printf("[ENV] Reload global data\r\n");
	_env_apply_load_data();
}

void ENV_Print_Load_Data( void )
{
	_env_print_load_data();
}

void ENV_Backup_To_Temperary( void )
{
	memset( &gstDeviceEnv_Tmp, 0, sizeof( DEVICEENV ) );
	memcpy( &gstDeviceEnv_Tmp, &gstDeviceEnv, sizeof( DEVICEENV ) );
}

void Env_Restore_From_Temperary( void )
{
	if( gstDeviceEnv.nBaudrate != gstDeviceEnv_Tmp.nBaudrate ){
		gcnRs485_Baud_Chg_f = 1;
	}
	memset( &gstDeviceEnv, 0, sizeof( DEVICEENV ) );
	memcpy( &gstDeviceEnv, &gstDeviceEnv_Tmp, sizeof( DEVICEENV ) );
}

void Env_Save_Flash( void )
{
	gcnSaveEnv_f = 1;
	gnSaveEnvDelayTimer = ENV_SAVE_FLASH_TIME_MS;
	_env_print_load_data();
}

const DEVICEENV* Env_Get_Enviroment( void )
{
	return &gstDeviceEnv;
}

DEVICEENV* Env_Get_Enviroment_Temperary( void )
{
	return &gstDeviceEnv_Tmp;
}
/////////////////////////////////////////////////////////////////////////////////////////////
// local function implement
void _env_init_factory_default( void )
{
	memset( &gstDeviceEnv, 0, sizeof( DEVICEENV ) );
	gstDeviceEnv.nMagicNum = ENV_MAGIC_CODE;

	gstDeviceEnv.nDeviceID = ENV_FACTORY_DEFAULT_DEVICE_ID;
    gstDeviceEnv.nBaudrate = ENV_FACTORY_DEFAULT_BAUDRATE;
    gstDeviceEnv.nTargetPa = ENV_FACTORY_DEFAULT_TARGET_PA;
    gstDeviceEnv.nNormalUpperRamp = ENV_FACTORY_DEFAULT_NORMAL_UPPER_RAMP;
    gstDeviceEnv.nNormalDownRamp = ENV_FACTORY_DEFAULT_NORMAL_DOWN_RAMP;
    gstDeviceEnv.nSafetyPa = ENV_FACTORY_DEFAULT_SAFETY_PA;
    gstDeviceEnv.nSafetyUpperRamp = ENV_FACTORY_DEFAULT_SAFETY_UPPER_RAMP;
    gstDeviceEnv.nSafetyDownRamp = ENV_FACTORY_DEFAULT_SAFETY_DOWN_RAMP;
    gstDeviceEnv.nDeadbandPa = ENV_FACTORY_DEFAULT_DEADBAND_PA;
    gstDeviceEnv.nDeadbandPeriod = ENV_FACTORY_DEFAULT_DEADBAND_PERIOD;
	gstDeviceEnv.nDeadbandUpperTick = ENV_FACTORY_DEFAULT_DEADBAND_UPPER_TICK;
    gstDeviceEnv.nDeadbandDownTick = ENV_FACTORY_DEFAULT_DEADBAND_DOWN_TICK;
    gstDeviceEnv.nMinUpperSpeed = ENV_FACTORY_DEFAULT_MOTOR_UPPER_MIN_SPEED;
	gstDeviceEnv.nMinDownSpeed = ENV_FACTORY_DEFAULT_MOTOR_DOWN_MIN_SPEED;
    gstDeviceEnv.nMaxUpperSpeed = ENV_FACTORY_DEFAULT_MOTOR_MAX_UPPER_SPEED;
    gstDeviceEnv.nMaxDownSpeed = ENV_FACTORY_DEFAULT_MOTOR_MAX_DOWN_SPEED;
    gstDeviceEnv.nPid_P = ENV_FACTORY_DEFAULT_PID_P;
    gstDeviceEnv.nPid_I = ENV_FACTORY_DEFAULT_PID_I;
    gstDeviceEnv.nPid_D = ENV_FACTORY_DEFAULT_PID_D;
    gstDeviceEnv.nBlankTime_ms = ENV_FACTORY_DEFAULT_MOTOR_BLANKTIME;
    gstDeviceEnv.nDeadBandTickMinPa = ENV_FACTORY_DEFAULT_NO_ACT_PA;
    gstDeviceEnv.nOverCurrent = ENV_FACTORY_DEFAULT_OVER_CURRENT;
    gstDeviceEnv.nMotorNoNc = ENV_FACTORY_DEFAULT_MOTOR_NONC;
}

void _env_apply_load_data( void )
{
	gcnDeviceId = (uint8_t)gstDeviceEnv.nDeviceID;
	gcnRs485_Baudrate = (uint8_t)gstDeviceEnv.nBaudrate;
	gnfTargetDpValue = (float)gstDeviceEnv.nTargetPa;
	gnfMotorPid_P = (float)gstDeviceEnv.nPid_P / 100.0f;
	gnfMotorPid_I = (float)gstDeviceEnv.nPid_I / 100.0f;
	gnfMotorPid_D = (float)gstDeviceEnv.nPid_D / 100.0f;
	gnfMotorMinUpperSpeed = (float)gstDeviceEnv.nMinUpperSpeed / 1000.0f;
	gnfMotorMinDownSpeed = (float)gstDeviceEnv.nMinDownSpeed / 1000.0f;
	gnfMotorMaxUpperSpeed = (float)gstDeviceEnv.nMaxUpperSpeed / 1000.0f;
	gnfMotorMaxDownSpeed = (float)gstDeviceEnv.nMaxDownSpeed / 1000.0f;
	gnfNormalUpperRamp = (float)gstDeviceEnv.nNormalUpperRamp / 1000.0f;
	gnfNormalDwonRamp = (float)gstDeviceEnv.nNormalDownRamp / 1000.0f;
	gnfSettledBand_Pa = (float)gstDeviceEnv.nSafetyPa;
	gnfSettleBandUpperRamp = (float)gstDeviceEnv.nSafetyUpperRamp / 1000.0f;
	gnfSettleBandDownRamp = (float)gstDeviceEnv.nSafetyDownRamp / 1000.0f;
	gnfDeadBand_Pa = (float)gstDeviceEnv.nDeadbandPa;
	gnDeadBand_Period = gstDeviceEnv.nDeadbandPeriod;
	gnDeadBand_UpperTick = gstDeviceEnv.nDeadbandUpperTick;
	gnDeadBand_DownTick = gstDeviceEnv.nDeadbandDownTick;
	if( 1000 > gstDeviceEnv.nBlankTime_ms){
		gnLimitBlankTime_ms = gstDeviceEnv.nBlankTime_ms;
	}
	else{
		gstDeviceEnv.nBlankTime_ms = ENV_FACTORY_DEFAULT_MOTOR_BLANKTIME;
		gnLimitBlankTime_ms = ENV_FACTORY_DEFAULT_MOTOR_BLANKTIME;
	}
	gnfDeadBand_Tick_Min_Pa = (float)gstDeviceEnv.nDeadBandTickMinPa;
	gnfMotorCurretValue = (float)gstDeviceEnv.nOverCurrent / 10.0f;
	gnMotorNonc = gstDeviceEnv.nMotorNoNc;
	MYMOTOR_Init();
	MYMODBUS_Update_Cfg( &gstDeviceEnv );
}

void _env_print_load_data( void )
{
	MYUTIL_Printf( "gcnDeviceId %d\r\n", gcnDeviceId );
	MYUTIL_Printf( "gcnRs485_Baudrate %d\r\n", gcnRs485_Baudrate );
	MYUTIL_Printf( "gnfTargetDpValue %.3f\r\n", gnfTargetDpValue );
	MYUTIL_Printf( "gnfNormalUpperRamp %.3f\r\n", gnfNormalUpperRamp );
	MYUTIL_Printf( "gnfNormalDwonRamp %.3f\r\n", gnfNormalDwonRamp );
	MYUTIL_Printf( "gnfSettledBand_Pa %.3f\r\n", gnfSettledBand_Pa );
	MYUTIL_Printf( "gnfSettleBandUpperRamp %.3f\r\n", gnfSettleBandUpperRamp );
	MYUTIL_Printf( "gnfSettleBandDownRamp %.3f\r\n", gnfSettleBandDownRamp );
	MYUTIL_Printf( "gnfDeadBand_Pa %.3f\r\n", gnfDeadBand_Pa );
	MYUTIL_Printf( "gnfDeadBand_Tick_Min_Pa %.3f\r\n", gnfDeadBand_Tick_Min_Pa );
	MYUTIL_Printf( "gnDeadBand_Period %d\r\n", gnDeadBand_Period );
	MYUTIL_Printf( "gnDeadBand_UpperTick %d\r\n", gnDeadBand_UpperTick );
	MYUTIL_Printf( "gnDeadBand_DownTick %d\r\n", gnDeadBand_DownTick );
	MYUTIL_Printf( "gnfMotorMinUpperSpeed %.3f\r\n", gnfMotorMinUpperSpeed );
	MYUTIL_Printf( "gnfMotorMinDownSpeed %.3f\r\n", gnfMotorMinDownSpeed );
	MYUTIL_Printf( "gnfMotorMaxUpperSpeed %.3f\r\n", gnfMotorMaxUpperSpeed );
	MYUTIL_Printf( "gnfMotorMaxDownSpeed %.3f\r\n", gnfMotorMaxDownSpeed );
	MYUTIL_Printf( "gnfMotorPid_P %.3f\r\n", gnfMotorPid_P );
	MYUTIL_Printf( "gnfMotorPid_I %.3f\r\n", gnfMotorPid_I );
	MYUTIL_Printf( "gnfMotorPid_D %.3f\r\n", gnfMotorPid_D );
	MYUTIL_Printf( "gnfMotorCurretValue %.1f\r\n", gnfMotorCurretValue );
	MYUTIL_Printf( "gnMotorNonc %d\r\n", gnMotorNonc );
}
