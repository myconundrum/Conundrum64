#ifndef ASM_H
#define ASM_H

#include "emu.h"

typedef enum {

	ASM_NONE,
	ASM_COMMENT, 
	ASM_PCDIRECTIVE,
	ASM_IDENTIFIER,
	ASM_DECNUMBER,
	ASM_HEXNUMBER,
	ASM_PRAGMASPECIFIER,
	ASM_EQUAL,
	ASM_COLON,
	ASM_STRING,
	ASM_IMMEDIATESPECIFIER,
	ASM_HEXSPECIFIER,
	ASM_PLUS, 
	ASM_MINUS,
	ASM_OPENPAREN,
	ASM_CLOSEPAREN,
	ASM_COMMA,
	ASM_INVALID,
	ASM_OPCODE,

} TOKEN_TYPE;


typedef struct {

	TOKEN_TYPE type;
	char string[256];
	char name[25];

} TOKEN;

typedef struct {
	char string[256]; 
	int  pos;
	bool hex;

} LINE; 



typedef bool (*TOKENRULE)(LINE*,TOKEN *,int,TOKEN_TYPE, char *);
typedef struct {
	TOKENRULE fn;
	TOKEN_TYPE type;
	int val;
	char * name;
} TOKEN_RECORD;


typedef struct {

	char name[8];
	byte high;
	byte low;

} KEYVALUE;

#define MAX_LABELS 256

typedef struct {

	int rulecount;
	TOKEN_RECORD tokenrules[256];

	FILE * log;
	CPU6502 * cpu;
	byte asmh;
	byte asml;


	KEYVALUE dict[MAX_LABELS];
	int dlast; 


} ASSEMBLER;

void assembleFile(ASSEMBLER *a, char * file);
void init_assembler(ASSEMBLER * a, CPU6502 *c);
void loadBinFile(ASSEMBLER *a, char * file, byte high, byte low); 

#endif 