#include "global.h"
#include "mymotor.h"
#include "mygpio.h"
#include "mytimer.h"
#include "myadc.h"

#if BOARD_MOTOR_ENABLE
/////////////////////////////////////////////////////////////////////////////////////////////
// import function prototype

/////////////////////////////////////////////////////////////////////////////////////////////
// global variable
// MOTOR
#define SWITCH_DELAY_MS     				100
#define SELECT_RAMP_PER_STEP_SETTLE_TICK  	2
#define SELECT_RAMP_PER_STEP_SETTLE_IN  	1
#define SELECT_RAMP_PER_STEP_SETTLE_OUT  	0
static uint8_t gcnInSettleBand_f = 0;
static uint8_t gcnPidMode_f = 0;

#define DEADBAND_TICK_SPEED_SCALE 			0.7f
#define SPEED_SCALE_OUTER  					1.0f
#define SPEED_SCALE_SETTLEBAND_MIN  		0.02f
#define SPEED_SCALE_SETTLEBAND_SCALE  		0.1f
#define MOTOR_PWM_VALUE_CLEAR  			1
#define MOTOR_PWM_VALUE_KEEP 			0

#define MOTOR_OPEN_CLOSE_RATIO  		3.5f

#define MOTOR_BLOCK_ACTIVE_TIME_MS 		50
static volatile float gnfMin_Speed = 0.001f;     		// 최소 PWM (모터 정지마찰 극복)
static int32_t gnDeadbandActTick = 0;
static int32_t gnTickPulseTimer_ms = 0;

// [추가] 틱 모드 동작을 위한 상태 저장 변수
static int gnTickDir = 0;
static float gnfTickSpeed = 0.0f;

// 센서 필터
#define Y_SLEW_PA_PER_20MS  			8.0f  		// 20ms당 측정값 최대 변화량 제한
// Motor State Machine
typedef enum { 
	SD_IDLE = 0, 
	SD_RUN, 
	SD_SWITCH_PAUSE 
}eSD_STATE_T;
static eSD_STATE_T geState = SD_IDLE;

// Command
static int gnCmd_Dir = 0;				// 외부에서 들어온 방향 명령 (+1/-1/0)
static float gnfCmd_Speed = 0.0f;  		// 외부 명령 속도(0.0~1.0)
static int gnCur_Dir = 0;   			// 현재 구동 방향
static float gnfCur_Pwm = 0.0f;  		// 현제 실제 PWM 값
static uint32_t gnDirSwitch_ts = 0;  	// DIR 스위칭용 타임스

static uint32_t gnMotorTick_prev = 0;
static uint32_t gnAcc2ms = 0;
static uint32_t gnAcc20ms = 0;

// Blanking / Deadband
static uint8_t gcnBlank_Active = 0;
static uint32_t gnBlank_t0_ms = 0;
static uint8_t gcnDB_Hold = 0;

// ramp scle
static float gnfRamp_Scale = 0.0f;
// 측정값 필터
static float gnfY_Prev = 0.0f;

// Limit switch edge detect
static uint8_t gcnCloseSw_prev = 0;  				// 리미트 에지 검출
// static uint8_t gcnOpenSw_prev = 0;  				// 리미트 에지 검출

// Fire Event
static uint8_t gcnFireEvent_prev = 0xFF;
static uint8_t gcnClose_Hold = 0;
static uint32_t gnDB_Hold_ms = 0;

static uint32_t gnPid_D_Period = 0;
static int32_t gnMotorTick_ms = 0;
// PID
typedef struct {
    float kp, ki, kd;
    float dt;                 // [s], 예: 0.01f (10ms)
    float out_min, out_max;   // 출력 한계 [-1..+1]
    float i_min, i_max;       // 적분 한계
    float d_hz;               // D 저역통과 컷오프(Hz), 0이면 비활성

    // 내부 상태
    float integrator;
    float prev_err;
    float d_state;
    uint8_t first_run;

    // 안정화 구간
    float settle_band;
    float settle_scale;
    float u_slew;
    float prev_u;
} MYPID_t;
static MYPID_t gstPid;

// gcnFireEvent_f 엣지 감지 (PID 상태 리셋 & 1→0 즉시 정지 조건 처리)
#define MOTOR_CONTROL_PERIOD_MS 			20
#define MOTOR_PID_DT_VALUE 					0.02f
#define MOTOR_LIMIT_SWITCH_STABLE_TIME_MS 	500
/////////////////////////////////////////////////////////////////////////////////////////////
// local function prototype
// MOTOR
void _mymotor_set_open_pwm( const float _nfduty );
void _mymotor_set_close_pwm( const float _nfduty );
void _mymotor_gnd_open_control( uint8_t _cnCtrl );
void _mymotor_gnd_close_control( uint8_t _cnCtrl );
void _mymotor_coast_off( void );
void _mymotor_break_stop( void );
void _mymotor_shtter_state_update( void );
void _mymotor_shttuer_cmd( int _nDir, float _nfSpeed, uint8_t _cnClearPwm );
uint8_t _mymotor_get_open_gnd_state( void );
uint8_t _mymotor_get_close_gnd_state( void );
uint8_t _mymotor_get_open_limit_switch( void );
uint8_t _mymotor_get_close_limit_switch( void );

// PID
// kp: 비례 게인 (Proportional Gain). 현재 오차에 비례하여 출력을 냅니다. 반응 속도를 결정
// ki: 적분 게인 (Integral Gain). 누적된 오차에 비례하여 출력을 냅니다. 정상 상태 오차(잔류 편차)를 제거
// kd: 미분 게인 (Derivative Gain). 오차의 변화율에 비례하여 출력을 냅니다. 오버슈트를 줄이고 급격한 변화를 억제
// dt: 제어 루프 주기 (Delta Time, 초 단위). 적분 및 미분 계산 시 시간 기준으로 사용됩니다. (예: 0.02f는 20ms)
void _mypid_init( MYPID_t *p, float kp, float ki, float kd, float dt );
void _mypid_set_limits( MYPID_t *p, float out_min, float out_max, float i_min, float i_max );
void _mypid_set_d_lpf( MYPID_t *p, float d_hz );
void _mypid_reset( MYPID_t *p );
float _mypid_update( MYPID_t *p, float sp, float y );

// UTIL
static float clampf( float v, float lo, float hi );
static float _mymotor_fabsf_local( float x );
/////////////////////////////////////////////////////////////////////////////////////////////
// public and export function implement

