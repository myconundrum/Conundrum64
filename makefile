CC = gcc
LFLAGS = ncurses
ALL: 
	$(CC) *.c -o emu -l$(LFLAGS)

