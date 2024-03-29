
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
MODULE: main.c

WORK ITEMS:

KNOWN BUGS:

*/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include "ini.h"
#include "ux.h"

#include "emu.h"

#include <time.h>


EMU_CONFIGURATION g_config = {0};
char g_nameString[255];
char * emu_getname(void) {return g_nameString;}


EMU_CONFIGURATION * emu_getconfig(void) {
	return &g_config;
}


static int config_handler(
	void* configdata, const char* section, const char* name, const char* value) {

#define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0

	EMU_CONFIGURATION * c = (EMU_CONFIGURATION *) configdata;

	if (MATCH("roms", "kernal")) {
        
        c->kernalpath = strdup(value);
        DEBUG_PRINT("%-40s [%s]\n","\tKernal rom Path:",c->kernalpath);

    } else if (MATCH("roms", "char")) {
        
        c->charpath = strdup(value);
        DEBUG_PRINT("%-40s [%s]\n","\tChar rom path:",c->charpath);

    } else if (MATCH("roms", "basic")) {
        
        c->basicpath = strdup(value);
        DEBUG_PRINT("%-40s [%s]\n","\tBasic rom path:",c->basicpath);
    
    } else if (MATCH("bin", "load")) {
    
        c->binload = strdup(value);
        DEBUG_PRINT("%-40s [%s]\n","\tBinary load:",c->binload);
    
    }  else if (MATCH("bin", "loadcart")) {
    
        c->cartload = strdup(value);
        DEBUG_PRINT("%-40s [%s]\n","\tLoad cartridge:",c->cartload);
    
    }  else if (MATCH("debug", "breakpoint")) {
    
        c->breakpoint = strtoul(value,NULL,16);
        DEBUG_PRINT("%-40s [%04X]\n","\tInitial breakpoint:",c->breakpoint);
    
    }  else if (MATCH("system", "region")) {
   
        c->region = strdup(value);
        DEBUG_PRINT("%-40s [%s]\n","\tRegion:",c->region);

    } else if (MATCH("system", "font")) {
   
        c->font = strdup(value);
        DEBUG_PRINT("%-40s [%s]\n","\tFont:",c->font);
   
    } else if (MATCH("disk", "disk")) {
   
        c->disk = strdup(value);
        DEBUG_PRINT("%-40s [%s]\n","\tDisk to insert:",c->disk);
   
    } else if (MATCH("disk", "program")) {
   
        c->program = strdup(value);
        DEBUG_PRINT("%-40s [%s]\n","\tProgram to load:",c->program);
   
    } else {
        return 0;  
    }

    return 1;
}




int main(int argc, char**argv) {

    sprintf(g_nameString,"%s (version %d.%d)",EMU_NAME,EMU_VERSION_MAJOR,EMU_VERSION_MINOR);
    time_t start = time(NULL);
    char * cfg_path = "conundrum64.ini";
	DEBUG_INIT("c64.log");
    DEBUG_PRINT("Local time: %s",asctime(localtime(&start)));

    if (argc > 1) {
        cfg_path = argv[1];
    }
		
    DEBUG_PRINT("Reading configuration file (%s).\n", cfg_path);
    if (ini_parse(cfg_path, config_handler, &g_config) < 0) {
        DEBUG_PRINT("Failed to load initialization file 'conundrum64.ini'\n");
    }

	c64_init();
	ux_init();
	ux_startemulator();
	
	do {
		ux_update();
        if (ux_running()) {
            c64_update();
        }

	} while (!ux_done());

	c64_destroy();
	ux_destroy();

	DEBUG_DESTROY();
	return 0;
}
