#include <stdio.h>
#include <stdlib.h>
#include <pty.h>
#include <unistd.h>
#include <string.h>

#include "console.h"

static char pty_name [256];
static int master;

int run_console (void)
{
	int slave;
	int rc;
    rc = openpty (&master, &slave, pty_name, 0,0);
	printf ("pty name: %s\n", pty_name);
	close (slave);
	return master;
}

void launch_minicom (void)
{
	if (fork() == 0)
	{
		printf( "Exec /usr/X11R6/bin/xterm -e /usr/bin/minicom -o -p %s\n", pty_name);
		execl ( "/usr/X11R6/bin/xterm", "xterm", "-e", "/usr/bin/minicom", "-o", "-p", pty_name, 0);
		perror ("run minicom");
		exit (1);
	}
	usleep (500000);
	const char * msg = "Close minicom using CTRL-A Q\r\n\r\n";
	write (master, msg, strlen(msg));
}

