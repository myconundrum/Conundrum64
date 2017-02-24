#include "emu.h"
#include <string.h>


void ruleNewLine(ASSEMBLER * a, LINE * line);


int dfind(ASSEMBLER *a, char *name) {

	int i;
	for (i = 0; i < a->dlast; i++) {
		if (strcmp(a->dict[i].name,name)==0) {
			return i;
		}
	}
	return -1;
}

void dadd(ASSEMBLER * a, char * name, byte high, byte low) {

	if (a->dlast == MAX_LABELS) {
		fprintf(a->log,"ASM: Error, too many labels. Ignoring.\n");
		return;
	}

	if (dfind(a,name) != -1) {
		fprintf(a->log,"ASM: Error, attempt to redefine label. Ignoring.\n");
		return;
	}

	strcpy(a->dict[a->dlast].name,name);
	a->dict[a->dlast].high = high;
	a->dict[a->dlast].low = low;
	a->dlast++;
}

char curch(LINE *line) {
	return line->string[line->pos]; 
}

char * curstr(LINE *line) {
	return line->string + line->pos;
}

void setcur(LINE *line, char ch) {
	line->string[line->pos] = ch;
}

bool isopcode(TOKEN * tok) {

	int i;
	OPCODE * op;
	for (int i = 0; i < 256; i++) {

		op = &g_opcodes[i];

		if (strcmp(tok->string,op->name)==0) {
			return true;
		}
	}	
	return false;
}

void addTokenRule(ASSEMBLER * a, TOKENRULE fn,TOKEN_TYPE type, int val,char * name) {

	a->tokenrules[a->rulecount].fn = fn;
	a->tokenrules[a->rulecount].type = type;
	a->tokenrules[a->rulecount].val  = val;
	a->tokenrules[a->rulecount].name  = name;
	a->rulecount++;
}

bool tr_singlechrule(LINE * line, TOKEN * tok, int val, TOKEN_TYPE type, char * name) {

	if (curch(line) == val) {
		*tok->string = curch(line);
		line->pos++;
		tok->type = type;	
		strcpy(tok->name,name);	
		return true;
	}

	return false;
}

bool tr_comment(LINE * line, TOKEN * tok,int val, TOKEN_TYPE type, char * name) {

	if (curch(line) == val) {

		//
		// eat up the rest of the line.
		//
		strncpy(tok->string,curstr(line),256);
		tok->string[strlen(tok->string)-1] = 0;
		setcur(line,0);
		tok->type = type;
		strcpy(tok->name,name);

		return true;
	}

	return false;
}


bool tr_string(LINE * line, TOKEN * tok,int val, TOKEN_TYPE type, char * name) {

	char *p = tok->string;

	if (curch(line) != '"') {
		return false;
	}

	line->pos++;
	while (curch(line) != '"' && curch(line)) {
		*p++ = curch(line);
		line->pos++; 
	}

	strcpy(tok->name,name);
	tok->type = type;

	return true;
}

bool tr_identifier(LINE * line, TOKEN * tok,int val, TOKEN_TYPE type, char * name) {

	bool found = false;
	int spos = line->pos;
	int count = 0;
	char * p;


	//
	// has to start with a letter
	//
	if (!isalpha(curch(line))) {
		return false;
	}

	
	while (isalpha(curch(line)) || isdigit(curch(line))) {
		count++;
		line->pos++;
	}

	if (count) {
		p = tok->string;
		while (spos < line->pos) {
			*p++ = line->string[spos];
			spos++;
		}
		found = true;
		strcpy(tok->name,name);
		tok->type = type;
	}

	return found;
}


bool tr_hexspecifier(LINE * line, TOKEN * tok, int val, TOKEN_TYPE type, char * name) {

	if (curch(line) == val) {
		*tok->string = curch(line);
		line->pos++;
		tok->type = type;
		line->hex = true;
		strcpy(tok->name,name);
		return true;
	}

	return false;
}


bool tr_number(LINE * line, TOKEN *tok, int val, TOKEN_TYPE type, char *name) {


	bool found = false;
	int spos = line->pos;
	int count = 0;

	if (val == true && !line->hex) {
		return false;
	}

	while (isdigit(curch(line)) || (val && isxdigit(curch(line)))) {
		count++;
		line->pos++;
	}

	if (count) {
		strncpy(tok->string,line->string+spos,count);
		found = true;
		line->hex = false;
		tok->type = type;
		strcpy(tok->name,name);
	}

	return found;
}





