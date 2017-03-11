#ifndef UX_H
#define UX_H 1

#include "emu.h"
#include "SDL_FontCache.h"
#include <sdl2/sdl.h>
#include <sdl2/SDL_ttf.h>



typedef struct {
	word address;
	char buf[32];
} DISLINE;


#define DISLINESCOUNT 16


#define MON_SCREEN_WIDTH 	800
#define MON_SCREEN_HEIGHT 	380

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
	ASSEMBLER  * assembler;

} UX;

extern void ux_init(UX * ux,ASSEMBLER *a);
extern void ux_destroy(UX * ux);
extern void ux_update(UX *ux);


#endif

