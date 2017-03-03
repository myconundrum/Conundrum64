
#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include "emu.h"

typedef struct cpu6502 {

	byte reg_a;					// accumulator
	byte reg_x;					// x register
	byte reg_y;					// y register
	byte reg_status;			// status byte
	byte reg_stack;				// stack pointer
	word pc;					// program counter;

	bool irq;					// irq signal.
	bool nmi;					// nmi signal.

} CPU6502;


typedef void (*OPHANDLER)(ENUM_AM);

typedef struct {

	const char * 		name;
	unsigned char 		op;
	ENUM_AM				am;
	OPHANDLER			fn;
	byte 				cycles;

} OPCODE;


OPCODE g_opcodes[256];
CPU6502 g_cpu;

byte fetch() {

	if (((g_cpu.pc + 1) & 0xFF) == 0) {
		sysclock_addticks(1);
	}

	return mem_peek(g_cpu.pc++);
}

word fetch_word() {

	return fetch() | (fetch() << 8);

}


bool cpu_isopcode(char * name) {

	int i;
	for (i = 0;i < 0xFF; i++) {
		if (!strcmp(g_opcodes[i].name,name)) {
			return true;
		}
	}
	return false;
}


bool cpu_getopcodeinfo(byte opcode, char *name, ENUM_AM * mode) {

	if (g_opcodes[opcode].am == AM_MAX) {
		return false;
	}

	strcpy(name, g_opcodes[opcode].name);
	*mode = g_opcodes[opcode].am;
	return true;
}


//
// Sets n flag if N & val otherwise clear
//
void setOrClearNFlag(byte val) {

	g_cpu.reg_status = (val & N_FLAG) ? g_cpu.reg_status | N_FLAG : 
	g_cpu.reg_status & ~N_FLAG;
}
//
// Sets Z flag if val==0 otherwise clear
//
void setOrClearZFlag(byte val) {
	
	g_cpu.reg_status = (val) ? (g_cpu.reg_status & (~Z_FLAG)) :
	 	(g_cpu.reg_status | Z_FLAG);
}
//
// Sets v flag if true otherwise clear
//
void setOrClearVFlag(byte val) {

	g_cpu.reg_status = (val) ? g_cpu.reg_status | V_FLAG : 
	g_cpu.reg_status & ~V_FLAG;
}

//
// Sets v flag if true otherwise clear
//
void setOrClearCFlag(byte val) {

	g_cpu.reg_status = (val) ? g_cpu.reg_status | C_FLAG : 
	g_cpu.reg_status & ~C_FLAG;
}


void setPCFromOffset(byte val) {
	
	word old = g_cpu.pc;

	if (val & N_FLAG) {
		val = ~val + 1;
		g_cpu.pc -= val;
	}
	else {
		g_cpu.pc += val;
	}

	//
	// add a cycle on successful branch.
	//
	sysclock_addticks(1);

	//
	// add cycle on page boundary cross.
	//
	if ((g_cpu.pc & 0xFF) != (old & 0xFF)) {
		sysclock_addticks(1);
	}
}


word cpu_getloc(ENUM_AM mode) {

	word address = 0x00; 

	switch(mode) {
		case AM_ZEROPAGE: 
			address = fetch();
		break;
		case AM_ZEROPAGEX: // LDA $00,X
			address = fetch() + g_cpu.reg_x;
		break;
		case AM_ABSOLUTE: // LDA $1234
			address = fetch_word();
		break;
		case AM_ABSOLUTEX: // LDA $1234,X
			address = fetch_word() + g_cpu.reg_x;
		break;
		case AM_ABSOLUTEY: // LDA $1234,Y
			address = fetch_word() + g_cpu.reg_y;
		break;
		case AM_INDIRECT: // JMP ($1234)
			address = fetch_word();
			address = mem_peekword(address);
		break;
		case AM_INDEXEDINDIRECT: // LDA ($00,X)
			address = mem_peekword(fetch() + g_cpu.reg_x);
		break;
		case AM_INDIRECTINDEXED: // LDA ($00),Y
			address = fetch();
			address = mem_peekword(address) + g_cpu.reg_y;
		break;
		default: break;
	}

	return address;
}

unsigned char getval(ENUM_AM m) {

	return m == AM_IMMEDIATE ? fetch() : mem_peek(cpu_getloc(m));
}

void handle_JMP(ENUM_AM m) {

	g_cpu.pc = cpu_getloc(m);
}

void handle_JSR(ENUM_AM m) {

	word address = cpu_getloc(m);

	g_cpu.pc--;

	mem_poke(STACK_BASE | g_cpu.reg_stack--,(byte) (g_cpu.pc >> 8));	
	mem_poke(STACK_BASE | g_cpu.reg_stack--,(byte) (g_cpu.pc & 0xFF));
	
	g_cpu.pc = address;
}

