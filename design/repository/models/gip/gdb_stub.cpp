/*a Copyright Gavin J Stark and John Croft, 2003
    */

/*a Includes
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

#include "gdb_stub.h"
#include "c_gip_pipeline_single.h"
#include "c_memory_model.h"
#include "c_mmio_model.h"

/*a Types
*/
/*t t_gdb_stub
 */
typedef struct t_gdb_stub
{
    int enabled;   
    int client_socket;
    int public_socket;
    c_gip_pipeline_single *gip;
    c_memory_model *memory;
    int status;
} t_gdb_stub;

/*a Statics
*/
/*v hex
*/
const char *hex = "0123456789abcdef";

/*v stub
*/
static t_gdb_stub stub;

/*a GDB communications functions
*/
/*f gdb_send - send a message on the client socket with checksum, meeting the gdb format
 */
static void gdb_send (const char * data)
{
    int len;
    len = strlen(data);
    send( stub.client_socket, "$", 1, 0);
    send( stub.client_socket, data, len, 0);
    send (stub.client_socket, "#", 1, 0);
    unsigned char check = 0;
    for (int i = 0; i < len; i++)
        check += data[i];
    send (stub.client_socket, &(hex[check >> 4]), 1, 0);
    send (stub.client_socket, &(hex[check & 15]), 1, 0);
}

/*f gdb_getch - get a character from the GDB connection socket
 */
static int gdb_getch (void)
{
    char c[1];
    int n = recv (stub.client_socket, c, 1, 0);
    if (n <= 0) return -1;
    //  printf ("%2.2x (%c)\n", c[0], c[0]);
    return c[0];
}

/*f parse_hex_char - return a number from 0 through 15 for 0-9a-fA-F
 */
static int parse_hex_char (char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1;
}

/*f gdb_receive - read a packet (blocking until complete) from the stub.client_stream, and check it conforms
 */
static int gdb_receive(char * packet)
{
    for (;;)
    {
        int pos = 0;
    
        for (;;)
        {
            int c = gdb_getch();
            if (c == '$') break;
            if (c < 0)
            {
                printf ("Connection closed\n");
                return 0;
            }
        }

        unsigned char csum = 0;

        for (;;)
        {
            int c = gdb_getch();
            if (c == '#') break;
            if (c < 0)
            {
                printf ("Connection closed\n");
                return 0;
            }
            packet[pos++] = c;
            csum += c;
        }

        unsigned char xcsum;
        xcsum = parse_hex_char (gdb_getch());
        xcsum <<= 4;
        xcsum |= parse_hex_char (gdb_getch());

        if (xcsum == csum)
        {
            //          printf ("Csum matches\n");
            packet[pos] = 0;
            send (stub.client_socket, "+", 1, 0);
            return 1;
        }
        printf ("Csum failed\n");
    }
}

/*f gdb_poll_socket - poll client and public sockets for read access, with no timeout
  Return 0 if no data ready, no accept ready
  Return 1 if no data ready, accept ready
  Return 2 if data ready, no accept ready
  (return 3... unlikely...)
 */
static int gdb_poll_socket( void )
{
    fd_set socks;
    struct timeval timeout;
    int readsocks;
    int result;

    FD_ZERO(&socks);
    if (stub.client_socket!=0)
    {
        FD_SET(stub.client_socket,&socks);
    }
    if (stub.public_socket!=0)
    {
        FD_SET(stub.public_socket,&socks);
    }
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
    readsocks = select( FD_SETSIZE, &socks, (fd_set *) NULL, (fd_set *) NULL, &timeout);
    result = 0;

    if (readsocks>00)
    {
        if ((stub.public_socket!=0) && (FD_ISSET(stub.public_socket,&socks)))
        {
            result |= 1;
        }
        if ((stub.client_socket!=0) && (FD_ISSET(stub.client_socket,&socks)))
        {
            result |= 2;
        }
    }
    return result;
}

/*f handle_packet - handle a received GDB packet
 */
