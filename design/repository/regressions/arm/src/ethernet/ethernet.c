/*a Includes
 */
#include "gip_support.h"
#include "postbus.h"
#include "../common/wrapper.h"

/*a Defines
 */

/*a Ethernet handling functions
 */
/*f Transmit an ethernet packet
 */
static void tx_ethernet_packet( int byte_length, unsigned int *data )
{
    int i, j;
    unsigned int s;
    for (i=j=0; i<(byte_length+3)/4; i++)
    {
        GIP_POST_TXD_0( data[i] );
        j++;
        if (j==8)
        {
            // wait for txpending[0] to be clear?
            GIP_POST_TXC_0_IO_FIFO_DATA( 0,j-1,0, 0,0,0 ); // add j words to etx fifo (egress data fifo 0)
            j = 0;
        }
    }
    if (j>0)
    {
        // wait for txpending[0] to be clear?
        GIP_POST_TXC_0_IO_FIFO_DATA( 0,j-1,0, 0,0,0 ); // add j words to etx fifo (egress data fifo 0)
    }

    // wait for txpending[0] to be clear?
    GIP_POST_TXD_0( 0x01000000 );
    GIP_POST_TXD_0( 0x21800000 | byte_length );
    GIP_POST_TXC_0_IO_FIFO_DATA( 0,1,1, 0,0,1 ); // add to egress cmd fifo 0, set semaphore 1 on completion
}

/*f Handle a non-empty ethernet rx status fifo
  Packet length is bottom 16 bits
  Block length is 8;16
  Status is 3;24 : FCS ok (0), FCS bad (1), odd nybbles (2), block complete (3), overrun (4), framing error (5)
 */
static void handle_rx_status_fifo( void )
{
    unsigned int s, time;

    GIP_READ_AND_CLEAR_SEMAPHORES( s, 1<<31 );
    GIP_POST_TXD_0( (0<<postbus_command_source_io_cmd_op_start) | (2<<postbus_command_source_io_length_start) | (31<<postbus_command_target_gip_rx_semaphore_start) ); // cmd_op[2]=2, length=1 (get status), semaphore=0
    GIP_POST_TXC_0_IO_CMD( 0, 0, 0, 0xc );

    // wait for the data to come back
    while (1)
    {
        GIP_READ_AND_CLEAR_SEMAPHORES( s, 1<<31 );
        if ((s>>31)&1) break;
    }
    GIP_POST_RXD_0(time);
    GIP_POST_RXD_0(s);

    unsigned int size, status;
    status = (s>>24)&7;
    size = (s>>16)&0xff; // read this (number of bytes+3)/4 words
    // 0 => fcs ok; 1=>fcs bad; 2=> odd nybbles; 3=> complete block; 4=>fifo overrun!; 5=>framing error
    // we get 03400040, 03400080, 00120092 for a 146 byte packet
    if (size>0)
    {
        int i;
        GIP_POST_TXD_0( (0<<postbus_command_source_io_cmd_op_start) | (((size+3)/4)<<postbus_command_source_io_length_start) | (31<<postbus_command_target_gip_rx_semaphore_start) ); // cmd_op[2]=0, length=1 (get status), semaphore=0
        GIP_POST_TXC_0_IO_CMD( 0, 0, 0, 0x4 ); // eth rx d, ingr data fifo 0
        while (1)
        {
            GIP_READ_AND_CLEAR_SEMAPHORES( s, 1<<31 );
            if ((s>>31)&1) break;
        }
        for (i=0; i<((size+3)/4); i++)
        {
            GIP_POST_RXD_0(s);
        }
    }
}

/*f Handle a non-empty ethernet tx fifo
  Length is bottom 16 bits
  Status is 2;16
 */
static void handle_tx_status_fifo( void )
{
    unsigned int s, time;

    GIP_POST_TXD_0( (0<<postbus_command_source_io_cmd_op_start) | (2<<postbus_command_source_io_length_start) | (0<<postbus_command_target_gip_rx_semaphore_start) ); // cmd_op[2]=2, length=1 (get status), semaphore=0
    GIP_POST_TXC_0_IO_CMD( 0, 0, 0, 0xd );

    // wait for the data to come back
    s = 0;
    while (!(s&2))
    {
        GIP_POST_STATUS_0(s);
    }
    GIP_POST_RXD_0(time);
    GIP_POST_RXD_0(s);

    s = (s>>16)&3;
    if (s==0) {} // tx ok
    if (s==1) {} // late coll
    if (s==2) {} // retries exceeded

}

/*a Test entry point
 */
