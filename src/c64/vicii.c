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
MODULE: vicii.c
VICII chip emulator source.


WORK ITEMS:

KNOWN BUGS:

*/

#include "emu.h"
#include "cpu.h"
#include "vicii.h"
#include "sysclock.h"

#define VICII_ICR_RASTER_INTERRUPT 				0b00000001
#define VICII_ICR_SPRITE_BACKGROUND_INTERRUPT	0b00000010
#define VICII_ICR_SPRITE_SPRITE_INTERRUPT		0b00000100
#define VICII_ICR_LIGHT_PEN_INTERRUPT			0b00001000 

#define VICII_NTSC_LINE_START_X			0x1A0
#define VICII_COLOR_MEM_BASE			0xD800


#define VICII_DISPLAY_TOP_0 			0x37
#define VICII_DISPLAY_TOP_1 			0x33
#define VICII_DISPLAY_BOTTOM_0  		0xf6
#define VICII_DISPLAY_BOTTOM_1  		0xfa
#define VICII_DISPLAY_LEFT_0 			0x27
#define VICII_DISPLAY_LEFT_1 			0x20
#define VICII_DISPLAY_RIGHT_0 	 		0x158
#define VICII_DISPLAY_RIGHT_1  			0x160


#define VICII_FIRST_DISPLAY_LINE 16


typedef struct {

	byte data;
	byte color;

} VICII_VIDEODATA;

typedef enum {
	VICII_S0X      			=0x00,  // S0X-S7X and S0Y-S7Y are X and Y positions for the seven HW Sprites.
	VICII_S0Y				=0x01,
	VICII_S1X				=0x02,
	VICII_S1Y				=0x03,
	VICII_S2X				=0x04,
	VICII_S2Y				=0x05,
	VICII_S3X				=0x06,
	VICII_S3Y				=0x07,
	VICII_S4X				=0x08,
	VICII_S4Y				=0x09,
	VICII_S5X				=0x0A,
	VICII_S5Y				=0x0B,
	VICII_S6X				=0x0C,
	VICII_S6Y				=0x0D,
	VICII_S7X				=0x0E,
	VICII_S7Y				=0x0F,
	VICII_SMSB				=0x10, // Sprite Most significant bits for Sprites 0-7 (bit by bite)
	VICII_CR1				=0x11, // Screen Control Register #1 
	VICII_RASTER			=0x12, // Current Raster position / Also latches Raster Interrupt Compare
	VICII_PENX				=0x13, // Light Pen X
	VICII_PENY				=0x14, // Light Pen Y
	VICII_SPRITEN			=0x15, // Sprite Enabled bit by bit
	VICII_CR2				=0x16, // Screen Control Register #2 
	VICII_SPRITEDH			=0x17, // Double Sprite Height, bit by bit 
	VICII_MEMSR				=0x18, // Memory Setup Register
	VICII_ISR				=0x19, // Interrupt Status Register
	VICII_ICR				=0x1A, // Interrupt Control Register
	VICII_SPRITEPRI			=0x1B, // Sprite Priority bit by bit
	VICII_SPRITEMCM			=0x1C, // Sprite Multi Color Mode bit by bit
	VICII_SPRITEDW			=0x1D, // Sprrite Double width bit by bit
	VICII_SSCOLLIDE			=0x1E, // Sprite to Sprite collisions
	VICII_SBCOLLIDE			=0x1F, // Sprite to background collisions
	VICII_BORDERCOL			=0x20, // Border color
	VICII_BACKCOL			=0x21, // background color
	VICII_EBACKCOL1			=0x22, // extra background color
	VICII_EBACKCOL2			=0x23, // extra background color
	VICII_EBACKCOL3			=0x24, // extra background color
	VICII_ESPRITECOL1		=0x25, // extra sprite color
	VICII_ESPRITECOL2		=0x26, // extra sprite color
	VICII_S0C				=0x27, // S0C - S07 Sprite Color
	VICII_S1C				=0x28, 
	VICII_S2C				=0x29, 
	VICII_S3C				=0x2A,
	VICII_S4C				=0x2B,
	VICII_S5C				=0x2C,
	VICII_S6C				=0x2D,
	VICII_S7C				=0x2E,
	VICII_UN0				=0x2F, // All unused below
	VICII_UN1				=0x30,
	VICII_UN2				=0x31,
	VICII_UN3				=0x32,
	VICII_UN4				=0x33,
	VICII_UN5				=0x34,
	VICII_UN6				=0x35,
	VICII_UN7				=0x36,
	VICII_UN8				=0x37,
	VICII_UN9				=0x38,
	VICII_UNA				=0x39,
	VICII_UNB				=0x3A,
	VICII_UNC				=0x3B,
	VICII_UND				=0x3C,
	VICII_UNE				=0x3D,
	VICII_UNF				=0x3E,
	VICII_UN10				=0x3F,	
	VICII_LAST
} VICII_REG;



