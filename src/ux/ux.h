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
MODULE: ux.h

WORK ITEMS:

KNOWN BUGS:

*/
#ifndef UX_H
#define UX_H 1

#include "emu.h"
#include "SDL_FontCache.h"
#include <sdl2/sdl.h>
#include <sdl2/SDL_ttf.h>



#define MON_SCREEN_WIDTH 	800
#define MON_SCREEN_HEIGHT 	380


void ux_init();
void ux_destroy();
void ux_update();
bool ux_running();
bool ux_done();
void ux_startemulator();

#endif

