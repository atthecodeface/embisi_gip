/*a Copyright Gavin J Stark, 2004
 */

/*a To do
 */

/*a Constants
 */

/*a Includes
 */
include "postbus.h"

/*a Types
 */

/*a Modules
 */
extern module io_postbus_target( clock int_clock "main system clck",
                                 input bit int_reset "Internal system reset",

                                 input t_postbus_type postbus_type,
                                 input t_postbus_data postbus_data,
                                 output t_postbus_ack postbus_ack,

                                 output t_io_fifo_op fifo_op,
                                 output bit fifo_op_to_cmd_status,
                                 output bit[2] fifo_to_access,
                                 output bit fifo_address_from_read_ptr,
                                 output t_io_fifo_event_type fifo_event_type,
                                 output t_io_sram_address_op sram_address_op,
                                 output t_io_sram_data_op sram_data_op,

                                 output bit egress_req,
                                 input bit egress_ack,

                                 output bit ingress_req,
                                 input bit ingress_ack,

                                 output bit command_req,
                                 input bit command_ack,

                                 output bit configuration_write,

                                 output bit[5] write_address,
                                 output bit[32] write_data
 )
{
    timing to rising clock int_clock int_reset;

    timing to rising clock int_clock postbus_type, postbus_data;
    timing from rising clock int_clock postbus_ack;

    timing from rising clock int_clock egress_req, ingress_req, command_req;
    timing to rising clock int_clock egress_ack, ingress_ack, command_ack;

    timing from rising clock int_clock fifo_op, fifo_op_to_cmd_status, fifo_to_access, fifo_event_type, fifo_address_from_read_ptr;
    timing from rising clock int_clock sram_address_op, sram_data_op;

    timing from rising clock int_clock configuration_write, write_address, write_data;

    timing comb input int_reset;
}

