#include "emu.h"

#define MAX_CHARS 256

typedef struct {
	byte column;
	byte row;
} KEYMAP;

KEYMAP g_c64KeyboardTable[MAX_CHARS] = {0};


byte g_c64kbd[0x08];					// keyboard column matrix.
	


byte c64kbd_getrow(byte col) {

	return g_c64kbd[col];	
}


void c64kbd_InitChar(byte ch, byte col, byte row) {

	g_c64KeyboardTable[ch].column = col;
	g_c64KeyboardTable[ch].row = row;
}

void c64kbd_destroy() {}

void c64kbd_init() {

	int i;

	c64kbd_InitChar(C64KEY_RUNSTOP,7,ROW_7); // STOP KEY NOT IMPL
	c64kbd_InitChar('/',6,ROW_7);
	c64kbd_InitChar(',',5,ROW_7);
	c64kbd_InitChar('N',4,ROW_7);
	c64kbd_InitChar('V',3,ROW_7);
	c64kbd_InitChar('X',2,ROW_7);
	c64kbd_InitChar(C64KEY_LSHIFT,1,ROW_7);
	c64kbd_InitChar(C64KEY_CURDOWN,0,ROW_7);

	c64kbd_InitChar('Q',7,ROW_6); 
	c64kbd_InitChar('^',6,ROW_6);
	c64kbd_InitChar('@',5,ROW_6);
	c64kbd_InitChar('O',4,ROW_6);
	c64kbd_InitChar('U',3,ROW_6);
	c64kbd_InitChar('T',2,ROW_6);
	c64kbd_InitChar('E',1,ROW_6);
	c64kbd_InitChar(C64KEY_F5,0,ROW_6);

	c64kbd_InitChar(C64KEY_C64,7,ROW_5); 
	c64kbd_InitChar('=',6,ROW_5);
	c64kbd_InitChar(':',5,ROW_5);
	c64kbd_InitChar('K',4,ROW_5);
	c64kbd_InitChar('H',3,ROW_5);
	c64kbd_InitChar('F',2,ROW_5);
	c64kbd_InitChar('S',1,ROW_5);
	c64kbd_InitChar(C64KEY_F3,0,ROW_5);

	c64kbd_InitChar(' ',7,ROW_4); 
	c64kbd_InitChar(C64KEY_RSHIFT,6,ROW_4);
	c64kbd_InitChar('.',5,ROW_4);
	c64kbd_InitChar('M',4,ROW_4);
	c64kbd_InitChar('B',3,ROW_4);
	c64kbd_InitChar('C',2,ROW_4);
	c64kbd_InitChar('Z',1,ROW_4);
	c64kbd_InitChar(C64KEY_F1,0,ROW_4);

	c64kbd_InitChar('2',7,ROW_3); 
	c64kbd_InitChar(C64KEY_HOME,6,ROW_3);
	c64kbd_InitChar('-',5,ROW_3);
	c64kbd_InitChar('0',4,ROW_3);
	c64kbd_InitChar('8',3,ROW_3);
	c64kbd_InitChar('6',2,ROW_3);
	c64kbd_InitChar('4',1,ROW_3);
	c64kbd_InitChar(C64KEY_F7,0,ROW_3);

	c64kbd_InitChar(C64KEY_CTRL,7,ROW_2); 
	c64kbd_InitChar(';',6,ROW_2);
	c64kbd_InitChar('L',5,ROW_2);
	c64kbd_InitChar('J',4,ROW_2);
	c64kbd_InitChar('G',3,ROW_2);
	c64kbd_InitChar('D',2,ROW_2);
	c64kbd_InitChar('A',1,ROW_2);
	c64kbd_InitChar(C64KEY_CURRIGHT,0,ROW_2);

	c64kbd_InitChar(C64KEY_BACK,7,ROW_1); 
	c64kbd_InitChar('*',6,ROW_1);
	c64kbd_InitChar('P',5,ROW_1);
	c64kbd_InitChar('I',4,ROW_1);
	c64kbd_InitChar('Y',3,ROW_1);
	c64kbd_InitChar('R',2,ROW_1);
	c64kbd_InitChar('W',1,ROW_1);
	c64kbd_InitChar('\n',0,ROW_1);

	c64kbd_InitChar('1',7,ROW_0); 
	c64kbd_InitChar(C64KEY_POUND,6,ROW_1);
	c64kbd_InitChar('+',5,ROW_0);
	c64kbd_InitChar('9',4,ROW_0);
	c64kbd_InitChar('7',3,ROW_0);
	c64kbd_InitChar('5',2,ROW_0);
	c64kbd_InitChar('3',1,ROW_0);
	c64kbd_InitChar(C64KEY_DELETE,0,ROW_0);

	c64kbd_InitChar(0xFF,0,0);

	for (i = 0; i < 8; i++) {
		g_c64kbd[i] = 0xff;
	}
}




void c64kbd_keyup(byte ch) {
	g_c64kbd[g_c64KeyboardTable[ch].column] |= g_c64KeyboardTable[ch].row;
}
void c64kbd_keydown(byte ch) {

	if (ch == C64KEY_RESTORE) {
		//
		// bugbug cia2 should handle this.
		//
		cpu_nmi();
	}
	g_c64kbd[g_c64KeyboardTable[ch].column] &= (~g_c64KeyboardTable[ch].row);
}


