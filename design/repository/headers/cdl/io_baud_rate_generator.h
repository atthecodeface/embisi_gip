/*a Copyright Gavin J Stark, 2004
 */

/*a To do
 */

/*a Constants
 */
constant integer io_baud_rate_divider_size=15;

/*a Types
 */

/*a Modules
 */
extern module io_baud_rate_generator( clock io_clock           "Clock for the module",
                                      input bit io_reset       "Reset for the module",
                                      input bit counter_enable "Clock enable for clocking the counter, for additional external divide-by",
                                      input bit counter_reset  "Counter reset, so that a known phase may be achieved for the counter; makes the subtract value be written to the counter, but only if counter_enable is asserted",
                                      output bit baud_clock_enable "Clock enable output, dependent solely on the counter value, not masked by the input enable; should be ORed with the input counter reset, the result ANDed with input counter enable",
                                      input bit set_clock_config "Asserted on a clock edge (independent of counter enable) to configure the add/subtract pair, also writes the subtract value to the counter",
                                      input bit[io_baud_rate_divider_size] config_baud_addition_value "Amount to be added to the counter on each enabled clock edge when the counter value is negative",
                                      input bit[io_baud_rate_divider_size] config_baud_subtraction_value "Amount to be subtracted from the counter on each enabled clock edge when the counter value is non-negative" )
{
    timing to rising clock io_clock io_reset;

    timing to rising clock io_clock   counter_enable, counter_reset;
    timing from rising clock io_clock baud_clock_enable;

    timing to rising clock io_clock set_clock_config;
    timing to rising clock io_clock config_baud_addition_value, config_baud_addition_value;

    timing comb input io_reset; // For async reset
}
