
#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include "emu.h"


const char * g_regNames[] = {

	"A",
	"X",
	"Y",
	"SP",
	"PS",
	"MAX_REGISTERS"
};

OPCODE g_opcodes[256];








void incLoc(byte *high, byte * low) {

	*low = *low+1;
	if (!*low) {
		*high = *high + 1;
	}
} 

byte getByte(CPU6502 *c,byte high, byte low) {
	
	return c->ram[high][low];
}

byte getNextByte(CPU6502 *c) {

	byte b;

	b = getByte(c,c->pc_high,c->pc_low++);
	if (!c->pc_low) {
		c->pc_high++;
		c->cycles++;
	}

	return b;
}

void setByte(CPU6502 *c, byte high, byte low, byte val) {

	c->ram[high][low] = val;
}


//
// Sets n flag if N & val otherwise clear
//
void setOrClearNFlag(CPU6502 *c, byte val) {

	c->reg_status = (val & N_FLAG) ? c->reg_status | N_FLAG : 
	c->reg_status & ~N_FLAG;
}
//
// Sets Z flag if val==0 otherwise clear
//
void setOrClearZFlag(CPU6502 *c, byte val) {
	
	c->reg_status = (val) ? (c->reg_status & (~Z_FLAG)) :
	 	(c->reg_status | Z_FLAG);
}
//
// Sets v flag if true otherwise clear
//
void setOrClearVFlag(CPU6502 *c, byte val) {

	c->reg_status = (val) ? c->reg_status | V_FLAG : 
	c->reg_status & ~V_FLAG;
}

//
// Sets v flag if true otherwise clear
//
void setOrClearCFlag(CPU6502 *c, byte val) {

	c->reg_status = (val) ? c->reg_status | C_FLAG : 
	c->reg_status & ~C_FLAG;
}




void setPCFromOffset(CPU6502 * c, byte val) {

	
	bool negative = val & N_FLAG;
	byte oldl;


	if (negative) {
		val = ~val + 1;
	}

	//
	// add a cycle on successful branch.
	//
	c->cycles++;
	oldl = c->pc_low;
	if (negative) {
		c->pc_low -= val;
		if (c->pc_low > oldl) {
			c->pc_high--;
			c->cycles++;
		}
		
	}
	else {
		c->pc_low += val;
		if (c->pc_low < oldl) {
			c->pc_high++;
			c->cycles++;
		}
	}
}


void getloc(CPU6502 *c, ENUM_AM m, byte * high, byte * low) {

	byte indirectl;
	byte indirecth;

	*low = getNextByte(c);
	*high = 0;

	switch(m) {
		case AM_ZEROPAGEX: // LDA $00,X
			*low += c->reg_x;
		break;
		case AM_ABSOLUTE: // LDA $1234
			*high = getNextByte(c);
		break;
		case AM_ABSOLUTEX: // LDA $1234,X
			*high = getNextByte(c);
			*low +=c->reg_x;
		break;
		case AM_ABSOLUTEY: // LDA $1234,Y
			*high = getNextByte(c);
			*low += c->reg_y;
		break;
		case AM_INDIRECT: // JMP ($1234)
			*high = getNextByte(c);
			indirectl = getByte(c,*high,*low);
			indirecth = getByte(c,*high,*low+1);
			*low = indirectl;
			*high = indirecth;
		break;
		case AM_INDEXEDINDIRECT: // LDA ($00,X)
		 	indirectl = getByte(c,*high,*low + c->reg_x);
			indirecth = getByte(c,*high,*low + c->reg_x + 1);
			*low = indirectl;
			*high = indirecth;
		break;
		case AM_INDIRECTINDEXED: // LDA ($00),Y
			indirectl = getByte(c,*high,*low);
			indirecth = getByte(c,*high,*low+1);
			*low = indirectl + c->reg_y;
			*high = indirecth;
		break;
		default: break;
	}

}

unsigned char getval(CPU6502 *c,ENUM_AM m) {

	byte high;
	byte low;
	byte val;

	if (m == AM_IMMEDIATE) {
		val = getNextByte(c);
	}
	else {
		getloc(c,m,&high,&low);
		val = getByte(c,high,low);
	}
	return val;
}

void handle_JMP(CPU6502 * c,ENUM_AM m) {

	byte high;
	byte low;



	getloc(c,m,&high,&low);

	c->pc_high = high;
	c->pc_low = low;
}

