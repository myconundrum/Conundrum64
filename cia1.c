
#include <time.h>
#include <string.h>
#include "emu.h"
#include "cia1.h"

//
// using row/col info from https://www.c64-wiki.com/wiki/Keyboard
//


#define COLUMN_0   0b00000001
#define COLUMN_1   0b00000010
#define COLUMN_2   0b00000100
#define COLUMN_3   0b00001000
#define COLUMN_4   0b00010000
#define COLUMN_5   0b00100000
#define COLUMN_6   0b01000000
#define COLUMN_7   0b10000000

#define ROW_0   0b00000001
#define ROW_1   0b00000010
#define ROW_2   0b00000100
#define ROW_3   0b00001000
#define ROW_4   0b00010000
#define ROW_5   0b00100000
#define ROW_6   0b01000000
#define ROW_7   0b10000000


#define MAX_CHARS 256

typedef struct {
	byte column;
	byte row;
} KEYMAP;


typedef struct {

	byte r[0x10]; // 16 registers.
	unsigned long lticks;


	byte tahilatch;				// latched start value high byte for timer a
	byte talolatch;				// latched start value lo byte for timer b
	byte tahicur;				// current timer a start value
	byte talocur;				// current timer b start value
	byte tbhilatch;				// latched start value high byte for timer a
	byte tblolatch;				// latched start value lo byte for timer b
	byte tbhicur;				// current timer a start value
	byte tblocur;				// current timer b start value
	byte todtenths; 			// current time of day tenths (BCD)
	byte todsecs;	 			// current time of day seconds (BCD)
	byte todmins; 				// current time of day minutes (BCD)
	byte todhrs;				// current time of day hours  (BCD)

	byte cra;					// timer a control bits
	byte crb; 					// timer b control bits






	//
	// various timer interrupts enabled or disabled. 
	//
	byte icr;		// write sets irq control 
	byte isr;  		// read  pulls current irq status

	FILE * log;

} CIA1;

CIA1 g_cia1;

byte cia1_peek(byte address) {

	byte val = 0;

	switch (address % 0x10) {

		case CIA1_PRA: 			// data port a register
		// not implemented yet
		break;
		case CIA1_PRB: 			// data port b register
		// not implemented yet
		break;
		case CIA1_DDRA: 		// porta data direction register
		// not implemented yet
		break;
		case CIA1_DDRB: 		// portb data direction register
		// not implemented yet
		break;
		case CIA1_TALO:			// Timer A Low Byte 
			val = g_cia1.talocur;				
		break;
		case CIA1_TAHI:			// Timer A High Byte 
			val = g_cia1.tahicur;				
		break;
		case CIA1_TBLO:			// Timer A Low Byte 
			val = g_cia1.tblocur;	
		break;
		case CIA1_TBHI:			// Timer A High Byte 
			val = g_cia1.tbhicur;		
		break;
		case CIA1_TODTENTHS:	// BCD Time of Day Tenths of Second
			val = g_cia1.todtenths;
		break;
		case CIA1_TODSECS:		// BCD TOD Seconds 
			val = g_cia1.todsecs;
		break;
		case CIA1_TODMINS:		// BCD TOD Minutes
			val = g_cia1.todmins; 				
		break;
		case CIA1_TODHRS:		// BCD TOD Hours
			val = g_cia1.todhrs;
		break;
		case CIA1_SDR: 			// Serial Shift Register 
		// not implemented yet
		break;
		case CIA1_ICR:			// Interupt control and status 
			val = g_cia1.isr;
		break;
		case CIA1_CRA:			// Timer A control register 
			val = g_cia1.cra;		
		break;
		case CIA1_CRB:			// Timer B control register 
			val = g_cia1.crb;
		break;
	}	

	return val;
}



