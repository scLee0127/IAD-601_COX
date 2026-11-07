#include "global.h"
#include "mygui.h"
#include "my7segment.h"
#include "mybutton.h"
#include "enviroment.h"
#include "mygpio.h"
#include "mymotor.h"
/////////////////////////////////////////////////////////////////////////////////////////////
// import function prototype


/////////////////////////////////////////////////////////////////////////////////////////////
// local function prototype


/////////////////////////////////////////////////////////////////////////////////////////////
// global variable
uint8_t gcnDispState = 0;

#define MYGUI_RETURN_MENU_TIME_MS 			10000
enum eMAIN_MODE
{
    MYGUI_STATE_IDLE = 0,   		/* 차압값 표시 */
    MYGUI_STATE_SAVE,
    MYGUI_STATE_SETTING,
    MYGUI_STATE_CHECK_LIMIT_SWITCH
};

enum eSETTING_MODE
{
	MYGUI_STATE_SET_RS485_ID,  			/* Id  안정화 +- 차압 */
    MYGUI_STATE_SET_TARGET_PA,  		/* PA  목표 차압 */
    MYGUI_STATE_SET_SAFTY_PA,  			/* SPA 안정화 +- 차압 */
    MYGUI_STATE_SET_SAFTY_UPPER_RAMP, 	/* SuR 안정화 증가 계수 */
    MYGUI_STATE_SET_SAFTY_DWON_RAMP, 	/* SdR 안정화 감소 게수 */
    MYGUI_STATE_SET_DEADBAND_PA,   		/* dPA 안정화 +- 차압 */
    MYGUI_STATE_SET_DEADBAND_MIN_PA,    /* nPa 정지 구간 */
    MYGUI_STATE_SET_DEADBAND_PERIOD,   	/* dPt 데드밴드 주기 */
    MYGUI_STATE_SET_DEADBAND_UPPER_TICK,   	/* dtt 데드밴드 Tick 시간(ms) */
	MYGUI_STATE_SET_DEADBAND_DOWN_TICK,   	/* dtt 데드밴드 Tick 시간(ms) */
    MYGUI_STATE_SET_NORMAL_UPPER_RAMP,  /* nuR 일반구간 증가 계수 */
    MYGUI_STATE_SET_NORMAL_DOWN_RAMP,   /* ndR 일반구간 감소 계수 */
    MYGUI_STATE_SET_MIN_UPPER_SPEED,  		/* LSd Min Upper Speed */
	MYGUI_STATE_SET_MIN_DOWN_SPEED,  		/* LSd Min Down Speed */
    MYGUI_STATE_SET_MAX_UPPER_SPEED,  		/* HSd Max Upper Speed */
	MYGUI_STATE_SET_MAX_DOWN_SPEED,  		/* HSd Max Down Speed */
    MYGUI_STATE_SET_PID_P,
    MYGUI_STATE_SET_PID_I,
    MYGUI_STATE_SET_PID_D,
    MYGUI_STATE_SET_OVER_CURRENT,
    MYGUI_STATE_SET_NONC,
    MYGUI_STATE_SET_RS485_BAUD,
    /*MYGUI_STATE_SET_SAVE,*/
    MYGUI_STATE_MAX
};

static uint8_t gcnGuiMainState = MYGUI_STATE_IDLE;
static int8_t gcnGuiSettingState = MYGUI_STATE_SET_RS485_ID;
static uint8_t gcnGuiSetSubState = 0;
static int16_t gnSetValue_Tmp = 0;

////////////////////////////////
void _mygui_main_page_idle_proc( uint8_t _cnBtn )
{
	// BTN
	if( MYBTN_STATE_LONGKEY_SEL == _cnBtn ){
		ENV_Backup_To_Temperary();
		gcnGuiMainState = MYGUI_STATE_SETTING;
		gcnGuiSettingState = MYGUI_STATE_SET_RS485_ID;
		gcnGuiSetSubState = 0;
		gnGuiReturnTimer = MYGUI_RETURN_MENU_TIME_MS;
		MYGPIO_Run_Buzzer( BUZZER_PLAY_TYPE_BEEP );
	}
	else
	if( MYBTN_STATE_LONGKEY_DOWN == _cnBtn ){
		gcnGuiMainState = MYGUI_STATE_CHECK_LIMIT_SWITCH;
		gnGuiReturnTimer = MYGUI_RETURN_MENU_TIME_MS;
		MYGPIO_Run_Buzzer( BUZZER_PLAY_TYPE_BEEP );
	}

	// Display Proc
	if( 0 == gcnMotorOverCurrent_f ){
		MY7SEGMENT_Set_3Digit_Value( gnDispSensorValue, SEVEN_SEG_NO_BAR, SEVEN_SEG_NO_DOT, SEVEN_SEG_BLINK_OFF );
	}
	else{
		MY7SEGMENT_Set_Value( FND_0_DOT, FND_U, FND_C, SEVEN_SEG_BLINK_OFF );
	}
}

void _mygui_change_digit_hundreds( int16_t* _pnVal, int8_t _cnSelBtn )
{
	int16_t nval = *_pnVal;

	// 자리 추출
	int16_t nhundreds =  nval / 100;         // 100의 자리
	int16_t ntens     = (nval / 10) % 10;    // 10의 자리 유지
	int16_t nones     =  nval % 10;          // 1의 자리 유지

	if( _cnSelBtn > 0 ){
	    // +100
	    nhundreds = (nhundreds + 1) % 10;     // 0~9 순환
	}
	else if( _cnSelBtn < 0 ){
	    // -100
	    nhundreds -= 1;
	    if( nhundreds < 0 ) nhundreds = 9;    // 0 아래는 9로 롤오버
	}

	// 합성
	nval = (nhundreds * 100) + (ntens * 10) + nones;

	*_pnVal = nval;
}

void _mygui_change_digit_tens( int16_t* _pnVal, int8_t _cnSelBtn )
{
	int16_t nval = *_pnVal;
	int16_t nones = nval % 10;
	int16_t ntens = nval / 10;

	if( 0 < _cnSelBtn ){
		ntens = ( ntens + 1 ) % 10;
	}
	else
	if( 0 > _cnSelBtn ){
		ntens -= 1;
		if( 0 > ntens ){ ntens = 9; }
	}
	*_pnVal = ( ntens * 10 ) + nones;
}

