#include "emu.h"

#define VICII_ICR_RASTER_INTERRUPT 				0b00000001
#define VICII_ICR_SPRITE_BACKGROUND_INTERRUPT	0b00000010
#define VICII_ICR_SPRITE_SPRITE_INTERRUPT		0b00000100
#define VICII_ICR_LIGHT_PEN_INTERRUPT			0b00001000 

#define VICII_NTSC_LINE_START_X		0x1A0
#define VICII_COLOR_MEM_BASE		0xD800


#define VICII_DISPLAY_TOP_0 			0x37
#define VICII_DISPLAY_TOP_1 			0x33
#define VICII_DISPLAY_BOTTOM_0  		0xf6
#define VICII_DISPLAY_BOTTOM_1  		0xfa
#define VICII_DISPLAY_LEFT_0 			0x1F
#define VICII_DISPLAY_LEFT_1 			0x18
#define VICII_DISPLAY_RIGHT_0 	 		0x150
#define VICII_DISPLAY_RIGHT_1  			0x158





//
// using frodo limits.
//
#define VICII_FIRST_DISPLAY_LINE 0x10
#define VICII_LAST_DISPLAY_LINE  0xFB


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
	byte lastchar;			// ready to render char
	byte lastcolor;			// ready to render color

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

/*
void vicii_flip() {

	byte * t;
	t = g_vic.lastframe;
	g_vic.lastframe = g_vic.curframe;
	g_vic.curframe = t;
}
*/



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

void vicii_drawpixel(byte c) {

	g_vic.out.data[g_vic.raster_y][g_vic.xpos] = c;
	g_vic.xpos++;
}


void vicii_drawborder() {
	if (g_vic.displayline) {
		vicii_drawpixel(g_vic.regs[VICII_BORDERCOL]);
		vicii_drawpixel(g_vic.regs[VICII_BORDERCOL]);
		vicii_drawpixel(g_vic.regs[VICII_BORDERCOL]);
		vicii_drawpixel(g_vic.regs[VICII_BORDERCOL]);
		vicii_drawpixel(g_vic.regs[VICII_BORDERCOL]);
		vicii_drawpixel(g_vic.regs[VICII_BORDERCOL]);
		vicii_drawpixel(g_vic.regs[VICII_BORDERCOL]);
		vicii_drawpixel(g_vic.regs[VICII_BORDERCOL]);
	}
	
}


void vicii_drawgraphics() {

	byte i;
	byte c;

	if (!g_vic.displayline) {
		return;
	}

	if (g_vic.idle) {
		vicii_drawborder();
		return;
	}

	for (i = BIT_7;i;i>>=1) {
		vicii_drawpixel((i & g_vic.lastchar) ? 
			(g_vic.lastcolor & 0xf) : (g_vic.regs[VICII_BACKCOL] & 0xf));
	}

}
//
// read data from memory into vic buffer.
//
void vicii_caccess() {
	g_vic.data[g_vic.vmli].data 	= vic_peekmem(g_vic.vc);
	g_vic.data[g_vic.vmli].color 	= vic_peekcolor(g_vic.vc);
}


void vicii_gaccess() {

	if (g_vic.idle) {return;}

	g_vic.lastchar = vic_peekchar((((word) g_vic.data[g_vic.vmli].data) << 3) | g_vic.rc);
	g_vic.lastcolor = g_vic.data[g_vic.vmli].color & 0xf;

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


//
// I like the frodo method of a big switch by cycle. 
//
void vicii_update_three() {

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

			g_vic.displayline = g_vic.raster_y >= VICII_FIRST_DISPLAY_LINE && 
				g_vic.raster_y <= VICII_LAST_DISPLAY_LINE;

			
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
			vicii_drawborder(); 
		break;
		// * reset internal indices on cycle 14.
		case 14: 
			vicii_drawborder();
			g_vic.vmli = 0;
			g_vic.vc = g_vic.vcbase;

			if (g_vic.badline) {
					g_vic.rc = 0;
			}
		break;
		// * start refreshing video matrix if balow.
		case 15:
			vicii_drawborder();
			vicii_caccess();
			
			break;
		case 16:
			vicii_drawborder();
			vicii_gaccess();
			vicii_caccess();
			break;
		// turn border off in 40 column display.
		case 17:
			vicii_drawborder();
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
			//vicii_drawborder();
			
		break;
		// * turn on border in 40 column mode.
		case 57:
			//vicii_drawborder();
		
		break;
		// * reset vcbase if rc == 7. handle display/idle.
		case 58: 
			//vicii_drawborder();
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
			//vicii_drawborder();
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

		case VICII_CR1: 	
			g_vic.regs[reg] = val;
			// latch bit 7. Its part of the irq raster compare. 
			g_vic.raster_irq = (g_vic.raster_irq & 0xFF) | (((word) val & BIT_7)<<1) ;
			// set the vertical screen height
			if (val & BIT_3) {
				g_vic.displaytop = VICII_DISPLAY_TOP_1;
				g_vic.displaybottom = VICII_DISPLAY_BOTTOM_1;
			}
			else {
				g_vic.displaytop = VICII_DISPLAY_TOP_0;
				g_vic.displaybottom = VICII_DISPLAY_BOTTOM_0;
			}
		break;
		case VICII_CR2: 	
			g_vic.regs[reg] = val;
			// set the horizaontal screen width
			if (val & BIT_3) {
				g_vic.displayleft = VICII_DISPLAY_LEFT_1;
				g_vic.displayright = VICII_DISPLAY_RIGHT_1;
			}
			else {
				g_vic.displayleft = VICII_DISPLAY_LEFT_0;
				g_vic.displayright = VICII_DISPLAY_RIGHT_0;
			}
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


