/*a Copyright Gavin J Stark, 2004
 */

/*a Includes
 */
include "postbus.h"

/*a Constants
 */

/*a Types
 */

/*a Modules
 */
extern module postbus_register_demux( clock int_clock, input bit int_reset,
                               input t_postbus_type src_type, input t_postbus_data src_data, output t_postbus_ack src_ack,
                               input t_postbus_ack tgt_ack_0, output t_postbus_type tgt_type_0,
                               input t_postbus_ack tgt_ack_1, output t_postbus_type tgt_type_1,
                               input t_postbus_ack tgt_ack_2, output t_postbus_type tgt_type_2,
                               input t_postbus_ack tgt_ack_3, output t_postbus_type tgt_type_3,
                               output t_postbus_data tgt_data 
    )
{
    timing to rising clock int_clock int_reset;
    timing comb input int_reset;

    timing to rising clock int_clock src_type_0, src_type_1, src_type_2, src_type_3;
    timing to rising clock int_clock src_data_0, src_data_1, src_data_2, src_data_3;
    timing from rising clock int_clock src_ack_0, src_ack_1, src_ack_2, src_ack_3;

    timing to rising clock int_clock tgt_ack;
    timing from rising clock int_clock tgt_type, tgt_data;

}
