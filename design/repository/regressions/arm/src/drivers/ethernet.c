/*a Includes
 */
#include <stdlib.h> // for NULL
#include "gip_support.h"
#include "postbus.h"
#include "ethernet.h"

/*a Defines
 */
#define DELAY(a) {int i;for (i=0; i<a; i++);}

/*a Types
 */
/*t t_eth_rx
 */
typedef struct t_eth_rx
{
    t_eth_buffer *buffer_list;
    t_eth_rx_callback_fn *callback; // on receive buffer called with two handles; global, and per-buffer
    void *callback_handle;
    int length_received;
    unsigned char *current_data;        // copied from first buffer when packet starts
    unsigned int current_buffer_size;  // copied from first buffer when packet starts
} t_eth_rx;

/*t t_eth_tx
 */
typedef struct t_eth_tx
{
    t_eth_buffer *buffers_to_tx; // list of buffers to go (after this one, if one in transit)
    t_eth_buffer *buffers_to_tx_tail; // tail of list of buffers to go (so we send in order)
    t_eth_buffer *buffer_in_transit; // buffer given to the ethernet tx
    t_eth_tx_callback_fn *callback; // when transmit complete this is called
    void *callback_handle;
} t_eth_tx;

/*a Static variables
 */
static t_eth_rx rx;
static t_eth_tx tx;

/*a Static functions
 */
/*f ethernet_tx_start
  make this is the buffer_in_transit, and push it to the IO interface
 */
static void ethernet_tx_start( t_eth_buffer *buffer )
{
    int i, j;
    unsigned int s;
    tx.buffer_in_transit = buffer;

    GIP_READ_AND_SET_SEMAPHORES( s, 1<<31 );
    for (i=j=0; i<(buffer->buffer_size+3)/4; i++) // i counts in words
    {
        GIP_POST_TXD_0( ((unsigned int *)(buffer->data))[i] );
        j++;
        if (j==8)
        {
            while (1)
            {
                unsigned int s;
                GIP_READ_AND_CLEAR_SEMAPHORES( s, 1<<31 );
                if ((s>>31)&1) break;
            }
            GIP_POST_TXC_0_IO_FIFO_DATA( 0,j-1,31, 0,0,0 ); // add j words to etx fifo (egress data fifo 0)
            j = 0;
        }
    }
    if (j>0)
    {
        while (1)
        {
            unsigned int s;
            GIP_READ_AND_CLEAR_SEMAPHORES( s, 1<<31 );
            if ((s>>31)&1) break;
        }
        GIP_POST_TXC_0_IO_FIFO_DATA( 0,j-1,31, 0,0,0 ); // add j words to etx fifo (egress data fifo 0)
    }

    while (1)
    {
        unsigned int s;
        GIP_READ_AND_CLEAR_SEMAPHORES( s, 1<<31 );
        if ((s>>31)&1) break;
    }
    GIP_POST_TXD_0( 0x01000000 );
    GIP_POST_TXD_0( 0x21800000 | buffer->buffer_size );
    GIP_POST_TXC_0_IO_FIFO_DATA( 0,1,31, 0,0,1 ); // add to egress cmd fifo 0, set semaphore 31 on completion

    while (1)
    {
        unsigned int s;
        GIP_READ_AND_CLEAR_SEMAPHORES( s, 1<<31 );
        if ((s>>31)&1) break;
    }
}

/*f Handle a non-empty ethernet tx status fifo
 */
