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
extern module io_slot_head( clock int_clock,
                            input bit int_reset,

                            input bit    io_slot_cfg_write       "asserted if cfg_data should be written, if selected",
                            input bit[8] io_slot_cfg_data        "data for setting a slot configuration",
                            output bit[8] io_slot_cfg            "actual configuration for the slot",

                            output bit       io_slot_egr_cmd_ready   "bus of command emptys from all slots - they are filled asynchronously to requests",
                            output bit       io_slot_egr_data_req    "OR of data requests, masked by pending acknowledgements",
                            output t_io_tx_data_fifo_cmd   io_slot_egr_data_cmd    "data command from lowest number slot with an unmasked request",
                            input bit        io_slot_egr_data_ack    "asserted to acknowledge the current data request",
                            input bit[32]    io_slot_egr_data        "contains data for writes to the slots, registered here, valid 3 cycles after acknowledged request (acked req in cycle 0, sram req in cycle 1, sram data stored end cycle 2, this valid in cycle 3",
                            input bit        io_slot_egr_slot_select "indicates which slot the egress data is for, registered here; ",
                            input bit        io_slot_egr_cmd_write   "asserted if the data on the bus in this cycle is for the command side interface - if so, it will drive the not empty signal to the slot client",
                            input bit        io_slot_egr_data_write  "asserted if the data on the bus in this cycle is for the data side interface",

                            output bit[32]  io_slot_ingr_data        "muxed in slot head from clients, ANDed with a select from io_slot_ing_number",
                            output bit      io_slot_ingr_status_req  "OR of status requests, masked by pending acknowledgements",
                            output bit      io_slot_ingr_data_req    "OR of rx data requests, masked by pending acknowledgements, clear if status_req is asserted",
                            input bit       io_slot_ingr_ack         "acknowledge, valid in same clock as status_req and rxd_req",
                            input bit       io_slot_ingr_data_full   "for use by I/O",

                            output bit[32] tx_data_fifo_data,
                            input t_io_tx_data_fifo_cmd tx_data_fifo_cmd,
                            input bit tx_data_fifo_toggle,

                            output bit cmd_fifo_empty,
                            output bit[32] cmd_fifo_data,
                            input bit cmd_fifo_toggle,

                            input bit[32] rx_data_fifo_data,
                            input bit rx_data_fifo_toggle,
                            output bit rx_data_fifo_full,

                            input bit status_fifo_toggle,
                            input bit[32] status_fifo_data )
{
    timing to rising clock int_clock int_reset;
    timing comb input int_reset;

    timing to rising clock int_clock io_slot_cfg_write, io_slot_cfg_data;
    timing from rising clock int_clock io_slot_cfg;

    timing from rising clock int_clock io_slot_egr_cmd_ready, io_slot_egr_data_req, io_slot_egr_data_cmd;
    timing to rising clock int_clock io_slot_egr_data_ack;
    timing to rising clock int_clock io_slot_egr_data, io_slot_egr_slot_select, io_slot_egr_cmd_write, io_slot_egr_data_write;

    timing from rising clock int_clock io_slot_ingr_data, io_slot_ingr_status_req, io_slot_ingr_data_req;
    timing to rising clock int_clock io_slot_ingr_ack;

    timing from rising clock int_clock tx_data_fifo_data;
    timing to rising clock int_clock tx_data_fifo_cmd, tx_data_fifo_toggle; // cmd is actually combinatorial, but it MUST have many cycles so we can treat it as clocked for now

    timing from rising clock int_clock cmd_fifo_empty, cmd_fifo_data;
    timing to rising clock int_clock cmd_fifo_toggle;

    timing to rising clock int_clock rx_data_fifo_data, rx_data_fifo_toggle; // rx_data fifo data is comb but it must be stable over many clocks

    timing to rising clock int_clock status_fifo_data, status_fifo_toggle; // status fifo data is comb but it must be stable over many clocks

    timing comb input io_slot_ingr_data_full;
    timing comb output rx_data_fifo_full;

}

