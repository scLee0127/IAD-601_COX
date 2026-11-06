#include "global.h"
#include "myadc.h"

static float vref = 3.3f;

extern ADC_HandleTypeDef hadc;

void MYADC_Init( void )
{
	/* 1) ADC 클록 & HSI14 */
    __HAL_RCC_ADC1_CLK_ENABLE();
    __HAL_RCC_HSI14_ENABLE();
    while (__HAL_RCC_GET_FLAG(RCC_FLAG_HSI14RDY) == RESET) { /* wait */ }

    /* 2) GPIO(아날로그) */
    // MX_ADC0_GPIO_Init();

    /* 3) ADC 인스턴스 설정 */
    hadc.Instance                   = ADC1;
    hadc.Init.ClockPrescaler        = ADC_CLOCK_ASYNC_DIV1;         // HSI14 직접
    hadc.Init.Resolution            = ADC_RESOLUTION_12B;
    hadc.Init.DataAlign             = ADC_DATAALIGN_RIGHT;
    hadc.Init.ScanConvMode          = ADC_SCAN_DIRECTION_FORWARD;   // 단일 채널
    hadc.Init.EOCSelection          = ADC_EOC_SINGLE_CONV;          // 매 변환마다 EOC
    hadc.Init.LowPowerAutoWait      = DISABLE;
    hadc.Init.LowPowerAutoPowerOff  = DISABLE;
    hadc.Init.ContinuousConvMode    = ENABLE;                       // ★ Continuous
    hadc.Init.DiscontinuousConvMode = DISABLE;
    hadc.Init.ExternalTrigConv      = ADC_SOFTWARE_START;
    hadc.Init.ExternalTrigConvEdge  = ADC_EXTERNALTRIGCONVEDGE_NONE;
    hadc.Init.DMAContinuousRequests = DISABLE;                      // 플래그 폴링 방식
    hadc.Init.Overrun               = ADC_OVR_DATA_OVERWRITTEN;     // ★ DR 미읽어도 덮어쓰기

    HAL_ADC_Init(&hadc);

    /* 4) 보정(Calibration) */
    HAL_ADCEx_Calibration_Start(&hadc);

    /* 5) 채널 설정: ADC_IN6 (PA6) */
    ADC_ChannelConfTypeDef sConfig = {0};
    sConfig.Channel      = ADC_CHANNEL_6;                 // PA6
    sConfig.Rank         = ADC_RANK_CHANNEL_NUMBER;
    /* 고임피던스 센서면 긴 샘플링 권장 (239.5 cycles) */
    sConfig.SamplingTime = ADC_SAMPLETIME_239CYCLES_5;
    HAL_ADC_ConfigChannel(&hadc, &sConfig);

    HAL_ADC_Start(&hadc);
}

uint8_t ADC0_TryGetRaw(uint16_t *raw)
{
    // EOC 플래그만 확인 (Blocking 없음)
    if (__HAL_ADC_GET_FLAG(&hadc, ADC_FLAG_EOC)) {
        // 읽으면 EOC 클리어됨
        *raw = (uint16_t)HAL_ADC_GetValue(&hadc);
        // 오버런 클리어(안전)
        __HAL_ADC_CLEAR_FLAG(&hadc, ADC_FLAG_OVR);
        return 1;
    }
    return 0; // 아직 변환 중
}
float gnfAdcVoltage = 0;
float gnfCurrent_Val = 0.0f;
void MYADC_Proc( void )
{
	uint16_t nadc_val = 0;
	// float nfVolt = 0.0f;
	if( 1 == ADC0_TryGetRaw(&nadc_val ) ){
		// gnAdcVoltage = nadc_val;
		gnfAdcVoltage = ( (float)nadc_val * vref) / (float)4095.0f;
		gnfCurrent_Val = ( gnfAdcVoltage - 1.65f )/ 0.4f;
	}

}

uint16_t ADC0_ReadRaw(void)
{
    HAL_ADC_Start(&hadc);
    HAL_ADC_PollForConversion(&hadc, 2);  // 10ms 타임아웃
    uint16_t val = (uint16_t)HAL_ADC_GetValue(&hadc);
    HAL_ADC_Stop(&hadc);
    return val; // 0..4095
}

/* 전압 변환 (Vref=3.3V 가정) */
float ADC0_ReadVoltage(void)
{
    // uint16_t raw = ADC0_ReadRaw();
    // printf("%d\r\n", raw);
    return gnfAdcVoltage; //((float)raw * vref) / (float)4095.0f;
}

float ADC0_Get_Current( void )
{
	// float nfVoltage = ADC0_ReadVoltage();
	return gnfCurrent_Val;//( nfVoltage - 1.65f )/ 0.4f;
}