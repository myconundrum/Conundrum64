#ifndef UX_H
#define UX_H 1

#include "emu.h"
#include <ncurses.h>



#define CONSOLE_LINES 10
#define REGISTER_COLS 20
#define DISPLAY_COLS 40

typedef struct {

	WINDOW * 	memory;
	WINDOW * 	console;
	WINDOW * 	registers;
	WINDOW * 	display;
	byte 		curpage;
	char 		buf[256];
	bool 		done;
	int 		bpos;
	byte 		asmh; // where to add asm instructions.
	byte 		asml; // where to add asm instructions.
	byte		brkh; // break instruction
	byte 		brkl; // break instruction low
	bool		brk; 
	FILE * 	    log;
	bool 		running;
	ASSEMBLER  * assembler;

} UX;

extern void init_ux(UX * ux,ASSEMBLER *a);
extern void destroy_ux(UX * ux);
extern void update_ux(UX *ux, CPU6502 *cpu);


#endif

