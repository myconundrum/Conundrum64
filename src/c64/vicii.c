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



#define VICII_HEIGHT_NTSC 							263
#define VICII_WIDTH_NTSC 							520 
#define VICII_HEIGHT_PAL							312
#define VICII_HEIGHT_WIDTH							504

 
//
// SCREEN is the 40X25 display grid, does not include border. 
//
#define VICII_SCREEN_WIDTH 			320
#define VICII_SCREEN_HEIGHT 		200
#define VICII_VBLANK_TOP	 		24 


//
// new settings
//


//
// BUGBUG: Based on the Bauer doc, this is what I calculate the first visible pixels. 
// but, I don't like either hte complicated screen math or the 
// fat borders, so I'm trimming them down. Probably needs to be fixed to be accurate at some point.
//
//#define VICII_RASTER_X_FIRST_VISIBLE_PIXEL_PAL			0x1E0
//#define VICII_RASTER_X_FIRST_VISIBLE_PIXEL_NTSC			0x1E8
//#define VICII_RASTER_X_LAST_VISIBLE_PIXEL_PAL				0x180
//#define VICII_RASTER_X_LAST_VISIBLE_PIXEL_NTSC			0x180
//#define VICII_VISIBLE_BORDER_CYCLES						11


#define VICII_RASTER_X_FIRST_VISIBLE_PIXEL_PAL			0x0
#define VICII_RASTER_X_FIRST_VISIBLE_PIXEL_NTSC			0x0
#define VICII_RASTER_X_LAST_VISIBLE_PIXEL_PAL			0x170
#define VICII_RASTER_X_LAST_VISIBLE_PIXEL_NTSC			0x170
#define VICII_RASTER_Y_LAST_VISIBLE_LINE_PAL			280
#define VICII_RASTER_Y_LAST_VISIBLE_LINE_NTSC			262
#define VICII_VISIBLE_BORDER_CYCLES						6
#define VICII_CANVAS_WIDTH								320
#define VICII_RASTER_X_OVERFLOW_PAL						0x1F8
#define VICII_RASTER_X_OVERFLOW_NTSC					0x200
#define VICII_RASTER_X_START_PAL						0x190
#define VICII_RASTER_X_START_NTSC						0x198
#define VICII_40COL_LEFT								0x18
#define VICII_38COL_LEFT								0x20
#define VICII_40COL_RIGHT								0x158
#define VICII_38COL_RIGHT								0x150
#define VICII_25ROW_TOP									0x33
#define VICII_24ROW_TOP									0x37
#define VICII_25ROW_BOTTOM								0xFA
#define VICII_24ROW_BOTTOM								0xF6
#define VICII_FRAMEBUFFER_WIDTH							(VICII_CANVAS_WIDTH+VICII_VISIBLE_BORDER_CYCLES*8)
#define VICII_FRAMEBUFFER_HEIGHT_PAL					(VICII_RASTER_Y_LAST_VISIBLE_LINE_PAL+1 - VICII_VBLANK_TOP)
#define VICII_FRAMEBUFFER_HEIGHT_NTSC					(VICII_RASTER_Y_LAST_VISIBLE_LINE_NTSC+1 - VICII_VBLANK_TOP)




#define VICII_ICR_RASTER_INTERRUPT 				0b00000001
#define VICII_ICR_SPRITE_BACKGROUND_INTERRUPT	0b00000010
#define VICII_ICR_SPRITE_SPRITE_INTERRUPT		0b00000100
#define VICII_ICR_LIGHT_PEN_INTERRUPT			0b00001000 

#define VICII_COLOR_MEM_BASE			0xD800

/*
	RGB values of C64 colors from 
	http://unusedino.de/ec64/technical/misc/vic656x/colors/
	Alpha  R G B 

*/
uint32_t g_colors[0x10] = {
	0xFF000000, 				// 0 black
	0xFFFFFFFF,					// 1 white
	0xFF683728,					// 2 red
	0xFF70A4B2,					// 3 cyan
	0xFF6F3D86,					// 4 pink
	0xFF588D43,					// 5 green
	0xFF352879,					// 6 blue
	0xFFB8C76F,					// 7 yellow
	0xFF6F4F25,					// 8 orange
	0xFF433900,					// 9 brown
	0xFF9A6759,					// 10 light red
	0xFF444444,					// 11 dark gray
	0xFF6C6C6C,					// 12 medium gray
	0xFF9AD284,					// 13 light green
	0xFF6C5EB5,					// 14 light blue
	0xFF959595					// 15 light gray
};


typedef struct {

	byte data;
	byte color;

} VICII_VIDEODATA;
							//$D000 + value below.
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
	VICII_SPRITEEN			=0x15, // Sprite Enabled bit by bit
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


typedef enum {
	VICII_FG_PIXEL,
	VICII_BG_PIXEL,
	VICII_BORDER_PIXEL,
	VICII_SPRITE_PIXEL
} VICII_PIXELTYPE;


