#ifndef __MYTIMER_H__
#define __MYTIMER_H__

extern TIM_HandleTypeDef htim6;  		// 7Segment Timer
#if BOARD_MOTOR_ENABLE
extern TIM_HandleTypeDef htim1;  		// PB13 TIM1_CH1N motor PWM
#endif

void MYTIMER_Init( void );
void MYTIMER_Proc( void );
void MYPWM_Set_Duty( uint8_t _cnChannel, float _nfduty );
#endif // __MYTIMER_H__
