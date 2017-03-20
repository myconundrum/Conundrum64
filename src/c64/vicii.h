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
MODULE: vicii.h

WORK ITEMS:

KNOWN BUGS:

*/

#ifndef VICII_H
#define VICII_H

#include "emu.h"

#define VICII_NTSC_HEIGHT 							263
#define VICII_NTSC_WIDTH  							520 // add the above.
 
//
// SCREEN is the 40X25 display grid, does not include border. 
//
#define VICII_SCREEN_WIDTH 			320
#define VICII_SCREEN_HEIGHT 		200
#define VICII_NTSC_VBLANK  			16 // 16 lines of vblank.
#define VICII_NTSC_VBLANK_LEFT		96 // n cycles of no drawgraphics.
#define VICII_NTSC_VBLANK_RIGHT		48
#define VICII_SCREENFRAME_HEIGHT (VICII_NTSC_HEIGHT - VICII_NTSC_VBLANK)
#define VICII_SCREENFRAME_WIDTH  (VICII_NTSC_WIDTH  - VICII_NTSC_VBLANK_LEFT - VICII_NTSC_VBLANK_RIGHT) 


typedef struct {
	byte data[VICII_SCREENFRAME_HEIGHT][VICII_SCREENFRAME_WIDTH];
} VICII_SCREENFRAME;



void vicii_init();
void vicii_update();
void vicii_destroy();


void vicii_setbank();

byte vicii_peek(word address);
void vicii_poke(word address,byte val);


bool vicii_badline();

VICII_SCREENFRAME * vicii_getframe();

#endif