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
MODULE: sysclock.c
system clock timer source.


WORK ITEMS:

KNOWN BUGS:

*/
#include "emu.h"
#include "string.h"

#include <time.h>
#include "cpu.h"
#include "sysclock.h"

#define SYSCLOCK_CATCHUP 20000

typedef struct {

	unsigned long total;			// total systicks
	word 		  clast;			// systicks since last catchup
	clock_t		  clastreal;		// time at last refresh
	word	      lastadd;			// amount of ticks added in last call to sysclock_addticks()
	unsigned long tickspersec;		// amount of ticks that should occur in one second. varies by
									// NTSC and PAL

	bool		  phi;				// true == high false == low 
} SYSCLOCK;

SYSCLOCK g_sysclock = {0};

void sysclock_init(void) {

	EMU_CONFIGURATION * cfg = emu_getconfig();
	DEBUG_PRINT("** Initializing System Clock...\n");

	g_sysclock.total 			= 0;
	g_sysclock.clast 			= 0;
	g_sysclock.clastreal		= clock();

	if (cfg->region && !strcmp(cfg->region,"PAL")) {
		g_sysclock.tickspersec = PAL_TICKS_PER_SECOND;
		DEBUG_PRINT("PAL region selected. %lu cycles per second.\n",g_sysclock.tickspersec);
	}
	else {
		g_sysclock.tickspersec = NTSC_TICKS_PER_SECOND;
		DEBUG_PRINT("NTSC region selected. %lu cycles per second.\n",g_sysclock.tickspersec);
	}

}

bool sysclock_isPALfrequency(void) {
	return g_sysclock.tickspersec == PAL_TICKS_PER_SECOND;
}


bool sysclock_isNTSCfrequency(void) {
	return g_sysclock.tickspersec == NTSC_TICKS_PER_SECOND;
}


bool sysclock_getphi(void) {

	return g_sysclock.phi;
}

void sysclock_update(void) {


	clock_t c;

	if(!g_sysclock.phi) {
		g_sysclock.total++;
		g_sysclock.clast++;
	}


	if (g_sysclock.clast > SYSCLOCK_CATCHUP) {
		//
		// wait for real clock to catchup.
		//
		do {

			c = clock();
		} while ((((double) g_sysclock.clast)/g_sysclock.tickspersec) > (((double) (c - g_sysclock.clastreal)) / CLOCKS_PER_SEC));

		g_sysclock.clastreal = c;
		g_sysclock.clast = 0;
	}

	g_sysclock.phi = !g_sysclock.phi;

}

double sysclock_getelapsedseconds(void) {
	return (double) g_sysclock.total / g_sysclock.tickspersec;
}

unsigned long sysclock_gettickspersec(void) {
	return g_sysclock.tickspersec;
}

unsigned long sysclock_getticks(void) {
	return g_sysclock.total;
}
