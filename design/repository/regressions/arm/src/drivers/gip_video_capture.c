/* gip_video_capture.c: a GIP net driver

*/

//#define USE_CIRC_BUFFERS

#define GIP_INCLUDE_FROM_C
#ifdef REGRESSION
#include <stdlib.h>
#include "../microkernel/microkernel.h"
#include "uart.h"
#include <gip_support.h>
#include <gip_system.h>
#include <postbus.h>
#include "gip_video_capture.h"
#include "io_parallel.h"
#else // REGRESSION

#include <linux/config.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/linkage.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>
#include <linux/init.h>

#include <asm/microkernel.h>
#include <asm/gip_support.h>
#include <asm/gip_system.h>
#include <asm/postbus.h>
#include "io_parallel.h"
#include "gip_video_capture.h"

#endif // REGRESSION

/*a Defines
 */
#define MAX_ACTIONS (64)
#define CIRC_BUFFER_BASE   (0x80200000)
#define CIRC_BUFFER_BASE_2 (0x80202000)

/*a Types
 */
/*t t_par_action
 */
typedef struct t_par_action
{
    char condition;
    char next_state;
    t_io_parallel_action action;
} t_par_action;

/*t t_par_state
 */
typedef struct t_par_state
{
    int *counter_value;
    int counter_number;
    t_par_action arc0;
    t_par_action arc1;
} t_par_state;

/*a Static variables
 */
static t_gip_vfc vfc;
static unsigned int action_array[ MAX_ACTIONS ];

/*v Frame capture state machine - fc_conds, fc_data, fc_states
  set parallel interface to wait for hsync, and capture 'n' lines
  First state is 'frame start', prior to first 'line start' - here it waits for hsync to be asserted - and stores the number of lines to capture in counter 0
  Second state is 'line start' - here it just sets counter 1 to be the number of pixels to capture on the line, and goes to the next state
  Here it sets counter 2 to be the initial pixel gap - this is gap from hsync asserted-2, and it transitions to state 3 - 'pixel wait and capture'
  The next state is 'pixel wait and capture' - here counter 2 is decremented, and on reaching zero a pixel is captured, the next state is entered. and counter 2 is set to the inter-pixel gap
  The next state is used to count the pixels per line - here counter 1 is decremented, and on reaching zero the 'end of line' state is entered, else it is back to 'pixel wait and capture'
  The final state is used to count the number of lines - here counter 0 is decremented, and on reaching zero the state machine completes, else 'line start' is the continuation

  condition inputs are 0<-(counter==0), 1<-hsync, 2<-vsync, 3-<x
  condition 0 is 'always' (4 is 'never')
  condition 1 is 'counter==0'
  condition 2 is 'hsync'
  condition 3 is 'vsync'
  so inputs->conditions:
    0000->0001, 0001->0011, 0010->0101, 0011->0111
    0100->1001, 0101->1011, 0110->1101, 0111->1111
    1000->0001, 1001->0011, 1010->0101, 1011->0111
    1100->1001, 1101->1011, 1110->1101, 1111->1111

  the states are:
    0000->
  We require no outputs from this state machine, so the output enables are all low.
*/
static const char fc_conds[16]="1357ikmo1357ikmo";
static unsigned int fc_data[5];
static const t_par_state fc_states[] =
{
    /* 0 */ {&fc_data[4], 0, {7,1,io_parallel_action_setcnt},    {4,0,io_parallel_action_idle}, },                       // wait for vsync to be low (asserted)
    /* 1 */ {       NULL, 0, {3,2,io_parallel_action_idle},                {4,1,io_parallel_action_idle}, },             // wait for vsync to be high (deasserted)
    /* 2 */ {       NULL, 0, {1,4,io_parallel_action_idle},                {2,3,io_parallel_action_deccnt}, },           // if counter 0 then go to frame start, else wait for hsync to be high, counting down the lines to skip
    /* 3 */ {       NULL, 0, {6,2,io_parallel_action_idle},                {4,3,io_parallel_action_idle}, },             // wait for hsync to be low, and go back to 2
    /* 4 */ {&fc_data[0], 0, {2,5,io_parallel_action_setcnt},    {4,4,io_parallel_action_idle}, },                       // 0 = frame start,  fc_data[0] must hold nlines
    /* 5 */ {&fc_data[1], 1, {6,6,io_parallel_action_setcnt},    {4,5,io_parallel_action_idle}, },                       // 1 = line start,   fc_data[1] must hold pixels per line, hsync low->next
    /* 6 */ {&fc_data[2], 2, {0,7,io_parallel_action_setcnt},    {4,6,io_parallel_action_idle}, },                       // 2                 fc_data[2] must hold initial pixels gap-2
    /* 7 */ {&fc_data[3], 2, {1,8,io_parallel_action_setcnt_capture},      {0,7,io_parallel_action_deccnt}, },           // 3 = pixel wait,   fc_data[3] must hold interpixel gap-1
    /* 8 */ {       NULL, 1, {1,9,io_parallel_action_idle},                {0,7,io_parallel_action_deccnt}, },           // 4 = count pixels,
    /* 9 */ {       NULL, 0, {1,10,io_parallel_action_end},                {2,5,io_parallel_action_deccnt}, },           // 5 = count lines,  done->end, hsync high->line start
    /*10 */ {       NULL, 0, {4,10,io_parallel_action_idle},               {4,10,io_parallel_action_idle}, },            // 5 = count lines,  done->end, hsync high->line start
};