/*f test_entry_point
The postbus interface has 4 status, 4 cmd, 4 fifo, 4 rx config, 4 tx config; 0,1,2,3 is status, command, tx fifo, rx fifo; 16/17 is rx/tx config. Add 4 for the fifo 1, 8 for fifo 2, 12 for fifo 3

constant integer postbus_command_last_bit = 0; // bit 0 of first transaction word indicates its also the last
constant integer postbus_command_route_start = 1; // bits 1-7 of first transaction word indicates the destination (routing information) for the transaction
constant integer postbus_command_rx_fifo_start = 8; // bit 8-9 of first transaction word indicates the receive FIFO to receive into
constant integer postbus_command_io_dest_type_start = 8; // bit 8-9 of first transaction word indicates the type of destination - 00=>channel, 01=>data FIFO, 10=>command FIFO, 11=>IO interface config
constant integer postbus_command_tx_length_start = 10; // bits 10-14 of first transaction word indicates the length (to GIP)
constant integer postbus_command_flow_type_start = 15; // bit 15-16 of first transaction word indicates the flow control action to do on receive (00=>write, 01=>set, 10=>add)
constant integer postbus_command_flow_value_start = 17; // bit 17-20 of first transaction word indicates the data for the flow control action
constant integer postbus_command_spare = 21; // bit 21 of first transaction word spare
constant integer postbus_command_tx_signal_start = 22; // bit 22-26 of command (first transcation word out) indicates (if nonzero) the semaphore to set on completion of push to postbus
constant integer postbus_command_rx_signal_start = 27; // bit 27-31 of first transaction word indicates (if nonzero) the semaphore to set on completion of push postbus
constant integer postbus_command_io_dest_start = 27; // bit 27-31 of first transaction word indicates the channel or data/command FIFO an IO transaction word is for

 */
