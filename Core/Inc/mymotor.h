#ifndef __MYMOTOR_H__
#define __MYMOTOR_H__

#define LIMIT_SWITCH_STATE_OFF 			0
#define LIMIT_SWITCH_STATE_ON 			1
void MYMOTOR_Init( void );
void MYMOTOR_Apply_Settings( void );
void MYMOTOR_Proc( void );
float MYMOTOR_Get_Motor_Pwm( void );
uint8_t MYMOTOR_Get_Open_LimitSwtich_St( void );
uint8_t MYMOTOR_Get_Close_LimitSwtich_St( void );
#endif // __MYMOTOR_H__
