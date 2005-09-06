/* parallel_2.c - handle the soft parallel interface

*/

//#define USE_CIRC_BUFFERS

#define GIP_INCLUDE_FROM_C
#ifdef REGRESSION
#include <stdlib.h>
#include <gip_support.h>
#include <postbus.h>
#include <io_parallel.h>
#include "parallel_2.h"
#else // REGRESSION

#endif // REGRESSION

/*a Defines
 */

/*a Types
 */

/*a Static variables
 */

/*a Parallel assist functions
 */
/*f parallel_config
 */
extern void parallel_config( unsigned int *action_array, int *action_number, t_io_parallel_cmd_type type, int data )
{
    action_array[ (*action_number)++ ] = (type<<28) | (data&~0xf0000000);
}

/*f parallel_config_fsm
  Configures the states AND the conditions
  Need to do_all at least once with states from 0 to n to set all the conditions initially
  Thereafter to just adjust states, do not use do_all
 */
extern void parallel_config_fsm( unsigned int *action_array, int *action_number, int nstates, const t_par_state *states, const char *conds, const char *ctl_out, int do_all )
{
    int i;
    unsigned int rf0, rf1;
    // rf0 is TTTTTTTT1000000000NNcccccccccccc
    // rf1 is TTTTTTTT........ssssAAAA11111111

    for (i=0; i<16; i++)
    {
        rf0 = 0;
        rf1 = 0; 
        if (i<nstates)
        {
            unsigned int arc0, arc1;
            arc0 = (states[i].arc0.condition<<0) | (states[i].arc0.action<<3) | (((states[i].arc0.next_state-states[i].state)&7)<<7);
            arc1 = (states[i].arc1.condition<<0) | (states[i].arc1.action<<3) | (((states[i].arc1.next_state-states[i].state)&7)<<7);
            rf0 = 0;
            if (states[i].counter_value)
            {
                rf0 = (states[i].counter_value[0]<<rf_state_counter_value_start);
            }
            rf0 |= (states[i].counter_number<<rf_state_counter_number_start) | (arc0<<rf_state_arc0_condition) | (arc1<<rf_state_arc1_condition);
            rf1 = arc1>>(28-rf_state_arc1_condition);
            rf1 |= (i<<24) | ((conds[i]&0xf)<<8) | ((ctl_out[i]&0xf)<<12);

            parallel_config( action_array, action_number, io_parallel_cmd_type_rf_0, rf0 );
            parallel_config( action_array, action_number, io_parallel_cmd_type_rf_1, rf1 );
        }
        else if (do_all)
        {
            rf1 |= (i<<24) | ((conds[i]&0xf)<<8) | ((ctl_out[i]&0xf)<<12);
            parallel_config( action_array, action_number, io_parallel_cmd_type_rf_0, rf0 );
            parallel_config( action_array, action_number, io_parallel_cmd_type_rf_1, rf1 );
        }
    }
}

/*b parallel_init_interface
 */
extern void parallel_init_interface( int slot, unsigned int *action_array, int action_number )
{
    int i;

    /*b Issue all the commands
     */
    for (i=0; i<action_number; i++)
    {
        GIP_POST_TXD_0( 0x01000000 ); // immediate
        GIP_POST_TXD_0( action_array[i] );
        GIP_POST_TXC_0_IO_FIFO_DATA( 0,1,0, slot,0,1 ); // add to egress cmd fifo ; note this may overflow the FIFO if we rush things
    }
}

/*b parallel_wait_for_status
 */
extern void parallel_wait_for_status( int slot )
{
    unsigned int s;
    int par_status_fifo_empty;

    par_status_fifo_empty = 1;
    while (par_status_fifo_empty)
    {
        GIP_POST_TXD_0( (2<<postbus_command_source_io_cmd_op_start) | (1<<postbus_command_source_io_length_start) | (31<<postbus_command_target_gip_rx_semaphore_start) ); // cmd 2 is send overall status, length of 1, to route 0, signal semaphore 31 when its here
        GIP_POST_TXC_0_IO_CMD( 0, 0, 0, 0xc|slot ); // do command '0x011par', which is immediate command to ingress, status FIFO for slot 'par', with route, length, op and rx semaphore given in the data
        do { GIP_READ_AND_CLEAR_SEMAPHORES( s, 1<<31 ); } while (s==0);
        GIP_POST_RXD_0(par_status_fifo_empty); // get status of par status fifo
        par_status_fifo_empty = ((par_status_fifo_empty & 0x80000000)!=0);
    }
}

/*b parallel_get_status
 */
extern void parallel_get_status( int slot, unsigned int *time, unsigned int *status )
{
    unsigned int s;
    GIP_POST_TXD_0( (0<<postbus_command_source_io_cmd_op_start) | (2<<postbus_command_source_io_length_start) | (31<<postbus_command_target_gip_rx_semaphore_start) ); // cmd 0 is get data, length of 2, to route 0, signal semaphore 31 on completion
    GIP_POST_TXC_0_IO_CMD( 0, 0, 0, 0xc|slot ); // do command '0x011ss', which is immediate command to ingress, status FIFO for slot 'ss', with route, length, op and rx semaphore given in the data
    do { GIP_READ_AND_CLEAR_SEMAPHORES( s, 1<<31 ); } while (s==0);
    GIP_POST_RXD_0(*time);
    GIP_POST_RXD_0(*status);
}

