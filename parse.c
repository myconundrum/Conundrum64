/**
Parse handles command line parsing and reading files from input. 

**/



#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include "emu.h"


typedef void (*PARSEFN)(UX *,char *);

typedef struct {

	const char * cmd;
	PARSEFN fn;

} PARSECMD;


void parseQuit(UX * ux, char * s) {

	ux->done = true;
}

void parseMem (UX * ux, char * s) {

	s = strtok(NULL," ");
	if (s) {
		ux->curpage = strtoul(s,NULL,16);
	}
}


void fillDisassembly(UX *ux, word address) {

	int i;

	ux->disstart = 0;
	ux->discur = 0;

	for (i =0 ; i <  DISLINESCOUNT; i++) {
		ux->dislines[i].address = address;
		disassembleLine(ux->assembler,ux->dislines[i].buf,&address);
	}
}



void parseDis (UX * ux, char * s) {

	word address;
	int  i;

	s = strtok(NULL," ");
	if (s) {
		address = strtoul(s,NULL,16);
	}
	else {
		address = cpu_getpc();
	}

	fillDisassembly(ux,address);
	
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

void parseOp (UX *ux,  char *s) {
	
	char buf[4];
	ADDRESSANDMODE am;
	int i = 0;
	ENUM_AM m;
	char opbuf[10];

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

		if (cpu_getopcodeinfo(i,opbuf,&m) &&
			!strcmp(buf,opbuf) && am.mode == m) {
		
			mem_poke(ux->asm_address++,i);
			if (am.bytes) {
				mem_poke(ux->asm_address++,am.low);
			}
			if (am.bytes == 2) {
				mem_poke(ux->asm_address++,am.hi);
			}
		}
	}	
}


void parseAsm (UX* ux,char * s) {
	s = strtok(NULL," ");
	if (s) {
		parseOp (ux,s);
	}
}

void parseBrk (UX* ux,char * s) {
	s = strtok(NULL," ");
	ux->brk_address = 0;

	if (s) {
		ux->brk = true;
		ux->brk_address = strtoul(s,NULL,16);
	}
}


void parseAsmat (UX* ux,char * s) {
	s = strtok(NULL," ");
	if (s) {
		ux->asm_address = strtoul(s,NULL,16);
	}
}

void parseExec (UX *ux,  char *s) {

	ux->running = true;
}

void parseStop (UX *ux, char *s) {

	ux->running = false;
}

void parsePassThru (UX *ux, char *s) {


	ux->passthru = true;
}


void parseAsmfile (UX *ux, char *s) {
	
	char * p = strtok(NULL," ");
	if (p) {
		assembleFile(ux->assembler,p);
	}
	
}

void parseComment (UX *ux, char *s) {}

void parseStep(UX *ux, char *s) {
	cpu_run();
}

PARSECMD g_commands[] = {
	{"QUIT",parseQuit},
	{"MEM",parseMem},
	{"BRK",parseBrk},
	{"ASM",parseAsm},
	{"ASMAT",parseAsmat},
	{"EXEC",parseExec},
	{"STOP",parseStop},
	{"DIS",parseDis},
	{"@",parseAsmfile},
	{"S",parseStep},
	{";",parseComment},
	{"\"",parsePassThru},
};

void handle_step(UX * ux) {

	cpu_run();
	ux->discur++;

	if (cpu_getpc() != ux->dislines[ux->discur].address || ux->discur == DISLINESCOUNT) {

		fillDisassembly(ux,cpu_getpc());
	}

}

void handle_command(UX * ux) {

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
			g_commands[i].fn(ux,p);
			break;
		}
	}
	if (i==sizeof(g_commands)/sizeof(PARSECMD)) {
		DEBUG_PRINT("Error. Could not parse %s\n ",ux->buf);
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