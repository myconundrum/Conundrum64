#include "emu.h"

#define VICII_ICR_RASTER_INTERRUPT 				0b00000001
#define VICII_ICR_SPRITE_BACKGROUND_INTERRUPT	0b00000010
#define VICII_ICR_SPRITE_SPRITE_INTERRUPT		0b00000100
#define VICII_ICR_LIGHT_PEN_INTERRUPT			0b00001000 


#define VICII_NTSC_LINE_START_X		0x1A0



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


#define VICII_LATCH 0x01
#define VICII_REAL  0x00


typedef struct {

	byte regs[2][0x30];
	byte membuf[40];				// this is copied from the video matrix during badlines.

	byte slcycles; 					// how many clocks on this scansline so far?
	word raster_y;					// faster look up of current raster line
	word raster_x;					// faster look up of x position within current raster line.
	
	bool badline;					// when badline is true, VICII prevents the CPU from getting clock cycles. 
	bool den;
	unsigned long startcycles; 		// what was the cycle count when this line started?
	

} VICII;




VICII g_vic = {0};


bool vicii_badline() 						{return g_vic.badline;}
byte vicii_getreal(byte reg) 				{return g_vic.regs[VICII_REAL][reg];}
void vicii_setreal(byte reg,byte val) 		{g_vic.regs[VICII_REAL][reg] = val;}
byte vicii_getlatched(byte reg) 			{return g_vic.regs[VICII_LATCH][reg];}
void vicii_setlatched(byte reg,byte val) 	{g_vic.regs[VICII_LATCH][reg] = val;}
void vicii_init() {

	//
	// BUGBUG: This magic value needs to be configurable by model and type of VICII
	// e.g. revision number, NTSC, PAL, etc.
	//
	g_vic.raster_x = VICII_NTSC_LINE_START_X; // starting X coordinate for an NTSC VICII.

}

void vicii_updateraster() {

	word iline;
	//
	// increment x raster position.
	//
	g_vic.raster_x +=8;
	if (g_vic.raster_x >= 0x1FF ) {
		g_vic.raster_x = 0;
	}

	//
	// update Y raster if we are at a new line.
	//
	if (g_vic.raster_x == VICII_NTSC_LINE_START_X) {
		
		g_vic.raster_y++;
		if (g_vic.raster_y == NTSC_LINES) {
			g_vic.raster_y = 0;
		}

		iline = vicii_getlatched(VICII_CR1) & BIT_7 ? 0x100 : 0;
		iline |= vicii_getlatched(VICII_RASTER);

		if (iline == g_vic.raster_y) {
			//
			// BUGBUG: not sure this gets cleared. 
			//
			vicii_setreal(VICII_ISR,vicii_getreal(VICII_ISR) | VICII_ICR_RASTER_INTERRUPT);
			if (vicii_getreal(VICII_ICR) & VICII_ICR_RASTER_INTERRUPT) {
				cpu_irq();
			}
		}

		//
		// save raster position in register.
		//
		vicii_setreal(VICII_RASTER,g_vic.raster_y & 0xFF);
		vicii_setreal(VICII_CR1, (g_vic.raster_y & 0x100) ? 
			(BIT_7 | vicii_getreal(VICII_CR1)) : (vicii_getreal(VICII_CR1) & (~BIT_7)));
	}
}


void vicii_update_new() {


	//
	// update raster position.
	//
	vicii_updateraster();

	//
	// reset display enable at the beginning of the frame (BUGBUG: is this right?)
	//
	if (g_vic.raster_y == 0) {
		g_vic.den = false;
	}

	//
	// display enable is indicated by the DEN bit being set during any cycle of raster line 0x30.
	//
	if (g_vic.raster_y == 0x30 && !g_vic.den && (vicii_getreal(VICII_CR1) & BIT_4))  { 
		DEBUG_PRINT("Display Enabled.\n");
		g_vic.den = true;
	}

	//
	// check to see if this is a badline.
	// (1) display is enabled
	// (2) beginning of a raster line
	// (3) between raster line 0x30 and 0xF7
	// (4) bottom 3 bits of raster_y == the SCROLLY bits in VICII_CR1
	//
	if (g_vic.den && 
		g_vic.raster_x == VICII_NTSC_LINE_START_X && 
		g_vic.raster_y >= 0x30 && g_vic.raster_y <= 0xF7 && 
		(g_vic.raster_y & 0b00000111) == (vicii_getreal(VICII_CR1) & 0b00000111)) {

		g_vic.badline = true;
		DEBUG_PRINT("Bad Line at Raster Line %d\n",g_vic.raster_y);
	}	
}

