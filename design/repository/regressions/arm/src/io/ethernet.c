/*a Includes
 */
#include <stdlib.h> // for NULL
#include "gip_support.h"
#include "postbus.h"
#include "../common/wrapper.h"
#include "ethernet.h"

/*a Defines
 */

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

/*t t_eth
 */
typedef struct t_eth
{
    t_eth_rx rx;
    t_eth_tx tx;
    int rx_slot;
    int tx_slot;
    int padding;
} t_eth;

/*a Static variables
 */
static t_eth eth;

/*a Static functions
 */
/*f ethernet_tx_start
  make this is the buffer_in_transit, and push it to the IO interface
 */
static void ethernet_tx_start( t_eth_buffer *buffer )
{
    int i, j;
    unsigned int s;
    eth.tx.buffer_in_transit = buffer;

    GIP_READ_AND_SET_SEMAPHORES( s, 1<<31 );
    for (i=j=0; i<(buffer->buffer_size+3+eth.padding)/4; i++) // i counts in words - note padding must be included here so it can be ignored by the interface
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
            GIP_POST_TXC_0_IO_FIFO_DATA( 0,j-1,31, eth.tx_slot,0,0 ); // add j words to etx fifo (egress data fifo 0)
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
        GIP_POST_TXC_0_IO_FIFO_DATA( 0,j-1,31, eth.tx_slot,0,0 ); // add j words to etx fifo (egress data fifo 0)
    }

    while (1)
    {
        unsigned int s;
        GIP_READ_AND_CLEAR_SEMAPHORES( s, 1<<31 );
        if ((s>>31)&1) break;
    }
    GIP_POST_TXD_0( 0x01000000 );
    GIP_POST_TXD_0( 0x21800000 | buffer->buffer_size ); // this is the true count of bytes in the packet
    GIP_POST_TXC_0_IO_FIFO_DATA( 0,1,31, eth.tx_slot,0,1 ); // add to egress cmd fifo 0, set semaphore 31 on completion

    while (1)
    {
        unsigned int s;
        GIP_READ_AND_CLEAR_SEMAPHORES( s, 1<<31 );
        if ((s>>31)&1) break;
    }
}

/*f Handle an ethernet transmit status
 */
static void handle_tx_status( unsigned int status )
{
    t_eth_buffer *buffer;

    /*b A buffer should have been in transit - grab it, start another tx if required, then call the callback
     */
    if (eth.tx.buffer_in_transit)
    {
        buffer = eth.tx.buffer_in_transit;
        eth.tx.buffer_in_transit = NULL;
        if (eth.tx.buffers_to_tx)
        {
            t_eth_buffer *buffer;
            buffer = eth.tx.buffers_to_tx;
            eth.tx.buffers_to_tx = buffer->next;
            ethernet_tx_start( buffer );
        }
        eth.tx.callback( eth.tx.callback_handle, buffer );
    }
    else
    {
        if (eth.tx.buffers_to_tx)
        {
            t_eth_buffer *buffer;
            buffer = eth.tx.buffers_to_tx;
            eth.tx.buffers_to_tx = buffer->next;
            ethernet_tx_start( buffer );
        }
    }
}

/*f Handle an ethernet receive status
  Packet length is bottom 16 bits
  Block length is 8;16
  Status is 3;24 : FCS ok (0), FCS bad (1), odd nybbles (2), block complete (3), overrun (4), framing error (5)
 */
