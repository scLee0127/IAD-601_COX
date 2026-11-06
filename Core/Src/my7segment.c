#include "main.h"
#include "global.h"
#include "mygpio.h"
#include "my7segment.h"


/////////////////////////////////////////////////////////////////////////////////////////////
// import function prototype


/////////////////////////////////////////////////////////////////////////////////////////////
// local function prototype
void _my7segment_data( uint8_t _cnData );
void _my7segment_digit( uint8_t _cnPos );
void _my7segment_disp_fnd( uint8_t _cnDigit, uint8_t _cnData );

/////////////////////////////////////////////////////////////////////////////////////////////
// global variable
#define SEVEN_SEG_DIGIT_ALL 		0
#define SEVEN_SEG_DIGIT_1 			1
#define SEVEN_SEG_DIGIT_2 			2
#define SEVEN_SEG_DIGIT_3 			3


#define SEVEN_SEG_REFRESH_TIME_MS  	1 		// 2miliseconds

uint8_t FND_TABLE[] = {
  0xC0, // 0
  0xF9, // 1
  0xA4, // 2
  0xB0, // 3
  0x99, // 4
  0x92, // 5
  0x82, // 6
  0xD8, // 7
  0x80, // 8
  0x90, // 9
  0x7F, // dot
  0x40, // 0.
  0x79, // 1.
  0x24, // 2.
  0x30, // 3.
  0x19, // 4.
  0x12, // 5.
  0x02, // 6.
  0x78, // 7.
  0x00, // 8.
  0x10, // 9.
  0xFF, // Empty
  0xC7, // L  
  0xA3, // o (lower case) 
  0x89, // H
  0xFE, // M1
  0xBF, // M2 & minus
  0xF7, // M3
  0x86, // E 
  0xAB, // n
  0x8C, // P
  0x0C, // P. 
  0xE3, // u
  0x8E, // F
  0x0E, // F.
  0xAF, // r
  0xC6, // C
  0xA7, // c
  0x88,	// R
  0xA1, // d
  0x88, // A
  0x87, // t
  0x83, // b
  0xC1, // U
  0x89
};

uint32_t gnEnd_Tick = 0;
uint8_t gcnFndIdx = 0;
uint8_t gabFndBuff[4] = {0,};
uint8_t gcnFndToggleIdx = 0;
/////////////////////////////////////////////////////////////////////////////////////////////
// public and export function implement
// void MY7SEGMENT_Test( void )
// {
// 	uint32_t nstart_tick = HAL_GetTick();

// 	if( SEVEN_SEG_REFRESH_TIME_MS < ( nstart_tick - gnEnd_Tick ) ){

// 		gcnFndIdx++;
// 		if( 2 < gcnFndIdx ){ gcnFndIdx = 0; }

// 		switch( gcnFndIdx & 0x7F )
// 		{
// 			case 0:
// 				_my7segment_disp_fnd( SEVEN_SEG_DIGIT_1, FND_TABLE[0] );
// 				break;
// 			case 1:
// 				_my7segment_disp_fnd( SEVEN_SEG_DIGIT_2, FND_TABLE[1] );
// 				break;
// 			case 2:
// 				_my7segment_disp_fnd( SEVEN_SEG_DIGIT_3, FND_TABLE[2] );
// 				break;
// 			default:
// 				break;
// 		}
// 		gnEnd_Tick = nstart_tick;
// 	}
// }

void MY7SEGMENT_Proc( void )
{
	gcnFndIdx++;
	if( 2 < gcnFndIdx ){ gcnFndIdx = 0; }

	switch( gcnFndIdx & 0x7F )
	{
		case 0:
			if( ( SEVEN_SEG_BLINK_DIGIT_1 == gcnFndToggleIdx ) && ( gnFndBlinkTimer & 0x100 ) ){
				_my7segment_disp_fnd( SEVEN_SEG_DIGIT_1, FND_TABLE[FND_SP] );
			}
			else{
				_my7segment_disp_fnd( SEVEN_SEG_DIGIT_1, gabFndBuff[0] );
			}
			break;
		case 1:
			if( ( SEVEN_SEG_BLINK_DIGIT_2 == gcnFndToggleIdx ) && ( gnFndBlinkTimer & 0x100 ) ){
				_my7segment_disp_fnd( SEVEN_SEG_DIGIT_2, FND_TABLE[FND_SP] );
			}
			else{
				_my7segment_disp_fnd( SEVEN_SEG_DIGIT_2, gabFndBuff[1] );
			}
			break;
		case 2:
			if( ( SEVEN_SEG_BLINK_DIGIT_3 == gcnFndToggleIdx ) && ( gnFndBlinkTimer & 0x100 ) ){
				_my7segment_disp_fnd( SEVEN_SEG_DIGIT_3, FND_TABLE[FND_SP] );
			}
			else{
				_my7segment_disp_fnd( SEVEN_SEG_DIGIT_3, gabFndBuff[2] );
			}
			break;
		default:
			break;
	}
}

