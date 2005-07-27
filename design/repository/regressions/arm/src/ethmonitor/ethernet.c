/*a Includes
 */
#include <stdlib.h> // for NULL
#include "gip_support.h"
#include "postbus.h"
#include "../common/wrapper.h"
#include "../drivers/ethernet.h"
#include "../drivers/uart.h"
#include "../drivers/flash.h"
#include "memory.h"
#include "cmd.h"

/*a Defines
 */
#define BUFFER_SIZE (2048)
#define CSUM32(csum,ptr) {csum+=(ptr[0]>>16)+(ptr[0]&0xffff);ptr++;}

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

/*a Command functions
 */
/*f udp_reply
 */
static void udp_reply( t_eth_buffer *buffer, unsigned int byte_length )
{
    unsigned int *data;
    unsigned int udp_src_port;
    unsigned int udp_byte_length;
    unsigned int csum, *csum_data;
    int i;

    udp_byte_length = byte_length+8;
    data = (unsigned int *)(buffer->data);
    data[0] = (data[1]<<16) | (data[2]>>16);
    data[1] = (data[2]<<16) | eth.hwaddress_high16;
    data[2] = eth.hwaddress_low32;
    udp_src_port = data[8]&0xffff;
    data[4] = ((udp_byte_length+20)<<16) | (data[4]&0xffff); // length
    data[5] = 0x40004011; // reset TTL, fragment, and protocol (UDP)
    data[8] = (data[7]&0xffff0000) | (data[9]>>16); // new source port, bottom of dest IP
    data[7] = (data[6]&0xffff) | (eth.ip_address<<16); // top of dest IP, bottom of src IP
    data[6] = (eth.ip_address>>16); // top of src IP; note hdr csum cleared
    data[9] = (udp_src_port<<16) | udp_byte_length;
    data[10] = data[10]&0xffff; // clear UDP csum
    // now do both csums
    csum = data[3]&0xffff;
    csum_data = data+4;
    CSUM32( csum, csum_data ); // data[4]
    CSUM32( csum, csum_data ); // data[5]
    CSUM32( csum, csum_data ); // data[6]
    CSUM32( csum, csum_data ); // data[7]
    csum = csum+(data[8]>>16);
    csum = (csum>>16)+(csum&0xffff);
    csum = (csum>>16)+(csum&0xffff);
    data[6] = ((~csum)<<16) | (data[6]&0xffff);

    csum = data[6]&0xffff; // source IP low to start
    csum += 0x11; // protocol in pseudo header
    csum += udp_byte_length; // length in pseudo header
    csum_data = data+7;
    for (i=0; i<(1+udp_byte_length/4); i++) // do 1+(byte_length/4) words from data[7] - for zero data (byte_length==8) this means data[7] thru data[9] - we still need 2 more bytes!
    {
        CSUM32( csum , csum_data );
    }
    // byte length=8/12/14 implies 2 more bytes; 9/13/17 implies 3 more bytes, 10 implies 4 more bytes, 11 implies 5 more bytes
    csum += (csum_data[0]>>16);
    switch (udp_byte_length&3)
    {
    case 0:
        break;
    case 1:
        csum += csum_data[0]&0xff00;
        break;
    case 2:
        csum += csum_data[0]&0xffff;
        break;
    case 3:
        csum += csum_data[0]&0xffff;
        csum += (csum_data[1]>>16)&0xff00;
        break;
    }
    csum = (csum>>16)+(csum&0xffff);
    csum = (csum>>16)+(csum&0xffff);
    csum = ~csum;
    if (csum==0) csum=0xffff;
    data[10] = (csum<<16) | data[10];

    buffer->buffer_size = 34+udp_byte_length; // 20 for IP header, 14 for eth header
    if (buffer->buffer_size<64)
    {
        buffer->buffer_size = 64;
    }
    ethernet_tx_buffer( buffer );
}

/*f mon_ethernet_cmd_done
  we should have a result packet that we can just turn round
  need to put src eth address as dest
  need to put our eth address as source
  need to swap IP addresses
  need to swap IP ports
  need to calculate the checksum
  then send the packet
 */