void _mygui_change_digit_ones( int16_t* _pnVal, int8_t _cnSelBtn )
{
	int16_t nval = *_pnVal;
	int16_t ntmp = 0;
	int16_t ndigit = 0;
	if( 9 < nval ){
		ntmp = ( nval / 10 ) * 10;
	}
	if( 0 < _cnSelBtn ){
		nval = ( (nval % 10) + 1 ) % 10;
		nval += ntmp;

	}
	else
	if( 0 > _cnSelBtn ){
		ndigit = ( nval % 10 ) - 1;
		if( 0 > ndigit ){ ndigit = 9; }
		
		nval = ntmp + ndigit;
	}
	*_pnVal = nval;
}

void _mygui_main_page_set_proc( int8_t _cnUpdownBtn, int8_t _cnSelBtn )
{
	DEVICEENV* pstDevEnv_Tmp = Env_Get_Enviroment_Temperary();

	// Display Proc
	if( 0 == gcnGuiSetSubState ){
		gcnGuiSettingState -= _cnUpdownBtn;
		if( 0 > gcnGuiSettingState ){
			gcnGuiSettingState = MYGUI_STATE_SET_RS485_BAUD;
		}
		else
		if( ( MYGUI_STATE_MAX - 1 ) < gcnGuiSettingState ){
			gcnGuiSettingState = MYGUI_STATE_SET_RS485_ID;
		}
	}

	if( 0 == gnGuiReturnTimer ){
		_cnSelBtn = 2;  			// 버튼 입력이 없으면 메인화면으로 복귀
	}

	if( 1 == _cnSelBtn ){
		gcnGuiSetSubState += 1;
	}
	else
	if( 2 == _cnSelBtn ){
		MYGPIO_Run_Buzzer( BUZZER_PLAY_TYPE_BEEP );
		Env_Restore_From_Temperary();
		Env_Save_Flash();
		gcnGuiMainState = MYGUI_STATE_SAVE;
		gnGuiTimer = 1000;
	}

	// Display표시
	if( MYGUI_STATE_SET_RS485_ID == gcnGuiSettingState  ){
		if( 0 == gcnGuiSetSubState ){
			MY7SEGMENT_Set_Value( FND_1, FND_d, FND_SP, SEVEN_SEG_BLINK_OFF );
			gnSetValue_Tmp = pstDevEnv_Tmp->nDeviceID;
		}
		else
		if( 1 == gcnGuiSetSubState ){
			_mygui_change_digit_tens( &gnSetValue_Tmp, _cnUpdownBtn );
			MY7SEGMENT_Set_3Digit_Value( gnSetValue_Tmp, SEVEN_SEG_USING_SP, SEVEN_SEG_NO_DOT, SEVEN_SEG_BLINK_DIGIT_2 );
		}
		else
		if( 2 == gcnGuiSetSubState ){
			_mygui_change_digit_ones( &gnSetValue_Tmp, _cnUpdownBtn );
			
			MY7SEGMENT_Set_3Digit_Value( gnSetValue_Tmp, SEVEN_SEG_USING_SP, SEVEN_SEG_NO_DOT, SEVEN_SEG_BLINK_DIGIT_3 );
			
		}
		else
		if( 3 == gcnGuiSetSubState ){
			gcnGuiSetSubState = 0;
			pstDevEnv_Tmp->nDeviceID = gnSetValue_Tmp;
		}

	}
	else
	if( MYGUI_STATE_SET_TARGET_PA == gcnGuiSettingState  ){
		if( 0 == gcnGuiSetSubState ){
			MY7SEGMENT_Set_Value( FND_P, FND_A, FND_SP, SEVEN_SEG_BLINK_OFF );
			gnSetValue_Tmp = pstDevEnv_Tmp->nTargetPa;
		}
		else
		if( 1 == gcnGuiSetSubState ){
			_mygui_change_digit_tens( &gnSetValue_Tmp, _cnUpdownBtn );
			MY7SEGMENT_Set_3Digit_Value( gnSetValue_Tmp, SEVEN_SEG_USING_SP, SEVEN_SEG_NO_DOT, SEVEN_SEG_BLINK_DIGIT_2 );
		}
		else
		if( 2 == gcnGuiSetSubState ){
			_mygui_change_digit_ones( &gnSetValue_Tmp, _cnUpdownBtn );
			
			MY7SEGMENT_Set_3Digit_Value( gnSetValue_Tmp, SEVEN_SEG_USING_SP, SEVEN_SEG_NO_DOT, SEVEN_SEG_BLINK_DIGIT_3 );
			
		}
		else
		if( 3 == gcnGuiSetSubState ){
			gcnGuiSetSubState = 0;
			pstDevEnv_Tmp->nTargetPa = gnSetValue_Tmp;
		}
	}
	else
	if( MYGUI_STATE_SET_SAFTY_PA == gcnGuiSettingState  ){
		if( 0 == gcnGuiSetSubState ){
			MY7SEGMENT_Set_Value( FND_5, FND_P, FND_A, SEVEN_SEG_BLINK_OFF );
			gnSetValue_Tmp = pstDevEnv_Tmp->nSafetyPa;
		}
		else
		if( 1 == gcnGuiSetSubState ){
			_mygui_change_digit_tens( &gnSetValue_Tmp, _cnUpdownBtn );
			MY7SEGMENT_Set_3Digit_Value( gnSetValue_Tmp, SEVEN_SEG_USING_SP, SEVEN_SEG_NO_DOT, SEVEN_SEG_BLINK_DIGIT_2 );
		}
		else
		if( 2 == gcnGuiSetSubState ){
			_mygui_change_digit_ones( &gnSetValue_Tmp, _cnUpdownBtn );
			MY7SEGMENT_Set_3Digit_Value( gnSetValue_Tmp, SEVEN_SEG_USING_SP, SEVEN_SEG_NO_DOT, SEVEN_SEG_BLINK_DIGIT_3 );
		}
		else
		if( 3 == gcnGuiSetSubState ){
			gcnGuiSetSubState = 0;
			pstDevEnv_Tmp->nSafetyPa = gnSetValue_Tmp;
		}
	}
	else
	if( MYGUI_STATE_SET_SAFTY_UPPER_RAMP == gcnGuiSettingState  ){
		if( 0 == gcnGuiSetSubState ){
			MY7SEGMENT_Set_Value( FND_5, FND_u, FND_r, SEVEN_SEG_BLINK_OFF );
			gnSetValue_Tmp = pstDevEnv_Tmp->nSafetyUpperRamp;
		}
		else
		if( 1 == gcnGuiSetSubState ){
			_mygui_change_digit_hundreds( &gnSetValue_Tmp, _cnUpdownBtn );
			MY7SEGMENT_Set_3Digit_Value( gnSetValue_Tmp, SEVEN_SEG_NO_BAR, SEVEN_SEG_NO_DOT, SEVEN_SEG_BLINK_DIGIT_1 );
		}
		else
		if( 2 == gcnGuiSetSubState ){
			_mygui_change_digit_tens( &gnSetValue_Tmp, _cnUpdownBtn );
			MY7SEGMENT_Set_3Digit_Value( gnSetValue_Tmp, SEVEN_SEG_NO_BAR, SEVEN_SEG_NO_DOT, SEVEN_SEG_BLINK_DIGIT_2 );
		}
		else
		if( 3 == gcnGuiSetSubState ){
			_mygui_change_digit_ones( &gnSetValue_Tmp, _cnUpdownBtn );
			MY7SEGMENT_Set_3Digit_Value( gnSetValue_Tmp, SEVEN_SEG_NO_BAR, SEVEN_SEG_NO_DOT, SEVEN_SEG_BLINK_DIGIT_3 );
		}
		else
		if( 4 == gcnGuiSetSubState ){
			gcnGuiSetSubState = 0;
			pstDevEnv_Tmp->nSafetyUpperRamp = gnSetValue_Tmp;
		}
	}
	else
	if( MYGUI_STATE_SET_SAFTY_DWON_RAMP == gcnGuiSettingState  ){
		if( 0 == gcnGuiSetSubState ){
			MY7SEGMENT_Set_Value( FND_5, FND_d, FND_r, SEVEN_SEG_BLINK_OFF );
			gnSetValue_Tmp = pstDevEnv_Tmp->nSafetyDownRamp;
		}
		else
		if( 1 == gcnGuiSetSubState ){
			_mygui_change_digit_hundreds( &gnSetValue_Tmp, _cnUpdownBtn );
			MY7SEGMENT_Set_3Digit_Value( gnSetValue_Tmp, SEVEN_SEG_NO_BAR, SEVEN_SEG_NO_DOT, SEVEN_SEG_BLINK_DIGIT_1 );
		}
		else
		if( 2 == gcnGuiSetSubState ){
			_mygui_change_digit_tens( &gnSetValue_Tmp, _cnUpdownBtn );
			MY7SEGMENT_Set_3Digit_Value( gnSetValue_Tmp, SEVEN_SEG_NO_BAR, SEVEN_SEG_NO_DOT, SEVEN_SEG_BLINK_DIGIT_2 );
		}
		else
		if( 3 == gcnGuiSetSubState ){
			_mygui_change_digit_ones( &gnSetValue_Tmp, _cnUpdownBtn );
			MY7SEGMENT_Set_3Digit_Value( gnSetValue_Tmp, SEVEN_SEG_NO_BAR, SEVEN_SEG_NO_DOT, SEVEN_SEG_BLINK_DIGIT_3 );
		}
		else
		if( 4 == gcnGuiSetSubState ){
			gcnGuiSetSubState = 0;
			pstDevEnv_Tmp->nSafetyDownRamp = gnSetValue_Tmp;
		}
	}
	else
	if( MYGUI_STATE_SET_DEADBAND_PA == gcnGuiSettingState  ){
		if( 0 == gcnGuiSetSubState ){
			MY7SEGMENT_Set_Value( FND_d, FND_P, FND_A, SEVEN_SEG_BLINK_OFF );
			gnSetValue_Tmp = pstDevEnv_Tmp->nDeadbandPa;
		}
		else
		if( 1 == gcnGuiSetSubState ){
			_mygui_change_digit_tens( &gnSetValue_Tmp, _cnUpdownBtn );
			MY7SEGMENT_Set_3Digit_Value( gnSetValue_Tmp, SEVEN_SEG_USING_SP, SEVEN_SEG_NO_DOT, SEVEN_SEG_BLINK_DIGIT_2 );
		}
		else
		if( 2 == gcnGuiSetSubState ){
			_mygui_change_digit_ones( &gnSetValue_Tmp, _cnUpdownBtn );
			MY7SEGMENT_Set_3Digit_Value( gnSetValue_Tmp, SEVEN_SEG_USING_SP, SEVEN_SEG_NO_DOT, SEVEN_SEG_BLINK_DIGIT_3 );
		}
		else
		if( 3 == gcnGuiSetSubState ){
			gcnGuiSetSubState = 0;
			pstDevEnv_Tmp->nDeadbandPa = gnSetValue_Tmp;
		}
	}
	else
	if( MYGUI_STATE_SET_DEADBAND_MIN_PA == gcnGuiSettingState  ){
		if( 0 == gcnGuiSetSubState ){
			MY7SEGMENT_Set_Value( FND_n, FND_P, FND_A, SEVEN_SEG_BLINK_OFF );
			gnSetValue_Tmp = pstDevEnv_Tmp->nDeadBandTickMinPa;
		}
		else
		if( 1 == gcnGuiSetSubState ){
			_mygui_change_digit_tens( &gnSetValue_Tmp, _cnUpdownBtn );
			MY7SEGMENT_Set_3Digit_Value( gnSetValue_Tmp, SEVEN_SEG_USING_SP, SEVEN_SEG_NO_DOT, SEVEN_SEG_BLINK_DIGIT_2 );
		}
		else
		if( 2 == gcnGuiSetSubState ){
			_mygui_change_digit_ones( &gnSetValue_Tmp, _cnUpdownBtn );
			MY7SEGMENT_Set_3Digit_Value( gnSetValue_Tmp, SEVEN_SEG_USING_SP, SEVEN_SEG_NO_DOT, SEVEN_SEG_BLINK_DIGIT_3 );
		}
		else
		if( 3 == gcnGuiSetSubState ){
			gcnGuiSetSubState = 0;
			pstDevEnv_Tmp->nDeadBandTickMinPa = gnSetValue_Tmp;
		}
	}
	else
	if( MYGUI_STATE_SET_DEADBAND_PERIOD == gcnGuiSettingState  ){
		if( 0 == gcnGuiSetSubState ){
			MY7SEGMENT_Set_Value( FND_d, FND_P, FND_t, SEVEN_SEG_BLINK_OFF );
			gnSetValue_Tmp = pstDevEnv_Tmp->nDeadbandPeriod;
		}
		else
		if( 1 == gcnGuiSetSubState ){
			_mygui_change_digit_hundreds( &gnSetValue_Tmp, _cnUpdownBtn );
			MY7SEGMENT_Set_3Digit_Value( gnSetValue_Tmp, SEVEN_SEG_NO_BAR, SEVEN_SEG_NO_DOT, SEVEN_SEG_BLINK_DIGIT_1 );
		}
		else
		if( 2 == gcnGuiSetSubState ){
			_mygui_change_digit_tens( &gnSetValue_Tmp, _cnUpdownBtn );
			MY7SEGMENT_Set_3Digit_Value( gnSetValue_Tmp, SEVEN_SEG_NO_BAR, SEVEN_SEG_NO_DOT, SEVEN_SEG_BLINK_DIGIT_2 );
		}
		else
		if( 3 == gcnGuiSetSubState ){
			_mygui_change_digit_ones( &gnSetValue_Tmp, _cnUpdownBtn );
			MY7SEGMENT_Set_3Digit_Value( gnSetValue_Tmp, SEVEN_SEG_NO_BAR, SEVEN_SEG_NO_DOT, SEVEN_SEG_BLINK_DIGIT_3 );
		}
		else
		if( 4 == gcnGuiSetSubState ){
			gcnGuiSetSubState = 0;
			pstDevEnv_Tmp->nDeadbandPeriod = gnSetValue_Tmp;
		}
	}
	else
	if( MYGUI_STATE_SET_DEADBAND_UPPER_TICK == gcnGuiSettingState  ){
		if( 0 == gcnGuiSetSubState ){
			MY7SEGMENT_Set_Value( FND_d, FND_u, FND_t, SEVEN_SEG_BLINK_OFF );
			gnSetValue_Tmp = pstDevEnv_Tmp->nDeadbandUpperTick;
		}
		else
		if( 1 == gcnGuiSetSubState ){
			_mygui_change_digit_tens( &gnSetValue_Tmp, _cnUpdownBtn );
			MY7SEGMENT_Set_3Digit_Value( gnSetValue_Tmp, SEVEN_SEG_USING_SP, SEVEN_SEG_NO_DOT, SEVEN_SEG_BLINK_DIGIT_2 );
		}
		else
		if( 2 == gcnGuiSetSubState ){
			_mygui_change_digit_ones( &gnSetValue_Tmp, _cnUpdownBtn );
			MY7SEGMENT_Set_3Digit_Value( gnSetValue_Tmp, SEVEN_SEG_USING_SP, SEVEN_SEG_NO_DOT, SEVEN_SEG_BLINK_DIGIT_3 );
		}
		else
		if( 3 == gcnGuiSetSubState ){
			gcnGuiSetSubState = 0;
			pstDevEnv_Tmp->nDeadbandUpperTick = gnSetValue_Tmp;
		}
	}
	else
	if( MYGUI_STATE_SET_DEADBAND_DOWN_TICK == gcnGuiSettingState  ){
		if( 0 == gcnGuiSetSubState ){
			MY7SEGMENT_Set_Value( FND_d, FND_d, FND_t, SEVEN_SEG_BLINK_OFF );
			gnSetValue_Tmp = pstDevEnv_Tmp->nDeadbandDownTick;
		}
		else
		if( 1 == gcnGuiSetSubState ){
			_mygui_change_digit_tens( &gnSetValue_Tmp, _cnUpdownBtn );
			MY7SEGMENT_Set_3Digit_Value( gnSetValue_Tmp, SEVEN_SEG_USING_SP, SEVEN_SEG_NO_DOT, SEVEN_SEG_BLINK_DIGIT_2 );
		}
		else
		if( 2 == gcnGuiSetSubState ){
			_mygui_change_digit_ones( &gnSetValue_Tmp, _cnUpdownBtn );
			MY7SEGMENT_Set_3Digit_Value( gnSetValue_Tmp, SEVEN_SEG_USING_SP, SEVEN_SEG_NO_DOT, SEVEN_SEG_BLINK_DIGIT_3 );
		}
		else
		if( 3 == gcnGuiSetSubState ){
			gcnGuiSetSubState = 0;
			pstDevEnv_Tmp->nDeadbandDownTick = gnSetValue_Tmp;
		}
	}
	else
	if( MYGUI_STATE_SET_NORMAL_UPPER_RAMP == gcnGuiSettingState  ){
		if( 0 == gcnGuiSetSubState ){
			MY7SEGMENT_Set_Value( FND_n, FND_u, FND_r, SEVEN_SEG_BLINK_OFF );
			gnSetValue_Tmp = pstDevEnv_Tmp->nNormalUpperRamp;
		}
		else
		if( 1 == gcnGuiSetSubState ){
			_mygui_change_digit_hundreds( &gnSetValue_Tmp, _cnUpdownBtn );
			MY7SEGMENT_Set_3Digit_Value( gnSetValue_Tmp, SEVEN_SEG_NO_BAR, SEVEN_SEG_NO_DOT, SEVEN_SEG_BLINK_DIGIT_1 );
		}
		else
		if( 2 == gcnGuiSetSubState ){
			_mygui_change_digit_tens( &gnSetValue_Tmp, _cnUpdownBtn );
			MY7SEGMENT_Set_3Digit_Value( gnSetValue_Tmp, SEVEN_SEG_NO_BAR, SEVEN_SEG_NO_DOT, SEVEN_SEG_BLINK_DIGIT_2 );
		}
		else
		if( 3 == gcnGuiSetSubState ){
			_mygui_change_digit_ones( &gnSetValue_Tmp, _cnUpdownBtn );
			MY7SEGMENT_Set_3Digit_Value( gnSetValue_Tmp, SEVEN_SEG_NO_BAR, SEVEN_SEG_NO_DOT, SEVEN_SEG_BLINK_DIGIT_3 );
		}
		else
		if( 4 == gcnGuiSetSubState ){
			gcnGuiSetSubState = 0;
			pstDevEnv_Tmp->nNormalUpperRamp = gnSetValue_Tmp;
		}
	}
	else
	if( MYGUI_STATE_SET_NORMAL_DOWN_RAMP == gcnGuiSettingState  ){
		if( 0 == gcnGuiSetSubState ){
			MY7SEGMENT_Set_Value( FND_n, FND_d, FND_r, SEVEN_SEG_BLINK_OFF );
			gnSetValue_Tmp = pstDevEnv_Tmp->nNormalDownRamp;
		}
		else
		if( 1 == gcnGuiSetSubState ){
			_mygui_change_digit_hundreds( &gnSetValue_Tmp, _cnUpdownBtn );
			MY7SEGMENT_Set_3Digit_Value( gnSetValue_Tmp, SEVEN_SEG_NO_BAR, SEVEN_SEG_NO_DOT, SEVEN_SEG_BLINK_DIGIT_1 );
		}
		else
		if( 2 == gcnGuiSetSubState ){
			_mygui_change_digit_tens( &gnSetValue_Tmp, _cnUpdownBtn );
			MY7SEGMENT_Set_3Digit_Value( gnSetValue_Tmp, SEVEN_SEG_NO_BAR, SEVEN_SEG_NO_DOT, SEVEN_SEG_BLINK_DIGIT_2 );
		}
		else
		if( 3 == gcnGuiSetSubState ){
			_mygui_change_digit_ones( &gnSetValue_Tmp, _cnUpdownBtn );
			MY7SEGMENT_Set_3Digit_Value( gnSetValue_Tmp, SEVEN_SEG_NO_BAR, SEVEN_SEG_NO_DOT, SEVEN_SEG_BLINK_DIGIT_3 );
		}
		else
		if( 4 == gcnGuiSetSubState ){
			gcnGuiSetSubState = 0;
			pstDevEnv_Tmp->nNormalDownRamp = gnSetValue_Tmp;
		}
	}
	else
	if( MYGUI_STATE_SET_MIN_UPPER_SPEED == gcnGuiSettingState  ){
		if( 0 == gcnGuiSetSubState ){
			MY7SEGMENT_Set_Value( FND_L, FND_5, FND_d, SEVEN_SEG_BLINK_OFF );
			gnSetValue_Tmp = pstDevEnv_Tmp->nMinUpperSpeed;
		}
		else
		if( 1 == gcnGuiSetSubState ){
			_mygui_change_digit_hundreds( &gnSetValue_Tmp, _cnUpdownBtn );
			MY7SEGMENT_Set_3Digit_Value( gnSetValue_Tmp, SEVEN_SEG_NO_BAR, SEVEN_SEG_NO_DOT, SEVEN_SEG_BLINK_DIGIT_1 );
		}
		else
		if( 2 == gcnGuiSetSubState ){
			_mygui_change_digit_tens( &gnSetValue_Tmp, _cnUpdownBtn );
			MY7SEGMENT_Set_3Digit_Value( gnSetValue_Tmp, SEVEN_SEG_NO_BAR, SEVEN_SEG_NO_DOT, SEVEN_SEG_BLINK_DIGIT_2 );
		}
		else
		if( 3 == gcnGuiSetSubState ){
			_mygui_change_digit_ones( &gnSetValue_Tmp, _cnUpdownBtn );
			MY7SEGMENT_Set_3Digit_Value( gnSetValue_Tmp, SEVEN_SEG_NO_BAR, SEVEN_SEG_NO_DOT, SEVEN_SEG_BLINK_DIGIT_3 );
		}
		else
		if( 4 == gcnGuiSetSubState ){
			gcnGuiSetSubState = 0;
			pstDevEnv_Tmp->nMinUpperSpeed = gnSetValue_Tmp;
		}
	}
	else
	if( MYGUI_STATE_SET_MIN_DOWN_SPEED == gcnGuiSettingState  ){
		if( 0 == gcnGuiSetSubState ){
			MY7SEGMENT_Set_Value( FND_L, FND_5, FND_d, SEVEN_SEG_BLINK_OFF );
			gnSetValue_Tmp = pstDevEnv_Tmp->nMinDownSpeed;
		}
		else
		if( 1 == gcnGuiSetSubState ){
			_mygui_change_digit_hundreds( &gnSetValue_Tmp, _cnUpdownBtn );
			MY7SEGMENT_Set_3Digit_Value( gnSetValue_Tmp, SEVEN_SEG_NO_BAR, SEVEN_SEG_NO_DOT, SEVEN_SEG_BLINK_DIGIT_1 );
		}
		else
		if( 2 == gcnGuiSetSubState ){
			_mygui_change_digit_tens( &gnSetValue_Tmp, _cnUpdownBtn );
			MY7SEGMENT_Set_3Digit_Value( gnSetValue_Tmp, SEVEN_SEG_NO_BAR, SEVEN_SEG_NO_DOT, SEVEN_SEG_BLINK_DIGIT_2 );
		}
		else
		if( 3 == gcnGuiSetSubState ){
			_mygui_change_digit_ones( &gnSetValue_Tmp, _cnUpdownBtn );
			MY7SEGMENT_Set_3Digit_Value( gnSetValue_Tmp, SEVEN_SEG_NO_BAR, SEVEN_SEG_NO_DOT, SEVEN_SEG_BLINK_DIGIT_3 );
		}
		else
		if( 4 == gcnGuiSetSubState ){
			gcnGuiSetSubState = 0;
			pstDevEnv_Tmp->nMinDownSpeed = gnSetValue_Tmp;
		}
	}
	else
	if( MYGUI_STATE_SET_MAX_UPPER_SPEED == gcnGuiSettingState  ){
		if( 0 == gcnGuiSetSubState ){
			MY7SEGMENT_Set_Value( FND_H, FND_u, FND_d, SEVEN_SEG_BLINK_OFF );
			gnSetValue_Tmp = pstDevEnv_Tmp->nMaxUpperSpeed;
		}
		else
		if( 1 == gcnGuiSetSubState ){
			_mygui_change_digit_hundreds( &gnSetValue_Tmp, _cnUpdownBtn );
			MY7SEGMENT_Set_3Digit_Value( gnSetValue_Tmp, SEVEN_SEG_NO_BAR, SEVEN_SEG_NO_DOT, SEVEN_SEG_BLINK_DIGIT_1 );
		}
		else
		if( 2 == gcnGuiSetSubState ){
			_mygui_change_digit_tens( &gnSetValue_Tmp, _cnUpdownBtn );
			MY7SEGMENT_Set_3Digit_Value( gnSetValue_Tmp, SEVEN_SEG_NO_BAR, SEVEN_SEG_NO_DOT, SEVEN_SEG_BLINK_DIGIT_2 );
		}
		else
		if( 3 == gcnGuiSetSubState ){
			_mygui_change_digit_ones( &gnSetValue_Tmp, _cnUpdownBtn );
			MY7SEGMENT_Set_3Digit_Value( gnSetValue_Tmp, SEVEN_SEG_NO_BAR, SEVEN_SEG_NO_DOT, SEVEN_SEG_BLINK_DIGIT_3 );
		}
		else
		if( 4 == gcnGuiSetSubState ){
			gcnGuiSetSubState = 0;
			pstDevEnv_Tmp->nMaxUpperSpeed = gnSetValue_Tmp;
		}
	}
	else
	if( MYGUI_STATE_SET_MAX_DOWN_SPEED == gcnGuiSettingState  ){
		if( 0 == gcnGuiSetSubState ){
			MY7SEGMENT_Set_Value( FND_H, FND_d, FND_d, SEVEN_SEG_BLINK_OFF );
			gnSetValue_Tmp = pstDevEnv_Tmp->nMaxDownSpeed;
		}
		else
		if( 1 == gcnGuiSetSubState ){
			_mygui_change_digit_hundreds( &gnSetValue_Tmp, _cnUpdownBtn );
			MY7SEGMENT_Set_3Digit_Value( gnSetValue_Tmp, SEVEN_SEG_NO_BAR, SEVEN_SEG_NO_DOT, SEVEN_SEG_BLINK_DIGIT_1 );
		}
		else
		if( 2 == gcnGuiSetSubState ){
			_mygui_change_digit_tens( &gnSetValue_Tmp, _cnUpdownBtn );
			MY7SEGMENT_Set_3Digit_Value( gnSetValue_Tmp, SEVEN_SEG_NO_BAR, SEVEN_SEG_NO_DOT, SEVEN_SEG_BLINK_DIGIT_2 );
		}
		else
		if( 3 == gcnGuiSetSubState ){
			_mygui_change_digit_ones( &gnSetValue_Tmp, _cnUpdownBtn );
			MY7SEGMENT_Set_3Digit_Value( gnSetValue_Tmp, SEVEN_SEG_NO_BAR, SEVEN_SEG_NO_DOT, SEVEN_SEG_BLINK_DIGIT_3 );
		}
		else
		if( 4 == gcnGuiSetSubState ){
			gcnGuiSetSubState = 0;
			pstDevEnv_Tmp->nMaxDownSpeed = gnSetValue_Tmp;
		}
	}
	else
	if( MYGUI_STATE_SET_PID_P == gcnGuiSettingState  ){
		if( 0 == gcnGuiSetSubState ){
			MY7SEGMENT_Set_Value( FND_P, FND_1, FND_d, SEVEN_SEG_BLINK_DIGIT_1 );
			gnSetValue_Tmp = pstDevEnv_Tmp->nPid_P;
		}
		else
		if( 1 == gcnGuiSetSubState ){
			_mygui_change_digit_hundreds( &gnSetValue_Tmp, _cnUpdownBtn );
			MY7SEGMENT_Set_3Digit_Value( gnSetValue_Tmp, SEVEN_SEG_NO_BAR, SEVEN_SEG_NO_DOT, SEVEN_SEG_BLINK_DIGIT_1 );
		}
		else
		if( 2 == gcnGuiSetSubState ){
			_mygui_change_digit_tens( &gnSetValue_Tmp, _cnUpdownBtn );
			MY7SEGMENT_Set_3Digit_Value( gnSetValue_Tmp, SEVEN_SEG_NO_BAR, SEVEN_SEG_NO_DOT, SEVEN_SEG_BLINK_DIGIT_2 );
		}
		else
		if( 3 == gcnGuiSetSubState ){
			_mygui_change_digit_ones( &gnSetValue_Tmp, _cnUpdownBtn );
			MY7SEGMENT_Set_3Digit_Value( gnSetValue_Tmp, SEVEN_SEG_NO_BAR, SEVEN_SEG_NO_DOT, SEVEN_SEG_BLINK_DIGIT_3 );
		}
		else
		if( 4 == gcnGuiSetSubState ){
			gcnGuiSetSubState = 0;
			pstDevEnv_Tmp->nPid_P = gnSetValue_Tmp;
		}
	}
	else
	if( MYGUI_STATE_SET_PID_I == gcnGuiSettingState  ){
		if( 0 == gcnGuiSetSubState ){
			MY7SEGMENT_Set_Value( FND_P, FND_1, FND_d, SEVEN_SEG_BLINK_DIGIT_2 );
			gnSetValue_Tmp = pstDevEnv_Tmp->nPid_I;
		}
		else
		if( 1 == gcnGuiSetSubState ){
			_mygui_change_digit_hundreds( &gnSetValue_Tmp, _cnUpdownBtn );
			MY7SEGMENT_Set_3Digit_Value( gnSetValue_Tmp, SEVEN_SEG_NO_BAR, SEVEN_SEG_NO_DOT, SEVEN_SEG_BLINK_DIGIT_1 );
		}
		else
		if( 2 == gcnGuiSetSubState ){
			_mygui_change_digit_tens( &gnSetValue_Tmp, _cnUpdownBtn );
			MY7SEGMENT_Set_3Digit_Value( gnSetValue_Tmp, SEVEN_SEG_NO_BAR, SEVEN_SEG_NO_DOT, SEVEN_SEG_BLINK_DIGIT_2 );
		}
		else
		if( 3 == gcnGuiSetSubState ){
			_mygui_change_digit_ones( &gnSetValue_Tmp, _cnUpdownBtn );
			MY7SEGMENT_Set_3Digit_Value( gnSetValue_Tmp, SEVEN_SEG_NO_BAR, SEVEN_SEG_NO_DOT, SEVEN_SEG_BLINK_DIGIT_3 );
		}
		else
		if( 4 == gcnGuiSetSubState ){
			gcnGuiSetSubState = 0;
			pstDevEnv_Tmp->nPid_I = gnSetValue_Tmp;
		}
	}
	else
	if( MYGUI_STATE_SET_PID_D == gcnGuiSettingState  ){
		if( 0 == gcnGuiSetSubState ){
			MY7SEGMENT_Set_Value( FND_P, FND_1, FND_d, SEVEN_SEG_BLINK_DIGIT_3 );
			gnSetValue_Tmp = pstDevEnv_Tmp->nPid_D;
		}
		else
		if( 1 == gcnGuiSetSubState ){
			_mygui_change_digit_hundreds( &gnSetValue_Tmp, _cnUpdownBtn );
			MY7SEGMENT_Set_3Digit_Value( gnSetValue_Tmp, SEVEN_SEG_NO_BAR, SEVEN_SEG_NO_DOT, SEVEN_SEG_BLINK_DIGIT_1 );
		}
		else
		if( 2 == gcnGuiSetSubState ){
			_mygui_change_digit_tens( &gnSetValue_Tmp, _cnUpdownBtn );
			MY7SEGMENT_Set_3Digit_Value( gnSetValue_Tmp, SEVEN_SEG_NO_BAR, SEVEN_SEG_NO_DOT, SEVEN_SEG_BLINK_DIGIT_2 );
		}
		else
		if( 3 == gcnGuiSetSubState ){
			_mygui_change_digit_ones( &gnSetValue_Tmp, _cnUpdownBtn );
			MY7SEGMENT_Set_3Digit_Value( gnSetValue_Tmp, SEVEN_SEG_NO_BAR, SEVEN_SEG_NO_DOT, SEVEN_SEG_BLINK_DIGIT_3 );
		}
		else
		if( 4 == gcnGuiSetSubState ){
			gcnGuiSetSubState = 0;
			pstDevEnv_Tmp->nPid_D = gnSetValue_Tmp;
		}
	}
	else
	if( MYGUI_STATE_SET_OVER_CURRENT == gcnGuiSettingState  ){
		if( 0 == gcnGuiSetSubState ){
			MY7SEGMENT_Set_Value( FND_c, FND_u, FND_r, SEVEN_SEG_BLINK_OFF );
			gnSetValue_Tmp = pstDevEnv_Tmp->nOverCurrent;
		}
		else
		if( 1 == gcnGuiSetSubState ){
			_mygui_change_digit_tens( &gnSetValue_Tmp, _cnUpdownBtn );
			MY7SEGMENT_Set_3Digit_Value( gnSetValue_Tmp, SEVEN_SEG_NO_BAR, SEVEN_SEG_USING_DOT, SEVEN_SEG_BLINK_DIGIT_2 );
		}
		else
		if( 2 == gcnGuiSetSubState ){
			_mygui_change_digit_ones( &gnSetValue_Tmp, _cnUpdownBtn );
			MY7SEGMENT_Set_3Digit_Value( gnSetValue_Tmp, SEVEN_SEG_NO_BAR, SEVEN_SEG_USING_DOT, SEVEN_SEG_BLINK_DIGIT_3 );
		}
		else
		if( 3 == gcnGuiSetSubState ){
			gcnGuiSetSubState = 0;
			pstDevEnv_Tmp->nOverCurrent = gnSetValue_Tmp;
		}
	}
	else
	if( MYGUI_STATE_SET_NONC == gcnGuiSettingState  ){
		if( 0 == gcnGuiSetSubState ){
			MY7SEGMENT_Set_Value( FND_n, FND_o, FND_c, SEVEN_SEG_BLINK_OFF );
			gnSetValue_Tmp = pstDevEnv_Tmp->nMotorNoNc;
		}
		else
		if( 1 == gcnGuiSetSubState ){
			if( MOTOR_STATE_NORMAL_CLOSE == gnSetValue_Tmp ){
				MY7SEGMENT_Set_Value( FND_SP, FND_n, FND_c, SEVEN_SEG_BLINK_OFF );
			}
			else{
				MY7SEGMENT_Set_Value( FND_SP, FND_n, FND_o, SEVEN_SEG_BLINK_OFF );
			}
			if( 0 != _cnUpdownBtn ){
				if( 0 == gnSetValue_Tmp ){ gnSetValue_Tmp = 1; }
				else{ gnSetValue_Tmp = 0; }
			}
		}
		else
		if( 2 == gcnGuiSetSubState ){
			gcnGuiSetSubState = 0;
			pstDevEnv_Tmp->nMotorNoNc = gnSetValue_Tmp;
		}
	}
	else
	if( MYGUI_STATE_SET_RS485_BAUD == gcnGuiSettingState  ){
		if( 0 == gcnGuiSetSubState ){
			MY7SEGMENT_Set_Value( FND_b, FND_A, FND_d, SEVEN_SEG_BLINK_OFF );
			gnSetValue_Tmp = pstDevEnv_Tmp->nBaudrate;
		}
		else
		if( 1 == gcnGuiSetSubState ){
			_mygui_change_digit_ones( &gnSetValue_Tmp, _cnUpdownBtn );
			MY7SEGMENT_Set_3Digit_Value( gnSetValue_Tmp, SEVEN_SEG_USING_SP, SEVEN_SEG_NO_DOT, SEVEN_SEG_BLINK_DIGIT_3 );
		}
		else
		if( 2 == gcnGuiSetSubState ){
			gcnGuiSetSubState = 0;
			pstDevEnv_Tmp->nBaudrate = gnSetValue_Tmp;
		}
	}
	// else
	// if( MYGUI_STATE_SET_SAVE == gcnGuiSettingState  ){
	// 	if( 0 == gcnGuiSetSubState ){
	// 		MY7SEGMENT_Set_Value( FND_5, FND_A, FND_U, SEVEN_SEG_BLINK_OFF );
	// 	}
	// 	else
	// 	if( 1 == gcnGuiSetSubState ){
	// 		Env_Restore_From_Temperary();
	// 		ENV_Save_Enviroment_Info();
	// 		gcnGuiSetSubState+= 1;
	// 	}
	// 	else
	// 	if( 2 == gcnGuiSetSubState ){
	// 		MY7SEGMENT_Set_Value( FND_SP, FND_0, FND_K, SEVEN_SEG_BLINK_OFF );
	// 	}
	// 	else
	// 	if( 3 == gcnGuiSetSubState ){
	// 		gcnGuiSetSubState = 0;
	// 	}
	// }
}

