#ifndef __MY7SEGMENT_H__
#define __MTY7SEGMENT_H__

typedef enum {
	FND_0,
	FND_1,
	FND_2,
	FND_3,
	FND_4,
	FND_5,
	FND_6,
	FND_7,
	FND_8,
	FND_9,
	FND_DOT,
	FND_0_DOT,
	FND_1_DOT,
	FND_2_DOT,
	FND_3_DOT,
	FND_4_DOT,
	FND_5_DOT,
	FND_6_DOT,
	FND_7_DOT,
	FND_8_DOT,
	FND_9_DOT,
	FND_SP,
	FND_L,
	FND_o,
	FND_H,
	FND_BAR_HIGH,
	FND_BAR_MIDDLE,
	FND_BAR_LOW,
	FND_E,
	FND_n,
	FND_P,
	FND_P_DOT,
	FND_u,
	FND_F,
	FND_F_DOT,
	FND_r,
	FND_C,
	FND_c,
	FND_R,
	FND_d,
	FND_A,
	FND_t,
	FND_b,
	FND_U,
	FND_K
}eFND_CHAR;

#define SEVEN_SEG_BLINK_OFF  		0
#define SEVEN_SEG_BLINK_DIGIT_1  	1
#define SEVEN_SEG_BLINK_DIGIT_2  	2
#define SEVEN_SEG_BLINK_DIGIT_3  	3

#define SEVEN_SEG_NO_BAR  			0
#define SEVEN_SEG_USING_BAR_1  		1
#define SEVEN_SEG_USING_BAR_2  		2
#define SEVEN_SEG_USING_BAR_3  		3
#define SEVEN_SEG_USING_BAR_P  		4
#define SEVEN_SEG_USING_BAR_N  		5
#define SEVEN_SEG_USING_SP  		6

#define SEVEN_SEG_NO_DOT  			0  		// 일의자리 소수점 표시
#define SEVEN_SEG_USING_DOT  		1

void MY7SEGMENT_Test( void );
void MY7SEGMENT_Proc( void );
void MY7SEGMENT_Set_Value( uint8_t _cnDig1_val, uint8_t _cnDig2_val, uint8_t _cnDig3_val, uint8_t _cnToggleIdx );
void MY7SEGMENT_Set_3Digit_Value( int _nVal, uint8_t _cnBarIdx, uint8_t _cnDot_f, uint8_t _cnToggleIdx );

#endif // __MY7SEGMENT_H__