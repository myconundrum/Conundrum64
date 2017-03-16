#include "emu.h"

#define VICII_ICR_RASTER_INTERRUPT 				0b00000001
#define VICII_ICR_SPRITE_BACKGROUND_INTERRUPT	0b00000010
#define VICII_ICR_SPRITE_SPRITE_INTERRUPT		0b00000100
#define VICII_ICR_LIGHT_PEN_INTERRUPT			0b00001000 

#define VICII_NTSC_LINE_START_X		0x1A0
#define VICII_COLOR_MEM_BASE		0xD800

typedef struct {

	byte data;
	byte color;

} VICII_VIDEODATA;

typedef struct {

	byte r;
	byte g;
	byte b;

} VICII_COLOR;



/*
	RGB values of C64 colors from 
	http://unusedino.de/ec64/technical/misc/vic656x/colors/

	0 black
  	1 white
  	2 red
  	3 cyan
  	4 pink
  	5 green
  	6 blue
  	7 yellow
  	8 orange
  	9 brown
 	10 light red
 	11 dark gray
 	12 medium gray
 	13 light green
 	14 light blue
 	15 light gray
*/

g_colors[0x10] = {
	{0x00, 0x00, 0x00},
	{0xFF, 0xFF, 0xFF},
	{0x68, 0x37, 0x2B},
	{0x70, 0xA4, 0xB2},
	{0x6F, 0x3D, 0x86},
	{0x58, 0x8D, 0x43},
	{0x35, 0x28, 0x79},
	{0xB8, 0xC7, 0x6F},
	{0x6F, 0x4F, 0x25},
	{0x43, 0x39, 0x00},
	{0x9A, 0x67, 0x59},
	{0x44, 0x44, 0x44},
	{0x6C, 0x6C, 0x6C},
	{0x9A, 0xD2, 0x84},
	{0x6C, 0x5E, 0xB5},
	{0x95, 0x95, 0x95}
};

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

	//
	// internal vic counters, side effect of other regiser writes can effect, but not directly accesible.
	//
	word vc; 						// (video counter -- 10 bit)
	word vcbase;					// (video counter base)
	byte rc;						// (row counter)
	byte vmli;						// (video matrix line index)

	//
	// internal cycle count per line.
	//
	byte cycle;

	//
	// the current bitmap frame.
	//
	VICII_COLOR frame[320*240];
	VICII_COLOR * curpixel;	
	

} VICII;

VICII g_vic = {0};

bool vicii_stuncpu() 						{return g_vic.balow;}
void vicii_init() {

	//
	// BUGBUG: This magic value needs to be configurable by model and type of VICII
	// e.g. revision number, NTSC, PAL, etc.
	//
	g_vic.raster_x = VICII_NTSC_LINE_START_X; // starting X coordinate for an NTSC VICII.

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


//
// memory peeks are determined by the bankswitch in CIA2 and char memory base in the VIC MSR 
//
byte vic_peekchar(word address) {

	word real_address = g_vic.bank | g_vic.charmembase | address;
	byte rval;

	switch(real_address & 0xF000) {
		case 0x1000: // character rom in bank 0
			rval = c64_charpeek(address & 0xFFF);
		break;
		case 0x8000: // character rom in bank 2
			rval = c64_charpeek(address & 0xFFF);
		break;
		default:
			rval = mem_peek(real_address); 
	}
	return rval;
}

//
// color always appears at VICII_COLOR_MEM_BASE in VIC space
//
byte vic_peekcolor(word address) {
	return mem_peek(VICII_COLOR_MEM_BASE | g_vic.vc);	
}

//
// memory peeks are determined by the bankswitch in CIA2 and video memory base in the VIC MSR 
//
byte vic_peekmem(word address) {
	return mem_peek(g_vic.bank | g_vic.vidmembase | address);
}

//
// read data from memory into vic buffer.
//
void vicii_caccess() {
	g_vic.data[g_vic.vmli].data 	= vic_peekmem(g_vic.vc);
	g_vic.data[g_vic.vmli].color 	= vic_peekcolor(g_vic.vc);
}

void vicii_gaccess() {
	byte pixels = vic_peekchar((((word) g_vic.data[g_vic.vmli].data) << 3) | g_vic.rc);
	g_vic.vmli = (g_vic.vmli + 1) & 0x3F;
	g_vic.vc = (g_vic.vc + 1) & 0x3FF;
}

//
// I like the frodo method of a big switch by cycle. 
//

void vicii_update_three() {

	g_vic.cycle++;										// update cycle count.
	vicii_updateraster();								// update raster x and y and check for raster IRQ
	
	switch(g_vic.cycle) {

		
		// * various setup activities. 
		case 1: 

			if (g_vic.raster_y == 0x30) {
				g_vic.den = g_vic.regs[VICII_CR1] & BIT_4;
			}

			if (g_vic.raster_y == 1) {
				g_vic.vcbase = 0;	// reset on line zero.
				g_vic.curpixel = g_vic.frame;
			}



			//
			// check to see if this is a badline.
			// (1) display is enabled
			// (2) beginning of a raster line
			// (3) between raster line 0x30 and 0xF7
			// (4) bottom 3 bits of raster_y == the SCROLLY bits in VICII_CR1
			//
			g_vic.badline = g_vic.den && g_vic.raster_y >= 0x30 && g_vic.raster_y <= 0xF7 && 
				(g_vic.raster_y & 0x7) == (g_vic.regs[VICII_CR1] & 0x7);
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
		break;
		// * reset internal indices on cycle 14.
		case 14: 
			g_vic.vmli = 0;
			g_vic.vc = g_vic.vcbase;

			if (g_vic.badline) {
					g_vic.rc = 0;
			}
		break;
		// * start refreshing video matrix if balow.
		case 15:
		case 16:
		case 17:
		case 18:
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
			vicii_caccess();
			vicii_gaccess();
		break;
		// * turn off balow because of badline.
		case 55: 
			g_vic.balow = false;	
		break;
		case 56: 
		break;
		case 57: 
		break;
		// * reset vcbase if rc == 7. handle display/idle.
		case 58: 
			
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
		break;
		case 60: 
		break;
		case 61: 
		break;
		case 62: 
		break;
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
		vicii_update_three();
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

		case VICII_CR1: 	// latch bit 7. Its part of the irq raster compare. 
			g_vic.regs[reg] = val;
			g_vic.raster_irq = (g_vic.raster_irq & 0xFF) | (((word) val & BIT_7)<<1) ;
		break;
		case VICII_RASTER: // latch raster line irq compare.
			g_vic.raster_irq = (g_vic.raster_irq & 0x0100) | val; 
		break;
		case VICII_MEMSR: 
			g_vic.regs[reg] = val;
			g_vic.vidmembase = (val & (BIT_7 | BIT_6 | BIT_5 | BIT_4)) << 6;
			g_vic.charmembase = (val & (BIT_1 | BIT_2 | BIT_3)) << 10; 

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