void handle_JSR(CPU6502 * c,ENUM_AM m) {

	byte high;
	byte low;

	getloc(c,m,&high,&low);

	setByte(c,STACK_BASE,c->reg_stack--,c->pc_high);	
	setByte(c,STACK_BASE,c->reg_stack--,c->pc_low-1);

	c->pc_high = high;
	c->pc_low = low;
}

void handle_RTS(CPU6502 * c,ENUM_AM m) {

	c->pc_low = getByte(c,STACK_BASE,++c->reg_stack) + 1;
	c->pc_high = getByte(c,STACK_BASE,++c->reg_stack);
}

void handle_BIT(CPU6502 * c,ENUM_AM m) {

	byte val = c->reg_a & getval(c,m);
	
	setOrClearNFlag(c,c->reg_a);
	setOrClearZFlag(c,c->reg_a);
	
	c->reg_status = (val & V_FLAG) ? (c->reg_status | V_FLAG) :
		(c->reg_status & (~V_FLAG));
}

void handle_AND(CPU6502 * c,ENUM_AM m) {

	c->reg_a &= getval(c,m);
	setOrClearNFlag(c,c->reg_a);
	setOrClearZFlag(c,c->reg_a);
}

void handle_EOR(CPU6502 * c,ENUM_AM m) {

	c->reg_a ^= getval(c,m);
	setOrClearNFlag(c,c->reg_a);
	setOrClearZFlag(c,c->reg_a);
}

void handle_ORA(CPU6502 * c,ENUM_AM m) {

	c->reg_a |= getval(c,m);
	setOrClearNFlag(c,c->reg_a);
	setOrClearZFlag(c,c->reg_a);
}

void handle_LDA(CPU6502 * c,ENUM_AM m) {

	c->reg_a = getval(c,m);
	setOrClearNFlag(c,c->reg_a);
	setOrClearZFlag(c,c->reg_a);
}

void handle_LDY(CPU6502 * c,ENUM_AM m) {

	c->reg_y = getval(c,m);
	setOrClearNFlag(c,c->reg_y);
	setOrClearZFlag(c,c->reg_y);
}

void handle_LDX(CPU6502 * c,ENUM_AM m) {

	c->reg_x = getval(c,m);
	setOrClearNFlag(c,c->reg_x);
	setOrClearZFlag(c,c->reg_x);
}


void handle_STA(CPU6502 * c,ENUM_AM m) {

	byte high;
	byte low;
	getloc(c,m,&high,&low);
	setByte(c,high,low,c->reg_a);
}

void handle_STX(CPU6502 * c,ENUM_AM m) {

	byte high;
	byte low;
	getloc(c,m,&high,&low);
	setByte(c,high,low,c->reg_x);
}

void handle_STY(CPU6502 * c,ENUM_AM m) {

	byte high;
	byte low;
	getloc(c,m,&high,&low);
	setByte(c,high,low,c->reg_y);
}

void handle_TAX(CPU6502 * c,ENUM_AM m) {

	c->reg_x = c->reg_a;
	setOrClearNFlag(c,c->reg_x);
	setOrClearZFlag(c,c->reg_x);
}


void handle_TAY(CPU6502 * c,ENUM_AM m) {

	c->reg_y = c->reg_a;
	setOrClearNFlag(c,c->reg_y);
	setOrClearZFlag(c,c->reg_y);
}


void handle_TXA(CPU6502 * c,ENUM_AM m) {

	c->reg_a = c->reg_x;	
	setOrClearNFlag(c,c->reg_a);
	setOrClearZFlag(c,c->reg_a);
}


void handle_TYA(CPU6502 * c,ENUM_AM m) {

	c->reg_a = c->reg_y;
	setOrClearNFlag(c,c->reg_a);
	setOrClearZFlag(c,c->reg_a);
}

void handle_TXS(CPU6502 * c,ENUM_AM m) {

	c->reg_stack = c->reg_x;	
}

void handle_TSX(CPU6502 * c,ENUM_AM m) {

	c->reg_x = c->reg_stack;
	setOrClearNFlag(c,c->reg_x);
	setOrClearZFlag(c,c->reg_x);
}

void handle_PHA(CPU6502 * c,ENUM_AM m) {

	setByte(c,STACK_BASE,c->reg_stack--,c->reg_a);
}

