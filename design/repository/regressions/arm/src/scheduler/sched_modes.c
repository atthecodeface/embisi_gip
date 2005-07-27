/*a Documentation
  This test runs 8 threads, using thread 0 as the base test thread
  It actually consists of a number of tests internally, each of which...
    starts up a set of threads
    a thread runs by engaging an LED, setting some new semaphores, then updating a checksum, then descheduling
  The test runs in a specific scheduling mode
  Depending on the order of threads and what semaphores they set internally, different checksums are produced
 */

/*a Includes
 */
#include "gip_support.h"
#include "../common/wrapper.h"

/*a Defines
 */

/*a Test entry point
 */
static int test_return( int result );
static int checksum;
static int test_stage;
static unsigned int *next_sem;
#define THREAD(thrd,sem,csum_add) \
static void thread_ ## thrd ## _entry( void )\
{ \
    unsigned int s; \
    {__asm__ volatile (".word 0xec007219 ; ldr r0, =next_sem ; ldr r1, [r0] ; ldr r2, [r1], #4 ; cmp r2, #0; beq 0f ; str r1, [r0] ; .word 0xec00c80e ; mov r0, r2 ; mov r1, r1 ; mov r1, r1 ; .word 0xec00ce08 ; mov r2, r1 ; 0:" ); } \
    GIP_LED_OUTPUT_CFG_WRITE( 0xaaaa | (3<<(thrd*2)) ); \
    NOP; NOP; NOP; NOP; NOP; NOP; \
    {__asm__ volatile (".word 0xec007209 ; ldr r0, =checksum ; ldr r1, [r0] ; mov r1, r1, ror #28 ; add r1, r1, # "csum_add"; str r1, [r0] " ); } \
    NOP; NOP; NOP; NOP; NOP; NOP; \
    GIP_READ_AND_CLEAR_SEMAPHORES(s,1<<(sem)); \
    NOP; NOP; NOP; NOP; NOP; NOP; \
    GIP_DESCHEDULE(); \
}

THREAD(1,7,"1");
THREAD(2,8,"2");
THREAD(3,12,"3");
THREAD(4,16,"4");
THREAD(5,20,"5");
THREAD(6,24,"6");
THREAD(7,28,"7");

typedef struct t_sched_test
{
    unsigned int config;
    unsigned int checksum;
    unsigned int initial_sems;
    unsigned int sems_to_set[16];
} t_sched_test;
static t_sched_test tests[] =
{
    { 0x40003, 0x01234567, 0xffffffff, {0} }, // cooperative round robin, start all, so we expect 1, 2, 3, 4, 5, 6, 7, 0
    { 0x40003, 0x07654321, 1<<28, {1<<24, 1<<20, 1<<16, 1<<12, 1<<8, 8<<4, 1<<0, 0x0} }, // cooperative round robin, start 7->6 6->5, 5->4, 4->3 3->2, 2->1, 1->0 so we expect 7,6,5,4,3,2,1,0
    { 0x40003, 0x07234572, 1<<28, {0x11100, 2, 0x10100100, 2, 2, 2, 1, 0x0} }, // cooperative round robin, start 7->234, 2, 3->257, 4, 5, 7, 2->0

    { 0x40001, 0x07654321, 0xffffffff, {0} }, // cooperative priority, start all, so we expect 7, 6, 5, 4, 3, 2, 1, 0
    { 0x40001, 0x0743752, 1<<28, {0x11100, 2, 0x10100100, 2, 2, 1, 0x0} }, // cooperative priority, start 7->234, 4, 3->257, 7, 5, 2->0
    { 0x40001, 0x07432, 1<<28, {0x11100, 2, 2, 1, 0} }, // cooperative priority, start 7->234, 4, 3, 2->0

    { 0x40000, 0x07654321, 0xffffffff, {0} }, // preemptive, start all, so we expect 7, 6, 5, 4, 3, 2, 1, 0
    { 0x40000, 0x00007523, 1<<8, {0x10101000, 2, 2, 1, 0x0} }, // preemptive priority, start 2i->753, 7, 5, r2, 3->0
    { 0x40000, 0x62654396, 1<<8, {0x10101000, 2, 1<<24, 2, 0x01110181, 0x0} }, // preemptive priority, start 2i->753, 7, 5->6, 6, r2, 3i->654210, 6, 5, 4, r3, 2, 1, 0

    { 0, 0, 0, {0, 1, 2, 3} }
};
static void next_test( void );
static void thread_0_reentry( void )
{
    unsigned int s;
    GIP_LED_OUTPUT_CFG_WRITE( 0xaaaa | (3<<(0*2)) ); \
    GIP_EXTBUS_DATA_WRITE( checksum );
    GIP_READ_AND_CLEAR_SEMAPHORES(s,0xffffffff); // We need to kill semaphores here in case we are running round-robin cooperatively, and we turn back to preemptive in a moment!

    if (checksum!=tests[test_stage].checksum)
    {
        GIP_EXTBUS_DATA_WRITE(tests[test_stage].checksum);
        test_return(test_stage+1);
    }
    next_test();
}

