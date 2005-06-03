/*m apb_target_uart
 */
extern module apb_target_uart( clock apb_clock "Internal system clock",
                         input bit int_reset "Internal reset",

                         input bit[3] apb_paddr,
                         input bit apb_penable,
                         input bit apb_pselect,
                         input bit[32] apb_pwdata,
                         input bit apb_prnw,
                        output bit[32] apb_prdata,

                         output bit cmd_fifo_empty,
                         output bit[32] cmd_fifo_data,
                         input bit cmd_fifo_toggle,

                         output bit status_fifo_full,
                         input bit status_fifo_toggle,
                         input bit[32] status_fifo_data )
{
    timing to rising clock apb_clock int_reset;

    timing to rising clock apb_clock apb_pselect, apb_penable, apb_paddr, apb_pwdata, apb_prnw;
    timing comb input apb_pselect, apb_penable, apb_paddr, apb_prnw;
    timing from rising clock apb_clock apb_prdata;
    timing comb output apb_prdata;

    timing to rising clock apb_clock cmd_fifo_toggle;
    timing from rising clock apb_clock cmd_fifo_empty, cmd_fifo_data;
    timing to rising clock apb_clock status_fifo_toggle, status_fifo_data;
    timing from rising clock apb_clock status_fifo_full;
}