void MYMOTOR_Init( void )
{
	// _mypid_init( &gstPid, 0.15f, 0.05f, 0.02f, 0.01f );   // 10ms
	// _mypid_init( &gstPid, 0.15f, 0.05f, 0.02f, 0.02f );   // 20ms kd -->> 
	_mypid_init( &gstPid, gnfMotorPid_P, gnfMotorPid_I, gnfMotorPid_D, MOTOR_PID_DT_VALUE );   // 10ms kd -->> 
    _mypid_set_limits( &gstPid, -1.0f, +1.0f, -0.5f, +0.5f );
    _mypid_set_d_lpf( &gstPid, 5.0f );                    // D LPF 5Hz
    _mypid_reset( &gstPid );

    gnMotorTick_prev = 0;
    gnAcc2ms  = 0;
    gnAcc20ms = 0;

    gcnBlank_Active = 0;
    gcnDB_Hold = 0;
    gnDeadbandActTick = 0;
    gnTickPulseTimer_ms = 0;
    gnDB_Hold_ms = 0;
    gcnMotorOverCurrent_f = 0;
    gcnClose_Hold = 0;
    gnPid_D_Period = MOTOR_CONTROL_PERIOD_MS;
    gnfRamp_Scale = SPEED_SCALE_OUTER;

}
static uint8_t gcnDeadbandTickAct_f = 0;
void MYMOTOR_Proc( void )
{
	uint8_t cnraw_close_sw = 0;
	uint8_t cnraw_open_sw = 0;
	uint8_t cnedge_close_rise = 0;
	// uint8_t cnedge_open_rise = 0;
	float nfcurrent_value = 0.0f;
	
	if( 0 == gnMotorTick_prev ){
		gnMotorTick_prev = HAL_GetTick();
	}
	uint32_t nmotortick_now = HAL_GetTick();
	uint32_t ndt = nmotortick_now - gnMotorTick_prev;
	gnMotorTick_prev = nmotortick_now;

	gnAcc2ms += ndt;
	gnAcc20ms += ndt;

	// [추가] 틱 펄스 타이머 처리 (PID 주기와 무관하게 정밀 제어)
	// 펄스가 활성화된 동안에는 시간을 차감하고 모터를 계속 구동
	if( ( 0 == gnMotorDeadbandTick ) && ( 1 == gcnDeadbandTickAct_f ) ){
		// MYUTIL_Printf("D\r\n");
		gcnDeadbandTickAct_f = 0;
		_mymotor_shttuer_cmd( 0, 0.0f, MOTOR_PWM_VALUE_KEEP );
		geState = SD_IDLE;
	}
	
	
	// if( 0 < gnTickPulseTimer_ms ){
	// 	gnTickPulseTimer_ms -= (int32_t)ndt;
	// 	MYUTIL_Printf("4");
		
	// 	if( 0 >= gnTickPulseTimer_ms ){
	// 		gnTickPulseTimer_ms = 0;
	// 		// 펄스 종료 시 정지 명령
	// 		_mymotor_shttuer_cmd( 0, 0.0f, MOTOR_PWM_VALUE_KEEP );
	// 	} else {
	// 		// 펄스 진행 중 구동 유지
	// 		_mymotor_shttuer_cmd( gnTickDir, gnfTickSpeed, MOTOR_PWM_VALUE_CLEAR );
	// 	}
	// }

	// [이동] 휴식(Period) 타이머 처리를 상단으로 올려 20ms 제어 주기와 분리
	if( 0 < gnMotorTick_ms ){
		gnMotorTick_ms -= (int32_t)ndt;
	}

	nfcurrent_value = ADC0_Get_Current();
	cnraw_open_sw = _mymotor_get_open_limit_switch();
	cnraw_close_sw = _mymotor_get_close_limit_switch();

	// printf("0->open %d close %d dir %d current %f \r\n", cnraw_open_sw, cnraw_close_sw, gnCmd_Dir, nfcurrent_value );
	// 과정류 방지
	if( gnfMotorCurretValue < nfcurrent_value ){
		printf("Motor Over current [%.3f]\r\n", nfcurrent_value );
		_mymotor_shttuer_cmd( 0, 0.0f, MOTOR_PWM_VALUE_KEEP );
		gnTickPulseTimer_ms = 0; // 펄스 중단
		_mymotor_break_stop();
		geState = SD_IDLE;
		gcnMotorOverCurrent_f = 1;
		return;
	}

	if( 1 == gcnMotorOverCurrent_f ){
		// 과전류 시 수동 클리어 해야 함
		_mymotor_shttuer_cmd( 0, 0.0f, MOTOR_PWM_VALUE_KEEP );
		gnTickPulseTimer_ms = 0;
		_mymotor_coast_off();
		geState = SD_IDLE;
		return;
	}

	if( ( cnraw_close_sw == 1 ) && ( 0 > gnCmd_Dir ) ){
		// 닫힘 리미트 감지 & 닫는 방향 명령일 때만 정지
		printf("Limit Close: open %d close %d dir %d current %f \r\n", cnraw_open_sw, cnraw_close_sw, gnCmd_Dir, nfcurrent_value );
		gnTickPulseTimer_ms = 0;
		_mymotor_shttuer_cmd( 0, 0.0f, MOTOR_PWM_VALUE_KEEP );
		_mymotor_break_stop();
		geState = SD_IDLE;
		return;
	}

	if ((cnraw_open_sw == 1) && ( 0 < gnCmd_Dir ) ){
		// 열림 리미트 감지 & 여는 방향 명령일 때만 정지
		printf("Limit Open: open %d close %d dir %d fire%d hold %d\r\n", cnraw_open_sw, cnraw_close_sw, gnCmd_Dir, gcnFireEvent_f, gcnClose_Hold );
		gnTickPulseTimer_ms = 0;
		_mymotor_shttuer_cmd( 0, 0.0f, MOTOR_PWM_VALUE_KEEP );
		_mymotor_break_stop();
		geState = SD_IDLE;
		gcnClose_Hold = 0;
		return;
	}

	


	// 2ms: 모터 상태머신 (dead-time/램프/상대편 OFF 보장)
    // [수정] while 루프 안에서 함수를 호출해야 밀린 시간만큼 모터 상태를 업데이트하여
    // 물리적인 가속/감속 속도를 일정하게 유지할 수 있음
    while (gnAcc2ms >= 2) {
		gnAcc2ms -= 2;
		if( 0 < gnMotorDeadbandTick ){
			gnMotorDeadbandTick--; 
		}
		_mymotor_shtter_state_update();
    }

    if( gnAcc20ms < gnPid_D_Period ){ return; }
	gnAcc20ms -= gnPid_D_Period;

	// 차압 변화율 계산 (헌팅 방지용)
	float pa_change_rate = _mymotor_fabsf_local(gnfLpfPaValue - gnfY_Prev);
    gnfY_Prev = gnfLpfPaValue;

	// printf("PA:%.1f, DIR:%d CDIR:%d PWM:%f \r\n", gnfLpfPaValue, gnCmd_Dir, gnCur_Dir, gnfCur_Pwm);

	// 펄스가 동작 중이라면 PID 계산 루틴 진입 금지 (펄스 구동 우선)
	if( gnTickPulseTimer_ms > 0 ){
		return;
	}
	
	// return;
	// 화재 이벤트 감시 Off -> ON
    if (gcnFireEvent_prev != gcnFireEvent_f) {
        if (gcnFireEvent_f == 1) {
        	gcnClose_Hold = 0;
        	gnTickPulseTimer_ms = 0;
            _mypid_reset( &gstPid );            // PID 시작 시 상태 리셋
            gcnDB_Hold   = 0;
        } 
        gcnFireEvent_prev = gcnFireEvent_f;
	}

	// 화재 이벤트 Off : 셔터 닫기 모드
	if( 0 == gcnFireEvent_f ){
		if( gcnClose_Hold ){
			_mymotor_shttuer_cmd( 0, 0.0f, MOTOR_PWM_VALUE_KEEP );
			_mymotor_coast_off();
			geState = SD_IDLE;
			return;
		}

		if( cnraw_close_sw == 1 || nfcurrent_value > gnfMotorCurretValue ) {
			_mymotor_shttuer_cmd( 0, 0.0f, MOTOR_PWM_VALUE_KEEP );
			_mymotor_coast_off();
			geState = SD_IDLE;
			gcnClose_Hold = 1;
		}
		else{
			// 닫기 진행
			gcnInSettleBand_f = SELECT_RAMP_PER_STEP_SETTLE_OUT;
            gnfRamp_Scale = SPEED_SCALE_OUTER; 
			_mymotor_shttuer_cmd( -1, 0.25, MOTOR_PWM_VALUE_KEEP );		// 닫기 속도 조정
		}
		return;
	}

	// 화재 이벤트 ON -> PID 제어
	// PID 모드(화재 감지)

	// 리미트 엣지 블랭키 시
	// cnedge_open_rise = ( cnraw_open_sw && !gcnOpenSw_prev );
	// gcnOpenSw_prev = cnraw_open_sw;
	// if( cnedge_close_rise || cnedge_open_rise ){
		// 	gcnBlank_Active = 1;
		// 	gnBlank_t0_ms = nmotortick_now;
		// }
		
	cnedge_close_rise = ( cnraw_close_sw && !gcnCloseSw_prev );
	gcnCloseSw_prev = cnraw_close_sw;
	if( cnedge_close_rise  ){
		gcnBlank_Active = 1;
		gnBlank_t0_ms = nmotortick_now;
	}

	// 리미트 직후 블랭킹 구간 : BLANK_AFTER_LIMIT_MS 시간만큼 PID 정지
	if( gcnBlank_Active ){
		gnMotorLimitSwStablTimer = MOTOR_LIMIT_SWITCH_STABLE_TIME_MS;
		if( ( nmotortick_now - gnBlank_t0_ms ) < MOTOR_BLOCK_ACTIVE_TIME_MS ){
			// [수정] 블랭킹 중이라도, 리미트에서 '벗어나는' 방향의 명령은 즉시 허용
			// 1. 현재 압력과 목표 압력으로 즉시 움직여야 할 방향을 계산
			// float nfmeasured_pa = gnfLpfPaValue;
			// float nfsp = gnfTargetDpValue;
			// float nfpa_gap = nfsp - nfmeasured_pa;
			// int desired_dir = 0;

			// // 2. Deadband를 고려하여 유효한 움직임 방향 결정
			// if (nfpa_gap > gnfDeadBand_Pa) {
			// 	desired_dir = 1; // Open 방향으로 움직여야 함
			// } else if (nfpa_gap < -gnfDeadBand_Pa) {
			// 	desired_dir = -1; // Close 방향으로 움직여야 함
			// }

			// // 3. 현재 눌린 리미트 스위치와 '반대' 방향으로 움직이려 하는지 확인
			// // gcnOpenSw_prev가 1이라는 것은, 이전 사이클에서 Open 리미트가 눌렸다는 의미
			// if ((gcnOpenSw_prev == 1 && desired_dir < 0) ||  // Open 리미트에서 Close 하려 할 때
			// 	(gcnCloseSw_prev == 1 && desired_dir > 0))    // Close 리미트에서 Open 하려 할 때
			// {
			// 	// 벗어나는 동작은 허용 -> 블랭킹 즉시 종료
			// 	gcnBlank_Active = 0;
			// } else {
				// 리미트 방향으로 더 가려 하거나, Deadband 내에 있어 멈추는 경우 -> 블랭킹 유지
				_mymotor_shttuer_cmd( 0, 0.0f, MOTOR_PWM_VALUE_KEEP );
				_mymotor_coast_off();
				geState = SD_IDLE;
				// MYUTIL_Printf("B\r\n");
				
				return;  // 블랭킹 중 : PID 동작 일시 정지
			// }
		}
		else{
			// MYUTIL_Printf("GO\r\n");
			_mypid_reset( &gstPid );
			gcnBlank_Active = 0;
		}
	}

	float nfmeasured_pa = gnfLpfPaValue;
	float nfsp = gnfTargetDpValue;
	float nfpa_gap = nfsp - nfmeasured_pa;
	float nfpa_abs_gap = ( nfpa_gap >= 0.0f ) ? nfpa_gap : -nfpa_gap;
	
	const float nfdeadband_tmp = gnfDeadBand_Pa;  			// 안전 구간
	const float nfsettleband_tmp = gstPid.settle_band;  	// settle_band
	const float pa_change_threshold = 1.1f; // 차압 급변 판정 임계값 (20ms당 변화량)

	// 리미트 스위치가 눌려있을 때는 Tick 모드(Deadband)에 진입하지 않고 PID 제어로 즉시 벗어나도록 함
	int is_at_limit =  cnraw_close_sw;  			// open 제외 cnraw_open_sw ||

	// Deadband In : 틱 모드(Hold 유지 + 아주 가끔씩 살짝 움직임)
	int ntick_dir = 0;
	float nftick_speed = 0.0f;
	uint8_t cnpa_chage_rate_f = 0;
	uint8_t cnlimit_sw_stable_f = 0;
	// [수정] 복잡한 조건문 단순화
	// 1. 리미트 스위치가 아닐 것
	// 2. 데드밴드(오차 범위) 안에 있을 것
	// 3. 차압 변화율이 임계값 이상일 것 (급변 시 Tick 모드로 진입하여 펄스 구동/정지로 제동)
	// MYUTIL_Printf("<%f/%f/%f/%d/%d>\r\n",pa_change_rate, nfpa_abs_gap, gnfCur_Pwm, cnraw_open_sw, cnraw_close_sw);
	if( pa_change_rate > pa_change_threshold ){
		// MYUTIL_Printf("A[%d]\r\n", is_at_limit);
		cnpa_chage_rate_f = 1;
	}
	// [수정] 리미트 스위치가 눌려있어도 오차가 설정된 최소값(Min_Pa) 이내라면 정지를 위해 틱 모드 로직 진입 허용
	// !is_at_limit 조건 때문에 최소 오차 이내에서도 PID로 넘어가 헌팅하는 문제 해결
	if( gnfDeadBand_Tick_Min_Pa >= nfpa_abs_gap ){  
		// MYUTIL_Printf("B\r\n");
		_mymotor_shttuer_cmd( 0, 0.0f, MOTOR_PWM_VALUE_KEEP );
		_mymotor_coast_off();
		geState = SD_IDLE;
		
		return;
	}
	
	if( is_at_limit ){
		if( gnfSettledBand_Pa > nfpa_abs_gap ){
			gnMotorLimitSwStablTimer = 5000;
		}
		else{
			gnMotorLimitSwStablTimer = 0;
		}
		
	}
	
	
	if( 0 != gnMotorLimitSwStablTimer ){
		// MYUTIL_Printf("C\r\n");
		cnlimit_sw_stable_f =1;
	}

	// 260325: simba 조건문 확인
	if ((!is_at_limit && (nfpa_abs_gap <= nfdeadband_tmp)) || (cnpa_chage_rate_f == 1) || (cnlimit_sw_stable_f == 1)){
		// 휴식 시간(Cycle OFF Time)이 끝났을 때만 동작 판단
		if (gnMotorTick_ms <= 0) {
			// 설정된 최소 오차보다 클 때만 틱 발생
			// [수정] 급변 상황(cnpa_chage_rate_f)이라도 목표값 근처(Min_Pa 이내)에 도달했으면 틱 동작을 하지 않도록 OR 조건 제거
			if( ( gnfDeadBand_Tick_Min_Pa < nfpa_abs_gap) || ( 1 == cnpa_chage_rate_f )){
				// 방향 및 속도 결정
				if( 0.0f < nfpa_gap ){
					nftick_speed = gnfMotorMinUpperSpeed;// * MOTOR_OPEN_CLOSE_RATIO;
					ntick_dir = +1;  		// 압력이 낮음 -> 조금 열기
					if( nftick_speed > gnfMotorMaxUpperSpeed ){
						nftick_speed = gnfMotorMaxUpperSpeed;
					}
				}
				else if( 0.0f > nfpa_gap ){
					nftick_speed = gnfMotorMinDownSpeed ;
					ntick_dir = -1;  		// 압력이 높음 -> 조금 닫기
					if( nftick_speed > gnfMotorMaxDownSpeed ){
						nftick_speed = gnfMotorMaxDownSpeed;
					}
				}

				// Safety Limit switch
				if( ( ( 0 < ntick_dir ) && ( 1 == cnraw_open_sw ) ) || ( ( 0 > ntick_dir ) && ( 1 == cnraw_close_sw ) ) ){
					ntick_dir = 0;
					nftick_speed = 0.0f;
					// MYUTIL_Printf("1");
				}

				// printf("DEADBAND TICK START dir:%d spd:%f pwm:%f\r\n", ntick_dir, nftick_speed, gnfCur_Pwm);
				// MYUTIL_Printf("2");
				if( ntick_dir != 0 ){
					// MYUTIL_Printf("3");
					// 상태 저장
					gnTickDir = ntick_dir;
					gnfTickSpeed = nftick_speed;
					gcnInSettleBand_f = SELECT_RAMP_PER_STEP_SETTLE_TICK;
					gnfRamp_Scale = SPEED_SCALE_OUTER;

					// PWM 동작 설정: ON 시간(Tick) 설정, OFF 시간(Period) 설정
					// [수정] Tick 단위를 10ms 단위로 변경 (1=10ms, 3=30ms). 1ms 단위는 너무 짧아 시스템 지연 시 동작 누락됨.
					uint32_t tick_duration = 0;//( gnDeadBand_Tick > 0 ) ? gnDeadBand_Tick : 20;
					if( 0.0f < nfpa_gap ){
						tick_duration = ( gnDeadBand_UpperTick > 0 ) ? gnDeadBand_UpperTick : 20;
					}
					else if( 0.0f > nfpa_gap ){
						tick_duration = ( gnDeadBand_DownTick > 0 ) ? gnDeadBand_DownTick : 20;
					}
					// gnTickPulseTimer_ms = tick_duration; // 모터 ON 시간 (ms 단위)
					gnMotorDeadbandTick = tick_duration;
					gcnDeadbandTickAct_f = 1;

					// 펄스 즉시 시작
					// if( !is_at_limit ){
						// MYUTIL_Printf("N\r\n");
						gnMotorTick_ms = gnDeadBand_Period;     // 다음 동작까지 쉴 시간 (OFF Time)
					// }
					// else{
						// gnMotorTick_ms = 100;
					// }
					_mymotor_shttuer_cmd( gnTickDir, gnfTickSpeed, MOTOR_PWM_VALUE_CLEAR );
				}
			}
			else{
				// MYUTIL_Printf("S\r\n");
				_mymotor_shttuer_cmd( 0, 0.0f, MOTOR_PWM_VALUE_KEEP );
				_mymotor_coast_off();
				geState = SD_IDLE;
				// gnTickPulseTimer_ms = 0;
				// gnMotorDeadbandTick = 0;
			}
			
		}

		gcnPidMode_f = 0; 	
		return;
	}
	if( 0 < gnMotorTick_ms ){
		// MYUTIL_Printf("T[%f]\r\n", pa_change_rate);
		 return;
		}
	// 조건 2: 급격하게 증가할 경우 OUTER가 아닌 SETTLEBAND로 동작하게 해줘

	// 데드밴드를 벗어나 PID 제어를 해야 한다면 휴식 타이머를 초기화하여 즉시 반응하도록 함
	

	// gnMotorTick_ms = 0;
	// MYUTIL_Printf("[%f]\r\n", nfpa_abs_gap);
	if( 0 == gcnPidMode_f ){
		// MYUTIL_Printf("PID START\r\n");
		_mypid_reset( &gstPid );
		gcnPidMode_f = 1;
	}
	

	// [추가] 리미트 스위치 감지 시 적분항(I-Term) Windup 방지 (즉시 반대 방향 구동을 위해)
	// 닫힘 리미트인데 적분항이 음수(닫힘)이거나, 열림 리미트인데 적분항이 양수(열림)인 경우 클리어
	// MYUTIL_Printf("cl %d op %d int:%f\r\n", cnraw_close_sw, cnraw_open_sw, gstPid.integrator);
	
	if( ( cnraw_close_sw && ( gstPid.integrator < 0.0f ) ) || ( cnraw_open_sw && ( gstPid.integrator > 0.0f ) ) ){
		gstPid.integrator = 0.0f;
	}

	// [수정] PID 내부에서 이미 Gain Scheduling을 수행하므로,
	// 외부에서 강제로 속도를 줄이는 nfspeed_scale 로직은 제거하여 모터 Stall 방지
	float nfspeed_scale = 1.0f;

	// 오차가 안정화 구간 내에 있거나, 차압이 급격하게 변할 경우(급증/급감) Settle Band 모드로 진입하여 감속 유도
	if( ( nfpa_abs_gap < nfsettleband_tmp ) || ( pa_change_rate > pa_change_threshold ) ){
		gcnInSettleBand_f = SELECT_RAMP_PER_STEP_SETTLE_IN;
		gnfRamp_Scale =SPEED_SCALE_SETTLEBAND_SCALE;
	}
	else{
		gcnInSettleBand_f = SELECT_RAMP_PER_STEP_SETTLE_OUT;
		gnfRamp_Scale = nfspeed_scale;
	}

	// Caclulate PID
	float nfu = _mypid_update( &gstPid, nfsp, nfmeasured_pa );
	int ndir = ( 0.0f < nfu ) ? +1 : ( nfu < 0.0f ? -1 : 0 );
	float nfmag = ( 0.0f <= nfu ) ? nfu : -nfu;
	float nfmax_upper = gnfMotorMaxUpperSpeed * nfspeed_scale;
	float nfmax_down = gnfMotorMaxDownSpeed * nfspeed_scale;

	// [추가] Settle Band에서 감속하더라도 모터 최소 구동 속도 이하로 떨어지지 않도록 보정
	if( nfmax_upper < gnfMotorMinUpperSpeed ){ nfmax_upper = gnfMotorMinUpperSpeed; }
	if( nfmax_down < gnfMotorMinDownSpeed ){ nfmax_down = gnfMotorMinDownSpeed; }

    // printf("target %f(%f) mag %f(d:%d) hold %d %f \r\n", nfsp, nfmeasured_pa, nfmag, ndir, gcnDB_Hold, gnfCur_Pwm);
	// printf("target (%f) mag %f(d:%d) hold %d %f \r\n", nfspeed_scale, nfmag, ndir, gcnDB_Hold, gnfCur_Pwm);

	// [수정] 이중 감속 제거
	// PID 내부에서 이미 settle_band에 따른 게인 조절을 하고 있으므로,
	// 여기서 nfspeed_scale을 또 곱하면 힘이 너무 약해져서 목표에 도달하지 못함.
	// nfmag *= nfspeed_scale;
		
	// 최소 듀티 보장
	if( ( ndir != 0 ) && ( nfmag > 0.0f )){
		if( nfmag < gnfMotorMinDownSpeed ){ 
			nfmag = gnfMotorMinDownSpeed;
		}

		if( 0 > ndir ) {
			if( nfmag > nfmax_down) {
				nfmag = nfmax_down;
			}
		}
		if( 0 < ndir ) {
			if( nfmag > nfmax_upper) {
				nfmag = nfmax_upper;
			}
		}
		
	}

	// 안전망(중복이지만 확실히)
    if ((ndir > 0 && cnraw_open_sw) || (ndir < 0 && cnraw_close_sw) ){
    	ndir = 0;
    	nfmag = 0.0f;
    	// [추가] 리미트 스위치에 막혀 이동할 수 없는 경우 PID 내부 상태(가속도, 적분항)를 리셋
    	// 이를 통해 목표값이 변경되어 반대 방향으로 이동해야 할 때 즉시 반응하도록 함 (Windup 방지)
    	gstPid.prev_u = 0.0f;
    	gstPid.integrator = 0.0f;
    }
	// MYUTIL_Printf("M\r\n");
    _mymotor_shttuer_cmd( ndir, nfmag, MOTOR_PWM_VALUE_KEEP );
}

