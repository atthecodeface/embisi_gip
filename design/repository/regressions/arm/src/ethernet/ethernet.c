/*a Includes
 */
#include <stdlib.h> // for NULL
#include "gip_support.h"
#include "postbus.h"
#include "../common/wrapper.h"
#include "../drivers/ethernet.h"

/*a Defines
 */

/*a Ethernet handling functions
 */

/*a Test entry point
 */
static t_eth_buffer buffers[16];

/*f test_entry_point
 */
#define DRAM_START ((void *)(0x80010000))
#define BUFFER_SIZE (2048)
static void tx_callback( void *handle, t_eth_buffer *buffer )
{
    buffer->buffer_size = BUFFER_SIZE;
    ethernet_add_rx_buffer( buffer );
}
static void rx_callback( void *handle, t_eth_buffer *buffer, int rxed_byte_length )
{
    FLASH_CONFIG_WRITE(0xffff);
    FLASH_ADDRESS_WRITE(buffer);
    FLASH_DATA_WRITE(rxed_byte_length);
    buffer->buffer_size = rxed_byte_length;
    ethernet_tx_buffer( buffer );
}
extern int test_entry_point()
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
    while (1)
    {
        ethernet_poll();
    }

}
