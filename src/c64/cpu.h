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
MODULE: cpu.h

WORK ITEMS:

KNOWN BUGS:

*/
#ifndef CPU_H
#define CPU_H 



//
// status flags
//
#define BIT_7 128
#define BIT_6 64
#define BIT_5 32 
#define BIT_4 16
#define BIT_3 8 
#define BIT_2 4
#define BIT_1 2 
#define BIT_0 1 


//
// status flags
//
#define N_FLAG 128
#define V_FLAG 64
#define X_FLAG 32
#define B_FLAG 16
#define D_FLAG 8
#define I_FLAG 4
#define Z_FLAG 2 
#define C_FLAG 1 

#define STACK_BASE 			0x0100 		// stack page 
#define VECTOR_RESET 		0xFFFC		// reset vector 
#define VECTOR_BRK	 		0xFFFE		// break vector  
#define VECTOR_IRQ	 		0xFFFE		// break vector  



typedef unsigned char 		byte; 
typedef unsigned short 		word; 



#define NTSC_CYCLES_PERSECOND 1022727 


//
// 6502 addressing modes
//
typedef enum {

	AM_IMPLICIT,
	AM_ACCUMULATOR,
	AM_IMMEDIATE,
	AM_ZEROPAGE,
	AM_ZEROPAGEX,
	AM_ZEROPAGEY,
	AM_RELATIVE,
	AM_ABSOLUTE,
	AM_ABSOLUTEX,
	AM_ABSOLUTEY,
	AM_INDIRECT,
	AM_INDEXEDINDIRECT,
	AM_INDIRECTINDEXED,
	AM_MAX

} ENUM_AM;

//
// initialization and cleanup routines
//
void cpu_init(void);
void cpu_destroy(void);

//
// state accessors
//
byte cpu_geta(void);
byte cpu_getx(void);
byte cpu_gety(void);
word cpu_getpc(void);
byte cpu_getstatus(void);
byte cpu_getstack(void);

//
// action signals
//

bool cpu_ready(void);   // cpu is ready to run one instruction.
void cpu_update(void);  // run one instruction 
void cpu_irq(void);  // signal irq line
void cpu_nmi(void);  // signal nmi line

//
// 6502 helper routines
//
byte cpu_disassemble(char * buf,word address);


#endif
