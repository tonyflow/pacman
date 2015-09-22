#include	<stdio.h>
#include	<string.h>
#include	<curses.h>
#include	<signal.h>
#include	<unistd.h>
#include	<errno.h>
char buf[128];

struct sigaction act1, act2;

void move_msg(int signum)
{
        act1.sa_handler = SIG_IGN;
        act2.sa_handler = move_msg;
	sigaction(SIGALRM, &act1, NULL);	/* reset, just in case	*/
	sigaction(SIGINT, &act2, NULL);	        /* reset, just in case	*/
        strcpy(buf, "Received Alarm\n");
        write(1, buf, strlen(buf));
}

int main() {
        int delay, ndelay, c;
        char ch;

        act1.sa_handler = SIG_IGN;
        act2.sa_handler = move_msg;
	sigaction(SIGALRM, &act1, NULL);	/* reset, just in case	*/
	sigaction(SIGINT, &act2, NULL);	        /* reset, just in case	*/
	set_ticker( 500 );

	while(1) {
		ndelay = 0;
		c = read(0, &ch, sizeof(char));
        	sprintf(buf, "Read char = %c and errno = %d and EINTR = %d\n", 
                        ch, errno, EINTR); 
        	write(1, buf, strlen(buf));
		if ( c == 'Q' ) break;
		if ( ndelay > 0 )
			set_ticker( delay = ndelay );
	}
	return 0;
}

