#include "emu.h"

void ux_handleKeyPress(UX *ux, SDL_Keycode code);


void ux_init(UX * ux,ASSEMBLER *a) {

	memset(ux,0,sizeof(UX));

	ux->assembler = a;

	//Initialize SDL
    if (SDL_Init (SDL_INIT_EVERYTHING) < 0 ) {
        DEBUG_PRINT( "SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return;
    }

    ux->wMon = SDL_CreateWindow ("Emulator Console", 
    	SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, MON_SCREEN_WIDTH, MON_SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
   	ux->rMon = SDL_CreateRenderer(ux->wMon, -1, SDL_RENDERER_ACCELERATED);
   	ux->font =  FC_CreateFont();

	SDL_RenderSetLogicalSize (ux->rMon, MON_SCREEN_WIDTH, MON_SCREEN_HEIGHT);
	SDL_SetRenderDrawColor (ux->rMon, 0, 0, 0, 255);
	FC_LoadFont(ux->font, ux->rMon, "/Library/Fonts/Andale Mono.ttf", 16, FC_MakeColor(255,255,255,255), TTF_STYLE_NORMAL);	

}

void ux_destroy(UX * ux) {

	FC_FreeFont(ux->font);
    SDL_DestroyWindow (ux->wMon);
    SDL_Quit();
}

void ux_updateMemory(UX *ux) {

	int i;
	int j; 
	char buf[255];


	SDL_Rect r = {MON_SCREEN_WIDTH - 540,40,54*10,16*20};
	SDL_RenderDrawRect(ux->rMon,&r);
	
	for (int i = 0; i < 16; i++) {
		sprintf(buf,"%02X:%02X:",ux->curpage,i*16);
		for (j = 0; j < 16; j++) {
			sprintf(buf+6+(j*3)," %02X",
				mem_peek((ux->curpage << 8) | (i*16+j)));
		}
		FC_Draw(ux->font,ux->rMon,MON_SCREEN_WIDTH - 540,40+i*20,buf);
	}
}

void ux_updateRegisters(UX *ux) {

	byte status = cpu_getstatus();
	SDL_Rect r = {0,0,MON_SCREEN_WIDTH,40};
	SDL_Color c = FC_MakeColor(102,255,51,255);

	SDL_RenderDrawRect(ux->rMon,&r);
	FC_Draw (ux->font, ux->rMon, 0, 0, 
		" A:      X:      Y:      Stack:      PC:        Ticks:             Secs: ");
	FC_Draw(ux->font,ux->rMon,0,20,"[N]  [V]  [X]  [B]  [D]  [I]  [Z]  [C]  ");

	FC_DrawColor (ux->font, ux->rMon, 0, 0, c,
		"   $%02X     $%02X     $%02X         $%02X      $%04X         %010d        %d",
		cpu_geta(),cpu_getx(),cpu_gety(),cpu_getstack(),cpu_getpc(),
		sysclock_getticks(),
		sysclock_getticks() / NTSC_TICKS_PER_SECOND);	

	FC_DrawColor(ux->font,ux->rMon,0,20,c,"   %d    %d    %d    %d    %d    %d    %d    %d ",
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


void ux_updateDisassembly(UX *ux) {
 	
	int i;
	SDL_Rect r = {0,40,260,16*20};
	SDL_RenderDrawRect(ux->rMon,&r);

	for (i = 0;i<DISLINESCOUNT;i++) {
		if (ux->discur == i) {
			FC_DrawColor(ux->font,ux->rMon,0,40+i*20,FC_MakeColor(102,255,51,255),"$%04X:    %s\n",
				ux->dislines[i].address,ux->dislines[i].buf);
		} else {
			FC_Draw(ux->font,ux->rMon,0,40+i*20,"$%04X:    %s\n",
				ux->dislines[i].address,ux->dislines[i].buf);
		}
	}
}

void ux_updateConsole(UX *ux) {

	SDL_Rect r = {0,360,MON_SCREEN_WIDTH,20};
	SDL_RenderDrawRect(ux->rMon,&r);
	FC_Draw(ux->font,ux->rMon,0,360,ux->buf);

}




void ux_update(UX *ux) {

	char ch;
	SDL_Event e;
	SDL_SetRenderDrawColor (ux->rMon, 0, 0, 0, 255);

	SDL_RenderClear(ux->rMon);
	SDL_SetRenderDrawColor(ux->rMon,255,255,255,255);
	
	ux_updateRegisters(ux);
	ux_updateMemory(ux);
	ux_updateDisassembly(ux);
	ux_updateConsole(ux);

	SDL_RenderPresent(ux->rMon);
    SDL_UpdateWindowSurface (ux->wMon);

	while (SDL_PollEvent (&e)) {
	
		switch (e.type) {
			case SDL_QUIT: 
				ux->done = true;
				break;
      		case SDL_KEYDOWN:
      			ux_handleKeyPress(ux,e.key.keysym.sym);
        	break;
      		case SDL_KEYUP:
      		break;
      		default:
        	break;
    	}
	}
			

}
/*
	if (ch != -1 && ux->passthru) {
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

void ux_handleKeyPress(UX *ux,SDL_Keycode code) {

	if (ux->passthru) {
		ux_handlec64key(code);
	}
	
	char ch = ux_getasciich(code);

	if (!ch) {
		switch (code) {
			case SDLK_ESCAPE:				// switch between c64 and regular mode.
				ux->buf[ux->bpos++] = '"';
				ux->buf[ux->bpos++] = ' ';
				handle_command(ux);
				memset(ux->buf,0,256);
				ux->bpos = 0;
			break;
			case SDLK_DOWN:					// step one instruction
				handle_step(ux);
			break;
			case SDLK_RIGHT:				// step over JSR
				//
				// set break point after jsr and run.
				//
				ux->brk = true;
				ux->brk_address = cpu_getpc() + 3;
				ux->running = true;
			break;
			default: break;
		}
	}
	else if (ch == '\n') {
		handle_command(ux);
		memset(ux->buf,0,256);
		ux->bpos = 0;
	}
	else {
		ux->buf[ux->bpos++] = ch;
	}
}