float MYMOTOR_Get_Motor_Pwm( void )
{
	return gnfCur_Pwm;
}


uint8_t MYMOTOR_Get_Open_LimitSwtich_St( void )
{
	return _mymotor_get_open_limit_switch();
}

uint8_t MYMOTOR_Get_Close_LimitSwtich_St( void )
{
	return _mymotor_get_close_limit_switch();
}

/////////////////////////////////////////////////////////////////////////////////////////////
// local function implement
// OPEN PLUS : MINUS >> M0
// ClOSE MINUS : PLUS >> M1
// OFF : MINUS : MINUS
void _mymotor_set_open_pwm( const float _nfduty )
{
	if( 1 == _mymotor_get_close_gnd_state() ){
		_mymotor_gnd_close_control( PIN_STATE_OFF );
	}
	else{
		if( MOTOR_STATE_NORMAL_CLOSE == gnMotorNonc ){
			MYPWM_Set_Duty( MOTOR_PWM_CHANNEL_M0, _nfduty );
		}
		else{
			MYPWM_Set_Duty( MOTOR_PWM_CHANNEL_M1, _nfduty );
		}
	}
}

void _mymotor_set_close_pwm( const float _nfduty )
{
	if( 1 == _mymotor_get_open_gnd_state() ){
		_mymotor_gnd_open_control( PIN_STATE_OFF );
	}
	else{
		if( MOTOR_STATE_NORMAL_CLOSE == gnMotorNonc ){
			MYPWM_Set_Duty( MOTOR_PWM_CHANNEL_M1, _nfduty );
		}
		else{
			MYPWM_Set_Duty( MOTOR_PWM_CHANNEL_M0, _nfduty );
		}
	}
}

