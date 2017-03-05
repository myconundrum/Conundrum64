
#include <time.h>
#include <string.h>
#include "emu.h"


typedef word (*CLOCKHANDLER)(byte mode);

#define CIA_ALARM 0x02 // used for TOD registers only.
#define CIA_LATCH 0x01
#define CIA_REAL  0x00


typedef struct {

	byte regs[3][0x10];			// CIA1 internal registers. use CIA1_REGS enum to address.
	byte isr;  					// read pulls current irq status

	unsigned long lticks;		// stores the value of sysclock_getticks() on last cia1_update()
	bool todlatched;			// if true, registers will not update on reads until
								// the tenths register is read. 

	unsigned long todstart;
} CIA;

CIA g_cia1;

byte cia_getreal(byte reg) {
	return g_cia1.regs[CIA_REAL][reg];
}

void cia_setreal(byte reg,byte val) {
	g_cia1.regs[CIA_REAL][reg] = val;
}

byte cia_getlatched(byte reg) {
	return g_cia1.regs[CIA_LATCH][reg];
}

void cia_setlatched(byte reg,byte val) {
	g_cia1.regs[CIA_LATCH][reg] = val;
}

void cia_latchtoreal(byte reg) {
	g_cia1.regs[CIA_REAL][reg] = g_cia1.regs[CIA_LATCH][reg];
}

void cia_realtolatch(byte reg) {
	g_cia1.regs[CIA_LATCH][reg] = g_cia1.regs[CIA_REAL][reg];
}

byte cia_getkbdprb() {

	int i = 0;
	byte val = 0xff;

	for (i = 0; i < 8; i++) {

		if (~cia_getreal(CIA_PRA) & (0x01 << i)) {
			val &= c64kbd_getrow(i);
		}
	}
	return val;

}


void cia_latchtod() {

	g_cia1.todlatched 		= true;
	cia_realtolatch(CIA_TODHRS);
	cia_realtolatch(CIA_TODMINS);
	cia_realtolatch(CIA_TODSECS);
	cia_realtolatch(CIA_TODTENTHS);
}

byte cia_readtod(byte reg) {
   return g_cia1.todlatched ? cia_getlatched(reg) : cia_getreal(reg);
}



byte cia_peek(byte address) {

	byte val;

	switch(address %0x10) {
		case CIA_ICR: 
			val = g_cia1.isr;
		break;
		case CIA_PRB:
			val = cia_getkbdprb();
		break;
		case CIA_TODHRS: 
			cia_latchtod();
			val = cia_readtod(CIA_TODHRS);
		break;
		case CIA_TODMINS: 
			val = cia_readtod(CIA_TODMINS);
		break;
		case CIA_TODSECS: 
			val = cia_readtod(CIA_TODSECS);
		break;		
		case CIA_TODTENTHS: 
			val = cia_readtod(CIA_TODTENTHS);
			g_cia1.todlatched = false;
		break;
		default: 
			val = cia_getreal(address % 0x10);
		break;
	}
	return val;
}

void cia_seticr(byte val) {

	byte old = cia_getreal(CIA_ICR);
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
	
	cia_setreal(CIA_ICR,new);
}

void cia_setport(CIA_REGISTERS reg, CIA_REGISTERS ddr, byte val) {

	byte test = 0b10000000;
	byte new = 0;
	byte portval = cia_getreal(reg);
	byte ddrval  = cia_getreal(ddr);

	while (test) {
		new |= (test & ddrval) ? (test & val) : (test & portval);
		test >>= 1;
	}
	cia_setreal(reg,new);
}

void cia_settod_reg(reg,val) {


	byte pm = val & BIT_7;
	val &= (~BIT_7);

	byte where = cia_getreal(CIA_CRB) & CIA_CRB_TODALARMORCLOCK ? CIA_REAL : CIA_ALARM;
	byte hi = (val / 10) << 4;
	byte low = (val % 10);

	g_cia1.regs[where][reg] = pm | hi | low;
}

void cia_poke(byte address,byte val) {

	byte reg = address % 0x10;

	switch (reg) {

		case CIA_PRA: 			// data port a register
			cia_setport(CIA_PRA,CIA_DDRA,val);
		break;
		case CIA_PRB: 			// data port b register
			cia_setport(CIA_PRB,CIA_DDRB,val);
		break;
		//
		// timers sets. 
		//
		case CIA_TALO: case CIA_TAHI: case CIA_TBLO: case CIA_TBHI:		 
			cia_setreal(reg,val);
			cia_setlatched(reg,val);			
		break;
		//
		// tod sets.
		//
		case CIA_TODTENTHS: case CIA_TODSECS: case CIA_TODMINS: case CIA_TODHRS:
			cia_settod_reg(reg,val);
		break;
		case CIA_ICR:			// Interupt control and status 
			cia_seticr(val);
		break;
		case CIA_CRA:			// Timer A control register 
			cia_setreal(reg,val);
			// force load startlatch into current 
			if (val & CIA_CR_FORCELATCH) { 
				cia_latchtoreal(CIA_TALO);
				cia_latchtoreal(CIA_TAHI);
			}		
		break;
		case CIA_CRB:			// Timer B control register 
			cia_setreal(reg,val);
			// force load startlatch into current 
			if (val & CIA_CR_FORCELATCH) { 
				cia_latchtoreal(CIA_TBLO);
				cia_latchtoreal(CIA_TBHI);
			}	
		break;
		default: cia_setreal(reg,val);
		break;
	}	
}

