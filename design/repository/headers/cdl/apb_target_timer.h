/*m apb_target_timer
 */
extern module apb_target_timer( clock apb_clock "Internal system clock",
                                input bit int_reset "Internal reset",

                                input bit[3] apb_paddr,
                                input bit apb_penable,
                                input bit apb_pselect,
                                input bit[32] apb_pwdata,
                                input bit apb_prnw,
                                output bit[32] apb_prdata,
                                output bit apb_pwait,

                                output bit[3]timer_equalled
    )
{
    timing to rising clock apb_clock int_reset;

    timing to rising clock apb_clock apb_pselect, apb_penable, apb_paddr, apb_pwdata, apb_prnw;
    timing comb input apb_pselect, apb_penable, apb_paddr, apb_prnw;
    timing from rising clock apb_clock apb_pwait;
    timing from rising clock apb_clock apb_prdata;

    timing from rising clock apb_clock timer_equalled;
}

