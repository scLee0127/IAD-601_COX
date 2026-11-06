#include "main.h"
#include "global.h"
#include "mydpsensor.h"
#include "myi2c.h"

/////////////////////////////////////////////////////////////////////////////////////////////
// import function prototype


/////////////////////////////////////////////////////////////////////////////////////////////
// local function prototype


/////////////////////////////////////////////////////////////////////////////////////////////
// global variable

/////////////////////////////////////////////////////////////////////////////////////////////
// public and export function implement

/////////////////////////////////////////////////////////////////////////////////////////////
// local function implement

static float gnfPrevAvg = 0;
// static uint16_t nSamplingdatalen = 0;

#define SM7291_ADDR 0xD8
#define PRESSURE_SAMPLING_COUNT 		4  		// 샘플링 카운트(차압값 안정화)
#define PRESSURE_SAMPLING_START  		1   	// 4개 중 가운데 2개(인덱스 1,2)를 평균내려면 1로 변경
#define PRESSURE_SAMPLING_END  			3
#define PRESSURE_SAMPLING_DATA_COUNT  	(float)( PRESSURE_SAMPLING_END - PRESSURE_SAMPLING_START )

static uint8_t gcnPressureDataIdx = 0;
static float gabPressureData[PRESSURE_SAMPLING_COUNT] = {0.0f,};
void _sort_f( float* _abinout, uint16_t _ncount );

// uint8_t CheckCrc(const uint8_t* _pbIn, uint8_t _cnSize, uint8_t _cnCheckSum)
// {
//   uint8_t crc = 0xFF;
  
//   // calculates 8-Bit _cnCheckSum with given polynomial 0x31 (x^8 + x^5 + x^4 + 1)
//   for(int i = 0; i < _cnSize; i++) {
//     crc ^= (_pbIn[i]);
//     for(uint8_t bit = 8; bit  > 0; --bit) {
//       if(crc & 0x80) crc = (crc << 1) ^ 0x31;
//       else           crc = (crc << 1);
//     }
//   }
  
//   // verify _cnCheckSum
//   return (crc == _cnCheckSum) ? 0 : 1;
// }

/*======================================================================
 * SM7291 Get Data
 *====================================================================*/
uint8_t SM7291_READ_P_VALUE( float* _nfpressure )
{
	uint8_t cner_check = 0;
	uint8_t abdp_buf[4] = {0,};
	int16_t npress = 0;
	uint8_t cnret = 0;

	//MYUTIL_Printf("SM7291 1-1-1 \n\r");
	I2C_Start_Condition();
	
	do{
		//slave address 0x6C + LSB = 0 for write
		cner_check = I2C_Send_Byte(SM7291_ADDR);
		if(cner_check) {
			// MYUTIL_Printf("I2C sending Error from 0xD8 \r\n");
			cnret = 1;
			break;
		}

		cner_check = I2C_Send_Byte(0x30);
		if(cner_check) {
			// MYUTIL_Printf("I2C sending Error from 0x2E \r\n");
			cnret = 2;
			break;
		}

		I2C_Restart_Condition();
		I2C_Delay(10);	

		// slave address 0x6C + LSB = 1 for read 
		cner_check = I2C_Send_Byte(SM7291_ADDR + 1);
		if(cner_check) {
			// MYUTIL_Printf("I2C sending Error from 0xD9 \r\n");
			cnret = 3;
			break;
		}

		I2C_Delay(10);
		abdp_buf[0] = I2C_Read_Byte();
		I2C_Delay(10);
		abdp_buf[1] = I2C_Read_Byte_END();
		I2C_Delay(10);
		// abdp_buf[2] = I2C_Read_Byte();
		// abdp_buf[3] = I2C_Read_Byte_END();

		npress =  ( ( (uint16_t)(abdp_buf[1]) << 8 ) + abdp_buf[0] );	
		*_nfpressure = ( ( (float)npress + 26214.0f ) / 52428.0f ) * 1000.0f;
		// MYUTIL_Printf("npress %02x %02x\r\n", abdp_buf[0], abdp_buf[1]);
	}while(0);

	I2C_Scl_Set_High();
	I2C_Stop_Condition();
	
	return cnret;
}

