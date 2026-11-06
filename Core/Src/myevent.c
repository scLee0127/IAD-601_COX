#include "global.h"
#include "myevent.h"
#include "mygpio.h"



/////////////////////////////////////////////////////////////////////////////////////////////
// import function prototype


/////////////////////////////////////////////////////////////////////////////////////////////
// local function prototype


/////////////////////////////////////////////////////////////////////////////////////////////
// global variable

/////////////////////////////////////////////////////////////////////////////////////////////
// public and export function implement

void MYEVENT_Proc( void )
{
	uint8_t cngidong_act_f = 0;
	uint8_t cnmanual_act_f = 0;

	if( PIN_STATE_ON == MYGPIO_Get_Gidong_State() ){
		cngidong_act_f = 1;
		MYGPIO_Led_Control( LED_SELECT_GIDONG, LED_CTRL_STATE_ON );
		MYGPIO_Relay_Control( RELAY_SEL_GIDONG, PIN_STATE_ON );
	}
	else{
		MYGPIO_Led_Control( LED_SELECT_GIDONG, LED_CTRL_STATE_OFF );
		MYGPIO_Relay_Control( RELAY_SEL_GIDONG, PIN_STATE_OFF );

	}

	if( PIN_STATE_ON == MYGPIO_Get_Manual_State() ){
		cnmanual_act_f = 1;
		MYGPIO_Led_Control( LED_SELECT_MANUAL, LED_CTRL_STATE_ON );
		MYGPIO_Relay_Control( RELAY_SEL_MANUAL, PIN_STATE_ON );
		// MYGPIO_Motor_Control( PIN_STATE_ON );
	}
	else{
		MYGPIO_Led_Control( LED_SELECT_MANUAL, LED_CTRL_STATE_OFF );
		MYGPIO_Relay_Control( RELAY_SEL_MANUAL, PIN_STATE_OFF );
		// MYGPIO_Motor_Control( PIN_STATE_OFF );
	}

	if( ( 1 == cngidong_act_f ) || ( 1 == cnmanual_act_f ) ){
		MYGPIO_Run_Buzzer( BUZZER_PLAY_TYPE_SIGNAL );
		gcnFireEvent_f = 1;
	}
	else{
		MYGPIO_Stop_Buzzer();
		gcnFireEvent_f = 0;
	}

}

/////////////////////////////////////////////////////////////////////////////////////////////
// local function implement