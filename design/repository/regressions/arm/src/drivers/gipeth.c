/* gipeth.c: a GIP net driver

*/

#include <stdlib.h>
#include <gip_support.h>
#include <postbus.h>
#include "../microkernel/microkernel.h"

#include "uart.h"

/*a Types
 */
#define GIPETH_BUFFER_LENGTH (2048)
#define GIPETH_BUFFER_LENGTH_INC_TIME (GIPETH_BUFFER_LENGTH+4)
#define NUM_TX_BUFFERS (4)
#define NUM_RX_BUFFERS (4)

/*t t_gipeth_buffer - this must match the assembler version
 */
typedef struct t_gipeth_buffer
{
    struct t_gipeth_buffer *next_in_list;
    unsigned int *data; // pointer to data start in the buffer - there needs to be one word before this pointer for the time the first block of data was received
    int length; // length in bytes - updated on receive by hardware thread, set on transmit by client
    int ready; // for rx indicates buffer is full and length set, client to read data and set done
    // for tx indicates buffer is full and length set, driver to transmit and set done
    int done; // for rx indicates buffer is available for driver to fill, client to clear upon handling of ready buffer
    // for tx indicates driver has transmitted
} t_gipeth_buffer;

/*t t_gipeth
 */
typedef struct t_gipeth
{
    t_gipeth_buffer *rx_next_buffer_to_reuse; // owned by hardware thread, indicates next buffer to receive into - not used by client
    unsigned int *rx_buffer_in_hand;    // owned by the hardware thread, buffer being received into (NULL if none)
    int rx_length_so_far;               // owned by the hardware thread, length received so far in buffer
    t_gipeth_buffer *tx_next_buffer_to_tx; // owned by the hardware thread, indicates next buffer to transmit from (if ready) - not used by client
    unsigned int *tx_buffer_in_hand;       // owned by the hardware thread, next data to be transmitted
    int tx_length;                         // owned by the hardware thread, indicates amount of data at tx_buffer_in_hand

    unsigned int regs[16]; // place to store registers when in hardware thread

    // rest is not accessible from hardware thread

    t_gipeth_buffer *rx_buffers; // circular linked chain of rx buffers
    t_gipeth_buffer *tx_buffers; // circular linked chain of tx buffers

    t_gipeth_buffer *rx_next_buffer_rxed; // owned by client, indicates next buffer to check for 'ready' in - not used by driver
    t_gipeth_buffer *tx_next_buffer_done; // owned by client, indicates next buffer to check for 'done' in - not used by driver

} t_gipeth;

/*a External functions - in asm version
 */
extern void gipeth_isr_asm( void );
extern void gipeth_hw_thread( void );

/*a Static variables
 */
static t_gipeth eth;

/*a Driver functions including hardware thread
 */
/*f gipdrv_ethernet_init
  we use semaphore 24 as virtual rx status ne
  we use semaphore 25 as virtual tx status ne
 */