void cia1_seticr(byte val) {

	byte old = g_cia1.icr;
	byte fillbit = val & CIA_FLAG_FILIRQ ? 0xFF : 0;
	byte new = 0;

	/*
		Enabling or Disabling IRQSs. Bits 0-6 specify which IRQs you want to enable/disable
		(a 1 in a bit position means you are attempting to change that IRQ)

		Bit 7 is the bit that is used for these. In other words -- if a bit is set in 
		positions 0 -6, then use the value of bit 7 for that bit, otherwise, keep existing
		value.
	*/

	new |= (val & 0x01) ? (fillbit & 0x01) : (old & 0x01);  
	new |= (val & 0x02) ? (fillbit & 0x02) : (old & 0x02);
	new |= (val & 0x04) ? (fillbit & 0x04) : (old & 0x04);  
	new |= (val & 0x08) ? (fillbit & 0x08) : (old & 0x08);
	new |= (val & 0x10) ? (fillbit & 0x10) : (old & 0x10);  
	new |= (val & 0x20) ? (fillbit & 0x20) : (old & 0x20);
	new |= (val & 0x40) ? (fillbit & 0x40) : (old & 0x40);  
	
	g_cia1.icr = new;
}


void cia1_poke(byte address,byte val) {

	switch (address % 0x10) {

		case CIA1_PRA: 			// data port a register
		// not implemented yet
		break;
		case CIA1_PRB: 			// data port b register
		// not implemented yet
		break;
		case CIA1_DDRA: 		// porta data direction register
		// not implemented yet
		break;
		case CIA1_DDRB: 		// portb data direction register
		// not implemented yet
		break;
		case CIA1_TALO:			// Timer A Low Byte 
			g_cia1.talolatch = val;	
			g_cia1.talocur = val;				
		break;
		case CIA1_TAHI:			// Timer A High Byte 
			g_cia1.tahilatch = val;	
			g_cia1.tahicur = val;					
		break;
		case CIA1_TBLO:			// Timer A Low Byte 
			g_cia1.tblolatch = val;	
			g_cia1.tblocur = val;		
		break;
		case CIA1_TBHI:			// Timer A High Byte 
			g_cia1.tbhilatch = val;	
			g_cia1.tblocur = val;	
		break;
		case CIA1_TODTENTHS:	// BCD Time of Day Tenths of Second
			g_cia1.todtenths = val;
		break;
		case CIA1_TODSECS:		// BCD TOD Seconds 
			g_cia1.todsecs = val;
		break;
		case CIA1_TODMINS:		// BCD TOD Minutes
			g_cia1.todmins = val; 				
		break;
		case CIA1_TODHRS:		// BCD TOD Hours
			g_cia1.todhrs = val;
		break;
		case CIA1_SDR: 			// Serial Shift Register 
		// not implemented yet
		break;
		case CIA1_ICR:			// Interupt control and status 
			cia1_seticr(val);
		break;
		case CIA1_CRA:			// Timer A control register 

			fprintf(g_cia1.log,"setting CRA to %02X\n",val);
			g_cia1.cra = val;

			if (g_cia1.cra & CIA_CRA_FORCELATCH) { 
				// force load startlatch into current 
				g_cia1.talocur = g_cia1.talolatch;
				g_cia1.tahicur = g_cia1.tahilatch;
			}		
		break;
		case CIA1_CRB:			// Timer B control register 
			g_cia1.crb = val;
			if (g_cia1.cra & CIA_CRA_FORCELATCH) { 
				// force load startlatch into current 
				g_cia1.tblocur = g_cia1.tblolatch;
				g_cia1.tbhicur = g_cia1.tbhilatch;
			}		
		break;
	}	
}


KEYMAP g_ciaKeyboardTable[MAX_CHARS] = {0};

byte g_ciapress = 0xFF;
int g_ciaticks = 0x00;

void ciaInitChar(byte ch, byte col, byte row) {

	g_ciaKeyboardTable[ch].column = col;
	g_ciaKeyboardTable[ch].column = row;
}


