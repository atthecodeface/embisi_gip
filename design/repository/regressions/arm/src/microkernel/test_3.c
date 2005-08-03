/*a Includes
 */
#include "microkernel.h"
#include "gip_support.h"

/*a Defines
 */

/*a Types
 */

/*a External functions
 */
/*f timer_operation
 */
static int led_cfg;
static void timer_operation( void )
{
    MK_INT_CLR(0);
    led_cfg ^= 0x5555;
    GIP_LED_OUTPUT_CFG_WRITE( led_cfg );
    MK_RETURN_TO_STACK_FRAME();
}
static void timer_operation_wrapper( void )
{
    __asm__ volatile ( "bl timer_operation" );
    MK_RETURN_TO_STACK_FRAME();
}

/*f timer_int_start
 */
extern void timer_int( void );
static void timer_int_start ( void )
{
    unsigned int s;

    led_cfg = 0xaaaa;
    microkernel_int_register( 0, timer_operation_wrapper );
    MK_INT_DIS();
    GIP_TIMER_DISABLE();
    GIP_ATOMIC_MAX_BLOCK();
    GIP_CLEAR_SEMAPHORES_ATOMIC(0xf<<28);
    GIP_SET_THREAD(7,timer_int,0x11); // set thread 7 startup to be ARM, on semaphore 28 set, and the entry point
    GIP_SET_LOCAL_EVENTS_CFG( 0xf ); // set config so that timer 1 (event 0) -> high priority thread 7
    GIP_READ_AND_SET_SEMAPHORES(s,0x1<<28); // fire the thread
    GIP_TIMER_ENABLE(); // enable the timer
    MK_INT_EN();
}

void analyzer_init( void )
{
#define GIP_ANALYZER_WRITE(t,s) {__asm__ volatile ( " .word 0xec00c20e+" #t "<<4 \n mov r0, %0 \n" : : "r" (s) ); NOP; GIP_BLOCK_ALL(); NOP; }
#define GIP_ANALYZER_READ_CONTROL(s)  {__asm__ volatile ( " .word 0xec00ce02 \n mov %0, r0 \n " : "=r" (s) ); }
#define GIP_ANALYZER_READ_DATA(s)  {__asm__ volatile ( " .word 0xec00ce02 \n mov %0, r1 \n " : "=r" (s) ); }
#define GIP_ANALYZER_TRIGGER_CONTROL(count,stage_if_true,action_if_true,stage_if_false,action_if_false) { GIP_ANALYZER_WRITE(1, ( (count)<<0) | ((stage_if_true)<<16) | ((action_if_true)<<20) | ((stage_if_false)<<24) | ((action_if_false)<<28)); }
#define GIP_ANALYZER_TRIGGER_MASK(mask) {GIP_ANALYZER_WRITE(2,(mask));}
#define GIP_ANALYZER_TRIGGER_COMPARE(mask) {GIP_ANALYZER_WRITE(3,(mask));}
    GIP_ANALYZER_WRITE(4,0); GIP_BLOCK_ALL(); // mux to 0 - gip memory address and controls
#ifdef USE_LOAD_TRIGGER
    GIP_ANALYZER_WRITE(0,0x001); GIP_BLOCK_ALL(); // analyzer reset, stage 0
    GIP_ANALYZER_WRITE(1,0x00310001); GIP_BLOCK_ALL(); // stage 0 count 1 true->stage 1
    GIP_ANALYZER_WRITE(2,0x83ffffff); GIP_BLOCK_ALL(); // mask rd and address
    GIP_ANALYZER_WRITE(3,0x80000000); GIP_BLOCK_ALL(); // 
    GIP_ANALYZER_WRITE(0,0x101); GIP_BLOCK_ALL(); // analyzer reset, stage 1
    GIP_ANALYZER_WRITE(1,0x00320200); GIP_BLOCK_ALL(); // stage 1 count 512 true&resided->stage 2
    GIP_ANALYZER_WRITE(2,0x00000000); GIP_BLOCK_ALL(); // mask rd and address
    GIP_ANALYZER_WRITE(3,0x00000000); GIP_BLOCK_ALL(); // 
    GIP_ANALYZER_WRITE(0,0x201); GIP_BLOCK_ALL(); // analyzer reset, stage 2
    GIP_ANALYZER_WRITE(1,0x00720000); GIP_BLOCK_ALL(); // stage 2 count 1 true->end
    GIP_ANALYZER_WRITE(2,0x00000000); GIP_BLOCK_ALL(); // mask rd and address
    GIP_ANALYZER_WRITE(3,0x00000000); GIP_BLOCK_ALL(); // 
#else
    GIP_ANALYZER_WRITE(0,0x001); GIP_BLOCK_ALL(); // analyzer reset, stage 0
    GIP_ANALYZER_WRITE(1,0x20210000); GIP_BLOCK_ALL(); // stage 0 count 0 true->stage 1, false->stage 0, store either way
    GIP_ANALYZER_WRITE(2,0xc3ffffff); GIP_BLOCK_ALL(); // mask rd and address
    GIP_ANALYZER_WRITE(3,0x4001fffc); GIP_BLOCK_ALL(); // write of address 0x1fffc - presumably the first stack push
    GIP_ANALYZER_WRITE(0,0x101); GIP_BLOCK_ALL(); // analyzer reset, stage 1 - store 64 of any type
    GIP_ANALYZER_WRITE(1,0x00320040); GIP_BLOCK_ALL(); // stage 1 count 64 true&reside->stage 2
    GIP_ANALYZER_WRITE(2,0x00000000); GIP_BLOCK_ALL(); // mask rd and address
    GIP_ANALYZER_WRITE(3,0x00000000); GIP_BLOCK_ALL(); // 
    GIP_ANALYZER_WRITE(0,0x201); GIP_BLOCK_ALL(); // analyzer reset, stage 2
    GIP_ANALYZER_WRITE(1,0x00720000); GIP_BLOCK_ALL(); // stage 2 count 1 true->end
    GIP_ANALYZER_WRITE(2,0x00000000); GIP_BLOCK_ALL(); // mask rd and address
    GIP_ANALYZER_WRITE(3,0x00000000); GIP_BLOCK_ALL(); // 
    GIP_ANALYZER_WRITE(0,0x80); GIP_BLOCK_ALL();// set circular buffer
#endif
}

void analyzer_trigger( void )
{
#ifdef USE_LOAD_TRIGGER
    GIP_ANALYZER_WRITE(0,0x02); GIP_BLOCK_ALL();// enable analyzer
#else
    GIP_ANALYZER_WRITE(0,0x82); GIP_BLOCK_ALL();// enable analyzer with circular buffer
#endif
    __asm__ volatile( "mov r0, #0 ; ldr r0, [r0] ; .word 0xec007281" : : : "r0" ); // trigger the analyzer
}

/*f test_entry_point
 */
extern int test_entry_point ( void )
{
    int i;

//    analyzer_init();
//    analyzer_trigger();
    NOP;NOP;NOP;NOP;
    GIP_EXTBUS_CONFIG_WRITE( 0x111 );
    NOP;NOP;NOP;NOP;
    GIP_EXTBUS_ADDRESS_WRITE( 0xc0000002 ); // Debug display address
    NOP;NOP;NOP;NOP;
    GIP_EXTBUS_DATA_WRITE( 0xffff );

    microkernel_init();
    timer_int_start();
    MK_INT_DIS();
    for (i=1; i<200; i++) { __asm__ volatile ("mov r0, r0"); };
    dprintf("Test passed");
    GIP_TIMER_DISABLE(); // enable the timer
    return 0;
}