//
// In this implementation, the three mode bits are combined into the low 3 bits of a mode value. 
// in the order ECM|BMM|MCM 
//
typedef enum {

	VICII_MODE_STANDARD_TEXT 		= 0x0,  		// 0 0 0 	
	VICII_MODE_MULTICOLOR_TEXT 		= 0x1,  		// 0 0 1 
	VICII_MODE_STANDARD_BITMAP 		= 0x2, 			// 0 1 0
	VICII_MODE_MULTICOLOR_BITMAP 	= 0x3, 			// 0 1 1 
	VICII_MODE_ECM_TEXT				= 0x4,			// 1 0 0 
	VICII_MODE_INVALID_TEXT 		= 0x5, 			// 1 0 1
	VICII_MODE_INVALID_BITMAP_1 	= 0x6, 			// 1 1 0
	VICII_MODE_INVALID_BITMAP_2		= 0x7,			// 1 1 1 
	VICII_MODE_LAST
} VICII_GRAPHICS_MODES;

typedef struct {

	byte regs[0x30];				// note some reg read/writes fall through to 
									// member variables below. 
	VICII_VIDEODATA data[40];		// copied during badlines.
	
	word raster_y;					// raster Y position -- CPU can get this through registers.
	word raster_irq;				// raster irq compare -- CPU can set this through registers.
	word raster_x;					// raster x position  -- VIC internal only. 
	
	bool badline;					// Vic needs extra cycles to fetch data during this line. 
	bool balow;						// BAlow stuns the CPU on badline situations.
	bool den;						// is display enabled? 
	bool idle; 						// idle or display state. 

	word bank;						// Base address for graphics addresses. 
	word vidmembase;				// video memory offset relative to graphics bank
	word charmembase;				// char memory offset relative to graphics bank
	word bitmapmembase;				// Bitmap memory offset.

	//
	// internal vic counters, side effect of other regiser writes can effect, but not directly accesible.
	//
	word vc; 						// (video counter -- 10 bit)
	word vcbase;					// (video counter base)
	byte rc;						// (row counter)
	byte vmli;						// (video matrix line index)


	//
	// graphics mode bits
	//
	byte mode;						// takes the BMM, MCM, and ECM bits and turns it into a number between 1 and 8.



	word  displaytop;				// these are the dimensions of the display window. 
	word  displaybottom; 			// they are slightly configurable (38 vs 40 columns, 24 vs 25 rows)
	word  displayleft;				// using vic registers;
	word  displayright;
	bool  mainborder;				// these are the two flip flops that control displaying the border.
	bool  vertborder;
	bool  displayline;				// if true, we are outside of vblanking lines.

	byte cycle;						// internal cycle count per line.


	//
	// the current bitmap frame.
	//
	byte lastchar;			// ready to render char (or extracolors in bitmap modes)
	byte lastcolor;			// ready to render color
	byte lastdata;			// ready to render data pattern

	VICII_SCREENFRAME out;
	word xpos;


} VICII;

VICII g_vic = {0};



bool vicii_stuncpu() 						{return g_vic.balow;}
void vicii_init() {

	//
	// BUGBUG: This magic value needs to be configurable by model and type of VICII
	// e.g. revision number, NTSC, PAL, etc.
	//
	g_vic.raster_x = VICII_NTSC_LINE_START_X; // starting X coordinate for an NTSC VICII.

	g_vic.displaytop 		= VICII_DISPLAY_TOP_0;
	g_vic.displaybottom 	= VICII_DISPLAY_BOTTOM_0;
	g_vic.displayleft 		= VICII_DISPLAY_LEFT_0;
	g_vic.displayright 		= VICII_DISPLAY_RIGHT_0;

}

