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





void handle_command(UX * ux,CPU6502 *c);
void handle_step(UX * ux,CPU6502 *c);
void fillDisassembly(UX *ux, CPU6502 *c, byte high, byte low);
int asmfile(char * path);


#endif