void vicii_update() {

	int i;
	int ticks = sysclock_getlastaddticks();

	for (i = 0; i < ticks; i++) {
		vicii_update_new();
	}
}

void vicii_destroy(){} 

byte vicii_peek(word address) {
	
	byte reg = address % VICII_LAST;
	byte rval;
	switch(reg) {
		case VICII_CR2: 
			// B7 and B6 not connected.
			rval = BIT_7 | BIT_6 | vicii_getreal(reg); 
		break;
		case VICII_ISR: 
			// B6,B5,B4 not connected.
			rval = BIT_6 | BIT_5 | BIT_4 | vicii_getreal(reg); 
		break;
		case VICII_ICR: 
			// B7-B4 not connected.
			rval = BIT_7 | BIT_6 | BIT_5 | BIT_4 | vicii_getreal(reg); 
		break;

		case VICII_SBCOLLIDE: case VICII_SSCOLLIDE: // these registers are cleared on reading.
			//
			// BUGBUG: Monitor will destroy these values.
			//
			rval = vicii_getreal(reg);
			vicii_setreal(reg,0);
		break;

		case VICII_BORDERCOL: case VICII_BACKCOL: case VICII_EBACKCOL1:		
		case VICII_EBACKCOL2: case VICII_EBACKCOL3: case VICII_ESPRITECOL1: case VICII_ESPRITECOL2:		
		case VICII_S0C:	case VICII_S1C:	case VICII_S2C:	case VICII_S3C:				
		case VICII_S4C:	case VICII_S5C:	case VICII_S6C:	case VICII_S7C:
			// B7-B4 not connected.
			rval = BIT_7 | BIT_6 | BIT_5 | BIT_4 | vicii_getreal(reg);
		break;	

		case VICII_UN0: case VICII_UN1: case VICII_UN2: case VICII_UN3:
		case VICII_UN4: case VICII_UN5: case VICII_UN6: case VICII_UN7:
		case VICII_UN8: case VICII_UN9: case VICII_UNA: case VICII_UNB:
		case VICII_UNC: case VICII_UND: case VICII_UNE: case VICII_UNF: case VICII_UN10:
			rval = 0xff;
		break;



		default: rval = vicii_getreal(reg);
	}

	return rval;
}

void vicii_poke(word address,byte val) {
	byte reg = address % VICII_LAST;

	switch(reg) {

		case VICII_CR1: 	// latch bit 7. Its part of the irq raster compare. 
			vicii_setlatched(reg,val & BIT_7);
			vicii_setreal(reg,val);
		break;
		case VICII_RASTER: // latch raster line irq compare.
			vicii_setlatched(reg,val);
		break;
		case VICII_SBCOLLIDE: case VICII_SSCOLLIDE: // cannot write to collision registers
		break;
		case VICII_UN0: case VICII_UN1: case VICII_UN2: case VICII_UN3:
		case VICII_UN4: case VICII_UN5: case VICII_UN6: case VICII_UN7:
		case VICII_UN8: case VICII_UN9: case VICII_UNA: case VICII_UNB:
		case VICII_UNC: case VICII_UND: case VICII_UNE: case VICII_UNF: case VICII_UN10:
			// do nothing.
		break;

		default:vicii_setreal(reg,val);break;
	}	
}



