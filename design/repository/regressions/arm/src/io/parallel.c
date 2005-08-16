/*a Includes
 */
#include <stdlib.h> // for NULL
#include "gip_support.h"
#include "postbus.h"
#include "io_parallel.h"
#include "../common/wrapper.h"

/*a Defines
 */
#define DEBUG

/*a Types
 */

/*a Static variables
 */

/*a External functions
 */
/*f parallel_wait_and_read_response
 */
extern void parallel_wait_and_read_response( int slot, int *time, int *status, int *read_data )
{
    int ss_status_fifo_empty;
    unsigned int s;

    ss_status_fifo_empty = 1;
    while (ss_status_fifo_empty)
    {
        GIP_POST_TXD_0( (2<<postbus_command_source_io_cmd_op_start) | (1<<postbus_command_source_io_length_start) | (31<<postbus_command_target_gip_rx_semaphore_start) ); // cmd 2 is send overall status, length of 1, to route 0, signal semaphore 31 when its here
        GIP_POST_TXC_0_IO_CMD( 0, 0, 0, 0xc|slot ); // do command '0x011ss', which is immediate command to ingress, status FIFO for slot 'ss', with route, length, op and rx semaphore given in the data
        do { GIP_READ_AND_CLEAR_SEMAPHORES( s, 1<<31 ); } while (s==0);
        GIP_POST_RXD_0(ss_status_fifo_empty); // get status of ss status fifo
        ss_status_fifo_empty = ((ss_status_fifo_empty & 0x80000000)!=0);
    }
    GIP_POST_TXD_0( (0<<postbus_command_source_io_cmd_op_start) | (2<<postbus_command_source_io_length_start) | (31<<postbus_command_target_gip_rx_semaphore_start) ); // cmd 0 is get data, length of 2, to route 0, signal semaphore 31 on completion
    GIP_POST_TXC_0_IO_CMD( 0, 0, 0, 0xc|slot ); // do command '0x011ss', which is immediate command to ingress, status FIFO for slot 'ss', with route, length, op and rx semaphore given in the data
    do { GIP_READ_AND_CLEAR_SEMAPHORES( s, 1<<31 ); } while (s==0);
    GIP_POST_TXD_0( (0<<postbus_command_source_io_cmd_op_start) | (1<<postbus_command_source_io_length_start) | (31<<postbus_command_target_gip_rx_semaphore_start) ); // cmd 0 is get data, length of 1, to route 0, signal semaphore 31 on completion
    GIP_POST_TXC_0_IO_CMD( 0, 0, 0, 0x4|slot ); // do command '0x001ss', which is immediate command to ingress, data FIFO for slot 'ss', with route, length, op and rx semaphore given in the data
    do { GIP_READ_AND_CLEAR_SEMAPHORES( s, 1<<31 ); } while (s==0);
    GIP_POST_RXD_0(*time);
    GIP_POST_RXD_0(*status);
    GIP_POST_RXD_0(*read_data);
}

/*f parallel_mdio_write
 */
extern void parallel_mdio_write( int slot, int clock_divider, unsigned int value )
{
    GIP_POST_TXD_0( value );
    GIP_POST_TXC_0_IO_FIFO_DATA(0,0,0,slot,0,0);   // add to egress data fifo
    GIP_POST_TXD_0( 0x01000000 );                      // immediate command
    GIP_POST_TXD_0( 0x10700020 | (clock_divider<<8) ); // MDIO write = tx no tristate, clock/n, length 32 (write)
    GIP_POST_TXC_0_IO_FIFO_DATA(0,1,0,slot,0,1);   // add to egress cmd fifo
}

/*f parallel_mdio_read
 */
extern void parallel_mdio_read( int slot, int clock_divider, unsigned int value )
{
    GIP_POST_TXD_0( value );
    GIP_POST_TXC_0_IO_FIFO_DATA(0,0,0,slot,0,0);   // add to egress data fifo
    GIP_POST_TXD_0( 0x01000000 );                      // immediate command
    GIP_POST_TXD_0( 0x107e0020 | (clock_divider<<8) ); // MDIO read = tristate after 14, clock/n, length 32 (write)
    GIP_POST_TXC_0_IO_FIFO_DATA(0,1,0,slot,0,1);   // add to egress cmd fifo
}