void cia_init() {

	memset(&g_cia1,0,sizeof(CIA));
	//
	// set default direction for ports. 
	//
	cia_setreal(CIA_DDRA,0xff);
	cia_setreal(CIA_DDRB,0x0);
	cia_setreal(CIA_PRB,0xff);
	g_cia1.lticks = 0;

	g_cia1.todstart = sysclock_getticks();
}

void cia_destroy() {
	
}

word cia_timera_clicks(byte mode) {

	if (mode & CIA_CRA_TIMERINPUT) {
		// BUGBUG: Not implemented. Should count down on  CNT presses here. 
	} else {
		return sysclock_getticks() - g_cia1.lticks;
	}
	return 0;
}

word cia_timerb_clicks(byte mode) {

	switch(mode & (CIA_CRB_TIMERINPUT1 | CIA_CRB_TIMERINPUT2)) {

		case 0b00000000: // sysclock ticks
			return sysclock_getticks() - g_cia1.lticks;
		break;
		case 0b00100000: // CNT pin
			// BUGBUG: Not implemented
		break;
		case 0b01000000: // TAU undeflow
			return (g_cia1.isr & CIA_FLAG_TAUIRQ) ? 1 : 0;
		break;
		case 0b01100000: // TAU underflow + CNT pin
			// BUGBUG: Not implemented
		break;
		default: break;
	}
	return 0;
}

void cia_update_timer(CLOCKHANDLER tickfn,byte controlreg,byte hi,byte low,byte underflowflag) {

	word ticks;
	word tval = 0;
	word updateval= 0;
	byte cr = cia_getreal(controlreg);

	//
	// is timer a enabled?
	//
	if ((cr & CIA_CR_TIMERSTART) == 0) {
		//
		// timer not running.
		//
		return;
	}
	
	ticks = tickfn(cr);

	tval = ((word) cia_getreal(hi) << 8 ) | cia_getreal(low);
	updateval = tval - (ticks);
	cia_setreal(hi,updateval >> 8);
	cia_setreal(low,updateval & 0xFF); 
	
	// 
	// check for underflow condition.
	//
	if (updateval > tval) { 
		//
		// set bit in ICS regiser. 
		//
		g_cia1.isr |= underflowflag; 
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
			cia_setreal(controlreg,cr & (~CIA_CR_TIMERSTART));
		}

		//
		// reset to latch value. 
		//
		cia_latchtoreal(hi);
		cia_latchtoreal(low);
	}
}

void cia_update_todreg(byte reg,double timeincrement,byte modval) {

	unsigned long val;
	byte high;
	byte low;



	val = (sysclock_getticks() - g_cia1.todstart) / (NTSC_TICKS_PER_SECOND * timeincrement); // total number of those increments
	val %= modval; 	   // what flips back to zero.
	high = (val / 10) << 4;
	low =  val % 10; 

	//
	// BUGBUG am/pm!
	//
	cia_setreal(reg,high|low);
	
}

void cia_update_timeofday() {


	byte pm = g_cia1.regs[CIA_REAL][CIA_TODHRS] & BIT_7;
	
	unsigned long nclock = sysclock_getticks();

	cia_update_todreg(CIA_TODTENTHS,.1,10);
	cia_update_todreg(CIA_TODSECS,1,60);
	cia_update_todreg(CIA_TODMINS,60,60);
	cia_update_todreg(CIA_TODHRS,3600,12);

	if (g_cia1.regs[CIA_REAL][CIA_TODHRS] == 0) {
		pm = pm ? 0 : BIT_7;	
	}
	g_cia1.regs[CIA_REAL][CIA_TODHRS] |= pm;

	if (g_cia1.regs[CIA_REAL][CIA_TODHRS] == g_cia1.regs[CIA_ALARM][CIA_TODHRS] && 
		g_cia1.regs[CIA_REAL][CIA_TODMINS] == g_cia1.regs[CIA_ALARM][CIA_TODMINS] &&
		g_cia1.regs[CIA_REAL][CIA_TODSECS] == g_cia1.regs[CIA_ALARM][CIA_TODSECS] &&
		g_cia1.regs[CIA_REAL][CIA_TODTENTHS] >= g_cia1.regs[CIA_ALARM][CIA_TODTENTHS]) {

		//
		// hit alarm
		//
		g_cia1.isr |= CIA_FLAG_TODIRQ; 
	}
}


void cia_check_interrupts() {

	byte icr = cia_getreal(CIA_ICR);

	if (g_cia1.isr & icr & (CIA_FLAG_TODIRQ | CIA_FLAG_TAUIRQ | CIA_FLAG_TBUIRQ)) {
		cpu_irq();
	}
}

void cia_update() {

	//
	// order here is important. Timerb can count timera underflows so needs to come 
	// second.
	//
	g_cia1.isr = 0;
	cia_update_timer(cia_timera_clicks,CIA_CRA,CIA_TAHI,CIA_TALO,CIA_FLAG_TAUIRQ);
	cia_update_timer(cia_timerb_clicks,CIA_CRB,CIA_TBHI,CIA_TBLO,CIA_FLAG_TBUIRQ);
	cia_update_timeofday();
	cia_check_interrupts();

	g_cia1.lticks = sysclock_getticks ();
}