void handle_RTS(ENUM_AM m) {

	g_cpu.pc = mem_peek(STACK_BASE | ++g_cpu.reg_stack) ;
	g_cpu.pc |= ((word) mem_peek(STACK_BASE | ++g_cpu.reg_stack) << 8);
	g_cpu.pc++;
}

void handle_BIT(ENUM_AM m) {

	byte val = getval(m);
	setOrClearNFlag(val & N_FLAG);
	setOrClearVFlag(val & V_FLAG);
	setOrClearZFlag(!(val & g_cpu.reg_a));
}

void handle_AND(ENUM_AM m) {

	g_cpu.reg_a &= getval(m);
	setOrClearNFlag(g_cpu.reg_a);
	setOrClearZFlag(g_cpu.reg_a);
}

void handle_EOR(ENUM_AM m) {

	g_cpu.reg_a ^= getval(m);
	setOrClearNFlag(g_cpu.reg_a);
	setOrClearZFlag(g_cpu.reg_a);
}


void handle_ORA(ENUM_AM m) {

	g_cpu.reg_a |= getval(m);
	setOrClearNFlag(g_cpu.reg_a);
	setOrClearZFlag(g_cpu.reg_a);
}

void handle_LDA(ENUM_AM m) {

	g_cpu.reg_a = getval(m);
	setOrClearNFlag(g_cpu.reg_a);
	setOrClearZFlag(g_cpu.reg_a);
}

void handle_LDY(ENUM_AM m) {
	
	g_cpu.reg_y = getval(m);
	setOrClearNFlag(g_cpu.reg_y);
	setOrClearZFlag(g_cpu.reg_y);
}

void handle_LDX(ENUM_AM m) {

	g_cpu.reg_x = getval(m);
	setOrClearNFlag(g_cpu.reg_x);
	setOrClearZFlag(g_cpu.reg_x);
}


void handle_STA(ENUM_AM m) {
	mem_poke(cpu_getloc(m), g_cpu.reg_a);
}

void handle_STX(ENUM_AM m) {
	mem_poke(cpu_getloc(m),g_cpu.reg_x);
}

void handle_STY(ENUM_AM m) {
	mem_poke(cpu_getloc(m),g_cpu.reg_y);
}

void handle_TAX(ENUM_AM m) {

	g_cpu.reg_x = g_cpu.reg_a;
	setOrClearNFlag(g_cpu.reg_x);
	setOrClearZFlag(g_cpu.reg_x);
}


void handle_TAY(ENUM_AM m) {

	g_cpu.reg_y = g_cpu.reg_a;
	setOrClearNFlag(g_cpu.reg_y);
	setOrClearZFlag(g_cpu.reg_y);
}


void handle_TXA(ENUM_AM m) {

	g_cpu.reg_a = g_cpu.reg_x;	
	setOrClearNFlag(g_cpu.reg_a);
	setOrClearZFlag(g_cpu.reg_a);
}


void handle_TYA(ENUM_AM m) {

	g_cpu.reg_a = g_cpu.reg_y;
	setOrClearNFlag(g_cpu.reg_a);
	setOrClearZFlag(g_cpu.reg_a);
}

void handle_TXS(ENUM_AM m) {
	g_cpu.reg_stack = g_cpu.reg_x;	
}

void handle_TSX(ENUM_AM m) {
	g_cpu.reg_x = g_cpu.reg_stack;
	setOrClearNFlag(g_cpu.reg_x);
	setOrClearZFlag(g_cpu.reg_x);
}

void handle_PHA(ENUM_AM m) {
	mem_poke( STACK_BASE | g_cpu.reg_stack--,g_cpu.reg_a);
}

void handle_PLA(ENUM_AM m) {

	g_cpu.reg_a = mem_peek(STACK_BASE | ++g_cpu.reg_stack);
	setOrClearNFlag(g_cpu.reg_a);
	setOrClearZFlag(g_cpu.reg_a);
}

void handle_PHP(ENUM_AM m) {

	mem_poke( STACK_BASE | g_cpu.reg_stack--,g_cpu.reg_status);
	
}

void handle_PLP(ENUM_AM m) {

	g_cpu.reg_status = mem_peek(STACK_BASE | ++g_cpu.reg_stack);
	
}

void handle_INC(ENUM_AM m) {

	byte val;	
	word address = cpu_getloc(m);
	val =  mem_peek(address) + 1;
	mem_poke(address, val);
	setOrClearNFlag(val);
	setOrClearZFlag(val);
}