/*
int MOS6569::EmulateLine(void)
{
	int cycles_left = ThePrefs.NormalCycles;	// Cycles left for CPU
	bool is_bad_line = false;

	// Get raster counter into local variable for faster access and increment
	unsigned int raster = raster_y+1;

	// End of screen reached?
	if (raster != TOTAL_RASTERS)
		raster_y = raster;
	else {
		vblank();
		raster = 0;
	}

	// Trigger raster IRQ if IRQ line reached
	if (raster == irq_raster)
		raster_irq();

	// In line $30, the DEN bit controls if Bad Lines can occur
	if (raster == 0x30)
		bad_lines_enabled = ctrl1 & 0x10;

	// Skip frame? Only calculate Bad Lines then
	if (frame_skipped) {
		if (raster >= FIRST_DMA_LINE && raster <= LAST_DMA_LINE && ((raster & 7) == y_scroll) && bad_lines_enabled) {
			is_bad_line = true;
			cycles_left = ThePrefs.BadLineCycles;
		}
		goto VIC_nop;
	}

	// Within the visible range?
	if (raster >= FIRST_DISP_LINE && raster <= LAST_DISP_LINE) {

		// Our output goes here
#ifdef __POWERPC__
		uint8 *chunky_ptr = (uint8 *)chunky_tmp;
#else
		uint8 *chunky_ptr = chunky_line_start;
#endif

		// Set video counter
		vc = vc_base;

		// Bad Line condition?
		if (raster >= FIRST_DMA_LINE && raster <= LAST_DMA_LINE && ((raster & 7) == y_scroll) && bad_lines_enabled) {

			// Turn on display
			display_state = is_bad_line = true;
			cycles_left = ThePrefs.BadLineCycles;
			rc = 0;

			// Read and latch 40 bytes from video matrix and color RAM
			uint8 *mp = matrix_line - 1;
			uint8 *cp = color_line - 1;
			int vc1 = vc - 1;
			uint8 *mbp = matrix_base + vc1;
			uint8 *crp = color_ram + vc1;
			for (int i=0; i<40; i++) {
				*++mp = *++mbp;
				*++cp = *++crp;
			}
		}

		// Handler upper/lower border
		if (raster == dy_stop)
			border_on = true;
		if (raster == dy_start && (ctrl1 & 0x10)) // Don't turn off border if DEN bit cleared
			border_on = false;

		if (!border_on) {

			// Display window contents
			uint8 *p = chunky_ptr + COL40_XSTART;		// Pointer in chunky display buffer
			uint8 *r = fore_mask_buf + COL40_XSTART/8;	// Pointer in foreground mask buffer
#ifdef ALIGNMENT_CHECK
			uint8 *use_p = ((((int)p) & 3) == 0) ? p : text_chunky_buf;
#endif

			{
				p--;
				uint8 b0cc = b0c_color;
				int limit = x_scroll;
				for (int i=0; i<limit; i++)	// Background on the left if XScroll>0
					*++p = b0cc;
				p++;
			}

			if (display_state) {
				switch (display_idx) {

					case 0:	// Standard text
#ifndef CAN_ACCESS_UNALIGNED
#ifdef ALIGNMENT_CHECK
						el_std_text(use_p, char_base + rc, r);
						if (use_p != p)
							memcpy(p, use_p, 8*40);
#else
						if (x_scroll) {
							el_std_text(text_chunky_buf, char_base + rc, r);
							memcpy(p, text_chunky_buf, 8*40);					        
						} else
							el_std_text(p, char_base + rc, r);
#endif
#else
						el_std_text(p, char_base + rc, r);
#endif
						break;

					case 1:	// Multicolor text
#ifndef CAN_ACCESS_UNALIGNED
#ifdef ALIGNMENT_CHECK
						el_mc_text(use_p, char_base + rc, r);
						if (use_p != p)
							memcpy(p, use_p, 8*40);
#else
						if (x_scroll) {
							el_mc_text(text_chunky_buf, char_base + rc, r);
							memcpy(p, text_chunky_buf, 8*40);					        
						} else
							el_mc_text(p, char_base + rc, r);
#endif
#else
						el_mc_text(p, char_base + rc, r);
#endif
						break;

					case 2:	// Standard bitmap
#ifndef CAN_ACCESS_UNALIGNED
#ifdef ALIGNMENT_CHECK
						el_std_bitmap(use_p, bitmap_base + (vc << 3) + rc, r);
						if (use_p != p)
							memcpy(p, use_p, 8*40);
#else
						if (x_scroll) {
							el_std_bitmap(text_chunky_buf, bitmap_base + (vc << 3) + rc, r);
							memcpy(p, text_chunky_buf, 8*40);					        
						} else
							el_std_bitmap(p, bitmap_base + (vc << 3) + rc, r);
#endif
#else
						el_std_bitmap(p, bitmap_base + (vc << 3) + rc, r);
#endif
						break;

					case 3:	// Multicolor bitmap
#ifndef CAN_ACCESS_UNALIGNED
#ifdef ALIGNMENT_CHECK
						el_mc_bitmap(use_p, bitmap_base + (vc << 3) + rc, r);
						if (use_p != p)
							memcpy(p, use_p, 8*40);
#else
						if (x_scroll) {
							el_mc_bitmap(text_chunky_buf, bitmap_base + (vc << 3) + rc, r);
							memcpy(p, text_chunky_buf, 8*40);					        
						} else
							el_mc_bitmap(p, bitmap_base + (vc << 3) + rc, r);
#endif
#else
						el_mc_bitmap(p, bitmap_base + (vc << 3) + rc, r);
#endif
						break;

					case 4:	// ECM text
#ifndef CAN_ACCESS_UNALIGNED
#ifdef ALIGNMENT_CHECK
						el_ecm_text(use_p, char_base + rc, r);
						if (use_p != p)
							memcpy(p, use_p, 8*40);
#else
						if (x_scroll) {
							el_ecm_text(text_chunky_buf, char_base + rc, r);
							memcpy(p, text_chunky_buf, 8*40);					        
						} else
							el_ecm_text(p, char_base + rc, r);
#endif
#else
						el_ecm_text(p, char_base + rc, r);
#endif
						break;

					default:	// Invalid mode (all black)
						memset(p, colors[0], 320);
						memset(r, 0, 40);
						break;
				}
				vc += 40;

			} else {	// Idle state graphics
				switch (display_idx) {

					case 0:		// Standard text
					case 1:		// Multicolor text
					case 4:		// ECM text
#ifndef CAN_ACCESS_UNALIGNED
#ifdef ALIGNMENT_CHECK
						el_std_idle(use_p, r);
						if (use_p != p) {memcpy(p, use_p, 8*40);}
#else
						if (x_scroll) {
							el_std_idle(text_chunky_buf, r);
							memcpy(p, text_chunky_buf, 8*40);					        
						} else
							el_std_idle(p, r);
#endif
#else
						el_std_idle(p, r);
#endif
						break;

					case 3:		// Multicolor bitmap
#ifndef CAN_ACCESS_UNALIGNED
#ifdef ALIGNMENT_CHECK
						el_mc_idle(use_p, r);
						if (use_p != p) {memcpy(p, use_p, 8*40);}
#else
						if (x_scroll) {
							el_mc_idle(text_chunky_buf, r);
							memcpy(p, text_chunky_buf, 8*40);					        
						} else
							el_mc_idle(p, r);
#endif
#else
						el_mc_idle(p, r);
#endif
						break;

					default:	// Invalid mode (all black)
						memset(p, colors[0], 320);
						memset(r, 0, 40);
						break;
				}
			}

			// Draw sprites
			if (sprite_on && ThePrefs.SpritesOn) {

				// Clear sprite collision buffer
				uint32 *lp = (uint32 *)spr_coll_buf - 1;
				for (int i=0; i<DISPLAY_X/4; i++)
					*++lp = 0;

				el_sprites(chunky_ptr);
			}

			// Handle left/right border
			uint32 *lp = (uint32 *)chunky_ptr - 1;
			uint32 c = ec_color_long;
			for (int i=0; i<COL40_XSTART/4; i++)
				*++lp = c;
			lp = (uint32 *)(chunky_ptr + COL40_XSTOP) - 1;
			for (int i=0; i<(DISPLAY_X-COL40_XSTOP)/4; i++)
				*++lp = c;
			if (!border_40_col) {
				c = ec_color;
				p = chunky_ptr + COL40_XSTART - 1;
				for (int i=0; i<COL38_XSTART-COL40_XSTART; i++)
					*++p = c;
				p = chunky_ptr + COL38_XSTOP - 1;
				for (int i=0; i<COL40_XSTOP-COL38_XSTOP; i++)
					*++p = c;
			}

#if 0
			if (is_bad_line) {
				chunky_ptr[DISPLAY_X-2] = colors[7];
				chunky_ptr[DISPLAY_X-3] = colors[7];
			}
			if (rc & 1) {
				chunky_ptr[DISPLAY_X-4] = colors[1];
				chunky_ptr[DISPLAY_X-5] = colors[1];
			}
			if (rc & 2) {
				chunky_ptr[DISPLAY_X-6] = colors[1];
				chunky_ptr[DISPLAY_X-7] = colors[1];
			}
			if (rc & 4) {
				chunky_ptr[DISPLAY_X-8] = colors[1];
				chunky_ptr[DISPLAY_X-9] = colors[1];
			}
			if (ctrl1 & 0x40) {
				chunky_ptr[DISPLAY_X-10] = colors[5];
				chunky_ptr[DISPLAY_X-11] = colors[5];
			}
			if (ctrl1 & 0x20) {
				chunky_ptr[DISPLAY_X-12] = colors[3];
				chunky_ptr[DISPLAY_X-13] = colors[3];
			}
			if (ctrl2 & 0x10) {
				chunky_ptr[DISPLAY_X-14] = colors[2];
				chunky_ptr[DISPLAY_X-15] = colors[2];
			}
#endif
		} else {

			// Display border
			uint32 *lp = (uint32 *)chunky_ptr - 1;
			uint32 c = ec_color_long;
			for (int i=0; i<DISPLAY_X/4; i++)
				*++lp = c;
		}

#ifdef __POWERPC__
		// Copy temporary buffer to bitmap
		fastcopy(chunky_line_start, (uint8 *)chunky_tmp);
#endif

		// Increment pointer in chunky buffer
		chunky_line_start += xmod;

		// Increment row counter, go to idle state on overflow
		if (rc == 7) {
			display_state = false;
			vc_base = vc;
		} else
			rc++;

		if (raster >= FIRST_DMA_LINE-1 && raster <= LAST_DMA_LINE-1 && (((raster+1) & 7) == y_scroll) && bad_lines_enabled)
			rc = 0;
	}

VIC_nop:
	// Skip this if all sprites are off
	if (me | sprite_on)
		cycles_left -= el_update_mc(raster);

	return cycles_left;
}


bool MOS6569::EmulateCycle(void)
{
	uint8 mask;
	int i;

	switch (cycle) {

		// Fetch sprite pointer 3, increment raster counter, trigger raster IRQ,
		// test for Bad Line, reset BA if sprites 3 and 4 off, read data of sprite 3
		case 1:
			if (raster_y == TOTAL_RASTERS-1)

				// Trigger VBlank in cycle 2
				vblanking = true;

			else {

				// Increment raster counter
				raster_y++;

				// Trigger raster IRQ if IRQ line reached
				if (raster_y == irq_raster)
					raster_irq();

				// In line $30, the DEN bit controls if Bad Lines can occur
				if (raster_y == 0x30)
					bad_lines_enabled = ctrl1 & 0x10;

				// Bad Line condition?
				is_bad_line = (raster_y >= FIRST_DMA_LINE && raster_y <= LAST_DMA_LINE && ((raster_y & 7) == y_scroll) && bad_lines_enabled);

				// Don't draw all lines, hide some at the top and bottom
				draw_this_line = (raster_y >= FIRST_DISP_LINE && raster_y <= LAST_DISP_LINE && !frame_skipped);
			}

			// First sample of border state
			border_on_sample[0] = border_on;

			SprPtrAccess(3);
			SprDataAccess(3, 0);
			DisplayIfBadLine;
			if (!(spr_dma_on & 0x18))
				the_cpu->BALow = false;
			break;

		// Set BA for sprite 5, read data of sprite 3
		case 2:
			if (vblanking) {

				// Vertical blank, reset counters
				raster_y = vc_base = 0;
				ref_cnt = 0xff;
				lp_triggered = vblanking = false;

				if (!(frame_skipped = --skip_counter))
					skip_counter = ThePrefs.SkipFrames;

				the_c64->VBlank(!frame_skipped);

				// Get bitmap pointer for next frame. This must be done
				// after calling the_c64->VBlank() because the preferences
				// and screen configuration may have been changed there
				chunky_line_start = the_display->BitmapBase();
				xmod = the_display->BitmapXMod();

				// Trigger raster IRQ if IRQ in line 0
				if (irq_raster == 0)
					raster_irq();

			}

			// Our output goes here
#ifdef __POWERPC__
			chunky_ptr = (uint8 *)chunky_tmp;
#else
			chunky_ptr = chunky_line_start;
#endif

			// Clear foreground mask
			memset(fore_mask_buf, 0, DISPLAY_X/8);
			fore_mask_ptr = fore_mask_buf;

			SprDataAccess(3,1);
			SprDataAccess(3,2);
			DisplayIfBadLine;
			if (spr_dma_on & 0x20)
				SetBALow;
			break;

		// Fetch sprite pointer 4, reset BA is sprite 4 and 5 off
		case 3:
			SprPtrAccess(4);
			SprDataAccess(4, 0);
			DisplayIfBadLine;
			if (!(spr_dma_on & 0x30))
				the_cpu->BALow = false;
			break;

		// Set BA for sprite 6, read data of sprite 4 
		case 4:
			SprDataAccess(4, 1);
			SprDataAccess(4, 2);
			DisplayIfBadLine;
			if (spr_dma_on & 0x40)
				SetBALow;
			break;

		// Fetch sprite pointer 5, reset BA if sprite 5 and 6 off
		case 5:
			SprPtrAccess(5);
			SprDataAccess(5, 0);
			DisplayIfBadLine;
			if (!(spr_dma_on & 0x60))
				the_cpu->BALow = false;
			break;

		// Set BA for sprite 7, read data of sprite 5
		case 6:
			SprDataAccess(5, 1);
			SprDataAccess(5, 2);
			DisplayIfBadLine;
			if (spr_dma_on & 0x80)
				SetBALow;
			break;

		// Fetch sprite pointer 6, reset BA if sprite 6 and 7 off
		case 7:
			SprPtrAccess(6);
			SprDataAccess(6, 0);
			DisplayIfBadLine;
			if (!(spr_dma_on & 0xc0))
				the_cpu->BALow = false;
			break;

		// Read data of sprite 6
		case 8:
			SprDataAccess(6, 1);
			SprDataAccess(6, 2);
			DisplayIfBadLine;
			break;

		// Fetch sprite pointer 7, reset BA if sprite 7 off
		case 9:
			SprPtrAccess(7);
			SprDataAccess(7, 0);
			DisplayIfBadLine;
			if (!(spr_dma_on & 0x80))
				the_cpu->BALow = false;
			break;

		// Read data of sprite 7
		case 10:
			SprDataAccess(7, 1);
			SprDataAccess(7, 2);
			DisplayIfBadLine;
			break;

		// Refresh, reset BA
		case 11:
			RefreshAccess;
			DisplayIfBadLine;
			the_cpu->BALow = false;
			break;

		// Refresh, turn on matrix access if Bad Line
		case 12:
			RefreshAccess;
			FetchIfBadLine;
			break;

		// Refresh, turn on matrix access if Bad Line, reset raster_x, graphics display starts here
		case 13:
			draw_background();
			SampleBorder;
			RefreshAccess;
			FetchIfBadLine;
			raster_x = 0xfffc;
			break;

		// Refresh, VCBASE->VCCOUNT, turn on matrix access and reset RC if Bad Line
		case 14:
			draw_background();
			SampleBorder;
			RefreshAccess;
			RCIfBadLine;
			vc = vc_base;
			break;

		// Refresh and matrix access, increment mc_base by 2 if y expansion flipflop is set
		case 15:
			draw_background();
			SampleBorder;
			RefreshAccess;
			FetchIfBadLine;

			for (i=0; i<8; i++)
				if (spr_exp_y & (1 << i))
					mc_base[i] += 2;

			ml_index = 0;
			matrix_access();
			break;

		// Graphics and matrix access, increment mc_base by 1 if y expansion flipflop is set
		// and check if sprite DMA can be turned off
		case 16:
			draw_background();
			SampleBorder;
			graphics_access();
			FetchIfBadLine;

			mask = 1;
			for (i=0; i<8; i++, mask<<=1) {
				if (spr_exp_y & mask)
					mc_base[i]++;
				if ((mc_base[i] & 0x3f) == 0x3f)
					spr_dma_on &= ~mask;
			}

			matrix_access();
			break;

		// Graphics and matrix access, turn off border in 40 column mode, display window starts here
		case 17:
			if (ctrl2 & 8) {
				if (raster_y == dy_stop)
					ud_border_on = true;
				else {
					if (ctrl1 & 0x10) {
						if (raster_y == dy_start)
							border_on = ud_border_on = false;
						else
							if (!ud_border_on)
								border_on = false;
					} else
						if (!ud_border_on)
							border_on = false;
				}
			}

			// Second sample of border state
			border_on_sample[1] = border_on;

			draw_background();
			draw_graphics();
			SampleBorder;
			graphics_access();
			FetchIfBadLine;
			matrix_access();
			break;

		// Turn off border in 38 column mode
		case 18:
			if (!(ctrl2 & 8)) {
				if (raster_y == dy_stop)
					ud_border_on = true;
				else {
					if (ctrl1 & 0x10) {
						if (raster_y == dy_start)
							border_on = ud_border_on = false;
						else
							if (!ud_border_on)
								border_on = false;
					} else
						if (!ud_border_on)
							border_on = false;
				}
			}

			// Third sample of border state
			border_on_sample[2] = border_on;

			// Falls through

		// Graphics and matrix access
		case 19: case 20: case 21: case 22: case 23: case 24:
		case 25: case 26: case 27: case 28: case 29: case 30:
		case 31: case 32: case 33: case 34: case 35: case 36:
		case 37: case 38: case 39: case 40: case 41: case 42:
		case 43: case 44: case 45: case 46: case 47: case 48:
		case 49: case 50: case 51: case 52: case 53: case 54:	// Gnagna...
			draw_graphics();
			SampleBorder;
			graphics_access();
			FetchIfBadLine;
			matrix_access();
			last_char_data = char_data;
			break;

		// Last graphics access, turn off matrix access, turn on sprite DMA if Y coordinate is
		// right and sprite is enabled, handle sprite y expansion, set BA for sprite 0
		case 55:
			draw_graphics();
			SampleBorder;
			graphics_access();
			DisplayIfBadLine;

			// Invert y expansion flipflop if bit in MYE is set
			mask = 1;
			for (i=0; i<8; i++, mask<<=1)
				if (mye & mask)
					spr_exp_y ^= mask;
			CheckSpriteDMA;

			if (spr_dma_on & 0x01) {	// Don't remove these braces!
				SetBALow;
			} else
				the_cpu->BALow = false;
			break;

		// Turn on border in 38 column mode, turn on sprite DMA if Y coordinate is right and
		// sprite is enabled, set BA for sprite 0, display window ends here
		case 56:
			if (!(ctrl2 & 8))
				border_on = true;

			// Fourth sample of border state
			border_on_sample[3] = border_on;

			draw_graphics();
			SampleBorder;
			IdleAccess;
			DisplayIfBadLine;
			CheckSpriteDMA;

			if (spr_dma_on & 0x01)
				SetBALow;
			break;

		// Turn on border in 40 column mode, set BA for sprite 1, paint sprites
		case 57:
			if (ctrl2 & 8)
				border_on = true;

			// Fifth sample of border state
			border_on_sample[4] = border_on;

			// Sample spr_disp_on and spr_data for sprite drawing
			if ((spr_draw = spr_disp_on))
				memcpy(spr_draw_data, spr_data, 8*4);

			// Turn off sprite display if DMA is off
			mask = 1;
			for (i=0; i<8; i++, mask<<=1)
				if ((spr_disp_on & mask) && !(spr_dma_on & mask))
					spr_disp_on &= ~mask;

			draw_background();
			SampleBorder;
			IdleAccess;
			DisplayIfBadLine;
			if (spr_dma_on & 0x02)
				SetBALow;
			break;

		// Fetch sprite pointer 0, mc_base->mc, turn on sprite display if necessary,
		// turn off display if RC=7, read data of sprite 0
		case 58:
			draw_background();
			SampleBorder;

			mask = 1;
			for (i=0; i<8; i++, mask<<=1) {
				mc[i] = mc_base[i];
				if ((spr_dma_on & mask) && (raster_y & 0xff) == my[i])
					spr_disp_on |= mask;
			}
			SprPtrAccess(0);
			SprDataAccess(0, 0);

			if (rc == 7) {
				vc_base = vc;
				display_state = false;
			}
			if (is_bad_line || display_state) {
				display_state = true;
				rc = (rc + 1) & 7;
			}
			break;

		// Set BA for sprite 2, read data of sprite 0
		case 59:
			draw_background();
			SampleBorder;
			SprDataAccess(0, 1);
			SprDataAccess(0, 2);
			DisplayIfBadLine;
			if (spr_dma_on & 0x04)
				SetBALow;
			break;

		// Fetch sprite pointer 1, reset BA if sprite 1 and 2 off, graphics display ends here
		case 60:
			draw_background();
			SampleBorder;

			if (draw_this_line) {

				// Draw sprites
				if (spr_draw && ThePrefs.SpritesOn)
					draw_sprites();

				// Draw border
#ifdef __POWERPC__
				if (border_on_sample[0])
					for (i=0; i<4; i++)
						memset8((uint8 *)chunky_tmp+i*8, border_color_sample[i]);
				if (border_on_sample[1])
					memset8((uint8 *)chunky_tmp+4*8, border_color_sample[4]);
				if (border_on_sample[2])
					for (i=5; i<43; i++)
						memset8((uint8 *)chunky_tmp+i*8, border_color_sample[i]);
				if (border_on_sample[3])
					memset8((uint8 *)chunky_tmp+43*8, border_color_sample[43]);
				if (border_on_sample[4])
					for (i=44; i<DISPLAY_X/8; i++)
						memset8((uint8 *)chunky_tmp+i*8, border_color_sample[i]);
#else
				if (border_on_sample[0])
					for (i=0; i<4; i++)
						memset8(chunky_line_start+i*8, border_color_sample[i]);
				if (border_on_sample[1])
					memset8(chunky_line_start+4*8, border_color_sample[4]);
				if (border_on_sample[2])
					for (i=5; i<43; i++)
						memset8(chunky_line_start+i*8, border_color_sample[i]);
				if (border_on_sample[3])
					memset8(chunky_line_start+43*8, border_color_sample[43]);
				if (border_on_sample[4])
					for (i=44; i<DISPLAY_X/8; i++)
						memset8(chunky_line_start+i*8, border_color_sample[i]);
#endif

#ifdef __POWERPC__
				// Copy temporary buffer to bitmap
				fastcopy(chunky_line_start, (uint8 *)chunky_tmp);
#endif

				// Increment pointer in chunky buffer
				chunky_line_start += xmod;
			}

			SprPtrAccess(1);
			SprDataAccess(1, 0);
			DisplayIfBadLine;
			if (!(spr_dma_on & 0x06))
				the_cpu->BALow = false;
			break;

		// Set BA for sprite 3, read data of sprite 1
		case 61:
			SprDataAccess(1, 1);
			SprDataAccess(1, 2);
			DisplayIfBadLine;
			if (spr_dma_on & 0x08)
				SetBALow;
			break;

		// Read sprite pointer 2, reset BA if sprite 2 and 3 off, read data of sprite 2
		case 62:
			SprPtrAccess(2);
			SprDataAccess(2, 0);
			DisplayIfBadLine;
			if (!(spr_dma_on & 0x0c))
				the_cpu->BALow = false;
			break;

		// Set BA for sprite 4, read data of sprite 2
		case 63:
			SprDataAccess(2, 1);
			SprDataAccess(2, 2);
			DisplayIfBadLine;

			if (raster_y == dy_stop)
				ud_border_on = true;
			else
				if (ctrl1 & 0x10 && raster_y == dy_start)
					ud_border_on = false;

			if (spr_dma_on & 0x10)
				SetBALow;

			// Last cycle
			raster_x += 8;
			cycle = 1;
			return true;
	}

	// Next cycle
	raster_x += 8;
	cycle++;
	return false;
}



*/





