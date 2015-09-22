#
# Makefile for Chapter 07
#
# Type  make    to compile all the programs
# in the chapter 
#
all: bounce1d bounce2d bounce_aio bounce_async hello1 	\
	hello2 hello3 hello4 hello5 sigactdemo sigdemo3 \
	sleep1 ticker_demo mytest mytest2 mytest3

clean:
	rm -f bounce1d bounce2d bounce_aio bounce_async hello1 	\
		hello2 hello3 hello4 hello5 sigactdemo sigdemo3 \
		sleep1 ticker_demo mytest mytest2 mytest3 *.o


bounce1d: bounce1d.c set_ticker.c 
	gcc -o bounce1d bounce1d.c set_ticker.c -lcurses

bounce2d: bounce2d.c set_ticker.c 
	gcc -o bounce2d bounce2d.c set_ticker.c -lcurses

bounce_aio: bounce_aio.c set_ticker.c 
	gcc -o bounce_aio bounce_aio.c set_ticker.c -lrt  -lcurses

bounce_async: bounce_async.c set_ticker.c 
	gcc -o bounce_async bounce_async.c set_ticker.c -lcurses

hello1: hello1.c 
	gcc -o hello1 hello1.c -lcurses

hello2: hello2.c 
	gcc -o hello2 hello2.c -lcurses

hello3: hello3.c 
	gcc -o hello3 hello3.c -lcurses

hello4: hello4.c 
	gcc -o hello4 hello4.c -lcurses

hello5: hello5.c 
	gcc -o hello5 hello5.c -lcurses

sigactdemo: sigactdemo.c
	gcc -o sigactdemo sigactdemo.c

sigdemo3: sigdemo3.c
	gcc -o sigdemo3 sigdemo3.c

sleep1: sleep1.c 
	gcc -o sleep1 sleep1.c 

ticker_demo: ticker_demo.c 
	gcc -o ticker_demo ticker_demo.c 

mytest: mytest.c set_ticker.c 
	gcc -o mytest mytest.c set_ticker.c -lcurses

mytest2: mytest2.c set_ticker.c 
	gcc -o mytest2 mytest2.c set_ticker.c -lcurses

mytest3: mytest3.c set_ticker.c 
	gcc -o mytest3 mytest3.c set_ticker.c -lcurses