void handle_DEC(ENUM_AM m) {

	byte val;	
	word address = cpu_getloc(m);
	val =  mem_peek(address) - 1;
	mem_poke(address, val);
	setOrClearNFlag(val);
	setOrClearZFlag(val);
}

void handle_INX(ENUM_AM m) {

	g_cpu.reg_x++;
	setOrClearNFlag(g_cpu.reg_x);
	setOrClearZFlag(g_cpu.reg_x);
}

void handle_INY(ENUM_AM m) {

	g_cpu.reg_y++;
	setOrClearNFlag(g_cpu.reg_y);
	setOrClearZFlag(g_cpu.reg_y);
}

void handle_DEX(ENUM_AM m) {

	g_cpu.reg_x--;
	setOrClearNFlag(g_cpu.reg_x);
	setOrClearZFlag(g_cpu.reg_x);
}


void handle_DEY(ENUM_AM m) {

	g_cpu.reg_y--;
	setOrClearNFlag(g_cpu.reg_y);
	setOrClearZFlag(g_cpu.reg_y);
}

void handle_NOP(ENUM_AM m) {}

void handle_CLC (ENUM_AM m) {

	g_cpu.reg_status &= ~C_FLAG;
}

void handle_CLD (ENUM_AM m) {

	g_cpu.reg_status &= ~D_FLAG;
}

void handle_CLI (ENUM_AM m) {

	g_cpu.reg_status &= ~I_FLAG;
}

void handle_CLV (ENUM_AM m) {

	g_cpu.reg_status &= ~V_FLAG;
}

void handle_SEC (ENUM_AM m) {

	g_cpu.reg_status |= C_FLAG;
}

void handle_SED (ENUM_AM m) {

	g_cpu.reg_status |= D_FLAG;
}

void handle_SEI (ENUM_AM m) {

	g_cpu.reg_status |= I_FLAG;
}

void handle_BRK (ENUM_AM m) {

	//
	// push program counter onto the stack followed by processor status
	//
	mem_poke(STACK_BASE | g_cpu.reg_stack--,g_cpu.pc >> 8);
	mem_poke(STACK_BASE | g_cpu.reg_stack--,g_cpu.pc &  0xFF);
	mem_poke(STACK_BASE | g_cpu.reg_stack--,g_cpu.reg_status);
	
	//
	// load interrupt vector
	//
	g_cpu.pc = mem_peekword(VECTOR_BRK);

	//
	// set break flag
	//
	g_cpu.reg_status |= B_FLAG;
}

void handle_RTI (ENUM_AM m) {

	g_cpu.reg_status = mem_peek(STACK_BASE | ++g_cpu.reg_stack);
	g_cpu.pc = mem_peekword(STACK_BASE | ++g_cpu.reg_stack);
	g_cpu.reg_stack++;

	g_cpu.reg_status &= ~B_FLAG;

}


void handle_BCC (ENUM_AM m) {

	byte val = fetch();
	if ((g_cpu.reg_status & C_FLAG) == 0) {
		setPCFromOffset(val);
	} 
}

void handle_BCS (ENUM_AM m) {

	byte val = fetch();
	if (g_cpu.reg_status & C_FLAG) {
		setPCFromOffset(val);
	} 
}

void handle_BEQ (ENUM_AM m) {

	byte val = fetch();
	if (g_cpu.reg_status & Z_FLAG) {
		setPCFromOffset(val);
	} 
}

void handle_BMI (ENUM_AM m) {

	byte val = fetch();
	if (g_cpu.reg_status & N_FLAG) {
		setPCFromOffset(val);
	} 
}

void handle_BPL (ENUM_AM m) {

	byte val = fetch();
	if ((g_cpu.reg_status & N_FLAG) == 0) {
		setPCFromOffset(val);
	} 
}

void handle_BNE (ENUM_AM m) {

	byte val = fetch();
	if ((g_cpu.reg_status & Z_FLAG) == 0) {
			setPCFromOffset(val);	
	} 
}

void handle_BVC (ENUM_AM m) {

	byte val = fetch();
	if ((g_cpu.reg_status & V_FLAG) == 0) {
		setPCFromOffset(val);
	} 
}

void handle_BVS (ENUM_AM m) {

	byte val = fetch();
	if (g_cpu.reg_status & V_FLAG) {
		setPCFromOffset(val);
	} 
}

void handle_LSR (ENUM_AM m) {

	byte src; 
	word address;

	if (m == AM_IMPLICIT) {
		src = g_cpu.reg_a;
	}
	else {
		address = cpu_getloc(m);
		src = mem_peek(address);
	}

	setOrClearCFlag(src & 0x01);
	src >>= 1;
	setOrClearNFlag(src);
	setOrClearZFlag(src);

	if (m == AM_IMPLICIT) {
		g_cpu.reg_a = src;
	}
	else {
		mem_poke(address, src);
	}	

}

