/*a Copyright Gavin J Stark, 2004
 */

/*a Constants
 */
constant integer timestamp_length=24;
constant integer timestamp_sublength=timestamp_length/4;
constant integer log_cmd_fifo_size=6; // 64 entries for now
constant integer num_cmd_fifos=4;

/*a Module prototypes
 */
extern module io_cmd_fifo_timer( clock int_clock,
                                 input bit int_reset,

                                 input bit[2] timestamp_segment
                                 "Indicates which quarter of the timer counter value is being presented (00=top, 01=middle high, 10=middle low, 11=bottom); always follows the order [00 01 10 11 [11*]]",

                                 input bit[timestamp_sublength] timestamp
                                 "Portion (given by timestamp_segment) of the synchronous timer counter value, which only increments when timestamp_segment==11;",

                                 input bit clear_ready_indication
                                 "Asserted to indicate that a ready indication should be removed, and the state machine be idled; in fact always forces the state machine to idle",

                                 input bit load_fifo_data
                                 "Asserted to indicate that new FIFO data is ready and it should be latched",

                                 input bit[timestamp_length+1] fifo_timestamp
                                 "Data from the FIFO giving the time to wait until; if top bit set, return immediately",

                                 output bit time_reached
                                 "Asserted if the timer value has been reached"
    );
