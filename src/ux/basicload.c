
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
MODULE: basicload.c
load a basic file into c64 memory.


WORK ITEMS:

KNOWN BUGS:

*/


/*
BUGBUG: This is temporary. Just allows you to insert a basic program in memory without working drive/storage.
Just hacked this together.



*/
#include "emu.h"
#include "cpu.h"
typedef struct {

	word line;
	byte tokens[80];

} BASICLINE;

typedef struct {

	byte 		opcode;
	char * 		name;

} BASICOPCODE;

BASICOPCODE g_basicopcodes[] = 
{
	{0x80,"END"},
	{0x81,"FOR"},
	{0x82,"NEXT"},
	{0x83,"DATA"},
	{0x84,"INPUT#"},
	{0x85,"INPUT"},
	{0x86,"DIM"},
	{0x87,"READ"},
	{0x88,"LET"},
	{0x89,"GOTO"},
	{0x8A,"RUN"},
	{0x8B,"IF"},
	{0x8C,"RESTORE"},
	{0x8D,"GOSUB"},
	{0x8E,"RETURN"},
	{0x8F,"REM"},
	{0x90,"STOP"},
	{0x91,"ON"},
	{0x92,"WAIT"},
	{0x93,"LOAD"},
	{0x94,"SAVE"},
	{0x95,"VERIFY"},
	{0x96,"DEF"},
	{0x97,"POKE"},
	{0x98,"PRINT#"},
	{0x99,"PRINT"},
	{0x9A,"CONT"},
	{0x9B,"LIST"},
	{0x9C,"CLR"},
	{0x9D,"CMD"},
	{0x9E,"SYS"},
	{0x9F,"OPEN"},
	{0xA0,"CLOSE"},
	{0xA1,"GET"},
	{0xA2,"NEW"},
	{0xA3,"TAB("},
	{0xA4,"TO"},
	{0xA5,"FN"},
	{0xA6,"SPC("},
	{0xA7,"THEN"},
	{0xA8,"NOT"},
	{0xA9,"STEP"},
	{0xAA,"+"},
	{0xAB,"-"},
	{0xAC,"*"},
	{0xAD,"/"},
	{0xAE,"^"},
	{0xAF,"AND"},
	{0xB0,"OR"},
	{0xB1,">"},
	{0xB2,"="},
	{0xB3,"<"},
	{0xB4,"SGN"},
	{0xB5,"INT"},
	{0xB6,"ABS"},
	{0xB7,"USR"},
	{0xB8,"FRE"},
	{0xB9,"POS"},
	{0xBA,"SQR"},
	{0xBB,"RND"},
	{0xBC,"LOG"},
	{0xBD,"EXP"},
	{0xBE,"COS"},
	{0xBF,"SIN"},
	{0xC0,"TAN"},
	{0xC1,"ATN"},
	{0xC2,"PEEK"},
	{0xC3,"LEN"},
	{0xC4,"STR$"},
	{0xC5,"VAL"},
	{0xC6,"ASC"},
	{0xC7,"CHR$"},
	{0xC8,"LEFT$"},
	{0xC9,"RIGHT$"},
	{0xCA,"MID$"},
	{0xCB,"GO"},
	{0xCD,"UNUSED"}
};


word bas_getlinenumber(char *line) {

	char numstr[20];
	char outline[256];
	char *p = numstr;
	int linenum;
	int i;

	for (i = 0;isdigit(line[i]);i++) {
		*p++ = line[i];
	}
	*p = 0;
	linenum = atoi(numstr);
	strcpy(outline,line+strlen(numstr));
	strcpy(line,outline);
	return linenum;
}

bool bas_inquotes(char *line,char *substr) {

	char *oq = strchr(line,'\"');
	char *cq;

	while (oq) {
		if (oq > substr) {
			return false;
		}
		cq = strchr(oq+1,'\"');
		if (cq > substr) {
			return true;
		}
		oq = strchr(cq+1,'\"');
	}
	return  false;
}

void bas_replacesubstrwithop(char * line, char *substrstart, byte length,byte op) {

	char outline[256] = "";
	char *out = outline;
	char *in  = line;

	while (in != substrstart) {
		*out++ = *in++;
	}

	*out++ = op;
	in +=  length;

	while (*in) {
		*out++ = *in++;
	}

	strcpy(line,outline);
}

void bas_tokenizeline(char * line) {

	int i = 0; 
	char * cur;

	while (g_basicopcodes[i].opcode != 0xCD) {
		do {
			cur = strstr(line,g_basicopcodes[i].name);
			if (cur) {
				if(!bas_inquotes(line,cur)) {
					bas_replacesubstrwithop(line,cur,strlen(g_basicopcodes[i].name),
						g_basicopcodes[i].opcode & 0xff );
				}
			}
		} while (cur);
		i++;
	}
}





void bas_loadfile(char * string) {

	FILE * f;
	char line[256];
	char outputline[256];
	word mem = 0x0800;
	word link;
	int i;

	f = fopen(string,"r+");
	if (!f) {
		DEBUG_PRINT("Failed to open file %s\n",string);
		return;
	}

	//
	// basic starts with zero byte before first line.
	//
	mem_poke(mem++,0);

 	while (fgets(line, 256, f)) {
 		if (line[strlen(line)-1] == '\n') {
 			line[strlen(line)-1] = 0;
 		} 

 		DEBUG_PRINT("tokenizing line: %s\n",line);
 		
 		//
 		// leave room for next record lnk
 		//
 		link = mem;
 		mem += 2;

 		mem_pokeword(mem,bas_getlinenumber(line));
 		mem+=2;

 		DEBUG_PRINT("After line number its %s\n",line);

 		bas_tokenizeline(line);
 		//DEBUG_PRINT("line: %s\n",line);
 		for (i = 0; i < strlen(line);i++) {
 			//DEBUG_PRINT("%02x \n",line[i]);
 			mem_poke(mem++,line[i]);
 		}
 		//
 		// trailing zero.
 		//
 		mem_poke(mem++,0);
 		//
 		// save link.
 		//
 		mem_pokeword(link,link+strlen(line)+5);
    }

	fclose(f);
}
