
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include "emu.h"

int main(int argc, char**argv) {

	DEBUG_INIT("c64.log");
	ux_init();
	c64_init();
	fillDisassembly(cpu_getpc());
	
	do {
		ux_update();
		if (ux_running()) {
			c64_update();
		}

	} while (!ux_done());

	ux_destroy();
	c64_destroy();

	DEBUG_DESTROY();
	return 0;
}