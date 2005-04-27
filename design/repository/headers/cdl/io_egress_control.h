/*a Copyright Gavin J Stark, 2004
 */

/*a To do
 */

/*a Constants
 */

/*a Includes
 */
include "io_cmd.h"
include "io_egress_fifos.h"

/*a Types
 */

/*a Modules
 */
extern module io_egress_control( clock int_clock "Internal system clock",
                                 input bit int_reset "Internal system reset",

                                 input bit[io_cmd_timestamp_length+2] io_timer "Timer for stamping received status and for outgoing command timing",

                                 output bit[4] cmd_valid "Valid indicator for registering command and its availability",
                                 input bit[4] cmd_available "Available from external data register and indicator to client",

                                 input bit[4] tx_data_req "Requests from Tx data toggles",
                                 output bit[4] tx_data_ack "Acknowledges to Tx data toggles",
                                 input t_io_tx_data_fifo_cmd tx_data_cmd "Command from chosen Tx data source",

                                 input bit postbus_req "Request from the postbus master",
                                 output bit postbus_ack "Acknowledge to the postbus master",

                                 input t_io_fifo_op postbus_fifo_op "Fifo op the postbus wants to do",
                                 input bit postbus_fifo_address_from_read_ptr "Asserted if the postbus wants to use the read address of a FIFO",
                                 input bit postbus_fifo_op_to_cmd  "Assert if the postbus wants to address the command FIFO",
                                 input bit[2] postbus_fifo_to_access "Fifo the postbus wants to address",
                                 input t_io_fifo_event_type postbus_fifo_event_type  "Type of event for the FIFO flags the postbus is generating",
                                 input t_io_sram_data_op postbus_sram_data_op  "SRAM data operation the postbus wants",
                                 input t_io_sram_address_op postbus_sram_address_op "SRAM address source that the postbus wants",

                                 output t_io_fifo_op egress_fifo_op "Operation to perform",
                                 output bit egress_fifo_op_to_cmd "Asserted for cmd FIFO operations, deasserted for Tx Data FIFO operations",
                                 output bit[2] egress_fifo_to_access "Number of FIFO to access for operations",
                                 output bit egress_fifo_address_from_read_ptr "Asserted if the FIFO address output should be for the read ptr of the specified FIFO, deasserted for write",
                                 output t_io_fifo_event_type egress_fifo_event_type "Type of flag event that should be flagged to the postbus system",
                                 input bit[4] egress_cmd_fifo_empty "Empty indications for command FIFOs, so their time values can be gathered",

                                 output t_io_sram_data_op egress_sram_data_op,
                                 output t_io_sram_data_reg_op egress_sram_data_reg_op,
                                 output t_io_sram_address_op egress_sram_address_op,
                                 input bit[32] egress_sram_read_data

    )
{
    timing to rising clock int_clock int_reset;

    timing to rising clock int_clock io_timer;

    timing from rising clock int_clock cmd_valid;
    timing to rising clock int_clock cmd_available;

    timing to rising clock int_clock tx_data_req, postbus_req;
    timing comb input tx_data_req, postbus_req;
    timing from rising clock int_clock tx_data_ack, postbus_ack;
    timing comb output tx_data_ack, postbus_ack;

    timing to rising clock int_clock tx_data_cmd;

    timing to rising clock int_clock postbus_fifo_op, postbus_fifo_address_from_read_ptr, postbus_fifo_op_to_cmd, postbus_fifo_to_access, postbus_fifo_event_type;
    timing to rising clock int_clock postbus_sram_data_op, postbus_sram_address_op;

    timing from rising clock int_clock egress_fifo_op, egress_fifo_op_to_cmd, egress_fifo_address_from_read_ptr, egress_fifo_to_access, egress_fifo_event_type;
    timing to rising clock int_clock egress_cmd_fifo_empty;

    timing from rising clock int_clock egress_sram_data_op, egress_sram_data_reg_op, egress_sram_address_op;
    timing to rising clock int_clock egress_sram_read_data;

    timing comb input int_reset;

}