void vicii_updateraster() {

	g_vic.raster_x +=8;										// Increment X raster position. wraps at 0x1FF
	if (g_vic.raster_x >= 0x1FF ) {
		g_vic.raster_x = 0;
	}

	if (g_vic.raster_x == VICII_NTSC_LINE_START_X) {		// Update Y raster position if we've reached end of line.
		
		g_vic.cycle = 1;
		g_vic.raster_y++;
		
		if (g_vic.raster_y == NTSC_LINES) {					//  End of screen, wrap to raster 0. 
			g_vic.raster_y = 0;
		}

		if (g_vic.raster_irq == g_vic.raster_y) {			// Check for Raster IRQ
			//
			// BUGBUG: not sure this gets cleared. 
			//
			g_vic.regs[VICII_ISR] |= VICII_ICR_RASTER_INTERRUPT;
			if (g_vic.regs[VICII_ICR] & VICII_ICR_RASTER_INTERRUPT) {
				cpu_irq();
			}
		}
	}
}


VICII_SCREENFRAME * vicii_getframe() {
	return &g_vic.out;
}


byte vic_realpeek(word address) {

	byte rval;
	switch(address & 0xF000) {
		case 0x1000: // character rom in bank 0
			rval = c64_charpeek(address & 0xFFF);
		break;
		case 0x8000: // character rom in bank 2
			rval = c64_charpeek(address & 0xFFF);
		break;
		default:
			rval = mem_peek(address); 
	}
	return rval;
}



//
// memory peeks are determined by the bankswitch in CIA2 and char memory base in the VIC MSR 
//
byte vic_peekchar(word address) {
	if (g_vic.mode != 0x4) {
		return vic_realpeek(g_vic.bank | g_vic.charmembase | address);
	}
	else {
		return vic_realpeek((g_vic.bank | g_vic.charmembase | address) & 0xF9FF);
	}
}

byte vic_peekbitmap(word address) {
	return vic_realpeek(g_vic.bank | g_vic.bitmapmembase | address);
}

//
// color always appears at VICII_COLOR_MEM_BASE in VIC space
//
byte vic_peekcolor(word address) {
	return mem_peek(VICII_COLOR_MEM_BASE | address);	
}

//
// memory peeks are determined by the bankswitch in CIA2 and video memory base in the VIC MSR 
//
byte vic_peekmem(word address) {
	return vic_realpeek(g_vic.bank | g_vic.vidmembase | address);
}

void vicii_drawpixel(byte c) {
	g_vic.out.data[g_vic.raster_y-VICII_NTSC_VBLANK][g_vic.xpos] = c;
	g_vic.xpos++;
}

void vicii_drawborder() {
	
	if (!g_vic.displayline) {return;}
	vicii_drawpixel(g_vic.regs[VICII_BORDERCOL]);
	vicii_drawpixel(g_vic.regs[VICII_BORDERCOL]);
	vicii_drawpixel(g_vic.regs[VICII_BORDERCOL]);
	vicii_drawpixel(g_vic.regs[VICII_BORDERCOL]);
	vicii_drawpixel(g_vic.regs[VICII_BORDERCOL]);
	vicii_drawpixel(g_vic.regs[VICII_BORDERCOL]);
	vicii_drawpixel(g_vic.regs[VICII_BORDERCOL]);
	vicii_drawpixel(g_vic.regs[VICII_BORDERCOL]);
}

void vicii_drawstandardtext() {
	int i;
	for (i = BIT_7;i;i>>=1) {	
		vicii_drawpixel((i & g_vic.lastchar) ? 
			(g_vic.lastcolor & 0xf) : (g_vic.regs[VICII_BACKCOL] & 0xf));
	}
}