void _mymotor_break_stop( void )
{
	MYPWM_Set_Duty( MOTOR_PWM_CHANNEL_M0, 0.0f );
	MYPWM_Set_Duty( MOTOR_PWM_CHANNEL_M1, 0.0f );
	_mymotor_gnd_open_control( PIN_STATE_ON );
	_mymotor_gnd_close_control( PIN_STATE_ON );

	gnCur_Dir = 0;
	gnfCur_Pwm = 0.0f;
	geState = SD_IDLE;
}



uint8_t _mymotor_get_open_limit_switch( void )
{
	uint8_t cnret  = 0;

	if( MOTOR_STATE_NORMAL_CLOSE == gnMotorNonc ){
		cnret = MYGPIO_Get_LimitSwitch_State( LIMIT_SWITCH_SEL_0 );
	}
	else{
		cnret = MYGPIO_Get_LimitSwitch_State( LIMIT_SWITCH_SEL_1 );
	}
	return cnret;
}

uint8_t _mymotor_get_close_limit_switch( void )
{
	uint8_t cnret  = 0;

	if( MOTOR_STATE_NORMAL_CLOSE == gnMotorNonc ){
		cnret = MYGPIO_Get_LimitSwitch_State( LIMIT_SWITCH_SEL_1 );
	}
	else{
		cnret = MYGPIO_Get_LimitSwitch_State( LIMIT_SWITCH_SEL_0 );
	}
	return cnret;
}

