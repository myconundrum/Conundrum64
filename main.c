
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include "emu.h"




int main(int argc, char**argv) {

	UX ux;
	ASSEMBLER assembler;
	int cycles = 0;


	DEBUG_INIT("c64.log");

	mem_init();								// init ram
	c64kbd_init();
	sysclock_init();						// init clock
	cpu_init();								// init 6502 CPU for C64 emulator.
	cia_init();								// init CIA1 chip for C64 emulator.
	init_assembler(&assembler);
	ux_init(&ux, &assembler);

	fillDisassembly(&ux,cpu_getpc());

	ux.running = true;

	do {

		if (!(cycles % 10000)) {
			ux_update(&ux);
			
		}
		if (ux.running) {
			if (ux.brk && cpu_getpc() == ux.brk_address) {
				ux.running = false;
				ux.brk = false;
				ux.passthru = false;
				fillDisassembly(&ux,cpu_getpc());
			}
			else {
				cia_update();
				cpu_run();
				mem_poke(0xD012,0); //BUGBUG hack. clear raster line interrupt. 
			}
		}
		cycles++;

	} while (!ux.done);

	ux_destroy(&ux);

	cpu_destroy();
	mem_destroy();
	cia_destroy(); 
	c64kbd_destroy();
	DEBUG_DESTROY();
	return 0;
}