extern int test_entry_point()
{
    unsigned int s;
    int empty;
    FLASH_CONFIG_WRITE( 0x355 );
    FLASH_ADDRESS_WRITE( 0x80000000 ); // CE 2
    GIP_POSTIF_CFG( 0, 0x00001000 ); // Rx fifo 0 config base 0, end 16, read 0, write 0 - can overlap with tx as we use one at a time
    GIP_POSTIF_CFG( 1, 0x00001000 ); // Tx fifo 0 config base 0, end 16, read 0, write 0

    NOP;
    NOP;
    NOP;
    NOP;

    GIP_POSTBUS_IO_FIFO_CFG( 0, 0, 0,  512,  15, 3 );  // etx cmd
    GIP_POSTBUS_IO_FIFO_CFG( 0, 0, 1,  528,  15, 3 );  // uart0 cmd
    GIP_POSTBUS_IO_FIFO_CFG( 0, 0, 2,  544,  15, 3 );  // ss cmd
    GIP_POSTBUS_IO_FIFO_CFG( 1, 0, 0,     0, 255, 3 ); // etx data
    GIP_POSTBUS_IO_FIFO_CFG( 1, 0, 1,   256, 32, 3 );  // ss tx data
    GIP_POSTBUS_IO_FIFO_CFG( 0, 1, 0,  512,  15, 3 );  // erx status
    GIP_POSTBUS_IO_FIFO_CFG( 0, 1, 1,  528,  15, 3 );  // etx status
    GIP_POSTBUS_IO_FIFO_CFG( 0, 1, 2,  544,  15, 3 );  // uar0 status
    GIP_POSTBUS_IO_FIFO_CFG( 0, 1, 3,  560,  15, 3 );  // ss status
    GIP_POSTBUS_IO_FIFO_CFG( 1, 1, 0,     0, 255, 3 ); // erx data
    GIP_POSTBUS_IO_FIFO_CFG( 1, 1, 1,   256, 255, 3 ); // ss rx data

// invoke the postbus target to send us a header back - test the path, really
//    s = 0x18000000; // header for I/O to send back: route(7;1)=0, dest GIP FIFO (2;8)=0, I/O length(4;10)=0, I/O cmd (2;22)=0 (send hdr), signal (5;27)=3
//    __asm__ volatile ( " .word 0xec00c62e \n mov r0, %0 \n" : : "r" (s) ); // extrnrm (d): dnxm n/m is top 3.1, type 7 for no override - tx fifo config
//    s = 0x38000000; // fifo(2;29)=1, status_fifo(28)=1, ingress(27)=1, length_m_one(5;10)=0, io_dest_type(2;8)=0, route(7)=0, last(1)=0 -- do 'send header' command
//    __asm__ volatile ( " .word 0xec00c61e \n mov r0, %0 \n" : : "r" (s) ); // extrnrm (d): dnxm n/m is top 3.1, type 7 for no override - tx fifo config

//    for (s=0; s<30; s++) NOP; // 650ns per loop

// header_details[0] is all we have at the moment
// set header_details[0] to point to us
// 0x11, 0x15, 0x19, 0x1d are 4 header_details
    GIP_POST_TXD_0( GIP_POSTBUS_COMMAND( 0, 0, 0 ) ); // header details: route=0
    GIP_POST_TXC_0_IO_CMD( 0, 0, 0, 0x11 );
    NOP;NOP;NOP;NOP;NOP;NOP;

// 0x10, 0x14, 0x18, 0x1c are 4 events
// set event[1] to be erxs not empty send header_details[0] as command, setting semaphore 3
// set event[2] to be etxs not empty send header_details[0] as command, setting semaphore 3
    GIP_POST_TXD_0( (0<<0) | (1<<2) | (1<<3) | (1<<4) | (0<<5) | (0<<postbus_command_source_io_cmd_op_start) | (0<<postbus_command_source_io_length_start) | (3<<postbus_command_target_gip_rx_semaphore_start) ); // fifo, cmdstatus, ingress, empty/watermark, level, target
    GIP_POST_TXC_0_IO_CMD( 0, 0, 0, 0x14 );
    NOP;NOP;NOP;NOP;NOP;NOP;
    GIP_POST_TXD_0( (1<<0) | (1<<2) | (1<<3) | (1<<4) | (0<<5) | (0<<postbus_command_source_io_cmd_op_start) | (0<<postbus_command_source_io_length_start) | (3<<postbus_command_target_gip_rx_semaphore_start) ); // fifo, cmdstatus, ingress, empty/watermark, level, target
    GIP_POST_TXC_0_IO_CMD( 0, 0, 0, 0x18 );
    NOP;NOP;NOP;NOP;NOP;NOP;

// MDIO something
    GIP_POST_TXD_0( 0x5ada1234 ); // data to go out... write to '10101' reg '' 
    GIP_POST_TXC_0_IO_FIFO_DATA(0,0,0,1,0,0); // add to egress data fifo 1
    NOP;NOP;NOP;NOP;NOP;NOP;
    GIP_POST_TXD_0( 0x01000000 ); // immediate
    GIP_POST_TXD_0( 0x10700420 ); // cont 0, type 2, cpha 0, cpol 0, cl 0, inp 0, cs 7, tristate no, div 8, 32 bits
    GIP_POST_TXC_0_IO_FIFO_DATA(0,1,0,2,0,1); // add to egress cmd fifo 2
    NOP;NOP;NOP;NOP;NOP;NOP;

// MDIO something else
    GIP_POST_TXD_0( 0x6ada0000 ); // data to go out... read of '10101' reg '10110'
    GIP_POST_TXC_0_IO_FIFO_DATA(0,0,0,1,0,0); // add to egress data fifo 1
    NOP;NOP;NOP;NOP;NOP;NOP;
    GIP_POST_TXD_0( 0x01000000 ); // immediate
    GIP_POST_TXD_0( 0x107e0420 ); // cont 0, type 2, cpha 0, cpol 0, cl 0, inp 0, cs 7, tristate after 14, div 8, 32 bits
    GIP_POST_TXC_0_IO_FIFO_DATA(0,1,2,2,0,1); // add to egress cmd fifo 2; set semaphore 2 on completion
    NOP;NOP;NOP;NOP;NOP;NOP;

    tx_ethernet_packet( 16, (unsigned int *)"this is the day that the Lord has made" );

    while (1)
    {
        unsigned int s;
        int tx_ne;
        GIP_READ_AND_CLEAR_SEMAPHORES( s, 1<<3 );
        if (s&(1<<3))
        { // status ready somewhere; read both FIFO status' for now
            GIP_POST_TXD_0( (2<<postbus_command_source_io_cmd_op_start) | (1<<postbus_command_source_io_length_start) | (0<<postbus_command_target_gip_rx_semaphore_start) ); // cmd_op[2]=2, length=1 (get status), semaphore=0
            GIP_POST_TXC_0_IO_CMD( 0, 0, 0, 0xc ); // fifo etc(4;27), length_m_one(5;10)=0, io_dest_type(2;8)=0 (cmd), route(7)=0, last(1)=0
            GIP_POST_TXD_0( (2<<postbus_command_source_io_cmd_op_start) | (1<<postbus_command_source_io_length_start) | (0<<postbus_command_target_gip_rx_semaphore_start) ); // cmd_op[2]=2, length=1 (get status), semaphore=0
            GIP_POST_TXC_0_IO_CMD( 0, 0, 0, 0xd ); // fifo etc(4;27), length_m_one(5;10)=0, io_dest_type(2;8)=0 (cmd), route(7)=0, last(1)=0
            NOP;NOP;NOP;NOP;NOP;NOP; // cmd and data are RF writes, so delay, and give a chance for the data to get back
            NOP;NOP;NOP;NOP;NOP;NOP; // cmd and data are RF writes, so delay, and give a chance for the data to get back
            GIP_POST_RXD_0(s); // get status of erx status fifo
            if ((s&0x80000000)==0) // if not empty
            {
                GIP_POST_RXD_0(s);
                tx_ne = ((s&0x80000000)==0);
                handle_rx_status_fifo();
            }
            else
            {
                GIP_POST_RXD_0(s);
                tx_ne = ((s&0x80000000)==0);
            }
            if (tx_ne)
            {
                handle_tx_status_fifo();
            }
        }
    }

}
