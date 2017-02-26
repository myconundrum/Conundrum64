
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include "emu.h"




int main(int argc, char**argv) {

	UX ux;
	ASSEMBLER assembler;
	int cycles = 0;

	mem_init();						// init ram
	init_computer();				// init 6502 CPU for C64 emulator.
	cia1_init();						// init CIA1 chip for C64 emulator.
	init_assembler(&assembler);
	init_ux(&ux, &assembler);
	
	//
	// load kernal and etc.
	//

	//loadBinFile(&assembler,"asm/901227-03-kernal.bin",0xE0,0x00);
	//loadBinFile(&assembler,"asm/901226-01-basic.bin",0xA0,0x00);
	//loadBinFile(&assembler,"asm/901225-01-char.bin",0xD0,0x00);

	fillDisassembly(&ux,cpu_getpc());

	do {

		if (!(cycles % 100)) {
			update_ux(&ux);
			
		}
		if (ux.running) {
			if (ux.brk && cpu_getpc() == ux.brk_address) {
				ux.running = false;
				ux.brk = false;
				fillDisassembly(&ux,cpu_getpc());
			}
			else {
				cia1_update();
				runcpu();
				mem_poke(0xD012,0); //BUGBUG hack. 
			}
		}
		cycles++;

	} while (!ux.done);

	destroy_ux(&ux);
	destroy_computer();
	mem_destroy();
	return 0;
}