#include "emu.h"


typedef void (*PARSEFN)(char *);

typedef struct {

	const char * cmd;
	PARSEFN fn;

} PARSECMD;



void parseQuit(char * s);
void parseMem (char * s);
void parseBrk (char * s);
void parseAsm (char * s);
void parseExec (char * s);
void parseStop  (char * s);
void parseDis  (char * s);
void parseStep (char * s);
void parseComment  (char * s);
void parsePassThru (char * s);






PARSECMD g_commands[] = {
	{"QUIT",parseQuit},
	{"MEM",parseMem},
	{"BRK",parseBrk},
	{"EXEC",parseExec},
	{"STOP",parseStop},
	{"DIS",parseDis},
	{"S",parseStep},
	{"\"",parsePassThru},
};


typedef struct {
	word address;
	char buf[32];
} DISLINE;

typedef struct {

    SDL_Window  	* wMon; 
    FC_Font 		* font;
    SDL_Renderer	* rMon;
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
	bool 		running;
	bool		passthru;
	int 		cycles;
	ASSEMBLER  * assembler;

} UX;

UX g_ux;

void fillDisassembly(word address) {

	int i;
	g_ux.disstart = 0;
	g_ux.discur = 0;

	for (i =0 ; i <  DISLINESCOUNT; i++) {
		g_ux.dislines[i].address = address;
		disassembleLine(g_ux.assembler,g_ux.dislines[i].buf,&address);
	}
}



void parseStep(char *s) { c64_update();}
void handle_step() {

	c64_update();
	g_ux.discur++;

	if (cpu_getpc() != g_ux.dislines[g_ux.discur].address || g_ux.discur == DISLINESCOUNT) {
		fillDisassembly(cpu_getpc());
	}
}

void parseExec (char *s) {g_ux.running = true;}
void parseStop (char *s) {g_ux.running = false;}
void parsePassThru (char *s) {g_ux.passthru = true;}
void parseQuit(char * s) {g_ux.done = true;}

void parseMem (char * s) {
	s = strtok(NULL," ");
	if (s) {
		g_ux.curpage = strtoul(s,NULL,16);
	}
}

void parseDis (char * s) {

	word address;
	int  i;

	s = strtok(NULL," ");
	if (s) {
		address = strtoul(s,NULL,16);
	}
	else {
		address = cpu_getpc();
	}
	fillDisassembly(address);	
}


typedef struct {
	byte hi;
	byte low;
	ENUM_AM mode;
	byte bytes;

} ADDRESSANDMODE;

int getBytesFromString(char *s, ADDRESSANDMODE * am) {

	char lows[3];
	char his[3];
	int count =0;
	int i;

	for (i = 0; i < strlen(s); i++) {
		if (!isxdigit(s[i])) {break;}
		if (i < 2) {
			his[i] = s[i];
		}
		else if (i < 4) {
			lows[i-2] = s[i];
		}	
		count++;	
	}

	if (count > 2) {
		am->hi = strtoul(his,NULL,16);
		am->low = strtoul(lows,NULL,16);
		am->bytes = 2;
	} else {
		am->hi= 0;
		am->low = strtol(his,NULL,16);
		am->bytes = 1;
	}

	return count;
}

void getAddressAndMode (char *s, ADDRESSANDMODE * am) {

	int i;
	bool immediate = false;
	bool indirect = false;
	bool xreg = false;
	bool yreg = false;
	int numlen = 0;


	for (i = 0; i < strlen(s); i++) {
		switch(s[i]) {
			case '#': immediate = true; break;
			case 'X': xreg = true; break;
			case 'Y': yreg = true; break;
			case '(': indirect = true; break;
			case '$': 
				numlen = getBytesFromString(s+i+1,am);
				i+=numlen;
				break;
			default:break; 
		}
	}

	if (immediate) {
		am->mode = AM_IMMEDIATE;
	}
	else if (indirect && am->bytes == 2) {
		am->mode = AM_INDIRECT;
	}
	else if (indirect && xreg) {
		am->mode = AM_INDEXEDINDIRECT;
	}
	else if (indirect && yreg) {
		am->mode = AM_INDIRECTINDEXED;
	}
	else if (am->bytes==2  && xreg) {
		am->mode = AM_ABSOLUTEX;
	}
	else if (am->bytes==2 && yreg) {
		am->mode = AM_ABSOLUTEY;
	}
	else if (am->bytes==2) {
		am->mode = AM_ABSOLUTE;
	}
	else if (am->bytes==1 && xreg) {
		am->mode = AM_ZEROPAGEX;
	}
	else if (am->bytes==1 && yreg) {
		am->mode = AM_ZEROPAGEY;
	}
	else if (am->bytes==1)  {

		am->mode = AM_ZEROPAGE;
	}
}

