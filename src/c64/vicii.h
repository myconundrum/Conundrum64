#ifndef VICII_H
#define VICII_H

#include "emu.h"


#define VICII_SCREEN_WIDTH_PIXELS  320
#define VICII_SCREEN_HEIGHT_PIXELS 200

//
// These constants are from https://github.com/dirkwhoffmann
// 
#define VICII_NTSC_LEFT_VBLANK 						77
#define VICII_NTSC_LEFT_BORDER_WIDTH 				55
#define VICII_NTSC_DISPLAY_WIDTH 					320
#define VICII_NTSC_RIGHT_BORDER_WIDTH 				53
#define VICII_NTSC_RIGHT_VBLANK 					15
#define VICII_NTSC_WIDTH  							520 // add the above.
#define VICII_NTSC_RENDERED_PIXELS_WIDTH 			428 // minus vblanking

#define VICII_NTSC_UPPER_VBLANK						16
#define VICII_NTSC_UPPER_BORDER_HEIGHT 				10
#define VICII_NTSC_DISPLAY_HEIGHT 					200
#define VICII_NTSC_LOWER_BORDER_HEIGHT 				25
#define VICII_NTSC_LOWER_VBLANK 					12
#define VICII_NTSC_HEIGHT 							263
#define VICII_NTSC_RENDERED_RASTERLINES 			235

typedef struct {
	byte data[VICII_NTSC_HEIGHT][VICII_NTSC_WIDTH];
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