#ifndef EMU_H
#define EMU_H


#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <ncurses.h>
#include "cpu.h"
#include "c64kbd.h"
#include "cia.h"
#include "mem.h"
#include "asm.h"
#include "ux.h"
#include "stdint.h"
#include "sysclock.h"
#include "c64.h"

#define DEBUG 1

#if defined(DEBUG) && DEBUG > 0

FILE * g_debug;

 #define DEBUG_INIT(log) g_debug = fopen(log,"w+")
 #define DEBUG_PRINT(fmt, args...) fprintf(g_debug, "DEBUG: %s:%d:%s(): " fmt, \
    __FILE__, __LINE__, __func__, ##args)
 #define DEBUG_DESTROY() fclose(g_debug)
#else
 #define DEBUG_INIT(log)
 #define DEBUG_PRINT(fmt, args...) /* Don't do anything in release builds */
 #define DEBUG_DESTROY()
#endif


#endif
