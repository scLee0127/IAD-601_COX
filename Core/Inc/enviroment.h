#ifndef __ENVIROMENT_H__
#define __ENVIROMENT_H__

#define ENV_MAGIC_CODE                  0x09211047


typedef struct __attribute__((packed))_DeviceEnv{
    uint32_t nMagicNum;
    uint16_t nDeviceID;
    uint16_t nBaudrate;
    uint16_t nOverCurrent;
    uint16_t nTargetPa;
    uint16_t nPid_P;
    uint16_t nPid_I;
    uint16_t nPid_D;
    uint16_t nMinUpperSpeed;
    uint16_t nMinDownSpeed;
    uint16_t nMaxUpperSpeed;
    uint16_t nMaxDownSpeed;
    uint16_t nNormalUpperRamp;
    uint16_t nNormalDownRamp; 
    uint16_t nSafetyUpperRamp;
    uint16_t nSafetyDownRamp;
    uint16_t nSafetyPa;
    uint16_t nDeadbandPa;
    uint16_t nDeadbandPeriod;
    uint16_t nDeadbandUpperTick;
    uint16_t nDeadbandDownTick;
    uint16_t nBlankTime_ms;
    uint16_t nDeadBandTickMinPa;         // 
    uint16_t nMotorNoNc;
}DEVICEENV;

// Uppper 2배 힘이 딸림
#define ENV_FACTORY_DEFAULT_DEVICE_ID               1
#define ENV_FACTORY_DEFAULT_BAUDRATE  			    3       // myuart.h에 9600bps로 정의됨
#define ENV_FACTORY_DEFAULT_TARGET_PA  			    50
#define ENV_FACTORY_DEFAULT_NORMAL_UPPER_RAMP  	    1
#define ENV_FACTORY_DEFAULT_NORMAL_DOWN_RAMP 	    1
#define ENV_FACTORY_DEFAULT_SAFETY_PA  			    25
#define ENV_FACTORY_DEFAULT_SAFETY_UPPER_RAMP  	    1
#define ENV_FACTORY_DEFAULT_SAFETY_DOWN_RAMP 	    1
#define ENV_FACTORY_DEFAULT_DEADBAND_PA  		    12
#define ENV_FACTORY_DEFAULT_DEADBAND_PERIOD  	    40
#define ENV_FACTORY_DEFAULT_DEADBAND_UPPER_TICK  	5
#define ENV_FACTORY_DEFAULT_DEADBAND_DOWN_TICK  	10
#define ENV_FACTORY_DEFAULT_MOTOR_UPPER_MIN_SPEED  	35
#define ENV_FACTORY_DEFAULT_MOTOR_DOWN_MIN_SPEED  	15
#define ENV_FACTORY_DEFAULT_MOTOR_MAX_UPPER_SPEED  	150
#define ENV_FACTORY_DEFAULT_MOTOR_MAX_DOWN_SPEED    20
#define ENV_FACTORY_DEFAULT_PID_P  				    15
#define ENV_FACTORY_DEFAULT_PID_I  				    5
#define ENV_FACTORY_DEFAULT_PID_D  				    2
#define ENV_FACTORY_DEFAULT_MOTOR_BLANKTIME         500
#define ENV_FACTORY_DEFAULT_NO_ACT_PA               4
#define ENV_FACTORY_DEFAULT_OVER_CURRENT            10
#define ENV_FACTORY_DEFAULT_MOTOR_NONC              0       // 0: Normal close, 1: Normar open
// extern DEVICEENV gstDeviceEnv;
// extern DEVICEENV gstDeviceEnv_Tmp;

void ENV_Load_Enviroment_Info( void );
int ENV_Save_Enviroment_Info( void );
void ENV_Init_Enviroment_MFG( void );
void ENV_Print_Load_Data( void );
void ENV_Backup_To_Temperary( void );
void Env_Restore_From_Temperary( void );
const DEVICEENV* Env_Get_Enviroment( void );
DEVICEENV* Env_Get_Enviroment_Temperary( void );
void ENV_Reload_Enviroment( void );
void Env_Save_Flash( void );
#endif //__ENVIROMENT_H__

#if 0
RS485 Baud 8/115200
gcnDeviceId 1
gcnRs485_Baudrate 8
gnfTargetDpValue 50.000
gnfNormalUpperRamp 0.500
gnfNormalDwonRamp 0.700
gnfSettledBand_Pa 35.000
gnfSettleBandUpperRamp 0.001
gnfSettleBandDownRamp 0.001
gnfDeadBand_Pa 20.000
gnfDeadBand_Tick_Min_Pa 5.000
gnDeadBand_Period 80
gnDeadBand_UpperTick 30
gnDeadBand_DownTick 30
gnfMotorMinUpperSpeed 0.020
gnfMotorMinDownSpeed 0.010
gnfMotorMaxUpperSpeed 0.500
gnfMotorMaxDownSpeed 0.800
gnfMotorPid_P 0.150
gnfMotorPid_I 0.050
gnfMotorPid_D 0.020
gnfMotorCurretValue 1.0
gnMotorNonc 0.0
====================================================
= GENP IAD200 by Simba
= Version : 1.00(2026-03-13 16:48:34.32)
= FW Date : Mar 13 2026 16:48:34
====================================================
#endif