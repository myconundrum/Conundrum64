#ifndef VICII_H
#define VICII_H

#include "emu.h"

#define VICII_NTSC_HEIGHT 							263
#define VICII_NTSC_WIDTH  							520 // add the above.
 
//
// SCREEN is the 40X25 display grid, does not include border. 
//
#define VICII_SCREEN_WIDTH 		320
#define VICII_SCREEN_HEIGHT 	200


#define VICII_NTSC_VBLANK  		16 // 16 lines of vblank.

#define VICII_SCREENFRAME_HEIGHT (VICII_NTSC_HEIGHT - VICII_NTSC_VBLANK)
#define VICII_SCREENFRAME_WIDTH VICII_NTSC_WIDTH


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