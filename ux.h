#ifndef UX_H
#define UX_H 1

#include "emu.h"
#include <ncurses.h>



#define CONSOLE_LINES 10
#define REGISTER_COLS 20
#define DISPLAY_COLS 40

typedef struct {
	word address;
	char buf[32];
} DISLINE;


#define DISLINESCOUNT 20


typedef struct {

	WINDOW * 	memory;
	WINDOW * 	console;
	WINDOW * 	registers;
	WINDOW * 	display;
	WINDOW * 	disassembly;
	byte 		curpage;
	DISLINE		dislines[DISLINESCOUNT];
	byte		disstart;
	byte 		discur;
	char 		buf[256];
	char 		disbuf[1024];
	bool 		done;
	int 		bpos;
	word 		asm_address;
	word		brk_address;
	bool		brk; 
	FILE * 	    log;
	bool 		running;
	bool		passthru;
	ASSEMBLER  * assembler;

} UX;

extern void init_ux(UX * ux,ASSEMBLER *a);
extern void destroy_ux(UX * ux);
extern void update_ux(UX *ux, CPU6502 *cpu);


#endif

