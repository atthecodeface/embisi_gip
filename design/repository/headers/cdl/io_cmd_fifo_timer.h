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
extern module io_cmd_fifo_timer( clock int_clock,
                          input bit int_reset,

                          input bit[2] timestamp_segment "Indicates which quarter of the timer counter value is being presented (00=top, 01=middle high, 10=middle low, 11=bottom); always follows the order [00 01 10 11 [11*]]",
                          input bit[io_cmd_timestamp_sublength] timestamp "Portion (given by timestamp_segment) of the synchronous timer counter value, which only increments when timestamp_segment==11;",

                          input bit clear_ready_indication "Asserted to indicate that a ready indication should be removed, and the state machine be idled; in fact always forces the state machine to idle",
                          input bit load_fifo_data "Asserted to indicate that new FIFO data is ready and it should be latched",
                          input bit[io_cmd_timestamp_length+1] fifo_timestamp "Data from the FIFO giving the time to wait until; if top bit set, return immediately",

                          output bit time_reached "Asserted if the timer value has been reached"
 )
{
    timing to rising clock int_clock timestamp_segment, timestamp;
    timing to rising clock int_clock clear_ready_indication, load_fifo_data, fifo_timestamp;
    timing from rising clock int_clock time_reached;

    timing comb input int_reset;
}
