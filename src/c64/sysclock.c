/*
Conundrum 64: Commodore 64 Emulator

MIT License

Copyright (c) 2017 Marc R. Whitten

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

#include <time.h>
#include "cpu.h"
#include "sysclock.h"

#define SYSCLOCK_CATCHUP 20000

typedef struct {

	unsigned long total;			// total systicks
	word 		  clast;			// systicks since last catchup
	clock_t		  clastreal;		// time at last refresh
	word	      lastadd;			// amount of ticks added in last call to sysclock_addticks()

} SYSCLOCK;

SYSCLOCK g_sysclock;

void sysclock_init(void) {

	g_sysclock.total 			= 0;
	g_sysclock.clast 			= 0;
	g_sysclock.clastreal		= clock();
}


word sysclock_getlastaddticks() {return g_sysclock.lastadd;}

void sysclock_addticks(unsigned long ticks) {

	clock_t c;

	g_sysclock.lastadd = ticks;
	g_sysclock.total += ticks;
	g_sysclock.clast += ticks;



	if (g_sysclock.clast > SYSCLOCK_CATCHUP) {
		//
		// wait for real clock to catchup.
		//
		do {

			c = clock();
		} while (
			(((double) g_sysclock.clast)/NTSC_TICKS_PER_SECOND) >
			(((double) (c - g_sysclock.clastreal)) / CLOCKS_PER_SEC));

		g_sysclock.clastreal = c;
		g_sysclock.clast = 0;
	}
}


unsigned long sysclock_getticks(void) {
	return g_sysclock.total;
}