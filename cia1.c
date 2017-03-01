
#include <time.h>
#include <string.h>
#include "emu.h"
#include "cia1.h"

#define MAX_CHARS 256

typedef struct {
	byte column;
	byte row;
} KEYMAP;

KEYMAP g_ciaKeyboardTable[MAX_CHARS] = {0};

typedef struct {


	byte kbd[0x08];				// keyboard column matrix.
	byte regs[0x10];			// CIA1 internal registers. use CIA1_REGS enum to address.

	unsigned long lticks;		// stores the value of sysclock_getticks() on last cia1_update()

	byte tahilatch;				// latched start value high byte for timer a
	byte talolatch;				// latched start value lo byte for timer b
	byte tbhilatch;				// latched start value high byte for timer a
	byte tblolatch;				// latched start value lo byte for timer b
	byte isr;  					// read pulls current irq status

} CIA1;


CIA1 g_cia1;


byte cia1_getkbdprb() {

	int i = 0;
	byte val = 0xff;

	for (i = 0; i < 8; i++) {
		if (~g_cia1.regs[CIA1_PRA] & (0x01 << i)) {
			val &= g_cia1.kbd[i];
		}
	}

	return val;

}

byte cia1_peek(byte address) {

	byte val;

	switch(address %0x10) {
		case CIA1_ICR: 
			val = g_cia1.isr;
		break;
		case CIA1_PRB:
			val = cia1_getkbdprb();
		break;
		default: 
			val = g_cia1.regs[address % 0x10];
		break;
	}

	return val;
}

void cia1_seticr(byte val) {

	byte old = g_cia1.regs[CIA1_ICR];
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
	
	g_cia1.regs[CIA1_ICR] = new;
}

void cia_setport(CIA1_REGISTERS reg, CIA1_REGISTERS ddr, byte val) {

	byte test = 0b10000000;
	byte new = 0;
	byte portval = g_cia1.regs[reg];
	byte ddrval  = g_cia1.regs[ddr];

	while (test) {
		new |= (test & ddrval) ? (test & val) : (test & portval);
		test >>= 1;
	}

	g_cia1.regs[reg] = new;
}

void cia1_poke(byte address,byte val) {

	switch (address % 0x10) {

		case CIA1_PRA: 			// data port a register
			cia_setport(CIA1_PRA,CIA1_DDRA,val);
		break;
		case CIA1_PRB: 			// data port b register
			cia_setport(CIA1_PRB,CIA1_DDRB,val);
			DEBUG_PRINT("PRB set to %02X (%02X) near %04X\n",val,g_cia1.regs[CIA1_PRB],cpu_getpc());
		break;
		case CIA1_TALO:			// Timer A Low Byte 
			g_cia1.talolatch = val;	
			g_cia1.regs[CIA1_TALO] = val;				
		break;
		case CIA1_TAHI:			// Timer A High Byte 
			g_cia1.tahilatch = val;	
			g_cia1.regs[CIA1_TAHI] = val;					
		break;
		case CIA1_TBLO:			// Timer A Low Byte 
			g_cia1.tblolatch = val;	
			g_cia1.regs[CIA1_TBLO] = val;		
		break;
		case CIA1_TBHI:			// Timer A High Byte 
			g_cia1.tbhilatch = val;	
			g_cia1.regs[CIA1_TBLO] = val;	
		break;
		case CIA1_ICR:			// Interupt control and status 
			cia1_seticr(val);
		break;
		case CIA1_CRA:			// Timer A control register 

			g_cia1.regs[CIA1_CRA] = val;

			if (val & CIA_CRA_FORCELATCH) { 
				// force load startlatch into current 
				g_cia1.regs[CIA1_TALO] = g_cia1.talolatch;
				g_cia1.regs[CIA1_TAHI] = g_cia1.tahilatch;
			}		
		break;
		case CIA1_CRB:			// Timer B control register 

			g_cia1.regs[CIA1_CRB] = val;
			if (val & CIA_CRA_FORCELATCH) { 
				// force load startlatch into current 
				g_cia1.regs[CIA1_TBLO] = g_cia1.tblolatch;
				g_cia1.regs[CIA1_TBHI] = g_cia1.tbhilatch;
			}		
		break;
		default: g_cia1.regs[address % 10] = val;
		break;
	}	
}