void init_assembler(ASSEMBLER *a, CPU6502 *c) {

	a->cpu = c;
	a->rulecount = 0;

	addTokenRule(a,tr_singlechrule,ASM_IMMEDIATESPECIFIER,'#',"IMMEDIATESPECIFIER");
	addTokenRule(a,tr_singlechrule,ASM_COLON,':',"COLON");
	addTokenRule(a,tr_singlechrule,ASM_COMMA,',',"COMMA");
	addTokenRule(a,tr_singlechrule,ASM_OPENPAREN,'(',"OPENPAREN");
	addTokenRule(a,tr_singlechrule,ASM_CLOSEPAREN,')',"CLOSEPAREN");
	addTokenRule(a,tr_singlechrule,ASM_EQUAL,'=',"EQUAL");
	addTokenRule(a,tr_singlechrule,ASM_PCDIRECTIVE,'*',"PCDIRECTIVE");
	addTokenRule(a,tr_singlechrule,ASM_PLUS,'+',"PLUS");
	addTokenRule(a,tr_hexspecifier,ASM_HEXSPECIFIER,'$',"HEXSPECIFIER");
	addTokenRule(a,tr_singlechrule,ASM_PRAGMASPECIFIER,'.',"PRAGMASPECIFIER");
	addTokenRule(a,tr_comment,ASM_COMMENT,';',"COMMENT");
	addTokenRule(a,tr_number,ASM_HEXNUMBER,true,"HEXNUMBER");
	addTokenRule(a,tr_number,ASM_DECNUMBER,false,"DECNUMBER");
	addTokenRule(a,tr_identifier,ASM_IDENTIFIER,0,"IDENTIFIER");
	addTokenRule(a,tr_string,ASM_STRING,0,"STRING");
}

void getNextToken(ASSEMBLER * a, LINE * line, TOKEN * tok) {



	int i;	
	memset(tok,0,sizeof(TOKEN));
	tok->type = ASM_NONE; 

	while (curch(line) && isspace(curch(line))) {
		line->pos++;
	}

	if (curch(line) == 0) {
		tok->type = ASM_NONE;
		strcpy(tok->name,"NONE");
		return;
	}

	i = 0;
	bool found = false;

	while (i < a->rulecount && !found) {
		found = a->tokenrules[i].fn(line,tok,
			a->tokenrules[i].val,a->tokenrules[i].type,a->tokenrules[i].name);
		i++;
	} 
}

void getBytesFromStr(char *s, bool hex, byte * high, byte *low, byte *count) {

	char lows[3];
	char his[3];
	int i;
	int c;

	c = 0;

	for (i = 0; i < strlen(s); i++) {
		if (i < 2) {
			his[i] = s[i];
		}
		else if (i < 4) {
			lows[i-2] = s[i];
		}	
		c++;	
	}

	if (c > 2) {
		*high  =  hex ? strtoul(his,NULL,16) : atoi(his);
		*low = hex ? strtoul(lows,NULL,16): atoi(his);
		*count = 2;
	} else {
		*high = 0;
		*low = hex ? strtol(his,NULL,16) : atoi(his);
		*count = 1;
	}
}

void getAddress(ASSEMBLER *a,LINE *line,byte *high, byte *low, byte *count) {

	TOKEN tok;
	int i;
	getNextToken(a,line,&tok);


	if (tok.type == ASM_HEXSPECIFIER) {
		getNextToken(a,line,&tok);
	}

	switch(tok.type) {
		case ASM_HEXNUMBER: 
			getBytesFromStr(tok.string,true,high,low,count);
		break;
		case ASM_DECNUMBER:
			getBytesFromStr(tok.string,false,high,low,count);	
		break;
		case ASM_IDENTIFIER:
			i = dfind(a,tok.string); 
			if (i == -1) {
				fprintf(a->log,"[ASM] Error. label not found. \n");
			}
			else {
				*high = a->dict[i].high;
				*low = a->dict[i].low;
				*count = 2;
			}
		break;
		default: 
			fprintf(a->log,"[ASM] Error: number or label expected.\n %s\n",
			line->string);

			fprintf(a->log,"[ASM] Token %s : %s.\n",
			tok.name,tok.string);
	}
}


