
# If you don't have gcc, try cc instead, if it is an ANSI compiler.

CC=gcc

wordplay : wordplay.c
	$(CC) -O -o wordplay wordplay.c