typedef struct {

	byte pointer;  		// fetched in paccess 	for sprite.
	byte data[3];  		// fetched in saccesses for sprite.
	byte idata;			// index of data byte.
	byte bitstodraw;    // how many bits are there to read?

	byte mc;		
	byte mcbase;

	word curx;			// where to draw next pixel

	bool on;			// if on, we are displaying this sprite.
	bool dma; 			// if true, we are loading data for this sprite.
	bool yex;			// y expansion flip flop logic.		
	bool fgpri;			// if true, foreground has priority over this sprite.
	bool dw;			// if true, this sprite is double width.

} VICII_SPRITE;


typedef struct {

	byte regs[0x30];				// note some reg read/writes fall through to 
									// member variables below. 

	VICII_SPRITE sprites[8];		// per sprite data.
	VICII_VIDEODATA data[40];		// copied during badlines.
	
	word raster_y;					// raster Y position -- CPU can get this through registers.
	word raster_irq;				// raster irq compare -- CPU can set this through registers.
	word raster_x;					// raster x position  -- VIC internal only. 

	bool hblank; 					// are we in horizontal blanking?
	bool badline;					// Vic needs extra cycles to fetch data during this line. 
	bool balow;						// BAlow stuns the CPU on badline situations.
	bool den;						// is display enabled? 
	bool idle; 						// idle or display state. 

	bool irqsprite;					// sprite-sprite collision irq is allowed.
	bool irqbackground;				// sprite-background collision irq is allowed.

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

	uint32_t ** out;			// colors for each pixel.
	byte ** type; 				// pixel type.
	word screenheight;			// varies by NTSC and PAL. Height of screen frame.
	word screenwidth;			// varies by NTSC and PAL. width of screen frame.
	word linestart_x; 		    // varies by NTSC and PAL. Value of x at start of a raster line.
	word rasterlines;			// varies by NTSC and PAL. How many raster lines?
	word firstvisible;			// varies by NTSC and PAL. First visible pixel on a line.
	word lastvisible;			// varies by NTSC and PAL. Last visible pixel on a line.
	word raster_x_overflow;		// varies by NTSC and PAL. where do x coordinates wrap on a line.
	word lastvisibleraster;		// varies by NTSC and PAL. where is the last visible raster?
	
	
	bool frameready; 			// ready when a new frame is being generated.
	

} VICII;

VICII g_vic = {0};



bool vicii_stuncpu() 			{return g_vic.balow;}
word vicii_getscreenheight() 	{return g_vic.screenheight;}
word vicii_getscreenwidth() 	{return g_vic.screenwidth;}

void vicii_init() {

	DEBUG_PRINT("** Initializing VICII...\n");




	g_vic.screenwidth = VICII_FRAMEBUFFER_WIDTH;

	if (sysclock_isNTSCfrequency()) {
		
		DEBUG_PRINT("VICII initialized in NTSC mode.\n");

		g_vic.screenheight 				= VICII_FRAMEBUFFER_HEIGHT_NTSC;
		g_vic.linestart_x 				= VICII_RASTER_X_START_NTSC;
		g_vic.rasterlines 				= NTSC_LINES;
		g_vic.firstvisible 				= VICII_RASTER_X_FIRST_VISIBLE_PIXEL_NTSC;
		g_vic.lastvisible 				= VICII_RASTER_X_LAST_VISIBLE_PIXEL_NTSC;
		g_vic.raster_x_overflow 		= VICII_RASTER_X_OVERFLOW_NTSC;
		g_vic.lastvisibleraster			= VICII_RASTER_Y_LAST_VISIBLE_LINE_NTSC;

	}
	else {

		DEBUG_PRINT("VICII initialized in PAL mode.\n");
		g_vic.screenheight 				= VICII_FRAMEBUFFER_HEIGHT_PAL;
		g_vic.linestart_x 				= VICII_RASTER_X_START_PAL;
		g_vic.rasterlines 				= PAL_LINES;
		g_vic.firstvisible 				= VICII_RASTER_X_FIRST_VISIBLE_PIXEL_PAL;
		g_vic.lastvisible 				= VICII_RASTER_X_LAST_VISIBLE_PIXEL_PAL;
		g_vic.raster_x_overflow 		= VICII_RASTER_X_OVERFLOW_PAL;
		g_vic.lastvisibleraster			= VICII_RASTER_Y_LAST_VISIBLE_LINE_PAL;
	}

	g_vic.out = malloc(sizeof(uint32_t *)*g_vic.screenheight);
	g_vic.type = malloc(sizeof(byte *)*g_vic.screenheight);

	if(!g_vic.out || !g_vic.type) {
		FATAL_ERROR("Fatal error initializing emulator graphics.\n");
	}
	for (int i = 0 ; i < g_vic.screenheight; i++) {

		g_vic.out[i] = malloc(sizeof(uint32_t) * g_vic.screenwidth);
		g_vic.type[i] = malloc(sizeof(byte) * g_vic.screenwidth);
		if(!g_vic.out[i] || !g_vic.type[i]) {
			FATAL_ERROR("Fatal error initializing emulator graphics.\n");
		}
	}


	g_vic.raster_x = g_vic.linestart_x; 


	g_vic.displaytop 		= VICII_25ROW_TOP;
	g_vic.displaybottom 	= VICII_25ROW_BOTTOM;
	g_vic.displayleft 		= VICII_40COL_LEFT;
	g_vic.displayright 		= VICII_40COL_RIGHT;

	if (!g_vic.out || !g_vic.type) {
		FATAL_ERROR("Fatal error intiailizing emulator graphics..\n");
	}
}

