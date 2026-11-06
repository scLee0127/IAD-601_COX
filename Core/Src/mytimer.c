  #include "global.h"
#include "mytimer.h"
#include "my7segment.h"

/////////////////////////////////////////////////////////////////////////////////////////////
// import function prototype


/////////////////////////////////////////////////////////////////////////////////////////////
// local function prototype
#define SEVEN_SEG_DIGIT_COUNT 	3
#define SEVEN_SEG_REFRESH_HZ 	1500
#define SEVEN_SEG_STEP_HZ 		( SEVEN_SEG_REFRESH_HZ / SEVEN_SEG_DIGIT_COUNT )

static void _mytimer_tim6_init( void );
#if BOARD_MOTOR_ENABLE
static void _mytimer_tim3_init( void );
static void _mytimer_tim14_init( void );
static uint32_t _mytimer_duty_to_ccr( float _nfduty, uint32_t _narr );
#endif

/////////////////////////////////////////////////////////////////////////////////////////////
// global variable
TIM_HandleTypeDef htim3;  		// PB0 PWM Timer
TIM_HandleTypeDef htim6;  		// 7Segment Timer
TIM_HandleTypeDef htim14;  		// PB1 PWM Timer

/////////////////////////////////////////////////////////////////////////////////////////////
// public and export function implement
void MYTIMER_Init( void )
{
    _mytimer_tim6_init();
#if BOARD_MOTOR_ENABLE
	_mytimer_tim3_init();
	_mytimer_tim14_init();
#endif
}

/* TIM6 update interrupt refreshes one display digit every 2 ms. */
void HAL_TIM_PeriodElapsedCallback( TIM_HandleTypeDef *htim )
{
    if( htim->Instance == TIM6 ){
        MY7SEGMENT_Proc();
    }
}

void MYTIMER_Proc( void )
{
	if( 0 < gnTestTimer ){ gnTestTimer--; }
	if( 0 < gnBootTimer ){ gnBootTimer--; }
	if( 0 < gnGuiTimer ){ gnGuiTimer--; }
	if( 0 < gnDpSensorTimer ){ gnDpSensorTimer--; }
	if( 0 < gnBtnTimer ){ gnBtnTimer--; }
	if( 0 < gnBuzzerTimer ){ gnBuzzerTimer--; }
	if( 0 < gnMotorTimer ){ gnMotorTimer--; }
	if( 0 < gnGuiSensorTimer ){gnGuiSensorTimer--; }
	if( 0 < gnMotorHoldTimer ){ gnMotorHoldTimer--; }
	if( 0 < gnMotorFixedSpeedTimer ){ gnMotorFixedSpeedTimer--; }
	if( 0 < gnGuiReturnTimer ){ gnGuiReturnTimer--; }
	if( 0 < gnSaveEnvDelayTimer ){ gnSaveEnvDelayTimer--; }
	if( 0 < gnModbusTxTimer ){ gnModbusTxTimer--; }
	gnFndBlinkTimer++;
	if( 0x1000000 < gnFndBlinkTimer ){ gnFndBlinkTimer = 0; }
	if( 0 < gnMotorDeadbandTick ){ gnMotorDeadbandTick--; }
	if( 0 < gnMotorLimitSwStablTimer ){ gnMotorLimitSwStablTimer--; }
	if( 0 < gnUartCheckTimer ){ gnUartCheckTimer--; }
}


