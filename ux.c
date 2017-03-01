
#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include "emu.h"
#include <math.h>
#include <time.h>

typedef struct {
	byte ch;
	clock_t t;
} KEYUP;

KEYUP g_keyqueue[10];


void init_ux(UX * ux, ASSEMBLER * a) {

	int i;

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

	init_pair(5,COLOR_WHITE,COLOR_GREEN); 

	memset(ux,0,sizeof(UX));

	ux->assembler = a;
	ux->registers 	= newwin (1,COLS,0,0);
	ux->display 	= newwin (29,44,1,61);
	ux->memory 		= newwin (18,60,1,0);
	ux->disassembly = newwin (20,60,20,0);
	ux->console  	= newwin (1,30,42,0);

	ux->log = fopen("log.txt","w+");
	ux->running = false;


	for (i = 0; i < 10; i++) {
		g_keyqueue[i].t = -1;
	}


}

void destroy_ux(UX * ux) {

	endwin();
	fclose(ux->log);
}

void refresh_memory(UX * ux) {

	int i;
	int j; 
	
	wattron(ux->memory,COLOR_PAIR(2));
	werase(ux->memory);

	for (int i = 0; i < 16; i++) {
		wmove(ux->memory,i,0);
		wprintw(ux->memory,"%02X:%02X:",
			ux->curpage,i*16);

		for (j = 0; j < 16; j++) {
			wprintw(ux->memory," %02X",
				mem_peek((ux->curpage << 8) | (i*16+j)));
		}
	}

	wrefresh(ux->memory);
	wattroff(ux->memory,COLOR_PAIR(2));
}

void draw_border(UX *ux) {
	
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

void refresh_disassembly(UX * ux) {

	int i;
	werase(ux->disassembly);

	wattron(ux->disassembly,COLOR_PAIR(3));

	for (i = 0;i<DISLINESCOUNT;i++) {
		if (ux->discur == i) {
			wattron(ux->disassembly,COLOR_PAIR(5));
		}
		wmove(ux->disassembly,i,0);
		wprintw(ux->disassembly,"$%04X:\t%s\n",
			ux->dislines[i].address,ux->dislines[i].buf);

		if (ux->discur == i) {
			wattroff(ux->disassembly,COLOR_PAIR(5));
			wattron(ux->disassembly,COLOR_PAIR(3));
		}
	}

	wrefresh(ux->disassembly);	
	wattroff(ux->disassembly,COLOR_PAIR(3));
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

void refresh_display(UX * ux) {

	int i;
	int j;
	byte high;
	byte low;
	int address;
	byte b;
	byte ch;

	werase(ux->display);
	draw_border(ux);
	
	wattron(ux->display,COLOR_PAIR(2));

	for (i = 0; i < 25; i++) {
		for (j = 0; j < 40; j++) {

			wmove(ux->display,i+2,j+2);
			address = 0x0400 + i * 40 + j;
			high = address >> 8;
			low = address & 0xff;
			b = mem_peek((high << 8) | low);
			ch = getScreenChar(b);
			
			//
			// cursor blink hack
			//
			if (ch == 0xa0) {
				ch = 0x20;

				wattroff(ux->display,COLOR_PAIR(2));
				wattron(ux->display,COLOR_PAIR(4));
				wprintw(ux->display,"%c",isprint(ch) ? ch : ' ');
				wattroff(ux->display,COLOR_PAIR(4));
				wattron(ux->display,COLOR_PAIR(2));

			} else {
				wprintw(ux->display,"%c",isprint(ch) ? ch : ' ');
			}
		}
	}

	wrefresh(ux->display);
	wattroff(ux->display,COLOR_PAIR(2));
}

//
// used for some debugging. Probably not needed any more. 
// converts a excess-128 value into a float.
//
double ux_convertfac(byte exp, byte m1, byte m2, byte m3, byte m4) {

	double rval;
	double dm1,dm2,dm3,dm4;
	exp = exp - 128;
	dm1 = m1 * pow(2,-8);
	dm2 = m2 * pow(2,-16);
	dm3 = m3 * pow(2,-24);
	dm4 = m4 * pow(2,-32);
	rval = pow(2,exp) * (dm1 + dm2 + dm3 + dm4);

	return rval;
}

void refresh_registers(UX * ux) {

	byte status = cpu_getstatus();

	wattron(ux->registers,COLOR_PAIR(1));
	werase(ux->registers);	
	wmove(ux->registers,0,0);
	wprintw(ux->registers,
		"A: $%02X X: $%02X Y: $%02X STACK: $%02X PC: $%04X N:%dV%dX%dB%dD%dI%dZ%dC%d ticks: %010d (%d c64 secs)",
		cpu_geta(),cpu_getx(),cpu_gety(),cpu_getstack(),cpu_getpc(),
		((status & N_FLAG )!= 0),
		((status & V_FLAG )!= 0),
		((status & X_FLAG )!= 0),
		((status & B_FLAG )!= 0),
		((status & D_FLAG )!= 0),
		((status & I_FLAG )!= 0),
		((status & Z_FLAG )!= 0),
		((status & C_FLAG )!= 0),
		sysclock_getticks(),
		sysclock_getticks() / NTSC_TICKS_PER_SECOND );

	wrefresh(ux->registers);
	wattroff(ux->registers,COLOR_PAIR(1));
}




void refresh_console(UX * ux) {

	int ch = getch();
	int i;

	wattron(ux->console,COLOR_PAIR(1));

	if (ch != -1 && ux->passthru) {
		if (ch == 27) {
			ux->passthru = false;
			getch();getch();
		}
		else { 
			for (i = 0; i < 10; i++) {
				if (g_keyqueue[i].t == -1) {
					cia1_keydown(toupper(ch));
					g_keyqueue[i].t = clock();
					g_keyqueue[i].ch =toupper(ch);
					break;
				}
			}
		}
	}
	else {
		switch(ch) {
			case 27:
				handle_step(ux); 
				//
				// BUGBUG on my keyboard, key down returns three keys.
				//
				getch();getch();

			break;
			case -1:
			break;
			case '\n':
				handle_command(ux);
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
	}
	werase(ux->console);
	wmove(ux->console,0,0);
	wprintw(ux->console,":> %s", ux->buf);
	wrefresh(ux->console);

	wattroff(ux->console,COLOR_PAIR(1));


	for (i = 0; i < 10; i++) {
		if (g_keyqueue[i].t != -1 && (clock() - g_keyqueue[i].t)/CLOCKS_PER_SEC > .1) {
			g_keyqueue[i].t = -1;
			cia1_keyup(g_keyqueue[i].ch);
		}
	}

}


void update_ux(UX *ux) {
	refresh(); // curses refresh
	refresh_memory(ux);
	refresh_registers(ux);
	refresh_console(ux);
	refresh_display(ux);
	refresh_disassembly(ux);
}