void _mymotor_coast_off( void )
{
	_mymotor_set_open_pwm( 0.0f );
	_mymotor_set_close_pwm( 0.0f );
	_mymotor_gnd_open_control( PIN_STATE_OFF );
	_mymotor_gnd_close_control( PIN_STATE_OFF );
}

uint8_t _mymotor_get_open_gnd_state( void )
{
#if 0
	uint8_t cnret = 0;
	if( MOTOR_STATE_NORMAL_CLOSE == gnMotorNonc ){
		cnret = HAL_GPIO_ReadPin( MYGPIO_MOTOR_GPIO_PORT, MYGPIO_MOTOR_CTRL_S1_GPIO_NUM );
	}
	else{
		cnret = HAL_GPIO_ReadPin( MYGPIO_MOTOR_GPIO_PORT, MYGPIO_MOTOR_CTRL_S0_GPIO_NUM );
	}
	return cnret;
#endif
	return 0;
}

void _mymotor_gnd_open_control( uint8_t _cnCtrl )
{
#if 0
	if( MOTOR_STATE_NORMAL_CLOSE == gnMotorNonc ){
		HAL_GPIO_WritePin( MYGPIO_MOTOR_GPIO_PORT, MYGPIO_MOTOR_CTRL_S1_GPIO_NUM, (GPIO_PinState)_cnCtrl );
	}
	else{
		HAL_GPIO_WritePin( MYGPIO_MOTOR_GPIO_PORT, MYGPIO_MOTOR_CTRL_S0_GPIO_NUM, (GPIO_PinState)_cnCtrl );
	}
#endif
}

