#ifndef MEMH
#define MEMH
#include "emu.h"

typedef void (*POKEHANDLER)(word,byte);
typedef byte (*PEEKHANDLER)(word);


#define MEM_PAGE_SIZE 	0xFF
#define MEM_PAGE_COUNT 	0xFF


void mem_init();
void mem_destroy();


void mem_poke(word address,byte value);
void mem_pokeword(word address,word value);
byte mem_peek(word address);
word mem_peekword(word address);




//
// memory mapping functions for i/o and other.
//
byte 	mem_map (word lowaddress,word hiaddress,PEEKHANDLER peekfn, POKEHANDLER pokefn);
void 	mem_mapactive (byte id, bool flag);
byte    mem_nonmappable_peek(word address);				
void    mem_nonmappable_poke(word address,byte val); 	

#endif