void MY7SEGMENT_Set_Value( uint8_t _cnDig1_val, uint8_t _cnDig2_val, uint8_t _cnDig3_val, uint8_t _cnToggleIdx )
{
	gabFndBuff[0] = FND_TABLE[_cnDig1_val];
	gabFndBuff[1] = FND_TABLE[_cnDig2_val];
	gabFndBuff[2] = FND_TABLE[_cnDig3_val];
	gcnFndToggleIdx = _cnToggleIdx;
}

 // (nsensor_val / 100) % 10, (nsensor_val / 10) % 10, nsensor_val % 10
void MY7SEGMENT_Set_3Digit_Value( int _nVal, uint8_t _cnBarIdx, uint8_t _cnDot_f, uint8_t _cnToggleIdx )
{
	uint8_t cntmp = 0;
	if( SEVEN_SEG_NO_BAR == _cnBarIdx ){
		cntmp = ( _nVal / 100 )  % 10;
		gabFndBuff[0] = FND_TABLE[cntmp];
	}
	else
	if( SEVEN_SEG_USING_BAR_1 == _cnBarIdx ){
		gabFndBuff[0] = FND_TABLE[FND_BAR_HIGH];	
	}
	else
	if( SEVEN_SEG_USING_BAR_2 == _cnBarIdx ){
		gabFndBuff[0] = FND_TABLE[FND_BAR_MIDDLE];	
	}
	else
	if( SEVEN_SEG_USING_BAR_3 == _cnBarIdx ){
		gabFndBuff[0] = FND_TABLE[FND_BAR_LOW];	
	}
	else
	if( SEVEN_SEG_USING_BAR_P == _cnBarIdx ){
		gabFndBuff[0] = FND_TABLE[FND_P];	
	}
	else
	if( SEVEN_SEG_USING_BAR_N == _cnBarIdx ){
		gabFndBuff[0] = FND_TABLE[FND_n];	
	}
	else{
		gabFndBuff[0] = FND_TABLE[FND_SP];	
	}

	// 음수에 대한 예외사항 처리
	cntmp = ( _nVal / 10 ) % 10;
	if( SEVEN_SEG_USING_DOT == _cnDot_f ){
		cntmp += ( FND_DOT + 1 );
	}
	gabFndBuff[1] = FND_TABLE[cntmp];

	cntmp = _nVal % 10;
	gabFndBuff[2] = FND_TABLE[cntmp];
	gcnFndToggleIdx = _cnToggleIdx;
}

// HAL_GPIO_WritePin( MYGPIO_RS485_ENABLE_GPIO_PORT, MYGPIO_RS485_ENABLE_GPIO_NUM, GPIO_PIN_RESET );
// 	/* 7Segment GPIO */
// 	GPIO_InitStruct.Pin = MYGPIO_7SEG_DIGIT1_GPIO_NUM | MYGPIO_7SEG_DIGIT2_GPIO_NUM | MYGPIO_7SEG_DIGIT3_GPIO_NUM 
// 						| MYGPIO_7SEG_A_GPIO_NUM | MYGPIO_7SEG_B_GPIO_NUM | MYGPIO_7SEG_C_GPIO_NUM
// 						| MYGPIO_7SEG_D_GPIO_NUM | MYGPIO_7SEG_E_GPIO_NUM | MYGPIO_7SEG_F_GPIO_NUM 
// 						| MYGPIO_7SEG_G_GPIO_NUM | MYGPIO_7SEG_DP_GPIO_NUM;

/////////////////////////////////////////////////////////////////////////////////////////////
// local function implement
void _my7segment_disp_fnd( uint8_t _cnDigit, uint8_t _cnData )
{
	_my7segment_digit( _cnDigit );
	_my7segment_data( _cnData );
}