static void handle_tx_status_fifo( void )
{
    unsigned int status, time;
    t_eth_buffer *buffer;

    /*b Read the status and time
     */
    GIP_READ_AND_CLEAR_SEMAPHORES( status, 1<<31 );
    GIP_POST_TXD_0( (0<<postbus_command_source_io_cmd_op_start) | (2<<postbus_command_source_io_length_start) | (31<<postbus_command_target_gip_rx_semaphore_start) ); // cmd_op[2]=2, length=1 (get status), semaphore=0
    GIP_POST_TXC_0_IO_CMD( 0, 0, 0, 0xd );

    // wait for the data to come back
    while (1)
    {
        unsigned int s;
        GIP_READ_AND_CLEAR_SEMAPHORES( s, 1<<31 );
        if ((s>>31)&1) break;
    }
    GIP_POST_RXD_0(time);
    GIP_POST_RXD_0(status);

    /*b A buffer should have been in transit - grab it, start another tx if required, then call the callback
     */
    if (tx.buffer_in_transit)
    {
        buffer = tx.buffer_in_transit;
        tx.buffer_in_transit = NULL;
        if (tx.buffers_to_tx)
        {
            t_eth_buffer *buffer;
            buffer = tx.buffers_to_tx;
            tx.buffers_to_tx = buffer->next;
            ethernet_tx_start( buffer );
        }
        tx.callback( tx.callback_handle, buffer );
    }
    else
    {
        if (tx.buffers_to_tx)
        {
            t_eth_buffer *buffer;
            buffer = tx.buffers_to_tx;
            tx.buffers_to_tx = buffer->next;
            ethernet_tx_start( buffer );
        }
    }
}

/*f Handle a non-empty ethernet rx status fifo
  Packet length is bottom 16 bits
  Block length is 8;16
  Status is 3;24 : FCS ok (0), FCS bad (1), odd nybbles (2), block complete (3), overrun (4), framing error (5)
 */
static void handle_rx_status_fifo( void )
{
    unsigned int status, time, size_so_far, block_size, reason;
    unsigned char *data;

    /*b Read the status and time
     */
    GIP_READ_AND_CLEAR_SEMAPHORES( status, 1<<31 );
    GIP_POST_TXD_0( (0<<postbus_command_source_io_cmd_op_start) | (2<<postbus_command_source_io_length_start) | (31<<postbus_command_target_gip_rx_semaphore_start) ); // cmd_op[2]=2, length=1 (get status), semaphore=0
    GIP_POST_TXC_0_IO_CMD( 0, 0, 0, 0xc );

    // wait for the data to come back
    while (1)
    {
        unsigned int s;
        GIP_READ_AND_CLEAR_SEMAPHORES( s, 1<<31 );
        if ((s>>31)&1) break;
    }
    GIP_POST_RXD_0(time);
    GIP_POST_RXD_0(status);

    /*b Break out the status; request data from rx fifo
     */
    uart_tx_string("rxs ");
    uart_tx_hex8(status);
    uart_tx_string(":");
    uart_tx_hex8(rx.current_data);
    uart_tx_string(":");
    uart_tx_hex8(rx.length_received);
    uart_tx_nl();
    reason = (status>>24)&7;
    block_size = (status>>16)&0xff; // read this (number of bytes+3)/4 words from the data FIFO unless there was overflow!
    size_so_far = (status&0xffff);
    if (block_size>0)
    {
        GIP_POST_TXD_0( (0<<postbus_command_source_io_cmd_op_start) | (((block_size+3)/4)<<postbus_command_source_io_length_start) | (31<<postbus_command_target_gip_rx_semaphore_start) ); // cmd_op[2]=0, length=1 (get status), semaphore=0
        GIP_POST_TXC_0_IO_CMD( 0, 0, 0, 0x4 ); // eth rx d, ingr data fifo 0
    }

    /*b Now figure out what to do with the data while it comes
    // 0 => fcs ok; 1=>fcs bad; 2=> odd nybbles; 3=> complete block; 4=>fifo overrun!; 5=>framing error
    // we get 03400040, 03400080, 00120092 for a 146 byte packet
     */
    data = NULL; // default is to discard
    if (size_so_far<=64) // start of packet (size_so_far is from the status we just read)
    {
        if (rx.current_data) // start of packet with an active packet already - just restart
        {
            data = rx.current_data;
            rx.length_received = size_so_far;
        }
        else
        {
            if (rx.buffer_list) // start of packet with no buffer allocated, but we have buffers - so allocate one
            {
                data = rx.current_data = rx.buffer_list->data;
                rx.current_buffer_size = rx.buffer_list->buffer_size;
                rx.length_received = size_so_far;
            }
            else // start of packet with no buffer allocated and no buffers available - we'll drop it
            {
                data = NULL;
            }
        }
    }
    else
    {
        if (rx.current_data) // middle of packet rxed and we are in a packet
        {
            data = rx.current_data+rx.length_received;
            rx.length_received += block_size;
            if (rx.length_received>rx.current_buffer_size) // check we have room for all the data - if not, drop packet
            {
                rx.current_data = NULL;
                data = NULL;
            }
        }
        else // middle of packet rxed and we are not receiving one - so drop it
        {
            data = NULL;
        }
    }

    /*b Wait for any new data and discard or copy to buffer
     */
    if (block_size>0)
    {
        while (1)
        {
            unsigned int s;
            GIP_READ_AND_CLEAR_SEMAPHORES( s, 1<<31 );
            if ((s>>31)&1) break;
        }
        if (data)
        {
            int i;
            unsigned int s;
            for (i=0; i<((block_size+3)/4); i++)
            {
                GIP_POST_RXD_0(s);
                ((unsigned int *)data)[i] = s;
            }
        }
        else
        {
            int i;
            unsigned int s;
            for (i=0; i<((block_size+3)/4); i++)
            {
                GIP_POST_RXD_0(s);
            }
        }
    }

    /*b Invoke callback if required
     */
    switch (reason)
    {
    case 0: // FCS ok
        uart_tx_string("fcs_ok ");
        uart_tx_hex8(rx.current_data);
        uart_tx_string(":");
        uart_tx_hex8(rx.length_received);
        uart_tx_nl();
        if (rx.current_data) // if we were receiving properly then invoke callback and pop buffer
        {
            t_eth_buffer *buffer;
            buffer = rx.buffer_list;
            rx.buffer_list = rx.buffer_list->next;
            rx.callback( rx.callback_handle, buffer, rx.length_received );
            rx.current_data = NULL;
        }
        break;
    case 1: // FCS bad
    case 2: // odd nybbles
    case 4: // Fifo overrun
    case 5: // Framing error
        rx.current_data = NULL;
        break;
    case 3: // block received - well, we've done that :-)
        break;
    }
}

