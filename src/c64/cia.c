/*
Conundrum 64: Commodore 64 Emulator

MIT License

Copyright (c) 2017 Marc R. Whitten

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
MODULE: cia.c
	Commodore 64 CIA chip emulation source


WORK ITEMS:

KNOWN BUGS:

*/
#include <time.h>
#include <string.h>
#include "emu.h"
#include "cia.h"
#include "sysclock.h"

#define CIA_ALARM 0x02 // used for TOD registers only.
#define CIA_LATCH 0x01
#define CIA_REAL  0x00


typedef struct _CIA CIA;
typedef byte (*PORTBDATAHANDLER)(CIA * c);
typedef void (*SIGNALIRQHANDLER)();

struct _CIA {

	byte regs[3][0x10];			// CIA1 internal registers. use CIA1_REGS enum to address.
	byte isr;  					// read pulls current irq status

	unsigned long lticks;		// stores the value of sysclock_getticks() on last cia1_update()
	bool todlatched;			// if true, registers will not update on reads until
								// the tenths register is read. 
	unsigned long todstart;

	PORTBDATAHANDLER bfn;		// port b data handler.
	SIGNALIRQHANDLER irqfn;		// signal irqs here.

};

typedef word (*CLOCKHANDLER)(CIA *,byte mode);

CIA g_cia1;
CIA g_cia2;

byte cia_peek(CIA *c,byte reg);
void cia_poke(CIA *c,byte reg,byte val);

byte cia1_peek(byte reg) {
	return cia_peek(&g_cia1,reg);
}
void cia1_poke(byte reg,byte val) {
	cia_poke(&g_cia1,reg,val);
}

byte cia2_peek(byte reg) {
	return cia_peek(&g_cia2,reg);
}
void cia2_poke(byte reg,byte val) {
	cia_poke(&g_cia2,reg,val);
	if (reg == 0x00) {
		//
		// VIC looks here to understand graphics bank switches.
		//
		vicii_setbank();
	}
}

byte cia_getreal(CIA * c,byte reg) {
	return c->regs[CIA_REAL][reg];
}

void cia_setreal(CIA * c,byte reg,byte val) {
	c->regs[CIA_REAL][reg] = val;
}

byte cia_getlatched(CIA * c,byte reg) {
	return c->regs[CIA_LATCH][reg];
}

void cia_setlatched(CIA * c,byte reg,byte val) {
	c->regs[CIA_LATCH][reg] = val;
}

void cia_latchtoreal(CIA * c,byte reg) {
	c->regs[CIA_REAL][reg] = c->regs[CIA_LATCH][reg];
}

void cia_realtolatch(CIA * c,byte reg) {
	c->regs[CIA_LATCH][reg] = c->regs[CIA_REAL][reg];
}

byte cia_getkbdprb(CIA * c) {

	int i = 0;
	byte val = 0xff;

	for (i = 0; i < 8; i++) {

		if (~cia_getreal(c,CIA_PRA) & (0x01 << i)) {
			val &= c64kbd_getrow(i);
		}
	}
	return val;

}

byte cia_getserialprb(CIA * c) {

	// not implemented
	return 0xff;

}


void cia_latchtod(CIA * c) {

	c->todlatched 		= true;
	cia_realtolatch(c,CIA_TODHRS);
	cia_realtolatch(c,CIA_TODMINS);
	cia_realtolatch(c,CIA_TODSECS);
	cia_realtolatch(c,CIA_TODTENTHS);
}

byte cia_readtod(CIA * c,byte reg) {
   return c->todlatched ? cia_getlatched(c,reg) : cia_getreal(c,reg);
}

byte cia_peek(CIA * c,byte address) {

	byte val;

	switch(address %0x10) {
		case CIA_ICR: 
			val = c->isr;
		break;
		case CIA_PRB:
			val = c->bfn(c);
		break;
		case CIA_TODHRS: 
			cia_latchtod(c);
			val = cia_readtod(c,CIA_TODHRS);
		break;
		case CIA_TODMINS: 
			val = cia_readtod(c,CIA_TODMINS);
		break;
		case CIA_TODSECS: 
			val = cia_readtod(c,CIA_TODSECS);
		break;		
		case CIA_TODTENTHS: 
			val = cia_readtod(c,CIA_TODTENTHS);
			c->todlatched = false;
		break;
		default: 
			val = cia_getreal(c,address % 0x10);
		break;
	}
	return val;
}

