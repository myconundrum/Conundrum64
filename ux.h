#ifndef UX_H
#define UX_H 1

#include "emu.h"
#include "SDL_FontCache.h"
#include <sdl2/sdl.h>
#include <sdl2/SDL_ttf.h>




#define DISLINESCOUNT 16


#define MON_SCREEN_WIDTH 	800
#define MON_SCREEN_HEIGHT 	380


void ux_init();
void ux_destroy();
void ux_update();
bool ux_running();
bool ux_done();




#endif