void vicii_drawmulticolortext() {

	int i;
	byte c;

	if (g_vic.lastcolor & BIT_3) { // MC bit is on. 2 bits per pixel mode.
		for (i = 0; i < 4; i++) {
			switch (g_vic.lastchar & 0b11000000) {
				case 0b00000000: c = g_vic.regs[VICII_BACKCOL] & 0xf; break;
				case 0b01000000: c = g_vic.regs[VICII_EBACKCOL1] & 0xf; break;
				case 0b10000000: c = g_vic.regs[VICII_EBACKCOL2] & 0xf; break;
				case 0b11000000: c = g_vic.lastcolor & 0xf; break;
			}
			g_vic.lastchar <<= 2;
			vicii_drawpixel(c);
			vicii_drawpixel(c);
		}
	}
	else { // MC bit is off, treat like standard text. 1 bit per pixel, with color nibble. 
		vicii_drawstandardtext();
	}
}

void vicii_drawmulticolorbitmap() {

	byte c00 = g_vic.regs[VICII_BACKCOL] & 0xf;
	byte c01 = g_vic.lastchar & 0xf;
	byte c10 = g_vic.lastchar >> 4;
	byte c11 = g_vic.lastcolor & 0xf;
	byte i;
	byte c;

	for (i = 0; i < 4; i++) {
		switch (g_vic.lastdata & 0b11000000) {
			case 0b00000000: c = c00;break;
			case 0b01000000: c = c01;break;
			case 0b10000000: c = c10;break;
			case 0b11000000: c = c11;break;
		}
		g_vic.lastdata <<= 2;
		vicii_drawpixel(c);
		vicii_drawpixel(c);
	}
}

void vicii_drawstandardbitmap() {

	byte c0 = g_vic.lastchar;
	byte c1 = g_vic.lastchar  >> 4;

	int i;
	for (i = BIT_7;i;i>>=1) {	
		vicii_drawpixel((i & g_vic.lastdata) ? c1 &0xf: c0 &0xf);
	}	
}

void vicii_drawecmtext() {

	byte c100 = g_vic.lastcolor & 0xf;
	byte c000 = g_vic.regs[VICII_BACKCOL] & 0xf; 
	byte c001 = g_vic.regs[VICII_EBACKCOL1] & 0xf;
	byte c010 = g_vic.regs[VICII_EBACKCOL2] & 0xf;
	byte c011 = g_vic.regs[VICII_EBACKCOL3] & 0xf; 
	byte c;
	int i;


	for (i = BIT_7;i;i>>=1) {	
		if (i & g_vic.lastchar) { 
			c = c100;
		} else {
			switch (g_vic.lastdata) {
				case 0x00: c = c000;break;
				case 0x01: c = c001;break;
				case 0x10: c = c010;break;
				case 0x11: c = c011;break;
			}
		}

		vicii_drawpixel(c);
	}
}



void vicii_drawgraphics() {

	if (!g_vic.displayline) {return;}
	if (g_vic.vertborder) {vicii_drawborder();return;}
	if (g_vic.mainborder) {vicii_drawborder();return;}
	
	switch(g_vic.mode) {
		case VICII_MODE_STANDARD_TEXT: 		vicii_drawstandardtext(); 		break;
		case VICII_MODE_MULTICOLOR_TEXT: 	vicii_drawmulticolortext(); 	break;
		case VICII_MODE_STANDARD_BITMAP:	vicii_drawstandardbitmap();		break;
		case VICII_MODE_MULTICOLOR_BITMAP:	vicii_drawmulticolorbitmap();	break;
		case VICII_MODE_ECM_TEXT:			vicii_drawecmtext();			break;
	}

	// BUGBUG: ECM Text not working correctly. 
	// BUGBUG: Have not implemented "invalid" modes.


}


//
// read data from memory into vic buffer.
//



void vicii_caccess() {

	switch(g_vic.mode) {

		case VICII_MODE_STANDARD_TEXT:
		case VICII_MODE_MULTICOLOR_TEXT:
		case VICII_MODE_STANDARD_BITMAP:
		case VICII_MODE_MULTICOLOR_BITMAP:
			g_vic.data[g_vic.vmli].data 		= vic_peekmem(g_vic.vc);
			g_vic.data[g_vic.vmli].color 		= vic_peekcolor(g_vic.vc);
		break;
		default: break;
	}
}




