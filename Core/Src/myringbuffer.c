#include "global.h"
#include "myringbuffer.h"

/////////////////////////////////////////////////////////////////////////////////////////////
// import function prototype


/////////////////////////////////////////////////////////////////////////////////////////////
// local function prototype


/////////////////////////////////////////////////////////////////////////////////////////////
// global variable

/////////////////////////////////////////////////////////////////////////////////////////////
// public and export function implement
void MYRING_Buffer_Init( ring_buffer_t* _pbRb, uint8_t* _pbBuff, uint16_t _nSize )
{
	_pbRb->pbBuffer = &_pbBuff[0];
	_pbRb->nBuffSize = _nSize;
	_pbRb->nhead = 0;
	_pbRb->ntail = 0;
}

int MYRING_Buffer_is_Empty( ring_buffer_t* _pbRb )
{
	return ( _pbRb->nhead == _pbRb->ntail );
}

int MYRING_Buffer_is_Full( ring_buffer_t* _pbRb )
{
	return ( ( ( _pbRb->nhead + 1 ) % _pbRb->nBuffSize ) == _pbRb->ntail );
}

uint16_t MYRING_Bufffer_Available( ring_buffer_t* _pbRb )
{
	if( _pbRb->nhead >= _pbRb->ntail ){
		return ( _pbRb->nhead - _pbRb->ntail );
	}
	else{
		return ( _pbRb->nBuffSize - ( _pbRb->ntail - _pbRb->nhead ) );
	}
}

int MYRING_Buffer_Write_Byte( ring_buffer_t* _pbRb, uint8_t _cnData )
{
	uint16_t nnext = ( _pbRb->nhead + 1 ) % _pbRb->nBuffSize;

	if( nnext == _pbRb->ntail ){
		return 0; 			// Full
	}

	_pbRb->pbBuffer[_pbRb->nhead] = _cnData;
	_pbRb->nhead = nnext;

	return 1;
}

int MYRING_Buffer_Write_Partial( ring_buffer_t* _pbRb, const uint8_t* _pbData, uint16_t _nLength )
{
	uint16_t ncount = 0;

	while( ncount < _nLength )
	{
		uint16_t nnext = ( _pbRb->nhead + 1 ) % _pbRb->nBuffSize;
	
		if( nnext == _pbRb->ntail ){
			break; 			// Full
		}
		_pbRb->pbBuffer[_pbRb->nhead] = _pbData[ncount++];
		_pbRb->nhead = nnext;
	}

	return ncount;
}

int MYRING_Buffer_Read_Byte( ring_buffer_t* _pbRb, uint8_t* _pbData )
{
	if( _pbRb->nhead == _pbRb->ntail ){
		return 0;  			// Enpty
	}
	*_pbData = _pbRb->pbBuffer[_pbRb->ntail];
	_pbRb->ntail = ( _pbRb->ntail + 1 ) % _pbRb->nBuffSize;

	return 1;
}

uint16_t MYRING_Buffer_Read_Block( ring_buffer_t* _pbRb, uint8_t* _pbData, uint16_t _nMaxLength )
{
	uint16_t ncount = 0;
	
	while( ( ncount < _nMaxLength ) && ( _pbRb->ntail != _pbRb->nhead ) )
	{
		_pbData[ncount++] = _pbRb->pbBuffer[_pbRb->ntail];
		_pbRb->ntail = ( _pbRb->ntail + 1 ) % _pbRb->nBuffSize;
	}
	return ncount;
}

/////////////////////////////////////////////////////////////////////////////////////////////
// local function implement