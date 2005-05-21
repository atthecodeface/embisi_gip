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

                        input bit[2] analyzer_mux_control,
                        output bit[32] analyzer_signals
    )
{
    timing to rising clock int_clock int_reset;

    timing to rising clock int_clock postbus_tgt_type, postbus_tgt_data, postbus_src_ack;
    timing from rising clock int_clock postbus_src_type, postbus_src_data, postbus_tgt_ack;

    timing to rising clock erx_clock erx_reset;
    timing to rising clock erx_clock erx_mii_dv, erx_mii_err, erx_mii_data;
    timing from rising clock erx_clock uart0_rxd_fc; // this is a kludge; the tool does not insert a clock call unless outputs depend on it

    timing to rising clock etx_clock etx_reset;
    timing to rising clock etx_clock etx_mii_crs, etx_mii_col;
    timing from rising clock etx_clock etx_mii_enable, etx_mii_data;

    timing from rising clock int_clock sscl, sscl_oe, ssdo, ssdo_oe, sscs;
    timing to rising clock int_clock ssdi;

    timing from rising clock int_clock analyzer_signals;
    timing from rising clock etx_clock analyzer_signals;
    timing from rising clock erx_clock analyzer_signals;
}
