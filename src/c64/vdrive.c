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
#define VDRIVE_COMMAND_RX       0x18

#define VDRIVE_BUS_CMD_MASK		0xF8
#define VDRIVE_BUS_DATA_MASK	0xF0


#define VDRIVE_MAX_READ_DATA 	0xFF


#define CIA2_SERIAL_BUS 0xDD00

typedef enum {

	VDRIVE_IDLE,
	VDRIVE_LISTENING,
	VDRIVE_RECEIVE_BYTE,	
	VDRIVE_TX0,
	VDRIVE_TX1,
	VDRIVE_RX0,
	VDRIVE_RX1

} VDRIVE_STATE;



typedef struct {

	VDRIVE_STATE state;				// current drive state.
	VDRIVE_STATE returnstate;		// rx and tx return to a state after constructing a byte.
	byte lastbus;					// the last read of CIA2 $DD00

	byte data;						// last byte being received.

	//
	// primitives.
	//
	byte rx;						// the byte being received. 
	byte tx;						// the byte being transmitted.

} VDRIVE;

VDRIVE g_vdrive = {0};



//
// tx byte        		$ED40
// listen         		$ED6C
// wait for ack   		$ED77
// send byte command    $ED81

byte g_vdrive_kpatch_1[] = {
	0x40, 0xed, 0x78, 0x48, 0x20, 0x81, 0xed, 0x68, 0x48, 0x18, 0x2a, 0x18, 0x2a, 0x18, 0x2a, 0x18,
	0x2a, 0x0d, 0x00, 0xdd, 0x09, 0x08, 0x8d, 0x00, 0xdd, 0x20, 0x77, 0xed, 0x68, 0x48, 0x29, 0xf0,
	0x0d, 0x00, 0xdd, 0x09, 0x08, 0x8d, 0x00, 0xdd, 0x20, 0x77, 0xed, 0x68, 0x58, 0x60, 0xad, 0x00,
	0xdd, 0x09, 0xf8, 0x8d, 0x00, 0xdd, 0x4c, 0x77, 0xed, 0xad, 0x00, 0xdd, 0x29, 0xf8, 0xc9, 0x00,
	0xd0, 0xf7, 0x60, 0xad, 0x00, 0xdd, 0x09, 0x18, 0x8d, 0x00, 0xdd, 0x20, 0x77, 0xed, 0x60
};




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

void vdrive_receivebyte() {
	g_vdrive.state = VDRIVE_RX0;
	g_vdrive.returnstate = VDRIVE_RECEIVE_BYTE;
}

void vdrive_idle() {
	g_vdrive.state = VDRIVE_IDLE;
}

void vdrive_rx() {
	g_vdrive.state = VDRIVE_RX0;
}


void vdrive_init() {

	DEBUG_PRINT("** Initializing virtual drive.\n");
	c64_patch_kernal(sizeof(g_vdrive_kpatch_1),g_vdrive_kpatch_1);
	vdrive_idle();
}



void vdrive_listen() {
	g_vdrive.state = VDRIVE_LISTENING;
}


void vdrive_update() {

	byte b = vdrive_readbus();

	switch(g_vdrive.state) {

		case VDRIVE_IDLE:
			if (b == VDRIVE_COMMAND_LISTEN) {
				DEBUG_PRINT("Vdrive was idle, but commanded to listen.\n");
				vdrive_ack();
				vdrive_listen();
			}
		break;

		case VDRIVE_LISTENING:
			if (b == VDRIVE_COMMAND_RX) {
				DEBUG_PRINT("Vdrive commanded to receive a byte.\n");
				vdrive_ack();
				vdrive_receivebyte();
			}
		break;

		case VDRIVE_RECEIVE_BYTE:
			g_vdrive.data = g_vdrive.rx;
			vdrive_idle();
			DEBUG_PRINT("Vdrive received byte 0x%02X\n",g_vdrive.data);
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
	}
}


#include "emu.h"
#include "vdrive.h"