/*f next_test
 */
static void next_test( void )
{
    unsigned int s;
    test_stage++;
    if (tests[test_stage].config==0)
    {
        test_return(0);
    }
    __asm__ volatile (".word 0xec00c83e \n mov r0, %0" : : "r" (tests[test_stage].config) ); // set special (4) reg gip_config (3) to 0x40000 (preemptive, prioritized, no privilege, 4 arm trap semaphore)

    next_sem = tests[test_stage].sems_to_set;
    checksum = 0;

    GIP_READ_AND_SET_SEMAPHORES(s,tests[test_stage].initial_sems ); // set all the semaphores at once
    GIP_DESCHEDULE(); // this thread will reschedule at thread_0_reentry
}

/*f test_internal
 */
static void test_internal( void )
{
    unsigned int s;
    GIP_READ_AND_CLEAR_SEMAPHORES(s,0xffffffff);

    GIP_LED_OUTPUT_CFG_WRITE( 0xaaaa );

    NOP;NOP;NOP;NOP;
    GIP_EXTBUS_CONFIG_WRITE( 0x111 );
    NOP;NOP;NOP;NOP;
    GIP_EXTBUS_ADDRESS_WRITE( 0xc0000002 ); // Debug display address
    NOP;NOP;NOP;NOP;
    GIP_EXTBUS_DATA_WRITE( checksum );

    GIP_SET_THREAD(0,thread_0_reentry,0x11); // set thread 1 startup to be ARM, on semaphore 0 set, and the entry point
    GIP_SET_THREAD(1,thread_1_entry,0x81); // set thread 1 startup to be ARM, on semaphore 7 set, and the entry point
    GIP_SET_THREAD(2,thread_2_entry,0x11); // set thread 2 startup to be ARM, on semaphore 8 set, and the entry point
    GIP_SET_THREAD(3,thread_3_entry,0x11); // set thread 3 startup to be ARM, on semaphore 12 set, and the entry point
    GIP_SET_THREAD(4,thread_4_entry,0x11); // set thread 4 startup to be ARM, on semaphore 16 set, and the entry point
    GIP_SET_THREAD(5,thread_5_entry,0x11); // set thread 5 startup to be ARM, on semaphore 20 set, and the entry point
    GIP_SET_THREAD(6,thread_6_entry,0x11); // set thread 6 startup to be ARM, on semaphore 24 set, and the entry point
    GIP_SET_THREAD(7,thread_7_entry,0x11); // set thread 7 startup to be ARM, on semaphore 28 set, and the entry point

    test_stage = -1;
    next_test();

}

/*f test_entry_point
 */
static int return_regs[15];
extern int test_entry_point( void )
{
    __asm__ volatile (" ldr r0, =return_regs ; stmia r0, {r1-r14} ; b test_internal" );
    return 0;
    test_internal();
}

/*f test_return
 */
static int test_return( int result )
{
    __asm__ volatile (" ldr r1, =return_regs ; ldmia r1, {r1-r14} ; mov pc, lr" );
    return 0;
    return_regs[0] = 0;
}
