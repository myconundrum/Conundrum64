#ifndef CPU_H
#define CPU_H 


//
// status flags
//
#define N_FLAG 0b10000000
#define V_FLAG 0b01000000
#define X_FLAG 0b00100000 
#define B_FLAG 0b00010000
#define D_FLAG 0b00001000 
#define I_FLAG 0b00000100
#define Z_FLAG 0b00000010 
#define C_FLAG 0b00000001 

#define STACK_BASE 			0x0100 		// stack page 
#define VECTOR_RESET 		0xFFFC		// reset vector 
#define VECTOR_BRK	 		0xFFFE		// break vector  


typedef unsigned char 		byte; 
typedef unsigned short 		word; 

typedef struct cpu6502 {

	//byte pc_low; 				// low byte of program counter
	//byte pc_high;				// hi byte of program counter
	byte reg_a;					// accumulator
	byte reg_x;					// x register
	byte reg_y;					// y register
	byte reg_status;			// status byte
	byte reg_stack;				// stack pointer
	word pc;					// program counter;

	unsigned int cycles; 		// tracks total cycles run. 
	FILE * log;					// log file. 
	

} CPU6502;


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




typedef void (*OPHANDLER)(CPU6502*,ENUM_AM);

typedef struct {

	const char * 		name;
	unsigned char 		op;
	ENUM_AM				am;
	OPHANDLER			fn;
	byte 				cycles;

} OPCODE;

extern OPCODE g_opcodes[256];


extern void runcpu(CPU6502 *);
extern void init_computer(CPU6502 *);
extern void destroy_computer(CPU6502 *);


void incLoc(byte *high, byte * low);
byte getByte(CPU6502 *c,byte high, byte low);
byte getNextByte(CPU6502 *c);
void setByte(CPU6502 *c, byte high, byte low, byte val);




#endif