void vicii_gaccess() {

	if (g_vic.idle) {return;}

	switch(g_vic.mode) {

		case VICII_MODE_STANDARD_TEXT:
		case VICII_MODE_MULTICOLOR_TEXT:
		case VICII_MODE_ECM_TEXT:
			g_vic.lastchar = vic_peekchar((((word) g_vic.data[g_vic.vmli].data) << 3) | g_vic.rc);
			g_vic.lastcolor = g_vic.data[g_vic.vmli].color & 0xf;
			g_vic.lastdata = g_vic.data[g_vic.vmli].data >> 6; // used in ECM mode
			
		break;
		case VICII_MODE_STANDARD_BITMAP:
		case VICII_MODE_MULTICOLOR_BITMAP:
			g_vic.lastcolor = g_vic.data[g_vic.vmli].color;  
			g_vic.lastchar = g_vic.data[g_vic.vmli].data;
			g_vic.lastdata = vic_peekbitmap(((word) g_vic.vc << 3)| g_vic.rc);
		break;
		default: break;
	}
	g_vic.vmli = (g_vic.vmli + 1) & 0x3F;
	g_vic.vc = (g_vic.vc + 1) & 0x3FF;

}

//
// all border flip flop logic in this function. May need to be broken into cycles later. 
//
vicii_checkborderflipflops() {

	if (g_vic.raster_x == g_vic.displayright)  {
		g_vic.mainborder =  true;
	}
	if (g_vic.cycle == 63 && g_vic.raster_y == g_vic.displaybottom) {
		g_vic.vertborder = true;
	}
	if (g_vic.cycle == 63 && g_vic.raster_y == g_vic.displaytop && g_vic.regs[VICII_CR1] & BIT_4) {
		g_vic.vertborder = false;
	}
	if (g_vic.raster_x == g_vic.displayleft && g_vic.raster_y == g_vic.displaybottom) {
		g_vic.vertborder = true;
	}
	if (g_vic.raster_x == g_vic.displayleft && g_vic.raster_y == g_vic.displaytop && g_vic.regs[VICII_CR1] & BIT_4) {
		g_vic.vertborder = false;
	}
	if (g_vic.raster_x == g_vic.displayleft && !g_vic.vertborder) {
		g_vic.mainborder = false;
	}
}