bool vicii_frameready() {return g_vic.frameready;}

void vicii_updateraster() {

									// Increment X raster position. wraps at 0x1FF
	if (g_vic.raster_x == g_vic.raster_x_overflow ) {
		g_vic.raster_x = 0;
	}

	if (g_vic.raster_x == g_vic.lastvisible) {
		g_vic.hblank = true;
	}
	if (g_vic.raster_x == g_vic.firstvisible) {
		g_vic.hblank = false;
	}



	if (g_vic.raster_x == g_vic.linestart_x) {		// Update Y raster position if we've reached end of line.
		g_vic.cycle = 1;
		g_vic.raster_y++;
		
		if (g_vic.raster_y == g_vic.rasterlines) {					//  End of screen, wrap to raster 0. 
			g_vic.raster_y = 0;
		}

		if (g_vic.raster_irq == g_vic.raster_y) {			// Check for Raster IRQ
			//
			// BUGBUG: not sure this gets cleared. 
			//
			g_vic.regs[VICII_ISR] |= VICII_ICR_RASTER_INTERRUPT;
			if (g_vic.regs[VICII_ICR] & VICII_ICR_RASTER_INTERRUPT) {
				DEBUG_PRINT("VICII is signalling raster irq.\n");
				cpu_irq();
			}
		}
	}
}


uint32_t ** vicii_getframe() {
	return g_vic.out;
}


byte vicii_realpeek(word address) {

	byte rval;
	switch(address & 0xF000) {
		case 0x1000: // character rom in bank 0
			rval = c64_charpeek(address & 0xFFF);
		break;
		case 0x8000: // character rom in bank 2
			rval = c64_charpeek(address & 0xFFF);
		break;
		default:
			rval = mem_nonmappable_peek(address); 
	}
	return rval;
}


//
// memory peeks are determined by the bankswitch in CIA2 and char memory base in the VIC MSR 
//
byte vicii_peekchar(word address) {
	if (g_vic.mode != 0x4) {
		return vicii_realpeek(g_vic.bank | g_vic.charmembase | address);
	}
	else {
		return vicii_realpeek((g_vic.bank | g_vic.charmembase | address) & 0xF9FF);
	}
}

byte vicii_peekbitmap(word address) {
	return vicii_realpeek(g_vic.bank | g_vic.bitmapmembase | address);
}

//
// color always appears at VICII_COLOR_MEM_BASE in VIC space
//
byte vicii_peekcolor(word address) {
	return mem_peek(VICII_COLOR_MEM_BASE | address);	
}

byte vicii_peekspritepointer(word address) {

	return vicii_realpeek(g_vic.bank | g_vic.vidmembase | address | 0x3F8);
}

byte vicii_peekspritedata(word sprite) {

	return vicii_realpeek(g_vic.bank | ((word)g_vic.sprites[sprite].pointer) << 6 | 
		g_vic.sprites[sprite].mc);
}

//
// memory peeks are determined by the bankswitch in CIA2 and video memory base in the VIC MSR 
//
byte vicii_peekmem(word address) {
	return vicii_realpeek(g_vic.bank | g_vic.vidmembase | address);
}

void vicii_drawpixel(byte c,VICII_PIXELTYPE type) {
	
	g_vic.out[g_vic.raster_y-VICII_VBLANK_TOP][g_vic.raster_x] = g_colors[c];	
	g_vic.type[g_vic.raster_y-VICII_VBLANK_TOP][g_vic.raster_x++] = type;

}

void vicii_checkcollisioninterrupt(byte bit,bool *cantrigger) {
	g_vic.regs[VICII_ISR] |= BIT_7;
	g_vic.regs[VICII_ISR] |= bit;

	if ((g_vic.regs[VICII_ICR] & bit) && *cantrigger) {
		*cantrigger = false;
		cpu_irq();
	}
}

void vicii_drawspritepixel(byte sprite,word row, byte c,bool shown) {

	VICII_SPRITE * s = &g_vic.sprites[sprite];

	if (shown) {

		//
		// sprite pixels can cause collision detection with other pixel types.
		// if enabled, this may cause an interrupt.
		//
		if (g_vic.type[row][s->curx] == VICII_FG_PIXEL) {
			g_vic.regs[VICII_SBCOLLIDE] |= (1 << sprite);
			vicii_checkcollisioninterrupt(BIT_1,&g_vic.irqbackground);
		} else if (g_vic.type[row][s->curx] == VICII_SPRITE_PIXEL) {
			g_vic.regs[VICII_SSCOLLIDE] |= (1 << sprite);
			vicii_checkcollisioninterrupt(BIT_2,&g_vic.irqsprite);
		}
	
		if (!s->fgpri || g_vic.type[row][s->curx] != VICII_FG_PIXEL) {
			g_vic.out[row][s->curx] = g_colors[c];
			g_vic.type[row][s->curx] = VICII_SPRITE_PIXEL;

		}


	}

	s->curx++;
}

