#ifndef MEMH
#define MEMH
#include "emu.h"


#define MEM_PAGE_SIZE 	0xFF
#define MEM_PAGE_COUNT 	0xFF

void mem_init();
void mem_destroy();
void mem_poke(word address,byte value);
byte mem_peek(word address);

#endif