extern void gipdrv_ethernet_init( void )
{
    int i;

    // GIP postbus RF split as 16 for rx/tx fifo 0, 16 for rx/tx fifo 1 (which does not yet exist due to Xilinx tool issues)
    GIP_POSTIF_CFG( 0, 0x00001000 ); // Rx fifo 0 config base 0, end 16, read 0, write 0 - can overlap with tx as we use one at a time
    GIP_POSTIF_CFG( 1, 0x00001000 ); // Tx fifo 0 config base 0, end 16, read 0, write 0

    // egress fifo sram (2kx32) split as ss txd 32, sscmd 32, etxcmd 32, other 32, etx 1920 (rest)
    GIP_POSTBUS_IO_FIFO_CFG( 1, 0, 1,    0,   31, 3 );  // ss txd - note status size and base are in entries, not words
    GIP_POSTBUS_IO_FIFO_CFG( 0, 0, 2, 32/2,   15, 3 );  // ss cmd
    GIP_POSTBUS_IO_FIFO_CFG( 0, 0, 0, 64/2,   15, 3 );  // etx cmd
    GIP_POSTBUS_IO_FIFO_CFG( 0, 0, 1, 96/2,   15, 3 );  // uart0 cmd
    GIP_POSTBUS_IO_FIFO_CFG( 0, 0, 3, 96/2,   15, 3 );  // unused
    GIP_POSTBUS_IO_FIFO_CFG( 1, 0, 2,   96,   31, 3 );  // unused
    GIP_POSTBUS_IO_FIFO_CFG( 1, 0, 3,   96,   31, 3 );  // unused
    GIP_POSTBUS_IO_FIFO_CFG( 1, 0, 0,  128, 1919, 1023 ); // etx data

    // ingress fifo sram (2kx32) split as ss rxd 32, ss status 32, etx status 32, erx status 32, erx 1920 (rest)
    GIP_POSTBUS_IO_FIFO_CFG( 1, 1, 1,    0,   31, 3 ); // ss rx data - note status size and base are in entries, not words
    GIP_POSTBUS_IO_FIFO_CFG( 0, 1, 3, 32/2,   15, 3 ); // ss status
    GIP_POSTBUS_IO_FIFO_CFG( 0, 1, 1, 64/2,   15, 3 ); // etx status
    GIP_POSTBUS_IO_FIFO_CFG( 0, 1, 0, 96/2,   15, 3 ); // erx status
    GIP_POSTBUS_IO_FIFO_CFG( 0, 1, 2, 96/2,   15, 3 ); // unused
    GIP_POSTBUS_IO_FIFO_CFG( 1, 1, 2,   96,   31, 3 ); // unused
    GIP_POSTBUS_IO_FIFO_CFG( 1, 1, 3,   96,   31, 3 ); // unused
    GIP_POSTBUS_IO_FIFO_CFG( 1, 1, 0,  128, 1919, 3 ); // erx data

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
// set event[3] to be etxd (watermark==0) send header_details[0] as command
// set header_details[0] to point to us
    unsigned int s;
    GIP_READ_AND_CLEAR_SEMAPHORES(s,0xf<<6);
    GIP_READ_AND_SET_SEMAPHORES(s,1<<27); // watermark is low to start with, so set our semaphore
    GIP_POST_TXD_0( GIP_POSTBUS_COMMAND( 0, 0, 0 ) ); // header details: route=0
    GIP_POST_TXC_0_IO_CMD( 0, 0, 0, 0x11 );
    NOP;NOP;NOP;NOP;NOP;NOP;
    GIP_POST_TXD_0( (0<<0) | (1<<2) | (1<<3) | (1<<4) | (0<<5) | (0<<postbus_command_source_io_cmd_op_start) | (0<<postbus_command_source_io_length_start) | (24<<postbus_command_target_gip_rx_semaphore_start) ); // ingress=1, cmd_status=1, fifo=0, empty_not_watermark=1, level=0, length=0, cmd_op=0 (read data length 0)
    GIP_POST_TXC_0_IO_CMD( 0, 0, 0, 0x14 );
    NOP;NOP;NOP;NOP;NOP;NOP;
    GIP_POST_TXD_0( (1<<0) | (1<<2) | (1<<3) | (1<<4) | (0<<5) | (0<<postbus_command_source_io_cmd_op_start) | (0<<postbus_command_source_io_length_start) | (25<<postbus_command_target_gip_rx_semaphore_start) ); // ingress=1, cmd_status=1, fifo=1, empty_not_watermark=1, level=0, length=0, cmd_op=0 (read data length 0)
    GIP_POST_TXC_0_IO_CMD( 0, 0, 0, 0x18 );
    NOP;NOP;NOP;NOP;NOP;NOP;
    GIP_POST_TXD_0( (0<<0) | (0<<2) | (0<<3) | (0<<4) | (0<<5) | (0<<postbus_command_source_io_cmd_op_start) | (0<<postbus_command_source_io_length_start) | (27<<postbus_command_target_gip_rx_semaphore_start) ); // ingress=0, cmd_status=0, fifo=0, empty_not_watermark=0, level=0, length=0, cmd_op=0 (read data length 0)
    GIP_POST_TXC_0_IO_CMD( 0, 0, 0, 0x1c );
    NOP;NOP;NOP;NOP;NOP;NOP;

    char *block;
    t_gipeth_buffer *buf, *last_buf;
//    block = kmalloc((sizeof(t_gipeth_buffer)+GIPETH_BUFFER_LENGTH_INC_TIME)*(NUM_TX_BUFFERS + NUM_RX_BUFFERS), GFP_KERNEL);
    block = (char *)0x80100000;

    last_buf = (t_gipeth_buffer *)(block + (sizeof(t_gipeth_buffer)+GIPETH_BUFFER_LENGTH_INC_TIME)*(NUM_TX_BUFFERS-1));
    for (i=0; i<NUM_TX_BUFFERS; i++)
    {
        buf = (t_gipeth_buffer *)block;
        buf->next_in_list = last_buf;
        buf->data = (unsigned int *)(block+sizeof(t_gipeth_buffer));
        buf->ready = 0; // initialized here, set by client, cleared on tx done
        buf->done = 1; // initialized here, cleared by us when ready, set on tx done, read by client
        last_buf = buf;
        block += (sizeof(t_gipeth_buffer)+GIPETH_BUFFER_LENGTH_INC_TIME);
    }
    
    eth.tx_next_buffer_to_tx = last_buf; // initialized here, owned by the hardware thread
    eth.tx_next_buffer_done = last_buf; // initialized here, owned by the client
    eth.tx_buffer_in_hand = NULL; // initialized here, owned by the hardware thread

    last_buf = (t_gipeth_buffer *)(block + (sizeof(t_gipeth_buffer)+GIPETH_BUFFER_LENGTH_INC_TIME)*(NUM_RX_BUFFERS-1));
    for (i=0; i<NUM_RX_BUFFERS; i++)
    {
        buf = (t_gipeth_buffer *)block;
        buf->next_in_list = last_buf;
        buf->data = ((unsigned int *)(block+sizeof(t_gipeth_buffer)))+1; // allow 1 word for time
        buf->ready = 0; // initialized here, set by hardware thread on rx done, read by client and cleared
        buf->done = 1; // initialized here, set by client on handling complete, cleared by driver when buffer taken by hardware thread
        last_buf = buf;
        block += (sizeof(t_gipeth_buffer)+GIPETH_BUFFER_LENGTH_INC_TIME);
    }
    eth.rx_buffer_in_hand = NULL; // initialized here, owned by hardware thread - takes rx_next_buffer_to_reuse if NULL and data comes in
    eth.rx_next_buffer_to_reuse = last_buf; // initialized here, owned by us - we reuse this buffer if it is done
    eth.rx_next_buffer_rxed = last_buf; // initialized here, owned by the client
    __asm__ volatile ( " .word 0xec00c1ce ; mov r0, %0 " : : "r" (&eth) );

    uart_tx_string_nl("here");
    MK_INT_DIS();
	MK_TRAP2( MK_TRAP_SET_IRQ_HANDLER, 1, (unsigned int)gipeth_isr_asm);
    GIP_ATOMIC_MAX_BLOCK();
    GIP_SET_THREAD(6,gipeth_hw_thread,0x71); // set thread 6 startup to be ARM, on semaphore 24, 25 or 26 set, and the entry point
    MK_INT_EN();
}

