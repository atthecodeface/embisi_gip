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
extern module io_ingress_control( clock int_clock "Internal system clock",
                                  input bit int_reset "Internal system reset",

                                  input bit status_req "Request from status - only one of these two will be asserted",
                                  input bit rx_data_req "Request from rx data - only one of these two will be asserted",
                                  input bit[2] status_rx_data_fifo "Number of FIFO the request is for",
                                  output bit status_rx_data_will_be_acked "Indicates that if a request is made then it would be acked - this acts as the ack",

                                  input bit postbus_req "Request from the postbus master",
                                  output bit postbus_ack "Acknowledge to the postbus master",

                                  input t_io_fifo_op postbus_fifo_op "Fifo op the postbus wants to do",
                                  input bit postbus_fifo_address_from_read_ptr "Asserted if the postbus wants to use the read address of a FIFO",
                                  input bit postbus_fifo_op_to_status  "Assert if the postbus wants to address the status FIFO",
                                  input bit[2] postbus_fifo_to_access "Fifo the postbus wants to address",
                                  input t_io_fifo_event_type postbus_fifo_event_type  "Type of event for the FIFO flags the postbus is generating",
                                  input t_io_sram_data_op postbus_sram_data_op  "SRAM data operation the postbus wants",
                                  input t_io_sram_address_op postbus_sram_address_op "SRAM address source that the postbus wants",

                                  output t_io_fifo_op ingress_fifo_op "Operation to perform",
                                  output bit ingress_fifo_op_to_status "Asserted for status FIFO operations, deasserted for Rx Data FIFO operations",
                                  output bit[2] ingress_fifo_to_access "Number of FIFO to access for operations",
                                  output bit ingress_fifo_address_from_read_ptr "Asserted if the FIFO address output should be for the read ptr of the specified FIFO, deasserted for write",
                                  output t_io_fifo_event_type ingress_fifo_event_type "Type of flag event that should be flagged to the postbus system",

                                  output t_io_sram_data_op ingress_sram_data_op,
                                  output bit ingress_sram_data_reg_hold,
                                  output t_io_sram_address_op ingress_sram_address_op

    )
{
    timing to rising clock int_clock int_reset;
    timing comb input int_reset;

    timing to rising clock int_clock status_req, rx_data_req, status_rx_data_fifo, postbus_req;
    timing comb input status_req, rx_data_req, status_rx_data_fifo, postbus_req;
    timing from rising clock int_clock status_rx_data_will_be_acked, postbus_ack;
    timing comb output postbus_ack, status_rx_data_will_be_acked;

    timing to rising clock int_clock postbus_fifo_op, postbus_fifo_address_from_read_ptr, postbus_fifo_op_to_status, postbus_fifo_to_access, postbus_fifo_event_type;
    timing to rising clock int_clock postbus_sram_data_op, postbus_sram_address_op;

    timing from rising clock int_clock ingress_fifo_op, ingress_fifo_op_to_status, ingress_fifo_address_from_read_ptr, ingress_fifo_to_access, ingress_fifo_event_type;
    timing from rising clock int_clock ingress_sram_data_op, ingress_sram_data_reg_hold, ingress_sram_address_op;

}

