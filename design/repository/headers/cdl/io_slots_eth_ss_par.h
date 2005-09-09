include "postbus.h"

/*a Modules
 */
extern module io_slots_eth_ss_par( clock int_clock,
                                   input bit int_reset,

                                   input bit    io_slot_cfg_write "asserted if cfg_data should be written to cfg_slot",
                                   input bit[8] io_slot_cfg_data  "data for setting a slot configuration",
                                   input bit[2] io_slot_cfg_slot  "number of slot cfg data is destined for",

                                   output bit[4]    io_slot_egr_cmd_ready  "bus of command emptys from all slots - they are filled asynchronously to requests",
                                   output bit       io_slot_egr_data_req    "OR of data requests, masked by pending acknowledgements",
                                   output t_io_tx_data_fifo_cmd   io_slot_egr_data_cmd    "data command from lowest number slot with an unmasked request",
                                   output bit[2]    io_slot_egr_data_slot   "slot the data command is coming from",
                                   input bit      io_slot_egr_data_ack "asserted to acknowledge the current data request",
                                   input bit[32]  io_slot_egr_data "contains data for writes to the slots, registered here, valid 3 cycles after acknowledged request (acked req in cycle 0, sram req in cycle 1, sram data stored end cycle 2, this valid in cycle 3",
                                   input bit[2]   io_slot_egr_slot "indicates which slot the egress data is for, registered here; ",
                                   input bit      io_slot_egr_cmd_write  "asserted if the data on the bus in this cycle is for the command side interface - if so, it will drive the not empty signal to the slot client",
                                   input bit      io_slot_egr_data_write "asserted if the data on the bus in this cycle is for the data side interface",
                                   input bit[4]  io_slot_egr_data_empty  "for use by I/O",

                                   output bit[32]  io_slot_ingr_data       "muxed in slot head from clients, ANDed with a select from io_slot_ing_number",
                                   output bit      io_slot_ingr_status_req "OR of status requests, masked by pending acknowledgements",
                                   output bit      io_slot_ingr_data_req    "OR of rx data requests, masked by pending acknowledgements, clear if status_req is asserted",
                                   output bit[2]   io_slot_ingr_slot       "indicates which slot the status or rx data request is from",
                                   input bit     io_slot_ingr_ack        "acknowledge, valid in same clock as status_req and data_req",
                                   input bit[4]  io_slot_ingr_data_full  "for use by I/O",

                                   clock erx_clock,
                                   input bit erx_reset,
                                   input bit erx_mii_dv, // goes high during the preamble OR at the latest at the start of the SFD
                                   input bit erx_mii_err, // if goes high with dv, then abort the receive; wait until dv goes low
                                   input bit[4] erx_mii_data,

                                   clock etx_clock,
                                   input bit etx_reset,
                                   output bit etx_mii_enable,
                                   output bit[4] etx_mii_data,
                                   input bit etx_mii_crs,
                                   input bit etx_mii_col,

                                   output bit uart0_txd,
                                   input bit uart0_txd_fc,
                                   input bit uart0_rxd,
                                   output bit uart0_rxd_fc,

                                   output bit[2] sscl,
                                   output bit[2] sscl_oe,
                                   output bit ssdo,
                                   output bit ssdo_oe,
                                   input bit[2] ssdi,
                                   output bit[8] sscs,

                                   clock par_a_clock,
                                   input bit[3] par_a_control_inputs,
                                   input bit[16] par_a_data_inputs,

                                   output bit[4] par_a_control_outputs,
                                   output bit[4] par_a_control_oes,
                                   output bit[16] par_a_data_outputs,
                                   output bit[3] par_a_data_output_width,
                                   output bit par_a_data_oe,

                                   clock par_b_clock,
                                   input bit[3] par_b_control_inputs,
                                   input bit[16] par_b_data_inputs,

                                   output bit[4] par_b_control_outputs,
                                   output bit[4] par_b_control_oes,
                                   output bit[16] par_b_data_outputs,
                                   output bit[3] par_b_data_output_width,
                                   output bit par_b_data_oe,

                                   input bit[4] analyzer_mux_control,
                                   output bit[32] analyzer_signals
    )
{
    timing to rising clock int_clock int_reset;

    timing to rising clock int_clock io_slot_cfg_write, io_slot_cfg_data, io_slot_cfg_slot;

    timing from rising clock int_clock io_slot_egr_cmd_ready, io_slot_egr_data_req, io_slot_egr_data_cmd, io_slot_egr_data_slot;
    timing to rising clock int_clock io_slot_egr_data_ack;
    timing to rising clock int_clock io_slot_egr_data, io_slot_egr_slot, io_slot_egr_cmd_write, io_slot_egr_data_write, io_slot_egr_data_empty;

    timing from rising clock int_clock io_slot_ingr_data, io_slot_ingr_status_req, io_slot_ingr_data_req, io_slot_ingr_slot;
    timing to rising clock int_clock io_slot_ingr_ack, io_slot_ingr_data_full;

    timing to rising clock erx_clock erx_reset;
    timing to rising clock erx_clock erx_mii_dv, erx_mii_err, erx_mii_data;
    timing from rising clock erx_clock uart0_rxd_fc; // this is a kludge; the tool does not insert a clock call unless outputs depend on it

    timing to rising clock etx_clock etx_reset;
    timing to rising clock etx_clock etx_mii_crs, etx_mii_col;
    timing from rising clock etx_clock etx_mii_enable, etx_mii_data;

    timing from rising clock int_clock sscl, sscl_oe, ssdo, ssdo_oe, sscs;
    timing to rising clock int_clock ssdi;

    timing to rising clock par_a_clock par_a_control_inputs, par_a_data_inputs;
    timing from rising clock par_a_clock par_a_control_outputs, par_a_control_oes, par_a_data_outputs, par_a_data_output_width, par_a_data_oe;

    timing to rising clock par_b_clock par_b_control_inputs, par_b_data_inputs;
    timing from rising clock par_b_clock par_b_control_outputs, par_b_control_oes, par_b_data_outputs, par_b_data_output_width, par_b_data_oe;

    timing from rising clock int_clock analyzer_signals;
    timing from rising clock etx_clock analyzer_signals;
    timing from rising clock erx_clock analyzer_signals;
}
