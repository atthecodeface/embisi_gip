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
/*f timer_int
  The order of operations is important, as the timer passed bit gets cleared when its write completes, and that then flows through the local semaphores which can take (!) 8 cycles to 'uneffect' our semaphore
 */
static void timer_int( void )
{
    __asm__ volatile ( " .word 0xec00c1f5 ; mov r0, r8 " );  // r31 <= periph[24] (timer value)
    __asm__ volatile ( " .word 0xec00c59e ; .word 0xec00d1fe ; add r0, r15, #0x80 " ); // periph[25] = timer value+0x80, and clears passed // 0x40 is too tight
    __asm__ volatile ( " .word 0xec00c80e ; mov r0, #0x1<<28 " ); // write to spec0 (sems to clr)
    __asm__ volatile ( " .word 0xec007281 ; .word 0xec00c1f8 ; mov r0, r0 " ); // r31 <= spec0 (read and clear semaphores)
    __asm__ volatile ( " .word 0xec00c80e ; mov r0, #0x4<<4 " ); // write to spec0 (sems to set)
    __asm__ volatile ( " .word 0xec00c11e ; .word 0xec00d11e ; orr r1, r1, #1 " ); // take some time setting our interrupt bit (r17 bit 0)
    __asm__ volatile ( " .word 0xec00c1f8 ; mov r0, r1 " ); // r31 <= spec1 (read and clear semaphores)
    __asm__ volatile ( " .word 0xec007305 " ); // deschedule
}

/*f timer_operation
 */
static int led_cfg;
static void timer_operation( void )
{
    MK_INT_CLR(0);
    led_cfg ^= 0x5555;
    GIP_LED_OUTPUT_CFG_WRITE( led_cfg );
    MK_RETURN_FROM_INT_TO_SYS();
}

/*f timer_int_start
 */
static void timer_int_start ( void )
{
    unsigned int s;

    led_cfg = 0xaaaa;
    microkernel_int_register( 0, timer_operation );
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

/*f test_entry_point
 */
extern int test_entry_point ( void )
{
    int i;

    NOP;NOP;NOP;NOP;
    GIP_EXTBUS_CONFIG_WRITE( 0x111 );
    NOP;NOP;NOP;NOP;
    GIP_EXTBUS_ADDRESS_WRITE( 0xc0000002 ); // Debug display address
    NOP;NOP;NOP;NOP;
    GIP_EXTBUS_DATA_WRITE( 0xffff );

    microkernel_init();
    timer_int_start();
    GIP_EXTBUS_DATA_WRITE( 0x0000 );
    for (i=0; i<200; i++) NOP;
    GIP_TIMER_DISABLE();
    GIP_EXTBUS_DATA_WRITE( 0x1111) ;
    for (i=0; i<200; i++) NOP;
    GIP_EXTBUS_DATA_WRITE( 0x2222) ;
    return 0;
}
