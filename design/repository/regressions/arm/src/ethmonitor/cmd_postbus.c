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
/*f command_io_fifo_status
  Display the IO block FIFO status for all 16 IO fifos
  corrupts header_details
 */
static int command_io_fifo_status( void *handle, int argc, unsigned int *args )
{
    unsigned int s;
    int i;

    for (i=0; i<16; i++)
    {
    // send postbus command dest_type 0 (channel provocation), write_address[5;dest], data write_data; clear bit 4 of write address for immediate cmd with event 0
    // fifo = write_address[2;2], cmd_status = write_address[1], ingress = write_address[0], op = data[2;cmd_op_start], length=data[5,tx_length_start], header_route_number=0, header_routes[0].details = write_Data[31;route start]
    // send overall status (op 2) (fifo 0, ingr/egr, status/control) gets EFWOU...size...base
        GIP_POST_TXD_0( (2<<postbus_command_source_io_cmd_op_start) | (1<<postbus_command_source_io_length_start) | (0<<postbus_command_target_gip_rx_semaphore_start) ); // cmd_op[2]=2, length=1 (get status), semaphore=0
        GIP_POST_TXC_0_IO_CMD( 0, 0, 0, i ); // fifo etc(4;27), length_m_one(5;10)=0, io_dest_type(2;8)=0 (cmd), route(7)=0, last(1)=0

        for (s=0; s<10; s++) // wait for status to be sent to us
        {
            NOP;
        }

        GIP_POST_STATUS_0(s);
        if (!(s&2))
            return 0;
        GIP_POST_RXD_0(s);
        cmd_result_hex8( handle, i );
        cmd_result_string( handle, " : " );
        cmd_result_hex8( handle, s );
        cmd_result_nl( handle );
    }
    return 0;
}

/*f command_io_fifo_get_status
  .. <fifo number - should be 3, 7, 11, 15>
  corrupts header_details
 */
static int command_io_fifo_get_status( void *handle, int argc, unsigned int *args )
{
    unsigned int s;
    int i;

    if (argc<1)
        return 1;

// send postbus command dest_type 0 (channel provocation), write_address[5;dest], data write_data; clear bit 4 of write address for immediate cmd with event 0
// fifo = write_address[2;2], cmd_status = write_address[1], ingress = write_address[0], op = data[2;cmd_op_start], length=data[5,tx_length_start], header_route_number=0, header_routes[0].details = write_Data[31;route start]
// get data from fifo (op 0) of specified length gets length data words
    GIP_POST_TXD_0( (0<<postbus_command_source_io_cmd_op_start) | (2<<postbus_command_source_io_length_start) | (0<<postbus_command_target_gip_rx_semaphore_start) ); // cmd_op[2]=2, length=1 (get status), semaphore=0
    GIP_POST_TXC_0_IO_CMD( 0, 0, 0, args[0] );

    for (i=0; i<10; i++) // wait for the status to be sent to us
    {
        NOP;
    }

    while (1)
    {
        GIP_POST_STATUS_0(s);
        if (!(s&2))
            return 0;
        GIP_POST_RXD_0(s);
        cmd_result_hex8( handle, s );
        cmd_result_nl( handle );
    }
    return 0;
}

/*f command_io_fifo_get_data
  .. <fifo number - 0 to 15> <length - 0 to 31>
  corrupts header_details
 */
static int command_io_fifo_get_data( void *handle, int argc, unsigned int *args )
{
    unsigned int s;
    int i;

    if (argc<2)
        return 1;

// send postbus command dest_type 0 (channel provocation), write_address[5;dest], data write_data; clear bit 4 of write address for immediate cmd with event 0
// fifo = write_address[2;2], cmd_status = write_address[1], ingress = write_address[0], op = data[2;cmd_op_start], length=data[5,tx_length_start], header_route_number=0, header_routes[0].details = write_Data[31;route start]
// get data from fifo (op 0) of specified length gets length data words
    GIP_POST_TXD_0( (0<<postbus_command_source_io_cmd_op_start) | (args[1]<<postbus_command_source_io_length_start) | (0<<postbus_command_target_gip_rx_semaphore_start) ); // cmd_op[2]=2, length=1 (get status), semaphore=0
    GIP_POST_TXC_0_IO_CMD( 0, 0, 0, args[0] ); // fifo etc(4;27), length_m_one(5;10)=0, io_dest_type(2;8)=0 (cmd), route(7)=0, last(1)=0

    for (i=0; i<10; i++)
    {
        NOP;
    }

    while (1)
    {
        GIP_POST_STATUS_0(s);
        if (!(s&2))
            return 0;
        GIP_POST_RXD_0(s);
        cmd_result_hex8( handle, s );
        cmd_result_nl( handle );
    }
    return 0;
}