void handle_PLA(CPU6502 * c,ENUM_AM m) {

	c->reg_a = getByte(c,STACK_BASE,++c->reg_stack);
	setOrClearNFlag(c,c->reg_a);
	setOrClearZFlag(c,c->reg_a);
}

void handle_PHP(CPU6502 * c,ENUM_AM m) {

	setByte(c,STACK_BASE,c->reg_stack--,c->reg_status);
}

void handle_PLP(CPU6502 * c,ENUM_AM m) {

	c->reg_status = getByte(c,STACK_BASE,++c->reg_stack);
	
}

void handle_INC(CPU6502 * c,ENUM_AM m) {

	byte high;
	byte low;
	byte val;
	
	getloc(c,m,&high,&low);
	val =  getByte(c,high,low) + 1;
	setByte(c,high,low,val);
	setOrClearNFlag(c,val);
	setOrClearZFlag(c,val);
}

void handle_DEC(CPU6502 * c,ENUM_AM m) {

	byte high;
	byte low;
	byte val;
	
	getloc(c,m,&high,&low);
	val =  getByte(c,high,low) - 1;
	setByte(c,high,low,val);
	setOrClearNFlag(c,val);
	setOrClearZFlag(c,val);
}

void handle_INX(CPU6502 * c,ENUM_AM m) {

	c->reg_x++;
	setOrClearNFlag(c,c->reg_x);
	setOrClearZFlag(c,c->reg_x);
}

void handle_INY(CPU6502 * c,ENUM_AM m) {

	c->reg_y++;
	setOrClearNFlag(c,c->reg_y);
	setOrClearZFlag(c,c->reg_y);
}

void handle_DEX(CPU6502 * c,ENUM_AM m) {

	c->reg_x--;
	setOrClearNFlag(c,c->reg_x);
	setOrClearZFlag(c,c->reg_x);
}


void handle_DEY(CPU6502 * c,ENUM_AM m) {

	c->reg_y--;
	setOrClearNFlag(c,c->reg_y);
	setOrClearZFlag(c,c->reg_y);
}

void handle_NOP(CPU6502 *c, ENUM_AM m) {}

void handle_CLC (CPU6502 * c,ENUM_AM m) {

	c->reg_status &= ~C_FLAG;
}

void handle_CLD (CPU6502 * c,ENUM_AM m) {

	c->reg_status &= ~D_FLAG;
}

void handle_CLI (CPU6502 * c,ENUM_AM m) {

	c->reg_status &= ~I_FLAG;
}

void handle_CLV (CPU6502 * c,ENUM_AM m) {

	c->reg_status = ~V_FLAG;
}

void handle_SEC (CPU6502 * c,ENUM_AM m) {

	c->reg_status |= C_FLAG;
}

void handle_SED (CPU6502 * c,ENUM_AM m) {

	c->reg_status |= D_FLAG;
}

void handle_SEI (CPU6502 * c,ENUM_AM m) {

	c->reg_status |= I_FLAG;
}

void handle_BRK (CPU6502 * c,ENUM_AM m) {

	//
	// push program counter onto the stack followed by processor status
	//
	setByte(c,STACK_BASE,c->reg_stack--,c->pc_high);	
	setByte(c,STACK_BASE,c->reg_stack--,c->pc_low);
	setByte(c,STACK_BASE,c->reg_stack--,c->reg_status);
	//
	// load interrupt vector
	//
	c->pc_low = getByte(c,VECTOR_BASE,BRK_LOW);
	c->pc_high = getByte(c,VECTOR_BASE,BRK_HIGH);

	//
	// set break flag
	//
	c->reg_status |= B_FLAG;
}

void handle_RTI (CPU6502 * c,ENUM_AM m) {

	c->reg_status = getByte(c,STACK_BASE,++c->reg_stack);
	c->pc_low = getByte(c,STACK_BASE,++c->reg_stack);
	c->pc_high = getByte(c,STACK_BASE,++c->reg_stack);

	c->reg_status &= ~B_FLAG;
}


void handle_BCC (CPU6502 * c,ENUM_AM m) {

	byte val = getNextByte(c);
	if ((c->reg_status & C_FLAG) == 0) {
		setPCFromOffset(c,val);
	} 
}

void handle_BCS (CPU6502 * c,ENUM_AM m) {

	byte val = getNextByte(c);
	if (c->reg_status & C_FLAG) {
		setPCFromOffset(c,val);
	} 
}