void cia1_init() {

	memset(&g_cia1,0,sizeof(CIA1));

	//ciaInitChar({STOP},COLUMN_7,ROW_7); // STOP KEY NOT IMPL
	ciaInitChar('/',COLUMN_6,ROW_7);
	ciaInitChar(',',COLUMN_5,ROW_7);
	ciaInitChar('N',COLUMN_4,ROW_7);
	ciaInitChar('V',COLUMN_3,ROW_7);
	ciaInitChar('X',COLUMN_2,ROW_7);
	//ciaInitChar({LSHIFT},COLUMN_1,ROW_7);
	//ciaInitChar({CURSOR DN},COLUMN_0,ROW_7);

	ciaInitChar('Q',COLUMN_7,ROW_6); 
	ciaInitChar('^',COLUMN_6,ROW_6);
	ciaInitChar('@',COLUMN_5,ROW_6);
	ciaInitChar('O',COLUMN_4,ROW_6);
	ciaInitChar('U',COLUMN_3,ROW_6);
	ciaInitChar('T',COLUMN_2,ROW_6);
	ciaInitChar('E',COLUMN_1,ROW_6);
	//ciaInitChar({F5},COLUMN_0,ROW_6);

	//ciaInitChar({C64},COLUMN_7,ROW_5); 
	ciaInitChar('=',COLUMN_6,ROW_5);
	ciaInitChar(':',COLUMN_5,ROW_5);
	ciaInitChar('K',COLUMN_4,ROW_5);
	ciaInitChar('H',COLUMN_3,ROW_5);
	ciaInitChar('F',COLUMN_2,ROW_5);
	ciaInitChar('S',COLUMN_1,ROW_5);
	//ciaInitChar({F3},COLUMN_0,ROW_5);

	ciaInitChar(' ',COLUMN_7,ROW_4); 
	//ciaInitChar({RSHIFT},COLUMN_6,ROW_4);
	ciaInitChar('.',COLUMN_5,ROW_4);
	ciaInitChar('M',COLUMN_4,ROW_4);
	ciaInitChar('B',COLUMN_3,ROW_4);
	ciaInitChar('C',COLUMN_2,ROW_4);
	ciaInitChar('Z',COLUMN_1,ROW_4);
	//ciaInitChar({F1},COLUMN_0,ROW_4);

	ciaInitChar('2',COLUMN_7,ROW_3); 
	//ciaInitChar({HOME},COLUMN_6,ROW_3);
	ciaInitChar('-',COLUMN_5,ROW_3);
	ciaInitChar('0',COLUMN_4,ROW_3);
	ciaInitChar('8',COLUMN_3,ROW_3);
	ciaInitChar('6',COLUMN_2,ROW_3);
	ciaInitChar('4',COLUMN_1,ROW_3);
	//ciaInitChar({F7},COLUMN_0,ROW_3);

	//ciaInitChar({CTRL},COLUMN_7,ROW_2); 
	ciaInitChar(';',COLUMN_6,ROW_2);
	ciaInitChar('L',COLUMN_5,ROW_2);
	ciaInitChar('J',COLUMN_4,ROW_2);
	ciaInitChar('G',COLUMN_3,ROW_2);
	ciaInitChar('D',COLUMN_2,ROW_2);
	ciaInitChar('A',COLUMN_1,ROW_2);
	//ciaInitChar({CRSR RIGHT},COLUMN_0,ROW_2);

	//ciaInitChar({BACK},COLUMN_7,ROW_1); 
	ciaInitChar('*',COLUMN_6,ROW_1);
	ciaInitChar('P',COLUMN_5,ROW_1);
	ciaInitChar('I',COLUMN_4,ROW_1);
	ciaInitChar('Y',COLUMN_3,ROW_1);
	ciaInitChar('R',COLUMN_2,ROW_1);
	ciaInitChar('W',COLUMN_1,ROW_1);
	ciaInitChar('\n',COLUMN_0,ROW_1);

	ciaInitChar('1',COLUMN_7,ROW_0); 
	//ciaInitChar({LB},COLUMN_6,ROW_1);
	ciaInitChar('+',COLUMN_5,ROW_0);
	ciaInitChar('9',COLUMN_4,ROW_0);
	ciaInitChar('7',COLUMN_3,ROW_0);
	ciaInitChar('5',COLUMN_2,ROW_0);
	ciaInitChar('3',COLUMN_1,ROW_0);
	//ciaInitChar({DELETE},COLUMN_0,ROW_0);


	ciaInitChar(0xFF,0,0);

	g_cia1.lticks = 0;
	g_cia1.log = fopen("cia.log","w+");
}