void vicii_main_update() {

	g_vic.cycle++;										// update cycle count.
	vicii_updateraster();								// update raster x and y and check for raster IRQ
	vicii_checkborderflipflops();

	switch(g_vic.cycle) {
		
		// * various setup activities. 
		case 1: 

			if (g_vic.raster_y == 0x30) {
				g_vic.den = g_vic.regs[VICII_CR1] & BIT_4;
			}

			g_vic.xpos = 0;

			if (g_vic.raster_y == 1) {
				g_vic.vcbase = 0;			// reset on line zero.
			}

			//
			// Check whether we should skip this line.
			//

			g_vic.displayline = g_vic.raster_y >= VICII_NTSC_VBLANK && 
				g_vic.raster_y <= NTSC_LINES;

			//
			// check to see if this is a badline.
			// (1) display is enabled
			// (2) beginning of a raster line
			// (3) between raster line 0x30 and 0xF7
			// (4) bottom 3 bits of raster_y == the SCROLLY bits in VICII_CR1
			//

			g_vic.badline = g_vic.den && g_vic.raster_y >= 0x30 && g_vic.raster_y <= 0xF7 && 
				(g_vic.raster_y & 0x7) == (g_vic.regs[VICII_CR1] & 0x7);

			if (g_vic.badline) {
				g_vic.idle = false;
			}
		
		break;
		case 2: 
		break;
		case 3: 
		break;
		case 4: 
		break;
		case 5: 
		break;
		case 6: 
		break;
		case 7: 
		break;
		case 8: 
		break;
		case 9: 
		break;
		case 10: 
		break;
		case 11:
		break;
		// * Set BA Low if BadLine 
		case 12: 
			if (g_vic.badline) {
				g_vic.balow = true;
			} 
		break;
		case 13:
			vicii_drawgraphics();
		break;
		// * reset internal indices on cycle 14.
		case 14: 
			vicii_drawgraphics();
			g_vic.vmli = 0;
			g_vic.vc = g_vic.vcbase;

			if (g_vic.badline) {
					g_vic.rc = 0;
			}
		break;
		// * start refreshing video matrix if balow.
		case 15:
			vicii_drawgraphics();
			vicii_caccess();
			break;
		case 16:
			vicii_drawgraphics();
			vicii_gaccess();
			vicii_caccess();
			break;
		case 17:
			vicii_drawgraphics();
			vicii_gaccess();
			vicii_caccess();
		break;
		case 18:
			vicii_drawgraphics();
			vicii_gaccess();
			vicii_caccess();
		break;
		case 19:
		case 20:
		case 21:
		case 22:
		case 23:
		case 24:
		case 25:
		case 26:
		case 27:
		case 28:
		case 29:
		case 30:
		case 31:
		case 32:
		case 33:
		case 34:
		case 35:
		case 36:
		case 37:
		case 38:
		case 39:
		case 40:
		case 41:
		case 42:
		case 43:
		case 44:
		case 45:
		case 46:
		case 47:
		case 48:
		case 49:
		case 50:
		case 51:
		case 52: 
		case 53: 
		case 54: 			
			vicii_drawgraphics();
			vicii_gaccess();
			vicii_caccess();
		break;
		// * turn off balow because of badline.
		case 55:
			g_vic.balow = false;
			vicii_drawgraphics();
			vicii_gaccess();	
		break;
		// * turn on border in 38 column mode.
		case 56: 
			vicii_drawgraphics();
		break;
		// * turn on border in 40 column mode.
		case 57:
			vicii_drawgraphics();//vicii_drawborder();
		break;
		// * reset vcbase if rc == 7. handle display/idle.
		case 58: 
			vicii_drawgraphics();//vicii_drawborder();
			if (g_vic.rc == 7) {
				g_vic.vcbase = g_vic.vc;
				g_vic.idle = true;
			}
			if (g_vic.badline || !g_vic.idle) {
				g_vic.idle = false;
				g_vic.rc = (g_vic.rc + 1) & 0x7;
			}

		break;
		case 59:
			vicii_drawgraphics(); 
		break;
		case 60: 
		break;
		case 61: 
		break;
		case 62: 
		break;
		// * turn on border. 
		case 63:

		break;
		case 64: 
		break;
		case 65: 
		break;
		default: 
		break;
	}
}

void vicii_update() {

	int i;
	int ticks = sysclock_getlastaddticks();

	for (i = 0; i < ticks; i++) {
		vicii_main_update();
	}
}

void vicii_destroy(){} 

void vicii_setbank() {
	g_vic.bank = ((~mem_peek(0xDD00)) & 0x03) << 6;
}

byte vicii_peek(word address) {
	
	byte reg = address % VICII_LAST;
	byte rval;
	switch(reg) {
		case VICII_RASTER: 
			rval = g_vic.raster_y & 0xFF; 
		break;
		case VICII_CR1:
			rval = g_vic.regs[VICII_CR1] | ((g_vic.raster_y & 0x100) >> 1);
		break;
		case VICII_CR2: 
			// B7 and B6 not connected.
			rval = BIT_7 | BIT_6 |  g_vic.regs[reg];
		break;
		case VICII_ISR: 
			// B6,B5,B4 not connected.
			rval = BIT_6 | BIT_5 | BIT_4 |  g_vic.regs[reg];
		break;
		case VICII_ICR: 
			// B7-B4 not connected.
			rval = BIT_7 | BIT_6 | BIT_5 | BIT_4 |  g_vic.regs[reg]; 
		break;

		case VICII_SBCOLLIDE: case VICII_SSCOLLIDE: // these registers are cleared on reading.
			//
			// BUGBUG: Monitor will destroy these values.
			//
			rval = g_vic.regs[reg];
			g_vic.regs[reg] = 0;
		break;

		case VICII_BORDERCOL: case VICII_BACKCOL: case VICII_EBACKCOL1:		
		case VICII_EBACKCOL2: case VICII_EBACKCOL3: case VICII_ESPRITECOL1: case VICII_ESPRITECOL2:		
		case VICII_S0C:	case VICII_S1C:	case VICII_S2C:	case VICII_S3C:				
		case VICII_S4C:	case VICII_S5C:	case VICII_S6C:	case VICII_S7C:
			// B7-B4 not connected.
			rval = BIT_7 | BIT_6 | BIT_5 | BIT_4 | g_vic.regs[reg];
		break;	

		case VICII_UN0: case VICII_UN1: case VICII_UN2: case VICII_UN3:
		case VICII_UN4: case VICII_UN5: case VICII_UN6: case VICII_UN7:
		case VICII_UN8: case VICII_UN9: case VICII_UNA: case VICII_UNB:
		case VICII_UNC: case VICII_UND: case VICII_UNE: case VICII_UNF: case VICII_UN10:
			rval = 0xff;
		break;



		default: rval = g_vic.regs[reg];
	}

	return rval;
}

