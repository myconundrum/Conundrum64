/*
Conundrum 64: Commodore 64 Emulator

MIT License

Copyright (c) 2017 Marc R. Whitten

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
MODULE: mem.h

WORK ITEMS:

KNOWN BUGS:

*/
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