/*a Parallel assist functions
 */
/*f parallel_config
 */
static void parallel_config( t_io_parallel_cmd_type type, int data )
{
    action_array[ vfc.actions_prepared++ ] = (type<<24) | data;
}

/*f parallel_config_fsm
 */
static void parallel_config_fsm( int nstates, const t_par_state *states, const char *conds )
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
        parallel_config( io_parallel_cmd_type_rf_0, rf0 );
        parallel_config( io_parallel_cmd_type_rf_1, rf1 );
    }
}

/*a Driver functions including hardware thread
 */
/*f gip_video_capture_hw_thread_init
  we use semaphore 24 as virtual rx status ne
  we use semaphore 25 as virtual tx status ne
 */
extern void gip_video_capture_hw_thread_init( int slot )
{
    GIP_ATOMIC_MAX_BLOCK();
    GIP_POST_TXD_0(0); // bit 0 is enable (hold in reset)
    GIP_POST_TXC_0_IO_CFG(0,0,slot);   // do I/O configuration
    GIP_POST_TXD_0(1); // bit 0 is enable (hold in reset)
    GIP_POST_TXC_0_IO_CFG(0,0,slot);   // do I/O configuration

    GIP_CLEAR_SEMAPHORES(0xf<<(VFC_THREAD_NUM*4));
    GIP_POST_TXD_0( (slot<<0) | (1<<2) | (1<<3) | (1<<4) | (0<<5) | (0<<postbus_command_source_io_cmd_op_start) | (0<<postbus_command_source_io_length_start) | (SEM_NUM_VFC_STATUS_VNE<<postbus_command_target_gip_rx_semaphore_start) ); // ingress=1, cmd_status=1, fifo=0, empty_not_watermark=1, level=0, length=0, cmd_op=0 (read data length 0)
    GIP_POST_TXC_0_IO_CMD( 0, 0, 0, 0x1c );
    NOP;NOP;NOP;NOP;NOP;NOP;

    vfc.si_slot = slot;
    vfc.si_action_array = action_array;
    vfc.si_action_array_length = 0;
    vfc.rxd_buffer_store = 0;
    vfc.rxd_buffer_length = 0;
    vfc.rxd_data_length = 0;
    vfc.buffer_presented = 0;
    vfc.actions_prepared = 0;
#define STRINGIFY_SUB(A) #A
#define STRINGIFY(A) STRINGIFY_SUB(A)
    __asm__ volatile ( " .word 0xec00c00e| (" STRINGIFY(VFC_PRIVATE_REG_F) ")<<4 ; mov r0, %0 " : : "r" (&vfc) ); // set our private register to our structure

#ifdef REGRESSION
    uart_tx_string_nl("here");
#else
    printk("GIP video frame capture initialized\n");
#endif
    MK_INT_DIS();
	MK_TRAP2( MK_TRAP_SET_IRQ_HANDLER, MK_INT_BIT_VFC_NUM, (unsigned int)gip_video_capture_isr_asm);
    GIP_ATOMIC_MAX_BLOCK();
    GIP_SET_THREAD(VFC_THREAD_NUM,gip_video_capture_hw_thread, (((SEM_VFC_STATUS_VNE | SEM_VFC_SOFT_IRQ | SEM_VFC_RX_DATA_VWM)>>(4*VFC_THREAD_NUM))<<4)|1 ) // set our flag dependencies and restart mode
    MK_INT_EN();
}

