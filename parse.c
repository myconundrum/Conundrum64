/**
Parse handles command line parsing and reading files from input. 

**/



#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include "emu.h"


typedef void (*PARSEFN)(UX *,CPU6502 *,char *);

typedef struct {

	const char * cmd;
	PARSEFN fn;

} PARSECMD;


void parseQuit(UX * ux, CPU6502 *c, char * s) {

	ux->done = true;
}

void parseMem (UX * ux, CPU6502 *c, char * s) {

	s = strtok(NULL," ");
	if (s) {
		ux->curpage = strtoul(s,NULL,16);
	}
}


void fillDisassembly(UX *ux, CPU6502 *c, byte high, byte low) {

	int i;


	ux->disstart = 0;
	ux->discur = 0;

	for (i =0 ; i <  DISLINESCOUNT; i++) {
		ux->dislines[i].high = high;
		ux->dislines[i].low = low;
		disassembleLine(ux->assembler,ux->dislines[i].buf,&high,&low);
	}

}



void parseDis (UX * ux, CPU6502 *c, char * s) {

	byte dish;
	byte disl;
	int  i;

	s = strtok(NULL," ");
	if (s) {
		dish = strtoul(s,NULL,16);
		s = strtok(NULL," ");
		disl = strtoul(s,NULL,16);
	}
	else {
		dish = c->pc_high;
		disl = c->pc_low;
	}

	fillDisassembly(ux,c,dish,disl);
	
}

void parseSet (UX * ux, CPU6502 *c, char * s) {

	int val = 0;
	char * p; 
	s = strtok(NULL," ");

	if (s) {
		p = strtok(NULL," ");
		if (p) {
			val = strtol(p,NULL,16);
		}

		if (strcmp(s,"A")==0) {
			c->reg_a = val;
		}
		if (strcmp(s,"X")==0) {
			c->reg_x = val;
		}
		if (strcmp(s,"Y")==0) {
			c->reg_y = val;
		}
	}
}

typedef struct {
	byte hi;
	byte low;
	ENUM_AM mode;
	byte bytes;

} ADDRESSANDMODE;

int getBytesFromString(char *s, ADDRESSANDMODE * am) {

	char lows[3];
	char his[3];
	int count =0;
	int i;

	for (i = 0; i < strlen(s); i++) {
		if (!isxdigit(s[i])) {break;}
		if (i < 2) {
			his[i] = s[i];
		}
		else if (i < 4) {
			lows[i-2] = s[i];
		}	
		count++;	
	}

	if (count > 2) {
		am->hi = strtoul(his,NULL,16);
		am->low = strtoul(lows,NULL,16);
		am->bytes = 2;
	} else {
		am->hi= 0;
		am->low = strtol(his,NULL,16);
		am->bytes = 1;
	}

	return count;
}

void getAddressAndMode (char *s, ADDRESSANDMODE * am) {

	int i;
	bool immediate = false;
	bool indirect = false;
	bool xreg = false;
	bool yreg = false;
	int numlen = 0;


	for (i = 0; i < strlen(s); i++) {
		switch(s[i]) {
			case '#': immediate = true; break;
			case 'X': xreg = true; break;
			case 'Y': yreg = true; break;
			case '(': indirect = true; break;
			case '$': 
				numlen = getBytesFromString(s+i+1,am);
				i+=numlen;
				break;
			default:break; 
		}
	}

	if (immediate) {
		am->mode = AM_IMMEDIATE;
	}
	else if (indirect && am->bytes == 2) {
		am->mode = AM_INDIRECT;
	}
	else if (indirect && xreg) {
		am->mode = AM_INDEXEDINDIRECT;
	}
	else if (indirect && yreg) {
		am->mode = AM_INDIRECTINDEXED;
	}
	else if (am->bytes==2  && xreg) {
		am->mode = AM_ABSOLUTEX;
	}
	else if (am->bytes==2 && yreg) {
		am->mode = AM_ABSOLUTEY;
	}
	else if (am->bytes==2) {
		am->mode = AM_ABSOLUTE;
	}
	else if (am->bytes==1 && xreg) {
		am->mode = AM_ZEROPAGEX;
	}
	else if (am->bytes==1 && yreg) {
		am->mode = AM_ZEROPAGEY;
	}
	else if (am->bytes==1)  {

		am->mode = AM_ZEROPAGE;
	}
}

void parseOp (UX *ux, CPU6502 *c, char *s) {
	
	char buf[4];
	ADDRESSANDMODE am;
	int i = 0;
	OPCODE *op;

	strncpy(buf,s,4);
	s = strtok(NULL," ");
	if (s) {
		getAddressAndMode(s,&am);
	} else {
		am.bytes = 0;
		am.mode = AM_IMPLICIT;
	} 

	//
	// fix up branch instructions.
	//
	if (buf[0] == 'B' && am.mode == AM_ZEROPAGE && (strcmp(buf,"BIT") != 0)) {
		am.mode = AM_RELATIVE;
	}

	for (int i = 0; i < 256; i++) {

		op = &g_opcodes[i];

		if ((strcmp(buf,op->name)==0) && am.mode == op->am) {
			c->ram[ux->asmh][ux->asml++] = op->op;
			if (am.bytes) {
				c->ram[ux->asmh][ux->asml++] = am.low;
			}
			if (am.bytes == 2) {
				c->ram[ux->asmh][ux->asml++] = am.hi;
			}
		}
	}	
}


void parseAsm (UX* ux,CPU6502 *c, char * s) {
	s = strtok(NULL," ");
	if (s) {
		parseOp (ux,c,s);
	}
}