void vicii_drawborder() {
	
	if (!g_vic.displayline) {return;}
	vicii_drawpixel(g_vic.regs[VICII_BORDERCOL],VICII_BORDER_PIXEL);
	vicii_drawpixel(g_vic.regs[VICII_BORDERCOL],VICII_BORDER_PIXEL);
	vicii_drawpixel(g_vic.regs[VICII_BORDERCOL],VICII_BORDER_PIXEL);
	vicii_drawpixel(g_vic.regs[VICII_BORDERCOL],VICII_BORDER_PIXEL);
	vicii_drawpixel(g_vic.regs[VICII_BORDERCOL],VICII_BORDER_PIXEL);
	vicii_drawpixel(g_vic.regs[VICII_BORDERCOL],VICII_BORDER_PIXEL);
	vicii_drawpixel(g_vic.regs[VICII_BORDERCOL],VICII_BORDER_PIXEL);
	vicii_drawpixel(g_vic.regs[VICII_BORDERCOL],VICII_BORDER_PIXEL);
}

void vicii_drawstandardtext() {
	int i;
	VICII_PIXELTYPE type;
	for (i = BIT_7;i;i>>=1) {
		type = (i & g_vic.lastchar) ? VICII_FG_PIXEL : VICII_BG_PIXEL;
		vicii_drawpixel(type == VICII_FG_PIXEL ? (g_vic.lastcolor & 0xf) : (g_vic.regs[VICII_BACKCOL] & 0xf),type);
	}
}