#if BOARD_MOTOR_ENABLE
void MYPWM_Set_Duty( uint8_t _cnChannel, float _nfduty )
{
	// if( 100 < _cnPercent ){ _cnPercent = 100; }
	// uint32_t npulse = ( 1000 * _cnPercent ) / 100; // ARR=999 기준
	uint32_t nccr = 0;
	uint32_t narr = 0;
	
	if( MOTOR_PWM_CHANNEL_M0 == _cnChannel ){
		// PB0 (TIM3_CH3)
		narr = __HAL_TIM_GET_AUTORELOAD( &htim3 );
		nccr = _mytimer_duty_to_ccr( _nfduty, narr );
		if( 0 != nccr ){
			// printf("M00 [%ld] duty %ld(%ld/%f)\r\n",(int32_t)gnfLpfPaValue, nccr, narr, _nfduty );
			// printf("Op [PA:%ld] ccr:%ld(arr:%ld/duty:%f)\r\n",(int32_t)gnfLpfPaValue, nccr, narr, _nfduty );
		}
		__HAL_TIM_SET_COMPARE( & htim3, TIM_CHANNEL_3, nccr );
	}
	else{
		// PB1 ( TIM14_CH1)
		narr = __HAL_TIM_GET_AUTORELOAD( &htim14 );
		nccr = _mytimer_duty_to_ccr( _nfduty, narr );
		if( 0 != nccr ){
			// printf("Cl [PA:%ld] ccr:%ld(arr:%ld/duty:%f)\r\n",(int32_t)gnfLpfPaValue, nccr, narr, _nfduty );
		}
		__HAL_TIM_SET_COMPARE( & htim14, TIM_CHANNEL_1, nccr );
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////
// local function implement

#endif /* BOARD_MOTOR_ENABLE */

static void _mytimer_tim6_init( void )
{
    uint32_t ntimclk = HAL_RCC_GetPCLK1Freq(); // 보통 48 MHz
    // 분주/주기 계산 (카운터클럭 ≈100kHz 권장)
    uint32_t npsc = ntimclk / 100000U; if (npsc == 0) npsc = 1; npsc -= 1;
    uint32_t ncntclk = ntimclk / (npsc + 1U);
    uint32_t narr = ncntclk / SEVEN_SEG_STEP_HZ; if (narr < 1) narr = 1; narr -= 1;

	htim6.Instance = TIM6;
	htim6.Init.Prescaler = (uint16_t)npsc;
	htim6.Init.Period = (uint16_t)narr;
	htim6.Init.CounterMode = TIM_COUNTERMODE_UP;
	htim6.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
	if (HAL_TIM_Base_Init(&htim6) != HAL_OK)
	{
		Error_Handler();
	}

	__HAL_TIM_CLEAR_FLAG( &htim6, TIM_FLAG_UPDATE );
    if (HAL_TIM_Base_Start_IT( &htim6) != HAL_OK ) {
        Error_Handler();
    }
}

#if BOARD_MOTOR_ENABLE
static void _mytimer_tim3_init( void )
{
	TIM_OC_InitTypeDef sConfigOC = {0};

    __HAL_RCC_TIM3_CLK_ENABLE();

    htim3.Instance = TIM3;
    htim3.Init.Prescaler = 0;          // 48MHz / 48 = 1MHz
    htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim3.Init.Period = 2399;            // 1MHz / 1000 = 1kHz
    htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    HAL_TIM_PWM_Init(&htim3);

    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = 0;              // 50%
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_3);

    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_3);

    MYPWM_Set_Duty( MOTOR_PWM_CHANNEL_M0, (float)0.0f );
}

static void _mytimer_tim14_init( void )
{
	TIM_OC_InitTypeDef sConfigOC = {0};

    __HAL_RCC_TIM14_CLK_ENABLE();

    htim14.Instance = TIM14;
    htim14.Init.Prescaler = 0;         // 48MHz / 48 = 1MHz
    htim14.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim14.Init.Period = 2399;           // 1kHz
    htim14.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    HAL_TIM_PWM_Init(&htim14);

    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = 0;              // 50%
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    HAL_TIM_PWM_ConfigChannel(&htim14, &sConfigOC, TIM_CHANNEL_1);

    HAL_TIM_PWM_Start(&htim14, TIM_CHANNEL_1);
    MYPWM_Set_Duty( MOTOR_PWM_CHANNEL_M1, (float)0.0f );
}

static uint32_t _mytimer_duty_to_ccr( float _nfduty, uint32_t _narr )
{
	if( _nfduty < 0.0f ){ _nfduty = 0.0f; }
	if( _nfduty > 1.0f ){ _nfduty = 1.0f; }


	return (uint32_t)( _nfduty * (float)_narr + 0.5f );
}

#endif /* BOARD_MOTOR_ENABLE */
