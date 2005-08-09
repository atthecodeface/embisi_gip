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
static t_eth_buffer buffers[16];
static int failures;
static int done;
static int rxed_number;

/*a Static functions
 */
/*f tx_callback
 */
static void tx_callback( void *handle, t_eth_buffer *buffer )
{
    buffer->buffer_size = BUFFER_SIZE;
    ethernet_add_rx_buffer( buffer );
}

/*f rx_callback
 */
static void rx_callback( void *handle, t_eth_buffer *buffer, int rxed_byte_length )
{
    unsigned int *data;
    unsigned int expected_header, expected_length;
    unsigned char expected_data;
    int i;

    GIP_EXTBUS_DATA_WRITE(buffer);
    GIP_BLOCK_ALL();
    GIP_EXTBUS_DATA_WRITE(rxed_byte_length);
    GIP_BLOCK_ALL();

    data = (unsigned int *)buffer->data;
    GIP_EXTBUS_DATA_WRITE(data[0]);
    GIP_BLOCK_ALL();
    GIP_EXTBUS_DATA_WRITE(data[1]);
    switch (rxed_number)
    {
    case 0:
        expected_header = 0x00001234;
        expected_data = 0x01;
        expected_length = 64;
        break;
    case 1:
        expected_header = 0x00001234;
        expected_data = 0x39;
        expected_length = 64;
        break;
    case 2:
        expected_header = 0x00001234;
        expected_data = 0x71;
        expected_length = 64;
        done = 1;
        break;
    default:
        return;
    }
    if (rxed_byte_length!=expected_length+2)
    {
        failures = 0x100+rxed_number;
        done = 1;
        return;
    }
    if ( ((data[0]&0xffff)!=(expected_header>>16)) ||
         ((data[1]>>16)!=(expected_header&0xffff)) )
    {
        failures = 0x200+rxed_number;
        done = 1;
        return;
    }
    for (i=6; i<expected_length+2-8; i++)
    {
        if (buffer->data[i]!=expected_data)
        {
            failures = 0x300+rxed_number+(i<<16);
            done = 1;
            return;
        }
        expected_data = expected_data+1;
    }
    rxed_number++;
}


/*a Test entry point
 */
/*f test_entry_point
 */
extern int test_entry_point()
{
    int i;
    failures = 0;
    done = 0;
    rxed_number = 0;

    postbus_config( 16, 16, 0, 0 ); // use 16 for tx and 16 for rx, no sharing
    GIP_CLEAR_SEMAPHORES( -1 );

    ethernet_init( IO_A_SLOT_ETHERNET_0, 0, 2 );
    ethernet_set_tx_callback( tx_callback, NULL );
    ethernet_set_rx_callback( rx_callback, NULL );
    for (i=0; i<8; i++)
    {
        buffers[i].data = DRAM_START+BUFFER_SIZE*i;
        buffers[i].buffer_size = BUFFER_SIZE;
        ethernet_add_rx_buffer( &buffers[i] );
    }

    GIP_LED_OUTPUT_CFG_WRITE( 0xaaaa );
    GIP_BLOCK_ALL();
    GIP_EXTBUS_CONFIG_WRITE( 0x111 );
    GIP_BLOCK_ALL();
    GIP_EXTBUS_ADDRESS_WRITE( 0xc0000002 ); // Debug display address
    GIP_BLOCK_ALL();
    GIP_EXTBUS_DATA_WRITE( 0xffff );

    while (!done)
    {
        ethernet_poll();
    }
    return failures;
}
