/*a Copyright Gavin J Stark, 2004
 */

/*a Includes
 */
include "postbus.h"

/*a Types
 */

/*a Modules
 */
extern module postbus_uart_target( clock int_clock,
                                   input bit int_reset,
                                   input t_postbus_type postbus_type,
                                   input t_postbus_data postbus_data,
                                   output t_postbus_ack postbus_ack,
                                   output bit txd )
{
    timing comb input int_reset;
    timing to rising clock int_clock int_reset, postbus_type, postbus_data;
    timing from rising clock int_clock postbus_ack, txd;
}