void vicii_drawmulticolortext() {

	int i;
	byte c;
	VICII_PIXELTYPE type;

	if (g_vic.lastcolor & BIT_3) { // MC bit is on. 2 bits per pixel mode.
		for (i = 0; i < 4; i++) {
			type = VICII_FG_PIXEL;
			switch (g_vic.lastchar & 0b11000000) {
				case 0b00000000: c = g_vic.regs[VICII_BACKCOL] & 0xf; 	type = VICII_BG_PIXEL; break;
				case 0b01000000: c = g_vic.regs[VICII_EBACKCOL1] & 0xf;	type = VICII_BG_PIXEL; break;
				case 0b10000000: c = g_vic.regs[VICII_EBACKCOL2] & 0xf; break;
				case 0b11000000: c = g_vic.lastcolor & 0xf; break;
			}
			g_vic.lastchar <<= 2;
			vicii_drawpixel(c,type);
			vicii_drawpixel(c,type);
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
	VICII_PIXELTYPE type;


	for (i = 0; i < 4; i++) {

		type = VICII_FG_PIXEL;

		switch (g_vic.lastdata & 0b11000000) {
			case 0b00000000: c = c00; type = VICII_BG_PIXEL; break;
			case 0b01000000: c = c01; type = VICII_BG_PIXEL; break;
			case 0b10000000: c = c10;break;
			case 0b11000000: c = c11;break;
		}
		g_vic.lastdata <<= 2;
		vicii_drawpixel(c,type);
		vicii_drawpixel(c,type);
	}
}

void vicii_drawstandardbitmap() {

	byte c0 = g_vic.lastchar;
	byte c1 = g_vic.lastchar  >> 4;
	VICII_PIXELTYPE type; 

	int i;
	for (i = BIT_7;i;i>>=1) {
		type = i & g_vic.lastdata ? VICII_FG_PIXEL: VICII_BG_PIXEL;	
		vicii_drawpixel( type == VICII_FG_PIXEL? c1 &0xf: c0 &0xf,type );
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
	VICII_PIXELTYPE type;


	for (i = BIT_7;i;i>>=1) {	
		if (i & g_vic.lastchar) { 
			c = c100;
		} else {
			type = VICII_FG_PIXEL;
			switch (g_vic.lastdata) {
				case 0x00: c = c000; type = VICII_BG_PIXEL;break;
				case 0x01: c = c001; type = VICII_BG_PIXEL;break;
				case 0x10: c = c010;break;
				case 0x11: c = c011;break;
			}
		}
		vicii_drawpixel(c,type);
	}
}

/*
6. If the sprite display for a sprite is turned on, the shift register is
   shifted left by one bit with every pixel as soon as the current X
   coordinate of the raster beam matches the X coordinate of the sprite
   (even registers $d000-$d00e), and the bits that "fall off" are
   displayed. If the MxXE bit belonging to the sprite in register $d01d is
   set, the shift is done only every second pixel and the sprite appears
   twice as wide. If the sprite is in multicolor mode, every two adjacent
   bits form one pixel.
 */


void vicii_drawstandardspritebyte(byte sprite) {

	int i;
	byte data = g_vic.sprites[sprite].data[g_vic.sprites[sprite].idata++];
	byte c = g_vic.regs[VICII_S0C+sprite] & 0xf;
	word y = g_vic.raster_y - VICII_VBLANK_TOP;
	
	for (i = 0; i < 8; i++) {

		if (g_vic.sprites[sprite].dw) {
			vicii_drawspritepixel(sprite,y,c,data&0x80);
		}
		vicii_drawspritepixel(sprite,y,c,data&0x80);

		data <<= 1;
	}
	
	if (g_vic.sprites[sprite].idata == 3) {
		g_vic.sprites[sprite].idata = 0;
	}
}

void vicii_drawmulticolorspritebyte(byte sprite) {

	int i;
	byte data = g_vic.sprites[sprite].data[g_vic.sprites[sprite].idata++];
	byte c 		= g_vic.regs[VICII_S0C+sprite] 	& 0xf;
	byte ec1 	= g_vic.regs[VICII_ESPRITECOL1] & 0xf;
	byte ec2 	= g_vic.regs[VICII_ESPRITECOL2] & 0xf;
	int uc;  // used color (which color to use for multicolor mode)
	word y = g_vic.raster_y - VICII_VBLANK_TOP;

	for (i = 0; i < 4; i++) {
		
		switch(data & 0b11000000) {
			case 0b00000000:	uc = -1;		break;/*transparent*/ 
			case 0b01000000:	uc = ec1;		break;
			case 0b10000000:	uc = c;			break;
			case 0b11000000:	uc = ec2; 		break;
		}

		if (g_vic.sprites[sprite].dw) {
			vicii_drawspritepixel(sprite,y,uc,uc!=-1);
			vicii_drawspritepixel(sprite,y,uc,uc!=-1);
		}
		vicii_drawspritepixel(sprite,y,uc,uc!=-1);
		vicii_drawspritepixel(sprite,y,uc,uc!=-1);
		data <<=2;
	}

	if (g_vic.sprites[sprite].idata == 3) {
		g_vic.sprites[sprite].idata = 0;
	}
}


void vicii_drawsprites() {

	int sprite;
	byte mcm;
	
	if (!g_vic.displayline) {return;}
	
	//
	// count down sprites from high to low. Lower sprite numbers have higher visibility priority.
	// so this will ensure that the highest priority sprite are drawn in the case where they overlap.
	//
	for (sprite = 7; sprite >= 0; sprite--) {
		
		if (g_vic.sprites[sprite].on == false) {
			continue;
		}

		g_vic.sprites[sprite].curx = g_vic.regs[VICII_S0X + sprite*2];
		g_vic.sprites[sprite].curx |= (g_vic.regs[VICII_SMSB] & (0x1 << sprite)) ? 0x0100 : 0;


		mcm = g_vic.regs[VICII_SPRITEMCM] & (0x1 << sprite);

		while (g_vic.sprites[sprite].bitstodraw) {
			g_vic.sprites[sprite].bitstodraw -= 8;

			if (mcm) {
				vicii_drawmulticolorspritebyte(sprite);
			} else {  
				vicii_drawstandardspritebyte(sprite); 
			}		
		}
	}
}

void vicii_drawgraphics() {


	//
	// if this is in a non display area, just move the raster beam forward 8 pixels.
	// BUGBUG: Not handling idle mode access correctly.
	//

	if (g_vic.hblank || !g_vic.displayline) {
		g_vic.raster_x+=8;
		return;
	}

	if (g_vic.vertborder || g_vic.mainborder) {
		vicii_drawborder();
		return;
	}
	
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

	g_vic.data[g_vic.vmli].data 		= vicii_peekmem(g_vic.vc);
	g_vic.data[g_vic.vmli].color 		= vicii_peekcolor(g_vic.vc);		
}

void vicii_gaccess() {

	if (g_vic.idle) {return;}

	switch(g_vic.mode) {

		case VICII_MODE_STANDARD_TEXT:
		case VICII_MODE_MULTICOLOR_TEXT:
		case VICII_MODE_ECM_TEXT:
			g_vic.lastchar = vicii_peekchar((((word) g_vic.data[g_vic.vmli].data) << 3) | g_vic.rc);
			g_vic.lastcolor = g_vic.data[g_vic.vmli].color & 0xf;
			g_vic.lastdata = g_vic.data[g_vic.vmli].data >> 6; // used in ECM mode
			
		break;
		case VICII_MODE_STANDARD_BITMAP:
		case VICII_MODE_MULTICOLOR_BITMAP:
			g_vic.lastcolor = g_vic.data[g_vic.vmli].color;  
			g_vic.lastchar = g_vic.data[g_vic.vmli].data;
			g_vic.lastdata = vicii_peekbitmap(((word) g_vic.vc << 3)| g_vic.rc);
		break;
		default: break;
	}
	g_vic.vmli = (g_vic.vmli + 1) & 0x3F;
	g_vic.vc = (g_vic.vc + 1) & 0x3FF;
}

//
// sprite pointer access. Always done, even if sprite DMA isn't on.
//
void vicii_paccess(byte num) {g_vic.sprites[num].pointer = vicii_peekspritepointer(num);} 

void vicii_saccess(byte num) {

	if (g_vic.sprites[num].dma) {
		g_vic.sprites[num].data[g_vic.sprites[num].idata++] = vicii_peekspritedata(num);
		g_vic.sprites[num].bitstodraw+=8;
	}
	if (g_vic.sprites[num].idata == 3) {
		g_vic.sprites[num].idata = 0;
	}

	g_vic.sprites[num].mc++;
	g_vic.sprites[num].mc &= 0x3F;
}	

/*
3. In the first phases of cycle 55 and 56, the VIC checks for every sprite
   if the corresponding MxE bit in register $d015 is set and the Y
   coordinate of the sprite (odd registers $d001-$d00f) match the lower 8
   bits of RASTER. If this is the case and the DMA for the sprite is still
   off, the DMA is switched on, MCBASE is cleared, and if the MxYE bit is
   set the expansion flip flip is reset.
*/

void vicii_checkspritesdmaon() {

	int i;

	for (i = 0; i < 8; i++) {

		if ((g_vic.regs[VICII_SPRITEEN] & (0x1 << i)) && 
			g_vic.regs[VICII_S0Y+i*2] == (g_vic.raster_y & 0xFF)) {
			if (!g_vic.sprites[i].dma) {
				g_vic.sprites[i].dma = true;
				g_vic.sprites[i].bitstodraw =0;
				g_vic.sprites[i].mcbase = 0;

				if (g_vic.regs[VICII_SPRITEDH] & (0x1 << i)) {
					g_vic.sprites[i].yex = false;
				}
			}
		}
	}
}

/* 

This debuggging addendum supercedes rules 7 and 8 from the text. Pulled from vice source.


-------------
Rules 7 and 8 in the article section 3.8.1 do not cover sprite crunch in full.
A more accurate replacement for both rules:

7. In the first phase of cycle 16, it is checked if the expansion flip flop
   is set. If so, MCBASE load from MC (MC->MCBASE), unless the CPU cleared
   the Y expansion bit in $d017 in the second phase of cycle 15, in which case
   MCBASE is set to X = (101010 & (MCBASE & MC)) | (010101 & (MCBASE | MC)).
   After the MCBASE update, the VIC checks if MCBASE is equal to 63 and turns
   off the DMA of the sprite if it is.

Note: The original rule 8 mentions turning the display of the sprite off
if MCBASE is equal to 63. If this were true, then the last line of the sprite
would not be displayed beyond coordinates corresponsing to cycle 16.
The above rewritten rule corrects this. The actual disabling of sprite display
is likely handled during the first phase of cycle 58 (see rule 4).


*/

//
// BUGBUG: Not handling the cpu clear in the second phase of cycle 15 yet.
//
void vicii_checkspritesdmaoff() {

 	int i;

 	for (i = 0; i < 8; i++) {
 		if (g_vic.sprites[i].yex) {

 			g_vic.sprites[i].mcbase = g_vic.sprites[i].mc;
 			if (g_vic.sprites[i].mcbase == 63) {
 				g_vic.sprites[i].dma = false;

 			}
 		}
 	}
}

/*

4. In the first phase of cycle 58, the MC of every sprite is loaded from
   its belonging MCBASE (MCBASE->MC) and it is checked if the DMA for the
   sprite is turned on and the Y coordinate of the sprite matches the lower
   8 bits of RASTER. If this is the case, the display of the sprite is
   turned on.

*/

void vicii_checkspriteson() {

 	int i;

 	for (i = 0; i < 8; i++) {
 		g_vic.sprites[i].mc = g_vic.sprites[i].mcbase;
 		if (g_vic.sprites[i].dma && g_vic.regs[VICII_S0Y+i*2] == (g_vic.raster_y & 0xFF)) {
 			g_vic.sprites[i].on = true;
 		}
 		if (!g_vic.sprites[i].dma) {
 			g_vic.sprites[i].on = false;
 		}
 	}
}

//
// all border flip flop logic in this function. May need to be broken into cycles later. 
//
void vicii_checkborderflipflops() {

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

	g_vic.cycle++;			// update cycle count.

	vicii_updateraster(); 	// update raster x and y and check for raster IRQ
	vicii_checkborderflipflops();

	
	switch(g_vic.cycle) {


		
		// * various setup activities. 
		case 1: 

			if (g_vic.raster_y == 0x30) {
				g_vic.den = g_vic.regs[VICII_CR1] & BIT_4;
			}

			if (g_vic.raster_y == 1) {
				g_vic.frameready = true;    // signal ux system to draw frame. 
				g_vic.vcbase = 0;			// reset on line zero.
			}

			//
			// Check whether we should skip this line.
			//
			g_vic.displayline = g_vic.raster_y >= VICII_VBLANK_TOP && 
				g_vic.raster_y <= g_vic.lastvisibleraster;

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
			vicii_paccess(3);
			vicii_saccess(3);
		break;
		case 2: 
			if (g_vic.raster_y == 1) {
				g_vic.frameready = false;
			}
			vicii_saccess(3);
			vicii_saccess(3);
		break;
		case 3: 
			vicii_paccess(4);
			vicii_saccess(4);
		break;
		case 4: 
			vicii_saccess(4);
			vicii_saccess(4);
		break;
		case 5: 
			vicii_paccess(5);
			vicii_saccess(5);
		break;
		case 6: 
			vicii_saccess(5);
			vicii_saccess(5);
		break;
		case 7: 
			vicii_paccess(6);
			vicii_saccess(6);
		break;
		case 8:
			vicii_saccess(6);
			vicii_saccess(6); 
		break;
		case 9: 
			vicii_paccess(7);
			vicii_saccess(7);
		break;
		case 10: 
			vicii_saccess(7);
			vicii_saccess(7);
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
			
			break;
		case 16:
			vicii_checkspritesdmaoff();
			vicii_caccess();
			break;
		case 17:
			
			vicii_gaccess();
			vicii_caccess();
		break;
		case 18:

			vicii_gaccess();
			vicii_caccess();
		break;
		case 19: case 20: case 21: case 22: case 23: case 24: case 25: case 26: 
		case 27: case 28: case 29: case 30: case 31: case 32: case 33: case 34: 
		case 35: case 36: case 37: case 38: case 39: case 40: case 41: case 42:
		case 43: case 44: case 45: case 46: case 47: case 48: case 49: case 50:
		case 51: case 52: case 53: case 54: 			
			
			vicii_gaccess();
			vicii_caccess();
		break;
		// * turn off balow because of badline.
		case 55:
			g_vic.balow = false;
			vicii_gaccess();	
			vicii_caccess();

			/*
				2. If the MxYE bit is set in the first phase of cycle 55, the expansion
   				flip flop is inverted.
   			*/
			for (int i = 0; i < 8; i++) {

				if (g_vic.regs[VICII_SPRITEDH] & (0x1 << i)) {
					g_vic.sprites[i].yex = !g_vic.sprites[i].yex;
				}
			}
			vicii_checkspritesdmaon();
		
		break;
		// * turn on border in 38 column mode.
		case 56: 
			vicii_gaccess();
			
			
				
			vicii_checkspritesdmaon();
		break;
		// * turn on border in 40 column mode.
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
			vicii_checkspriteson();
			vicii_paccess(0);
			vicii_saccess(0);

		break;
		case 59:
			vicii_saccess(0);
			vicii_saccess(0);
		break;
		case 60: 
			vicii_drawsprites();
			vicii_paccess(1);
			vicii_saccess(1);
		break;
		case 61: 
			vicii_saccess(1);
			vicii_saccess(1);
		break;
		case 62: 
			vicii_paccess(2);
			vicii_saccess(2);
		break;
		// * turn on border. 
		case 63:
			vicii_saccess(2);
			vicii_saccess(2);
		break;
		case 64: 
		break;
		case 65: 
		break;
		default: 
		break;
	}

	//
	// draw 8 pixels of grpahics (or idle if we are in vblank/hblank)
	//
	vicii_drawgraphics();
}

void vicii_update() {

	int i;
	int ticks = sysclock_getlastaddticks();

	for (i = 0; i < ticks; i++) {
		vicii_main_update();
	}
}

void vicii_destroy() {

	for (int i = 0;i < g_vic.screenheight; i++) {
		free(g_vic.out[i]);
		free(g_vic.type[i]);
	}
	free(g_vic.out);
	free(g_vic.type);

} 

void vicii_setbank() {
	g_vic.bank = ((~mem_peek(0xDD00)) & 0x03);
	g_vic.bank <<=14;
	DEBUG_PRINT("VICII bank updated.\n");
	DEBUG_PRINT("\tViewing memory between 0x%04X and 0x%04X.\n",g_vic.bank,g_vic.bank+0x3FFF);
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

		case VICII_SBCOLLIDE: // this register is cleared on reading.
			//
			// BUGBUG: Monitor will destroy these values.
			//
			rval = g_vic.regs[reg];
			g_vic.regs[reg] = 0;
			g_vic.irqbackground = true;
		break;
		case VICII_SSCOLLIDE: // this register is cleared on reading.
			//
			// BUGBUG: Monitor will destroy these values.
			//
			rval = g_vic.regs[reg];
			g_vic.regs[reg] = 0;
			g_vic.irqsprite = true;
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

const char * vicii_getmodename() {

	switch(g_vic.mode) {



	

	case VICII_MODE_STANDARD_TEXT: 		return "Standard Text";break;
	case VICII_MODE_MULTICOLOR_TEXT: 	return "Multicolor Text";break;
	case VICII_MODE_STANDARD_BITMAP: 	return "Standard Bitmap";break;
	case VICII_MODE_MULTICOLOR_BITMAP: 	return "Multicolor Bitmap";break;
	case VICII_MODE_ECM_TEXT:			return "Extended Color Mode Text";break;
	case VICII_MODE_INVALID_TEXT: 		return "Invalid Text";break;
	case VICII_MODE_INVALID_BITMAP_1: 	return "Invalid Bitmap 1";break;
	case VICII_MODE_INVALID_BITMAP_2:	return "Invalid Bitmap 2";break;
	} 

	return "undefined";


}

void vicii_poke(word address,byte val) {
	byte reg = address % VICII_LAST;

	switch(reg) {

		case VICII_CR1: 	
			DEBUG_PRINT("VICII Control Register 1 updated.\n");
			g_vic.regs[reg] = val;
			// latch bit 7. Its part of the irq raster compare. 
			g_vic.raster_irq = (g_vic.raster_irq & 0xFF) | (((word) val & BIT_7)<<1) ;
			// set the vertical screen height
			if (val & BIT_3) {
				DEBUG_PRINT("\tScreen Height is 1\n");
				g_vic.displaytop = VICII_25ROW_TOP;
				g_vic.displaybottom = VICII_25ROW_BOTTOM;
			}
			else {
				DEBUG_PRINT("\tScreen Height is 0\n");
				g_vic.displaytop = VICII_24ROW_TOP;
				g_vic.displaybottom = VICII_24ROW_BOTTOM;
			}

			// update graphics mode bits.
			g_vic.mode &= ~(BIT_2 | BIT_1);
			if (val & BIT_5) {
				g_vic.mode |= BIT_1;
			}
			if (val & BIT_6) {
				g_vic.mode |= BIT_2;
			}
			DEBUG_PRINT("\tGraphics Mode is: %s\n",vicii_getmodename());
		break;
		case VICII_CR2: 
			DEBUG_PRINT("VICII Control Register 2 updated.\n");	
			g_vic.regs[reg] = val;
			// set the horizaontal screen width
			if (val & BIT_3) {
				DEBUG_PRINT("\tScreen Width is 1\n");
				g_vic.displayleft = VICII_40COL_LEFT;
				g_vic.displayright = VICII_40COL_RIGHT;
			}
			else {
				DEBUG_PRINT("\tScreen Width is 0\n");
				g_vic.displayleft = VICII_38COL_LEFT;
				g_vic.displayright = VICII_38COL_RIGHT;
			}

			g_vic.mode &= ~(BIT_0);
			if (val & BIT_4) {
				g_vic.mode |= BIT_0;
				
			}
			DEBUG_PRINT("\tGraphics Mode is: %s\n",vicii_getmodename());
		break;
		case VICII_RASTER: // latch raster line irq compare.
			g_vic.raster_irq = (g_vic.raster_irq & 0x0100) | val; 
			DEBUG_PRINT("VICII Raster IRQ Set to raster line %d\n",g_vic.raster_irq);
		break;

		case VICII_MEMSR: 
			g_vic.regs[reg] = val;
			g_vic.vidmembase = (val & (BIT_7 | BIT_6 | BIT_5 | BIT_4)) << 6;
			g_vic.charmembase = (val & (BIT_1 | BIT_2 | BIT_3)) << 10; 
			g_vic.bitmapmembase = (val & BIT_3) << 10;
			DEBUG_PRINT("VICII Memory Setup register updated.\n");
			DEBUG_PRINT("\tVideo Memory base:      0x%04X\n",g_vic.vidmembase);
			DEBUG_PRINT("\tCharacter Memory base:  0x%04X\n",g_vic.charmembase);
			DEBUG_PRINT("\tBitmap Memory base:     0x%04X\n",g_vic.bitmapmembase);
		break;

		case VICII_SPRITEMCM: 

			g_vic.regs[reg] = val;


		break;

		case VICII_SPRITEDW:

			g_vic.regs[reg] = val;
			for (int i = 0; i < 8; i++) {
				g_vic.sprites[i].dw = (val & (0x1 << i)) > 0;
			}


		break;

		case VICII_ICR:
			g_vic.regs[reg] = val;
			DEBUG_PRINT("VICII Interrupt Control Register updated.\n");
			DEBUG_PRINT("\tRaster Interrupt:              %sABLED.\n",val & BIT_0 ? "EN":"DIS");
			DEBUG_PRINT("\tSprite-Background Interrupt:   %sABLED.\n",val & BIT_1 ? "EN":"DIS");
			DEBUG_PRINT("\tSprite-Sprite Interrupt:       %sABLED.\n",val & BIT_2 ? "EN":"DIS");
			DEBUG_PRINT("\tLight Pen Interrupt:           %sABLED.\n",val & BIT_3 ? "EN":"DIS");
		break;

		case VICII_SPRITEPRI:

			g_vic.regs[reg] = val;
			for (int i = 0; i < 8; i++) {
				g_vic.sprites[i].fgpri = (val & (0x1 << i)) > 0;
			}
		break;

		case VICII_SPRITEDH: 


			g_vic.regs[reg] = val;

			/*
				1. The expansion flip flip is set as long as the bit in MxYE in register
	 		  $d017 corresponding to the sprite is cleared.
			*/

			for(int i = 0; i < 8; i++) {
				g_vic.sprites[i].yex = (val & (0x1 << i)) == 0;
			}

		break;
		case VICII_SPRITEEN:


			g_vic.regs[reg] = val;

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


