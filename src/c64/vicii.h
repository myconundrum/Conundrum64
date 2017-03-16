#ifndef VICII_H
#define VICII_H

#include "emu.h"


#define VICII_SCREEN_WIDTH_PIXELS  320
#define VICII_SCREEN_HEIGHT_PIXELS 200

void vicii_init();
void vicii_update();
void vicii_destroy();


void vicii_setbank();

byte vicii_peek(word address);
void vicii_poke(word address,byte val);


bool vicii_badline();

#endif