/*a Forward declarations
 */

/*a Static variables
 */
/*f gipeth_isr
  called when a transmit or a receive completes
  the transmit block ptrs and receive block ptrs should be checked to see if there is anything in the FIFOs
 */
extern void gipeth_isr (void)
{
//	printk ("Eth ISR\n");

    uart_init();
    while (1)
	{
        t_gipeth_buffer *buf;
        int len;

        // hardware thread takes buffer marked 'done', clears 'done', fills buffer, sets 'ready' and won't touch the buffer until 'done' is set
        // if ready is set then we handle the buffer clear ready, then set done and watch the next buffer
        buf = eth.rx_next_buffer_rxed;
        if (buf->ready)
        {
            len = buf->length;
            uart_tx_string("Pkt ");
            uart_tx_hex8(len);
            uart_tx_nl();
            if (len>0)
            {
            }
            eth.rx_next_buffer_rxed = buf->next_in_list;
            buf->ready = 0;
            buf->done = 1;
        }
        else
        {
            break;
        }
	}
    while (0) // data queued for transmit
    {
//        int i;
//        i = tx.next_buffer_done;
//        if (tx.buffers[i].done)
//        {
//            tx.next_buffer_done = (i+1)%NUM_TX_BUFFERS;
//        }
        // transmit to buffer i, mark it as ready
    }
}

/*f gipeth_xmit
  If there is a tx block available then copy it to the tx buffer and free
  Else add it to our tx queue
 */
extern int gipeth_xmit( unsigned int *data, int length )
{
    unsigned int s;
    t_gipeth_buffer *buf;
	
    buf = eth.tx_next_buffer_done;
    if (buf->done)
    {
        int i;
        if (length>GIPETH_BUFFER_LENGTH) length=GIPETH_BUFFER_LENGTH;
        for (i=0; (i<length/4); i++ )
        {
            buf->data[i] = data[i];
        }
        for (;i<16;i++)
        {
            buf->data[i] = 0;
        }
        buf->length = length<64?64:length;
        buf->done = 0;
        buf->ready = 1;
        eth.tx_next_buffer_done = buf->next_in_list;

        GIP_READ_AND_SET_SEMAPHORES( s, 1<<26 );        // hit hardware thread with softirq
    }
    else
    {
        // enqueue buffer
        return 1;
    }
	
	return 0;
}

/*f gipeth_setup
 */
extern void gipeth_setup(void)
{
    gipdrv_ethernet_init();
}
