/*a Includes
 */
//#include <stdlib.h> // for NULL
#ifdef REGRESSION
#include "gip_support.h"
#include "postbus.h"
#else
#include <asm/gip_support.h>
#include <asm/postbus.h>
#endif

/*a Defines
 */

/*a Types
 */

/*a Static variables
 */

/*a External functions
 */
/*f sync_serial_wait_and_read_response
 */
extern void sync_serial_wait_and_read_response( int postbus_route, int slot, int *time, int *status, int *read_data )
{
    int ss_status_fifo_empty;
    unsigned int s;

    ss_status_fifo_empty = 1;
    while (ss_status_fifo_empty)
    {
        GIP_POST_TXD_0( (2<<postbus_command_source_io_cmd_op_start) | (1<<postbus_command_source_io_length_start) | (31<<postbus_command_target_gip_rx_semaphore_start) ); // cmd 2 is send overall status, length of 1, to route 0, signal semaphore 31 when its here
        GIP_POST_TXC_0_IO_CMD( postbus_route, 0, 0, 0xc|slot ); // do command '0x011ss', which is immediate command to ingress, status FIFO for slot 'ss', with route, length, op and rx semaphore given in the data
        do { GIP_READ_AND_CLEAR_SEMAPHORES( s, 1<<31 ); } while (s==0);
        GIP_POST_RXD_0(ss_status_fifo_empty); // get status of ss status fifo
        ss_status_fifo_empty = ((ss_status_fifo_empty & 0x80000000)!=0);
    }
    GIP_POST_TXD_0( (0<<postbus_command_source_io_cmd_op_start) | (2<<postbus_command_source_io_length_start) | (31<<postbus_command_target_gip_rx_semaphore_start) ); // cmd 0 is get data, length of 2, to route 0, signal semaphore 31 on completion
    GIP_POST_TXC_0_IO_CMD( postbus_route, 0, 0, 0xc|slot ); // do command '0x011ss', which is immediate command to ingress, status FIFO for slot 'ss', with route, length, op and rx semaphore given in the data
    do { GIP_READ_AND_CLEAR_SEMAPHORES( s, 1<<31 ); } while (s==0);
    GIP_POST_TXD_0( (0<<postbus_command_source_io_cmd_op_start) | (1<<postbus_command_source_io_length_start) | (31<<postbus_command_target_gip_rx_semaphore_start) ); // cmd 0 is get data, length of 1, to route 0, signal semaphore 31 on completion
    GIP_POST_TXC_0_IO_CMD( postbus_route, 0, 0, 0x4|slot ); // do command '0x001ss', which is immediate command to ingress, data FIFO for slot 'ss', with route, length, op and rx semaphore given in the data
    do { GIP_READ_AND_CLEAR_SEMAPHORES( s, 1<<31 ); } while (s==0);
    GIP_POST_RXD_0(*time);
    GIP_POST_RXD_0(*status);
    GIP_POST_RXD_0(*read_data);
}

/*f sync_serial_mdio_write
 */
extern void sync_serial_mdio_write( int postbus_route, int slot, int clock_divider, unsigned int value )
{
    GIP_POST_TXD_0( value );
    GIP_POST_TXC_0_IO_FIFO_DATA(postbus_route,0,0,slot,0,0);   // add to egress data fifo
    GIP_POST_TXD_0( 0x01000000 );                      // immediate command
    GIP_POST_TXD_0( 0x10700020 | (clock_divider<<8) ); // MDIO write = tx no tristate, clock/n, length 32 (write)
    GIP_POST_TXC_0_IO_FIFO_DATA(postbus_route,1,0,slot,0,1);   // add to egress cmd fifo
}

/*f sync_serial_mdio_read
 */
extern void sync_serial_mdio_read( int postbus_route, int slot, int clock_divider, unsigned int value )
{
    GIP_POST_TXD_0( value );
    GIP_POST_TXC_0_IO_FIFO_DATA(postbus_route,0,0,slot,0,0);   // add to egress data fifo
    GIP_POST_TXD_0( 0x01000000 );                      // immediate command
    GIP_POST_TXD_0( 0x107e0020 | (clock_divider<<8) ); // MDIO read = tristate after 14, clock/n, length 32 (write)
    GIP_POST_TXC_0_IO_FIFO_DATA(postbus_route,1,0,slot,0,1);   // add to egress cmd fifo
}

/*f sync_serial_init
  take sync_serial out of reset
 */
extern void sync_serial_init( int postbus_route, int slot )
{
    GIP_POST_TXD_0(1); // bit 0 is disable (hold in reset)
    GIP_POST_TXC_0_IO_CFG(postbus_route,0,slot);   // do I/O configuration
}
