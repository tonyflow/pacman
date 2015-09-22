#include	<stdio.h>
#include	<string.h>
#include	<curses.h>
#include	<signal.h>
#include	<unistd.h>
#include	<errno.h>
char buf[128];

void move_msg(int signum)
{
	signal(SIGALRM, SIG_IGN);	/* reset, just in case	*/
	signal(SIGINT, move_msg);	/* reset, just in case	*/
        strcpy(buf, "Received Alarm\n");
        write(1, buf, strlen(buf));
}

int main() {
        int delay, ndelay, c;
        char ch;

	signal(SIGALRM, SIG_IGN);	/* reset, just in case	*/
	signal(SIGINT, move_msg);	/* reset, just in case	*/
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

