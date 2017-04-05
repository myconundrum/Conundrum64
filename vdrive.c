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
MODULE: c64kbd.c
	Commodore 64 keyboard emulator. 

WORK ITEMS:

KNOWN BUGS:

*/

#define VDRIVE_ACK				0x00
#define VDRIVE_COMMAND_LISTEN 	0xF8

#define VDRIVE_BUS_MASK			0xF8

typedef enum {

	VDRIVE_IDLE,
	VDRIVE_LISTENING,
	VDRIVE_TX0,
	VDRIVE_TX1,
	VDRIVE_RX0,
	VDRIVE_RX1

} VDRIVE_STATE;


typedef struct {

	byte state;			// current drive state.
	byte lastbus;		// the last read of CIA2 $DD00
	byte rx;			// the byte being received. 
	byte tx;			// the byte being transmitted.

} VDRIVE;

g_vdrive = {0};

#define CIA2_SERIAL_BUS 0xDD00


byte vdrive_readbus() {
	return mem_peek(CIA2_SERIAL_BUS) & VDRIVE_BUS_MASK;
}

void vdrive_writebus(byte b) {

	g_vdrive.lastbus = b;
	mem_poke ((mem_peek(CIA2_SERIAL_BUS) & ~VDRIVE_BUS_MASK) | b);
}

void vdrive_update() {


	byte b = vdrive_readbus();

	if(b == g_vdrive.lastbus) {
		//
		// No change on the bus. Just return.
		//
		return;
	}

	g_vdrive.lastbus = b;

	switch(g_vdrive.state) {

		case VDRIVE_IDLE:
			if (b == VDRIVE_COMMAND_LISTEN) {
				DEBUG_PRINT("Vdrive: comamnded to listen.\n");
				vdrive_writebus(VDRIVE_ACK);
				g_vdrive.state = VDRIVE_RX0;
			}
		break;
		case VDRIVE_RX0:


		break;
	}




}


#include "emu.h"
#include "vdrive.h"