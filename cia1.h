#ifndef CIA1_H
#define CIA1_H

#include "emu.h"


#define CIA1_PAGE 		0xDC
#define CIA1_PORTA 		0x00
#define CIA1_PORTB		0x01
#define CIA1_PORTA_DDR	0x02
#define CIA1_PORTB_DDR	0x03


#define CIA1_PORTA_ADD			0xDC00
#define CIA1_PORTB_ADD			0xDC01
#define CIA1_PORTA_DDR_ADD		0xDC02
#define CIA1_PORTB_DDR_ADD		0xDC03

void cia1_update();
void cia1_keypress(byte ch);
void cia1_init();


#endif



