/*
Conundrum 64: Commodore 64 Emulator

MIT License

Copyright (c) 2017 

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
MODULE: emu.h

WORK ITEMS:

KNOWN BUGS:

*/

#ifndef EMU_H
#define EMU_H


#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <stdbool.h>
/*
#include <ncurses.h>
#include "cpu.h"
#include "c64kbd.h"
#include "cia.h"
#include "mem.h"
#include "ux.h"
#include "stdint.h"
#include "sysclock.h"
#include "c64.h"
#include "vicii.h"

*/
#define EMU_NAME 				"Conundrum 64"
#define EMU_VERSION_MINOR		1
#define EMU_VERSION_MAJOR		0


#define DEBUG 1
#define EMU_DOUBLE_SCREEN 1


#if defined(DEBUG) && DEBUG > 0

FILE * g_debug;

char * emu_getname();

 #define DEBUG_INIT(log) g_debug = fopen(log,"w+")
 #define DEBUG_PRINT(fmt, args...) fprintf(g_debug, "DEBUG: %s:%d:%s(): " fmt, \
    __FILE__, __LINE__, __func__, ##args)
 #define DEBUG_DESTROY() fclose(g_debug)
#else
 #define DEBUG_INIT(log)
 #define DEBUG_PRINT(fmt, args...) /* Don't do anything in release builds */
 #define DEBUG_DESTROY()
#endif


#endif
