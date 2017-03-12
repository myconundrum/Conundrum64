#ifndef VICII_H
#define VICII_H

#include "emu.h"

void vicii_init();
void vicii_update();
void vicii_destroy();

byte vicii_peek(word address);
void vicii_poke(word address,byte val);

#endif