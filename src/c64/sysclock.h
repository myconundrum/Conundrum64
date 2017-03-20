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
MODULE: sysclock.h

WORK ITEMS:

KNOWN BUGS:

*/
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
