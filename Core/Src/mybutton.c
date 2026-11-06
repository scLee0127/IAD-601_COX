#include "main.h"
#include "global.h"
#include "mygpio.h"
#include "mybutton.h"

/////////////////////////////////////////////////////////////////////////////////////////////
// import function prototype


/////////////////////////////////////////////////////////////////////////////////////////////
// local function prototype

/////////////////////////////////////////////////////////////////////////////////////////////
// global variable
#define MYBTN_PROC_REFRESH_TIME_MS  				100
#define MYBTN_PROC_LONGKEY_TIME  					20  		// 1.5seconds
// #define MYBTN_STATE_IDLE  							0
// #define MYBTN_STATE_CLICK_UP  						1
// #define MYBTN_STATE_CLICK_SEL  						2
// #define MYBTN_STATE_LONGKEY_UP  					3
// #define MYBTN_STATE_LONGKEY_SEL  					4

#define MYBTN_PUSH_STATE 							0
#define MYBTN_PULL_STATE 							1
static uint8_t gcnBtn_Up_St_prev = MYBTN_PULL_STATE;
static uint8_t gcnBtn_Dwon_St_prev = MYBTN_PULL_STATE;
static uint8_t gcnBtn_Sel_St_prev = MYBTN_PULL_STATE;
static uint16_t gnBtn_Sel_Longkey_Count = 0;
static uint8_t gcnBtn_Sel_Longkey_f = 0;
static uint16_t gnBtn_Up_Longkey_Count = 0;
static uint8_t gcnBtn_Up_Longkey_f = 0;
static uint16_t gnBtn_Down_Longkey_Count = 0;
static uint8_t gcnBtn_Down_Longkey_f = 0;
/////////////////////////////////////////////////////////////////////////////////////////////
// public and export function implement
uint8_t MYBTN_Proc( void )
{
	uint8_t cnbtn_up_st = 0;
	uint8_t cnbtn_down_st = 0;
	uint8_t cnbtn_sel_st = 0;
	uint8_t cnret = MYBTN_STATE_IDLE;
	if( 0 != gnBtnTimer ){ return MYBTN_STATE_IDLE; }
	gnBtnTimer = MYBTN_PROC_REFRESH_TIME_MS;

	cnbtn_up_st = HAL_GPIO_ReadPin( MYGPIO_BTN_GPIO_PORT, MYGPIO_BTN_UP_GPIO_NUM );
	cnbtn_down_st = HAL_GPIO_ReadPin( MYGPIO_BTN_GPIO_PORT, MYGPIO_BTN_DOWN_GPIO_NUM );
	cnbtn_sel_st = HAL_GPIO_ReadPin( MYGPIO_BTN_GPIO_PORT, MYGPIO_BTN_SEL_GPIO_NUM );
	// MYUTIL_Printf("up %d, sel %d\r\n", cnbtn_up_st, cnbtn_sel_st);

	if( ( MYBTN_PULL_STATE == cnbtn_up_st) && ( MYBTN_PUSH_STATE == gcnBtn_Up_St_prev ) ){
		if( 1 != gcnBtn_Up_Longkey_f ){
			MYUTIL_Printf("Click UP\r\n");
			cnret = MYBTN_STATE_CLICK_UP;
		}
		gcnBtn_Up_Longkey_f = 0;
		gnBtn_Up_Longkey_Count = 0;
	}
	else
	if( ( MYBTN_PUSH_STATE == cnbtn_up_st) && ( MYBTN_PUSH_STATE == gcnBtn_Up_St_prev ) ){
		if( 0 == gcnBtn_Up_Longkey_f ){
			if( MYBTN_PROC_LONGKEY_TIME < gnBtn_Up_Longkey_Count ){
				gcnBtn_Up_Longkey_f = 1;
				MYUTIL_Printf("Long key UP\r\n");
				cnret = MYBTN_STATE_LONGKEY_UP;
			}
			else{
				gnBtn_Up_Longkey_Count++;
			}
		}
	}
	else{
		gnBtn_Up_Longkey_Count = 0;
		gcnBtn_Up_Longkey_f = 0;
	}

	if( ( MYBTN_PULL_STATE == cnbtn_down_st) && ( MYBTN_PUSH_STATE == gcnBtn_Dwon_St_prev ) ){
		if( 1 != gcnBtn_Down_Longkey_f ){
			MYUTIL_Printf("Click DOWN\r\n");
			cnret = MYBTN_STATE_CLICK_DOWN;
		}
		gcnBtn_Down_Longkey_f = 0;
		gnBtn_Down_Longkey_Count = 0;
	}
	else
	if( ( MYBTN_PUSH_STATE == cnbtn_down_st) && ( MYBTN_PUSH_STATE == gcnBtn_Dwon_St_prev ) ){
		if( 0 == gcnBtn_Down_Longkey_f ){
			if( MYBTN_PROC_LONGKEY_TIME < gnBtn_Down_Longkey_Count ){
				gcnBtn_Down_Longkey_f = 1;
				MYUTIL_Printf("Long key DOWN\r\n");
				cnret = MYBTN_STATE_LONGKEY_DOWN;
			}
			else{
				gnBtn_Down_Longkey_Count++;
			}
		}
	}
	else{
		gnBtn_Down_Longkey_Count = 0;
		gcnBtn_Down_Longkey_f = 0;
	}

	if( ( MYBTN_PULL_STATE == cnbtn_sel_st) && ( MYBTN_PUSH_STATE == gcnBtn_Sel_St_prev ) ){
		if( 1 != gcnBtn_Sel_Longkey_f ){
			MYUTIL_Printf("Click SEL\r\n");
			cnret = MYBTN_STATE_CLICK_SEL;
			if( 1 == gcnMotorOverCurrent_f ){
				gcnMotorOverCurrent_f = 0;
			}
		}
		gnBtn_Sel_Longkey_Count = 0;
		gcnBtn_Sel_Longkey_f = 0;
	}
	else
	if( ( MYBTN_PUSH_STATE == cnbtn_sel_st) && ( MYBTN_PUSH_STATE == gcnBtn_Sel_St_prev ) ){
		if( 0 == gcnBtn_Sel_Longkey_f ) {
			if( MYBTN_PROC_LONGKEY_TIME < gnBtn_Sel_Longkey_Count ){
				gcnBtn_Sel_Longkey_f = 1;
				MYUTIL_Printf("Long key SEL\r\n");
				cnret = MYBTN_STATE_LONGKEY_SEL;
			}
			else{
				gnBtn_Sel_Longkey_Count++;

			}
		}
	}
	else{
		gnBtn_Sel_Longkey_Count = 0;
		gcnBtn_Sel_Longkey_f = 0;
	}
	gcnBtn_Up_St_prev = cnbtn_up_st;
	gcnBtn_Dwon_St_prev = cnbtn_down_st;
	gcnBtn_Sel_St_prev = cnbtn_sel_st;

	if( cnret != MYBTN_STATE_IDLE ){
		MYUTIL_Printf("--->> Not Idle %d\r\n", cnret);
		MYGPIO_Run_Buzzer( BUZZER_PLAY_TYPE_BEEP );
	}
	return cnret;
}

/////////////////////////////////////////////////////////////////////////////////////////////
// local function implement
