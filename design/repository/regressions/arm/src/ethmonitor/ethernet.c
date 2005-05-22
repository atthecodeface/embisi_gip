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

    /*b check for our address or broadcast address;
     */
    data = (unsigned int *)buffer->data;
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
    if ((data[3]==0x08060001) && (data[4]==0x08000604) && (rxed_byte_length>46))
    {
        //arp_entry_set( data[7], data[5]&0xffff, data[6] );
        if ( ((data[5]>>16)==1) && // ARP request for us
             ((data[9]&0xffff) == (eth.ip_address>>16)) &&
             ((data[10]>>16) == (eth.ip_address&0xffff)) )
        {
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

    /*b Done
     */
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
