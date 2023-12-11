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
MODULE: mem.c
Memory emulation source for Commodore 64


WORK ITEMS:

KNOWN BUGS:

*/
#include <string.h>
#include "emu.h"
#include "cpu.h"
#include "mem.h"

typedef struct {

	word low;
	word high;

	POKEHANDLER poke;
	PEEKHANDLER peek;

	bool active; 

} MEMORY_MAP;

#define MAX_MEMORY_MAPS 30 // arbitrary

typedef struct {
	byte 		ram  [MEM_PAGE_SIZE * MEM_PAGE_COUNT];
	MEMORY_MAP 	maps [MAX_MEMORY_MAPS];
	byte 		mapNext;
} MEMORY;

MEMORY g_memory;


byte mem_map(word low, word high, PEEKHANDLER peekfn, POKEHANDLER pokefn) {

	if (g_memory.mapNext == MAX_MEMORY_MAPS) {
		FATAL_ERROR("Memory: Out of memory mapping space.\n");
	}
	g_memory.maps[g_memory.mapNext].low = low;
	g_memory.maps[g_memory.mapNext].high = high;
	g_memory.maps[g_memory.mapNext].peek = peekfn;
	g_memory.maps[g_memory.mapNext].poke = pokefn;

	return g_memory.mapNext++;
}


void mem_mapactive(byte map, bool flag) {g_memory.maps[map].active = flag;}

void mem_init(void) {
	DEBUG_PRINT("** Initializing Memory...\n");
	memset(&g_memory,0,sizeof(MEMORY));}
void mem_destroy(void) {}

MEMORY_MAP *mem_getmap(word address) {
	
	int i;

	for (i = 0; i < g_memory.mapNext; i++) {
		if (address >= g_memory.maps[i].low && 
			address <= g_memory.maps[i].high && g_memory.maps[i].active) {
			return &g_memory.maps[i];
		}
	}
	return NULL;
}

void mem_nonmappable_poke(word address,byte value) {g_memory.ram[address] = value;}
byte mem_nonmappable_peek(word address) {return g_memory.ram[address];}

void mem_poke(word address,byte value) {

	MEMORY_MAP * map = mem_getmap(address);
	if (map) {
		map->poke(address-map->low,value);
	}
	else {
		g_memory.ram[address] = value;
	}
}
byte mem_peek(word address) {

	MEMORY_MAP * map = mem_getmap(address);

	if (map) {
		return map->peek(address-map->low);
	}
	else {
		return g_memory.ram[address];
	}
}
void mem_pokeword(word address,word value) {

	mem_poke(address, (byte) (value & 0xFF));
	mem_poke(address+1,(byte) (value >> 8));
}
word mem_peekword(word address) {
	return (mem_peek(address+1) << 8) | mem_peek(address);
}