void _mygui_main_page_check_limit_switch( uint8_t _cnSel_Btn )
{
	if( ( 1 == _cnSel_Btn ) || ( 0 == gnGuiReturnTimer ) ){
		gcnGuiMainState = MYGUI_STATE_IDLE;
	}
	
	if( LIMIT_SWITCH_STATE_ON == MYMOTOR_Get_Open_LimitSwtich_St() ){
		gnGuiReturnTimer = MYGUI_RETURN_MENU_TIME_MS;
		MY7SEGMENT_Set_Value( FND_BAR_MIDDLE, FND_0, FND_P, SEVEN_SEG_BLINK_OFF );
	}
	else
	if( LIMIT_SWITCH_STATE_ON == MYMOTOR_Get_Close_LimitSwtich_St() ){
		gnGuiReturnTimer = MYGUI_RETURN_MENU_TIME_MS;
		MY7SEGMENT_Set_Value( FND_BAR_MIDDLE, FND_C, FND_L, SEVEN_SEG_BLINK_OFF );
	}
	else{
		MY7SEGMENT_Set_Value( FND_BAR_MIDDLE, FND_SP, FND_SP, SEVEN_SEG_BLINK_OFF );
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////
// public and export function implement
void MYGUI_Proc( void )
{
	uint8_t cnbtn_st = MYBTN_Proc();
	int8_t cnupdown_btn = 0;
	int8_t cnsel_btn = 0;

	if( MYBTN_STATE_IDLE != cnbtn_st ){
		gnGuiReturnTimer = MYGUI_RETURN_MENU_TIME_MS;
	}

	// Select Button
	if( MYBTN_STATE_CLICK_SEL == cnbtn_st ){ cnsel_btn = 1; }
	if( MYBTN_STATE_LONGKEY_SEL == cnbtn_st ){ cnsel_btn = 2; }
	// UpDown Button
	if( MYBTN_STATE_CLICK_UP == cnbtn_st ){ cnupdown_btn = +1; }
	else
	if( MYBTN_STATE_CLICK_DOWN == cnbtn_st ){ cnupdown_btn = -1; }


	// if( ( MYGUI_STATE_IDLE != gcnGuiMainState ) || ( MYBTN_STATE_CLICK_SEL == cnbtn_st ) ){
	// 	gcnGuiMainState = MYGUI_STATE_IDLE;
	// }

	if( 1 == gcnError_msg ){
		if( MYBTN_STATE_IDLE != cnbtn_st ){
			gcnError_msg = 0;
		}
		MY7SEGMENT_Set_Value( FND_E, FND_r, FND_r, SEVEN_SEG_BLINK_OFF );
	}

	switch( gcnGuiMainState )
	{
		case MYGUI_STATE_IDLE:
			_mygui_main_page_idle_proc( cnbtn_st );
			break;
		case MYGUI_STATE_SETTING:
			_mygui_main_page_set_proc( cnupdown_btn, cnsel_btn );
			break;
		case MYGUI_STATE_SAVE:
			MY7SEGMENT_Set_Value( FND_5, FND_A, FND_U, SEVEN_SEG_BLINK_OFF );
			if( 0 == gnGuiTimer ){
				gcnGuiMainState = MYGUI_STATE_IDLE;
			}
			break;
		case MYGUI_STATE_CHECK_LIMIT_SWITCH:
			_mygui_main_page_check_limit_switch( cnsel_btn );
			break;
		default:
			gcnGuiMainState = MYGUI_STATE_IDLE;
			break;
	}
}



#if 0
		switch( gcnDispState )
		{
			case PMode_0:
				MY7SEGMENT_Set_3Digit_Value( gnDispSensorValue, SEVEN_SEG_NO_BAR, SEVEN_SEG_NO_DOT, SEVEN_SEG_BLINK_OFF );
				break;
			case PMode_1:
				MY7SEGMENT_Set_Value( FND_5, FND_E, FND_L, SEVEN_SEG_BLINK_OFF );
				gcnDispState = PMode_2;
				gnGuiTimer = 1000;
				break;
			case PMode_2:
				if( 0 == gnGuiTimer ){
					gcnDispState = PMode_3;
				}
				break;
			case PMode_3:
				if( MYBTN_STATE_CLICK_UP == cnbtn_st ){
					gcnDeviceId = ( gcnDeviceId + 10 ) % 100;
				}
				MY7SEGMENT_Set_3Digit_Value( gcnDeviceId, SEVEN_SEG_USING_BAR_1, SEVEN_SEG_NO_DOT, SEVEN_SEG_BLINK_DIGIT_2 );
				break;
			case PMode_4:
				if( MYBTN_STATE_CLICK_UP == cnbtn_st ){
					gcnDeviceId = gcnDeviceId + 1;
					if( 0 == ( gcnDeviceId % 10 ) ){
						gcnDeviceId -= 10;
					}
				}
				MY7SEGMENT_Set_3Digit_Value( gcnDeviceId, SEVEN_SEG_USING_BAR_1, SEVEN_SEG_NO_DOT, SEVEN_SEG_BLINK_DIGIT_3 );
				break;
			case PMode_5:
				if( MYBTN_STATE_CLICK_UP == cnbtn_st ){
					if( 0 == gcnSensor_Offset_Sign ){ gcnSensor_Offset_Sign = 1; }
					else{ gcnSensor_Offset_Sign = 0; }
				}

				if( 0 == gcnSensor_Offset_Sign ){ cntmp = SEVEN_SEG_USING_BAR_P; }
				else{ cntmp = SEVEN_SEG_USING_BAR_N; }
				MY7SEGMENT_Set_3Digit_Value( gnSensor_Offset, cntmp, SEVEN_SEG_USING_DOT, SEVEN_SEG_BLINK_DIGIT_1 );
				break;
			case PMode_6:
				if( MYBTN_STATE_CLICK_UP == cnbtn_st ){
					gnSensor_Offset = ( gnSensor_Offset + 10 ) % 100;
				}
				MY7SEGMENT_Set_3Digit_Value( gnSensor_Offset, SEVEN_SEG_USING_BAR_P, SEVEN_SEG_USING_DOT, SEVEN_SEG_BLINK_DIGIT_2 );
				break;
			case PMode_7:
				if( MYBTN_STATE_CLICK_UP == cnbtn_st ){
					gnSensor_Offset = gnSensor_Offset + 1;
					if( 0 == ( gnSensor_Offset % 10 ) ){
						gnSensor_Offset -= 10;
					}
				}
				MY7SEGMENT_Set_3Digit_Value( gnSensor_Offset, SEVEN_SEG_USING_BAR_P, SEVEN_SEG_USING_DOT, SEVEN_SEG_BLINK_DIGIT_3 );
				break;
			case PMode_8:
				if( MYBTN_STATE_CLICK_UP == cnbtn_st ){
					gnSensor_xTimes = ( gnSensor_xTimes + 10 ) % 100;
				}
				MY7SEGMENT_Set_3Digit_Value( gnSensor_xTimes, SEVEN_SEG_USING_BAR_3, SEVEN_SEG_USING_DOT, SEVEN_SEG_BLINK_DIGIT_2 );
				break;
			case PMode_9:
				if( MYBTN_STATE_CLICK_UP == cnbtn_st ){
					gnSensor_xTimes = gnSensor_xTimes + 1;
					if( 0 == ( gnSensor_xTimes % 10 ) ){
						gnSensor_xTimes -= 10;
					}
				}
				MY7SEGMENT_Set_3Digit_Value( gnSensor_xTimes, SEVEN_SEG_USING_BAR_3, SEVEN_SEG_USING_DOT, SEVEN_SEG_BLINK_DIGIT_3 );
				break;
			case PMode_10:
				MY7SEGMENT_Set_Value( FND_SP, FND_F, FND_0, SEVEN_SEG_BLINK_OFF );
				break;
			case PMode_11:
				MY7SEGMENT_Set_Value( FND_SP, FND_R, FND_E, SEVEN_SEG_BLINK_OFF );
				break;
			
			default:
				break;
		}
	}
#endif

	// if( 0 == gnGuiTimer ){
	// 	gnGuiTimer = GUI_REFRESH_TIME_MS;
		
	// }
// }
/////////////////////////////////////////////////////////////////////////////////////////////
// local function implement