uint8_t _mymotor_get_close_gnd_state( void )
{
#if 0
	uint8_t cnret = 0;

	if( MOTOR_STATE_NORMAL_CLOSE == gnMotorNonc ){
		cnret = HAL_GPIO_ReadPin( MYGPIO_MOTOR_GPIO_PORT, MYGPIO_MOTOR_CTRL_S0_GPIO_NUM );
	}
	else{
		cnret = HAL_GPIO_ReadPin( MYGPIO_MOTOR_GPIO_PORT, MYGPIO_MOTOR_CTRL_S1_GPIO_NUM );
	}
	return cnret;
#endif
	return 0;
}

void _mymotor_gnd_close_control( uint8_t _cnCtrl )
{
#if 0
	if( MOTOR_STATE_NORMAL_CLOSE == gnMotorNonc ){
		HAL_GPIO_WritePin( MYGPIO_MOTOR_GPIO_PORT, MYGPIO_MOTOR_CTRL_S0_GPIO_NUM, (GPIO_PinState)_cnCtrl );
	}
	else{
		HAL_GPIO_WritePin( MYGPIO_MOTOR_GPIO_PORT, MYGPIO_MOTOR_CTRL_S1_GPIO_NUM, (GPIO_PinState)_cnCtrl );
	}
#endif
}	

static float _mymotor_clamp01( float _nfv )
{
	float nfret = _nfv;
	if( nfret < 0.0f ){
		nfret = 0.0f;
	}
	if( nfret > 1.0f ){
		nfret = 1.0f;
	}
	
	return nfret;
}

static float _mymotor_fabsf_local(float x){
	return ( x < 0 ) ? -x : x;
}

void _mymotor_shttuer_cmd( int _nDir, float _nfSpeed, uint8_t _cnClearPwm )
{
	gnfCmd_Speed = _mymotor_clamp01( _nfSpeed );

	if( MOTOR_PWM_VALUE_CLEAR == _cnClearPwm ){
		// [수정] 틱 모드 등에서 즉시 반응하기 위해 초기 PWM을 MinSpeed가 아닌 목표 속도로 바로 설정
		gnfCur_Pwm = gnfCmd_Speed;
	}
	
	gnCmd_Dir = ( _nDir > 0 ) ? +1 : ( _nDir < 0 ? -1 : 0 );
	// [수정] gnfCur_Pwm은 float 타입이므로 %d 대신 %f를 사용해야 인자가 밀리지 않고 정상 출력됨
	// MYUTIL_Printf("speed[%f] PWM[%f] dir[%d/%d]\r\n", gnfCmd_Speed, gnfCur_Pwm, gnCmd_Dir, _nDir);
}