void cia_seticr(CIA * c,byte val) {

	byte old = cia_getreal(c,CIA_ICR);
	byte fillbit = val & CIA_FLAG_FILIRQ ? 0xFF : 0;
	byte new = 0;

	/*
		Enabling or Disabling IRQSs. Bits 0-6 specify which IRQs you want to enable/disable
		(a 1 in a bit position means you are attempting to change that IRQ)

		Bit 7 is the bit that is used for these. In other words -- if a bit is set in 
		positions 0-6, then use the value of bit 7 for that bit, otherwise, keep existing
		value.

		At least that's how I read the documentation :-)
	*/

	new |= (val & 0x01) ? (fillbit & 0x01) : (old & 0x01);  
	new |= (val & 0x02) ? (fillbit & 0x02) : (old & 0x02);
	new |= (val & 0x04) ? (fillbit & 0x04) : (old & 0x04);  
	new |= (val & 0x08) ? (fillbit & 0x08) : (old & 0x08);
	new |= (val & 0x10) ? (fillbit & 0x10) : (old & 0x10);  
	new |= (val & 0x20) ? (fillbit & 0x20) : (old & 0x20);
	new |= (val & 0x40) ? (fillbit & 0x40) : (old & 0x40);  
	
	cia_setreal(c,CIA_ICR,new);
}

void cia_setport(CIA * c,CIA_REGISTERS reg, CIA_REGISTERS ddr, byte val) {

	byte test = 0b10000000;
	byte new = 0;
	byte portval = cia_getreal(c,reg);
	byte ddrval  = cia_getreal(c,ddr);

	while (test) {
		new |= (test & ddrval) ? (test & val) : (test & portval);
		test >>= 1;
	}
	cia_setreal(c,reg,new);
}

void cia_settod_reg(CIA * c,byte reg,byte val) {


	byte pm = val & BIT_7;
	val &= (~BIT_7);

	byte where = cia_getreal(c,CIA_CRB) & CIA_CRB_TODALARMORCLOCK ? CIA_REAL : CIA_ALARM;
	byte hi = (val / 10) << 4;
	byte low = (val % 10);

	c->regs[where][reg] = pm | hi | low;
}

void cia_poke(CIA * c,byte address,byte val) {

	byte reg = address % 0x10;

	switch (reg) {

		case CIA_PRA: 			// data port a register
			cia_setport(c,CIA_PRA,CIA_DDRA,val);
		break;
		case CIA_PRB: 			// data port b register
			cia_setport(c,CIA_PRB,CIA_DDRB,val);
		break;
		//
		// timers sets. 
		//
		case CIA_TALO: case CIA_TAHI: case CIA_TBLO: case CIA_TBHI:		 
			cia_setreal(c,reg,val);
			cia_setlatched(c,reg,val);			
		break;
		//
		// tod sets.
		//
		case CIA_TODTENTHS: case CIA_TODSECS: case CIA_TODMINS: case CIA_TODHRS:
			cia_settod_reg(c,reg,val);
		break;
		case CIA_ICR:			// Interupt control and status 
			cia_seticr(c,val);
		break;
		case CIA_CRA:			// Timer A control register 
			cia_setreal(c,reg,val);
			// force load startlatch into current 
			if (val & CIA_CR_FORCELATCH) { 
				cia_latchtoreal(c,CIA_TALO);
				cia_latchtoreal(c,CIA_TAHI);
			}		
		break;
		case CIA_CRB:			// Timer B control register 
			cia_setreal(c,reg,val);
			// force load startlatch into current 
			if (val & CIA_CR_FORCELATCH) { 
				cia_latchtoreal(c,CIA_TBLO);
				cia_latchtoreal(c,CIA_TBHI);
			}	
		break;
		default: cia_setreal(c,reg,val);
		break;
	}	
}

void cia_init() {

	memset(&g_cia1,0,sizeof(CIA));
	memset(&g_cia2,0,sizeof(CIA));
	//
	// set default direction for ports. 
	//
	cia_setreal(&g_cia1,CIA_DDRA,0xff);
	cia_setreal(&g_cia1,CIA_DDRB,0x0);
	cia_setreal(&g_cia1,CIA_PRB,0xff);


	//
	// set default direction for ports. 
	//
	cia_setreal(&g_cia2,CIA_DDRA,0x3F);
	cia_setreal(&g_cia2,CIA_DDRB,0x0);
	
	g_cia1.lticks = 0;
	g_cia1.todstart = sysclock_getticks();

	g_cia2.lticks = 0;
	g_cia2.todstart = sysclock_getticks();

	//
	// connect the cia chips to other components
	//
	g_cia1.bfn = cia_getkbdprb;
	g_cia2.bfn = cia_getserialprb;

	g_cia1.irqfn = cpu_irq;
	g_cia2.irqfn = cpu_nmi;
}

void cia_destroy() {
	
}

word cia_timera_clicks(CIA * c,byte mode) {

	if (mode & CIA_CRA_TIMERINPUT) {
		// BUGBUG: Not implemented. Should count down on  CNT presses here. 
	} else {
		return sysclock_getticks() - c->lticks;
	}
	return 0;
}

