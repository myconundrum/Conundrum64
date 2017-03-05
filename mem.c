#include <string.h>
#include "emu.h"




#define ISKERNALADDRESS(x) (g_memory.kernalmapped && (x) >= KERNAL_ROM_LOW_ADDRESS && (x) <= KERNAL_ROM_HIGH_ADDRESS)
#define ISBASICADDRESS(x) (g_memory.basicmapped && (x) >= BASIC_ROM_LOW_ADDRESS && (x) <= BASIC_ROM_HIGH_ADDRESS)
#define ISCHARADDRESS(x) (g_memory.charmapped && (x) >= CHAR_ROM_LOW_ADDRESS && (x) <= CHAR_ROM_HIGH_ADDRESS)
//#define ISIOADDRESS(x) (g_memory.iomapped && (x) >= IO_AREA_LOW_ADDRESS && (x) <= IO_AREA_HIGH_ADDRESS)
#define ISCIA1ADDRESS(x) (g_memory.iomapped && (x) >= CIA1_AREA_LOW_ADDRESS && (x) <= CIA1_AREA_HIGH_ADDRESS)
#define ISCIA2ADDRESS(x) (g_memory.iomapped && (x) >= CIA2_AREA_LOW_ADDRESS && (x) <= CIA2_AREA_HIGH_ADDRESS)


typedef struct {

	byte ram[MEM_PAGE_SIZE * MEM_PAGE_COUNT];
	byte * charrom;
	byte * kernalrom;
	byte * basicrom;
	bool kernalmapped;
	bool charmapped;
	bool basicmapped;
	bool iomapped;
	

} MEMORY;

MEMORY g_memory;



//
// BUGBUG: I don't handle extended ram and cartridge ram yet. 
//
void mem_config_change() {

	byte val = mem_peek(BANKSWITCH_ADDRESS);

	g_memory.basicmapped 	= false;
	g_memory.iomapped 		= false;
	g_memory.charmapped 	= false;
	g_memory.kernalmapped 	= false;


	if (!(val & 0x03)) {
		//
		// all ram.
		//
	} else {
		g_memory.kernalmapped 	= val & 0x02;
		g_memory.basicmapped 	= val & 0x01;
		g_memory.iomapped 		= val & 0x04;
		g_memory.charmapped 	= !g_memory.iomapped;

	}
}

byte * mem_init_rom(char * name) {

	FILE * 	f;
	int 	len;
	byte * 	where = NULL;

	f = fopen(name,"rb");
	
	if (f) {

		fseek(f, 0, SEEK_END);          
    	len = ftell(f);            
    	rewind(f);
    	DEBUG_PRINT("MEM: loaded binary image %s size %d\n",name,len);

    	where = (byte *) malloc(sizeof(byte) * len);   
		fread(where,1,len,f);
		fclose(f);
	}

	return where;
}


void mem_init() {
	memset(&g_memory,0,sizeof(MEMORY));
	g_memory.kernalrom = mem_init_rom("asm/901227-03-kernal.bin");
	g_memory.basicrom = mem_init_rom("asm/901226-01-basic.bin");
	g_memory.charrom = mem_init_rom("asm/901225-01-char.bin");

	
}
void mem_destroy() {
	free(g_memory.kernalrom);
	free(g_memory.basicrom);
	free(g_memory.charrom);
}

void mem_poke(word address,byte value) {

	if (ISKERNALADDRESS(address) || ISBASICADDRESS(address) || ISCHARADDRESS(address)) {
		// do nothing
		return;
	}
	
	if (ISCIA1ADDRESS(address)) {
		cia1_poke(address,value);
	} else if (ISCIA2ADDRESS(address)) {
		cia2_poke(address,value);
	}
	else {
		g_memory.ram[address] = value;
		if (address == BANKSWITCH_ADDRESS) {
			mem_config_change();
		}
	}
}



void mem_pokeword(word address,word value) {

	mem_poke(address, (byte) (value & 0xFF));
	mem_poke(address+1,(byte) (value >> 8));
}

word mem_peekword(word address) {
	return (mem_peek(address+1) << 8) | mem_peek(address);
}


byte mem_peek(word address) {


	byte val = 0;

	if (ISKERNALADDRESS(address)) {
		val = g_memory.kernalrom[address - KERNAL_ROM_LOW_ADDRESS];
	}
	else if (ISCHARADDRESS(address)) {
		val = g_memory.charrom[address - CHAR_ROM_LOW_ADDRESS];
	}
	else if (ISBASICADDRESS(address)) {
		val = g_memory.basicrom[address - BASIC_ROM_LOW_ADDRESS];
	}
	else if (ISCIA1ADDRESS(address)) {

		val = cia1_peek(address);
	}
	else if (ISCIA2ADDRESS(address)) {

		val = cia2_peek(address);
	}
	else {val = g_memory.ram[address];}

	return val;
}

