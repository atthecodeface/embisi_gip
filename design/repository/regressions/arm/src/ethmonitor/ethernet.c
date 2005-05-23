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

    /*b Not accepted? bounce for now
     */
    if (!accept) // for debug we bounce all unexpected packets :-)
    {
        uart_tx_string( "nfu\r\n");
        buffer->buffer_size = rxed_byte_length;
        ethernet_tx_buffer( buffer );
        return;
    }

    /*b Check for ARP first...
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

    /*b Check its a valid IP packet
      eth hdr (6 dest, 6 src, 2 prot=0800 IP)
      4500 0054 0000 4000 400153a1 0a016401 0a016405 0800 c5 92 19 70 0001 data(56)
      VHSS leng Iden fFrg TTPPcsum source   dest     data
      leng is #32 bit words

      version 4, IHL 5, SS=0 (type of service blah), length including hdr and data
      flags (3 bits) bit zero and two clear, frag (13 bits) must be zero; iden ignored
      TT (time to live) ignored
      prot: 01 => ICMP
      csum:
      0x4500+0x54+0x4000+0x4001+0x0a01+0x6401+0x0a01+0x6405 = 0x1a15d -> 0xa15e ~-> 0x5ea1
      0x4500+0x54+0x4000+0x4001+0x0a01+0x6401+0x0a01+0x6405+0x5ea1 == 0x1fffe
                                                                      seq  data
      0000  12 34 56 78 9a bc 00 05 5d 49 31 1e 08 00 45 00   .4Vx....]I1...E.
      0010  00 54 00 00 40 00 40 01 5e a1 0a 01 64 01 0a 01   .T..@.@.^...d...
      0020  64 05 08 00 c5 92 19 70 01 00 bb e8 90 42 d2 ce   d......p.....B..
      0030  0e 00 08 09 0a 0b 0c 0d 0e 0f 10 11 12 13 14 15   ................
      0040  16 17 18 19 1a 1b 1c 1d 1e 1f 20 21 22 23 24 25   .......... !"#$%
      0050  26 27 28 29 2a 2b 2c 2d 2e 2f 30 31 32 33 34 35   &'()*+,-./012345
      0060  36 37                                             67

      0: 12345678
      1: 9abc0005
      2: 5d49311e
      3: 08004500 IP..VHSS
      4: 00540000 lengIden
      5: 40004001 fFrgTTPP
      6: 5ea10a01 csum..IP
      7: 64010a01 SRC...IP
      8: 64050800 DST.data
      9: c5921970
      10: 0100bbe8
      11: 9042d2ce
      12: 0e000809 ...
    */
    accept = 0;
    if ( (data[3]==0x08004500) && // check IP, version, IHL, SS
         ((data[4]>>16) <= (rxed_byte_length-14)) && // check length is all within received buffer
         ((data[5]&0xbfff0000)==0) && // fragment = 0, flags bits 0 and 2 clear
         ((data[7]&0xffff)==(eth.ip_address>>16)) && // dest IP
         ((data[8]>>16)==(eth.ip_address&0xffff)) )
    {
        uart_tx_string( "csum?\r\n");
        accept = 0x4500+(data[4]>>16)+(data[4]&0xffff)+(data[5]>>16)+(data[5]&0xffff)+(data[6]>>16)+(data[6]&0xffff)+(data[7]>>16)+(data[7]&0xffff)+(data[8]>>16);
        accept = (accept&0xffff)+(accept>>16);
        accept = (accept&0xffff)+(accept>>16);
        accept = (accept==0xffff);
    }

    /*b Not accepted? bounce for now
     */
    if (!accept) // for debug we bounce all unexpected packets :-)
    {
        uart_tx_string( "nfu/bad IP\r\n");
        buffer->buffer_size = rxed_byte_length;
        ethernet_tx_buffer( buffer );
        return;
    }

    /*b If ICMP ping, handle it
      IP data is...
      08               00   c592                  1970       0100       data...
      8=echo, 0=reply  00   1's comp 16bit-csum   identifier sequence   data...
     */
    if ( ((data[5]&0xff)==1) && // ICMP
         ((data[8]&0xffff)==0x0800) ) // echo
    {
        unsigned int csum;
        // swap src and dest (does not effect crc)
        // set reply not echo (+8 from body csum, <0 =>?)
        // send to source eth address
        data[8] = (data[7]&0xffff0000) | 0x0000; // set reply not echo and copy src to dest
        data[7] = (data[6]&0xffff) | (eth.ip_address<<16); // copy src to dest
        data[6] = (data[6]&0xffff0000) | (eth.ip_address>>16); // note hdr csum unchanged
        csum = (data[9]>>16)+8;
        csum = ((csum>>16)+csum)&0xffff;
        data[9] = (csum<<16) | (data[9]&0xffff);
        data[0] = (data[1]<<16) | (data[2]>>16);
        data[1] = (data[2]<<16) | eth.hwaddress_high16;
        data[2] = eth.hwaddress_low32;
        buffer->buffer_size = rxed_byte_length; // should have at least 64 bytes; we are sending a crc and another crc though!
        ethernet_tx_buffer( buffer );
        return;
    }

    /*b UDP packet?
      // source port 1234 (0x4d2), dest port 2345 (0x929
    0000  12 34 56 78 9a bc 00 05 5d 49 31 1e 08 00 45 00   .4Vx....]I1...E.
    0010  00 3a 6a 9a 40 00 40 11 f4 10 0a 01 64 01 0a 01   .:j.@.@.....d...
    0020  64 05 04 d2 09 29 00 26 e6 17 54 68 69 73 20 69   d....).&..This i
    0030  73 20 73 6f 6d 65 20 74 65 78 74 20 49 20 61 6d   s some text I am
    0040  20 73 65 6e 64 69 6e 67                            sending

    0: 12345678
    1: 9abc0005
    2: 5d49311e
    3: 08004500 IP..VHSS
    4: 003a6a9a lengIden
    5: 40004011 fFregTTPP (0x11 = UDP)
    6: f4100a01 csum..IP
    7: 64010a01 SRC...IP
    8: 640504d2 DST.SrcP
    9: 09290026 DstPLeng
    10: e6175468 CsumData
    11: 69732069 UDP payload...

    04d2   0929  0026    e617
    src    dst   length  csum
     */
    if ( ((data[5]&0xff)==0x11) && // UDP
         ((data[9]&0xffff)==((data[4]>>16)+20)) ) // lengths match
    {
        unsigned int csum;
        csum = 0;// source and dest IP, 0x0011 (protocol), (udp length+12) are pseudo-header; then all the data, which (if odd) is right-padded with a zero byte
    }

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