extern void mon_ethernet_cmd_done( void *handle, int space_remaining )
{
    t_eth_buffer *buffer;
    unsigned int byte_length;
    buffer = (t_eth_buffer *)handle;
    byte_length = 1200-space_remaining;

    udp_reply( buffer, byte_length);
}

/*a Memory operations
 */
static void mem_obey( t_eth_buffer *buffer, char *data, int pkt_length )
{
    unsigned int type;
    unsigned int address;
    unsigned int length;
    int i;

    type = data[0] | (data[1]<<8);
    address = data[2] | (data[3]<<8) | (data[4]<<16) | (data[5]<<24);
    length = data[6] | (data[7]<<8) | (data[8]<<16) | (data[9]<<24);
    switch (type)
    {
    case 0: // read memory
        data[1] = 1;
        for (i=0; (i<length) && (i<1200); i++)
        {
            data[10+i] = ((char *)address)[i];
        }
        pkt_length = 10+i;
        break;
    case 1: // write and verify memory
        data[1] = 1;
        for (i=0; (i<length) && (i<pkt_length-10); i++)
        {
            ((volatile char *)address)[i] = data[10+i];
            data[10+i] = ((volatile char *)address)[i];
        }
        pkt_length = 10+i;
        break;
    case 8: // read flash
        data[1] = 1;
        if (!flash_read_buffer( address, data+10, length>(pkt_length-10)?(pkt_length-10):length )) data[0] = 15;
        break;
    case 9: // write and verify flash
        data[1] = 1;
        if (!flash_write_buffer( address, data+10, length>(pkt_length-10)?(pkt_length-10):length )) data[0] = 15;
        if (!flash_read_buffer( address, data+10, length>(pkt_length-10)?(pkt_length-10):length )) data[0] = 15;
        break;
    case 10: // erase flash
        data[1] = 1;
        if (!flash_erase_block( address )) data[0] = 15;
        break;
    case 16: // load registers and run
    {
        unsigned int regs[16];
        data[1] = 1;
        for (i=0; (i<16) && ((i*4)<pkt_length-10); i++)
        {
            regs[i] = data[10+i*4] | (data[11+i*4]<<8) | (data[12+i*4]<<16) | (data[13+i*4]<<24);
        }
        udp_reply( buffer, pkt_length );
        // wait for a short while then load the registers
        for (i=1; i<10000; i++) NOP;
         __asm__ volatile (" mov r0, %0 \n ldmia r0, {r0-pc} \n movnv r0, r0 \n movnv r0, r0 \n movnv r0, r0" : : "r" (regs) );
        break;
    }
    }
    udp_reply( buffer, pkt_length );
}

/*a Ethernet handling functions
 */
/*f tx_callback
 */