/*======================================================================

=======================================================================*/
uint32_t gnTick_prev = 0;
uint8_t SM7291_Sampling_VAL( float* _nfDpVal )
{
	uint8_t i = 0;
	uint8_t cnret = 0;
	uint8_t cnres = 0;
	float nfdp_tmp = 0.0f;
	float nftotal = 0.0f;
	
	cnres = SM7291_READ_P_VALUE( &nfdp_tmp );
	if( 0 == cnres ){
		gabPressureData[gcnPressureDataIdx] = nfdp_tmp;
		gcnPressureDataIdx++;
	}
	else{
		// Error....
	}

	if( (PRESSURE_SAMPLING_COUNT - 1) < gcnPressureDataIdx ){
		gcnPressureDataIdx = 0;
		_sort_f( gabPressureData, PRESSURE_SAMPLING_COUNT );

		for( i = PRESSURE_SAMPLING_START; i < PRESSURE_SAMPLING_END; i++ )
		{
			nftotal += gabPressureData[i];	
		}

		*_nfDpVal = nftotal / PRESSURE_SAMPLING_DATA_COUNT;
		cnret = 1;
	}
	return cnret;
}

void SM7291_Sensor_Proc(void)
{
  	float nfdp_val_tmp = 0.0f;
	const float nfdt = 0.002f;  		// 4ms
	const float nffc = 2.0f;  		// 4hz LPF  -> 2hz 로 변경하면 느려짐
	const float nfa = ( 2.0f * 3.1415926f * nffc * nfdt ) / ( 1.0f + 2.0f * 3.1415926f * nffc * nfdt );

  	if( 0 != gnDpSensorTimer ){ return; }
  	gnDpSensorTimer = 2;
  	if( 1 == SM7291_Sampling_VAL( &nfdp_val_tmp ) ){
  		// y[k] = y[k-1] + a * ( x[k] - y[k-1] )
  		gnfPrevAvg = gnfPrevAvg + nfa * ( nfdp_val_tmp - gnfPrevAvg );
		gnfLpfPaValue = ( gnfPrevAvg < 0.0f ) ? -gnfPrevAvg : gnfPrevAvg;

  		if( 3.0f > gnfLpfPaValue){ gnfLpfPaValue = 0.0f; }

  		if( 0 == gnGuiSensorTimer ){
			gnGuiSensorTimer = 1000;
			gnDispSensorValue = (int32_t)gnfLpfPaValue;  		// 추후 디스플레이 변화량을 500ms 마다 출력하게 변경
		}
  	}
}

void _sort_f( float* _abinout, uint16_t _ncount )
{
	int i, j;
	float nftemp = 0.0f;

	for( i = 0; i < ( _ncount - 1 ); i++ ) 
	{ 
		for( j = ( i + 1 ); j < ( _ncount - 1 ); j++ ) 
		{ 
			if( _abinout[j] > _abinout[j+1] ){ 
				nftemp = _abinout[j];
				_abinout[j] = _abinout[j+1]; 
				_abinout[j+1] = nftemp; 
			}
		}
	} 
}

#if 0
void SM7291_Sensor_Proc(void)
{
  	float nfdp_val_tmp = 0.0f;
  	float nfavg = 0.0f;
  	int32_t npa_tmp = 0;
  	int32_t nTick_Curr = 0;
	const float nfdt = 0.004f;  		// 4ms
	const float nffc = 4.0f;  		// 4hz LPF  -> 2hz 로 변경하면 느려짐
	const float nfa = ( 2.0f * 3.1415926f * nffc * nfdt ) / ( 1.0f + 2.0f * 3.1415926f * nffc * nfdt );

  	if( 0 != gnDpSensorTimer ){ return; }
  	gnDpSensorTimer = 4;
  	if( 1 == SM7291_Sampling_VAL( &nfdp_val_tmp ) ){
  		
  		if( 4 > nSamplingdatalen ){ nSamplingdatalen++; }
  		nfavg = ( gnfPrevAvg * ( (nSamplingdatalen-1) / (float)nSamplingdatalen ) ) + ( nfdp_val_tmp * ( 1 / (float)nSamplingdatalen ) );	

  		// y[k] = y[k-1] + a * ( x[k] - y[k-1] )
  		gnfPrevAvg = gnfPrevAvg + nfa * ( nfavg - gnfPrevAvg );

		npa_tmp = (int32_t)( gnfPrevAvg );
		gnfLpfPaValue = ( gnfPrevAvg < 0.0f ) ? -gnfPrevAvg : gnfPrevAvg;
  		// gnCorrectValue = (npa_tmp < 0) ? -npa_tmp : npa_tmp;
  		if( 3.0f > gnfLpfPaValue){ gnfLpfPaValue = 0.0f; }

  		if( 0 == gnGuiSensorTimer ){
			gnGuiSensorTimer = 500;
		}
		gnDispSensorValue = (int32_t)gnfLpfPaValue;  		// 추후 디스플레이 변화량을 500ms 마다 출력하게 변경
  	}
}
#endif