void handle_ROL (ENUM_AM m) {

	unsigned int src; 
	word address;

	if (m == AM_IMPLICIT) {
		src = g_cpu.reg_a;
	}
	else {
		address = cpu_getloc(m);
		src = mem_peek(address);
	}

	src <<=1;
	if (g_cpu.reg_status & C_FLAG) {
		src |= 0x1;
	}
	setOrClearCFlag(src > 0xff);
	src &= 0xff;
	setOrClearNFlag(src);
	setOrClearZFlag(src);

	if (m == AM_IMPLICIT) {
		g_cpu.reg_a = src;
	}
	else {
		mem_poke(address, src);
	}	

}

void handle_ROR (ENUM_AM m) {

	unsigned int src; 
	word address;

	if (m == AM_IMPLICIT) {
		src = g_cpu.reg_a;
	}
	else {
		address = cpu_getloc(m);
		src = mem_peek(address);
	}

	
	if (g_cpu.reg_status & C_FLAG) {
		src |= 0x100;
	}
	setOrClearCFlag(src & 0x01);
	src >>= 1;
	src &=0xff;
	setOrClearNFlag(src);
	setOrClearZFlag(src);

	if (m == AM_IMPLICIT) {
		g_cpu.reg_a = src;
	}
	else {
		mem_poke(address, src);
	}	
}


void handle_ASL (ENUM_AM m) {

	byte src; 
	word address;

	if (m == AM_IMPLICIT) {
		src = g_cpu.reg_a;
	}
	else {
		address = cpu_getloc(m);
		src = mem_peek(address);
	}

	setOrClearCFlag(src & 0x80);
	src <<= 1;
	setOrClearNFlag(src);
	setOrClearZFlag(src);

	if (m == AM_IMPLICIT) {
		g_cpu.reg_a = src;
	}
	else {
		mem_poke(address, src);
	}	

}

void handle_CMP (ENUM_AM m) {

	byte src = getval(m);
	byte res = g_cpu.reg_a - src;
	
	setOrClearCFlag(g_cpu.reg_a >= src);
	setOrClearZFlag(res);
	setOrClearNFlag(res);
}

void handle_CPX (ENUM_AM m) {

	byte src = getval(m); 
	byte res = g_cpu.reg_x - src;

	setOrClearCFlag(g_cpu.reg_x >= src);
	setOrClearZFlag(res);
	setOrClearNFlag(res);
}

void handle_CPY (ENUM_AM m) {

	byte src = getval(m); 
	byte res = g_cpu.reg_y - src;

	setOrClearCFlag(g_cpu.reg_y >= src);
	setOrClearZFlag(res);
	setOrClearNFlag(res);
}


void handle_SBC (ENUM_AM m) {
 		
	word src, tmp;
	unsigned int tmp_a;  

	src = getval(m);

	tmp = g_cpu.reg_a - src - ((g_cpu.reg_status & C_FLAG) ? 0 : 1);

	if (g_cpu.reg_status & D_FLAG) {                                                            
	    
	    tmp_a = (g_cpu.reg_a & 0xf) - (src & 0xf) - ((g_cpu.reg_status & C_FLAG) ? 0 : 1);         
	    
	    if (tmp_a & 0x10) {                                                             
	        tmp_a = ((tmp_a - 6) & 0xf) | ((g_cpu.reg_a & 0xf0) - (src & 0xf0) - 0x10);  
	    } else {                                                                        
	        tmp_a = (tmp_a & 0xf) | ((g_cpu.reg_a& 0xf0) - (src & 0xf0));               
	    }                                                                               
	    if (tmp_a & 0x100) {                                                            
	        tmp_a -= 0x60;                                                              
	    }
	    setOrClearCFlag(tmp < 0x100);
	    setOrClearNFlag(tmp & 0xff);
	    setOrClearZFlag(tmp & 0xff);
	    setOrClearVFlag(((g_cpu.reg_a ^ tmp) & 0x80) && ((g_cpu.reg_a ^ src) & 0x80)); 
	    g_cpu.reg_a = tmp_a & 0xff;

	} else {   
		setOrClearNFlag(tmp & 0xff);
		setOrClearZFlag(tmp & 0xff);
		setOrClearCFlag(tmp < 0x100);                                                   
	    setOrClearVFlag(((g_cpu.reg_a ^ tmp) & 0x80) && ((g_cpu.reg_a ^ src) & 0x80)); 
	    g_cpu.reg_a = tmp & 0xff;                                                      
	}          
}


