#ifndef C64KBD_H
#define C64KBD_H

#include "emu.h"


byte c64kbd_getrow(byte);
void c64kbd_init();
void c64kbd_destroy();
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



#define ROW_0   0x01
#define ROW_1   0x02
#define ROW_2   0x04
#define ROW_3   0x08
#define ROW_4   0x10
#define ROW_5   0x20
#define ROW_6   0x40
#define ROW_7   0x80






#endif