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



					a
					t    
					t
		data 		n  unused
	   |---------| 	  |------|
$DD00: D4 D3 D2 D1 AB B2 B1 B0


Data is the top four bits of DD00 "data"
Attention is Bit 3 of DD00. "attn"
B0-B2 are unused by the bus



command nibbles:
0x10 				Rx Byte: Listener should prepare to receive a byte.
0x20				Tx Byte: Listener should prepare to send a byte.


Bus idle state: CPU in control, no active operations, ATTN is clear.


CPU Send Byte Sequence:

(Bus Idle)

	CPU [AB = 1] [DATA = 0x10]										// rx byte command
		Drive [AB = 0]												// acknowledge
	CPU [AB = 1] [DATA = lower nibble]								// lower nibble
		Drive [AB = 0]												// acknowledge
	CPU [AB = 1] [Data = upper nibble]								// upper nibble
		Drive [AB = 0]												// acknowledge

(Bus Idle)

Cpu Receive Byte Sequence:

(Bus Idle)

	CPU [AB = 1] [DATA = 0x20]										// tx byte command		
		Drive [AB = 0] 												// acknowledge
	Drive [AB = 1] [Data = 0x10]									// rx byte command
		CPU [AB = 0]												// acknowledge
	Drive [AB = 1] [Data = lower nibble]							// lower nibble
		CPU [AB = 0]												// acknowledge
	Drive [AB = 1] [Data = upper nibble]							// upper nibble
		CPU [AB = 0]												// acknowledge
(Bus Idle)


*/

#include "emu.h"
#include "vdrive.h"
#include "cpu.h"


#define VDRIVE_ATTN_BIT  		0x08
#define VDRIVE_DATA_MASK        0xF0
#define VDRIVE_RX_BYTE          0x10
#define VDRIVE_TX_BYTE          0x20
#define VDRIVE_BUS_BITS         (VDRIVE_ATTN_BIT | VDRIVE_DATA_MASK)


#define CIA2_SERIAL_BUS 0xDD00

typedef enum {

	VDRIVE_STATE_IDLE,
	VDRIVE_STATE_TX0,
	VDRIVE_STATE_TX1,
	VDRIVE_STATE_RX0,
	VDRIVE_STATE_RX1

} VDRIVE_STATE;



typedef struct {

	VDRIVE_STATE state;				// current drive state.
	byte rx;						// the byte being received. 
	byte tx;						// the byte being transmitted.

} VDRIVE;

VDRIVE g_vdrive = {0};




/*
         
--- Symbol List (sorted by symbol)

acknowledge              ed80              (R )
cleardata                ed98              (R )
sendbuscmd               ed8b              (R )
sendbyte                 ed40                  
waitforack               eda3              (R )
waitforattn              ed62                  
waitforrx                ed6c                  

--- End of Symbol List.
 
*/

byte g_vdrive_kpatch_1[] = {

0x40, 0xED, 0x78, 0x48, 0xA9, 0x08, 0x09, 0x10, 0x20, 0x8B, 0xED, 0x68, 0x48, 0x2A, 0x2A, 
0x2A, 0x2A, 0x29, 0xF0, 0x09, 0x08, 0x20, 0x8B, 0xED, 0x68, 0x48, 0x29, 0xF0, 0x09, 0x08, 
0x20, 0x8B, 0xED, 0x68, 0x58, 0x60, 0x48, 0xA9, 0x08, 0x2C, 0x00, 0xDD, 0xD0, 0xF9, 0x68, 
0x60, 0x48, 0xAD, 0x00, 0xDD, 0x29, 0x07, 0x09, 0x08, 0x09, 0x10, 0xCD, 0x00, 0xDD, 0xD0, 
0xF2, 0x20, 0x80, 0xED, 0x68, 0x60, 0x48, 0xAD, 0x00, 0xDD, 0x29, 0xF7, 0x8D, 0x00, 0xDD, 
0x68, 0x60, 0x20, 0x98, 0xED, 0x0D, 0x00, 0xDD, 0x8D, 0x00, 0xDD, 0x20, 0xA3, 0xED, 0x60, 
0x48, 0xAD, 0x00, 0xDD, 0x29, 0x0F, 0x8D, 0x00, 0xDD, 0x68, 0x60, 0x48, 0xA9, 0x08, 0x2C, 
0x00, 0xDD, 0xD0, 0xFB, 0x68, 0x60

};

//
// patch LISTEN entry point to NOP
//
byte g_vdrive_kpatch_2[] = {
	0x0c,0xed, 0x60
};
//
// change default CIA2 DDRB value
//
byte g_vdrive_kpatch_3[] = {
	0xd0,0xfd,0xa9,0xff
};

