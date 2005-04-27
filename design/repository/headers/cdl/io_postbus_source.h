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
/*t t_postbus_src_cmd_op
 */
typedef enum [2]
{
    post_src_cmd_op_read_data = 0,
    post_src_cmd_op_send_header_obsolete = 1,
    post_src_cmd_op_send_overall_status = 2,
} t_postbus_src_cmd_op;

/*a Modules
 */
extern module io_postbus_source ( clock int_clock "main system clck",
                                 input bit int_reset "Internal system reset",

                                  input bit post_src_cmd_req,
                                  output bit post_src_cmd_ack,
                                  input t_postbus_src_cmd_op post_src_cmd_op, // (read and send data, send single header, send overall status, FIFO is post_src_cmd/status, FIFO number)
                                  input bit [2]post_src_cmd_fifo, // FIFO number (0 to 3)
                                  input bit post_src_cmd_from_cmd_status, // 1 if from data FIFO
                                  input bit post_src_cmd_from_ingress, // 1 if from ingress FIFO
                                  input bit[5] post_src_cmd_length, // 0 (for 16) through 15 (for 15), if op is read and send data
                                  input bit[31] post_src_cmd_hdr_details, // route(7;1), semaphore(5;27), destination target information (2;8), source information, flow control (6;15)

                                  output t_postbus_type postbus_type,
                                 output t_postbus_data postbus_data,
                                 input t_postbus_ack postbus_ack,

                                 output t_io_fifo_op fifo_op,
                                 output bit fifo_op_to_cmd_status,
                                 output bit[2] fifo_to_access,
                                  output t_io_fifo_event_type fifo_event_type,
                                 output bit fifo_address_from_read_ptr,
                                 output t_io_sram_address_op sram_address_op,
                                 output t_io_sram_data_op sram_data_op,

                                 output bit egress_req,
                                 input bit egress_ack,

                                 output bit ingress_req,
                                 input bit ingress_ack,

                                 input bit[32] read_data
 )
{
    timing to rising clock int_clock int_reset;

    timing to rising clock int_clock post_src_cmd_req, post_src_cmd_op, post_src_cmd_fifo, post_src_cmd_from_cmd_status, post_src_cmd_from_ingress, post_src_cmd_length, post_src_cmd_hdr_details;
    timing from rising clock int_clock post_src_cmd_ack;

    timing from rising clock int_clock postbus_type, postbus_data;
    timing to rising clock int_clock postbus_ack;

    timing from rising clock int_clock egress_req, ingress_req;
    timing to rising clock int_clock egress_ack, ingress_ack;

    timing from rising clock int_clock fifo_op, fifo_op_to_cmd_status, fifo_to_access, fifo_event_type, fifo_address_from_read_ptr;
    timing from rising clock int_clock sram_address_op, sram_data_op;

    timing to rising clock int_clock read_data;

    timing comb input int_reset;
}

