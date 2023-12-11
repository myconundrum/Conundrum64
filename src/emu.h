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
#include <time.h>
#include "cpu.h"
#include "c64.h"

#define EMU_NAME 				"Conundrum 64"
#define EMU_VERSION_MINOR		1
#define EMU_VERSION_MAJOR		0

typedef struct {
    
    const char* 	kernalpath;
    const char* 	charpath;
    const char* 	basicpath;
    const char* 	binload;
    const char* 	cartload;
    const char*     region;
	const char*     font;
    const char*     disk;
    const char*     program;
    uint16_t  breakpoint;

} EMU_CONFIGURATION;


EMU_CONFIGURATION * emu_getconfig(void);




#define DEBUG 1
#define EMU_DOUBLE_SCREEN 1
//#define DEBUG_SHOW_SOURCE 1



#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"


#define FATAL_ERROR(fmt, args...) do {		\
		printf(fmt, ##args); 				\
		DEBUG_DESTROY();					\
		abort();							\
	} while (0)								\

#if defined(DEBUG) && DEBUG > 0


	FILE * 	g_debug;
	clock_t g_debugstart;

	char * emu_getname(void);

	#define DEBUG_INIT(log) do {    					\
			g_debug = fopen(log,"w+");   				\
			g_debugstart = clock();						\
			DEBUG_PRINT("Starting %s.\n",emu_getname()); \
		}while (0)

	#define DEBUG_DESTROY() do {											\
			DEBUG_PRINT("Ending %s. Run lasted %lf seconds.\n",				\
				emu_getname(), 												\
				(double) (clock() - g_debugstart)/CLOCKS_PER_SEC);          \
			fclose(g_debug);												\
		}while (0)
	#define DEBUG_IF(x) if(x) {
	#define DEBUG_ENDIF() }

	#ifdef DEBUG_SHOW_SOURCE
		#define DEBUG_PRINT(fmt, args...) fprintf(g_debug, "[%s@%d] %s(): " fmt, \
    		__FILE__, __LINE__, __func__, ##args)
		#define DEBUG_PRINTIF(b,fmt,args...) if ((b)) {fprintf(g_debug, "[%s@%d] %s(): " fmt, \
    		__FILE__, __LINE__, __func__, ##args);}
	#else 
		#define DEBUG_PRINT(fmt, args...) fprintf(g_debug, "[PC: 0x%04X] " fmt,cpu_getpc(),##args)
		#define DEBUG_PRINTIF(b,fmt,args...) if ((b)) {fprintf(g_debug, "[PC: 0x%04X] " fmt,cpu_getpc(),##args);}
	#endif





#else
	#define DEBUG_INIT(log)
	#define DEBUG_PRINT(fmt, args...) /* Don't do anything in release builds */
	#define DEBUG_PRINTIF(b,fmt,args...) 
	#define DEBUG_DESTROY()
	#define DEBUG_IF(b)
	#define DEBUG_ENDIF()
#endif



#pragma clang diagnostic pop

#endif
