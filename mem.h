#ifndef MEMH
#define MEMH
#include "emu.h"

#define MEM_PAGE_SIZE 	0xFF
#define MEM_PAGE_COUNT 	0xFF

#define BANKSWITCH_ADDRESS 				0x0001
#define BANKSWITCH_DIRECTION_ADDRESS	0x0000

#define BASIC_ROM_LOW_ADDRESS			0xA000
#define BASIC_ROM_HIGH_ADDRESS			0xBFFF

#define CHAR_ROM_LOW_ADDRESS			0xD000
#define CHAR_ROM_HIGH_ADDRESS			0xDFFF

#define KERNAL_ROM_LOW_ADDRESS			0xE000
#define KERNAL_ROM_HIGH_ADDRESS			0xFFFF

#define IO_AREA_LOW_ADDRESS				0xD000
#define IO_AREA_HIGH_ADDRESS			0xDFFF

#define CIA1_AREA_LOW_ADDRESS			0xDC00
#define CIA1_AREA_HIGH_ADDRESS			0xDCFF

void mem_init();
void mem_destroy();
void mem_poke(word address,byte value);
void mem_pokeword(word address,word value);
byte mem_peek(word address);
word mem_peekword(word address);

#endif
