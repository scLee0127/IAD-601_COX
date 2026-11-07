#ifndef __MYMOTOR_H__
#define __MYMOTOR_H__

#include <stdint.h>

/* Watch gMotorOcpDebug after a trip; captured before outputs are stopped.
 * reason: bit 0 = ADC saturation, bit 1 = current above configured limit.
 * count increments last, once per newly latched trip.
 */
typedef struct {
    uint32_t count;
    uint32_t tick_ms;
    uint32_t reason;
    float current_a;
    float limit_a;
    float pa6_v;
    int command_dir;
    int applied_dir;
    float applied_duty;
    uint32_t gpio_b_idr;
    uint32_t tim1_ccr1;
    uint32_t tim1_cnt;
    uint32_t tim1_cr1;
    uint32_t tim1_ccer;
    uint32_t tim1_bdtr;
} MOTOR_OCP_DEBUG_T;
extern volatile MOTOR_OCP_DEBUG_T gMotorOcpDebug;

#define LIMIT_SWITCH_STATE_OFF 			0
#define LIMIT_SWITCH_STATE_ON 			1
void MYMOTOR_Init( void );
void MYMOTOR_Apply_Settings( void );
void MYMOTOR_Proc( void );
float MYMOTOR_Get_Motor_Pwm( void );
uint8_t MYMOTOR_Get_Open_LimitSwtich_St( void );
uint8_t MYMOTOR_Get_Close_LimitSwtich_St( void );
#endif // __MYMOTOR_H__
