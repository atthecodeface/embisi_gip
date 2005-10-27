/* parallel_2.c - handle the soft parallel interface

*/

//#define USE_CIRC_BUFFERS

#define GIP_INCLUDE_FROM_C
#ifdef REGRESSION
#include <stdlib.h>
#include <gip_support.h>
#include <gip_system.h>
#include <postbus.h>
#include <io_parallel.h>
#include "postbus_config.h"
#include "parallel_2.h"
#else // REGRESSION

#endif // REGRESSION

/*a Defines
 */
#define MAX_ACTIONS 128

/*a Types
 */

/*a Static variables
 */
static unsigned int action_array[ MAX_ACTIONS ];
static int action_number;

/*v PS2 state machine - fc_conds, fc_data, fc_states
  The theory behind this FSM is to have a central bit FSM that captures and drives output bits on the PS2 open-collector data pin, while clocking the clock open-collector
  For a host-to-keyboard (us) transfer, the host pulls clock low, pulls data low, releases clock, then expects us to drive clock 10 times while reading data, then once more while we drive a '0' acknowledgement on the data
  For a keyboard-to-host transfer, we output eleven bits, pulsing the clock low during the stable period of the bit
  We can poll for a host-to-keyboard transfer by looking at clock when we are not driving it - if low, then it is a host-to-keyboard transaction, and that should be perfomed
  If we start a keyboard-to-host transfer and the clock is low when we are not driving it, we can abort our transmission and the software can make us poll instead
  For data tx out we need to ensure the data we are transmitting (11 bits - start, 8 data, parity, stop) is in the TX data fifo before we start the FSM; start is low, stop is high, parity is odd (data=0, parity=1)
  For polling we need to ensure the data we are transmitting (12 bits - start, 8 data, parity, stop, ack) is all 1's except the ack
  So the central bit FSM has a start state of clock undriven (high we expect); it is assumed that this state is entered for the first time when the start bit is already being transmitted
  If the clock is low at this point then we abort - the host is driving the clock low
  If the clock is high and remains so for 1/4 of a full clock period, we drive the clock low
  After driving the clock low for 1/2 of a full clock period, we capture a data bit and go to a state to drive the clock high and to test to see if we have done all the bits required
  If we have done all the bits required, we end
  If not, we let the clock go high for a 1/4 of a clock, then transition back to the start of the loop while moving on to the next transmit bit

  So this main loop sends or receives the appropriate number of bits, and it aborts on an unexpected host->keyboard transaction

  Entering the loop can come from a tx start path, which has to set the bit count and drive out the start bit
  Or it can come from an rx poll path, which starts by checking for clock low - if not, it ends, as the host is not attempting transmission
  If the clock is low then the FSM waits for clock high or data low - if clock goes high first, the host timed out, so the poll is over; if the data is low then the host is attempting transmission
  In the latter case the FSM waits for the clock to go high before entering the bit loop

  Note:
  The polling must be done at intervals of 1ms or less
  The full clock period must be between 60 and 100 us

  For implementation, then we have the following conditions:
  cntr==0
  clk in high
  data in high
  clk in low
  data in low

  We can use registered clock and data in, provided we realize that with open collector and a slow rise time we dont check for clock low until a good while after we stop driving it low (which is guaranteed by the FSM)

  Bit state machine... 7 states .. use states 8, 9, 10, 11, 12 for the loop (tx enters at 9, rx enters at 8); state 13 for data hold, 15 for abort, 14 for end

  8 (ck rising, data out hold); on entry count (0) should be 1/4 clock period; count (0) expires -> 9 setting count(0) to 1/2 clock period for state 9, transmitting next data bit
  9 (ck high, data out setup half); on entry count (0) should be 1/4 clock period ; count (0) expires -> 10 setting count(0) to 1/2 clock period for state 10, else -> 12 to check clock low
  10 (ck low, data out stable); on entry count (0) should be 1/2 clock period; count (0) expires -> 11 setting count(0) to 1/4 clock period for state 8
  11 (ck rising); on entry count(1) should be number of bits left, capture data on entry; finished bits (count 1==0) -> data hold (13); not -> decrement counter, restart loop at state 8
  12 (ck high, data out setup other half): clock low -> abort; else dec clock and -> 9 to check counter
  13 (data hold before end): count (0) expires -> 14
  14 (end); end
  15 (abort); end

  Rx entry (poll)... 4 states (start, clock low wait for data low, data low wait for clock high, and poll end); data low state must be able to jump to bit entry at 8, so use 2, 3, 4, 7
  2 (start poll); clock high -> poll end (3); clock low -> wait for data low (4) setting count (0) to 1/4 clock period
  3 (poll end); end
  4 (wait for data low); clock high -> poll end (3); data low -> wait for clock high (7)
  7 (wait for clock high); data high -> poll end (3); clock high -> bit state machine rx entry (8) setting number of bits (count 1) to 10

  Tx entry ... 2 states (start tx, second); second must enter bit state machine at tx entry (9), so use 5,6

  5 (start tx); set count (1) to number of bits, go to state 6
  6 (second); set count (0) to 1/4 clock period for state 9, go to state 9

  Then we need the outputs configured as follows:
  decode state[3,2,1], data out[0] to create control signals
  put clock on control[0] driven with control[1] as oe (control[1] = ~control[0] to ensure we have open collector)
  put data on control[2] with oe from control[3] as oe (control[1] = ~control[0] to ensure we have open collector)
  drive data and clock in states 6/7, 8/9, 10/11, but no others
  clock high in 6/7, 8/9, 12/13
  clock low in 10/11
  data to data_out[0] in 6/7, 8/9, 10/11, 12/13

  state[3,2,1].data[0]   ctl_out[3,2,1,0]
   00xx                     0x0x
   010x                     0x0x
   0110                     1001 - 6/7, clk high, data out
   0111                     0101 - 6/7, clk high, data out
   1000                     1001 - 8/9, clk high, data out
   1001                     0101 - 8/9, clk high, data out
   1010                     1010 - 10/11, clk low, data out
   1011                     0110 - 10/11, clk low, data out
   1100                     1001 - 12/13, clk high, data out
   1101                     0101 - 12/13, clk high, data out
   111x                     0x0x

   conditions bit 0->always, bit 1->counter==0, bit 2->clock high, bit 3->data high
   condition inputs: [0]<=counter==0, [1]<=ctl in [0] (clock), [2]<=ctl in [1], [3]<= ctl in [2] (data)
   dxcC                    condition value
   xxxx                     xxx1
   xxx1                     xx1x
   xx1x                     x1xx
   1xxx                     1xxx
   
*/
static const char fc_conds[16]="13571357ikmoikmo";
static const char fc_ctl_out[16]="0000009595j69500";
static unsigned int fc_data[6];
#define C_AL (0)
#define C_EQ (1)
#define C_CH (2)
#define C_DH (3)
#define C_NV (4)
#define C_NE (5)
#define C_CL (6)
#define C_DL (7)
#define SET(c,st) {c,st,io_parallel_action_setcnt}
#define SETC(c,st)  {c,st,io_parallel_action_setcnt_capture}
#define SETT(c,st)  {c,st,io_parallel_action_setcnt_transmit}
#define DEC(c,st) {c,st,io_parallel_action_deccnt},
#define END(c,st)   {c,st,io_parallel_action_end}
#define IDLE(c,st)  {c,st,io_parallel_action_idle}
#define QUARTER_CLK (&fc_data[0])
#define HALF_CLK    (&fc_data[1])
#define NUM_TX_BITS (&fc_data[2])
#define NUM_RX_BITS (&fc_data[3])
static const t_par_state fc_init_states[] =
{
    {0,         NULL, 0, END(C_AL,0),   END(C_AL,0) },
    {1,         NULL, 0, END(C_AL,1),   END(C_AL,1) },
    {2,  QUARTER_CLK, 0, END(C_CH,3),   SET(C_CL,4) },              // rx start poll
    {3,         NULL, 0, END(C_AL,3),   IDLE(C_NV,3) },             // poll end
    {4,         NULL, 0, END(C_CH,3),   IDLE(C_DL,7) },             // rx wait for data low
    {5,  NUM_TX_BITS, 1, SET(C_AL,6),   IDLE(C_NV,5) },             // tx start
    {6,  QUARTER_CLK, 0, SET(C_AL,9),   IDLE(C_NV,6) },             // tx second
    {7,  NUM_RX_BITS, 1, IDLE(C_DH,3),  SETC(C_CH,8) },             // rx wait for clock high
    {8,  QUARTER_CLK, 0, SETT(C_EQ,9),  DEC(C_AL,8) },              // bits, clk rising, data out hold - wait here until counter expires
    {9,     HALF_CLK, 0, SET(C_EQ,10),  DEC(C_AL,12) },             // bits, clk high, clk low check, data out setup - wait in (9/12) until counter expires or clock is low for abort
    {10, QUARTER_CLK, 0, SETC(C_EQ,11), DEC(C_AL,10) },             // bits, clk low, wait for counter to expire then capture data, -> 11
    {11,        NULL, 1, IDLE(C_EQ,13), DEC(C_AL,8) },              // bits, test end of bits -> 13 (data hold), else -> 8 decrementing bits left
    {12,        NULL, 0, END(C_CL,15),  IDLE(C_AL,9) },             // bits, clk high, clk low check, data out setup (pt 2) - checking for clock low abort else return to 9
    {13,        NULL, 0, END(C_EQ,14),  DEC(C_AL,13) },             // bits, data hold before end
    {14,        NULL, 0, END(C_AL,14),  END(C_AL,14) },             // bits, end okay
    {15,        NULL, 0, END(C_AL,15),  END(C_AL,15) },             // bits, abort
};

