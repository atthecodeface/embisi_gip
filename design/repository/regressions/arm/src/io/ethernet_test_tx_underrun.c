/*a Includes
 */
#include <stdlib.h> // for NULL
#include "gip_support.h"
#include "gip_system.h"
#include "postbus.h"
#include "../common/wrapper.h"
#include "ethernet.h"
#include "postbus_config.h"

/*a Defines
 */
#define DRAM_START ((void *)(0x80010000))
#define BUFFER_SIZE (2048)

/*a Types
 */

/*a Static variables
 */
static int failures;

/*a Static functions
 */
/*f tx_callback
 */
static void tx_callback( void *handle, t_eth_buffer *buffer )
{
}

/*f rx_callback
 */
static void rx_callback( void *handle, t_eth_buffer *buffer, int rxed_byte_length )
{
}

/*a Test entry point
 */
/*f test_entry_point
 */
extern int test_entry_point()
{
    failures = 0;

    postbus_config( 16, 16, 0, 0 ); // use 16 for tx and 16 for rx, no sharing
    GIP_CLEAR_SEMAPHORES( -1 );

    ethernet_init( IO_A_SLOT_ETHERNET_0, 0, 2 );
    ethernet_set_tx_callback( tx_callback, NULL );
    ethernet_set_rx_callback( rx_callback, NULL );

    GIP_LED_OUTPUT_CFG_WRITE( 0xaaaa );
    GIP_BLOCK_ALL();
    GIP_EXTBUS_CONFIG_WRITE( 0x111 );
    GIP_BLOCK_ALL();
    GIP_EXTBUS_ADDRESS_WRITE( 0xc0000002 ); // Debug display address
    GIP_BLOCK_ALL();
    GIP_EXTBUS_DATA_WRITE( 0xffff );


    // ask to send 32 byte packet, but only giving 16 bytes
    GIP_POST_TXD_0( 0x12345678 );
    GIP_POST_TXD_0( 0x12345678 );
    GIP_POST_TXD_0( 0x12345678 );
    GIP_POST_TXD_0( 0x12345678 );
    GIP_POST_TXC_0_IO_FIFO_DATA( 0,4-1,31, IO_A_SLOT_ETHERNET_0,0,0 ); // add j words to etx fifo (egress data fifo 0)

    while (1)
    {
        unsigned int s;
        GIP_READ_AND_CLEAR_SEMAPHORES( s, 1<<31 );
        if ((s>>31)&1) break;
    }

    GIP_POST_TXD_0( 0x01000000 );
    GIP_POST_TXD_0( 0x21800000 | 0x20 ); // send packet of 32 bytes, although we only added 16
    GIP_POST_TXC_0_IO_FIFO_DATA( 0,1,31, IO_A_SLOT_ETHERNET_0,0,1 );

    // wait for status
    unsigned int tx_s;
    do
    {
        GIP_POST_TXD_0( (2<<postbus_command_source_io_cmd_op_start) | (1<<postbus_command_source_io_length_start) | (31<<postbus_command_target_gip_rx_semaphore_start) ); // cmd_op[2]=2, length=1 (get status), semaphore=0
        GIP_POST_TXC_0_IO_CMD( 0, 0, 0, 0xc|IO_A_SLOT_ETHERNET_0 ); // fifo etc(4;27), length_m_one(5;10)=0, io_dest_type(2;8)=0 (cmd), route(7)=0, last(1)=0
        while (1)
        {
            unsigned int s;
            GIP_READ_AND_CLEAR_SEMAPHORES( s, 1<<31 );
            if ((s>>31)&1) break;
        }
        GIP_POST_RXD_0(tx_s);
    } while ((tx_s&0x80000000)!=0);

    // read tx status
    /*b Read the status and time
     */
    unsigned int time;
    unsigned int status;
    GIP_READ_AND_CLEAR_SEMAPHORES( status, 1<<31 );
    GIP_POST_TXD_0( (0<<postbus_command_source_io_cmd_op_start) | (2<<postbus_command_source_io_length_start) | (31<<postbus_command_target_gip_rx_semaphore_start) ); // cmd_op[2]=2, length=1 (get status), semaphore=0
    GIP_POST_TXC_0_IO_CMD( 0, 0, 0, 0xc|IO_A_SLOT_ETHERNET_0 );

    // wait for the data to come back
    while (1)
    {
        unsigned int s;
        GIP_READ_AND_CLEAR_SEMAPHORES( s, 1<<31 );
        if ((s>>31)&1) break;
    }
    GIP_POST_RXD_0(time);
    GIP_BLOCK_ALL();
    GIP_EXTBUS_DATA_WRITE( time );
    GIP_POST_RXD_0(status);
    GIP_BLOCK_ALL();
    GIP_BLOCK_ALL();
    GIP_BLOCK_ALL();
    GIP_BLOCK_ALL();
    GIP_BLOCK_ALL();
    GIP_BLOCK_ALL();
    GIP_EXTBUS_DATA_WRITE( status );
    // status back is:
    // [31] set (tx status)
    // [8;23] undefined (actually last incoming command, so from 0x21800000)
    // [22] underrun (1)
    // [2;20] status (ok==0)
    // [4;16] retries (0)
    // [16;0] length (32)
    // so we expect
    // 1 010.0001.1 1 00 0000 0000.0000.0010.0000
    // a1c00020
    if (status!=0xa1c00020) failures++;

    return failures;
}
