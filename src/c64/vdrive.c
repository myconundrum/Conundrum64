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


Virtual Drive is a slave device to the CPU. It only responds to commands, never initiates.


Idle - waiting for a command.
	Listen for ATN command
	ACK
	move to RX (byte count)
	while (byte count) {
		move to RX 
		save byte to command buffer
		byte count--
	}

RX
	Look for change
	Move bits to lower nibble
	ACK
	Look for change
	Move bits to upper nibble
	ACK
	return to previous state













*/

#include "emu.h"
#include "vdrive.h"
#include "cpu.h"

#define VDRIVE_ACK				0x00
#define VDRIVE_COMMAND_LISTEN 	0xF8

#define VDRIVE_BUS_CMD_MASK		0xF8
#define VDRIVE_BUS_DATA_MASK	0xF0


#define VDRIVE_MAX_READ_DATA 	0xFF


#define CIA2_SERIAL_BUS 0xDD00

typedef enum {

	VDRIVE_IDLE,
	VDRIVE_READBYTES,	
	VDRIVE_TX0,
	VDRIVE_TX1,
	VDRIVE_RX0,
	VDRIVE_RX1

} VDRIVE_STATE;


typedef struct {

	VDRIVE_STATE state;			// current drive state.
	byte returnstate;			// rx and tx return to a state after constructing a byte.
	byte lastbus;				// the last read of CIA2 $DD00
	byte rx;					// the byte being received. 
	byte tx;					// the byte being transmitted.


	int readcount;		

	byte data[VDRIVE_MAX_READ_DATA];
	byte idata;

} VDRIVE;

VDRIVE g_vdrive = {0};


void vdrive_init() {

	DEBUG_PRINT("** Initializing virtual drive.\n");

	g_vdrive.state 			= VDRIVE_IDLE;
	g_vdrive.returnstate 	= VDRIVE_IDLE;
	g_vdrive.readcount		= -1;

}


byte vdrive_readbus() {
	return mem_peek(CIA2_SERIAL_BUS) & VDRIVE_BUS_CMD_MASK;
}

void vdrive_writebus(byte b) {

	g_vdrive.lastbus = b;
	mem_poke (CIA2_SERIAL_BUS,(mem_peek(CIA2_SERIAL_BUS) & ~VDRIVE_BUS_CMD_MASK) | b);
}

void vdrive_ack() {
	vdrive_writebus(VDRIVE_ACK);
}

void vdrive_update() {


	byte b = vdrive_readbus();

	switch(g_vdrive.state) {

		case VDRIVE_IDLE:
			if (b == VDRIVE_COMMAND_LISTEN) {
				DEBUG_PRINT("Vdrive was idle, but commanded to listen.\n");
				vdrive_ack();
				g_vdrive.state = VDRIVE_RX0;
				g_vdrive.returnstate = VDRIVE_READBYTES;
			}
		break;
		case VDRIVE_RX0:
			if (b != g_vdrive.lastbus) {
				g_vdrive.rx = b >> 4;
				vdrive_ack();
				g_vdrive.state = VDRIVE_RX1;
			}
		break;
		case VDRIVE_RX1:
			if (b != g_vdrive.lastbus) {
				g_vdrive.rx |= (b & VDRIVE_BUS_DATA_MASK);
				g_vdrive.state = g_vdrive.returnstate; 
				vdrive_ack();
			}
		break;
		case VDRIVE_READBYTES:

			if (g_vdrive.readcount == -1) {
				g_vdrive.readcount = g_vdrive.rx; 
				g_vdrive.idata = 0;
				DEBUG_PRINT("Vdrive instructed to read %d bytes.\n",g_vdrive.rx);

			} else if (g_vdrive.readcount) {
				g_vdrive.data[g_vdrive.idata++] = g_vdrive.rx;
				g_vdrive.state = VDRIVE_RX0;
				g_vdrive.readcount--;
				g_vdrive.returnstate =VDRIVE_READBYTES;
			}
			else {
				//
				// finished reading bytes. save last byte.
				//
				g_vdrive.data[g_vdrive.idata++] = g_vdrive.rx;

				DEBUG_PRINT("Vdrive read %d bytes.\n",g_vdrive.data[0]);
				for (int i = 1; i < g_vdrive.idata;i++) {
					DEBUG_PRINT("\tbyte 0x%02X\n",g_vdrive.data[i]);
				}
				g_vdrive.state = VDRIVE_IDLE;
				g_vdrive.readcount = -1;
				g_vdrive.idata = 0;
			}

		break;
	}




}


#include "emu.h"
#include "vdrive.h"