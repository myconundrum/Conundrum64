
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include "emu.h"

int main(int argc, char**argv) {

	DEBUG_INIT("c64.log");
	

	c64_init();
	ux_init();
	
	do {
		ux_update();
		if (ux_running()) {c64_update();}

	} while (!ux_done());

	c64_destroy();
	ux_destroy();

	DEBUG_DESTROY();
	return 0;
}