void cia1_destroy() {
	fclose(g_cia1.log);
}



void cia1_update_timera() {

	unsigned long ticks;
	word tval = 0;
	word updateval= 0;

	//
	// is timer a enabled?
	//
	if ((g_cia1.cra & CIA_CRA_TIMERSTART) == 0) {
		//
		// timer not running.
		//
		return;
	}

	//
	// count down ticks
	//
	if (g_cia1.cra & CIA_CRA_TIMERINPUT) {

		ticks = sysclock_getticks();
		tval = ((word) g_cia1.tahicur << 8 ) | g_cia1.talocur;
		updateval = tval - (ticks - g_cia1.lticks);
		g_cia1.lticks = ticks;
		g_cia1.tahicur = updateval >> 8;
		g_cia1.talocur = updateval & 0xFF; 

	} else {
		//
		// BUGBUG: Not implemented. Should count down on  CNT presses here. 
		//
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
		if (g_cia1.cra & CIA_CRA_PORTBSELECT) {

			if (g_cia1.cra & CIA_CRA_PORTBMODE) {
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
		if (g_cia1.cra & CIA_CRA_TIMERRUNMODE) {
			g_cia1.cra &= (~CIA_CRA_TIMERSTART);
		}

		//
		// should we trigger an interrupt? 
		//
		if (g_cia1.icr & CIA_FLAG_TAUIRQ) {
			fprintf(g_cia1.log,"IRQ signaled at clock ticks %010d (elapsed secs %d) curval %02X%02X\n",
				(int) sysclock_getticks(),(int) sysclock_getticks() / NTSC_TICKS_PER_SECOND,
				g_cia1.tahicur, g_cia1.talocur);
				

			//
			// set isr bit that we did do an interrupt and signal IRQ line on CPU. 
			//
			g_cia1.isr |=  CIA_FLAG_CIAIRQ;	
			cpu_setirq();
		}

		//
		// reset to latch value. 
		//
		g_cia1.tahicur = g_cia1.tahilatch;
		g_cia1.talocur = g_cia1.talolatch;
	
	}

}

void cia1_update_timerb() {
	
}

void cia1_update_timeofday() {
	
}



void cia1_update() {

	cia1_update_timera();
	cia1_update_timerb();
	cia1_update_timeofday();

}


void cia1_update_old() {

	KEYMAP km;

	unsigned long ticks  = sysclock_getticks();

	word tval = ((word)cia1_peek(CIA1_TAHI) << 8) | cia1_peek(CIA1_TALO);
	word updateval = tval - (ticks - g_cia1.lticks);
	g_cia1.lticks = ticks;

	cia1_poke(CIA1_TAHI, updateval >> 8);
	cia1_poke(CIA1_TALO, updateval & 0xFF);
/*
	//
	// very simpliefied irq handling right now. just timera ...
	//
	if (updateval > tval && g_cia1.TAUirq) {
		//
		// Timer A underflow IRQ occurred.
		//
		fprintf(g_cia1.log,"IRQ signaled at clock ticks %010d (elapsed secs %d) curval %02X%02X\n",
			(int) sysclock_getticks(),(int) sysclock_getticks() / NTSC_TICKS_PER_SECOND,
			cia1_peek(CIA1_TAHI),cia1_peek(CIA1_TALO));
		cpu_setirq();
	}
*/
	if (g_ciaticks) {
		g_ciaticks--;
		if (!g_ciaticks) {
			g_ciapress = 0xff;
		}
	}

	if (g_ciaticks) {

		if (g_ciaKeyboardTable[g_ciapress].column & g_cia1.r[CIA1_PRA]) {
			g_cia1.r[CIA1_PRB] = g_ciaKeyboardTable[g_ciapress].row;		
		}
	}
}

#define TICKCOUNT 10000

void cia1_keypress(byte ch) {

	if (!g_ciaticks) {
		g_ciaticks = TICKCOUNT;
		g_ciapress = ch;
	}
}


