
#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include "emu.h"


void init_ux(UX * ux, ASSEMBLER * a) {


	// curses initialization 
	initscr(); 
	noecho();
	timeout(0);
	curs_set(0);
	start_color();

	init_pair(1,COLOR_GREEN,COLOR_BLACK);
	init_pair(2,COLOR_BLUE,COLOR_WHITE);
	init_pair(3,COLOR_WHITE,COLOR_BLACK);
	init_pair(4,COLOR_WHITE,COLOR_BLUE);
	

	memset(ux,0,sizeof(UX));

	ux->assembler = a;
	ux->registers 	= newwin (1,COLS,0,0);
	ux->display 	= newwin (29,44,1,61);
	ux->memory 		= newwin (20,60,1,0);
	ux->disassembly = newwin (18,60,21,0);
	ux->console  	= newwin (1,30,41,0);

	ux->log = fopen("log.txt","w+");
	ux->running = false;



}

void destroy_ux(UX * ux) {

	// curses destructor
	endwin();
	fclose(ux->log);
}



void refresh_memory(UX * ux, CPU6502 * c) {

	int i;
	int j; 
	
	wattron(ux->memory,COLOR_PAIR(2));
	werase(ux->memory);

	for (int i = 0; i < 16; i++) {
		wmove(ux->memory,i,0);
		wprintw(ux->memory,"%02X:%02X:",
			ux->curpage,i*16);

		for (j = 0; j < 16; j++) {
			wprintw(ux->memory," %02X",c->ram[ux->curpage][i*16+j]);
		}
	}

	wrefresh(ux->memory);
	wattroff(ux->memory,COLOR_PAIR(2));
}

void draw_border(UX *ux, CPU6502 *c) {
	
	int i;
	wattron(ux->display,COLOR_PAIR(4));
	for (i = 0; i < 44; i++) {
		wmove(ux->display,0,i);
		waddch(ux->display,' ');
		wmove(ux->display,1,i);
		waddch(ux->display,' ');
		wmove(ux->display,27,i);
		waddch(ux->display,' ');
		wmove(ux->display,28,i);
		waddch(ux->display,' ');
	}
	for (i = 0; i < 29; i++) {
		wmove(ux->display,i,0);
		waddch(ux->display,' ');
		wmove(ux->display,i,1);
		waddch(ux->display,' ');
		wmove(ux->display,i,42);
		waddch(ux->display,' ');
		wmove(ux->display,i,43);
		waddch(ux->display,' ');
	}
	wattroff(ux->display,COLOR_PAIR(4));
}


void refresh_disassembly(UX * ux, CPU6502 * c) {

	werase(ux->disassembly);
	wprintw(ux->disassembly,"%s",ux->disbuf);
	wrefresh(ux->disassembly);
	
}


//
// BUGBUG terribly incomplete.
//
char getScreenChar(byte code) {

	if (code < 0x20) {
		code += 0x40;
	}
	return  code;
}



void refresh_display(UX * ux, CPU6502 * c) {

	int i;
	int j;
	byte high;
	byte low;
	int address;
	byte b;
	byte ch;

	werase(ux->display);
	draw_border(ux,c);
	
	wattron(ux->display,COLOR_PAIR(2));



	for (i = 0; i < 25; i++) {
		for (j = 0; j < 40; j++) {

			wmove(ux->display,i+2,j+2);
			address = 0x0400 + i * 40 + j;
			high = address >> 8;
			low = address & 0xff;
			b = c->ram[high][low];
			ch = getScreenChar(b);
			wprintw(ux->display,"%c",isprint(ch) ? ch : ' ');
		}
	}

	wrefresh(ux->display);
	wattroff(ux->display,COLOR_PAIR(2));
}

void refresh_registers(UX * ux, CPU6502 * c) {

	wattron(ux->registers,COLOR_PAIR(1));
	werase(ux->registers);	
	wmove(ux->registers,0,0);
	wprintw(ux->registers,
		"A: $%02X X: $%02X Y: $%02X STACK: $%02X PC: $%02X:%02X N:%dV%dX%dB%dD%dI%dZ%dC%d CYCLES: %010d",
		c->reg_a,c->reg_x,c->reg_y,c->reg_stack,c->pc_high,c->pc_low,
		((c->reg_status & N_FLAG )!= 0),
		((c->reg_status & V_FLAG )!= 0),
		((c->reg_status & X_FLAG )!= 0),
		((c->reg_status & B_FLAG )!= 0),
		((c->reg_status & D_FLAG )!= 0),
		((c->reg_status & I_FLAG )!= 0),
		((c->reg_status & Z_FLAG )!= 0),
		((c->reg_status & C_FLAG )!= 0),
		c->cycles);

	wrefresh(ux->registers);
	wattroff(ux->registers,COLOR_PAIR(1));
}


void refresh_console(UX * ux, CPU6502 * c) {

	int ch = getch();

	wattron(ux->console,COLOR_PAIR(1));

	switch(ch) {
		case 27:
			handle_step(ux,c); 
			//
			// BUGBUG on my keyboard, key down returns three keys.
			//
			getch();getch();
			
		break;
		case -1:
		break;
		case '\n':
			handle_command(ux,c);
			memset(ux->buf,0,256);
			ux->bpos = 0;
		break;
		default:
			if (isprint(ch)) {
				ch = toupper(ch);
				ux->buf[ux->bpos++] = ch;
			}
		break;
	}

	werase(ux->console);
	wmove(ux->console,0,0);
	wprintw(ux->console,":> %s", ux->buf);
	wrefresh(ux->console);

	wattroff(ux->console,COLOR_PAIR(1));
}


void update_ux(UX *ux, CPU6502 *cpu) {
	refresh(); // curses refresh
	refresh_memory(ux,cpu);
	refresh_registers(ux,cpu);
	refresh_console(ux,cpu);
	refresh_display(ux,cpu);
	refresh_disassembly(ux,cpu);
}
