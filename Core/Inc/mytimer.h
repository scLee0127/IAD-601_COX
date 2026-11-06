#ifndef __MYTIMER_H__
#define __MYTIMER_H__

extern TIM_HandleTypeDef htim3;  		// PB0 PWM Timer
extern TIM_HandleTypeDef htim6;  		// 7Segment Timer
extern TIM_HandleTypeDef htim14;  		// PB1 PWM Timer

#define PWM3_ARR() __HAL_TIM_GET_AUTORELOAD( &htim3 )
#define PWM14_ARR() __HAL_TIM_GET_AUTORELOAD( &htim14 )

void MYTIMER_Init( void );
void MYTIMER_Proc( void );
void MYPWM_Set_Duty( uint8_t _cnChannel, float _nfduty );
#endif // __MYTIMER_H__