void handle_BEQ (CPU6502 * c,ENUM_AM m) {

	byte val = getNextByte(c);
	if (c->reg_status & Z_FLAG) {
		setPCFromOffset(c,val);
	} 
}

void handle_BMI (CPU6502 * c,ENUM_AM m) {

	byte val = getNextByte(c);
	if (c->reg_status & N_FLAG) {
		setPCFromOffset(c,val);
	} 
}

void handle_BPL (CPU6502 * c,ENUM_AM m) {

	byte val = getNextByte(c);
	if ((c->reg_status & N_FLAG) == 0) {
		setPCFromOffset(c,val);
	} 
}

void handle_BNE (CPU6502 * c,ENUM_AM m) {


	byte val = getNextByte(c);

	if ((c->reg_status & Z_FLAG) == 0) {
	
			setPCFromOffset(c,val);	
	} 
}

void handle_BVC (CPU6502 * c,ENUM_AM m) {

	byte val = getNextByte(c);
	if ((c->reg_status & V_FLAG) == 0) {
		setPCFromOffset(c,val);
	} 
}

void handle_BVS (CPU6502 * c,ENUM_AM m) {

	byte val = getNextByte(c);
	if (c->reg_status & V_FLAG) {
		setPCFromOffset(c,val);
	} 
}

void handle_LSR (CPU6502 *c, ENUM_AM m) {

	byte src; 
	byte high;
	byte low;

	if (m == AM_IMPLICIT) {
		src = c->reg_a;
	}
	else {
		getloc(c,m,&high,&low);
		src = getByte(c,high,low);
	}

	setOrClearCFlag(c,src & 0x01);
	src >>= 1;
	setOrClearNFlag(c,src);
	setOrClearZFlag(c,src);

	if (m == AM_IMPLICIT) {
		c->reg_a = src;
	}
	else {
		setByte(c,high,low,src);
	}	

}

void handle_ROL (CPU6502 *c, ENUM_AM m) {

	unsigned int src; 
	byte high;
	byte low;

	if (m == AM_IMPLICIT) {
		src = c->reg_a;
	}
	else {
		getloc(c,m,&high,&low);
		src = getByte(c,high,low);
	}

	src <<=1;
	if (c->reg_status & C_FLAG) {
		src |= 0x1;
	}
	setOrClearCFlag(c,src > 0xff);
	src &= 0xff;
	setOrClearNFlag(c,src);
	setOrClearZFlag(c,src);

	if (m == AM_IMPLICIT) {
		c->reg_a = src;
	}
	else {
		setByte(c,high,low,src);
	}	

}

void handle_ROR (CPU6502 *c, ENUM_AM m) {

	unsigned int src; 
	byte high;
	byte low;

	if (m == AM_IMPLICIT) {
		src = c->reg_a;
	}
	else {
		getloc(c,m,&high,&low);
		src = getByte(c,high,low);
	}

	
	if (c->reg_status & C_FLAG) {
		src |= 0x100;
	}
	setOrClearCFlag(c,src & 0x01);
	src >>= 1;
	src &=0xff;
	setOrClearNFlag(c,src);
	setOrClearZFlag(c,src);

	if (m == AM_IMPLICIT) {
		c->reg_a = src;
	}
	else {
		setByte(c,high,low,src);
	}	

}


void handle_ASL (CPU6502 *c, ENUM_AM m) {

	byte src; 
	byte high;
	byte low;

	if (m == AM_IMPLICIT) {
		src = c->reg_a;
	}
	else {
		getloc(c,m,&high,&low);
		src = getByte(c,high,low);
	}

	setOrClearCFlag(c,src & 0x80);
	src <<= 1;
	setOrClearNFlag(c,src);
	setOrClearZFlag(c,src);

	if (m == AM_IMPLICIT) {
		c->reg_a = src;
	}
	else {
		setByte(c,high,low,src);
	}	

}


void handle_CMP (CPU6502 *c, ENUM_AM m) {

	byte src = getval(c,m);
	byte res = c->reg_a - src;
	
	setOrClearCFlag(c,c->reg_a >= src);
	setOrClearZFlag(c,res);
	setOrClearNFlag(c,res);
}

void handle_CPX (CPU6502 *c, ENUM_AM m) {

	byte src = getval(c,m); 
	byte res = c->reg_x - src;

	setOrClearCFlag(c,c->reg_x >= src);
	setOrClearZFlag(c,res);
	setOrClearNFlag(c,res);
}