void ruleNewIdentifier(ASSEMBLER *a,TOKEN * last,LINE *line) {

	byte high;
	byte low;
	byte count;
	TOKEN tok = {0};
	getNextToken(a,line,&tok);

	switch(tok.type) {
		case ASM_COLON:
			dadd(a,last->string,a->asmh,a->asml);
			ruleNewLine(a,line);
		break;
		case ASM_EQUAL:
			getAddress(a,line,&high,&low,&count);
			dadd(a,last->string,high,low);
		break;
		default:
			fprintf(a->log,"[ASM] invalid new label syntax. Expected ':' or '='.\n");
		break;
	}
}


void rulePCDirective(ASSEMBLER *a, LINE *line) {

	TOKEN tok = {0};
	getNextToken(a,line,&tok);
	byte high;
	byte low;
	byte count;

	if (tok.type != ASM_EQUAL)  {
		fprintf(a->log,"[ASM] Error: missing assignment operator after PC directive.\n %s\n",
			line->string);
	}
	else {
		getAddress(a,line,&high,&low,&count);
		a->asmh = high;
		a->asml = low;
		fprintf(a->log,"$%02X%02X:\t        \t:%s\n",high,low,line->string);
	}
}

void asmbyte(ASSEMBLER *a, byte b) {

	word add = (a->asmh << 8) | a->asml;
	mem_poke(add,b);
	a->asml++;
	if (a->asml == 0) {
		a->asmh++;
	}
}

byte getRelativeOffset(ASSEMBLER * a,byte high,byte low) {

	int nloc = ((int)high << 8) | low;
	int cloc = (((int) a->asmh << 8) | a->asml) + 1;
	int diff = nloc -cloc;
	byte r = 0;
	
	if (diff < -128 || diff > 127) {
		fprintf(a->log,"[ASM] Relative address out of range.\n");
	}
	else {

		if (diff < 0)  {
			r = abs(diff) - 1;
			r = ~r;
		}
		else {
			r = diff;
		}
	}
	return r;
}

void disassembleLine(ASSEMBLER * a,char * buf,word * address) {
	
	int i = 0;
	byte b = mem_peek(*address);
	byte h;
	char * p = buf;
	word reladdress;


	*address += 1;

	for (i = 0; i < 256; i++) {
		if (g_opcodes[i].op == b) {
			break;
		}
	}
	if (i == 256) {
		sprintf(p,"$%02X",b);
	}
	else {
		strcpy(p,g_opcodes[i].name);
		p += strlen(p);
		switch(g_opcodes[i].am) {
			case AM_IMMEDIATE:
				sprintf(p," #$%02X", mem_peek(*address));
				*address += 1;
			break;
			case AM_ZEROPAGE:
				sprintf(p," $%02X",mem_peek(*address));
				*address += 1;
			break;
			case AM_ZEROPAGEX:
				sprintf(p," $%02X,X",mem_peek(*address));
				*address += 1;
			break;
			case AM_ZEROPAGEY:
				sprintf(p," $%02X,Y",mem_peek(*address));
				*address += 1;
			case AM_ABSOLUTE:
				b = mem_peek(*address);
				*address += 1;
				h = mem_peek(*address);
				*address += 1;
				sprintf(p," $%02X%02X",h,b);
			break;
			case AM_ABSOLUTEX:
				b = mem_peek(*address);
				*address += 1;
				h = mem_peek(*address);
				*address += 1;
				sprintf(p," $%02X%02X,X",h,b);
			break;
			case AM_ABSOLUTEY:
				b = mem_peek(*address);
				*address += 1;
				h = mem_peek(*address);
				*address += 1;
				sprintf(p," $%02X%02X,Y",h,b);
			break;
			case AM_INDIRECT:
				b = mem_peek(*address);
				*address += 1;
				h = mem_peek(*address);
				*address += 1;
				sprintf(p," ($%02X%02X)",h,b);
			break;
			case AM_INDEXEDINDIRECT:
				sprintf(p," ($%02X,X)",mem_peek(*address));
				*address += 1;
			break;
			case AM_INDIRECTINDEXED:
				sprintf(p," ($%02X),Y",mem_peek(*address));
				*address += 1;
			break;
			case AM_RELATIVE:
				b = mem_peek(*address);
				*address += 1;
				if (b & N_FLAG) {
					b = ~b + 1;
					reladdress = *address - b;
				}
				else {
					reladdress = *address + b;
				}
				sprintf(p," $%04X",reladdress);
			break;
			default: 
			break;
		}
	}


}


