/*a Copyright Gavin J Stark, 2004
 */

/*a To do
 */

/*a Includes
 */
include "io.h"
include "io_egress_control_cmd_fsm.h"
include "io_cmd_fifo_timer.h"

/*a Types
 */

/*a Module
 */
extern module io_egress_control_cmd_fsm( clock int_clock "Internal system clock",
                                  input bit int_reset "Internal system reset",
                                  input bit[2] timestamp_segment,
                                  input bit[io_cmd_timestamp_sublength] timestamp,
                                  input bit[io_cmd_timestamp_length] fifo_timestamp_data,

                                  input bit cmd_fifo_empty,
                                  output bit cmd_timer_req,
                                  output bit cmd_data_req,
                                  input bit cmd_ack,
                                  output bit cmd_valid,
                                  input bit cmd_available )
{
    timing to rising clock int_clock int_reset;

    timing to rising clock int_clock timestamp_segment, timestamp, fifo_timestamp_data;

    timing to rising clock int_clock cmd_fifo_empty, cmd_ack, cmd_available;
    timing from rising clock int_clock cmd_timer_req, cmd_data_req;

    timing comb input int_reset;
}