void handle_CPY (CPU6502 *c, ENUM_AM m) {

	byte src = getval(c,m); 
	byte res = c->reg_y - src;

	setOrClearCFlag(c,c->reg_y >= src);
	setOrClearZFlag(c,res);
	setOrClearNFlag(c,res);
}

void handle_SBC (CPU6502 *c, ENUM_AM m) {

	unsigned int temp;
	unsigned int temp2;
	byte src = getval(c,m);

	temp = 	 c->reg_a - src - ((c->reg_status & C_FLAG) ? 0 : 1);
	
	setOrClearNFlag(c,temp);
	setOrClearZFlag(c,temp & 0xff);
	setOrClearVFlag(c,((c->reg_a ^ temp) &0x80) && ((c->reg_a ^ src) & 0x80));
	if (c->reg_status & D_FLAG) {
		if (((c->reg_a & 0xf) - ((c->reg_status & C_FLAG) ? 0 : 1)) < (src & 0xf)) {
			temp -= 6;
		}
		if (temp > 0x99) {
			temp += 0x60; 
		}
	}



/*
	uint16_t diff = lhs - rhs - carry;
	if((lhs & 0xf) - carry < (rhs & 0xf))
	{
		diff -= 0x6;
	}
	if(diff > 0x99)
	{
		diff += 0x60;
	}
	return diff;
}
*/
	setOrClearCFlag(c,temp < 0x100);
	c->reg_a = (temp & 0xff);




}



void handle_ADC (CPU6502 *c, ENUM_AM m) {

	//
	// start with the carry flag;
	//
	unsigned int temp = (c->reg_status & C_FLAG) ? 1 : 0;
	byte src = getval(c,m);

	//
	// add in the accumuator and location.
	//
	temp += c->reg_a;
	temp += src;
    
	setOrClearZFlag(c,temp & 0xff);	// not valid in BCD;
    if (c->reg_status & D_FLAG) {
    	//
    	// BCD
    	//
    	if (((c->reg_a & 0xf) + (src & 0xf) + (c->reg_status & C_FLAG) ? 1 : 0) > 9) {
    		temp += 6;
    	} 
    	setOrClearNFlag(c,temp);
    	setOrClearVFlag(c,!((c->reg_a ^ src) & 0x80) && ((c->reg_a ^ temp) & 0x80));
    	if (temp > 0x99) {
    		temp += 96;
    	}
    	setOrClearCFlag(c,temp > 0x99);
    } else {
    	setOrClearNFlag(c,temp);
    	setOrClearVFlag(c,!((c->reg_a ^ src) & 0x80) && ((c->reg_a ^ temp) & 0x80));
    	setOrClearCFlag(c,temp > 0xff);

    }
    c->reg_a = (byte) temp;
}



void runcpu(CPU6502 *c) {

	byte op;

	op = getNextByte(c);
	g_opcodes[op].fn(c,g_opcodes[op].am);
	c->cycles += g_opcodes[op].cycles;

}

void setopcode(int op, char * name,ENUM_AM mode,OPHANDLER fn,byte c) {
	g_opcodes[op].name = name;
	g_opcodes[op].op = op;
	g_opcodes[op].am = mode;
	g_opcodes[op].fn = fn;
	g_opcodes[op].cycles = c;
}

void initPC(CPU6502 *c) {


	c->pc_low = c->ram[VECTOR_BASE][RESET_LOW];
	c->pc_high = c->ram[VECTOR_BASE][RESET_HIGH];
}

void destroy_computer(CPU6502 *c) {
	fclose(c->log);
}

void init_computer(CPU6502 *c) {

	int i = 0;
	//
	// clear all memory
	//
	memset (c,0,sizeof(CPU6502));	

	c->log = fopen("cpu.log","w+");
	fprintf(c->log,"starting cpu...\n");
	


	//
	// load all opcodes
	//
	for (i = 0; i < 256;i++ ) {
		g_opcodes[i].name = "NOP";
		g_opcodes[i].fn = handle_NOP;
		g_opcodes[i].op = i;
	}

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
	// 6502 reset vector
	//
	c->ram[VECTOR_BASE][RESET_LOW] = 0xE2;
	c->ram[VECTOR_BASE][RESET_HIGH] = 0xFC;

	initPC(c);
}



