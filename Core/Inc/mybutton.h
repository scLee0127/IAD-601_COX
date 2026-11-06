#ifndef __MYBUTTON_H__
#define __MYBUTTON_H__

#define MYBTN_STATE_IDLE  							0
#define MYBTN_STATE_CLICK_UP  						1
#define MYBTN_STATE_CLICK_DOWN  					2
#define MYBTN_STATE_CLICK_SEL  						3
#define MYBTN_STATE_LONGKEY_UP  					4
#define MYBTN_STATE_LONGKEY_DOWN  					5
#define MYBTN_STATE_LONGKEY_SEL  					6

uint8_t MYBTN_Proc( void );

#endif // __MYBUTTON_H__