void handle_ADC (ENUM_AM m) {
		
	unsigned int src = getval(m);                                                                     
   	unsigned int tmp;                                                                           
              
    if (g_cpu.reg_status & D_FLAG) {

		tmp = (g_cpu.reg_a & 0xf) + (src & 0xf) + (g_cpu.reg_status & 0x1);                           
	    if (tmp > 0x9) {                                                                        
	        tmp += 0x6;                                                                         
	    }                                                                                       
	    if (tmp <= 0x0f) {                                                                      
	        tmp = (tmp & 0xf) + (g_cpu.reg_a & 0xf0) + (src & 0xf0);                       
	    } else {                                                                                
	        tmp = (tmp & 0xf) + (g_cpu.reg_a & 0xf0) + (src & 0xf0) + 0x10;                
	    }
	    setOrClearZFlag(!((g_cpu.reg_a + src + (g_cpu.reg_status & 0x1)) & 0xff));                                                                                       
	    setOrClearNFlag(tmp & 0x80);                                                             
	    setOrClearVFlag(((g_cpu.reg_a ^ tmp) & 0x80)  && !((g_cpu.reg_a ^ src) & 0x80)); 
	    if ((tmp & 0x1f0) > 0x90) {                                                             
	        tmp += 0x60;                                                                        
	    }                                                                                       
	    setOrClearCFlag((tmp & 0xff0) > 0xf0); 
	} else {                                                                                    
	    tmp = src + g_cpu.reg_a + (g_cpu.reg_status & C_FLAG);     
	    setOrClearZFlag(tmp&0xff);
	    setOrClearNFlag(tmp&0xff);   
	    setOrClearVFlag(!((g_cpu.reg_a ^ src) & 0x80)  && ((g_cpu.reg_a ^ tmp) & 0x80));                                                           
	   	setOrClearCFlag(tmp > 0xff);                                                            
	}                 

	g_cpu.reg_a = tmp; 
}


void cpu_checkinterrupts() {

	if (g_cpu.nmi) {

		//
		// BUGBUG not implemented yet...
		//
		g_cpu.nmi = false;


	}
	if (g_cpu.irq && !(g_cpu.reg_status & I_FLAG)) {
		//
		// save PC and status on IRQ
		//
		mem_poke(STACK_BASE | g_cpu.reg_stack--,g_cpu.pc >> 8);
		mem_poke(STACK_BASE | g_cpu.reg_stack--,g_cpu.pc &  0xFF);
		mem_poke(STACK_BASE | g_cpu.reg_stack--,g_cpu.reg_status);

		g_cpu.pc = mem_peekword(VECTOR_BRK);
		g_cpu.irq = false;
	
	}

}


void cpu_run() {

	byte op;


	cpu_checkinterrupts();

	op = fetch();
	g_opcodes[op].fn(g_opcodes[op].am);
	sysclock_addticks(g_opcodes[op].cycles);

}

void setopcode(int op, char * name,ENUM_AM mode,OPHANDLER fn,byte c) {
	g_opcodes[op].name = name;
	g_opcodes[op].op = op;
	g_opcodes[op].am = mode;
	g_opcodes[op].fn = fn;
	g_opcodes[op].cycles = c;
}


void cpu_destroy() {

}