static int handle_packet( char * rx_packet )
{
    int addr, len, value, i, bp_type;
    const char * ptr;
    char * optr;
    char response_buf [16*8+16*12+2*4+1];
           
//    printf("gdb:handle_packet '%s'\n",rx_packet);
    switch (rx_packet[0])
    {
    case 'c':
        return 1;
        
    case '?':
        char msg[4];
        msg[0] = 'S';
        msg[1] = '0';
        msg[2] = hex[stub.status];
        msg[3] = 0;
        gdb_send (msg);
        break;

    case 'H':
        gdb_send ("OK");
        break;
            
    case 'g':
	optr = response_buf;
        for (i = 0; i < 16; i++)
        {
            unsigned int reg = stub.gip->get_register (i);
            //          printf ("Read register as %8.8x\n", reg);
            *(optr++) = hex[(reg >> 4) & 0xf];
            *(optr++) = hex[(reg >> 0) & 0xf];
            *(optr++) = hex[(reg >> 12) & 0xf];
            *(optr++) = hex[(reg >> 8) & 0xf];
            *(optr++) = hex[(reg >> 20) & 0xf];
            *(optr++) = hex[(reg >> 16) & 0xf];
            *(optr++) = hex[(reg >> 28) & 0xf];
            *(optr++) = hex[(reg >> 24) & 0xf];
        }
	for (i = 0; i < 16; i++)
	{
		int j;
		for (j = 0; j < 12; j++)
			*(optr++) = '0';
	}
	for (i = 0; i < 8; i++)
		*(optr++) = '0';
	{
		int raw_flags = stub.gip->get_flags();
		unsigned int reg;
/*
		unsigned int reg = 0;
		if (raw_flags & am_flag_n) reg |= 1 << 31;
		if (raw_flags & am_flag_z) reg |= 1 << 30;
		if (raw_flags & am_flag_c) reg |= 1 << 29;
		if (raw_flags & am_flag_v) reg |= 1 << 28;
*/
		reg = raw_flags;
	        *(optr++) = hex[(reg >> 4) & 0xf];
		*(optr++) = hex[(reg >> 0) & 0xf];
		*(optr++) = hex[(reg >> 12) & 0xf];
		*(optr++) = hex[(reg >> 8) & 0xf];
		*(optr++) = hex[(reg >> 20) & 0xf];
		*(optr++) = hex[(reg >> 16) & 0xf];
		*(optr++) = hex[(reg >> 28) & 0xf];
		*(optr++) = hex[(reg >> 24) & 0xf];
	}
	
        *optr = 0;
        gdb_send (response_buf);
        break;

    case 'm':
        addr = 0;
        ptr = rx_packet+1;
        while (*ptr && *ptr != ',')
            addr = (addr << 4) | parse_hex_char (*(ptr++));
        if (*ptr == ',')
        {
            *ptr++;
            len = 0;
            while (*ptr && *ptr != ',')
                len = (len << 4) | parse_hex_char (*(ptr++));
            //          printf ("addr = %x, len = %x\n", addr, len);
            if (len > 16) len = 16;
                
            ptr = response_buf;
            for (i = 0; i < len; i++)
            {
                unsigned int word = stub.memory->read_memory((addr+i) & ~3);
                int byte_in_word = (addr+i) & 3;
                unsigned char byte = (word >> (byte_in_word * 8));
                response_buf[i*2+0] = hex[(byte >> 4) & 0xf];
                response_buf[i*2+1] = hex[(byte >> 0) & 0xf];
            }
            response_buf[len*2] = 0;
            gdb_send (response_buf);
        }
        break;
            
    case 'X':
        addr = 0;
        ptr = rx_packet+1;
        while (*ptr && *ptr != ',')
            addr = (addr << 4) | parse_hex_char (*(ptr++));
        if (*ptr == ',')
        {
            *ptr++;
            value = 0;
            while (*ptr && *ptr != ':')
                value = (value << 4) | parse_hex_char (*(ptr++));
                      printf ("gdb:write:addr = %x, value = %x\n", addr, value);
	    stub.memory->write_memory(addr & ~3, value, 1<<(addr&3));
	    gdb_send ("OK");
        }
        break;
            
    case 'v':
        if (!strcmp (rx_packet, "vCont?"))
        {
            gdb_send ("vCont");
        }
        break;
                
    case 'Z':
    case 'z':
        bp_type = parse_hex_char (rx_packet[1]);
        addr = 0;
        ptr = rx_packet+3;
        while (*ptr && *ptr != ',')
            addr = (addr << 4) | parse_hex_char (*(ptr++));
        if (rx_packet[0] == 'Z')
        {
            stub.gip->set_breakpoint(addr);
        }
        else
        {
            stub.gip->unset_breakpoint(addr);
        }
        gdb_send ("OK");
        break;

    case 'q':
	if (!strcmp (rx_packet, "qSymbol::"))
		gdb_send ("OK");
	break;
        
    default:
        printf ("got unknown command '%s'\n", rx_packet);
        break;
    }
    return 0;
}