/*a External functions
 */
/*f ethernet_tx_buffer
 */
extern void ethernet_tx_buffer( t_eth_buffer *buffer )
{
    if (tx.buffer_in_transit)
    {
        if (tx.buffers_to_tx)
        {
            tx.buffers_to_tx_tail->next = buffer;
        }
        else
        {
            tx.buffers_to_tx = buffer;
            tx.buffers_to_tx_tail = buffer;
        }
        buffer->next = NULL;
    }
    else
    {
        ethernet_tx_start( buffer );
    }
}

/*f ethernet_set_tx_callback
 */
extern void ethernet_set_tx_callback( t_eth_tx_callback_fn callback, void *handle )
{
    tx.callback = callback;
    tx.callback_handle = handle;
}

/*f ethernet_set_rx_callback
 */
extern void ethernet_set_rx_callback( t_eth_rx_callback_fn callback, void *handle )
{
    rx.callback = callback;
    rx.callback_handle = handle;
}

/*f ethernet_add_rx_buffer
 */
extern void ethernet_add_rx_buffer( t_eth_buffer *buffer )
{
    if (rx.buffer_list) // we may be receiving in to this one!
    {
        uart_tx_string("addrxbuf ");
        uart_tx_hex8(buffer);
        uart_tx_string(":");
        uart_tx_hex8(rx.buffer_list);
        uart_tx_nl();
        buffer->next = rx.buffer_list->next;
        rx.buffer_list->next = buffer;
    }
    else
    {
        uart_tx_string("addrxbuf ");
        uart_tx_hex8(buffer);
        uart_tx_nl();
        buffer->next = NULL;
        rx.buffer_list = buffer;
    }
}

/*f ethernet_poll
 */
