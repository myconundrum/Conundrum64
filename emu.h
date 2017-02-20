#ifndef EMU_H
#define EMU_H


#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <ncurses.h>
#include "cpu.h"
#include "asm.h"
#include "ux.h"




void handle_command(UX * ux,CPU6502 *c);
int asmfile(char * path);


#endif
