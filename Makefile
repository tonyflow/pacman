#This is the makefile for the pacman game program exercise
#Type make to compile the source files included


clean:
	rm -f pacmam *.o

pacman:pacman.c
    gcc -o pacman pacman.c -lcurses
