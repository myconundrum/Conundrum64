#include <string.h>
#include "emu.h"




typedef struct {

	byte ram[MEM_PAGE_SIZE * MEM_PAGE_COUNT];

} MEMORY;

MEMORY g_memory;



void mem_init() {
	memset(&g_memory,0,sizeof(MEMORY));
}
void mem_destroy() {

}

void mem_poke(word address,byte value) {

	g_memory.ram[address] = value;
}

void mem_pokeword(word address,word value) {

	g_memory.ram[address] = (byte) (value & 0xFF);
	g_memory.ram[address+1] = (byte) (value >> 8);
}

byte mem_peek(word address) {
	return g_memory.ram[address];
}

word mem_peekword(word address) {
	return (g_memory.ram[address+1] << 8) | g_memory.ram[address];
}


