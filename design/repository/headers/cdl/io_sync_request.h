/*a Copyright Gavin J Stark, 2004
 */

/*a To do
 */

/*a Constants
 */

/*a Types
 */

/*a Modules
 */
extern module io_sync_request(
    clock int_clock "main system clck",
    input bit int_reset "main system reset",
    input bit io_cmd_toggle "Toggle from IO clock domain that indicates a request",
    output bit io_arb_request "Actual request to arbiter",
    input bit arb_io_ack "Combinatorial acknowledge of request" )
{
    timing to rising clock int_clock int_reset;
    timing to rising clock int_clock io_cmd_toggle;
    timing to rising clock int_clock arb_io_ack;
    timing from rising clock int_clock io_arb_request;
}

