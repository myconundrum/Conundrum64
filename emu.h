#ifndef EMU_H
#define EMU_H


#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <ncurses.h>
#include "cpu.h"
#include "mem.h"
#include "asm.h"
#include "ux.h"
#include "cia1.h"
#include "stdint.h"





void handle_command(UX * ux);
void handle_step(UX * ux);
void fillDisassembly(UX *ux, word address);
int asmfile(char * path);


#endif
