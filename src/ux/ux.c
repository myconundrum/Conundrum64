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
#include "joystick.h"



typedef struct {
	bool shift;
	bool control;
	char ch;
} UX_C64KEYSTATE;

UX_C64KEYSTATE g_c64keymapping[2000] = {0};



typedef struct {
	word address;
	char buf[32];
} DISLINE;



#define DISLINESCOUNT 16


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

	bool 			joyon;						// joystick mode (vs keyboard mode)
	byte			joyport;						// which joystick port (0 or 1)
	
	int 			cycles;						// track ux cycles (used for rendering perf)

	char        	nameString;

	bool 			deferredinit;
	
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

//
// BUGBUG: Terrible, terrible hack to deal with big numbers in SDLK enum.
//
word ux_getcompressedkey(unsigned int key) {

	if (key > 0xFF) {
		key = 0x100 | (key & 0xFF);
	}
	return key;
}

void ux_mapkey(unsigned int key, char c64key, bool shift, bool control) {

	key = ux_getcompressedkey(key);

	g_c64keymapping[key].ch = c64key;
	g_c64keymapping[key].shift = shift;
	g_c64keymapping[key].control = control;
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
	} else if (!strcmp(p,"BAS")) {
		p = strtok(NULL," ");
		if (p) {
			bas_loadfile(p);
		}
	}  else if (!strcmp(p,"ASM")) {
		p = strtok(NULL," ");
		if (p) {
			asm_loadfile(p);
		}
	} else if (!strcmp(p,"JOY")) {
		p = strtok(NULL," ");
		if (p) {
			g_ux.joyport = atoi(p);	
		} else {
			g_ux.joyport = 0;
		}
		g_ux.joyon 	= true;
	} else if (!strcmp(p,"KEY")) {
		g_ux.joyon 	= false;

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

void ux_init_c64keymapping() {

	int i; 
	for (i = 0; i < 2000; i++) {
		ux_mapkey(i,C64KEY_UNUSED,false,false);
	}

	// unshifted keys
	ux_mapkey(SDLK_BACKQUOTE,C64KEY_RUNSTOP,false,false); 
	ux_mapkey(SDLK_SLASH,'/',false,false);
	ux_mapkey(SDLK_COMMA,',',false,false);
	ux_mapkey(SDLK_n,'N',false,false);
	ux_mapkey(SDLK_v,'V',false,false);
	ux_mapkey(SDLK_x,'X',false,false);
	ux_mapkey(SDLK_LSHIFT,C64KEY_LSHIFT,false,false);
	ux_mapkey(SDLK_DOWN,C64KEY_CURDOWN,false,false);

	ux_mapkey(SDLK_q,'Q',false,false); 
	ux_mapkey(SDLK_CARET,'^',false,false);
	ux_mapkey(SDLK_AT,'@',false,false);
	ux_mapkey(SDLK_o,'O',false,false);
	ux_mapkey(SDLK_u,'U',false,false);
	ux_mapkey(SDLK_t,'T',false,false);
	ux_mapkey(SDLK_e,'E',false,false);
	ux_mapkey(SDLK_F5,C64KEY_F5,false,false);

	ux_mapkey(SDLK_LALT,C64KEY_C64,false,false); 
	ux_mapkey(SDLK_EQUALS,'=',false,false);
	ux_mapkey(SDLK_COLON,':',false,false);
	ux_mapkey(SDLK_k,'K',false,false);
	ux_mapkey(SDLK_h,'H',false,false);
	ux_mapkey(SDLK_f,'F',false,false);
	ux_mapkey(SDLK_s,'S',false,false);
	ux_mapkey(SDLK_F3,C64KEY_F3,false,false);

	ux_mapkey(SDLK_SPACE,' ',false,false); 
	ux_mapkey(SDLK_RSHIFT,C64KEY_RSHIFT,false,false);
	ux_mapkey(SDLK_PERIOD,'.',false,false);
	ux_mapkey(SDLK_m,'M',false,false);
	ux_mapkey(SDLK_b,'B',false,false);
	ux_mapkey(SDLK_c,'C',false,false);
	ux_mapkey(SDLK_z,'Z',false,false);
	ux_mapkey(SDLK_F1,C64KEY_F1,false,false);

	ux_mapkey(SDLK_2,'2',false,false); 
	ux_mapkey(SDLK_HOME,C64KEY_HOME,false,false);
	ux_mapkey(SDLK_MINUS,'-',false,false);
	ux_mapkey(SDLK_0,'0',false,false);
	ux_mapkey(SDLK_8,'8',false,false);
	ux_mapkey(SDLK_6,'6',false,false);
	ux_mapkey(SDLK_4,'4',false,false);
	ux_mapkey(SDLK_F7,C64KEY_F7,false,false);

	ux_mapkey(SDLK_LCTRL,C64KEY_CTRL,false,false);
	ux_mapkey(SDLK_SEMICOLON,';',false,false);
	ux_mapkey(SDLK_l,'L',false,false);
	ux_mapkey(SDLK_j,'J',false,false);
	ux_mapkey(SDLK_g,'G',false,false);
	ux_mapkey(SDLK_d,'D',false,false);
	ux_mapkey(SDLK_a,'A',false,false);
	ux_mapkey(SDLK_RIGHT,C64KEY_CURRIGHT,false,false);

	ux_mapkey(SDLK_BACKSPACE,C64KEY_BACK,false,false); 
	ux_mapkey(SDLK_ASTERISK,'*',false,false);
	ux_mapkey(SDLK_p,'P',false,false);
	ux_mapkey(SDLK_i,'I',false,false);
	ux_mapkey(SDLK_y,'Y',false,false);
	ux_mapkey(SDLK_r,'R',false,false);
	ux_mapkey(SDLK_w,'W',false,false);
	ux_mapkey(SDLK_RETURN,'\n',false,false);

	ux_mapkey(SDLK_1,'1',false,false); 
	// unmapped. ux_mapkey(C64KEY_POUND,false,false);
	ux_mapkey(SDLK_PLUS,'+',false,false);
	ux_mapkey(SDLK_9,'9',false,false);
	ux_mapkey(SDLK_7,'7',false,false);
	ux_mapkey(SDLK_5,'5',false,false);
	ux_mapkey(SDLK_3,'3',false,false);
	ux_mapkey(SDLK_BACKSPACE,C64KEY_DELETE,false,false);

	// shifted keys
	ux_mapkey(SDLK_F12,C64KEY_RUNSTOP,true,false); 
	ux_mapkey(SDLK_QUESTION,'/',true,false);
	ux_mapkey(SDLK_LESS,',',true,false);
	//ux_mapkey(SDLK_n,'N',true,false);  GFX
	//ux_mapkey(SDLK_v,'V',true,false);  GFX
	//ux_mapkey(SDLK_x,'X',true,false);  GFX
	//ux_mapkey(SDLK_LSHIFT,C64KEY_LSHIFT,true,false);
	//ux_mapkey(SDLK_DOWN,C64KEY_CURDOWN,true,false);

	//ux_mapkey(SDLK_q,'Q',true,false); 
	//ux_mapkey(SDLK_CARET,'^',true,false);
	//ux_mapkey(SDLK_AT,'@',true,false);
	//ux_mapkey(SDLK_o,'O',true,false);
	//ux_mapkey(SDLK_u,'U',true,false);
	//ux_mapkey(SDLK_t,'T',true,false);
	//ux_mapkey(SDLK_e,'E',true,false);
	ux_mapkey(SDLK_F6,C64KEY_F5,true,false);

	//ux_mapkey(SDLK_LALT,C64KEY_C64,true,false); 
	//ux_mapkey(SDLK_EQUALS,'=',true,false);
	ux_mapkey(SDLK_LEFTBRACKET,':',true,false);
	//ux_mapkey(SDLK_k,'K',true,false);
	//ux_mapkey(SDLK_h,'H',true,false);
	//ux_mapkey(SDLK_f,'F',true,false);
	//ux_mapkey(SDLK_s,'S',true,false);
	ux_mapkey(SDLK_F4,C64KEY_F3,true,false);

	//ux_mapkey(SDLK_SPACE,' ',true,false); 
	//ux_mapkey(SDLK_RSHIFT,C64KEY_RSHIFT,true,false);
	ux_mapkey(SDLK_GREATER,'.',true,false);
	//ux_mapkey(SDLK_m,'M',true,false);
	//ux_mapkey(SDLK_b,'B',true,false);
	//ux_mapkey(SDLK_c,'C',true,false);
	//ux_mapkey(SDLK_z,'Z',true,false);
	ux_mapkey(SDLK_F2,C64KEY_F1,true,false);

	ux_mapkey(SDLK_QUOTEDBL,'2',true,false); 
	//ux_mapkey(SDLK_HOME,C64KEY_HOME,true,false);
	//ux_mapkey(SDLK_MINUS,'-',true,false);
	//ux_mapkey(SDLK_0,'0',true,false);
	ux_mapkey(SDLK_LEFTPAREN,'8',true,false);
	ux_mapkey(SDLK_AMPERSAND,'6',true,false);
	ux_mapkey(SDLK_DOLLAR,'4',true,false);
	ux_mapkey(SDLK_F8,C64KEY_F7,true,false);

	//ux_mapkey(SDLK_LCTRL,C64KEY_CTRL,true,false);
	ux_mapkey(SDLK_RIGHTBRACKET,';',true,false);
	//ux_mapkey(SDLK_l,'L',true,false);
	//ux_mapkey(SDLK_j,'J',true,false);
	//ux_mapkey(SDLK_g,'G',true,false);
	//ux_mapkey(SDLK_d,'D',true,false);
	//ux_mapkey(SDLK_a,'A',true,false);
	ux_mapkey(SDLK_LEFT,C64KEY_CURRIGHT,true,false);

	//ux_mapkey(SDLK_BACKSPACE,C64KEY_BACK,true,false); 
	//ux_mapkey(SDLK_ASTERISK,'*',true,false);
	//ux_mapkey(SDLK_p,'P',true,false);
	//ux_mapkey(SDLK_i,'I',true,false);
	//ux_mapkey(SDLK_y,'Y',true,false);
	//ux_mapkey(SDLK_r,'R',true,false);
	//ux_mapkey(SDLK_w,'W',true,false);
	//ux_mapkey(SDLK_RETURN,'\n',true,false);

	ux_mapkey(SDLK_EXCLAIM,'1',true,false); 
	// unmapped. ux_mapkey(C64KEY_POUND,true,false);
	//ux_mapkey(SDLK_PLUS,'+',true,false);
	ux_mapkey(SDLK_RIGHTPAREN,'9',true,false);
	ux_mapkey(SDLK_QUOTE,'7',true,false);
	ux_mapkey(SDLK_PERCENT,'5',true,false);
	ux_mapkey(SDLK_HASH,'3',true,false);
	//ux_mapkey(SDLK_DELETE,C64KEY_DELETE,true,false);
}

void ux_deferredinit() {

	EMU_CONFIGURATION *cfg = emu_getconfig();
	
	if (cfg->binload != NULL) {
		asm_loadfile(cfg->binload);
	}


	g_ux.deferredinit = false;
}


void ux_init() {

	char buf[255];
	EMU_CONFIGURATION *cfg = emu_getconfig();

	memset(&g_ux,0,sizeof(UX));

	g_ux.deferredinit = true; // initialization hook post C64 bootup.

	ux_init_monitor();
	ux_init_screen();	
	ux_init_c64keymapping();

	ux_fillDisassembly(cpu_getpc());

	if (cfg->cartload != NULL) {
		asm_loadcart(cfg->cartload);
	}

	if (cfg->breakpoint != 0) {
		g_ux.brk = true;
		g_ux.brk_address = cfg->breakpoint;

	}
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

			*dst++ = frame->data[row][col];
#ifdef EMU_DOUBLE_SCREEN
			*dst++ = frame->data[row][col];
			*dst2++ = frame->data[row][col];
			*dst2++ = frame->data[row][col];
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


void ux_handleevents() {

	SDL_Event e;
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
}

void ux_update() {

	

	if (ux_running()) {
		
		if (g_ux.deferredinit && cpu_getpc() == 0xA480) { // basic warm start. 
			ux_deferredinit();
		}

		if (g_ux.brk && cpu_getpc() == g_ux.brk_address) {
			g_ux.running = false;
			g_ux.brk = false;
			g_ux.passthru = false;
			ux_fillDisassembly(cpu_getpc());
		}

		if (vicii_frameready()) {
			ux_handleevents();
				ux_updateScreenWindow();
			
			
		}
	}

	if (g_ux.cycles++ % 1000 == 0) {

		if (!ux_running()) {
			ux_updateScreenWindow();
			
		}
		ux_updateMonitorWindow();
		ux_handleevents();

		//
		// if all windows have been closed, exit.
		//
		if (ux_allWindowsClosed()) {
			g_ux.done = true;
		}
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


/*
BUGBUG: Terrible Hack. Should move to textinput events to get correct mapping.
*/ 
unsigned int ux_checkshiftedkey(SDL_Event e) {

	unsigned int key = e.key.keysym.sym;

	if (e.key.keysym.mod & KMOD_SHIFT) {
		switch(key) {
			case SDLK_1: 			key = SDLK_EXCLAIM;				break;
			case SDLK_2: 			key = SDLK_AT;					break;
			case SDLK_3: 			key = SDLK_HASH;				break;
			case SDLK_4: 			key = SDLK_DOLLAR;				break;
			case SDLK_5:	 		key = SDLK_PERCENT;				break;
			case SDLK_6: 			key = SDLK_CARET;				break;
			case SDLK_7: 			key = SDLK_AMPERSAND;			break;
			case SDLK_8: 			key = SDLK_ASTERISK;			break;
			case SDLK_9: 			key = SDLK_LEFTPAREN;			break;
			case SDLK_0: 			key = SDLK_RIGHTPAREN;			break;
			case SDLK_MINUS: 		key = SDLK_UNDERSCORE;			break;
			case SDLK_EQUALS: 		key = SDLK_PLUS;				break;
			case SDLK_SEMICOLON: 	key = SDLK_COLON;				break;
			case SDLK_QUOTE: 		key = SDLK_QUOTEDBL;			break;
			case SDLK_COMMA: 		key = SDLK_LESS;				break;
			case SDLK_PERIOD: 		key = SDLK_GREATER;				break;
			case SDLK_BACKSLASH: 	key = SDLK_QUESTION;			break;
			
			default:
			break;
		}
	}

	return key;
}

bool g_pressed = false;
void ux_handlec64joystick(SDL_Event e) {

	unsigned int key = e.key.keysym.sym;
	byte input = 0;
	switch(key) {

		case SDLK_w: input = JOY_UP; 				break;
		case SDLK_a: input = JOY_LEFT; 				break;
		case SDLK_s: input = JOY_DOWN; 				break;
		case SDLK_d: input = JOY_RIGHT; 			break;
		case SDLK_q: input = JOY_UP    | JOY_LEFT; 	break;
		case SDLK_e: input = JOY_UP    | JOY_RIGHT; break;
		case SDLK_z: input = JOY_DOWN  | JOY_LEFT; 	break;
		case SDLK_c: input = JOY_DOWN  | JOY_RIGHT; break;
		case SDLK_SPACE: input = JOY_FIRE; 			break;
	}

	joy_input(g_ux.joyport,input,e.type == SDL_KEYDOWN);	
}


void ux_handlec64key(SDL_Event e) {

	unsigned int key = e.key.keysym.sym;

	switch(key) {

		//
		// ignoring shifts and ctrl for now. These are simulated. 
		//
		case SDLK_LCTRL: break;
		case SDLK_LSHIFT: break;
		case SDLK_RSHIFT: break;
		default: 
			key = ux_checkshiftedkey(e);
			key = ux_getcompressedkey(key);
			if (g_c64keymapping[key].ch != C64KEY_UNUSED) {

				if (e.type == SDL_KEYDOWN) {

					if (g_c64keymapping[key].shift) {
						c64kbd_keydown(C64KEY_LSHIFT);
					}
					if (g_c64keymapping[key].control) {
						c64kbd_keydown(C64KEY_CTRL);
					}
					c64kbd_keydown(g_c64keymapping[key].ch);
				} else {
					/*
					
					if (g_c64keymapping[key].shift) {
						c64kbd_keyup(C64KEY_LSHIFT);
					}
					if (g_c64keymapping[key].control) {
						c64kbd_keyup(C64KEY_CTRL);
					}
					c64kbd_keyup(g_c64keymapping[key].ch);
					*/
					//
					// BUGBUG: terrible hack. But SDL keeps losing shift state every once and a while, and it 
					// kills the C64 keyboard. Fix this.
					//
					c64kbd_reset();
				}
			}
		break;
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

	//
	// BUGBUG: Hack to toggle joystick and keyboard mode.
	//
	if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_TAB) {

		if (e.key.keysym.mod == KMOD_RSHIFT || e.key.keysym.mod ==KMOD_LSHIFT) {
			g_ux.joyon = !g_ux.joyon;
		}
		
	}
}

void ux_handleKeyPress(SDL_Event e) {

	//
	// check to see if we are switching modes between monitor and c64.
	//
	ux_handleMetaCommands(e);

	if (g_ux.passthru) {
		if (!g_ux.joyon) {
			ux_handlec64key(e);
		}
		else {
			ux_handlec64joystick(e);
		}
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
