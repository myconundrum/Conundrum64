#include "emu.h"


unsigned long g_sysclockticks;


void sysclock_init(void) {
	g_sysclockticks = 0;
}


void sysclock_addticks(unsigned long ticks) {
	g_sysclockticks += ticks;
}


unsigned long sysclock_getticks(void) {
	return g_sysclockticks;
}