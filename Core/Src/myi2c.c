#include "global.h"
#include "myi2c.h"
#include "mygpio.h"
/////////////////////////////////////////////////////////////////////////////////////////////
// import function prototype


/////////////////////////////////////////////////////////////////////////////////////////////
// local function prototype


/////////////////////////////////////////////////////////////////////////////////////////////
// global variable
#define	SCL_Low()		HAL_GPIO_WritePin( MYGPIO_DP_SENSOR_GPIO_PORT, MYGPIO_DP_SENSOR_SCL_GPIO_NUM, GPIO_PIN_RESET )
#define	SCL_High()		HAL_GPIO_WritePin( MYGPIO_DP_SENSOR_GPIO_PORT, MYGPIO_DP_SENSOR_SCL_GPIO_NUM, GPIO_PIN_SET )
#define	SCL_Status()	HAL_GPIO_ReadPin( MYGPIO_DP_SENSOR_GPIO_PORT, MYGPIO_DP_SENSOR_SCL_GPIO_NUM )
       	
#define	SDA_Low()		HAL_GPIO_WritePin( MYGPIO_DP_SENSOR_GPIO_PORT, MYGPIO_DP_SENSOR_SDA_GPIO_NUM, GPIO_PIN_RESET )
#define	SDA_High()		HAL_GPIO_WritePin( MYGPIO_DP_SENSOR_GPIO_PORT, MYGPIO_DP_SENSOR_SDA_GPIO_NUM, GPIO_PIN_SET )
#define	SDA_Status()	HAL_GPIO_ReadPin( MYGPIO_DP_SENSOR_GPIO_PORT, MYGPIO_DP_SENSOR_SDA_GPIO_NUM )
uint8_t I2C_Busy;
/////////////////////////////////////////////////////////////////////////////////////////////
// public and export function implement
void I2C_Delay( uint32_t _cnDelay )
{
	volatile uint32_t ntmp;
	ntmp = _cnDelay;
	
	while( ntmp-- ){ __NOP(); }
}

void I2C_Start_Condition(void)
{
	I2C_Delay(5);
	SDA_Low();
	I2C_Delay(10);
	SCL_Low();
	I2C_Delay(5);	
}

void I2C_Stop_Condition(void)
{
	SDA_Low();
	I2C_Delay(5);
	SCL_High();
	I2C_Delay(10);
	SDA_High();
	I2C_Delay(5);		
}

void I2C_Restart_Condition( void )
{
	SCL_Low();
	I2C_Delay(5);
	SDA_High();
	I2C_Delay(5);
	SCL_High();
	I2C_Delay(5);
	SDA_Low();
	I2C_Delay(5);
	SCL_Low();
	I2C_Delay(5);
}

static void I2C_SCL_Dir_Out( void ) {
    GPIO_InitTypeDef g = {0};
    g.Pin = MYGPIO_DP_SENSOR_SCL_GPIO_NUM;
    g.Mode = GPIO_MODE_OUTPUT_OD; // 오픈드레인 권장
    g.Pull = GPIO_NOPULL;
    g.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(MYGPIO_DP_SENSOR_GPIO_PORT, &g);
}

static void I2C_SCL_Dir_In( void ) {
    GPIO_InitTypeDef g = {0};
    g.Pin = MYGPIO_DP_SENSOR_SCL_GPIO_NUM;
    g.Mode = GPIO_MODE_INPUT;
    g.Pull = GPIO_NOPULL; // 외부 풀업
    HAL_GPIO_Init(MYGPIO_DP_SENSOR_GPIO_PORT, &g);
}

static void I2C_SDA_Dir_Out( void ) {
    GPIO_InitTypeDef g = {0};
    g.Pin = MYGPIO_DP_SENSOR_SDA_GPIO_NUM;
    g.Mode = GPIO_MODE_OUTPUT_OD; // 오픈드레인 권장
    g.Pull = GPIO_NOPULL;
    g.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(MYGPIO_DP_SENSOR_GPIO_PORT, &g);
}

static void I2C_SDA_Dir_In( void ) {
    GPIO_InitTypeDef g = {0};
    g.Pin = MYGPIO_DP_SENSOR_SDA_GPIO_NUM;
    g.Mode = GPIO_MODE_INPUT;
    g.Pull = GPIO_NOPULL; // 외부 풀업
    HAL_GPIO_Init(MYGPIO_DP_SENSOR_GPIO_PORT, &g);
}

//
uint8_t Check_HandShake(void)
{
	I2C_SCL_Dir_In();
	if( SCL_Status() == 1 ){
		SCL_Low();
		I2C_Busy = 0;
		I2C_Delay(10);
	}
	else{
		I2C_Busy = 1;
	}
		
	I2C_SCL_Dir_Out();
	return I2C_Busy;
}

//
uint8_t I2C_Send_Byte(uint8_t dat)
{
	int8_t i;
	
	for( i = 7; i >= 0; i-- )
	{
		I2C_Delay(2);
		if( ( dat >> i ) & 0x01 ){
			SDA_High();
		}
		else{
		 	SDA_Low();
		 }
		I2C_Delay(2);
		SCL_High();
		I2C_Delay(5);
		SCL_Low();
	}
	
	SDA_High();
	I2C_Delay(5);
	
	I2C_SDA_Dir_In();
	if( SDA_Status() == 1 ){
		I2C_SDA_Dir_Out();
		return 1;
	}
	I2C_SDA_Dir_Out();
	SCL_High();
	I2C_Delay(5);
	
	return Check_HandShake();
}

//
uint8_t I2C_Read_Byte(void)
{
	uint8_t i;
	uint8_t cnret = 0;
	
	I2C_SDA_Dir_In();
	for( i = 0; i < 8; i++ )
	{
		I2C_Delay(2);		
		cnret = ( cnret << 1 ) | SDA_Status();				
		I2C_Delay(2);		
		
		SCL_High();
		I2C_Delay(5);
		SCL_Low();
	}
	I2C_SDA_Dir_Out();
	
	I2C_Delay(2);
	SDA_Low();
	I2C_Delay(2);
	
	SCL_High();
	I2C_Delay(2);
	SCL_Low();
	I2C_Delay(2);
	SDA_High();
	
	return cnret;	
}

//
uint8_t I2C_Read_Byte_END(void)
{
	uint8_t i;
	uint8_t cnret = 0;
	
	I2C_SDA_Dir_In();
	for(i=0;i<8;i++)
	{
		I2C_Delay(2);		
		cnret = (cnret<<1) | SDA_Status();				
		I2C_Delay(2);		
		
		SCL_High();
		I2C_Delay(5);
		SCL_Low();

	}
	I2C_SDA_Dir_Out();
	
	I2C_Delay(2);
	SDA_High();
	I2C_Delay(2);
	SCL_High();
	I2C_Delay(2);
	SCL_Low();
	I2C_Delay(2);
		
	return cnret;	
}

void I2C_Scl_Set_High( void )
{
	SCL_High();
}

/////////////////////////////////////////////////////////////////////////////////////////////
// local function implement