/*f parallel_frame_capture_init
  set parallel interface to wait for hsync, and capture 'n' lines
  First state is 'frame start', prior to first 'line start' - here it waits for hsync to be asserted - and stores the number of lines to capture in counter 0
  Second state is 'line start' - here it just sets counter 1 to be the number of pixels to capture on the line, and goes to the next state
  Here it sets counter 2 to be the initial pixel gap - this is gap from hsync asserted-2, and it transitions to state 3 - 'pixel wait and capture'
  The next state is 'pixel wait and capture' - here counter 2 is decremented, and on reaching zero a pixel is captured, the next state is entered. and counter 2 is set to the inter-pixel gap
  The next state is used to count the pixels per line - here counter 1 is decremented, and on reaching zero the 'end of line' state is entered, else it is back to 'pixel wait and capture'
  The final state is used to count the number of lines - here counter 0 is decremented, and on reaching zero the state machine completes, else 'line start' is the continuation

  condition inputs are 0<-(counter==0), 1<-hsync, 2<-x, 3-<x
  condition 0 is 'always' (4 is 'never')
  condition 1 is 'counter==0'
  condition 2 is 'hsync'
  condition 3 is dont care
  so inputs->conditions:
    0000->0001, 0001->0011, 0010->0101, 0011->0111
    0100->0001, 0101->0011, 0110->0101, 0111->0111
    1000->0001, 1001->0011, 1010->0101, 1011->0111
    1100->0001, 1101->0011, 1110->0101, 1111->0111

  the states are:
    0000->
  We require no outputs from this state machine, so the output enables are all low.
 */
static const char fc_conds[16]="1357135713571357";
static unsigned int fc_data[4];
typedef struct t_par_action
{
    char condition;
    char next_state;
    t_io_parallel_action action;
} t_par_action;

typedef struct t_par_state
{
    int *counter_value;
    int counter_number;
    t_par_action arc0;
    t_par_action arc1;
} t_par_state;

static const t_par_state fc_states[] =
{
    {&fc_data[0], 0, {2,1,io_parallel_action_setcnt_nocapture},    {4,0,io_parallel_action_idle}, },             // 0 = frame start,  fc_data[0] must hold nlines
    {&fc_data[1], 1, {0,2,io_parallel_action_setcnt_nocapture},    {4,0,io_parallel_action_idle}, },             // 1 = line start,   fc_data[1] must hold pixels per line
    {&fc_data[2], 2, {0,3,io_parallel_action_setcnt_nocapture},    {4,0,io_parallel_action_idle}, },             // 2                 fc_data[2] must hold initial pixels gap-2
    {&fc_data[3], 2, {1,4,io_parallel_action_setcnt_capture},      {0,3,io_parallel_action_deccnt_nocapture}, }, // 3 = pixel wait,   fc_data[3] must hold interpixel gap-1
    {       NULL, 1, {1,5,io_parallel_action_idle},                {0,3,io_parallel_action_deccnt_nocapture}, }, // 4 = count pixels,
    {       NULL, 0, {1,6,io_parallel_action_end},                 {2,1,io_parallel_action_deccnt_nocapture}, }, // 5 = count lines
};

static void parallel_config( int slot, t_io_parallel_cmd_type type, int data )
{
    GIP_POST_TXD_0( 0x01000000 ); // immediate
    GIP_POST_TXD_0( (type<<24) | data );
    GIP_POST_TXC_0_IO_FIFO_DATA( 0,1,0, slot,0,1 ); // add to egress cmd fifo ; note this may overflow the FIFO if we rush things
}

static void parallel_config_fsm( int slot, int nstates, const t_par_state *states, const char *conds )
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
            arc0 = (states[i].arc0.condition<<0) | (states[i].arc0.action<<3) | (((states[i].arc0.next_state-i)&7)<<6);
            arc1 = (states[i].arc1.condition<<0) | (states[i].arc1.action<<3) | (((states[i].arc1.next_state-i)&7)<<6);
            rf0 = 0;
            if (states[i].counter_value)
            {
                rf0 = (states[i].counter_value[0]<<0);
            }
            rf0 |= (states[i].counter_number<<12) | (arc0<<14) | ((arc1&1)<<23);
            rf1 = arc1>>1;
        }
        rf1 |= (i<<16) | ((conds[i]&0xf)<<8);
        parallel_config( slot, io_parallel_cmd_type_rf_0, rf0 );
        parallel_config( slot, io_parallel_cmd_type_rf_1, rf1 );
    }
}

