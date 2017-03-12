#include "emu.h"


typedef enum {
	//
	// Sprite positions in M0 through M7.
	//
	VICII_M0X,
	VICII_M0Y,
	VICII_M1X,
	VICII_M1Y,
	VICII_M2X,
	VICII_M2Y,
	VICII_M3X,
	VICII_M3Y,
	VICII_M4X,
	VICII_M4Y,
	VICII_M5X,
	VICII_M5Y,
	VICII_M6X,
	VICII_M6Y,
	VICII_M7X,
	VICII_M7Y,
	VICII_MX8,
	VICII_CR1,
	VICII_RASTER,
	VICII_RSTCMP,
	VICII_LPX,
	VICII_LPY,
	VICII_ME,
	VICII_VMCB,
	VICII_IRQST,
	VICII_IRQEN,
	VICII_MDP,
	VICII_MMC,
	VICII_MXE,
	VICII_MM,
	VICII_MD,
	VICII_EC,
	VICII_B0C,
	VICII_B1C,
	VICII_B2C,
	VICII_B3C,
	VICII_MM0,
	VICII_MM1,
	VICII_M0C,
	VICII_M1C,
	VICII_M2C,
	VICII_M3C,
	VICII_M4C,
	VICII_M5C,
	VICII_M6C,
	VICII_M7C,
	VICII_KCR,
	VICII_FAST
} VICII_REG;


typedef struct {

	byte registers[0x30];



} VICII;



void vicii_init() {}
void vicii_update() {
	mem_poke(0xD012,0); //BUGBUG hack. clear raster line interrupt. 
}
void vicii_destroy() {}

byte vicii_peek(word address) {return mem_nonmappable_peek(address);}
void vicii_poke(word address,byte val) {mem_nonmappable_poke(address,val);}







