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
MODULE: c64.h
	Commodore 64 "system" source file. Manages initialiation/destruction/update of variouos
	c64 components.


WORK ITEMS:

KNOWN BUGS:

*/
#ifndef C64KBD_H
#define C64KBD_H

#include "emu.h"


byte c64kbd_getrow(byte);
void c64kbd_init(void);
void c64kbd_destroy(void);
void c64kbd_reset(void);
void c64kbd_keyup	(byte ch);
void c64kbd_keydown	(byte ch);



//
// BUGBUG dummy values for c64 keys
//

#define C64KEY_LSHIFT 	0xD0
#define C64KEY_CTRL  	0xD1
#define C64KEY_RUNSTOP	0xD2
#define C64KEY_CURDOWN	0xD3
#define C64KEY_CURLEFT	0xD4
#define C64KEY_CURRIGHT	0xD5
#define C64KEY_CURUP	0xD6
#define C64KEY_F1		0xD6
#define C64KEY_F3		0xD7
#define C64KEY_F5		0xD8
#define C64KEY_F7		0xD9
#define C64KEY_C64		0xDA
#define C64KEY_RSHIFT	0xDB
#define C64KEY_HOME		0xDC
#define C64KEY_BACK		0xDD
#define C64KEY_POUND	0xDD
#define C64KEY_DELETE   0xDE
#define C64KEY_RESTORE  0xDF
#define C64KEY_UNUSED   0xFF



#define ROW_0   0x01
#define ROW_1   0x02
#define ROW_2   0x04
#define ROW_3   0x08
#define ROW_4   0x10
#define ROW_5   0x20
#define ROW_6   0x40
#define ROW_7   0x80






#endif
