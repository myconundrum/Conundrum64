/*
Conundrum 64: Commodore 64 Emulator

MIT License

Copyright (c) 2017 

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

-------------------------------------------------------------------------------
MODULE: cia.h

WORK ITEMS:

KNOWN BUGS:

*/

#ifndef CIA1_H
#define CIA1_H

#include "emu.h"
#include "cpu.h"


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

#define CIA_FLAG_TAUIRQ		0x01		//	0b00000001 		// Timer a irq 
#define CIA_FLAG_TBUIRQ		0x02		//	0b00000010 		// Timer b irq 
#define CIA_FLAG_TODIRQ		0x04		//	0b00000100 		// TOD irq	
#define CIA_FLAG_SHRIRQ		0x08		//	0b00001000 		// shift register irq 
#define CIA_FLAG_FLGIRQ		0x10		//	0b00010000 		// flag irq 
#define CIA_FLAG_UN1IRQ		0x20		//	0b00100000 		// unused three
#define CIA_FLAG_UN2IRQ  	0x40		//	0b01000000 	 	// unused two 
#define CIA_FLAG_FILIRQ		0x80		//	0b10000000    	// Fill bit
#define CIA_FLAG_CIAIRQ		0x80		//	0b10000000    	// set when any cia irq is fired.


#define CIA_CR_TIMERSTART  		0x01	//	0b00000001   // 1 running, 0 stopped.
#define CIA_CR_PORTBSELECT		0x02	//	0b00000010   // 1 send to bit 6 of port b 
#define CIA_CR_PORTBMODE		0x04	//	0b00000100   // 1 toggle bit 6, 0 pule bit 6 1 cycle.
#define CIA_CR_TIMERRUNMODE		0x08	//	0b00001000 	 // 1 one shot timer, 0 continuous timer. 
#define CIA_CR_FORCELATCH		0x10	//	0b00010000 	 // force latch into current
#define CIA_CRA_TIMERINPUT		0x20	//	0b00100000 	 // 0 count microprocessor cycles, 1 count CNT press
#define CIA_CRA_SERIALMODE		0x40	//	0b01000000	 // serial mode (1 output, 0 input)
#define CIA_CRA_TODFREQUENCY	0x80	//	0b10000000   // 1 50hz TOD, 0 60hz TOD 
#define CIA_CRB_TIMERINPUT1		0x20	//	0b00100000 	 // 00 - clock cycles 01 CNT pin 
#define CIA_CRB_TIMERINPUT2		0x40	//	0b01000000	 // 10 - TA underflow 11 TA underflow + CNT pin
#define CIA_CRB_TODALARMORCLOCK	0x80	//	0b10000000   // 0 write to TOD registers is alarm, 1 is clock set


//
// these methods work on both cia chips on the c64.
//
void cia_update(void);
void cia_init(void);
void cia_destroy(void); 

//
// you have to pick which cia you are talking to on a peek or poke only used by mem.
//
byte cia1_peek(word register);
void cia1_poke(word register, byte val);

byte cia2_peek(word register);
void cia2_poke(word register, byte val);

#endif



