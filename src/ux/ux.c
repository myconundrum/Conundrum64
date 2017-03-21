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
MODULE: ux.c
SDL based ux source.


WORK ITEMS:

KNOWN BUGS:

*/

#include "emu.h"
#include "cpu.h"
#include "ux.h"
#include "vicii.h"
#include "sysclock.h"
#include "c64kbd.h"

typedef struct {
	word address;
	char buf[32];
} DISLINE;



#define DISLINESCOUNT 16


typedef struct {

	byte r;
	byte g;
	byte b;
	byte a;

} VICII_COLOR;


/*
	RGB values of C64 colors from 
	http://unusedino.de/ec64/technical/misc/vic656x/colors/

	0 black
  	1 white
  	2 red
  	3 cyan
  	4 pink
  	5 green
  	6 blue
  	7 yellow
  	8 orange
  	9 brown
 	10 light red
 	11 dark gray
 	12 medium gray
 	13 light green
 	14 light blue
 	15 light gray
*/

VICII_COLOR g_colors[0x10] = {
	{0x00, 0x00, 0x00, 0xFF},
	{0xFF, 0xFF, 0xFF, 0xFF},
	{0x68, 0x37, 0x2B, 0xFF},
	{0x70, 0xA4, 0xB2, 0xFF},
	{0x6F, 0x3D, 0x86, 0xFF},
	{0x58, 0x8D, 0x43, 0xFF},
	{0x35, 0x28, 0x79, 0xFF},
	{0xB8, 0xC7, 0x6F, 0xFF},
	{0x6F, 0x4F, 0x25, 0xFF},
	{0x43, 0x39, 0x00, 0xFF},
	{0x9A, 0x67, 0x59, 0xFF},
	{0x44, 0x44, 0x44, 0xFF},
	{0x6C, 0x6C, 0x6C, 0xFF},
	{0x9A, 0xD2, 0x84, 0xFF},
	{0x6C, 0x5E, 0xB5, 0xFF},
	{0x95, 0x95, 0x95, 0xFF}
};

typedef struct {

	SDL_Window 		* window;
	FC_Font  		* font;
	SDL_Renderer    * renderer;
	SDL_Texture 	* texture;
	int  			  id;

} UX_WINDOW;

typedef struct {


	//
	// window structures for each screen in the emulator.
	//
	UX_WINDOW 		mon;
	UX_WINDOW 		screen;

    //
    // UX state
    //
	byte 			curpage;					// current page of memory in monitor.
	DISLINE			dislines[DISLINESCOUNT];	// disassembled lines.
	byte 			discur;						// current index of disassembly lines.
	
	char 			buf[256];					// captures keyboard input
	int 			bpos;						// keyboard input buffer position

	word			brk_address;				// breakpoint address if brk is set
	bool			brk; 						// look for breakpoints.
	
	bool 			running;					// c64 is running
	bool 			done;						// user wishes to quit when true
	bool			passthru;					// send input to c64 if true, otherwise monitor
	
	int 			cycles;						// track ux cycles (used for rendering perf)

	char        	nameString
	
} UX;

UX g_ux;


void ux_handleKeyPress(SDL_Event);

void ux_fillDisassembly(word address) {

	int i;
	g_ux.discur = 0;

	for (i =0 ; i < DISLINESCOUNT; i++) {
		g_ux.dislines[i].address = address;
		address += cpu_disassemble(g_ux.dislines[i].buf,address);
	}
}
void ux_handlestep() {

	c64_update();
	g_ux.discur++;
	if (cpu_getpc() != g_ux.dislines[g_ux.discur].address || g_ux.discur == DISLINESCOUNT) {
		ux_fillDisassembly(cpu_getpc());
	}
}

bool ux_running() {return g_ux.running;}
bool ux_done() {return g_ux.done;}
void ux_startemulator(){g_ux.running = true;}

