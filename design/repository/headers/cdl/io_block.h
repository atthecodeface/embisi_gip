include "postbus.h"

/*a Modules
 */
extern module io_block( clock int_clock,
                        input bit int_reset,

                        input t_postbus_type postbus_tgt_type,
                        input t_postbus_data postbus_tgt_data,
                        output t_postbus_ack postbus_tgt_ack,

                        output t_postbus_type postbus_src_type,
                        output t_postbus_data postbus_src_data,
                        input t_postbus_ack postbus_src_ack,

                        output bit    io_slot_cfg_write "asserted if cfg_data should be written to cfg_slot",
                        output bit[8] io_slot_cfg_data  "data for setting a slot configuration",
                        output bit[2] io_slot_cfg_slot  "number of slot cfg data is destined for",

                        input bit[4]    io_slot_egr_cmd_ready  "bus of command emptys from all slots - they are filled asynchronously to requests",
                        input bit       io_slot_egr_data_req    "OR of data requests, masked by pending acknowledgements",
                        input t_io_tx_data_fifo_cmd   io_slot_egr_data_cmd    "data command from lowest number slot with an unmasked request",
                        input bit[2]    io_slot_egr_data_slot   "slot the data command is coming from",
                        output bit      io_slot_egr_data_ack "asserted to acknowledge the current data request",
                        output bit[32]  io_slot_egr_data "contains data for writes to the slots, registered here, valid 3 cycles after acknowledged request (acked req in cycle 0, sram req in cycle 1, sram data stored end cycle 2, this valid in cycle 3",
                        output bit[2]   io_slot_egr_slot "indicates which slot the egress data is for, registered here; ",
                        output bit      io_slot_egr_cmd_write  "asserted if the data on the bus in this cycle is for the command side interface - if so, it will drive the not empty signal to the slot client",
                        output bit      io_slot_egr_data_write "asserted if the data on the bus in this cycle is for the data side interface",
                        output bit[4]  io_slot_egr_data_empty  "for use by I/O",

                        input bit[32]  io_slot_ingr_data       "muxed in slot head from clients, ANDed with a select from io_slot_ing_number",
                        input bit      io_slot_ingr_status_req "OR of status requests, masked by pending acknowledgements",
                        input bit      io_slot_ingr_data_req    "OR of rx data requests, masked by pending acknowledgements, clear if status_req is asserted",
                        input bit[2]   io_slot_ingr_slot       "indicates which slot the status or rx data request is from",
                        output bit     io_slot_ingr_ack        "acknowledge, valid in same clock as status_req and data_req",
                        output bit[4]  io_slot_ingr_data_full  "for use by I/O",

                        input bit[3] analyzer_mux_control,
                        output bit[32] analyzer_signals
    )
{
    timing to rising clock int_clock int_reset;

    timing to rising clock int_clock postbus_tgt_type, postbus_tgt_data, postbus_src_ack;
    timing from rising clock int_clock postbus_src_type, postbus_src_data, postbus_tgt_ack;

    timing from rising clock int_clock io_slot_cfg_write, io_slot_cfg_data, io_slot_cfg_slot;

    timing to rising clock int_clock io_slot_egr_cmd_ready, io_slot_egr_data_req, io_slot_egr_data_cmd, io_slot_egr_data_slot;
    timing from rising clock int_clock io_slot_egr_data_ack, io_slot_egr_data_empty;
    timing from rising clock int_clock io_slot_egr_data, io_slot_egr_slot, io_slot_egr_cmd_write, io_slot_egr_data_write;

    timing to rising clock int_clock io_slot_ingr_data, io_slot_ingr_status_req, io_slot_ingr_data_req, io_slot_ingr_slot;
    timing from rising clock int_clock io_slot_ingr_ack, io_slot_ingr_data_full;

    timing from rising clock int_clock analyzer_signals;
}