void ciaInitChar(byte ch, byte col, byte row) {

	g_ciaKeyboardTable[ch].column = col;
	g_ciaKeyboardTable[ch].row = row;
}

void cia1_init() {

	int i;
	memset(&g_cia1,0,sizeof(CIA1));

	ciaInitChar(C64KEY_RUNSTOP,7,ROW_7); // STOP KEY NOT IMPL
	ciaInitChar('/',6,ROW_7);
	ciaInitChar(',',5,ROW_7);
	ciaInitChar('N',4,ROW_7);
	ciaInitChar('V',3,ROW_7);
	ciaInitChar('X',2,ROW_7);
	ciaInitChar(C64KEY_LSHIFT,1,ROW_7);
	ciaInitChar(C64KEY_CURDOWN,0,ROW_7);

	ciaInitChar('Q',7,ROW_6); 
	ciaInitChar('^',6,ROW_6);
	ciaInitChar('@',5,ROW_6);
	ciaInitChar('O',4,ROW_6);
	ciaInitChar('U',3,ROW_6);
	ciaInitChar('T',2,ROW_6);
	ciaInitChar('E',1,ROW_6);
	ciaInitChar(C64KEY_F5,0,ROW_6);

	ciaInitChar(C64KEY_C64,7,ROW_5); 
	ciaInitChar('=',6,ROW_5);
	ciaInitChar(':',5,ROW_5);
	ciaInitChar('K',4,ROW_5);
	ciaInitChar('H',3,ROW_5);
	ciaInitChar('F',2,ROW_5);
	ciaInitChar('S',1,ROW_5);
	ciaInitChar(C64KEY_F3,0,ROW_5);

	ciaInitChar(' ',7,ROW_4); 
	ciaInitChar(C64KEY_RSHIFT,6,ROW_4);
	ciaInitChar('.',5,ROW_4);
	ciaInitChar('M',4,ROW_4);
	ciaInitChar('B',3,ROW_4);
	ciaInitChar('C',2,ROW_4);
	ciaInitChar('Z',1,ROW_4);
	ciaInitChar(C64KEY_F1,0,ROW_4);

	ciaInitChar('2',7,ROW_3); 
	ciaInitChar(C64KEY_HOME,6,ROW_3);
	ciaInitChar('-',5,ROW_3);
	ciaInitChar('0',4,ROW_3);
	ciaInitChar('8',3,ROW_3);
	ciaInitChar('6',2,ROW_3);
	ciaInitChar('4',1,ROW_3);
	ciaInitChar(C64KEY_F7,0,ROW_3);

	ciaInitChar(C64KEY_CTRL,7,ROW_2); 
	ciaInitChar(';',6,ROW_2);
	ciaInitChar('L',5,ROW_2);
	ciaInitChar('J',4,ROW_2);
	ciaInitChar('G',3,ROW_2);
	ciaInitChar('D',2,ROW_2);
	ciaInitChar('A',1,ROW_2);
	ciaInitChar(C64KEY_CURRIGHT,0,ROW_2);

	ciaInitChar(C64KEY_BACK,7,ROW_1); 
	ciaInitChar('*',6,ROW_1);
	ciaInitChar('P',5,ROW_1);
	ciaInitChar('I',4,ROW_1);
	ciaInitChar('Y',3,ROW_1);
	ciaInitChar('R',2,ROW_1);
	ciaInitChar('W',1,ROW_1);
	ciaInitChar('\n',0,ROW_1);

	ciaInitChar('1',7,ROW_0); 
	ciaInitChar(C64KEY_POUND,6,ROW_1);
	ciaInitChar('+',5,ROW_0);
	ciaInitChar('9',4,ROW_0);
	ciaInitChar('7',3,ROW_0);
	ciaInitChar('5',2,ROW_0);
	ciaInitChar('3',1,ROW_0);
	ciaInitChar(C64KEY_DELETE,0,ROW_0);

	ciaInitChar(0xFF,0,0);

	for (i = 0; i < 8; i++) {
		g_cia1.kbd[i] = 0xff;
	}
	//
	// set default direction for ports. 
	//
	g_cia1.regs[CIA1_DDRA] = 0xff;
	g_cia1.regs[CIA1_DDRB] = 0x0;
	g_cia1.regs[CIA1_PRB] = 0xff;

	g_cia1.lticks = 0;
}