void ux_handlecommand() {

	char * p = NULL;
	word address;	
		
	p = strtok(g_ux.buf," ");
	if (!p) {
		p = g_ux.buf;
	}

	if (!strcmp(p,"QUIT")) {
		g_ux.done = true;
	} else if (!strcmp(p,"EXEC")) {
		g_ux.running = true;
	} else if (!strcmp(p,"STOP")) {
		g_ux.running = false;
	} else if (!strcmp(p,"DIS")) {
 		p = strtok(NULL," ");
 		address = p ? strtoul(p,NULL,16) : cpu_getpc();
		ux_fillDisassembly(address);	
		g_ux.running = false;
	} else if (!strcmp(p,"BRK")) {
		p = strtok(NULL," ");
		if (p) {
			g_ux.brk = true;
			g_ux.brk_address = strtoul(p,NULL,16);
		}
		else {
			g_ux.brk = false;
			g_ux.brk_address = 0;
		} 		
	} else if (!strcmp(p,"MEM")) {
		p = strtok(NULL," ");
		if (p) {
			g_ux.curpage = strtoul(p,NULL,16);
		}
	}
}


ux_init_monitor() {

	char buf[256];

	sprintf(buf,"%s Monitor",emu_getname());

	g_ux.mon.window 	= SDL_CreateWindow (buf, 
    	SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, MON_SCREEN_WIDTH, MON_SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
	g_ux.mon.renderer 	= SDL_CreateRenderer(g_ux.mon.window, -1, SDL_RENDERER_ACCELERATED);
	g_ux.mon.font 		= FC_CreateFont();
   	g_ux.mon.id 		= SDL_GetWindowID(g_ux.mon.window);

   	SDL_RenderSetLogicalSize (g_ux.mon.renderer, MON_SCREEN_WIDTH, MON_SCREEN_HEIGHT);
	SDL_SetRenderDrawColor (g_ux.mon.renderer, 0, 0, 0, 255);
	FC_LoadFont(g_ux.mon.font, g_ux.mon.renderer, 
		"/Library/Fonts/Andale Mono.ttf", 16, FC_MakeColor(255,255,255,255), TTF_STYLE_NORMAL);	

	//
	// BUGBUG: When the window is created above with SDL_WINDOW_SHOWN something doesn't init right and
	// when you later reveal the window it contains garbage. 
	// Therefore, currently creating the window and then immediately hiding. Seems to work. b
	//
	SDL_HideWindow(g_ux.mon.window);

}

ux_init_screen() {

	int width = VICII_SCREENFRAME_WIDTH;
	int height = VICII_SCREENFRAME_HEIGHT;

#ifdef EMU_DOUBLE_SCREEN
	width *=2;
	height *=2;
#endif

	g_ux.screen.window =  SDL_CreateWindow (emu_getname(), 
    	SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, SDL_WINDOW_SHOWN);
   	g_ux.screen.renderer = SDL_CreateRenderer(g_ux.screen.window, -1, 0);
   	g_ux.screen.texture = SDL_CreateTexture(g_ux.screen.renderer, SDL_PIXELFORMAT_ARGB8888, 
   		SDL_TEXTUREACCESS_STREAMING, width, height);
   	g_ux.screen.id = SDL_GetWindowID(g_ux.screen.window);
}

void ux_init() {

	char buf[255];

	memset(&g_ux,0,sizeof(UX));

	ux_init_monitor();
	ux_init_screen();
	ux_fillDisassembly(cpu_getpc());

	g_ux.passthru = true;

}

void ux_destroy() {

	FC_FreeFont(g_ux.mon.font);
    SDL_DestroyWindow (g_ux.mon.window);
    SDL_DestroyWindow (g_ux.screen.window);
    SDL_Quit();
}

void ux_updateMemory() {

	int i;
	int j; 
	char buf[255];

	SDL_Rect r = {MON_SCREEN_WIDTH - 540,40,54*10,16*20};
	SDL_RenderDrawRect(g_ux.mon.renderer,&r);
	
	for (int i = 0; i < 16; i++) {
		sprintf(buf,"%02X:%02X:",g_ux.curpage,i*16);
		for (j = 0; j < 16; j++) {
			sprintf(buf+6+(j*3)," %02X",
				mem_peek((g_ux.curpage << 8) | (i*16+j)));
		}
		FC_Draw(g_ux.mon.font,g_ux.mon.renderer,MON_SCREEN_WIDTH - 540,40+i*20,buf);
	}
}

void ux_updateRegisters() {

	byte status = cpu_getstatus();
	SDL_Rect r = {0,0,MON_SCREEN_WIDTH,40};
	SDL_Color c = FC_MakeColor(102,255,51,255);

	SDL_RenderDrawRect(g_ux.mon.renderer,&r);
	FC_Draw (g_ux.mon.font, g_ux.mon.renderer, 0, 0, 
		" A:      X:      Y:      Stack:      PC:        Ticks:             Secs: ");
	FC_Draw(g_ux.mon.font,g_ux.mon.renderer,0,20,"[N]  [V]  [X]  [B]  [D]  [I]  [Z]  [C]  ");

	FC_DrawColor (g_ux.mon.font, g_ux.mon.renderer, 0, 0, c,
		"   $%02X     $%02X     $%02X         $%02X      $%04X         %010d        %d",
		cpu_geta(),cpu_getx(),cpu_gety(),cpu_getstack(),cpu_getpc(),
		sysclock_getticks(),
		sysclock_getticks() / NTSC_TICKS_PER_SECOND);	

	FC_DrawColor(g_ux.mon.font,g_ux.mon.renderer,0,20,c,"   %d    %d    %d    %d    %d    %d    %d    %d ",
		((status & N_FLAG )!= 0),
		((status & V_FLAG )!= 0),
		((status & X_FLAG )!= 0),
		((status & B_FLAG )!= 0),
		((status & D_FLAG )!= 0),
		((status & I_FLAG )!= 0),
		((status & Z_FLAG )!= 0),
		((status & C_FLAG )!= 0)
		);
}

void ux_updateDisassembly() {
 	
	int i;
	SDL_Rect r = {0,40,260,16*20};
	SDL_RenderDrawRect(g_ux.mon.renderer,&r);

	for (i = 0;i<DISLINESCOUNT;i++) {
		if (g_ux.discur == i) {
			FC_DrawColor(g_ux.mon.font,g_ux.mon.renderer,0,40+i*20,FC_MakeColor(102,255,51,255),"$%04X:    %s\n",
				g_ux.dislines[i].address,g_ux.dislines[i].buf);
		} else {
			FC_Draw(g_ux.mon.font,g_ux.mon.renderer,0,40+i*20,"$%04X:    %s\n",
				g_ux.dislines[i].address,g_ux.dislines[i].buf);
		}
	}
}


void ux_updateConsole() {

	SDL_Rect r = {0,360,MON_SCREEN_WIDTH,20};
	SDL_RenderDrawRect(g_ux.mon.renderer,&r);
	FC_Draw(g_ux.mon.font,g_ux.mon.renderer,0,360,g_ux.buf);

}


void ux_updateScreen() {

	VICII_SCREENFRAME *frame = vicii_getframe();
	void * pixels;
	int    pitch;
	int 	row;
	int 	col;
	Uint32 * dst;
	Uint32  val;
#ifdef EMU_DOUBLE_SCREEN
	Uint32 * dst2;
#endif 

	if (SDL_LockTexture(g_ux.screen.texture, NULL, &pixels, &pitch) <0) {
		DEBUG_PRINT("can't lock texure %s!\n",SDL_GetError());
		return;
	}

	for (row = 0 ; row < VICII_SCREENFRAME_HEIGHT ; row++) {

#ifdef EMU_DOUBLE_SCREEN
		dst = (Uint32*) ((Uint8 *)pixels + (row*2) * pitch);
		dst2 = (Uint32*) ((Uint8 *)pixels + (row*2+1) * pitch);
#else 
		dst = (Uint32*) ((Uint8 *)pixels + (row) * pitch);
#endif 

		for (col = 0; col < VICII_SCREENFRAME_WIDTH; col++) {

			val = (
				(g_colors[frame->data[row][col]].a << 24)|
				(g_colors[frame->data[row][col]].r << 16) | 
				(g_colors[frame->data[row][col]].g << 8) |
				(g_colors[frame->data[row][col]].b));
			
			*dst++ = val;
#ifdef EMU_DOUBLE_SCREEN
			*dst++ = val;
			*dst2++ = val;
			*dst2++ = val;
#endif 

		}
	}

	SDL_UnlockTexture(g_ux.screen.texture);
	SDL_RenderClear(g_ux.screen.renderer);
	SDL_RenderCopy(g_ux.screen.renderer, g_ux.screen.texture, NULL, NULL);
	SDL_RenderPresent(g_ux.screen.renderer);
}

bool ux_allWindowsClosed() {

	return ((SDL_GetWindowFlags(g_ux.mon.window) & SDL_WINDOW_HIDDEN) &&
	(SDL_GetWindowFlags(g_ux.screen.window) & SDL_WINDOW_HIDDEN));

}

void ux_handleWindowEvent(SDL_Event * e) {

	switch (e->window.event) {
		case SDL_WINDOWEVENT_CLOSE:
			if (e->window.windowID == g_ux.mon.id) {
				SDL_HideWindow(g_ux.mon.window);
			} else if (e->window.windowID == g_ux.screen.id) {
				SDL_HideWindow(g_ux.screen.window);
			}
		break;
		default:
		break;
	}
}

void ux_updateMonitorWindow() {

	if (SDL_GetWindowFlags(g_ux.mon.window) & SDL_WINDOW_SHOWN) {
		
		SDL_SetRenderDrawColor (g_ux.mon.renderer, 0, 0, 0, 255);
		SDL_RenderClear(g_ux.mon.renderer);
		SDL_SetRenderDrawColor(g_ux.mon.renderer,255,255,255,255);
		ux_updateRegisters();
		ux_updateMemory();
		ux_updateDisassembly();
		ux_updateConsole();
		SDL_RenderPresent(g_ux.mon.renderer);
	}
}

void ux_updateScreenWindow() {

	if (SDL_GetWindowFlags(g_ux.screen.window) & SDL_WINDOW_SHOWN) {
		ux_updateScreen();
	}
}

void ux_update() {

	char ch;

	if (ux_running()) {
		if (g_ux.brk && cpu_getpc() == g_ux.brk_address) {
			g_ux.running = false;
			g_ux.brk = false;
			g_ux.passthru = false;
			ux_fillDisassembly(cpu_getpc());
		}
	}

	if (g_ux.cycles++ % 10000) {
		return;
	}

	SDL_Event e;

	ux_updateScreenWindow();
	ux_updateMonitorWindow();

	while (SDL_PollEvent (&e)) {
	
		switch (e.type) {
			case SDL_WINDOWEVENT:
				ux_handleWindowEvent(&e);
				break;
			case SDL_QUIT: 
				g_ux.done = true;
				break;
      		case SDL_KEYDOWN:
      			ux_handleKeyPress(e);
        	break;
      		case SDL_KEYUP:
      			ux_handleKeyPress(e);
      		break;
      		default:
        	break;
    	}
	}

	//
	// if all windows have been closed, exit.
	//
	if (ux_allWindowsClosed()) {
		g_ux.done = true;
	}
}
//
// bugbug not handling shift keys yet.
//

char ux_getasciich(SDL_Keycode code) {
	
	char ch = 0;

	switch(code) {
		case SDLK_a: ch = 'A';break;
		case SDLK_b: ch = 'B';break;
		case SDLK_c: ch = 'C';break;
		case SDLK_d: ch = 'D';break;
		case SDLK_e: ch = 'E';break;
		case SDLK_f: ch = 'F';break;
		case SDLK_g: ch = 'G';break;
		case SDLK_h: ch = 'H';break;
		case SDLK_i: ch = 'I';break;
		case SDLK_j: ch = 'J';break;
		case SDLK_k: ch = 'K';break;
		case SDLK_l: ch = 'L';break;
		case SDLK_m: ch = 'M';break;
		case SDLK_n: ch = 'N';break;
		case SDLK_o: ch = 'O';break;
		case SDLK_p: ch = 'P';break;
		case SDLK_q: ch = 'Q';break;
		case SDLK_r: ch = 'R';break;
		case SDLK_s: ch = 'S';break;
		case SDLK_t: ch = 'T';break;
		case SDLK_u: ch = 'U';break;
		case SDLK_v: ch = 'V';break;
		case SDLK_w: ch = 'W';break;
		case SDLK_x: ch = 'X';break;
		case SDLK_y: ch = 'Y';break;
		case SDLK_z: ch = 'Z';break;
		case SDLK_0: ch = '0';break;
		case SDLK_1: ch = '1';break;
		case SDLK_2: ch = '2';break;
		case SDLK_3: ch = '3';break;
		case SDLK_4: ch = '4';break;
		case SDLK_5: ch = '5';break;
		case SDLK_6: ch = '6';break;
		case SDLK_7: ch = '7';break;
		case SDLK_8: ch = '8';break;
		case SDLK_9: ch = '9';break;
		case SDLK_SPACE: ch = ' ';break;
		case SDLK_QUOTE: ch = '\'';break;
		case SDLK_BACKSLASH: ch = '\\';break;
		case SDLK_COMMA: ch = ',';break;
		case SDLK_EQUALS: ch = '=';break;
		case SDLK_BACKQUOTE: ch = '`';break;
		case SDLK_LEFTBRACKET: ch = '[';break;
		case SDLK_RIGHTBRACKET: ch = ']';break;
		case SDLK_LEFTPAREN: ch = '(';break;
		case SDLK_MINUS: ch = '-';break;
		case SDLK_PERIOD: ch = '.';break;
		case SDLK_SEMICOLON: ch = ';';break;
		case SDLK_SLASH: ch = '/';break;
		case SDLK_RETURN: ch = '\n';break;
		default:break;
	}
	return ch;
}

void ux_handlec64key(SDL_Event e) {


	char ch = ux_getasciich(e.key.keysym.sym);
	char oldch;
	if (!ch) {
		switch(e.key.keysym.sym) {

		case SDLK_RIGHT: 		ch = C64KEY_CURRIGHT;break;
		case SDLK_DOWN:  		ch = C64KEY_CURDOWN;break;
		case SDLK_LEFT:  		ch = C64KEY_CURRIGHT;break;
		case SDLK_UP:    		ch = C64KEY_CURUP;break;
		case SDLK_LCTRL: 		ch = C64KEY_CTRL;break;
		case SDLK_LSHIFT: 		ch = C64KEY_LSHIFT;break;
		case SDLK_RSHIFT: 		ch = C64KEY_RSHIFT;break;	
		case SDLK_BACKSPACE: 	ch = C64KEY_DELETE;break;	
		default:break;
		}
	}	

	if (e.key.keysym.mod == KMOD_RSHIFT || e.key.keysym.mod ==KMOD_LSHIFT) {
		oldch = ch;
		switch(oldch) {
			case '\'': ch = '2';break;
			default:break;
		}
	}

	if (e.type == SDL_KEYDOWN) {
		c64kbd_keydown(ch);
	} else {
		c64kbd_keyup(ch);
	}
}

//
// switch input from monitor to c64 and back, also, bring up monitor if it is not displayed
//
void ux_handleMetaCommands(SDL_Event e) {

	if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) {

		if (e.key.keysym.mod == KMOD_RSHIFT || e.key.keysym.mod ==KMOD_LSHIFT) {
			SDL_ShowWindow(g_ux.mon.window);
		}
		else {
			memset(g_ux.buf,0,256);
			g_ux.bpos = 0;	
			g_ux.passthru = !g_ux.passthru;
		}
	}
}

void ux_handleKeyPress(SDL_Event e) {

	//
	// check to see if we are switching modes between monitor and c64.
	//
	ux_handleMetaCommands(e);

	if (g_ux.passthru) {
		ux_handlec64key(e);
		return;
	}

	if (e.type == SDL_KEYUP) {
		return;
	}
	
	char ch = ux_getasciich(e.key.keysym.sym);

	if (!ch) {
		switch (e.key.keysym.sym) {
			case SDLK_DOWN:					// step one instruction
				ux_handlestep();
			break;
			case SDLK_RIGHT:				// step over JSR
				//
				// set break point after jsr and run.
				//
				g_ux.brk = true;
				g_ux.brk_address = cpu_getpc() + 3;
				g_ux.running = true;
			break;
			default: break;
		}
	}
	else if (ch == '\n') {
		ux_handlecommand();
		memset(g_ux.buf,0,256);
		g_ux.bpos = 0;
	}
	else {
		g_ux.buf[g_ux.bpos++] = ch;
	}
}
