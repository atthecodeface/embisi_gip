/*a Copyright Gavin J Stark, 2004
 */

/*a To do
 */

/*a Constants
 */

/*a Includes
 */

/*a Types
 */

/*a Modules
 */
extern module io_postbus_event_manager ( clock int_clock "main system clck",
                                         input bit int_reset "Internal system reset",

                                         input bit target_req,
                                         output bit target_ack,
                                         input bit[5] target_write_address,
                                         input bit[32] target_write_data,

                                         input bit egress_event_from_cmd "Asserted if event comes from a status FIFO",
                                         input bit[2] egress_event_fifo "Fifo number last written to",
                                         input t_io_fifo_event egress_event_empty "Indicates value of empty and if empty changed (and edge event)",
                                         input t_io_fifo_event egress_event_watermark "Indicates value of watermark and if watermark changed (and edge event)",

                                         input bit ingress_event_from_status "Asserted if event comes from a status FIFO",
                                         input bit[2] ingress_event_fifo "Fifo number last written to",
                                         input t_io_fifo_event ingress_event_empty "Indicates value of empty and if empty changed (and edge event)",
                                         input t_io_fifo_event ingress_event_watermark "Indicates value of watermark and if watermark changed (and edge event)",

                                         output bit post_src_cmd_req,
                                         input bit post_src_cmd_ack,
                                         output t_postbus_src_cmd_op post_src_cmd_op, // (read and send data, send single header, send overall status, FIFO is post_src_cmd/status, FIFO number)
                                         output bit [2]post_src_cmd_fifo, // FIFO number (0 to 3)
                                         output bit post_src_cmd_from_cmd_status, // 1 if from data FIFO
                                         output bit post_src_cmd_from_ingress, // 1 if from ingress FIFO
                                         output bit[5] post_src_cmd_length, // 0 (for 16) through 15 (for 15), if op is read and send data
                                         output bit[31] post_src_cmd_hdr_details // route(7;1), semaphore(5;27), destination target information (2;8), source information, flow control (6;15)
 )
{
    timing to rising clock int_clock int_reset;

    timing to rising clock int_clock target_req, target_write_data, target_write_address;
    timing from rising clock int_clock target_ack;

    timing to rising clock int_clock egress_event_from_cmd, egress_event_fifo, egress_event_empty, egress_event_watermark;
    timing to rising clock int_clock ingress_event_from_status, ingress_event_fifo, ingress_event_empty, ingress_event_watermark;

    timing from rising clock int_clock post_src_cmd_req, post_src_cmd_op, post_src_cmd_fifo, post_src_cmd_from_cmd_status, post_src_cmd_from_ingress, post_src_cmd_length, post_src_cmd_hdr_details;
    timing to rising clock int_clock post_src_cmd_ack;

}