extern void ethernet_poll( void )
{
    unsigned int s;
    GIP_READ_AND_CLEAR_SEMAPHORES( s, 1<<3 );
    if (s&(1<<3))
    { // status ready somewhere; read both FIFO status' for now
        unsigned int rx_s, tx_s;
        GIP_POST_TXD_0( (2<<postbus_command_source_io_cmd_op_start) | (1<<postbus_command_source_io_length_start) | (0<<postbus_command_target_gip_rx_semaphore_start) ); // cmd_op[2]=2, length=1 (get status), semaphore=0
        GIP_POST_TXC_0_IO_CMD( 0, 0, 0, 0xc ); // fifo etc(4;27), length_m_one(5;10)=0, io_dest_type(2;8)=0 (cmd), route(7)=0, last(1)=0
        GIP_POST_TXD_0( (2<<postbus_command_source_io_cmd_op_start) | (1<<postbus_command_source_io_length_start) | (0<<postbus_command_target_gip_rx_semaphore_start) ); // cmd_op[2]=2, length=1 (get status), semaphore=0
        GIP_POST_TXC_0_IO_CMD( 0, 0, 0, 0xd ); // fifo etc(4;27), length_m_one(5;10)=0, io_dest_type(2;8)=0 (cmd), route(7)=0, last(1)=0
        NOP;NOP;NOP;NOP;NOP;NOP; // cmd and data are RF writes, so delay, and give a chance for the data to get back
        NOP;NOP;NOP;NOP;NOP;NOP; // cmd and data are RF writes, so delay, and give a chance for the data to get back
        GIP_POST_RXD_0(rx_s); // get status of erx status fifo
        GIP_POST_RXD_0(tx_s);
        uart_tx_string("statuses ");
        uart_tx_hex8(rx_s );
        uart_tx_string(":");
        uart_tx_hex8(tx_s );
        uart_tx_nl();
        if ((rx_s&0x80000000)==0) // if not empty
        {
            handle_rx_status_fifo();
        }
        if ((tx_s&0x80000000)==0)
        {
            handle_tx_status_fifo();
        }
    }
}

/*f ethernet_init
 */
