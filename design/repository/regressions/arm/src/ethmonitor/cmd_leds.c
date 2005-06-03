/*a Includes
 */
#include "gip_support.h"
#include "cmd.h"
#include <stdlib.h> // for NULL

/*a Defines
 */

/*a Types
 */

/*a Static functions
 */
#define TIMER_1_THREAD (4)
/*f command_leds_eth
  cannot corrupt any registers
 */
static int timer_1_delay;
static int timer_1_reg_store[16];
static void timer_1_entry( void )
{
    // preserve r0 in r20
    __asm__ volatile( " .word 0xec00c14e \n mov r0, r0 \n " );
    // preserve r1-r12 in memory
    __asm__ volatile( " ldr r0, =timer_1_reg_store \n stmia r0, {r1-r14} \n" );

    {
        unsigned int t, s;
        // read timer
        GIP_TIMER_READ_0( t );
        // clear semaphore
        GIP_READ_AND_CLEAR_SEMAPHORES( s, 1<<(TIMER_1_THREAD*4+0) );
        // set timer to new value (old+x)
        t += timer_1_delay;
        GIP_TIMER_WRITE( 1, t );
        // get led 0 ; this section should be atomic, but its only an LED!
        GIP_LED_OUTPUT_CFG_READ( s );
        // toggle bit 0 value
        s ^= 1;
        // write ld 0
        GIP_LED_OUTPUT_CFG_WRITE( s );
        __asm__ volatile (" mov r8, %0, lsl #8 \n orr r8, r8, #0x96 \n orr r8, r8, #0x16000000 \n .word 0xec00c40e \n mov r8, r8" :  : "r" ('*') : "r8" ); // extrdrm (c): cddm rd is (3.5 type 2 reg 0) (m is top 3.1 - type of 7 for no override)
    }
    // recover r1-r12 in memory
    __asm__ volatile( " ldr r0, =timer_1_reg_store \n ldmia r0, {r1-r14} \n" );
    // recover r0, from r20
    __asm__ volatile( " .word 0xec00ce01 \n mov r0, r4 \n " );
    // deschedule
    GIP_DESCHEDULE();
    NOP;NOP;NOP;
}

static int command_leds( void *handle, int argc, unsigned int *args )
{
    unsigned int s;
    uart_tx_string_nl("A");
    __asm__ volatile (".word 0xec00c83e \n mov r0, %0" : : "r" (0x40000) ); // set special (4) reg gip_config (3) to 0x40000 (preemptive, prioritized, no privilege, 4 arm trap semaphore)
    GIP_SET_THREAD(TIMER_1_THREAD,timer_1_entry,0x111); // set timer 1 thread startup to be ARM, on its semaphore 0 set, and the entry point
    uart_tx_string_nl("B");
    timer_1_delay = args[0];
    GIP_TIMER_ENABLE();
    uart_tx_string_nl("C");
    GIP_SET_LOCAL_EVENTS_CFG((8 + TIMER_1_THREAD)<<(0*4)); // timer 1 is event 0; attach to our thread and enable
    GIP_READ_AND_SET_SEMAPHORES( s, 1<<(TIMER_1_THREAD*4+0) );
    uart_tx_string_nl("Done");
    return 0;
}

/*a External variables
 */
/*v monitor_cmds_leds
 */
extern const t_command monitor_cmds_leds[];
const t_command monitor_cmds_leds[] =
{
    {"leds", command_leds},
    {(const char *)0, (t_command_fn *)0},
};

