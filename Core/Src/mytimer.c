#include "global.h"
#include "mytimer.h"
#include "my7segment.h"

#define SEVEN_SEG_DIGIT_COUNT  3U
#define SEVEN_SEG_REFRESH_HZ   1500U
#define SEVEN_SEG_STEP_HZ      (SEVEN_SEG_REFRESH_HZ / SEVEN_SEG_DIGIT_COUNT)

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

static void _mytimer_tim6_init(void);
#if BOARD_MOTOR_ENABLE
static void _mytimer_tim1_init(void);
static uint32_t _mytimer_duty_to_ccr(float duty, uint32_t arr);
#endif

TIM_HandleTypeDef htim6; /* 7-segment scan timer */
#if BOARD_MOTOR_ENABLE
TIM_HandleTypeDef htim1; /* PB13 TIM1_CH1N motor PWM */
#endif

void MYTIMER_Init(void)
{
    _mytimer_tim6_init();
#if BOARD_MOTOR_ENABLE
    _mytimer_tim1_init();
#endif
}

/* Kept for HAL users; the IRQ handler also performs the scan directly. */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM6) {
        MY7SEGMENT_Proc();
    }
}

void MYTIMER_Proc(void)
{
    if (0 < gnTestTimer) { gnTestTimer--; }
    if (0 < gnBootTimer) { gnBootTimer--; }
    if (0 < gnGuiTimer) { gnGuiTimer--; }
    if (0 < gnDpSensorTimer) { gnDpSensorTimer--; }
    if (0 < gnBtnTimer) { gnBtnTimer--; }
    if (0 < gnBuzzerTimer) { gnBuzzerTimer--; }
    if (0 < gnMotorTimer) { gnMotorTimer--; }
    if (0 < gnGuiSensorTimer) { gnGuiSensorTimer--; }
    if (0 < gnMotorHoldTimer) { gnMotorHoldTimer--; }
    if (0 < gnMotorFixedSpeedTimer) { gnMotorFixedSpeedTimer--; }
    if (0 < gnGuiReturnTimer) { gnGuiReturnTimer--; }
    if (0 < gnSaveEnvDelayTimer) { gnSaveEnvDelayTimer--; }
    if (0 < gnModbusTxTimer) { gnModbusTxTimer--; }
    gnFndBlinkTimer++;
    if (0x1000000 < gnFndBlinkTimer) { gnFndBlinkTimer = 0; }
    if (0 < gnMotorDeadbandTick) { gnMotorDeadbandTick--; }
    if (0 < gnMotorLimitSwStablTimer) { gnMotorLimitSwStablTimer--; }
    if (0 < gnUartCheckTimer) { gnUartCheckTimer--; }
}

#if BOARD_MOTOR_ENABLE
void MYPWM_Set_Duty(uint8_t channel, float duty)
{
    static uint8_t pwm_active = 0U;
    uint32_t ccr;
    const uint32_t arr = __HAL_TIM_GET_AUTORELOAD(&htim1);

    (void)channel; /* VNH7070 has one PWM input; retain the old API. */
    ccr = _mytimer_duty_to_ccr(duty, arr);
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, ccr);

    if (0U == ccr) {
        /* Stop CH1N and actively drive PB13 low. */
        (void)HAL_TIMEx_PWMN_Stop(&htim1, TIM_CHANNEL_1);
        HAL_GPIO_WritePin(MOTOR_PWM_GPIO_Port, MOTOR_PWM_Pin, GPIO_PIN_RESET);
        pwm_active = 0U;
    }
    else if (0U == pwm_active) {
        HAL_TIM_MspPostInit(&htim1);
        if (HAL_OK == HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_1)) {
            pwm_active = 1U;
        }
    }
}

static void _mytimer_tim1_init(void)
{
    TIM_OC_InitTypeDef oc = {0};
    GPIO_InitTypeDef gpio = {0};

    /* PB0/PB1 remain FND A/B GPIOs; only PB13 is motor PWM. */
    HAL_GPIO_WritePin(MOTOR_PWM_GPIO_Port, MOTOR_PWM_Pin, GPIO_PIN_RESET);
    gpio.Pin = MOTOR_PWM_Pin;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(MOTOR_PWM_GPIO_Port, &gpio);

    htim1.Instance = TIM1;
    htim1.Init.Prescaler = 0U;
    htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim1.Init.Period = 2399U; /* 48 MHz / 2400 = 20 kHz */
    htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim1.Init.RepetitionCounter = 0U;
    htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_OK != HAL_TIM_PWM_Init(&htim1)) {
        Error_Handler();
    }

    /* PWM2 + CH1N active-high makes CCR=0 a LOW output on PB13. */
    oc.OCMode = TIM_OCMODE_PWM2;
    oc.Pulse = 0U;
    oc.OCPolarity = TIM_OCPOLARITY_HIGH;
    oc.OCNPolarity = TIM_OCNPOLARITY_HIGH;
    oc.OCFastMode = TIM_OCFAST_DISABLE;
    oc.OCIdleState = TIM_OCIDLESTATE_RESET;
    oc.OCNIdleState = TIM_OCNIDLESTATE_RESET;
    if (HAL_OK != HAL_TIM_PWM_ConfigChannel(&htim1, &oc, TIM_CHANNEL_1)) {
        Error_Handler();
    }
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 0U);
}

static uint32_t _mytimer_duty_to_ccr(float duty, uint32_t arr)
{
    if (duty < 0.0f) { duty = 0.0f; }
    if (duty > 1.0f) { duty = 1.0f; }
    return (uint32_t)(duty * (float)arr + 0.5f);
}
#else
void MYPWM_Set_Duty(uint8_t channel, float duty)
{
    (void)channel;
    (void)duty;
}
#endif

static void _mytimer_tim6_init(void)
{
    const uint32_t timclk = HAL_RCC_GetPCLK1Freq();
    uint32_t psc = timclk / 100000U;
    uint32_t cntclk;
    uint32_t arr;

    if (0U == psc) { psc = 1U; }
    psc -= 1U;
    cntclk = timclk / (psc + 1U);
    arr = cntclk / SEVEN_SEG_STEP_HZ;
    if (arr < 1U) { arr = 1U; }
    arr -= 1U;

    htim6.Instance = TIM6;
    htim6.Init.Prescaler = (uint16_t)psc;
    htim6.Init.Period = (uint16_t)arr;
    htim6.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim6.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_OK != HAL_TIM_Base_Init(&htim6)) {
        Error_Handler();
    }

    __HAL_TIM_CLEAR_FLAG(&htim6, TIM_FLAG_UPDATE);
    if (HAL_OK != HAL_TIM_Base_Start_IT(&htim6)) {
        Error_Handler();
    }
}
