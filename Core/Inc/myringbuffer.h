#ifndef __MYRINGBUFFER_H__
#define __MYRINGBUFFER_H__

#include <stdint.h>

typedef struct{
	uint8_t* pbBuffer;
	uint16_t nBuffSize;
	volatile uint16_t nhead;
	volatile uint16_t ntail;
}ring_buffer_t;

void MYRING_Buffer_Init( ring_buffer_t* _pbRb, uint8_t* _pbBuff, uint16_t _nSize );
int MYRING_Buffer_Write_Byte( ring_buffer_t* _pbRb, uint8_t _cnData );
int MYRING_Buffer_Write_Partial( ring_buffer_t* _pbRb, const uint8_t* _pbData, uint16_t _nLength );
int MYRING_Buffer_Read_Byte( ring_buffer_t* _pbRb, uint8_t* _pbData );
uint16_t MYRING_Buffer_Read_Block( ring_buffer_t* _pbRb, uint8_t* _pbData, uint16_t _nMaxLength );
int MYRING_Buffer_is_Empty( ring_buffer_t* _pbRb );

#endif // __MYRINGBUFFER_H__