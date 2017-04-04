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
MODULE: c64.c
	Commodore 64 "system" source file. Manages initialiation/destruction/update of variouos
	c64 components.


WORK ITEMS:

KNOWN BUGS:

*/


#include "emu.h"
#include "cpu.h"
#include "vicii.h"
#include "cia.h"
#include "mem.h"
#include "sysclock.h"




#define KERNAL_ROM_LOW_ADDRESS			0xE000
#define KERNAL_ROM_HIGH_ADDRESS			0xFFFF

#define BANKSWITCH_ADDRESS 				0x0001
#define BANKSWITCH_DIRECTION_ADDRESS	0x0000

#define BASIC_ROM_LOW_ADDRESS			0xA000
#define BASIC_ROM_HIGH_ADDRESS			0xBFFF

#define CHAR_ROM_LOW_ADDRESS			0xD000
#define CHAR_ROM_HIGH_ADDRESS			0xDFFF

#define IO_AREA_LOW_ADDRESS				0xD000
#define IO_AREA_HIGH_ADDRESS			0xDFFF

#define CIA1_AREA_LOW_ADDRESS			0xDC00
#define CIA1_AREA_HIGH_ADDRESS			0xDCFF

#define CIA2_AREA_LOW_ADDRESS			0xDD00
#define CIA2_AREA_HIGH_ADDRESS			0xDDFF


#define VICII_AREA_LOW_ADDRESS			0xD000
#define VICII_AREA_HIGH_ADDRESS			0xD3FF


typedef struct {

	//
	// these hold the memory map ids from mem_map();
	//
	byte mBankSwitch;	
	byte mKernal;
	byte mBasic;
	byte mChar;
	byte mCia1;
	byte mCia2;
	byte mVicii;

	//
	// ROMs
	//
	byte * rKernal;
	byte * rBasic;
	byte * rChar;

} C64_MAPPED_IO;

C64_MAPPED_IO g_io;


void c64_rompoke(word address, byte val) {}
byte c64_kernalpeek(word address) 				{return g_io.rKernal[address];}
void c64_kernalpoke(word address,byte val) 		{return mem_nonmappable_poke(address+KERNAL_ROM_LOW_ADDRESS,val);}
void c64_basicpoke(word address,byte val) 		{return mem_nonmappable_poke(address+BASIC_ROM_LOW_ADDRESS,val);}
byte c64_basicpeek(word address) 				{return g_io.rBasic[address];}
byte c64_charpeek(word address) 				{return g_io.rChar[address];}
byte c64_bankswitchpeek(word address) 			{return mem_nonmappable_peek(address+1);}

void c64_bankswitchpoke(word address, byte val) {
	
	bool allram = false;

	if ((val & 0x03) ==0) {
		allram = true;
	}
	
	//
	// do memory mapping.
	//
	mem_mapactive(g_io.mKernal,!allram && (val & 0x02));
	mem_mapactive(g_io.mBasic, !allram && (val & 0x01));
	mem_mapactive(g_io.mChar, !allram && (val && ((val & 0x04) == 0)));
	mem_mapactive(g_io.mCia1, !allram && (val & 0x04));
	mem_mapactive(g_io.mCia2, !allram && (val & 0x04));
	mem_mapactive(g_io.mVicii, !allram && (val & 0x04));


	DEBUG_IF((mem_nonmappable_peek(address + 1) & 0x7) != (val & 0x7))
		DEBUG_PRINT("Memory changed at 0x0001. C64 memory mappings updated.\n");
		DEBUG_PRINT("%-40s [%sMAPPED]\n","\tKernal:",!allram && (val & 0x02) ? "":"UN");
		DEBUG_PRINT("%-40s [%sMAPPED]\n","\tChar Rom:",!allram && (val && ((val & 0x04) == 0))? "":"UN");
		DEBUG_PRINT("%-40s [%sMAPPED]\n","\tI/O Devices:",!allram && (val & 0x04) ? "":"UN");
	DEBUG_ENDIF()

	mem_nonmappable_poke(address+1,val);
}