void assemble_instruction(ASSEMBLER *a, LINE * line, char * name, ENUM_AM mode, byte high, byte low, byte count) {

	int i;
	OPCODE *op;
	for (int i = 0; i < 256; i++) {

		op = &g_opcodes[i];
		if (strcmp(name,op->name) == 0 && (op->am == mode || op->am == AM_RELATIVE)) {
			fprintf(a->log,"$%02X%02X:\t",a->asmh,a->asml);
			asmbyte(a,op->op);
			fprintf(a->log,"%02X",op->op);

			if (op->am == AM_RELATIVE) {
				low = getRelativeOffset(a,high,low);
				asmbyte(a,low);
				fprintf(a->log," %02X   ",low);
			}
			else {
				switch(count) {

					case 2: 
						asmbyte(a,low);
						asmbyte(a,high);
						fprintf(a->log," %02X %02X",low,high);
						break;
					case 1:
						asmbyte(a,low);
						fprintf(a->log," %02X   ",low);
						break;
					default:
						fprintf(a->log,"       ");
					break; 

				}
			}
			fprintf(a->log,"\t:%s\n",line->string);
			return;
		}
	}
	fprintf(a->log,"[ASM] Invalid instruction or mode.  %s %d\n",
		name,mode);
}


void ruleHandleDB(ASSEMBLER *a, LINE *line) {
	
	TOKEN tok = {0};
	byte high,low,count;

	fprintf(a->log,"$%02X%02X:\t",a->asmh,a->asml);

	getNextToken(a,line,&tok);

	while (tok.type != ASM_NONE) {

		switch(tok.type) {
			case ASM_DECNUMBER: 
				getBytesFromStr(tok.string,false,&high,&low,&count);
				asmbyte(a,low);
				fprintf(a->log,"%02X ",low);
			break;
			case ASM_HEXNUMBER:
				getBytesFromStr(tok.string,true,&high,&low,&count);
				asmbyte(a,low);
				fprintf(a->log,"%02X ",low);
			break;
			default:
			break;
		}
		getNextToken(a,line,&tok);
	}
	fprintf(a->log,"\t:%s\n",line->string);
}

void ruleHandleString(ASSEMBLER *a, LINE *line) {
	TOKEN tok = {0};
	int i;
	getNextToken(a,line,&tok);
	if (tok.type != ASM_STRING) {
		fprintf(a->log,"[ASM] Expected string.\n");
	}

	fprintf(a->log,"$%02X%02X:\t%02X ",a->asmh,a->asml,(byte) strlen(tok.string));
	asmbyte(a,strlen(tok.string));
	for (i = 0; i < strlen(tok.string);i++) {
		fprintf(a->log,"%02X ",tok.string[i]);
		asmbyte(a,tok.string[i]);
	}
	fprintf(a->log,"\t:.STRING \"%s\"\n",tok.string);
}

void ruleHandlePragma(ASSEMBLER *a, TOKEN * last, LINE *line) {

	TOKEN tok = {0};
	getNextToken(a,line,&tok);
	if (tok.type != ASM_IDENTIFIER) {
		fprintf(a->log,"[ASM] Expected pragma directive.\n");
	}

	if (!strcmp(tok.string,"STRING")) {
		ruleHandleString(a,line);
	}
	else if (!strcmp(tok.string,"DB")) {
		ruleHandleDB(a,line);
	}

}

