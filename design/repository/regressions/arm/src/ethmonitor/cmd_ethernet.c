/*a Includes
 */
#include "gip_support.h"
#include "postbus.h"
#include "cmd.h"

/*a Defines
 */
#define DELAY(a) {int i;for (i=0; i<a; i++);}

/*a Types
 */

/*a Static functions
 */
/*f command_eth_tx
  .. <byte_length> [<data>*]
 */
static int command_eth_tx( void *handle, int argc, unsigned int *args )
{
    int i, j, k;
    int data_length, byte_length;
    unsigned int *data;
    unsigned int dummy;

    if (argc<1)
        return 1;
    byte_length = args[0];

    dummy = 0;
    if (argc>1)
    {
        data = &args[1];
        data_length = 4*(argc-1);
    }
    else
    {
        data=&dummy;
        data_length = 4;
    }

    for (i=j=k=0; i<(byte_length+3)/4; i++ )
    {
        GIP_POST_TXD_0( data[k/4] );
        k+=4;
        if (k>=data_length) k=0;
        j++;
        if (j==8)
        {
            // wait for txpending[0] to be clear?
            GIP_POST_TXC_0( (2<<8) | ((j-1)<<10) | (0<<27) ); // io_dest_type(2;8)=2, length_m_one = (j-1), io_dest=0 (etx data fifo)
            j = 0;
        }
    }
    if (j>0)
    {
        // wait for txpending[0] to be clear?
        GIP_POST_TXC_0( (2<<8) | ((j-1)<<10) | (0<<27) ); // io_dest_type(2;8)=2, length_m_one = (j-1), io_dest=0 (etx data fifo)
    }

    // wait for txpending[0] to be clear?
    GIP_POST_TXD_0( 0x01000000 );
    GIP_POST_TXD_0( 0x21800000 | byte_length );
    GIP_POST_TXC_0( (2<<8) | (1<<10) | (2<<27) ); // io_dest_type(2;8)=2, length_m_one = 1, io_dest=2 (etx cmd fifo)

    return 0;
}

/*f command_eth_poll
  polls semaphore 3; if set, get status of erx and etx, and handle each
 */
static int command_eth_poll( void *handle, int argc, unsigned int *args )
{
    unsigned int s;
    GIP_READ_AND_CLEAR_SEMAPHORES( s, 1<<3 );
    if (s&(1<<3))
    { // status ready somewhere;
    }
    return 0;
}

/*f command_eth_init
 */
static int command_eth_init( void *handle, int argc, unsigned int *args )
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
    GIP_POST_TXD_0( (0<<0) | (1<<2) | (1<<3) | (1<<4) | (0<<5) | (0<<postbus_command_source_io_cmd_op_start) | (1<<postbus_command_source_io_length_start) | (3<<postbus_command_target_gip_rx_semaphore_start) ); // ingress=1, cmd_status=1, fifo=0, empty_not_watermark=1, level=0, length=0, cmd_op=0 (read data length 0)
    GIP_POST_TXC_0_IO_CMD( 0, 0, 0, 0x14 );
    NOP;NOP;NOP;NOP;NOP;NOP;
    GIP_POST_TXD_0( (1<<0) | (1<<2) | (1<<3) | (1<<4) | (0<<5) | (0<<postbus_command_source_io_cmd_op_start) | (1<<postbus_command_source_io_length_start) | (3<<postbus_command_target_gip_rx_semaphore_start) ); // ingress=1, cmd_status=1, fifo=1, empty_not_watermark=1, level=0, length=0, cmd_op=0 (read data length 0)
    GIP_POST_TXC_0_IO_CMD( 0, 0, 0, 0x18 );
    NOP;NOP;NOP;NOP;NOP;NOP;

    return 0;
}

/*a External variables
 */
/*v monitor_cmds_ethernet
 */
extern const t_command monitor_cmds_ethernet[];
const t_command monitor_cmds_ethernet[] =
{
    {"etx", command_eth_tx},
    {"epoll", command_eth_poll},
    {"einit", command_eth_init},
    {(const char *)0, (t_command_fn *)0},
};

