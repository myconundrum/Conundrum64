#include "emu.h"

#include <time.h>

#define SYSCLOCK_CATCHUP 20000

typedef struct {

	unsigned long total;		// total systicks
	word 		  clast;		// systicks since last catchup
	clock_t		  clastreal;		// time at last refresh

} SYSCLOCK;

SYSCLOCK g_sysclock;

void sysclock_init(void) {

	g_sysclock.total 			= 0;
	g_sysclock.clast 			= 0;
	g_sysclock.clastreal		= clock();
}


void sysclock_addticks(unsigned long ticks) {

	g_sysclock.total += ticks;
	g_sysclock.clast += ticks;
	clock_t c;

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