/*a Test entry point
 */
/*f test_entry_point
 */
extern int test_entry_point()
{
    unsigned int base_config;
    int clock_period;
    unsigned int time, status;
    unsigned int s;

    /*b Configure the postbus
     */
    postbus_config( 16, 16, 0, 0 ); // use 16 for tx and 16 for rx, no sharing
    GIP_CLEAR_SEMAPHORES( -1 );

    GIP_LED_OUTPUT_CFG_WRITE( 0xaaaa );
    GIP_BLOCK_ALL();
    GIP_EXTBUS_CONFIG_WRITE( 0x111 );
    GIP_BLOCK_ALL();
    GIP_EXTBUS_ADDRESS_WRITE( 0xc0000002 ); // Debug display address
    GIP_BLOCK_ALL();

    GIP_POST_TXD_0(1); // bit 0 is disable (hold in reset)
    GIP_POST_TXC_0_IO_CFG(0,0,IO_A_SLOT_PARALLEL_1);   // do I/O configuration

    /*b Configure the FSM
     */
    action_number = 0;
    parallel_config( action_array, &action_number, io_parallel_cmd_type_reset, 0 );
    base_config = ( (io_parallel_cfg_data_size_1<<io_parallel_cfd_data_size_start_bit) | 
                    (1<<io_parallel_cfd_use_registered_control_inputs_start_bit) |
                    (1<<io_parallel_cfd_use_registered_data_inputs_start_bit) |
                    (4<<io_parallel_cfd_holdoff_start_bit) |
                    (1<<io_parallel_cfd_data_capture_enabled_start_bit) |
                    (1<<io_parallel_cfd_interim_status_start_bit) |
                    (1<<io_parallel_cfd_ctl_out_state_override_start_bit) |
                    (1<<io_parallel_cfd_data_out_enable_start_bit) |
                    (0<<io_parallel_cfd_data_out_use_ctl3_start_bit) |
                    (io_parallel_ctl_oe_controlled<<io_parallel_cfd_ctl_oe01_start_bit) |
                    (io_parallel_ctl_oe_controlled<<io_parallel_cfd_ctl_oe23_start_bit)
        );


    parallel_config( action_array, &action_number, io_parallel_cmd_type_config, base_config );

    clock_period = 20;
    *(QUARTER_CLK) = clock_period/4;
    *(HALF_CLK) = clock_period/2;
    *(NUM_TX_BITS) = 10;
    *(NUM_RX_BITS) = 10;

    parallel_config_fsm( action_array, &action_number, 16, fc_init_states, fc_conds, fc_ctl_out, 1 );
    parallel_config( action_array, &action_number, io_parallel_cmd_type_enable, 0 );

    GIP_EXTBUS_DATA_WRITE( action_number );
    parallel_init_interface( IO_A_SLOT_PARALLEL_1, action_array, action_number );

    /*b Transmit a byte
     */
    action_number = 0;
    parallel_config( action_array, &action_number, io_parallel_cmd_type_go, 5 ); // 5 for tx
    GIP_POST_TXD_0( (0<<0) | (0x41<<1) | (1<<9) | (0xff<<10) ); // letter A, odd parity -> parity of 1
    GIP_POST_TXC_0_IO_FIFO_DATA( 0,1-1,31, IO_A_SLOT_PARALLEL_1,0,0 ); // add j words to etx fifo (egress data fifo 0)
    GIP_EXTBUS_DATA_WRITE( action_number );
    parallel_init_interface( IO_A_SLOT_PARALLEL_1, action_array, action_number );

    parallel_wait_for_status( IO_A_SLOT_PARALLEL_1 );
    parallel_get_status( IO_A_SLOT_PARALLEL_1, &time, &status );
    GIP_EXTBUS_DATA_WRITE( time );
    GIP_BLOCK_ALL();
    GIP_BLOCK_ALL();
    GIP_EXTBUS_DATA_WRITE( status );
    GIP_BLOCK_ALL();
    GIP_BLOCK_ALL();
    if ((status>>24)!=14) return 1; // end state is 'end', not 'abort'
    if ((status&0xff)!=11) return 2; // captured 11 bits
    // check data top 11 bits is what we transmitted
    GIP_POST_TXD_0( (0<<postbus_command_source_io_cmd_op_start) | (1<<postbus_command_source_io_length_start) | (31<<postbus_command_target_gip_rx_semaphore_start) ); // read 'n' data words and signal when they arrive
    GIP_POST_TXC_0_IO_CMD( 0, 0, 0, 0x4|IO_A_SLOT_PARALLEL_1 ); // do command '0x001ss', which is immediate command to ingress, data FIFO for slot 'ss', with route, length, op and rx semaphore given in the data
    do { GIP_READ_AND_CLEAR_SEMAPHORES( s, 1<<31 ); } while (s==0);
    GIP_POST_RXD_0(s);
    if ((s>>21)!= 0x682) return 3;

    /*b Set up for poll
     */
    action_number = 0;
    parallel_config( action_array, &action_number, io_parallel_cmd_type_go, 2 ); // 2 for rx

    /*b Do a poll - expect it to fail
     */
    {int i; for (i=0; i<55; i++) NOP;}
    GIP_TIMER_READ_0(time);
    GIP_EXTBUS_DATA_WRITE( time );
    GIP_TIMER_READ_0(time);
    GIP_EXTBUS_DATA_WRITE( time );

    GIP_POST_TXD_0( 0xf7ff ); // ACK low only
    GIP_POST_TXC_0_IO_FIFO_DATA( 0,1-1,31, IO_A_SLOT_PARALLEL_1,0,0 ); // add j words to etx fifo (egress data fifo 0)
    GIP_EXTBUS_DATA_WRITE( action_number );
    parallel_init_interface( IO_A_SLOT_PARALLEL_1, action_array, action_number );

    parallel_wait_for_status( IO_A_SLOT_PARALLEL_1 );
    parallel_get_status( IO_A_SLOT_PARALLEL_1, &time, &status );
    GIP_EXTBUS_DATA_WRITE( time );
    GIP_BLOCK_ALL();
    GIP_BLOCK_ALL();
    GIP_EXTBUS_DATA_WRITE( status );
    GIP_BLOCK_ALL();
    GIP_BLOCK_ALL();
    if ((status>>24)!=3) return 4; // end state is 'no poll'

    /*b Do a poll - expect it to fail
     */
    GIP_POST_TXD_0( 0xf7ff ); // ACK low only
    GIP_POST_TXC_0_IO_FIFO_DATA( 0,1-1,31, IO_A_SLOT_PARALLEL_1,0,0 ); // add j words to etx fifo (egress data fifo 0)
    GIP_EXTBUS_DATA_WRITE( action_number );
    parallel_init_interface( IO_A_SLOT_PARALLEL_1, action_array, action_number );

    parallel_wait_for_status( IO_A_SLOT_PARALLEL_1 );
    parallel_get_status( IO_A_SLOT_PARALLEL_1, &time, &status );
    GIP_EXTBUS_DATA_WRITE( time );
    GIP_BLOCK_ALL();
    GIP_BLOCK_ALL();
    GIP_EXTBUS_DATA_WRITE( status );
    GIP_BLOCK_ALL();
    GIP_BLOCK_ALL();
    if ((status>>24)!=3) return 5; // end state is 'no poll'

    /*b Do a poll - expect it to pass
     */
    GIP_POST_TXD_0( 0xf7ff ); // ACK low only
    GIP_POST_TXC_0_IO_FIFO_DATA( 0,1-1,31, IO_A_SLOT_PARALLEL_1,0,0 ); // add j words to etx fifo (egress data fifo 0)
    GIP_EXTBUS_DATA_WRITE( action_number );
    parallel_init_interface( IO_A_SLOT_PARALLEL_1, action_array, action_number );

    parallel_wait_for_status( IO_A_SLOT_PARALLEL_1 );
    parallel_get_status( IO_A_SLOT_PARALLEL_1, &time, &status );
    GIP_EXTBUS_DATA_WRITE( time );
    GIP_BLOCK_ALL();
    GIP_BLOCK_ALL();
    GIP_EXTBUS_DATA_WRITE( status );
    GIP_BLOCK_ALL();
    GIP_BLOCK_ALL();
    if ((status>>24)!=14) return 6; // end state is 'end okay'
    if ((status&0xff)!=12) return 7; // captured 12 bits - start bit is captured twice
    // check data top 11 bits is what we transmitted
    GIP_POST_TXD_0( (0<<postbus_command_source_io_cmd_op_start) | (1<<postbus_command_source_io_length_start) | (31<<postbus_command_target_gip_rx_semaphore_start) ); // read 'n' data words and signal when they arrive
    GIP_POST_TXC_0_IO_CMD( 0, 0, 0, 0x4|IO_A_SLOT_PARALLEL_1 ); // do command '0x001ss', which is immediate command to ingress, data FIFO for slot 'ss', with route, length, op and rx semaphore given in the data
    do { GIP_READ_AND_CLEAR_SEMAPHORES( s, 1<<31 ); } while (s==0);
    GIP_POST_RXD_0(s);
    GIP_EXTBUS_DATA_WRITE( s );
    if ((s>>21)!= 0x341) return 8; // 682d0400.. = 011 0100 0001 001101 ...

    /*b Transmit a byte - expect it to pass
     */
    action_number = 0;
    parallel_config( action_array, &action_number, io_parallel_cmd_type_go, 5 ); // 5 for tx
    GIP_POST_TXD_0( (0<<0) | (0x41<<1) | (1<<9) | (0xff<<10) ); // letter A, odd parity -> parity of 1
    GIP_POST_TXC_0_IO_FIFO_DATA( 0,1-1,31, IO_A_SLOT_PARALLEL_1,0,0 ); // add j words to etx fifo (egress data fifo 0)
    GIP_EXTBUS_DATA_WRITE( action_number );
    parallel_init_interface( IO_A_SLOT_PARALLEL_1, action_array, action_number );

    parallel_wait_for_status( IO_A_SLOT_PARALLEL_1 );
    parallel_get_status( IO_A_SLOT_PARALLEL_1, &time, &status );
    GIP_EXTBUS_DATA_WRITE( status );
    GIP_BLOCK_ALL();
    GIP_BLOCK_ALL();
    if ((status>>24)!=14) return 0x101; // end state is 'end', not 'abort'
    if ((status&0xff)!=11) return 0x102; // captured 11 bits
    // check data top 11 bits is what we transmitted
    GIP_POST_TXD_0( (0<<postbus_command_source_io_cmd_op_start) | (1<<postbus_command_source_io_length_start) | (31<<postbus_command_target_gip_rx_semaphore_start) ); // read 'n' data words and signal when they arrive
    GIP_POST_TXC_0_IO_CMD( 0, 0, 0, 0x4|IO_A_SLOT_PARALLEL_1 ); // do command '0x001ss', which is immediate command to ingress, data FIFO for slot 'ss', with route, length, op and rx semaphore given in the data
    do { GIP_READ_AND_CLEAR_SEMAPHORES( s, 1<<31 ); } while (s==0);
    GIP_POST_RXD_0(s);
    if ((s>>21)!= 0x682) return 0x103; // we don't expect this, to be honest

    /*b Transmit another  byte - expect it to fail
     */
    GIP_POST_TXD_0( (0<<0) | (0x41<<1) | (1<<9) | (0xff<<10) ); // letter A, odd parity -> parity of 1
    GIP_POST_TXC_0_IO_FIFO_DATA( 0,1-1,31, IO_A_SLOT_PARALLEL_1,0,0 ); // add j words to etx fifo (egress data fifo 0)
    parallel_init_interface( IO_A_SLOT_PARALLEL_1, action_array, action_number );

    parallel_wait_for_status( IO_A_SLOT_PARALLEL_1 );
    parallel_get_status( IO_A_SLOT_PARALLEL_1, &time, &status );
    GIP_EXTBUS_DATA_WRITE( time );
    GIP_BLOCK_ALL();
    GIP_BLOCK_ALL();
    GIP_EXTBUS_DATA_WRITE( status );
    GIP_BLOCK_ALL();
    GIP_BLOCK_ALL();
    if ((status>>24)!=15) return 0x110; // end state is abort'
    if ((status&0xff)!=0) return 0x111; // captured 0 bits

    return 0;
}