void parseOp (char *s) {
	
	char buf[4];
	ADDRESSANDMODE am;
	int i = 0;
	ENUM_AM m;
	char opbuf[10];

	strncpy(buf,s,4);
	s = strtok(NULL," ");
	if (s) {
		getAddressAndMode(s,&am);
	} else {
		am.bytes = 0;
		am.mode = AM_IMPLICIT;
	} 

	//
	// fix up branch instructions.
	//
	if (buf[0] == 'B' && am.mode == AM_ZEROPAGE && (strcmp(buf,"BIT") != 0)) {
		am.mode = AM_RELATIVE;
	}

	for (int i = 0; i < 256; i++) {

		if (cpu_getopcodeinfo(i,opbuf,&m) &&
			!strcmp(buf,opbuf) && am.mode == m) {
		
			mem_poke(g_ux.asm_address++,i);
			if (am.bytes) {
				mem_poke(g_ux.asm_address++,am.low);
			}
			if (am.bytes == 2) {
				mem_poke(g_ux.asm_address++,am.hi);
			}
		}
	}	
}

void parseBrk (char * s) {
	s = strtok(NULL," ");
	g_ux.brk_address = 0;

	if (s) {
		g_ux.brk = true;
		g_ux.brk_address = strtoul(s,NULL,16);
	}
}


bool ux_running() {
	return g_ux.running;
}

bool ux_done() {
	return g_ux.done;
}


void ux_handle_command() {

	char * p = NULL;
	int i;

	p = strtok(g_ux.buf," ");
	if (!p) {
		if (!p) {
			return;
		}
	}

	for (i = 0; i < sizeof(g_commands)/sizeof(PARSECMD); i++) {
		if (strcmp(g_commands[i].cmd,p)==0) {
			g_commands[i].fn(p);
			break;
		}
	}
	if (i==sizeof(g_commands)/sizeof(PARSECMD)) {
		DEBUG_PRINT("Error. Could not parse %s\n ",g_ux.buf);
	}
}



void ux_handle_step();
void fillDisassembly(word address);