void _my7segment_digit( uint8_t _cnPos )
{
	if( SEVEN_SEG_DIGIT_ALL == _cnPos ){
		HAL_GPIO_WritePin( MYGPIO_7SEG_GPIO_PORT, MYGPIO_7SEG_DIGIT1_GPIO_NUM, GPIO_PIN_SET );
		HAL_GPIO_WritePin( MYGPIO_7SEG_GPIO_PORT, MYGPIO_7SEG_DIGIT2_GPIO_NUM, GPIO_PIN_SET );
		HAL_GPIO_WritePin( MYGPIO_7SEG_GPIO_PORT, MYGPIO_7SEG_DIGIT3_GPIO_NUM, GPIO_PIN_SET );
	}
	else
	if( SEVEN_SEG_DIGIT_1 == _cnPos ){
		HAL_GPIO_WritePin( MYGPIO_7SEG_GPIO_PORT, MYGPIO_7SEG_DIGIT1_GPIO_NUM, GPIO_PIN_RESET );
		HAL_GPIO_WritePin( MYGPIO_7SEG_GPIO_PORT, MYGPIO_7SEG_DIGIT2_GPIO_NUM, GPIO_PIN_SET );
		HAL_GPIO_WritePin( MYGPIO_7SEG_GPIO_PORT, MYGPIO_7SEG_DIGIT3_GPIO_NUM, GPIO_PIN_SET );
	}
	else
	if( SEVEN_SEG_DIGIT_2 == _cnPos ){
		HAL_GPIO_WritePin( MYGPIO_7SEG_GPIO_PORT, MYGPIO_7SEG_DIGIT1_GPIO_NUM, GPIO_PIN_SET );
		HAL_GPIO_WritePin( MYGPIO_7SEG_GPIO_PORT, MYGPIO_7SEG_DIGIT2_GPIO_NUM, GPIO_PIN_RESET );
		HAL_GPIO_WritePin( MYGPIO_7SEG_GPIO_PORT, MYGPIO_7SEG_DIGIT3_GPIO_NUM, GPIO_PIN_SET );
	}
	else
	if( SEVEN_SEG_DIGIT_3 == _cnPos ){
		HAL_GPIO_WritePin( MYGPIO_7SEG_GPIO_PORT, MYGPIO_7SEG_DIGIT1_GPIO_NUM, GPIO_PIN_SET );
		HAL_GPIO_WritePin( MYGPIO_7SEG_GPIO_PORT, MYGPIO_7SEG_DIGIT2_GPIO_NUM, GPIO_PIN_SET );
		HAL_GPIO_WritePin( MYGPIO_7SEG_GPIO_PORT, MYGPIO_7SEG_DIGIT3_GPIO_NUM, GPIO_PIN_RESET );
	}
}

void _my7segment_data( uint8_t _cnData )
{
	// MYUTIL_Printf("%02x %02x\r\n", _cnData);
	HAL_GPIO_WritePin( MYGPIO_7SEG_DATA_GPIO_PORT
					 , MYGPIO_7SEG_A_GPIO_NUM | MYGPIO_7SEG_B_GPIO_NUM | MYGPIO_7SEG_C_GPIO_NUM
					 | MYGPIO_7SEG_D_GPIO_NUM | MYGPIO_7SEG_E_GPIO_NUM | MYGPIO_7SEG_F_GPIO_NUM 
					 | MYGPIO_7SEG_G_GPIO_NUM | MYGPIO_7SEG_DP_GPIO_NUM
					 , GPIO_PIN_RESET );

	if( 0x01 & _cnData ){
		HAL_GPIO_WritePin( MYGPIO_7SEG_DATA_GPIO_PORT, MYGPIO_7SEG_A_GPIO_NUM, GPIO_PIN_SET );
	}
	if( 0x02 & _cnData ){
		HAL_GPIO_WritePin( MYGPIO_7SEG_DATA_GPIO_PORT, MYGPIO_7SEG_B_GPIO_NUM, GPIO_PIN_SET );
	}
	if( 0x04 & _cnData ){
		HAL_GPIO_WritePin( MYGPIO_7SEG_DATA_GPIO_PORT, MYGPIO_7SEG_C_GPIO_NUM, GPIO_PIN_SET );
	}
	if( 0x08 & _cnData ){
		HAL_GPIO_WritePin( MYGPIO_7SEG_DATA_GPIO_PORT, MYGPIO_7SEG_D_GPIO_NUM, GPIO_PIN_SET );
	}
	if( 0x10 & _cnData ){
		HAL_GPIO_WritePin( MYGPIO_7SEG_DATA_GPIO_PORT, MYGPIO_7SEG_E_GPIO_NUM, GPIO_PIN_SET );
	}
	if( 0x20 & _cnData ){
		HAL_GPIO_WritePin( MYGPIO_7SEG_DATA_GPIO_PORT, MYGPIO_7SEG_F_GPIO_NUM, GPIO_PIN_SET );
	}
	if( 0x40 & _cnData ){
		HAL_GPIO_WritePin( MYGPIO_7SEG_DATA_GPIO_PORT, MYGPIO_7SEG_G_GPIO_NUM, GPIO_PIN_SET );
	}
	if( 0x80 & _cnData ){
		HAL_GPIO_WritePin( MYGPIO_7SEG_DATA_GPIO_PORT, MYGPIO_7SEG_DP_GPIO_NUM, GPIO_PIN_SET );
	}
}