/*f command_io_fifo_put_data
  .. <fifo number - 0 to 15> <data>+
 Can only put data in egress fifos; there is no path for ingress FIFOs
 */
static int command_io_fifo_put_data( void *handle, int argc, unsigned int *args )
{
    int i;

    if (argc<2)
        return 1;

// send postbus command dest_type 2 (add data to FIFO), write_address[5;dest], data write_data; clear bit 4 of write address for immediate cmd with event 0
// fifo = write_address[2;2], cmd_status = write_address[1], ingress = write_address[0], op = data[2;cmd_op_start], length=data[5,tx_length_start], header_route_number=0, header_routes[0].details = write_Data[31;route start]
// get data from fifo (op 0) of specified length gets length data words
    for (i=1; i<argc; i++)
    {
        GIP_POST_TXD_0( args[i] );
    }
    GIP_POST_TXC_0_IO_FIFO_DATA( 0, argc-2, 0, args[0], 0, 0 ); // fifo etc(4;27), length_m_one(5;10)=0, io_dest_type(2;8)=0 (cmd), route(7)=0, last(1)=0

    return 0;
}

/*f command_postbus_ptrs
  .. <command> <data>*
 */
static int command_postbus_ptrs( void *handle, int argc, unsigned int *args )
{
    unsigned int s;
    GIP_POST_RXCFG_0( s );
    cmd_result_hex8( handle, s );
    cmd_result_string( handle, " : ");
    GIP_POST_TXCFG_0( s );
    cmd_result_hex8( handle, s );
    cmd_result_nl( handle );
    return 0;
}

/*f command_postbus_tx
  .. <command> <data>*
  pbtx 0c000000 <data> for mdio read
  pbtx 4c000000 01000000 107e0a20

  pbtx 0c000000 <data> for mdio write
  pbtx 4c000000 01000000 10700a20

  write data is 58920040 01 01 10001 00100 10 0000 0000 0100 0000 - address is 0x11, reg address 00100, write data is bottom 16 bits
  read data  is 68900000 01 10 10001 00100 00 0000 0000 0000 0000 - address is 0x11, reg address 00100

pbtx 0c000000 ffffffff
pbtx 4c000100 01000000 10700a20
pbtx 0c000000 61900000
pbtx 4c000100 01000000 107e0a20
ifgd 5 1
ifgs d
ifgd 5 1
ifgs d

write register 00000 to 10Mbps FD only...
pbtx 0c000000 ffffffff
pbtx 4c000100 01000000 10700a20
pbtx 0c000000 51820100
pbtx 4c000100 01000000 10700a20
ifgd 5 1
ifgs d
ifgd 5 1
ifgs d

 */
static int command_postbus_tx( void *handle, int argc, unsigned int *args )
{
    int i;

    if (argc<1)
        return 1;

    for (i=1; i<argc; i++)
    {
        GIP_POST_TXD_0( args[i] );
    }
    GIP_POST_TXC_0( args[0] );

    return 0;
}

/*a External variables
 */
/*v monitor_cmds_postbus
 */
extern const t_command monitor_cmds_postbus[];
const t_command monitor_cmds_postbus[] =
{
    {"ifs", command_io_fifo_status},
    {"ifgs", command_io_fifo_get_status},
    {"ifgd", command_io_fifo_get_data},
    {"ifpd", command_io_fifo_put_data},
    {"pbp", command_postbus_ptrs},
    {"pbtx", command_postbus_tx},
    {(const char *)0, (t_command_fn *)0},
};