extern void parallel_frame_capture_init( int slot, int nlines, int init_gap, int pixel_gap, int pixels_per_line )
{
    GIP_POST_TXD_0(0); // bit 0 is enable (hold in reset)
    GIP_POST_TXC_0_IO_CFG(0,0,slot);   // do I/O configuration
    GIP_POST_TXD_0(1); // bit 0 is enable (hold in reset)
    GIP_POST_TXC_0_IO_CFG(0,0,slot);   // do I/O configuration

    // parallel_config issues an immediate command to the parallel I/O interface of the specified type with specified data content
    parallel_config( slot, io_parallel_cmd_type_config, ( (io_parallel_cfg_capture_size_2<<io_parallel_cfd_capture_size_start_bit) | 
                                                          (1<<io_parallel_cfd_use_registered_control_inputs_start_bit) |
                                                          (1<<io_parallel_cfd_use_registered_data_inputs_start_bit) |
                                                          (15<<io_parallel_cfd_data_holdoff_start_bit) |
                                                          (15<<io_parallel_cfd_status_holdoff_start_bit) |
                                                          (1<<io_parallel_cfd_data_capture_enabled_start_bit) |
                                                          (1<<io_parallel_cfd_interim_status_start_bit) ));
    fc_data[0] = nlines-1;
    fc_data[1] = pixels_per_line-1;
    fc_data[2] = init_gap-2;
    fc_data[3] = pixel_gap-1;
    parallel_config_fsm( slot, 6, fc_states, fc_conds );

 }

extern void parallel_frame_capture_start( int slot, unsigned int time )
{
    parallel_config( slot, io_parallel_cmd_type_enable, 0 );
    parallel_config( slot, io_parallel_cmd_type_go, 0 );
}

    int ss_status_fifo_empty;
    unsigned int s;

static void parallel_frame_capture_wait_for_status( int slot )
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

extern int parallel_frame_capture_buffer( int slot, unsigned int *buffer, int *capture_time, int *num_status )
{
    unsigned int status, time;
    int final_status;
    int words_so_far, data_captured, words_captured;

    *num_status = 0;
    words_so_far = 0;
    do
    {
        parallel_frame_capture_wait_for_status( slot );
        GIP_POST_TXD_0( (0<<postbus_command_source_io_cmd_op_start) | (2<<postbus_command_source_io_length_start) | (31<<postbus_command_target_gip_rx_semaphore_start) ); // cmd 0 is get data, length of 2, to route 0, signal semaphore 31 on completion
        GIP_POST_TXC_0_IO_CMD( 0, 0, 0, 0xc|slot ); // do command '0x011ss', which is immediate command to ingress, status FIFO for slot 'ss', with route, length, op and rx semaphore given in the data
        do { GIP_READ_AND_CLEAR_SEMAPHORES( s, 1<<31 ); } while (s==0);
        GIP_POST_RXD_0(time);
        GIP_POST_RXD_0(status);

        GIP_EXTBUS_DATA_WRITE( status );
        (*num_status)++;

        final_status = !(status&0x80000000);
        data_captured = status&0xffffff; // data captured in samples, each of which is 2 bits at present, so number of words is data_captured/16
        words_captured = (data_captured/16); // number of words pushed in to data FIFO
        if (final_status && (data_captured&0xf)) words_captured++; // last status pushes any remaining data, so there would be an extra word

        while (words_so_far < words_captured)
        {
            int n;
            unsigned int s;

            n = (words_captured-words_so_far);
            n = (n>8)?8:n;
            GIP_POST_TXD_0( (0<<postbus_command_source_io_cmd_op_start) | (n<<postbus_command_source_io_length_start) | (31<<postbus_command_target_gip_rx_semaphore_start) ); // read 'n' data words and signal when they arrive
            GIP_POST_TXC_0_IO_CMD( 0, 0, 0, 0x4|slot ); // do command '0x001ss', which is immediate command to ingress, data FIFO for slot 'ss', with route, length, op and rx semaphore given in the data
            do { GIP_READ_AND_CLEAR_SEMAPHORES( s, 1<<31 ); } while (s==0);
            while (n>0)
            {
                GIP_POST_RXD_0(s);
                buffer[words_so_far] = s;
                n--;
                words_so_far++;
            }
        }
    } while (!final_status); // intermediate

    *capture_time = time;
    return data_captured/4;
}