// gcnDeviceId 2
// gcnRs485_Baudrate 2
// gnfTargetDpValue 50.000
// gnfNormalUpperRamp 0.050
// gnfNormalDwonRamp 0.010
// gnfSettledBand_Pa 15.000
// gnfSettleBandUpperRamp 0.003
// gnfSettleBandDownRamp 0.003
// gnfDeadBand_Pa 8.000
// gnDeadBand_Period 700
// gnDeadBand_Tick 3
// gnfMotorMinSpeed 0.002
// gnfMotorMaxSpeed 0.010
// gnfMotorPid_P 0.150
// gnfMotorPid_I 0.050
// gnfMotorPid_D 0.020
void _mymotor_shtter_state_update( void )
{
	uint32_t nnow = HAL_GetTick();
	float nf_norm = gnfCmd_Speed;
	float nftargetspd = 0.0f;
	float nfstep_up = gcnInSettleBand_f ? gnfSettleBandUpperRamp : gnfNormalUpperRamp;
	float nfstep_down = gcnInSettleBand_f ? gnfSettleBandDownRamp : gnfNormalDwonRamp;

	nfstep_up *= gnfRamp_Scale;
	nfstep_down *= gnfRamp_Scale;

	if( ( 0 != gnCmd_Dir ) && ( 0.0f >= nf_norm ) ){
		nftargetspd = 0.0f;
	}
	else{
		// if( SELECT_RAMP_PER_STEP_SETTLE_IN == gcnInSettleBand_f ){
		// // 	nftargetspd = nf_norm;// _mymotor_speed_to_duty( nf_norm );
		// 	if( 0 < gnCmd_Dir ){
		// 		nftargetspd = _mymotor_speed_to_duty( nf_norm, gnfMotorMaxUpperSpeed );	
		// 	}
		// 	else{
		// 		nftargetspd = _mymotor_speed_to_duty( nf_norm, gnfMotorMaxDownSpeed );
		// 	}
		// }
		// else{
		// // if( SELECT_RAMP_PER_STEP_SETTLE_TICK == gcnInSettleBand_f ){
		// 	nftargetspd = nf_norm;
		// }
        // [수정] 상위 로직(MYMOTOR_Proc)에서 이미 Min/Max 속도 및 감속 비율을 
        // 계산하여 nf_norm에 반영했으므로, 여기서는 그대로 목표 속도로 사용함.
		nftargetspd = nf_norm;
		
	}
	// if( 0.0f != nftargetspd ){
    // [수정] 2ms 주기 함수 내 printf 제거 (시스템 지연 방지)
	// MYUTIL_Printf("PA:%.1f\r\n", gnfLpfPaValue);
	// [주의] 2ms 루프에서 printf 사용 시 UART 지연(약 7ms)으로 인해 Tick 타이머가 즉시 만료되는 문제 발생하므로 주석 처리
	// printf("22 st[%d] up:%.4f st down:%.4f target:%.4f pwm:%.4f\r\n",geState, nfstep_up, nfstep_down, nftargetspd, gnfCur_Pwm);
    // }
	
    switch (geState)
    {
	    case SD_IDLE:
	        if( ( 0 == gnCmd_Dir ) || ( 0.0f >= nftargetspd ) ){
	            _mymotor_coast_off();
	            gnCur_Dir = 0;
	            gnfCur_Pwm = 0.0f;
	        } 
	        else{
	        	// 방향이 바뀌면 먼저 정지 후 SWITCH_PAUSE 함
	            if( ( gnCmd_Dir != gnCur_Dir ) && ( 0 != gnCur_Dir ) ){
	                _mymotor_coast_off();
	                gnDirSwitch_ts = nnow;
	                geState = SD_SWITCH_PAUSE;
	            }
	            else{
	                gnCur_Dir = gnCmd_Dir;
	                // 상대편 완전 OFF
	                if( 0 < gnCur_Dir ){
	                	_mymotor_gnd_close_control( PIN_STATE_OFF ); 
	                	_mymotor_set_close_pwm( 0.0f ); 
	                }
	                else{ 
	                	_mymotor_gnd_open_control( PIN_STATE_OFF );
	                	_mymotor_set_open_pwm( 0.0f );
	                }

	                // 해당편 GND ON 후 램프업
	                if( 0 < gnCur_Dir ){
	                	_mymotor_gnd_open_control( PIN_STATE_ON );
	                }
	                else{
	                	_mymotor_gnd_close_control( PIN_STATE_ON );
	                }

	                float next = gnfCur_Pwm + nfstep_up;
	                if( next > nftargetspd ){
	                	next = nftargetspd;
	                }

	                gnfCur_Pwm = next;

	                if( 0 < gnCur_Dir ){
	                	_mymotor_set_open_pwm(gnfCur_Pwm);
	                }
	                else{
	                	_mymotor_set_close_pwm(gnfCur_Pwm);
	                }

	                geState = SD_RUN;
	            }
	        }
	        break;

	    case SD_RUN:
	        if( ( ( 0 != gnCmd_Dir ) && ( gnCmd_Dir != gnCur_Dir ) ) 
	         || ( ( 0 == gnCmd_Dir ) || ( 0.0f >= nftargetspd  ) ) ){
	            // 램프다운
	            float next = gnfCur_Pwm - nfstep_down;
	            if( 0.0f > next ){
	            	next = 0.0f;
	            }
	            gnfCur_Pwm = next;
	            
	            if( 0 < gnCur_Dir ){
	            	_mymotor_set_open_pwm(gnfCur_Pwm);
	            }
	            else{
	            	_mymotor_set_close_pwm(gnfCur_Pwm);
	            }

	            if( 0.0f >= gnfCur_Pwm ) {
	                _mymotor_coast_off();
	                gnDirSwitch_ts = nnow;
	                geState = SD_SWITCH_PAUSE;
	            }
	        }
	        else{
	            // 같은 방향 유지: 목표로 램프
	            float step = (nftargetspd >= gnfCur_Pwm) ? nfstep_up : nfstep_down;
	            float delta = nftargetspd - gnfCur_Pwm;

	            if( _mymotor_fabsf_local(delta) <= step ){
	            	gnfCur_Pwm = nftargetspd;
	            }
	            else{
	            	gnfCur_Pwm += (delta > 0 ? step : -step);
	            }

	            // 상대편은 항상 OFF 유지
	            if( 0 < gnCur_Dir ){
	                _mymotor_gnd_close_control( PIN_STATE_OFF );
	                _mymotor_set_close_pwm( 0.0f );
	                _mymotor_gnd_open_control( PIN_STATE_ON );
	                _mymotor_set_open_pwm(gnfCur_Pwm);
	            } 
	            else{
	                _mymotor_gnd_open_control( PIN_STATE_OFF );
	                _mymotor_set_open_pwm( 0.0f );
	                _mymotor_gnd_close_control( PIN_STATE_ON );
	                _mymotor_set_close_pwm(gnfCur_Pwm);
	            }
	        }
	        break;

	    case SD_SWITCH_PAUSE:
	        if( SWITCH_DELAY_MS <= ( nnow - gnDirSwitch_ts ) ){
	            if( ( 0 == gnCmd_Dir ) || ( 0.0f >= nftargetspd ) ) {
	                _mymotor_coast_off();
	                gnCur_Dir = 0;
	                gnfCur_Pwm = 0.0f;
	                geState = SD_IDLE;
	            }
	            else{
	                gnCur_Dir = gnCmd_Dir;

	                // 상대편 OFF 보증
	                if( 0 < gnCur_Dir ){
	                	_mymotor_gnd_close_control( PIN_STATE_OFF );
	                	_mymotor_set_close_pwm(0.0f);
	                }
	                else{
	                	_mymotor_gnd_open_control( PIN_STATE_OFF );
	                	_mymotor_set_open_pwm(0.0f);
	                }

	                // 해당편 ON + 램프업 시작
	                if( 0 < gnCur_Dir ){
	                	_mymotor_gnd_open_control( PIN_STATE_ON );
	                }
	                else{
	                	_mymotor_gnd_close_control( PIN_STATE_ON );
	                }

	                float next = gnfCur_Pwm + nfstep_up;
	                if( next > nftargetspd ){
	                	next = nftargetspd;
	                }
	                gnfCur_Pwm = next;
	                
	                if( 0 < gnCur_Dir ){
	                	_mymotor_set_open_pwm(gnfCur_Pwm);
	                }
	                else{
	                	_mymotor_set_close_pwm(gnfCur_Pwm);
	                }

	                geState = SD_RUN;
	            }
	        } 
	        else{
	            _mymotor_coast_off();
	        }
	        break;
    }
}
// Forward 1:+, 2:-, Reverse 1:-, 2:+

