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

                                  input bit[4] status_req "Requests from status command toggles",
                                  output bit[4] status_ack "Acknowledges to status toggles",
                                  input bit[4] rx_data_req "Requests from Rx data toggles",
                                  output bit[4] rx_data_ack "Acknowledges to rx data toggles",

                                  input bit postbus_req "Request from the postbus master",
                                  output bit postbus_ack "Acknowledge to the postbus master",

                                  input t_io_fifo_op postbus_fifo_op "Fifo op the postbus wants to do",
                                  input bit postbus_fifo_address_from_read_ptr "Asserted if the postbus wants to use the read address of a FIFO",
                                  input bit postbus_fifo_op_to_status  "Assert if the postbus wants to address the status FIFO",
                                  input bit[2] postbus_fifo_to_access "Fifo the postbus wants to address",
                                  input t_io_sram_data_op postbus_sram_data_op  "SRAM data operation the postbus wants",
                                  input t_io_sram_address_op postbus_sram_address_op "SRAM address source that the postbus wants",

                                  output t_io_fifo_op ingress_fifo_op "Operation to perform",
                                  output bit ingress_fifo_op_to_status "Asserted for status FIFO operations, deasserted for Rx Data FIFO operations",
                                  output bit ingress_fifo_address_from_read_ptr "Asserted if the FIFO address output should be for the read ptr of the specified FIFO, deasserted for write",
                                  output bit[2] ingress_fifo_to_access "Number of FIFO to access for operations",

                                  output t_io_sram_data_op ingress_sram_data_op,
                                  output t_io_sram_data_reg_op ingress_sram_data_reg_op,
                                  output t_io_sram_address_op ingress_sram_address_op

    )
{
    timing to rising clock int_clock int_reset;
    timing comb input int_reset;

    timing to rising clock int_clock status_req, rx_data_req, postbus_req;
    timing comb input status_req, rx_data_req, postbus_req;
    timing from rising clock int_clock status_ack, rx_data_ack, postbus_ack;
    timing comb output status_ack, rx_data_ack, postbus_ack;

    timing to rising clock int_clock postbus_fifo_op, postbus_fifo_address_from_read_ptr, postbus_fifo_op_to_status;
    timing to rising clock int_clock postbus_sram_data_op, postbus_sram_address_op;

    timing from rising clock int_clock ingress_fifo_op, ingress_fifo_op_to_status, ingress_fifo_address_from_read_ptr, ingress_fifo_to_access;
    timing from rising clock int_clock ingress_sram_data_op, ingress_sram_data_reg_op, ingress_sram_address_op;

}