void vicii_poke(word address,byte val) {
	byte reg = address % VICII_LAST;

	switch(reg) {

		case VICII_CR1: 	
			g_vic.regs[reg] = val;
			// latch bit 7. Its part of the irq raster compare. 
			g_vic.raster_irq = (g_vic.raster_irq & 0xFF) | (((word) val & BIT_7)<<1) ;
			// set the vertical screen height
			if (val & BIT_3) {
				DEBUG_PRINT("Screen Height is 1\n");
				g_vic.displaytop = VICII_DISPLAY_TOP_1;
				g_vic.displaybottom = VICII_DISPLAY_BOTTOM_1;
			}
			else {
				DEBUG_PRINT("Screen Height is 0\n");
				g_vic.displaytop = VICII_DISPLAY_TOP_0;
				g_vic.displaybottom = VICII_DISPLAY_BOTTOM_0;
			}

			// update graphics mode bits.
			g_vic.mode &= ~(BIT_2 | BIT_1);
			if (val & BIT_5) {
				g_vic.mode |= BIT_1;
			}
			if (val & BIT_6) {
				g_vic.mode |= BIT_2;
			}
			DEBUG_PRINT("CR1: Graphics Mode is %d\n",g_vic.mode);
		break;
		case VICII_CR2: 	
			g_vic.regs[reg] = val;
			// set the horizaontal screen width
			if (val & BIT_3) {
				DEBUG_PRINT("Screen Width is 1\n");
				g_vic.displayleft = VICII_DISPLAY_LEFT_1;
				g_vic.displayright = VICII_DISPLAY_RIGHT_1;
			}
			else {
				DEBUG_PRINT("Screen Width is 0\n");
				g_vic.displayleft = VICII_DISPLAY_LEFT_0;
				g_vic.displayright = VICII_DISPLAY_RIGHT_0;
			}

			g_vic.mode &= ~(BIT_0);
			if (val & BIT_4) {
				g_vic.mode |= BIT_0;
				
			}
			DEBUG_PRINT("CR2: Graphics Mode is %d\n",g_vic.mode);
		break;
		case VICII_RASTER: // latch raster line irq compare.
			g_vic.raster_irq = (g_vic.raster_irq & 0x0100) | val; 
			DEBUG_PRINT("Raster IRQ Set to %d\n",g_vic.raster_irq);
		break;

		case VICII_MEMSR: 
			g_vic.regs[reg] = val;
			g_vic.vidmembase = (val & (BIT_7 | BIT_6 | BIT_5 | BIT_4)) << 6;
			g_vic.charmembase = (val & (BIT_1 | BIT_2 | BIT_3)) << 10; 
			g_vic.bitmapmembase = (val & BIT_3) << 10;
			DEBUG_PRINT("Video Memory Base:      0x%04X\n",g_vic.vidmembase);
			DEBUG_PRINT("Character Memory Base:  0x%04X\n",g_vic.charmembase);
			DEBUG_PRINT("Bitmap Memory Base:     0x%04X\n",g_vic.bitmapmembase);


		break;
		case VICII_SBCOLLIDE: case VICII_SSCOLLIDE: // cannot write to collision registers
		break;
		case VICII_UN0: case VICII_UN1: case VICII_UN2: case VICII_UN3:
		case VICII_UN4: case VICII_UN5: case VICII_UN6: case VICII_UN7:
		case VICII_UN8: case VICII_UN9: case VICII_UNA: case VICII_UNB:
		case VICII_UNC: case VICII_UND: case VICII_UNE: case VICII_UNF: case VICII_UN10:
			// do nothing.
		break;

		default:g_vic.regs[reg] = val;break;
	}	
}


