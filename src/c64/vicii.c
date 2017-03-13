#include "emu.h"


typedef enum {
	//
	// Sprite positions in M0 through M7.
	//
	VICII_M0X,
	VICII_M0Y,
	VICII_M1X,
	VICII_M1Y,
	VICII_M2X,
	VICII_M2Y,
	VICII_M3X,
	VICII_M3Y,
	VICII_M4X,
	VICII_M4Y,
	VICII_M5X,
	VICII_M5Y,
	VICII_M6X,
	VICII_M6Y,
	VICII_M7X,
	VICII_M7Y,
	VICII_MX8,
	VICII_CR1,
	VICII_RASTER,
	VICII_RSTCMP,
	VICII_LPX,
	VICII_LPY,
	VICII_ME,
	VICII_VMCB,
	VICII_IRQST,
	VICII_IRQEN,
	VICII_MDP,
	VICII_MMC,
	VICII_MXE,
	VICII_MM,
	VICII_MD,
	VICII_EC,
	VICII_B0C,
	VICII_B1C,
	VICII_B2C,
	VICII_B3C,
	VICII_MM0,
	VICII_MM1,
	VICII_M0C,
	VICII_M1C,
	VICII_M2C,
	VICII_M3C,
	VICII_M4C,
	VICII_M5C,
	VICII_M6C,
	VICII_M7C,
	VICII_KCR,
	VICII_FAST,
	VICII_LAST
} VICII_REG;


typedef struct {

	byte reg[0x30];
	byte slcycles; 			// how many clocks on this scansline so far?

} VICII;

VICII g_vic = {0};

void vicii_init() {


}

void vicii_updateraster() {
	word rline;

	g_vic.slcycles += sysclock_getticks();

	if (g_vic.slcycles > CLOCK_TICKS_PER_LINE_NTSC) {
		rline = g_vic.reg[VICII_CR1] & BIT_7 ? 0x100 : 0;
		rline += g_vic.reg[VICII_RASTER];
		
		if (++rline == NTSC_LINES) {
			rline = 0;
		}
		
		g_vic.reg[VICII_RASTER] = rline & 0xFF;
		g_vic.reg[VICII_CR1] = (rline & 0x100) ? 
			(BIT_7 | g_vic.reg[VICII_CR1]) : (g_vic.reg[VICII_CR1] & (~BIT_7));

		g_vic.slcycles = g_vic.slcycles - CLOCK_TICKS_PER_LINE_NTSC;
	}
}


void vicii_update() {

	vicii_updateraster();

}
void vicii_destroy() {}

byte vicii_peek(word address) {
	return g_vic.reg[address % VICII_LAST];
}

void vicii_poke(word address,byte val) {
	g_vic.reg[address % VICII_LAST] = val;
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





