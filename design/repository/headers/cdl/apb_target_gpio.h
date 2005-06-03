/*a Constants
 */
constant integer apb_gpio_input_status = 0;
constant integer apb_gpio_input_reg = 1;
constant integer apb_gpio_output_reg=2;

/*a Modules
 */
/*m apb_target_gpio
 */
extern module apb_target_gpio( clock apb_clock "Internal system clock",
                                input bit int_reset "Internal reset",

                                input bit[3] apb_paddr,
                                input bit apb_penable,
                                input bit apb_pselect,
                                input bit[32] apb_pwdata,
                                input bit apb_prnw,
                                output bit[32] apb_prdata,

                                output bit[16]gpio_output,
                                output bit[16]gpio_output_enable,
                                input bit[4]gpio_input,
                                output bit gpio_input_event
    )
{
    timing to rising clock apb_clock int_reset;

    timing to rising clock apb_clock apb_pselect, apb_penable, apb_paddr, apb_pwdata, apb_prnw;
    timing comb input apb_pselect, apb_penable, apb_paddr, apb_prnw;
    timing from rising clock apb_clock apb_prdata;
    timing comb output apb_prdata;

    timing from rising clock apb_clock gpio_output_enable, gpio_output;
    timing to rising clock apb_clock gpio_input;
    timing from rising clock apb_clock gpio_input_event;
}

