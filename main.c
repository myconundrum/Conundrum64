
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include "emu.h"

int main(int argc, char**argv) {

	UX ux;
	CPU6502 cpu;
	ASSEMBLER assembler;
	int cycles = 0;

	init_computer(&cpu);
	init_assembler(&assembler, &cpu);
	init_ux(&ux, &assembler);

	//
	// load kernal and etc.
	//
	//assembleFile(&assembler,"asm//reset.asm");
	//assembleFile(&assembler,"asm//basic.asm");
	//assembleFile(&assembler,"asm//testcart.asm");
	loadBinFile(&assembler,"asm/901227-03-kernal.bin",0xE0,0x00);
	loadBinFile(&assembler,"asm/901226-01-basic.bin",0xA0,0x00);


	do {

		if (!(cycles % 100)) {
			update_ux(&ux,&cpu);
			
		}
		if (ux.running) {
			if (ux.brk && cpu.pc_high == ux.brkh && cpu.pc_low == ux.brkl) {
				ux.running = false;
				ux.brk = false;
			}
			else {
				runcpu(&cpu);
			}
		}
		cycles++;

	} while (!ux.done);

	destroy_ux(&ux);
	destroy_computer(&cpu);
	return 0;
}