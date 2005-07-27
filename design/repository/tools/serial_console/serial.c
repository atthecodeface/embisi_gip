/*a Includes
 */
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h> /* for errno */

#include <sys/ioctl.h> /* For ioctl() */

#include <sys/types.h> /* For FDs */
#include <sys/stat.h>
#include <fcntl.h>

#include <linux/serial.h> /* For struct serial_struct */
#include <asm/ioctls.h> /* For TIOCGSERIAL, TCGETA, TCSETA */

#include <sys/select.h> /* For select */

#include <sys/time.h> /* For select timeout */

//#include <asm/termios.h> /* For struct termio  - not needed with sys/ioctl.h */
#include <asm/termbits.h> /* For CBAUD, B9600, B19200, B38400, B57600, B115200 */

#include <string.h>

#include "serial_console.h"
#include "serial.h"

/*a Defines
 */
// CHAR_TIME is time for 10 bits at the baud rate in us, so 1e6/(baud/10)
//#define CHAR_TIME (1000*1000/120)
#define CHAR_TIME (1000*1000/960)

/*a Serial handling routines
 */
/*f open_serial
 */
extern int open_serial( const char *filename, int baud, int verbose )
{
     int fd;
     struct serial_struct ser_data;
     struct termio termio_data;
//     fd = open(filename, O_RDWR | O_NONBLOCK );
     fd = open(filename, O_RDWR ); // Cannot use NONBLOCK, as then read is always allowed, and we need to use select!
     if (fd<0) 
     {
          fprintf(stderr,"Failed to open %s\n", filename);
          return -1;
     }

    ioctl( fd, TCGETA, &termio_data );
    ioctl( fd, TIOCGSERIAL, &ser_data );

    if (verbose)
    {
         printf( "c_iflag %08x\n", termio_data.c_iflag );
         printf( "c_oflag %08x\n", termio_data.c_oflag );
         printf( "c_cflag %08x\n", termio_data.c_cflag );
         printf( "c_lflag %08x\n", termio_data.c_lflag );
         printf( "c_line %08x\n", termio_data.c_line );
    }

    termio_data.c_cflag &= ~CBAUD;
    switch (baud)
    {
    case 1200: termio_data.c_cflag |= B1200; break;
    case 2400: termio_data.c_cflag |= B2400; break;
    case 4800: termio_data.c_cflag |= B4800; break;
    case 9600: termio_data.c_cflag |= B9600; break;
    case 19200: termio_data.c_cflag |= B19200; break;
    case 38400: termio_data.c_cflag |= B38400; break;
    case 57600: termio_data.c_cflag |= B57600; break;
    case 115200: termio_data.c_cflag |= B115200; break;
    default:
        int div;
        fprintf(stderr,"Warning: using custom divisor\n");
        div = ser_data.baud_base/baud;
        ser_data.custom_divisor = (div>=1)?div:1;
        ser_data.flags &= ~ASYNC_SPD_MASK;
        ser_data.flags |= ASYNC_SPD_CUST;
        ioctl( fd, TIOCSSERIAL, &ser_data );
        termio_data.c_cflag |= B38400;
        break;
    }
    ioctl( fd, TCSETA, &termio_data );

    ioctl( fd, TCGETA, &termio_data );
    if (verbose)
    {
         printf( "c_iflag %08x\n", termio_data.c_iflag );
         printf( "c_oflag %08x\n", termio_data.c_oflag );
         printf( "c_cflag %08x\n", termio_data.c_cflag );
         printf( "c_lflag %08x\n", termio_data.c_lflag );
         printf( "c_line %08x\n", termio_data.c_line );
    }

    ioctl( fd, TIOCGSERIAL, &ser_data );
    if (verbose)
    {
         printf( "type %d\n", ser_data.type );
         printf( "line %d\n", ser_data.line );
         printf( "port %d\n", ser_data.port );
         printf( "irq %d\n", ser_data.irq );
         printf( "flags %d\n", ser_data.flags );
         printf( "xmit_fifo_size %d\n", ser_data.xmit_fifo_size );
         printf( "custom_divisor %d\n", ser_data.custom_divisor );
         printf( "baud_base %d\n", ser_data.baud_base );
         printf( "close_delay %d\n", ser_data.close_delay );
         printf( "io_type %d\n", ser_data.io_type );
         printf( "hub6 %d\n", ser_data.hub6 );
         printf( "closing_wait %d\n", ser_data.closing_wait );
         printf( "closing_wait2 %d\n", ser_data.closing_wait2 );
         printf( "iomem_base %p\n", ser_data.iomem_base );
         printf( "iomem_reg_shift %d\n", ser_data.iomem_reg_shift );
         printf( "port_high %d\n", ser_data.port_high );
    }

    return fd;
}

/*f poll_fd
  poll an fd
  if timeout, return 0
  if exception, return -1
  if read, return 2
  if write, return 1
 */
extern int poll_fd( int fd, int read, int write, int timeout_s, int timeout_ms )
{
     int result;
     struct timeval timeout;
     fd_set readfds, writefds, exceptfds;

     FD_ZERO( &readfds );
     if (read) FD_SET( fd, &readfds );
     FD_ZERO( &writefds );
     if (write) FD_SET( fd, &writefds );
     FD_ZERO( &exceptfds );
     FD_SET( fd, &exceptfds );
     timeout.tv_sec = timeout_s;
     timeout.tv_usec = 1000*timeout_ms;
     if (select( fd+1, &readfds, &writefds, &exceptfds, &timeout )<=0)
     {
          return 0;
     }
     if (FD_ISSET( fd, &exceptfds ) )
     {
          return -1;
     }
     result = 0;
     if (FD_ISSET( fd, &readfds ) ) result|=2;
     if (FD_ISSET( fd, &writefds ) ) result|=1;
//     printf( "Flags %08x %08x %08x\n",
//             ((int *)&readfds)[0],
//             ((int *)&writefds)[0],
//             ((int *)&exceptfds)[0] );
     return result;
}

/*f serial_putchar
 */
extern void serial_putchar( int ch )
{
    fprintf(serlog,"!%02x",ch&0xff);
    write( serial_fd, &ch, 1 );
    usleep(CHAR_TIME);
}