//
// change set serial clock high to nop
//
byte g_vdrive_kpatch_4[] = {
	0x85,0xEE,0x60
};

//
// change set serial clock low to nop
//
byte g_vdrive_kpatch_5[] = {
	0x8e,0xEE,0x60
};

//
// change set serial data high to nop
//
byte g_vdrive_kpatch_6[] = {
	0x97,0xEE,0x60
};

//
// change set serial data low to nop
//
byte g_vdrive_kpatch_7[] = {
	0xA0,0xEE,0x60
};

//
// change send byte to ignore deferred bytes.
//
byte g_vdrive_kpatch_8[] = {
	0xDD,0xED,0x20,0x40,0xED,0x60
};

//
// 
//
byte g_vdrive_kpatch_9[] = {
	0x13, 0xEE, 0x78, 0xA9, 0x08, 0x09, 0x20, 0x20, 0x8B, 0xED, 0x20, 0x6C, 0xED, 
	0x20, 0x62, 0xED, 0xAD, 0x00, 0xDD, 0x6A, 0x6A, 0x6A, 0x6A, 0x29, 0x0F, 0x85, 
	0xA4, 0x20, 0x80, 0xED, 0x20, 0x62, 0xED, 0x29, 0xF0, 0x05, 0xA4, 0x85, 0xA4, 
	0x20, 0x80, 0xED, 0x58, 0x60
};



void vdrive_writebus(byte b) {mem_poke (CIA2_SERIAL_BUS,(mem_peek(CIA2_SERIAL_BUS) & ~VDRIVE_BUS_BITS) | b);}
byte vdrive_readbus() {return mem_peek(CIA2_SERIAL_BUS) & VDRIVE_BUS_BITS;}

void vdrive_set_attention() {mem_poke(CIA2_SERIAL_BUS,mem_peek(CIA2_SERIAL_BUS) | VDRIVE_ATTN_BIT);}

void vdrive_clear_attention() {mem_poke(CIA2_SERIAL_BUS,mem_peek(CIA2_SERIAL_BUS) & ~VDRIVE_ATTN_BIT);}



void vdrive_init() {

	DEBUG_PRINT("** Initializing virtual drive.\n");
	c64_patch_kernal(sizeof(g_vdrive_kpatch_1),g_vdrive_kpatch_1);
	c64_patch_kernal(sizeof(g_vdrive_kpatch_2),g_vdrive_kpatch_2);
	c64_patch_kernal(sizeof(g_vdrive_kpatch_3),g_vdrive_kpatch_3);
	c64_patch_kernal(sizeof(g_vdrive_kpatch_4),g_vdrive_kpatch_4);
	c64_patch_kernal(sizeof(g_vdrive_kpatch_5),g_vdrive_kpatch_5);
	c64_patch_kernal(sizeof(g_vdrive_kpatch_6),g_vdrive_kpatch_6);
	c64_patch_kernal(sizeof(g_vdrive_kpatch_7),g_vdrive_kpatch_7);
	c64_patch_kernal(sizeof(g_vdrive_kpatch_8),g_vdrive_kpatch_8);
	c64_patch_kernal(sizeof(g_vdrive_kpatch_9),g_vdrive_kpatch_9);
	g_vdrive.state = VDRIVE_STATE_IDLE;

	//
	// c64_create_patch_array("asm/kpbusv2.prg");
	// printf("******\n");
	// c64_create_patch_array("asm/kpbusv2p2.prg");
	//
}

void vdrive_update() {

	byte b = vdrive_readbus();

	switch(g_vdrive.state) {

		case VDRIVE_STATE_IDLE:
			if (b == (VDRIVE_ATTN_BIT | VDRIVE_RX_BYTE)) {
				DEBUG_PRINT("Vdrive was idle, but commanded to receive a byte.\n");
				vdrive_clear_attention();
				g_vdrive.state = VDRIVE_STATE_RX0;
			}
		break;
		case VDRIVE_STATE_RX0:
			if (b & VDRIVE_ATTN_BIT) {
				g_vdrive.rx = b >> 4;
				g_vdrive.state = VDRIVE_STATE_RX1;
				vdrive_clear_attention();
			}
		break;
		case VDRIVE_STATE_RX1:
			if (b & VDRIVE_ATTN_BIT) {
				g_vdrive.rx |= (b & VDRIVE_DATA_MASK);
				DEBUG_PRINT("Vdrive received byte 0x%02X\n",g_vdrive.rx);
				g_vdrive.state = VDRIVE_STATE_IDLE;
				vdrive_clear_attention();
			}
		break;
	}
}


#include "emu.h"
#include "vdrive.h"