/*a Copyright Gavin J Stark, 2004
 */

/*a To do
 */

/*a Constants
 */

/*a Includes
 */
include "io.h"

/*a Types
 */

/*a Modules
 */
extern module io_postbus( clock int_clock,
                          input bit int_reset,

                          output t_postbus_type postbus_src_type,
                          output t_postbus_data postbus_src_data,
                          input t_postbus_ack postbus_src_ack,

                          input t_postbus_type postbus_tgt_type,
                          input t_postbus_data postbus_tgt_data,
                          output t_postbus_ack postbus_tgt_ack,

                          output bit egress_req,
                          input bit egress_ack,

                          output t_io_fifo_op egress_fifo_op,
                          output bit egress_fifo_op_to_cmd_status,
                          output bit[2] egress_fifo_to_access,
                          output bit egress_fifo_address_from_read_ptr,

                          output t_io_sram_address_op egress_sram_address_op,
                          output t_io_sram_data_op egress_sram_data_op,

                          output bit ingress_req,
                          input bit ingress_ack,

                          output t_io_fifo_op ingress_fifo_op,
                          output bit ingress_fifo_op_to_cmd_status,
                          output bit[2] ingress_fifo_to_access,
                          output bit ingress_fifo_address_from_read_ptr,

                          output t_io_sram_address_op ingress_sram_address_op,
                          output t_io_sram_data_op ingress_sram_data_op,

                          input bit[32] read_data,

                          output bit configuration_write,
                          output bit[5] write_address,
                          output bit[32] write_data
    )
{
    timing comb input int_reset;

    timing to rising clock int_clock int_reset;

    timing to rising clock int_clock postbus_src_ack, postbus_tgt_type, postbus_tgt_data;
    timing from rising clock int_clock postbus_src_type, postbus_src_data, postbus_tgt_ack;

    timing to rising clock int_clock egress_ack, ingress_ack;
    timing from rising clock int_clock egress_req, ingress_req;

    timing from rising clock int_clock egress_fifo_op, egress_fifo_op_to_cmd_status, egress_fifo_to_access, egress_fifo_address_from_read_ptr;
    timing from rising clock int_clock egress_sram_address_op, egress_sram_data_op;
    timing from rising clock int_clock ingress_fifo_op, ingress_fifo_op_to_cmd_status, ingress_fifo_to_access, ingress_fifo_address_from_read_ptr;
    timing from rising clock int_clock ingress_sram_address_op, ingress_sram_data_op;

    timing to rising clock int_clock read_data;
    timing from rising clock int_clock configuration_write, write_address, write_data;

    timing comb input egress_ack, ingress_ack;
}