static void tx_callback( void *handle, t_eth_buffer *buffer )
{
#ifdef DEBUG
    uart_tx_string( "txcb ");
    uart_tx_hex8((unsigned int)buffer);
    uart_tx_nl();
#endif
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

#ifdef DEBUG
    uart_tx_string( "rxcallback ");
    uart_tx_hex8((unsigned int)buffer);
    uart_tx_nl();
#endif

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

    /*b Not accepted? ignore
     */
    if (!accept) // for debug we bounce all unexpected packets :-)
    {
        uart_tx_string( "nfu ");
        uart_tx_hex8( data[8] );
        uart_tx_nl();
        buffer->buffer_size = BUFFER_SIZE;
        ethernet_add_rx_buffer( buffer );
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
        accept = 0x4500+(data[4]>>16)+(data[4]&0xffff)+(data[5]>>16)+(data[5]&0xffff)+(data[6]>>16)+(data[6]&0xffff)+(data[7]>>16)+(data[7]&0xffff)+(data[8]>>16);
        accept = (accept&0xffff)+(accept>>16);
        accept = (accept&0xffff)+(accept>>16);
        accept = (accept==0xffff);
    }

    /*b Not accepted? drop it
     */
    if (!accept) // for debug we bounce all unexpected packets :-)
    {
        uart_tx_string( "nfu/bad IP\r\n");
        buffer->buffer_size = BUFFER_SIZE;
        ethernet_add_rx_buffer( buffer );
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

    /*b UDP packet? (RFC768)
      // source port 1234 (0x4d2), dest port 2345 (0x929
    0000  12 34 56 78 9a bc 00 05 5d 49 31 1e 08 00 45 00   .4Vx....]I1...E.
    0010  00 3a 6a 9a 40 00 40 11 f4 10 0a 01 64 01 0a 01   .:j.@.@.....d...
    0020  64 05 04 d2 09 29 00 26 e6 17 54 68 69 73 20 69   d....).&..This i
    0030  73 20 73 6f 6d 65 20 74 65 78 74 20 49 20 61 6d   s some text I am
    0040  20 73 65 6e 64 69 6e 67                            sending

    0000  12 34 56 78 9a bc 00 05 5d 49 31 1e 08 00 45 00   .4Vx....]I1...E.
    0010  00 1c e8 85 40 00 40 11 76 43 0a 01 64 01 0a 01   ....@.@.vC..d...
    0020  64 05 04 d2 09 29 00 08 15 db                     d....)....

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

    9: 09290008
    10: 15db

    04d2   0929  0026    e617
    04d2   0929  0008    15db
    src    dst   length  csum
     */
    int udp_byte_length;
    udp_byte_length = data[9]&0xffff;
    if ( ((data[5]&0xff)==0x11) && // UDP
         ((udp_byte_length+20)==(data[4]>>16)) ) // lengths match
    {
        unsigned int csum;
        unsigned int *csum_data;
        int i;
        // source and dest IP, 0x0011 (protocol), udp length are pseudo-header; then all the data, which (if odd) is right-padded with a zero byte; if csum==0, then send ffff
        csum = data[6]&0xffff; // source IP low to start
        csum += 0x11; // protocol in pseudo header
        csum += udp_byte_length; // length in pseudo header
        csum_data = data+7;
        for (i=0; i<(1+udp_byte_length/4); i++) // do 1+(byte_length/4) words from data[7] - for zero data (byte_length==8) this means data[7] thru data[9] - we still need 2 more bytes!
        {
            CSUM32( csum , csum_data );
        }
        // byte length=8/12/14 implies 2 more bytes; 9/13/17 implies 3 more bytes, 10 implies 4 more bytes, 11 implies 5 more bytes
        csum += (csum_data[0]>>16);
        switch (udp_byte_length&3)
        {
        case 0:
            break;
        case 1:
            csum += csum_data[0]&0xff00;
            break;
        case 2:
            csum += csum_data[0]&0xffff;
            break;
        case 3:
            csum += csum_data[0]&0xffff;
            csum += (csum_data[1]>>16)&0xff00;
            break;
        }
        csum = (csum>>16)+(csum&0xffff);
        csum = (csum>>16)+(csum&0xffff);
        if (csum==0xffff) // csum okay!
        {
            if ((data[9]>>16) == 0x929)
            {
                uart_tx_string_nl( "c");
                cmd_obey( buffer, ((char *)data)+42, udp_byte_length-8, 1200 );
                return;
            }
            if ((data[9]>>16) == 0x92a)
            {
                uart_tx_string( "m");
                mem_obey( buffer, ((char *)data)+42, udp_byte_length-8 );
                return;
            }
            buffer->buffer_size = BUFFER_SIZE;
            ethernet_add_rx_buffer( buffer );
            return;
        }
        uart_tx_string( "badudp ");
        uart_tx_hex8( udp_byte_length );
        uart_tx_string( " : ");
        uart_tx_hex8( csum );
        uart_tx_nl();
        for (i=0; i<rxed_byte_length; i+=4)
        {
            int j;
            uart_tx_hex8( data[i/4] );
            uart_tx(' ');
            for (j=0; j<10000; j++) NOP;
        }
        buffer->buffer_size = rxed_byte_length;
        ethernet_tx_buffer( buffer );
        return;
    }

    /*b Done
     */
    uart_tx_string_nl( "unknown");
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
extern void mon_ethernet_init( unsigned int eth_address_hi, unsigned int eth_address_lo, unsigned int ip_address )
{
    int i;

//    eth.hwaddress_high16 = 0x1234;
//    eth.hwaddress_low32 = 0x56789abc;
//    eth.ip_address = 0x0a016405;
    eth.hwaddress_high16 = eth_address_hi;
    eth.hwaddress_low32 = eth_address_lo;
    eth.ip_address = ip_address;

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