////////////////////////////
static float clampf(float v, float lo, float hi)
{
    return ( v < lo ) ? lo : ( ( v > hi ) ? hi : v );
}

void _mypid_init( MYPID_t *p, float kp, float ki, float kd, float dt )
{
    p->kp = kp;
    p->ki = ki;
    p->kd = kd;
    p->dt = dt;
    p->out_min =-1.0f;
    p->out_max =+1.0f;
    p->i_min =-0.5f;
    p->i_max =+0.5f;
    p->d_hz = 0.0f;
    p->integrator = 0.0f;
    p->prev_err = 0.0f;
    p->d_state = 0.0f;
    p->first_run = 1;

    // 안정화 구간
    p->settle_band = gnfSettledBand_Pa;  			// gnfDeadBand <= settle_band * 0.5
    p->settle_scale = 0.0001f;
    // p->u_slew = 0.01f;
    p->u_slew = 0.002f;
    p->prev_u = 0.0f;
}

void _mypid_set_limits( MYPID_t *p, float out_min, float out_max, float i_min, float i_max )
{
    p->out_min = out_min;
    p->out_max = out_max;
    p->i_min = i_min;
    p->i_max = i_max;
}

void _mypid_set_d_lpf(MYPID_t *p, float d_hz)
{
	p->d_hz = d_hz; 
}

void _mypid_reset(MYPID_t *p)
{
    p->integrator = 0.0f;
    p->prev_err = 0.0f;
    p->d_state = 0.0f;
    p->first_run = 1;
    p->prev_u = 0.0f;
}

float _mypid_update(MYPID_t *p, float sp, float y)
{
    float err = sp - y;
    
    // 안정화 구간 페이퍼 게인
    float ae = (err >=0)? err: -err;
    float s = clampf(ae / (p->settle_band > 1e-6f ? p->settle_band : 1e-6f ), 0.0f, 1.0f);
	float scale = p->settle_scale + ( 1.0f - p->settle_scale) * (s*s);
	float kp = p->kp * scale;
	float ki = p->ki * scale;
	float kd = p->kd * scale;
    // P
    float P = kp * err;

    // I (clamp)
    p->integrator += ( ki * err ) * p->dt;
    p->integrator = clampf( p->integrator, p->i_min, p->i_max );

    // D with 1st-order LPF on derr/dt
    float D = 0.0f;
    if( ( kd > 0.0f ) && ( p->d_hz > 0.0f ) ){
        float derr = p->first_run ? 0.0f : (err - p->prev_err) / p->dt;
        float a = ( 2.0f * 3.1415926f * p->d_hz * p->dt );
        a = a / ( 1.0f + a );                    // alpha
        p->d_state = p->d_state + a * ( derr - p->d_state );
        D = kd * p->d_state;
    }

    float u = P + p->integrator + D;

    // Saturation + simple back-calculation anti-windup
    float u_clamped = clampf(u, p->out_min, p->out_max);
    if( u != u_clamped ){
        p->integrator += 0.1f * (u_clamped - u);
        p->integrator = clampf( p->integrator, p->i_min, p->i_max );
        u = u_clamped;
    }

    float du = u - p->prev_u;
    float max_du = p->u_slew;

    if( du > max_du ){
    	u = p->prev_u + max_du;
    }
    if( du < -max_du ){
    	u = p->prev_u - max_du;
    }
    p->prev_u = u;
    p->prev_err = err;
    p->first_run = 0;
    return u; // [-1..+1] : +는 OPEN, -는 CLOSE
}

#else
/* Motor control is disabled; keep GUI and Modbus status queries available. */
void MYMOTOR_Init( void ) {}
void MYMOTOR_Proc( void ) {}
float MYMOTOR_Get_Motor_Pwm( void ) { return 0.0f; }
uint8_t MYMOTOR_Get_Open_LimitSwtich_St( void ) { return LIMIT_SWITCH_STATE_OFF; }
uint8_t MYMOTOR_Get_Close_LimitSwtich_St( void ) { return LIMIT_SWITCH_STATE_OFF; }
#endif /* BOARD_MOTOR_ENABLE */
