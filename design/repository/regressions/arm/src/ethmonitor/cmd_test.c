/*a Includes
 */
#include "gip_support.h"
#include "../drivers/ethernet.h"
#include "ethernet.h"
#include "cmd.h"
#include <stdlib.h> // for NULL

/*a Defines
 */
#define DELAY(a) {int i;for (i=0; i<a; i++);}

/*a Types
 */
/*t t_data_fn, t_data_init_fn
 */
typedef void (t_data_init_fn)(unsigned int *ptr, unsigned int size );
typedef unsigned int (t_data_fn)(void);

/*t t_data_type
 */
typedef struct t_data_type
{
    t_data_init_fn *init_fn;
    t_data_fn *data_fn;
} t_data_type;

/*a Static functions
 */
/*f command_test_eth
 totaltx size_each max_in_queue
 */
static int outstanding_tx;
static void tst_rx_callback( void *handle, t_eth_buffer *buffer, int rxed_byte_length )
{
    buffer->buffer_size = 2048;
    ethernet_add_rx_buffer( buffer );
}
static void tst_tx_callback( void *handle, t_eth_buffer *buffer )
{
    uart_tx_string("V");
    outstanding_tx--;
}
static int command_test_eth( void *handle, int argc, unsigned int *args )
{
    t_eth_buffer buffers[16];
    int i;

    if (argc<3)
        return 1;
    if (args[2]>16)
        return 1;

    ethernet_set_tx_callback( tst_tx_callback, NULL );
    ethernet_set_rx_callback( tst_rx_callback, NULL );

    outstanding_tx = 0;
    for (i=0; i<args[0]; )
    {
        if (outstanding_tx<args[2])
        {
            uart_tx_string("^");
            buffers[i&0xf].buffer_size = args[1];
            buffers[i&0xf].data = (char *)command_test_eth;
            ethernet_tx_buffer( &buffers[i&0xf] );
            outstanding_tx++;
            i++;
        }
        mon_uart_poll();
        mon_ethernet_poll();
    }
    while (outstanding_tx>0)
    {
        mon_uart_poll();
        mon_ethernet_poll();
    }
    uart_tx_string_nl("Done");
    return 0;
}

/*a External variables
 */
/*v monitor_cmds_test
 */
extern const t_command monitor_cmds_test[];
const t_command monitor_cmds_test[] =
{
    {"testeth", command_test_eth},
    {(const char *)0, (t_command_fn *)0},
};