extern void ethernet_init( void )
{
    // GIP postbus RF split as 16 for rx/tx fifo 0, 16 for rx/tx fifo 1 (which does not yet exist due to Xilinx tool issues)
    GIP_POSTIF_CFG( 0, 0x00001000 ); // Rx fifo 0 config base 0, end 16, read 0, write 0 - can overlap with tx as we use one at a time
    GIP_POSTIF_CFG( 1, 0x00001000 ); // Tx fifo 0 config base 0, end 16, read 0, write 0

    // egress fifo sram (2kx32) split as ss txd 32, sscmd 32, etxcmd 32, other 32, etx 1920 (rest)
    GIP_POSTBUS_IO_FIFO_CFG( 1, 0, 1,    0,   31, 3 );  // ss txd
    GIP_POSTBUS_IO_FIFO_CFG( 0, 0, 2,   32,   63, 3 );  // ss cmd
    GIP_POSTBUS_IO_FIFO_CFG( 0, 0, 0,   64,   95, 3 );  // etx cmd
    GIP_POSTBUS_IO_FIFO_CFG( 0, 0, 1,   96,  127, 3 );  // uart0 cmd
    GIP_POSTBUS_IO_FIFO_CFG( 0, 0, 3,   96,  127, 3 );  // unused
    GIP_POSTBUS_IO_FIFO_CFG( 1, 0, 2,   96,  127, 3 );  // unused
    GIP_POSTBUS_IO_FIFO_CFG( 1, 0, 3,   96,  127, 3 );  // unused
    GIP_POSTBUS_IO_FIFO_CFG( 1, 0, 0, 1920, 2047, 3 ); // etx data

    // ingress fifo sram (2kx32) split as ss rxd 32, ss status 32, etx status 32, erx status 32, erx 1920 (rest)
    GIP_POSTBUS_IO_FIFO_CFG( 1, 1, 1,    0,   31, 3 ); // ss rx data
    GIP_POSTBUS_IO_FIFO_CFG( 0, 1, 3,   32,   63, 3 ); // ss status
    GIP_POSTBUS_IO_FIFO_CFG( 0, 1, 1,   64,   95, 3 ); // etx status
    GIP_POSTBUS_IO_FIFO_CFG( 0, 1, 0,   96,  127, 3 ); // erx status
    GIP_POSTBUS_IO_FIFO_CFG( 0, 1, 2,   96,  127, 3 ); // unused
    GIP_POSTBUS_IO_FIFO_CFG( 1, 1, 2,   96,  127, 3 ); // unused
    GIP_POSTBUS_IO_FIFO_CFG( 1, 1, 3,   96,  127, 3 ); // unused
    GIP_POSTBUS_IO_FIFO_CFG( 1, 1, 0,  128, 2047, 3 ); // erx data

    // set auto-neg to 10FD only
    // clock MDC 32 times with MDIO high, then send command
    GIP_POST_TXD_0( 0xffffffff );
    GIP_POST_TXC_0_IO_FIFO_DATA(0,0,0,1,0,0); // add to egress data fifo 1
    GIP_POST_TXD_0( 0x01000000 );
    GIP_POST_TXD_0( 0x10701820 ); // MDIO tx no tristate, clock/24, length 32 (write)
    GIP_POST_TXC_0_IO_FIFO_DATA(0,1,0,2,0,1); // add to egress cmd fifo 2

    GIP_POST_TXD_0( 0x58920040 );
    GIP_POST_TXC_0_IO_FIFO_DATA(0,0,0,1,0,0); // add to egress data fifo 1
    GIP_POST_TXD_0( 0x01000000 );
    GIP_POST_TXD_0( 0x10701820 ); // MDIO tx no tristate, clock/24, length 32 (write)
    GIP_POST_TXC_0_IO_FIFO_DATA(0,1,0,2,0,1); // add to egress cmd fifo 2

    // restart auto-neg
    // clock MDC 32 times with MDIO high, then send command
    GIP_POST_TXD_0( 0xffffffff );
    GIP_POST_TXC_0_IO_FIFO_DATA(0,0,0,1,0,0); // add to egress data fifo 1
    GIP_POST_TXD_0( 0x01000000 );
    GIP_POST_TXD_0( 0x10701820 ); // MDIO tx no tristate, clock/24, length 32 (write)
    GIP_POST_TXC_0_IO_FIFO_DATA(0,1,0,2,0,1); // add to egress cmd fifo 2

    GIP_POST_TXD_0( 0x58821340 );
    GIP_POST_TXC_0_IO_FIFO_DATA(0,0,0,1,0,0); // add to egress data fifo 1
    GIP_POST_TXD_0( 0x01000000 );
    GIP_POST_TXD_0( 0x10701820 ); // MDIO tx no tristate, clock/24, length 32 (write)
    GIP_POST_TXC_0_IO_FIFO_DATA(0,1,0,2,0,1); // add to egress cmd fifo 2

// header_details[0] is all we have at the moment (in event occurrence)
// set event[1] to be erxs (empty==0) send header_details[0] as command
// set event[2] to be etxs (empty==0) send header_details[0] as command
// set header_details[0] to point to us
    GIP_POST_TXD_0( GIP_POSTBUS_COMMAND( 0, 0, 0 ) ); // header details: route=0
    GIP_POST_TXC_0_IO_CMD( 0, 0, 0, 0x11 );
    NOP;NOP;NOP;NOP;NOP;NOP;
    GIP_POST_TXD_0( (0<<0) | (1<<2) | (1<<3) | (1<<4) | (0<<5) | (0<<postbus_command_source_io_cmd_op_start) | (0<<postbus_command_source_io_length_start) | (3<<postbus_command_target_gip_rx_semaphore_start) ); // ingress=1, cmd_status=1, fifo=0, empty_not_watermark=1, level=0, length=0, cmd_op=0 (read data length 0)
    GIP_POST_TXC_0_IO_CMD( 0, 0, 0, 0x14 );
    NOP;NOP;NOP;NOP;NOP;NOP;
    GIP_POST_TXD_0( (1<<0) | (1<<2) | (1<<3) | (1<<4) | (0<<5) | (0<<postbus_command_source_io_cmd_op_start) | (0<<postbus_command_source_io_length_start) | (3<<postbus_command_target_gip_rx_semaphore_start) ); // ingress=1, cmd_status=1, fifo=1, empty_not_watermark=1, level=0, length=0, cmd_op=0 (read data length 0)
    GIP_POST_TXC_0_IO_CMD( 0, 0, 0, 0x18 );
    NOP;NOP;NOP;NOP;NOP;NOP;

    rx.buffer_list = NULL;
    rx.current_data = NULL;

    tx.buffers_to_tx = NULL;
    tx.buffer_in_transit = NULL;
}