word cia_timerb_clicks(CIA * c,byte mode) {

	switch(mode & (CIA_CRB_TIMERINPUT1 | CIA_CRB_TIMERINPUT2)) {

		case 0b00000000: // sysclock ticks
			return sysclock_getticks() - c->lticks;
		break;
		case 0b00100000: // CNT pin
			// BUGBUG: Not implemented
		break;
		case 0b01000000: // TAU undeflow
			return (c->isr & CIA_FLAG_TAUIRQ) ? 1 : 0;
		break;
		case 0b01100000: // TAU underflow + CNT pin
			// BUGBUG: Not implemented
		break;
		default: break;
	}
	return 0;
}

void cia_update_timer(CIA * c,CLOCKHANDLER tickfn,byte controlreg,byte hi,byte low,byte underflowflag) {

	word ticks;
	word tval = 0;
	word updateval= 0;
	byte cr = cia_getreal(c,controlreg);

	//
	// is timer a enabled?
	//
	if ((cr & CIA_CR_TIMERSTART) == 0) {
		//
		// timer not running.
		//
		return;
	}
	
	ticks = tickfn(c,cr);

	tval = ((word) cia_getreal(c,hi) << 8 ) | cia_getreal(c,low);
	updateval = tval - (ticks);
	cia_setreal(c,hi,updateval >> 8);
	cia_setreal(c,low,updateval & 0xFF); 
	
	// 
	// check for underflow condition.
	//
	if (updateval > tval) { 
		//
		// set bit in ICS regiser. 
		//
		c->isr |= underflowflag; 
		//
		// check to see if (and how) to signal underflow on port b bit six. 
		//
		if (cr & CIA_CR_PORTBSELECT) {

			if (cr & CIA_CR_PORTBMODE) {
				//
				// BUGBUG Not Implemented
				//
			}
			else {
				//
				// BUGBUG Not Implemented
				//
			}
		}
		//
		// if runmode is one shot, turn timer off.  
		// 
		if (cr & CIA_CR_TIMERRUNMODE) {
			cia_setreal(c,controlreg,cr & (~CIA_CR_TIMERSTART));
		}

		//
		// reset to latch value. 
		//
		cia_latchtoreal(c,hi);
		cia_latchtoreal(c,low);
	}
}

void cia_update_todreg(CIA * c,byte reg,double timeincrement,byte modval) {

	unsigned long val;
	byte high;
	byte low;



	val = (sysclock_getticks() - c->todstart) / (NTSC_TICKS_PER_SECOND * timeincrement); // total number of those increments
	val %= modval; 	   // what flips back to zero.
	high = (val / 10) << 4;
	low =  val % 10; 
	//
	// BUGBUG am/pm!
	//
	cia_setreal(c,reg,high|low);
	
}

void cia_update_timeofday(CIA * c) {


	byte pm = c->regs[CIA_REAL][CIA_TODHRS] & BIT_7;
	
	unsigned long nclock = sysclock_getticks();

	cia_update_todreg(c,CIA_TODTENTHS,.1,10);
	cia_update_todreg(c,CIA_TODSECS,1,60);
	cia_update_todreg(c,CIA_TODMINS,60,60);
	cia_update_todreg(c,CIA_TODHRS,3600,12);

	if (c->regs[CIA_REAL][CIA_TODHRS] == 0) {
		pm = pm ? 0 : BIT_7;	
	}
	c->regs[CIA_REAL][CIA_TODHRS] |= pm;

	if (c->regs[CIA_REAL][CIA_TODHRS] == c->regs[CIA_ALARM][CIA_TODHRS] && 
		c->regs[CIA_REAL][CIA_TODMINS] == c->regs[CIA_ALARM][CIA_TODMINS] &&
		c->regs[CIA_REAL][CIA_TODSECS] == c->regs[CIA_ALARM][CIA_TODSECS] &&
		c->regs[CIA_REAL][CIA_TODTENTHS] >= c->regs[CIA_ALARM][CIA_TODTENTHS]) {
		//
		// hit alarm
		//
		c->isr |= CIA_FLAG_TODIRQ; 
	}
}


void cia_check_interrupts(CIA * c) {

	byte icr = cia_getreal(c,CIA_ICR);

	if (c->isr & icr & (CIA_FLAG_TODIRQ | CIA_FLAG_TAUIRQ | CIA_FLAG_TBUIRQ)) {
		c->irqfn();
	}
}


void cia_update_internal(CIA *c) {

	//
	// order here is important. Timerb can count timera underflows so needs to come 
	// second.
	//
	c->isr = 0;
	cia_update_timer(c,cia_timera_clicks,CIA_CRA,CIA_TAHI,CIA_TALO,CIA_FLAG_TAUIRQ);
	cia_update_timer(c,cia_timerb_clicks,CIA_CRB,CIA_TBHI,CIA_TBLO,CIA_FLAG_TBUIRQ);
	cia_update_timeofday(c);
	cia_check_interrupts(c);

	c->lticks = sysclock_getticks ();
}

void cia_update() {

	cia_update_internal(&g_cia1);
	cia_update_internal(&g_cia2);
}