void ruleOpCode(ASSEMBLER *a, TOKEN * last, LINE *line) {

	TOKEN tok = {0};
	ENUM_AM am = AM_IMPLICIT;
	byte high = 0;
	byte low = 0;
	byte count = 0;
	bool immediate = false;
	bool xreg = false;
	bool yreg = false;
	bool indirect = false;
	bool done = false;
	int i;

	getNextToken(a,line,&tok);
	while (!done) {

			
			switch(tok.type) {
			case ASM_IMMEDIATESPECIFIER: immediate = true; break;
			case ASM_NONE: done = true; break;
			case ASM_OPENPAREN: indirect = true; break; 
			case ASM_IDENTIFIER:
				if (!strcmp(tok.string,"X")) {
					xreg = true;
				}
				else if (!strcmp(tok.string,"Y")) {
					yreg = true;
				}
				else if ((i = dfind(a,tok.string)) != -1) {
					high = a->dict[i].high;
					low = a->dict[i].low;
					count = 1 + (high > 0);
				}
				else {
					fprintf(a->log,"[ASM] unexpected identifier.\n");
				}
			break;

			case ASM_DECNUMBER: 
				getBytesFromStr(tok.string,false,&high,&low,&count);
				break;
			case ASM_HEXNUMBER:
				getBytesFromStr(tok.string,true,&high,&low,&count);
				break;
			
			case ASM_HEXSPECIFIER: break; // ignore
			case ASM_COMMA: break; // ignore
			case ASM_CLOSEPAREN: break; // ignore
			case ASM_COMMENT: break; // ignore
			
			default: break;
		}

		
		getNextToken(a,line,&tok);
		
	}

	if (immediate) {am = AM_IMMEDIATE;}
	else if (indirect && count == 2) {am = AM_INDIRECT;}
	else if (indirect && xreg) {am = AM_INDEXEDINDIRECT;}
	else if (indirect && yreg) {am = AM_INDIRECTINDEXED;}
	else if (count==2  && xreg) {am = AM_ABSOLUTEX;}
	else if (count==2 && yreg) {am = AM_ABSOLUTEY;}
	else if (count==2) {am = AM_ABSOLUTE;}
	else if (count==1 && xreg) {am = AM_ZEROPAGEX;}
	else if (count==1 && yreg) {am = AM_ZEROPAGEY;}
	else if (count==1)  {am = AM_ZEROPAGE;}

	assemble_instruction(a, line, last->string, am, high, low, count);



}

void ruleNewLine(ASSEMBLER * a, LINE * line) {

	TOKEN tok = {0};
	getNextToken(a,line,&tok);
	
	switch(tok.type) {
		case ASM_COMMENT: 
		break;
		case ASM_PCDIRECTIVE:
			rulePCDirective(a,line);
		break;
		case ASM_IDENTIFIER:
			if (isopcode(&tok)) {
				ruleOpCode(a,&tok,line);
			}
			else {
				ruleNewIdentifier(a,&tok,line);
			}
		break;
		case ASM_PRAGMASPECIFIER:
			ruleHandlePragma(a,&tok,line);
		break;
		case ASM_NONE:
		break;
		default: 
			fprintf(a->log,"[ASM] invalid syntax. %s\n",tok.name);
		break;
	}
}


void loadBinFile(ASSEMBLER *a, char * file, byte high, byte low) {

	FILE * f;
	char logbuf[256];
	int len;
	int i;
	byte b; 
	int count = 0;

	f = fopen(file,"rb");
	
	a->asmh = high;
	a->asml = low;

	sprintf(logbuf,"%s.log",file);

	a->log = fopen(logbuf,"w+");
	fprintf(a->log,"[ASM] binary file %s\n",file);

	if (f) {


		fseek(f, 0, SEEK_END);          
    	len = ftell(f);            
    	rewind(f);   

		fprintf(a->log,"[ASM] file is %d bytes long.\n",len);
		for (i = 0; i < len; i++) {
			fread(&b,1,1,f);
			asmbyte(a,b);
			count++;
		}

		fclose(f);
		fprintf(a->log,"[ASM] added %d bytes at $%02X%02X\n",count,high,low);

	}

	fclose(a->log);

}

void assembleFile(ASSEMBLER *a,char * file) {

	FILE * f;
	char * p;
	LINE line;
	char logbuf[256];
	sprintf(logbuf,"%s.log",file);

	a->log = fopen(logbuf,"w+");

	f = fopen (file,"r");
	if (f) {
		a->dlast = 0;
		fprintf(a->log,"Assembling %s...\n",file);
		do {
			memset(&line,0,sizeof(LINE));
			p = fgets(line.string,256,f);
			if (line.string[strlen(line.string) - 1] == '\n') {
				line.string[strlen(line.string) - 1] = 0;
			}
			if (p) {
				line.pos = 0;
				ruleNewLine(a,&line);
			}
		} while (p);

		fclose(f);
	}
	fclose (a->log);
}




