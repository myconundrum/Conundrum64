#ifndef SYSCLOCK_H
#define SYSCLOCK_H



/*

	PAL: 504 Pixels / 8 = 63 Cycles per Line * 312 Lines = 19656 total CPU Cycles 
	NTSC: 520 Pixels / 8 = 65 Cycles per Line * 263 Lines = 17095 total CPU Cycles 
	Clock cycles for a screen refresh...
	17095 * 60 = 1025700 cycles per second.
	NTSC: 8.18MHz / 8 = 1.023MHz

*/

#define CLOCK_TICKS_PER_LINE_PAL 		63
#define CLOCK_TICKS_PER_LINE_NTSC		65
#define PAL_LINES						312
#define NTSC_LINES						263
#define PAL_FPS							50
#define NTSC_FPS						60

#define NTSC_TICKS_PER_SECOND 		(NTSC_FPS*CLOCK_TICKS_PER_LINE_NTSC*NTSC_LINES)
#define PAL_TICKS_PER_SECOND 		(PAL_FPS*CLOCK_TICKS_PER_LINE_PAL*PAL_LINES)


void sysclock_init(void);
void sysclock_addticks(unsigned long ticks);
unsigned long sysclock_getticks(void);
word sysclock_getlastaddticks(void);

#endif
