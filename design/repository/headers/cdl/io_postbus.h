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
                          output t_io_fifo_event_type egress_fifo_event_type,
                          output bit egress_fifo_address_from_read_ptr,

                          output t_io_sram_address_op egress_sram_address_op,
                          output t_io_sram_data_op egress_sram_data_op,

                          input bit egress_event_from_cmd "Asserted if event comes from a status FIFO",
                          input bit[2] egress_event_fifo "Fifo number last written to",
                          input t_io_fifo_event egress_event_empty "Indicates value of empty and if empty changed (and edge event)",
                          input t_io_fifo_event egress_event_watermark "Indicates value of watermark and if watermark changed (and edge event)",

                          output bit ingress_req,
                          input bit ingress_ack,

                          output t_io_fifo_op ingress_fifo_op,
                          output bit ingress_fifo_op_to_cmd_status,
                          output bit[2] ingress_fifo_to_access,
                          output t_io_fifo_event_type ingress_fifo_event_type,
                          output bit ingress_fifo_address_from_read_ptr,

                          output t_io_sram_address_op ingress_sram_address_op,
                          output t_io_sram_data_op ingress_sram_data_op,

                          input bit ingress_event_from_status "Asserted if event comes from a status FIFO",
                          input bit[2] ingress_event_fifo "Fifo number last written to",
                          input t_io_fifo_event ingress_event_empty "Indicates value of empty and if empty changed (and edge event)",
                          input t_io_fifo_event ingress_event_watermark "Indicates value of watermark and if watermark changed (and edge event)",

                          input bit[32] read_data,

                          output bit configuration_write,
                          output bit[5] write_address,
                          output bit[32] write_data
    )
{
    timing to rising clock int_clock int_reset;

    timing to rising clock int_clock postbus_src_ack, postbus_tgt_type, postbus_tgt_data;
    timing from rising clock int_clock postbus_src_type, postbus_src_data, postbus_tgt_ack;

    timing to rising clock int_clock egress_ack, ingress_ack;
    timing from rising clock int_clock egress_req, ingress_req;

    timing from rising clock int_clock egress_fifo_op, egress_fifo_op_to_cmd_status, egress_fifo_to_access, egress_fifo_event_type, egress_fifo_address_from_read_ptr;
    timing from rising clock int_clock egress_sram_address_op, egress_sram_data_op;
    timing from rising clock int_clock ingress_fifo_op, ingress_fifo_op_to_cmd_status, ingress_fifo_to_access, ingress_fifo_event_type, ingress_fifo_address_from_read_ptr;
    timing from rising clock int_clock ingress_sram_address_op, ingress_sram_data_op;

    timing to rising clock int_clock egress_event_from_cmd, egress_event_fifo, egress_event_empty, egress_event_watermark;
    timing to rising clock int_clock ingress_event_from_status, ingress_event_fifo, ingress_event_empty, ingress_event_watermark;

    timing to rising clock int_clock read_data;
    timing from rising clock int_clock configuration_write, write_address, write_data;

}