void cia1_destroy() {
	
}

void cia1_update_timera() {

	unsigned long ticks;
	word tval = 0;
	word updateval= 0;

	//
	// is timer a enabled?
	//
	if ((g_cia1.regs[CIA1_CRA] & CIA_CRA_TIMERSTART) == 0) {
		//
		// timer not running.
		//
		return;
	}
	//
	// count down ticks
	//
	if (g_cia1.regs[CIA1_CRA] & CIA_CRA_TIMERINPUT) {

		//
		// BUGBUG: Not implemented. Should count down on  CNT presses here. 
		//

	} else {
		ticks = sysclock_getticks();
		tval = ((word) g_cia1.regs[CIA1_TAHI] << 8 ) | g_cia1.regs[CIA1_TALO];
		updateval = tval - (ticks - g_cia1.lticks);
		g_cia1.lticks = ticks;
		g_cia1.regs[CIA1_TAHI] = updateval >> 8;
		g_cia1.regs[CIA1_TALO] = updateval & 0xFF; 
	
	}
	// 
	// check for underflow condition.
	//
	if (updateval > tval) { 

		//
		// set bit in ICS regiser. 
		//
		g_cia1.isr = CIA_FLAG_TAUIRQ; 
		//
		// check to see if (and how) to signal underflow on port b bit six. 
		//
		if (g_cia1.regs[CIA1_CRA] & CIA_CRA_PORTBSELECT) {

			if (g_cia1.regs[CIA1_CRA] & CIA_CRA_PORTBMODE) {
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
		if (g_cia1.regs[CIA1_CRA] & CIA_CRA_TIMERRUNMODE) {
			g_cia1.regs[CIA1_CRA] &= (~CIA_CRA_TIMERSTART);
		}
		//
		// should we trigger an interrupt? 
		//
		if (g_cia1.regs[CIA1_ICR] & CIA_FLAG_TAUIRQ) {
			//
			// set isr bit that we did do an interrupt and signal IRQ line on CPU. 
			//
			g_cia1.isr |=  CIA_FLAG_CIAIRQ;	
			cpu_irq();
		}
		//
		// reset to latch value. 
		//
		g_cia1.regs[CIA1_TAHI] = g_cia1.tahilatch;
		g_cia1.regs[CIA1_TALO] = g_cia1.talolatch;
	}
}


void cia_triggernmi() {

}

void cia1_update_timerb() {}

void cia1_update_timeofday() {}

void cia1_update_keyboard() {}

void cia1_update() {

	cia1_update_timera();
	cia1_update_timerb();
	cia1_update_timeofday();
	cia1_update_keyboard();
}

#define TICKCOUNT 2000


void cia1_keyup(byte ch) {
	g_cia1.kbd[g_ciaKeyboardTable[ch].column] |= g_ciaKeyboardTable[ch].row;
}
void cia1_keydown(byte ch) {

	if (ch == C64KEY_RESTORE) {
		//
		// bugbug cia2 should handle this.
		//
		cpu_nmi();
	}

	g_cia1.kbd[g_ciaKeyboardTable[ch].column] &= (~g_ciaKeyboardTable[ch].row);
}

