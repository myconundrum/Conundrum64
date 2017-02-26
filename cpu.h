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




extern void runcpu();
extern void init_computer();
extern void destroy_computer();

byte cpu_geta();
byte cpu_getx();
byte cpu_gety();
word cpu_getpc();
byte cpu_getstatus();
byte cpu_getstack();
unsigned int cpu_getcycles();


bool cpu_isopcode(char * name);
bool cpu_getopcodeinfo(byte opcode, char *name, ENUM_AM *mode);



#endif
