#include <string.h>
#include "emu.h"


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
		DEBUG_PRINT("Memory: Out of memory mapping space.\n");
		return 0;
	}
	g_memory.maps[g_memory.mapNext].low = low;
	g_memory.maps[g_memory.mapNext].high = high;
	g_memory.maps[g_memory.mapNext].peek = peekfn;
	g_memory.maps[g_memory.mapNext].poke = pokefn;

	return g_memory.mapNext++;
}


void mem_mapactive(byte map, bool flag) {g_memory.maps[map].active = flag;}

void mem_init() {memset(&g_memory,0,sizeof(MEMORY));}
void mem_destroy() {}

MEMORY_MAP *mem_getmap(address) {
	
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