static void handle_rx_status( unsigned int status )
{
    unsigned int size_so_far, block_size, reason;
    unsigned char *data;

    /*b Break out the status; request data from rx fifo
     */
    reason = (status>>24)&7;
    block_size = (status>>16)&0xff; // read this (number of bytes+3)/4 words from the data FIFO unless there was overflow!
    size_so_far = (status&0xffff);
    if (block_size>0)
    {
        GIP_POST_TXD_0( (0<<postbus_command_source_io_cmd_op_start) | (((block_size+3)/4)<<postbus_command_source_io_length_start) | (31<<postbus_command_target_gip_rx_semaphore_start) ); // cmd_op[2]=0, length=1 (get status), semaphore=0
        GIP_POST_TXC_0_IO_CMD( 0, 0, 0, 0x4|eth.rx_slot ); // eth rx d, ingr data fifo 0
    }

    /*b Now figure out what to do with the data while it comes
    // 0 => fcs ok; 1=>fcs bad; 2=> odd nybbles; 3=> complete block; 4=>fifo overrun!; 5=>framing error
    // we get 03400040, 03400080, 00120092 for a 146 byte packet
     */
    data = NULL; // default is to discard
    if (size_so_far<=64) // start of packet (size_so_far is from the status we just read)
    {
        if (eth.rx.current_data) // start of packet with an active packet already - just restart
        {
            data = eth.rx.current_data;
            eth.rx.length_received = size_so_far;
        }
        else
        {
            if (eth.rx.buffer_list) // start of packet with no buffer allocated, but we have buffers - so allocate one
            {
                data = eth.rx.current_data = eth.rx.buffer_list->data;
                eth.rx.current_buffer_size = eth.rx.buffer_list->buffer_size;
                eth.rx.length_received = size_so_far;
            }
            else // start of packet with no buffer allocated and no buffers available - we'll drop it
            {
                data = NULL;
            }
        }
    }
    else
    {
        if (eth.rx.current_data) // middle of packet rxed and we are in a packet
        {
            data = eth.rx.current_data+eth.rx.length_received;
            eth.rx.length_received += block_size;
            if (eth.rx.length_received>eth.rx.current_buffer_size) // check we have room for all the data - if not, drop packet
            {
                eth.rx.current_data = NULL;
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
        if (eth.rx.current_data) // if we were receiving properly then invoke callback and pop buffer
        {
            t_eth_buffer *buffer;
            buffer = eth.rx.buffer_list;
            eth.rx.buffer_list = eth.rx.buffer_list->next;
            eth.rx.callback( eth.rx.callback_handle, buffer, eth.rx.length_received );
            eth.rx.current_data = NULL;
        }
        break;
    case 1: // FCS bad
    case 2: // odd nybbles
    case 4: // Fifo overrun
    case 5: // Framing error
        eth.rx.current_data = NULL;
        break;
    case 3: // block received - well, we've done that :-)
        break;
    }
}

/*f Handle a non-empty ethernet tx status fifo
 */
static void handle_tx_status_fifo( void )
{
    unsigned int status, time;

    /*b Read the status and time
     */
    GIP_READ_AND_CLEAR_SEMAPHORES( status, 1<<31 );
    GIP_POST_TXD_0( (0<<postbus_command_source_io_cmd_op_start) | (2<<postbus_command_source_io_length_start) | (31<<postbus_command_target_gip_rx_semaphore_start) ); // cmd_op[2]=2, length=1 (get status), semaphore=0
    GIP_POST_TXC_0_IO_CMD( 0, 0, 0, 0xc|eth.tx_slot );

    // wait for the data to come back
    while (1)
    {
        unsigned int s;
        GIP_READ_AND_CLEAR_SEMAPHORES( s, 1<<31 );
        if ((s>>31)&1) break;
    }
    GIP_POST_RXD_0(time);
    GIP_POST_RXD_0(status);

    handle_tx_status( status );
}

/*f Handle a non-empty ethernet rx status fifo
 */
static void handle_rx_status_fifo( void )
{
    unsigned int status, time;

    /*b Read the status and time
     */
    GIP_READ_AND_CLEAR_SEMAPHORES( status, 1<<31 );
    GIP_POST_TXD_0( (0<<postbus_command_source_io_cmd_op_start) | (2<<postbus_command_source_io_length_start) | (31<<postbus_command_target_gip_rx_semaphore_start) ); // cmd_op[2]=2, length=1 (get status), semaphore=0
    GIP_POST_TXC_0_IO_CMD( 0, 0, 0, 0xc|eth.rx_slot );

    // wait for the data to come back
    while (1)
    {
        unsigned int s;
        GIP_READ_AND_CLEAR_SEMAPHORES( s, 1<<31 );
        if ((s>>31)&1) break;
    }
    GIP_POST_RXD_0(time);
    GIP_POST_RXD_0(status);
    handle_rx_status( status );
}

/*f Handle a non-empty shared ethernet status fifo
 */
static void handle_eth_status_fifo( void )
{
    unsigned int status, time;

    /*b Read the status and time
     */
    GIP_READ_AND_CLEAR_SEMAPHORES( status, 1<<31 );
    GIP_POST_TXD_0( (0<<postbus_command_source_io_cmd_op_start) | (2<<postbus_command_source_io_length_start) | (31<<postbus_command_target_gip_rx_semaphore_start) ); // cmd_op[2]=2, length=1 (get status), semaphore=0
    GIP_POST_TXC_0_IO_CMD( 0, 0, 0, 0xc|eth.rx_slot );

    // wait for the data to come back
    while (1)
    {
        unsigned int s;
        GIP_READ_AND_CLEAR_SEMAPHORES( s, 1<<31 );
        if ((s>>31)&1) break;
    }
    GIP_POST_RXD_0(time);
    GIP_POST_RXD_0(status);
    if (status&0x80000000)
    {
        handle_tx_status( status );
    }
    else
    {
        handle_rx_status( status );
    }
}

/*a External functions
 */
/*f ethernet_tx_buffer
 */
extern void ethernet_tx_buffer( t_eth_buffer *buffer )
{
    if (eth.tx.buffer_in_transit)
    {
        if (eth.tx.buffers_to_tx)
        {
            eth.tx.buffers_to_tx_tail->next = buffer;
        }
        else
        {
            eth.tx.buffers_to_tx = buffer;
        }
        eth.tx.buffers_to_tx_tail = buffer;
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
    eth.tx.callback = callback;
    eth.tx.callback_handle = handle;
}

/*f ethernet_set_rx_callback
 */
extern void ethernet_set_rx_callback( t_eth_rx_callback_fn callback, void *handle )
{
    eth.rx.callback = callback;
    eth.rx.callback_handle = handle;
}

/*f ethernet_add_rx_buffer
 */
extern void ethernet_add_rx_buffer( t_eth_buffer *buffer )
{
    if (eth.rx.buffer_list) // we may be receiving in to this one!
    {
        buffer->next = eth.rx.buffer_list->next;
        eth.rx.buffer_list->next = buffer;
    }
    else
    {
        buffer->next = NULL;
        eth.rx.buffer_list = buffer;
    }
}

/*f ethernet_poll
 */
extern void ethernet_poll( void )
{
    unsigned int s;
    GIP_READ_AND_CLEAR_SEMAPHORES( s, (1<<3) | (1<<31) );
    if (s&(1<<3))
    { // status ready somewhere; read both FIFO status' for now
        unsigned int rx_s, tx_s;
        if (eth.tx_slot != eth.rx_slot)
        {
            GIP_POST_TXD_0( (2<<postbus_command_source_io_cmd_op_start) | (1<<postbus_command_source_io_length_start) | (0<<postbus_command_target_gip_rx_semaphore_start) ); // cmd_op[2]=2, length=1 (get status), semaphore=0
            GIP_POST_TXC_0_IO_CMD( 0, 0, 0, 0xc|eth.rx_slot ); // fifo etc(4;27), length_m_one(5;10)=0, io_dest_type(2;8)=0 (cmd), route(7)=0, last(1)=0
        }
        GIP_POST_TXD_0( (2<<postbus_command_source_io_cmd_op_start) | (1<<postbus_command_source_io_length_start) | (31<<postbus_command_target_gip_rx_semaphore_start) ); // cmd_op[2]=2, length=1 (get status), semaphore=0
        GIP_POST_TXC_0_IO_CMD( 0, 0, 0, 0xc|eth.tx_slot ); // fifo etc(4;27), length_m_one(5;10)=0, io_dest_type(2;8)=0 (cmd), route(7)=0, last(1)=0
        while (1)
        {
            unsigned int s;
            GIP_READ_AND_CLEAR_SEMAPHORES( s, 1<<31 );
            if ((s>>31)&1) break;
        }
        if (eth.tx_slot != eth.rx_slot)
        {
            GIP_POST_RXD_0(rx_s); // get status of erx status fifo
            GIP_POST_RXD_0(tx_s);
            if ((rx_s&0x80000000)==0) // if not empty
            {
                handle_rx_status_fifo();
            }
            if ((tx_s&0x80000000)==0)
            {
                handle_tx_status_fifo();
            }
        }
        else
        {
            GIP_POST_RXD_0(rx_s); // get status of erx status fifo
            if ((rx_s&0x80000000)==0) // if not empty
            {
                handle_eth_status_fifo();
            }
        }
    }
}

/*f ethernet_init
 */
extern void ethernet_init( int slot, int endian_swap, int padding )
{
    eth.rx_slot = slot;
    eth.tx_slot = slot;
    eth.padding = padding;

    GIP_POST_TXD_0(1|(endian_swap<<1)|(padding<<2)); // bit 0 is disable (hold in reset)
    GIP_POST_TXC_0_IO_CFG(0,0,eth.rx_slot);   // do I/O configuration
    if (eth.tx_slot != eth.rx_slot)
    {
        GIP_POST_TXD_0(1|(endian_swap<<1)|(padding<<2)); // bit 0 is disable (hold in reset)
        GIP_POST_TXC_0_IO_CFG(0,0,eth.tx_slot);   // do I/O configuration
    }

    eth.rx.buffer_list = NULL;
    eth.rx.current_data = NULL;

    eth.tx.buffers_to_tx = NULL;
    eth.tx.buffer_in_transit = NULL;

    // set up events 2 and 3 to be virtual rxs fifo ne and virtual txs fifo ne, semaphore 3
    GIP_CLEAR_SEMAPHORES( (1<<3) );
    GIP_POST_TXD_0( GIP_POSTBUS_COMMAND( 0, 0, 0 ) ); // header details: route=0
    GIP_POST_TXC_0_IO_CMD( 0, 0, 0, 0x11 );
    NOP;NOP;NOP;NOP;NOP;NOP;
    GIP_POST_TXD_0( (eth.rx_slot<<0) | (1<<2) | (1<<3) | (1<<4) | (0<<5) | (0<<postbus_command_source_io_cmd_op_start) | (0<<postbus_command_source_io_length_start) | (3<<postbus_command_target_gip_rx_semaphore_start) ); // ingress=1, cmd_status=1, fifo=0, empty_not_watermark=1, level=0, length=0, cmd_op=0 (read data length 0)
    GIP_POST_TXC_0_IO_CMD( 0, 0, 0, 0x12 );
    NOP;NOP;NOP;NOP;NOP;NOP;
    if (eth.tx_slot != eth.rx_slot)
    {
        GIP_POST_TXD_0( (eth.tx_slot<<0) | (1<<2) | (1<<3) | (1<<4) | (0<<5) | (0<<postbus_command_source_io_cmd_op_start) | (0<<postbus_command_source_io_length_start) | (3<<postbus_command_target_gip_rx_semaphore_start) ); // ingress=1, cmd_status=1, fifo=1, empty_not_watermark=1, level=0, length=0, cmd_op=0 (read data length 0)
        GIP_POST_TXC_0_IO_CMD( 0, 0, 0, 0x14 );
        NOP;NOP;NOP;NOP;NOP;NOP;
    }

}


