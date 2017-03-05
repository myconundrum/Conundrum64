#ifndef CIA1_H
#define CIA1_H

#include "emu.h"


typedef enum {

CIA_PRA 				=	0x00,  // data port a register
CIA_PRB 				=	0x01,  // data port b register
CIA_DDRA 				=	0x02,  // porta data direction register
CIA_DDRB 				=	0x03,  // portb data direction register
CIA_TALO				=	0x04,  // Timer A Low Byte 
CIA_TAHI				=	0x05,  // Timer A High Byte 
CIA_TBLO				=	0x06,  // Timer A Low Byte 
CIA_TBHI				=	0x07,  // Timer A High Byte 
CIA_TODTENTHS			=	0x08,  // BCD Time of Day Tenths of Second
CIA_TODSECS				=	0x09,  // BCD TOD Seconds 
CIA_TODMINS				=	0x0A,  // BCD TOD Minutes
CIA_TODHRS				=	0x0B,  // BCD TOD Hours
CIA_SDR 				=	0x0C,  // Serial Shift Register 
CIA_ICR					=	0x0D,  // Interupt control and status 
CIA_CRA					=	0x0E,  // Timer A control register 
CIA_CRB 				=	0x0F  // Timer B control register 

} CIA_REGISTERS;

#define CIA_FLAG_TAUIRQ					0b00000001 		// Timer a irq 
#define CIA_FLAG_TBUIRQ					0b00000010 		// Timer b irq 
#define CIA_FLAG_TODIRQ					0b00000100 		// TOD irq	
#define CIA_FLAG_SHRIRQ					0b00001000 		// shift register irq 
#define CIA_FLAG_FLGIRQ					0b00010000 		// flag irq 
#define CIA_FLAG_UN1IRQ					0b00100000 		// unused three
#define CIA_FLAG_UN2IRQ  				0b01000000 	 	// unused two 
#define CIA_FLAG_FILIRQ					0b10000000    	// Fill bit
#define CIA_FLAG_CIAIRQ					0b10000000    	// set when any cia irq is fired.


#define CIA_CR_TIMERSTART  				0b00000001   // 1 running, 0 stopped.
#define CIA_CR_PORTBSELECT				0b00000010   // 1 send to bit 6 of port b 
#define CIA_CR_PORTBMODE				0b00000100   // 1 toggle bit 6, 0 pule bit 6 1 cycle.
#define CIA_CR_TIMERRUNMODE				0b00001000 	 // 1 one shot timer, 0 continuous timer. 
#define CIA_CR_FORCELATCH				0b00010000 	 // force latch into current
#define CIA_CRA_TIMERINPUT				0b00100000 	 // 0 count microprocessor cycles, 1 count CNT press
#define CIA_CRA_SERIALMODE				0b01000000	 // serial mode (1 output, 0 input)
#define CIA_CRA_TODFREQUENCY			0b10000000   // 1 50hz TOD, 0 60hz TOD 
#define CIA_CRB_TIMERINPUT1				0b00100000 	 // 00 - clock cycles 01 CNT pin 
#define CIA_CRB_TIMERINPUT2				0b01000000	 // 10 - TA underflow 11 TA underflow + CNT pin
#define CIA_CRB_TODALARMORCLOCK			0b10000000   // 0 write to TOD registers is alarm, 1 is clock set


#define ROW_0   0x01
#define ROW_1   0x02
#define ROW_2   0x04
#define ROW_3   0x08
#define ROW_4   0x10
#define ROW_5   0x20
#define ROW_6   0x40
#define ROW_7   0x80


//
// BUGBUG dummy values for c64 keys
//
#define C64KEY_LSHIFT 	0xD0
#define C64KEY_CTRL  	0xD1
#define C64KEY_RUNSTOP	0xD2
#define C64KEY_CURDOWN	0xD3
#define C64KEY_CURLEFT	0xD4
#define C64KEY_CURRIGHT	0xD5
#define C64KEY_CURUP	0xD6
#define C64KEY_F1		0xD6
#define C64KEY_F3		0xD7
#define C64KEY_F5		0xD8
#define C64KEY_F7		0xD9
#define C64KEY_C64		0xDA
#define C64KEY_RSHIFT	0xDB
#define C64KEY_HOME		0xDC
#define C64KEY_BACK		0xDD
#define C64KEY_POUND	0xDD
#define C64KEY_DELETE   0xDE
#define C64KEY_RESTORE  0xDF





void cia_update();
void cia_keyup(byte ch);
void cia_keydown(byte ch);

void cia_init();
void cia_destroy(); 
byte cia_peek(byte register);
void cia_poke(byte register,byte val);


#endif