/*f gdb_ensure_connected
 */
static void gdb_ensure_connected( void )
{
    if (stub.client_socket == 0)
    {
        struct sockaddr_in caddr;
        unsigned int caddr_len = sizeof(caddr);
    
        printf ("Waiting for connection from gdb\n");   
        stub.client_socket = accept (stub.public_socket, (struct sockaddr *)&caddr, &caddr_len);
        printf ("Got connection\n");

        send (stub.client_socket, "||||", 4, 0);
    }
}

/*f gdb_ensured_receive
 */
static void gdb_ensured_receive( char *rx_packet )
{
    while (!gdb_receive(rx_packet))
    {
        if (stub.client_socket) close(stub.client_socket);
        stub.client_socket = 0;
        gdb_ensure_connected();
    }
}

/*a External functions
 */
/*f gdb_stub_disable
*/
void gdb_stub_disable( void )
{
    stub.enabled = 0;
}

/*f gdb_stub_init
  Open the public server socket for a gdb client to connect to
  Record the GIP model we are connected to, so we can get data
  Record the memory model so we can set watchpoints and read memory
*/
void gdb_stub_init( c_gip_pipeline_single *gip, c_memory_model *memory, c_mmio_model *mmio )
{
    stub.enabled = 1;
    stub.gip = gip;
    stub.memory = memory;
    stub.public_socket = socket (AF_INET, SOCK_STREAM, PF_UNSPEC);
    struct sockaddr_in addr;
    memset (&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons (1234);
    int tru = 1;
    int rc;
    rc = setsockopt (stub.public_socket, SOL_TCP, TCP_NODELAY, &tru, sizeof(tru));
    printf ("setsockopt rc = %d\n",rc);
    rc = setsockopt (stub.public_socket, SOL_SOCKET, SO_REUSEADDR, &tru, sizeof(tru));
    printf ("setsockopt rc = %d\n",rc);
    rc = bind (stub.public_socket, (struct sockaddr *)&addr, sizeof(addr));
    printf ("bind rc = %d\n",rc);
    listen (stub.public_socket, 5);
    gdb_trap(0);
}

/*f gdb_poll
  If the arg is 0, then just poll for messages
  Else we are stopped, so send a signal of our own and wait for response
  Return 0 for continue until breakpoint, or a positive instruction count
*/
int gdb_poll( int reason )
{
    if (stub.enabled==0)
        return 0;
    if (reason==0)
    {
        gdb_ensure_connected();
        if (gdb_poll_socket()&2)
        {
            gdb_trap(5);
            //gdb_ensured_receive(rx_packet);
            //handle_packet(rx_packet);
        }
        return 0;
    }
    else
    {
        gdb_trap(5);
    }
    return 0;
}

/*f gdb_trap
  Send the given signal
*/
void gdb_trap (int signalid)
{
    char rx_packet [1024];
    int cont;

    static int in_trap;
    if (in_trap) return;
    in_trap = 1;
    
    if (stub.enabled==0)
        return;

    stub.status = signalid;
    if (stub.client_socket)
    {
        send (stub.client_socket, "||||", 4, 0);
        char msg[4];
        msg[0] = 'S';
        msg[1] = '0';
        msg[2] = hex[stub.status];
        msg[3] = 0;
        gdb_send (msg);
    }
    
    cont = 0;
    while (!cont)
    {
        gdb_ensured_receive(rx_packet);
        cont = handle_packet( rx_packet);
    }
    in_trap = 0;
}

                                                           