void cpu_init() {

	int i = 0;
	//
	// clear all memory
	//
	memset (&g_cpu,0,sizeof(CPU6502));	

	
	DEBUG_PRINT("starting cpu...\n");
	
	//
	// load all opcodes
	//
	for (i = 0; i < 256;i++ ) {
		g_opcodes[i].name = "NOP";
		g_opcodes[i].fn = handle_NOP;
		g_opcodes[i].op = i;
		g_opcodes[i].am = AM_MAX;
	}

	
	setopcode(0xea,"NOP",AM_IMPLICIT,handle_NOP,2);	

	setopcode(0xa9,"LDA",AM_IMMEDIATE,handle_LDA,2);
	setopcode(0xa5,"LDA",AM_ZEROPAGE,handle_LDA,3);
	setopcode(0xb5,"LDA",AM_ZEROPAGEX,handle_LDA,4);
	setopcode(0xad,"LDA",AM_ABSOLUTE,handle_LDA,4);
	setopcode(0xbd,"LDA",AM_ABSOLUTEX,handle_LDA,4);
	setopcode(0xb9,"LDA",AM_ABSOLUTEY,handle_LDA,4);
	setopcode(0xa1,"LDA",AM_INDEXEDINDIRECT,handle_LDA,6);
	setopcode(0xb1,"LDA",AM_INDIRECTINDEXED,handle_LDA,5);

	setopcode(0xa2,"LDX",AM_IMMEDIATE,handle_LDX,2);
	setopcode(0xa6,"LDX",AM_ZEROPAGE,handle_LDX,3);
	setopcode(0xb6,"LDX",AM_ZEROPAGEY,handle_LDX,4);
	setopcode(0xae,"LDX",AM_ABSOLUTE,handle_LDX,4);
	setopcode(0xbe,"LDX",AM_ABSOLUTEY,handle_LDX,4);

	setopcode(0xa0,"LDY",AM_IMMEDIATE,handle_LDY,2);
	setopcode(0xa4,"LDY",AM_ZEROPAGE,handle_LDY,3);
	setopcode(0xb4,"LDY",AM_ZEROPAGEX,handle_LDY,4);
	setopcode(0xac,"LDY",AM_ABSOLUTE,handle_LDY,4);
	setopcode(0xbc,"LDY",AM_ABSOLUTEX,handle_LDY,4);

	setopcode(0x85,"STA",AM_ZEROPAGE,handle_STA,3);
	setopcode(0x95,"STA",AM_ZEROPAGEX,handle_STA,4);
	setopcode(0x8d,"STA",AM_ABSOLUTE,handle_STA,4);
	setopcode(0x9d,"STA",AM_ABSOLUTEX,handle_STA,5);
	setopcode(0x99,"STA",AM_ABSOLUTEY,handle_STA,5);
	setopcode(0x81,"STA",AM_INDEXEDINDIRECT,handle_STA,6);
	setopcode(0x91,"STA",AM_INDIRECTINDEXED,handle_STA,6);

	setopcode(0x86,"STX",AM_ZEROPAGE,handle_STX,3);
	setopcode(0x96,"STX",AM_ZEROPAGEY,handle_STX,4);
	setopcode(0x8e,"STX",AM_ABSOLUTE,handle_STX,4);
	
	setopcode(0x84,"STY",AM_ZEROPAGE,handle_STY,3);
	setopcode(0x94,"STY",AM_ZEROPAGEX,handle_STY,4);
	setopcode(0x8c,"STY",AM_ABSOLUTE,handle_STY,4);

	setopcode(0xaa,"TAX",AM_IMPLICIT,handle_TAX,2);
	setopcode(0xa8,"TAY",AM_IMPLICIT,handle_TAY,2);
	setopcode(0x8a,"TXA",AM_IMPLICIT,handle_TXA,2);
	setopcode(0x98,"TYA",AM_IMPLICIT,handle_TYA,2);
	setopcode(0x9a,"TXS",AM_IMPLICIT,handle_TXS,2);
	setopcode(0xba,"TSX",AM_IMPLICIT,handle_TSX,2);

	setopcode(0x48,"PHA",AM_IMPLICIT,handle_PHA,3);
	setopcode(0x68,"PLA",AM_IMPLICIT,handle_PLA,4);
	setopcode(0x08,"PHP",AM_IMPLICIT,handle_PHP,3);
	setopcode(0x28,"PLP",AM_IMPLICIT,handle_PLP,4);

	setopcode(0xe6,"INC",AM_ZEROPAGE,handle_INC,5);
	setopcode(0xf6,"INC",AM_ZEROPAGEX,handle_INC,6);
	setopcode(0xee,"INC",AM_ABSOLUTE,handle_INC,6);
	setopcode(0xfe,"INC",AM_ABSOLUTEX,handle_INC,7);
	
	setopcode(0xc6,"DEC",AM_ZEROPAGE,handle_DEC,5);
	setopcode(0xd6,"DEC",AM_ZEROPAGEX,handle_DEC,6);
	setopcode(0xce,"DEC",AM_ABSOLUTE,handle_DEC,6);
	setopcode(0xde,"DEC",AM_ABSOLUTEX,handle_DEC,7);

	setopcode(0xe8,"INX",AM_IMPLICIT,handle_INX,2);
	setopcode(0xc8,"INY",AM_IMPLICIT,handle_INY,2);	
	setopcode(0xca,"DEX",AM_IMPLICIT,handle_DEX,2);
	setopcode(0x88,"DEY",AM_IMPLICIT,handle_DEY,2);

	setopcode(0x29,"AND",AM_IMMEDIATE,handle_AND,2);
	setopcode(0x25,"AND",AM_ZEROPAGE,handle_AND,3);
	setopcode(0x35,"AND",AM_ZEROPAGEX,handle_AND,4);
	setopcode(0x2d,"AND",AM_ABSOLUTE,handle_AND,4);
	setopcode(0x3d,"AND",AM_ABSOLUTEX,handle_AND,4);
	setopcode(0x39,"AND",AM_ABSOLUTEY,handle_AND,4);
	setopcode(0x21,"AND",AM_INDEXEDINDIRECT,handle_AND,6);
	setopcode(0x31,"AND",AM_INDIRECTINDEXED,handle_AND,5);

	setopcode(0x49,"EOR",AM_IMMEDIATE,handle_EOR,2);
	setopcode(0x45,"EOR",AM_ZEROPAGE,handle_EOR,3);
	setopcode(0x55,"EOR",AM_ZEROPAGEX,handle_EOR,4);
	setopcode(0x4d,"EOR",AM_ABSOLUTE,handle_EOR,4);
	setopcode(0x5d,"EOR",AM_ABSOLUTEX,handle_EOR,4);
	setopcode(0x59,"EOR",AM_ABSOLUTEY,handle_EOR,4);
	setopcode(0x41,"EOR",AM_INDEXEDINDIRECT,handle_EOR,6);
	setopcode(0x51,"EOR",AM_INDIRECTINDEXED,handle_EOR,5);

	setopcode(0x09,"ORA",AM_IMMEDIATE,handle_ORA,2);
	setopcode(0x05,"ORA",AM_ZEROPAGE,handle_ORA,3);
	setopcode(0x15,"ORA",AM_ZEROPAGEX,handle_ORA,4);
	setopcode(0x0d,"ORA",AM_ABSOLUTE,handle_ORA,4);
	setopcode(0x1d,"ORA",AM_ABSOLUTEX,handle_ORA,4);
	setopcode(0x19,"ORA",AM_ABSOLUTEY,handle_ORA,4);
	setopcode(0x01,"ORA",AM_INDEXEDINDIRECT,handle_ORA,6);
	setopcode(0x11,"ORA",AM_INDIRECTINDEXED,handle_ORA,5);

	setopcode(0x24,"BIT",AM_ZEROPAGE,handle_BIT,3);
	setopcode(0x2c,"BIT",AM_ABSOLUTE,handle_BIT,4);
	

	setopcode(0x18,"CLC",AM_IMPLICIT,handle_CLC,2);
	setopcode(0xd8,"CLD",AM_IMPLICIT,handle_CLD,2);
	setopcode(0x58,"CLI",AM_IMPLICIT,handle_CLI,2);
	setopcode(0xb8,"CLV",AM_IMPLICIT,handle_CLV,2);
	setopcode(0x38,"SEC",AM_IMPLICIT,handle_SEC,2);
	setopcode(0xf8,"SED",AM_IMPLICIT,handle_SED,2);
	setopcode(0x78,"SEI",AM_IMPLICIT,handle_SEI,2);

	setopcode(0x00,"BRK",AM_IMPLICIT,handle_BRK,7);
	setopcode(0x40,"RTI",AM_IMPLICIT,handle_RTI,6);

	setopcode(0x4c,"JMP",AM_ABSOLUTE,handle_JMP,3);
	setopcode(0x6c,"JMP",AM_INDIRECT,handle_JMP,5);

	setopcode(0x20,"JSR",AM_ABSOLUTE,handle_JSR,6);
	setopcode(0x60,"RTS",AM_IMPLICIT,handle_RTS,6);

	setopcode(0x90,"BCC",AM_RELATIVE,handle_BCC,2);
	setopcode(0xb0,"BCS",AM_RELATIVE,handle_BCS,2);
	setopcode(0xf0,"BEQ",AM_RELATIVE,handle_BEQ,2);
	setopcode(0x30,"BMI",AM_RELATIVE,handle_BMI,2);
	setopcode(0xd0,"BNE",AM_RELATIVE,handle_BNE,2);
	setopcode(0x10,"BPL",AM_RELATIVE,handle_BPL,2);
	setopcode(0x50,"BVC",AM_RELATIVE,handle_BVC,2);
	setopcode(0x70,"BVS",AM_RELATIVE,handle_BVS,2);
	
	setopcode(0x69,"ADC",AM_IMMEDIATE,handle_ADC,2);
	setopcode(0x65,"ADC",AM_ZEROPAGE,handle_ADC,3);
	setopcode(0x75,"ADC",AM_ZEROPAGEX,handle_ADC,4);
	setopcode(0x6d,"ADC",AM_ABSOLUTE,handle_ADC,4);
	setopcode(0x7d,"ADC",AM_ABSOLUTEX,handle_ADC,4);
	setopcode(0x79,"ADC",AM_ABSOLUTEY,handle_ADC,4);
	setopcode(0x61,"ADC",AM_INDEXEDINDIRECT,handle_ADC,6);
	setopcode(0x71,"ADC",AM_INDIRECTINDEXED,handle_ADC,5);

	setopcode(0xe9,"SBC",AM_IMMEDIATE,handle_SBC,2);
	setopcode(0xe5,"SBC",AM_ZEROPAGE,handle_SBC,3);
	setopcode(0xf5,"SBC",AM_ZEROPAGEX,handle_SBC,4);
	setopcode(0xed,"SBC",AM_ABSOLUTE,handle_SBC,4);
	setopcode(0xfd,"SBC",AM_ABSOLUTEX,handle_SBC,4);
	setopcode(0xf9,"SBC",AM_ABSOLUTEY,handle_SBC,4);
	setopcode(0xe1,"SBC",AM_INDEXEDINDIRECT,handle_SBC,6);
	setopcode(0xf1,"SBC",AM_INDIRECTINDEXED,handle_SBC,5);

	setopcode(0xc9,"CMP",AM_IMMEDIATE,handle_CMP,2);
	setopcode(0xc5,"CMP",AM_ZEROPAGE,handle_CMP,3);
	setopcode(0xd5,"CMP",AM_ZEROPAGEX,handle_CMP,4);
	setopcode(0xcd,"CMP",AM_ABSOLUTE,handle_CMP,4);
	setopcode(0xdd,"CMP",AM_ABSOLUTEX,handle_CMP,4);
	setopcode(0xd9,"CMP",AM_ABSOLUTEY,handle_CMP,4);
	setopcode(0xc1,"CMP",AM_INDEXEDINDIRECT,handle_CMP,6);
	setopcode(0xd1,"CMP",AM_INDIRECTINDEXED,handle_CMP,5);

	setopcode(0xe0,"CPX",AM_IMMEDIATE,handle_CPX,2);
	setopcode(0xe4,"CPX",AM_ZEROPAGE,handle_CPX,3);
	setopcode(0xec,"CPX",AM_ABSOLUTE,handle_CPX,4);
	
	setopcode(0xc0,"CPY",AM_IMMEDIATE,handle_CPY,2);
	setopcode(0xc4,"CPY",AM_ZEROPAGE,handle_CPY,3);
	setopcode(0xcc,"CPY",AM_ABSOLUTE,handle_CPY,4);

	setopcode(0x0a,"ASL",AM_IMPLICIT,handle_ASL,2);
	setopcode(0x06,"ASL",AM_ZEROPAGE,handle_ASL,5);
	setopcode(0x16,"ASL",AM_ZEROPAGEX,handle_ASL,6);
	setopcode(0x0e,"ASL",AM_ABSOLUTE,handle_ASL,6);
	setopcode(0x1e,"ASL",AM_ABSOLUTEX,handle_ASL,7);

	setopcode(0x4a,"LSR",AM_IMPLICIT,handle_LSR,2);
	setopcode(0x46,"LSR",AM_ZEROPAGE,handle_LSR,5);
	setopcode(0x56,"LSR",AM_ZEROPAGEX,handle_LSR,6);
	setopcode(0x4e,"LSR",AM_ABSOLUTE,handle_LSR,6);
	setopcode(0x5e,"LSR",AM_ABSOLUTEX,handle_LSR,7);
	
	setopcode(0x2a,"ROL",AM_IMPLICIT,handle_ROL,2);
	setopcode(0x26,"ROL",AM_ZEROPAGE,handle_ROL,5);
	setopcode(0x36,"ROL",AM_ZEROPAGEX,handle_ROL,6);
	setopcode(0x2e,"ROL",AM_ABSOLUTE,handle_ROL,6);
	setopcode(0x3e,"ROL",AM_ABSOLUTEX,handle_ROL,7);
	
	setopcode(0x6a,"ROR",AM_IMPLICIT,handle_ROR,2);
	setopcode(0x66,"ROR",AM_ZEROPAGE,handle_ROR,5);
	setopcode(0x76,"ROR",AM_ZEROPAGEX,handle_ROR,6);
	setopcode(0x6e,"ROR",AM_ABSOLUTE,handle_ROR,6);
	setopcode(0x7e,"ROR",AM_ABSOLUTEX,handle_ROR,7);
	

	//
	// Set initial bankswitch configuration (IO, Basic, Kernal mapped in)
	//
	mem_poke(BANKSWITCH_ADDRESS,0xE7);
	g_cpu.pc = mem_peekword(VECTOR_RESET);



}

void cpu_irq() {g_cpu.irq = true;}
void cpu_nmi() {g_cpu.nmi = true;}

byte cpu_geta() 				{return g_cpu.reg_a;}
byte cpu_getx()					{return g_cpu.reg_x;}
byte cpu_gety()					{return g_cpu.reg_y;}
word cpu_getpc() 				{return g_cpu.pc;}
byte cpu_getstatus()			{return g_cpu.reg_status;}	
byte cpu_getstack()				{return g_cpu.reg_stack;}


