#ifndef __MYADC_H__
#define __MYADC_H__

void MYADC_Init( void );
float ADC0_ReadVoltage(void);
float ADC0_Get_Current( void );
void MYADC_Proc( void );
#endif // __MYADC_H__