void ux_handleKeyPress(SDL_Keycode code);
void ux_init(ASSEMBLER *a) {

	memset(&g_ux,0,sizeof(UX));

	g_ux.assembler = a;

    if (SDL_Init (SDL_INIT_EVERYTHING) < 0 ) {
        DEBUG_PRINT( "SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return;
    }

    g_ux.wMon = SDL_CreateWindow ("Emulator Console", 
    	SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, MON_SCREEN_WIDTH, MON_SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
   	g_ux.rMon = SDL_CreateRenderer(g_ux.wMon, -1, SDL_RENDERER_ACCELERATED);
   	g_ux.font =  FC_CreateFont();

	SDL_RenderSetLogicalSize (g_ux.rMon, MON_SCREEN_WIDTH, MON_SCREEN_HEIGHT);
	SDL_SetRenderDrawColor (g_ux.rMon, 0, 0, 0, 255);
	FC_LoadFont(g_ux.font, g_ux.rMon, "/Library/Fonts/Andale Mono.ttf", 16, FC_MakeColor(255,255,255,255), TTF_STYLE_NORMAL);	

}

void ux_destroy() {

	FC_FreeFont(g_ux.font);
    SDL_DestroyWindow (g_ux.wMon);
    SDL_Quit();
}

void ux_updateMemory() {

	int i;
	int j; 
	char buf[255];


	SDL_Rect r = {MON_SCREEN_WIDTH - 540,40,54*10,16*20};
	SDL_RenderDrawRect(g_ux.rMon,&r);
	
	for (int i = 0; i < 16; i++) {
		sprintf(buf,"%02X:%02X:",g_ux.curpage,i*16);
		for (j = 0; j < 16; j++) {
			sprintf(buf+6+(j*3)," %02X",
				mem_peek((g_ux.curpage << 8) | (i*16+j)));
		}
		FC_Draw(g_ux.font,g_ux.rMon,MON_SCREEN_WIDTH - 540,40+i*20,buf);
	}
}

void ux_updateRegisters() {

	byte status = cpu_getstatus();
	SDL_Rect r = {0,0,MON_SCREEN_WIDTH,40};
	SDL_Color c = FC_MakeColor(102,255,51,255);

	SDL_RenderDrawRect(g_ux.rMon,&r);
	FC_Draw (g_ux.font, g_ux.rMon, 0, 0, 
		" A:      X:      Y:      Stack:      PC:        Ticks:             Secs: ");
	FC_Draw(g_ux.font,g_ux.rMon,0,20,"[N]  [V]  [X]  [B]  [D]  [I]  [Z]  [C]  ");

	FC_DrawColor (g_ux.font, g_ux.rMon, 0, 0, c,
		"   $%02X     $%02X     $%02X         $%02X      $%04X         %010d        %d",
		cpu_geta(),cpu_getx(),cpu_gety(),cpu_getstack(),cpu_getpc(),
		sysclock_getticks(),
		sysclock_getticks() / NTSC_TICKS_PER_SECOND);	

	FC_DrawColor(g_ux.font,g_ux.rMon,0,20,c,"   %d    %d    %d    %d    %d    %d    %d    %d ",
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
	SDL_RenderDrawRect(g_ux.rMon,&r);

	for (i = 0;i<DISLINESCOUNT;i++) {
		if (g_ux.discur == i) {
			FC_DrawColor(g_ux.font,g_ux.rMon,0,40+i*20,FC_MakeColor(102,255,51,255),"$%04X:    %s\n",
				g_ux.dislines[i].address,g_ux.dislines[i].buf);
		} else {
			FC_Draw(g_ux.font,g_ux.rMon,0,40+i*20,"$%04X:    %s\n",
				g_ux.dislines[i].address,g_ux.dislines[i].buf);
		}
	}
}

void ux_updateConsole() {

	SDL_Rect r = {0,360,MON_SCREEN_WIDTH,20};
	SDL_RenderDrawRect(g_ux.rMon,&r);
	FC_Draw(g_ux.font,g_ux.rMon,0,360,g_ux.buf);

}




void ux_update() {

	char ch;

	if (ux_running()) {
		if (g_ux.brk && cpu_getpc() == g_ux.brk_address) {
			g_ux.running = false;
			g_ux.brk = false;
			g_ux.passthru = false;
			fillDisassembly(cpu_getpc());
		}
	}

	if (g_ux.cycles++ % 10000) {
		return;
	}

	SDL_Event e;
	SDL_SetRenderDrawColor (g_ux.rMon, 0, 0, 0, 255);

	SDL_RenderClear(g_ux.rMon);
	SDL_SetRenderDrawColor(g_ux.rMon,255,255,255,255);
	
	ux_updateRegisters();
	ux_updateMemory();
	ux_updateDisassembly();
	ux_updateConsole();

	SDL_RenderPresent(g_ux.rMon);
    SDL_UpdateWindowSurface (g_ux.wMon);

	while (SDL_PollEvent (&e)) {
	
		switch (e.type) {
			case SDL_QUIT: 
				g_ux.done = true;
				break;
      		case SDL_KEYDOWN:
      			ux_handleKeyPress(e.key.keysym.sym);
        	break;
      		case SDL_KEYUP:
      		break;
      		default:
        	break;
    	}
	}
}
/*
	if (ch != -1 && g_ux.passthru) {
		switch(ch) {
			
			case 0x1B : handle_specialkeys(ux);break;
			case 0x7F : kq_add(C64KEY_DELETE,false,false);break;
			case '`'  : kq_add(C64KEY_RUNSTOP,false,false);break;
			case '~'  : kq_add(C64KEY_RESTORE,false,false);break;
			case '!'  : kq_add('1',true,false);break;
			case '"'  : kq_add('2',true,false);break;
			case '#'  : kq_add('3',true,false);break;
			case '$'  : kq_add('4',true,false);break;
			case '%'  : kq_add('5',true,false);break;
			case '&'  : kq_add('6',true,false);break;
			case '\'' : kq_add('7',true,false);break;
			case '('  : kq_add('8',true,false);break;
			case ')'  : kq_add('9',true,false);break;
			default: kq_add(toupper(ch),false,false); break;
		}	
	}
*/


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

void ux_handlec64key(SDL_Keycode code) {

}

void ux_handleKeyPress(SDL_Keycode code) {

	if (g_ux.passthru) {
		ux_handlec64key(code);
	}
	
	char ch = ux_getasciich(code);

	if (!ch) {
		switch (code) {
			case SDLK_ESCAPE:				// switch between c64 and regular mode.
				g_ux.buf[g_ux.bpos++] = '"';
				g_ux.buf[g_ux.bpos++] = ' ';
				ux_handle_command(g_ux.buf);
				memset(g_ux.buf,0,256);
				g_ux.bpos = 0;
			break;
			case SDLK_DOWN:					// step one instruction
				handle_step();
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
		ux_handle_command();
		memset(g_ux.buf,0,256);
		g_ux.bpos = 0;
	}
	else {
		g_ux.buf[g_ux.bpos++] = ch;
	}
}





