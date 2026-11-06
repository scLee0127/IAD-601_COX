#ifndef __MYI2C_H__
#define __MYI2C_H__

void I2C_Delay( uint32_t _cnDelay );
void I2C_Start_Condition( void );
void I2C_Stop_Condition( void );
void I2C_Restart_Condition( void );
uint8_t I2C_Send_Byte( uint8_t dat );
uint8_t I2C_Read_Byte( void );
uint8_t I2C_Read_Byte_END( void );
void I2C_Scl_Set_High( void );

#endif // __MYI2C_H__