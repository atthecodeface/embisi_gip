/*a Includes
 */
#include <stdlib.h> // for NULL
#include "gip_support.h"
#include "postbus.h"
#include "../common/wrapper.h"
#include "../drivers/ethernet.h"
#include "memory.h"

/*a Defines
 */
#define BUFFER_SIZE (2048)

/*a Types
 */
typedef struct t_eth_data
{
    unsigned int hwaddress_high32;
    unsigned int hwaddress_low16;
    unsigned int hwaddress_high16;
    unsigned int hwaddress_low32;
    unsigned int ip_address;
} t_eth_data;

/*a Static variables
 */
static t_eth_buffer buffers[16];
static t_eth_data eth;

/*a Ethernet handling functions
 */
/*f tx_callback
 */
static void tx_callback( void *handle, t_eth_buffer *buffer )
{
    uart_tx_string( "txcallback ");
    uart_tx_hex8(buffer);
    uart_tx_nl();
    buffer->buffer_size = BUFFER_SIZE;
    ethernet_add_rx_buffer( buffer );
}

/*f rx_callback
Ethernet packet: 6 bytes dest, 6 bytes source, 2 bytes protocol type, rest...
ARP: 2 byte address space; 2 byte protocol type; 1 byte length of hw address; 1 byte length of protocol address; 2 bytes opcode (reply/request); hw sender addr; protocol sender address; hw target address; protocol target address

for all ethernet packets: check dest is us or broadcast
for ARP: check eth protocol type, arp prot type, address space, hw address len, prot address len
         if match, add sender prot/hw to our small arp table; if request, put sender->target fields, fill sender fields, send to hw sender address

ARP...
0: ffffffff
1: ffff0005
2: 5d49311e
3: 08060001
4: 08000604
5: 00010005
6: 5d49311e
7 :0a016401
8: 00000000
9: 00000a01
10: 6405xxxx

 */
static void rx_callback( void *handle, t_eth_buffer *buffer, int rxed_byte_length )
{
    unsigned int *data;
    int accept;

    uart_tx_string( "rxcallback ");
    uart_tx_hex8(buffer);
    uart_tx_nl();

    /*b check for our address or broadcast address;
     */
    data = (unsigned int *)(buffer->data);
    accept = 0;
    if ((data[0]==0xffffffff) && ((data[1]>>16)==0xffff))
    {
        accept = 1;
    }
    else if ( (data[0]==eth.hwaddress_high32) && ((data[1]>>16)==eth.hwaddress_low16) )
    {
        accept = 1;
    }

    /*b Not accepted, so bounce for now
     */
    if (!accept) // for debug we bounce all unexpected packets :-)
    {
        uart_tx_string( "nfu\r\n");
        buffer->buffer_size = rxed_byte_length;
        ethernet_tx_buffer( buffer );
        return;
    }

    /*b If accepted, handle it and return
      check for ARP first...
     */
    // ARP...
    // 08 06 00 01  08 00  06 04  00 01        00055d49311e 0a016401   000000000000 0a016405    xxxxx
    // ethpt addsp  arppt  hw ip  opcode(req)  senderhw     senderip   targethw     targetip
    if ((data[3]==0x08060001) && (data[4]==0x08000604) && (rxed_byte_length>=46))
    {
        uart_tx_string( "arp\r\n");
        //arp_entry_set( data[7], data[5]&0xffff, data[6] );
        if ( ((data[5]>>16)==1) && // ARP request for us
             ((data[9]&0xffff) == (eth.ip_address>>16)) &&
             ((data[10]>>16) == (eth.ip_address&0xffff)) )
        {
            uart_tx_string( "to us\r\n");
            data[0] = (data[5]<<16) | (data[6]>>16);
            data[1] = (data[6]<<16) | eth.hwaddress_high16;
            data[2] = eth.hwaddress_low32;
            data[8] = (data[5]<<16) | (data[6]>>16);
            data[9] = (data[6]<<16) | (data[7]>>16);
            data[10] = (data[7]<<16);
            data[5] = (0x0002 << 16) | (eth.hwaddress_high16); // ARP reply
            data[6] = eth.hwaddress_low32;
            data[7] = eth.ip_address;

            buffer->buffer_size = 64; // always pad to 64 minimum eth packet size...
            ethernet_tx_buffer( buffer );
            return;
        }
    }

    /*b ping request
      eth hdr (6 dest, 6 src, 2 prot=0x80 ICMP)
      4500 0054 0000 4000 400153a1 0a016401 0a016405 0800 c5 92 19 70 0001 data(56)
                                                                      seq  data
      0000  12 34 56 78 9a bc 00 05 5d 49 31 1e 08 00 45 00   .4Vx....]I1...E.
      0010  00 54 00 00 40 00 40 01 5e a1 0a 01 64 01 0a 01   .T..@.@.^...d...
      0020  64 05 08 00 c5 92 19 70 01 00 bb e8 90 42 d2 ce   d......p.....B..
      0030  0e 00 08 09 0a 0b 0c 0d 0e 0f 10 11 12 13 14 15   ................
      0040  16 17 18 19 1a 1b 1c 1d 1e 1f 20 21 22 23 24 25   .......... !"#$%
      0050  26 27 28 29 2a 2b 2c 2d 2e 2f 30 31 32 33 34 35   &'()*+,-./012345
      0060  36 37                                             67
    */

    /*b Done
     */
    uart_tx_string( "done\r\n");
    buffer->buffer_size = BUFFER_SIZE;
    ethernet_add_rx_buffer( buffer );
}

/*a External functions
 */
/*f mon_ethernet_poll
 */
extern void mon_ethernet_poll( void )
{
    ethernet_poll();
}

/*f mon_ethernet_init
 */
extern void mon_ethernet_init( void )
{
    int i;

    eth.hwaddress_high16 = 0x1234;
    eth.hwaddress_low32 = 0x56789abc;
    eth.ip_address = 0x0a016405;

    eth.hwaddress_high32 = (eth.hwaddress_high16<<16) | (eth.hwaddress_low32>>16);
    eth.hwaddress_low16  = (eth.hwaddress_low32&0xffff);
    ethernet_init();
    ethernet_set_tx_callback( tx_callback, NULL );
    ethernet_set_rx_callback( rx_callback, NULL );
    for (i=0; i<8; i++)
    {
        buffers[i].data = DRAM_START+BUFFER_SIZE*i;
        buffers[i].buffer_size = BUFFER_SIZE;
        ethernet_add_rx_buffer( &buffers[i] );
    }
}