byte * c64_init_rom(char * name) {

	FILE * 	f;
	int 	len;
	byte * 	where = NULL;

	f = fopen(name,"rb");
	
	if (f) {

		fseek(f, 0, SEEK_END);          
    	len = ftell(f);            
    	rewind(f);
    	DEBUG_PRINT("Loaded %d bytes from ROM image %s.\n",len,name);
    	where = (byte *) malloc(sizeof(byte) * len);   
		fread(where,1,len,f);
		fclose(f);
	}

	return where;
}


void c64_init() {



	EMU_CONFIGURATION * cfg = emu_getconfig();


	DEBUG_PRINT("** Initializing computer...\n");

	memset(&g_io,0,sizeof(C64_MAPPED_IO));
	mem_init();									// init ram


	//
	// Load ROMs and initialize memory maps
	//
	g_io.mBankSwitch 	= mem_map(0x0001,0x0001,c64_bankswitchpeek,c64_bankswitchpoke);
	g_io.rKernal 		= c64_init_rom(cfg->kernalpath);
	g_io.rBasic 		= c64_init_rom(cfg->basicpath);
	g_io.rChar 			= c64_init_rom(cfg->charpath);
	g_io.mKernal 		= mem_map(KERNAL_ROM_LOW_ADDRESS,KERNAL_ROM_HIGH_ADDRESS,c64_kernalpeek,c64_kernalpoke);
	g_io.mBasic 		= mem_map(BASIC_ROM_LOW_ADDRESS,BASIC_ROM_HIGH_ADDRESS,c64_basicpeek,c64_basicpoke);
	g_io.mChar 			= mem_map(CHAR_ROM_LOW_ADDRESS,CHAR_ROM_HIGH_ADDRESS,c64_charpeek,c64_rompoke);
	g_io.mCia1			= mem_map(CIA1_AREA_LOW_ADDRESS,CIA1_AREA_HIGH_ADDRESS,cia1_peek,cia1_poke);
	g_io.mCia2			= mem_map(CIA2_AREA_LOW_ADDRESS,CIA2_AREA_HIGH_ADDRESS,cia2_peek,cia2_poke);
	g_io.mVicii			= mem_map(VICII_AREA_LOW_ADDRESS,VICII_AREA_HIGH_ADDRESS,vicii_peek,vicii_poke);


	if (!g_io.rKernal || !g_io.rBasic || !g_io.rChar) {
		FATAL_ERROR("%s: Failed to load roms. Exiting.\n",emu_getname());
	}
	//
	// Bankswitching is always active. Determines which other memory locations are currently mapped. 
	//
	mem_mapactive(g_io.mBankSwitch,true);

	//
	// Set initial bankswitch configuration (IO, Basic, Kernal mapped in)
	//
	mem_poke(BANKSWITCH_ADDRESS,0xE7);

	//
	// initialize rest of system.
	//
	c64kbd_init();
	sysclock_init();						// init clock
	cpu_init();								// init 6502 CPU for C64 emulator.
	cia_init();	
	vicii_init();

}

void c64_update() {

	sysclock_update();
	fflush(g_debug);
	
	if (sysclock_getphi() == PHI_HIGH) {
		//
		// CPU update on high signal, unless VIC has claimed the bus.
		//
		cia_update();
		if (!vicii_stuncpu()) {
			cpu_update();
		}
		else {
			vicii_update();
		}
	} else {
		//
		// VIC can use the memory bus on all PHI_LOW cycles.
		//
		vicii_update();
	}	
}

void c64_destroy() {
	cpu_destroy();
	mem_destroy();
	cia_destroy(); 
	c64kbd_destroy();
	vicii_destroy();

	free(g_io.rKernal);
	free(g_io.rBasic);
	free(g_io.rChar);
}







