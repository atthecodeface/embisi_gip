/*a Notes
stty /dev/ttyS0 raw
stty /dev/ttyS0 -echo
stty -F /dev/ttyS0 ispeed 2400 ospeed 2400

 */

/*a Includes
 */
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include <sys/types.h> /* For FDs */
#include <sys/stat.h>
#include <fcntl.h>

#include <curses.h>

#include "serial_console.h"
#include "serial.h"
#include "flash.h"

/*a Flash download functions
 */
/*f flash_download_wait
 */
extern int flash_download_wait( void )
{
     char buffer[16];
     int i;

     i = 0;
     while (i<(int)sizeof(buffer))
     {
          if ((poll_fd(serial_fd, 1, 0, 4, 0)&2)==0)
          {
               return 0;
          }
          read( serial_fd, buffer+i, 1 );
          fprintf(serlog,"#%02x",buffer[i]&0xff);
          if (buffer[i]<32)
          {
               buffer[i] = 0;
               if (i>=2)
               {
                    if (!strcmp(buffer+i-2, "dc"))
                    {
                         return 1;
                    }
                    if (!strcmp(buffer+i-2, "de"))
                    {
                         return 0;
                    }
               }
               i = 0;
          }
          else
          {
               i++;
          }
     }
     return 0;
}

/*f flash_download_command
 */
extern void flash_download_command( char cmd, unsigned char *data, int length )
{
    int i, j;
    int c;
    unsigned char buffer[256+4]; // this is the limit in the boot ROM in the FPGA
    unsigned int csum;

    sprintf( (char *)buffer, "fdc:'%c':%d:", cmd, length );
    waddstr(command_window, (char *)buffer );
    for (i=0; (i<length) && (i<64); i++)
    {
        sprintf( (char *)buffer, "%02x ", data[i] );
        waddstr(command_window, (char *)buffer );
    }
    wechochar( command_window, '\n' );
    wrefresh(command_window);
    csum = cmd;
    for (i=0; i<length; i++)
    {
        csum += data[i];
    }
    csum = (-csum)&0xff;
    j = 0;
#define CBYTE( buffer, j, x ) { int y; y = x^0x80; if ((y<32) || (y>=0xf0)) { buffer[j++]=0xf0|((x>>4)&0xf); buffer[j++]=0xf0|(x&0xf); } else { buffer[j++]=y; } }
    CBYTE( buffer, j, csum );
    CBYTE( buffer, j, cmd );
    for (i=0; (i<length) && (j<(int)sizeof(buffer)-3); i++)
    {
        CBYTE( buffer, j, data[i] );
    }
    buffer[j++] = 10;
    if (j>=(int)sizeof(buffer)-3)
    {
        waddstr( command_window, "Flash download command too long!!! Bug!\n" );
        wrefresh( command_window );
        return;
    }
    for (i=0; i<j; i++)
    {
        if (poll_fd(serial_fd, 0, 1, 2, 0)==1)
        {
            serial_putchar( buffer[i] );
        }
        else
        {
            return;
        }
    }
}

