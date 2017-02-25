

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


KEYMAP g_ciaKeyboardTable[MAX_CHARS] = {0};


byte g_ciapress = 0xFF;
int g_ciaticks = 0x00;

void ciaInitChar(byte ch, byte col, byte row) {

	g_ciaKeyboardTable[ch].column = col;
	g_ciaKeyboardTable[ch].column = row;
}


void cia1_init() {

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

}

void cia1_update() {

	KEYMAP km;

	if (g_ciaticks) {
		g_ciaticks--;
		if (!g_ciaticks) {
			g_ciapress = 0xff;
		}
	}

	mem_poke(CIA1_PORTB_ADD,0);
	if (g_ciaticks) {

		if (g_ciaKeyboardTable[g_ciapress].column & mem_peek(CIA1_PORTA_ADD)) {
			mem_poke(CIA1_PORTB_ADD,mem_peek(CIA1_PORTB_ADD | g_ciaKeyboardTable[g_ciapress].row));
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


