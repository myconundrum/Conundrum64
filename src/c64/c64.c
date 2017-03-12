#include "emu.h"

void c64_init() {

	mem_init();								// init ram
	c64kbd_init();
	sysclock_init();						// init clock
	cpu_init();								// init 6502 CPU for C64 emulator.
	cia_init();								// init CIA1 chip for C64 emulator.
}

void c64_update() {
	cia_update();
	cpu_run();
	mem_poke(0xD012,0); //BUGBUG hack. clear raster line interrupt. 

}

void c64_destroy() {
	cpu_destroy();
	mem_destroy();
	cia_destroy(); 
	c64kbd_destroy();
}