void parseBrk (UX* ux,CPU6502 *c, char * s) {
	s = strtok(NULL," ");
	ux->brkh = 0;
	ux->brkl = 0;

	if (s) {
		ux->brk = true;
		ux->brkh = strtoul(s,NULL,16);
		s = strtok(NULL," ");

		if (s) {
			ux->brkl = strtoul(s,NULL,16);
		}
	}
}


void parseAsmat (UX* ux,CPU6502 *c, char * s) {
	char * h;
	char * l;
	h = strtok(NULL," ");
	if (h) {
		l = strtok(NULL," ");
		if (l) {
			ux->asmh = atoi(h);
			ux->asml = atoi(l);
		}
	}
}

void parseExec (UX *ux, CPU6502 *c, char *s) {

	ux->running = true;
}

void parseStop (UX *ux, CPU6502 *c, char *s) {

	ux->running = false;
}


void parseAsmfile (UX *ux, CPU6502 *c, char *s) {
	
	char * p = strtok(NULL," ");
	if (p) {
		assembleFile(ux->assembler,p);
	}
	
}

void parseComment (UX *ux, CPU6502 *c, char *s) {}
void parsePC (UX *ux, CPU6502 *c, char *s) {

	ADDRESSANDMODE am;
	char * p; 
	
	fprintf(ux->log,"parsepc...%s\n",s);
	p = strtok(NULL," ");
	if (p) {
		p = strtok(NULL," ");
	}
	fprintf(ux->log,"next %s\n",p);

	if (p) {
		fprintf(ux->log,"starting parsepc\n");
		*p++ = 0;
		getAddressAndMode(p,&am);
		ux->asmh = am.hi;
		ux->asml = am.low;
		fprintf(ux->log,"setting asmat to %x %x\n",ux->asmh,ux->asml);
	}

}

void parseStep(UX *ux, CPU6502 *c, char *s) {
	runcpu(c);
}

PARSECMD g_commands[] = {
	{"QUIT",parseQuit},
	{"MEM",parseMem},
	{"BRK",parseBrk},
	{"SET",parseSet},
	{"ASM",parseAsm},
	{"ASMAT",parseAsmat},
	{"EXEC",parseExec},
	{"STOP",parseStop},
	{"DIS",parseDis},
	{"@",parseAsmfile},
	{"S",parseStep},
	{";",parseComment},
	{"*",parsePC}
};

void handle_step(UX * ux,CPU6502 *c) {

	if (c->pc_high != ux->dislines[ux->discur].high || c->pc_low != ux->dislines[ux->discur].low
		|| ux->discur == DISLINESCOUNT) {

		fillDisassembly(ux,c,c->pc_high,c->pc_low);
	}

	runcpu(c);
	ux->discur++;
}

void handle_command(UX * ux, CPU6502 *c) {

	char * p = NULL;
	int i;

	p = strtok(ux->buf," ");
	if (!p) {
		if (!p) {
			return;
		}
	}

	
	for (i = 0; i < sizeof(g_commands)/sizeof(PARSECMD); i++) {
		if (strcmp(g_commands[i].cmd,p)==0) {
			g_commands[i].fn(ux,c,p);
			break;
		}
	}
	if (i==sizeof(g_commands)/sizeof(PARSECMD)) {
		fprintf(ux->log,"Error. Could not parse %s\n ",ux->buf);
	}
}

/*

Syntax

The assembler supports the following syntax:

Opcodes and Addressing
  	Instructions are always 3 letter mnemonics followed by an (optional) operand/address:
  	OPC	....	implied
 	OPC A	....	Accumulator
 	OPC #BB	....	immediate
 	OPC HHLL	....	absolute
 	OPC HHLL,X	....	absolute, X-indexed
 	OPC HHLL,Y	....	absolute, Y-indexed
 	OPC *LL	....	zeropage
 	OPC *LL,X	....	zeropage, X-indexed
 	OPC *LL,Y	....	zeropage, Y-indexed
 	OPC (BB,X)	....	X-indexed, indirect
 	OPC (LL),Y	....	indirect, Y-indexed
 	OPC (HHLL)	....	indirect
 	OPC BB	....	relative
 	 
Where HHLL is a 16 bit word and LL or BB a 8 bit byte, and A is literal "A".
There must not be any white space in any part of an instruction's address.
 
Number Formats
 
  	$[0-9A-Zaz]	....	hex
 	[0-9]	....	decimal

Labels and Identifiers
  	Identifiers must begin with a letter [A-Z] and contain letters, digits, and the underscore [A-Z0-9_]. Only the first 6 characters are significant.
All identifiers, numbers, opcodes, and pragmas are case insensitive and translated to upper case. Identifiers must not be the same as valid opcodes.
The special identifier "*" refers to the program counter (PC).
  	Exampels:
 	* = $C000	....	Set start address (PC) to C000.
 	LABEL1 LDA #4	....	Define LABEL1 with the address of instruction LDA.
 	       BNE LABEL2	....	Jump to address of label LABEL2.
 	STORE = $0800	....	Define STORE with value 0800.
 	HERE = *	....	Define HERE with current address (PC).
 	HERE2	....	Define HERE2 with current address (PC).
 	       LDA #<VAL1	....	Load LO-byte of VAL1.
 
Pragmas
  	Pragmas start with a dot (.) and must be the only expression in a line:
  	.STRING "STR"		Insert string at current address
  	.BYTE BB	....	Insert 8 bit byte at current address into code.
  	.WORD HHLL	....	Insert 16 bit word at current address into code.
  	.END	....	End of source, stop assembly. (optional)
 
Comments
  	; comment	....	Any sequence of characters starting with a semicolon till the end of the line are ignored.
*/