/*a ISR and driver init
 */
/*f gip_video_capture_isr
  called when a frame capture completes or when actions have been completed
 */
#ifdef USE_CIRC_BUFFERS
static unsigned int *circ_buffer = (unsigned int *)CIRC_BUFFER_BASE;
#endif
extern void gip_video_capture_isr (void)
{
//	printk ("Eth ISR\n");

#ifdef REGRESSION
    uart_init();
#endif
}

/*f gip_video_capture_configure
  Either initialize the hardware, or make it go
 */
extern void analyzer_enable( void );
extern int gip_video_capture_configure( int line_skip, int nlines, int init_gap, int pixel_gap, int pixels_per_line )
{
    do { NOP; } while (vfc.si_action_array_length>0);
    parallel_config( io_parallel_cmd_type_reset, 0 );
    parallel_config( io_parallel_cmd_type_config, ( (io_parallel_cfg_data_size_2<<io_parallel_cfd_data_size_start_bit) | 
                                                          (1<<io_parallel_cfd_use_registered_control_inputs_start_bit) |
                                                          (1<<io_parallel_cfd_use_registered_data_inputs_start_bit) |
                                                          (15<<io_parallel_cfd_holdoff_start_bit) |
                                                          (1<<io_parallel_cfd_data_capture_enabled_start_bit) ) );
    fc_data[0] = nlines-1;
    fc_data[1] = pixels_per_line-1;
    fc_data[2] = init_gap-2;
    fc_data[3] = pixel_gap-1;
    fc_data[4] = line_skip;
    parallel_config_fsm( sizeof(fc_states)/sizeof(fc_states[0]), fc_states, fc_conds );
    parallel_config( io_parallel_cmd_type_enable, 0 );
    vfc.si_action_array_length = vfc.actions_prepared;
    vfc.actions_prepared = 0;
    GIP_SET_SEMAPHORES( SEM_VFC_SOFT_IRQ );        // hit hardware thread with softirq
	return 0;
}

/*f gip_video_capture_start
 */
extern void gip_video_capture_start( unsigned char *buffer, int buffer_length )
{
    do { NOP; } while (vfc.si_action_array_length>0);
    parallel_config( io_parallel_cmd_type_go, 0 );
    vfc.buffer_presented = buffer;
    vfc.rxd_buffer_store = buffer;
    vfc.rxd_buffer_length = buffer_length;
    vfc.rxd_data_length = 0;
    vfc.si_action_array_length = vfc.actions_prepared;
    vfc.actions_prepared = 0;
    GIP_SET_SEMAPHORES( SEM_VFC_SOFT_IRQ );        // hit hardware thread with softirq
}

/*f gip_video_capture_poll
  Return 1 if capture complete, else 0
 */
extern int gip_video_capture_poll( void )
{
    return (vfc.rxd_buffer_store==0);
}
