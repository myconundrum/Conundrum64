
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
MODULE: main.c

WORK ITEMS:

KNOWN BUGS:

*/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include "emu.h"

char g_nameString[255];
char * emu_getname() {return g_nameString;}



int main(int argc, char**argv) {

	DEBUG_INIT("c64.log");
	
	sprintf(g_nameString,"%s (version %d.%d)",EMU_NAME,EMU_VERSION_MAJOR,EMU_VERSION_MINOR);


	c64_init();
	ux_init();


	ux_startemulator();
	
	do {
		ux_update();
		if (ux_running()) {c64_update();}

	} while (!ux_done());

	c64_destroy();
	ux_destroy();

	DEBUG_DESTROY();
	return 0;
}