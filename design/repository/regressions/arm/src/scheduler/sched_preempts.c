/*a Documentation
  This test exercises preemption of medium and low priority threads running code that has no atomic sections
  The tests are designed to fully exercise preemption occurring at arbitrary point
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

/*f preempt_high_end
  disable the semaphores for this thread so that it never fires again
 */
typedef struct t_thread_detail // must be 32 bytes long - this is used in the preempt_* routines
{
    unsigned int semaphores_to_clear;
    unsigned int interval;
    unsigned int count;
    unsigned int spare;
} t_thread_detail;
t_thread_detail thread_details[3];

#define PREEMPT_TESTS(a) extern void preempt_ ##a## _2_to_4(void);extern void preempt_ ##a## _5_to_7(void);extern void preempt_ ##a## _8_to_10(void);

PREEMPT_TESTS(simple)
PREEMPT_TESTS(branch)
PREEMPT_TESTS(alus)
PREEMPT_TESTS(mems)

extern void preempt_end_8_to_10( void )
{
    __asm__ volatile ( ".word 0xec00723f" ); // Atomic
    __asm__ volatile ( " mov r0, #0xba; .word 0xec00c52e ; orr r0, r0, #0xaa00 " ); // write LEDs
    __asm__ volatile ( ".word 0xec00c84e ; mov r0, #4 ; .word 0xec00c86e ; mov r0, #0x100 " ); // write selected thread 4, selected thread's flags 0
    __asm__ volatile ( ".word 0xec007285 " ); // Atomic block, so that the write to the thread flags completes; we don't want to deschedule for two clocks after that
    __asm__ volatile ( ".word 0xec007305 " ); // Deschedule
}

extern void preempt_end_5_to_7( void )
{
    __asm__ volatile ( ".word 0xec00723f" ); // Atomic
    __asm__ volatile ( " mov r0, #0xae; .word 0xec00c52e ; orr r0, r0, #0xaa00 " ); // write LEDs
    __asm__ volatile ( ".word 0xec00c84e ; mov r0, #2 ; .word 0xec00c86e ; mov r0, #0x100 " ); // write selected thread 2, selected thread's flags 0
    __asm__ volatile ( ".word 0xec007285 " ); // Atomic block, so that the write to the thread flags completes; we don't want to deschedule for two clocks after that
    __asm__ volatile ( ".word 0xec007305 " ); // Deschedule
}

static unsigned int results_store[3];
extern void preempt_end_2_to_4( void )
{
 // Completed the low priority test - we assume the others have completed (they should have, they are higher priority, and we can always run more low priority iterations to wait)
    __asm__ volatile ( ".word 0xec00723f" ); // Atomic
    __asm__ volatile ( " mov r0, #0xab; .word 0xec00c52e ; orr r0, r0, #0xaa00 " ); // write LEDs
// Record r8, r5, r2 in static space for testing! they should all be 0; <0 means error
    __asm__ volatile ( " ldr r1, =results_store; stmia r1, {r2, r5, r8} " ); // Store results
// return from subroutine should work - it should return us to the end of next_test
    __asm__ volatile ( " mov pc, lr " ); // return from subroutine
}

static int test_number;
char inverse_byte_table[256] = {
//    0x0f, 0x0e, 0x0d, 0x0c, 0x0b, 0x0a, 0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x00,
    0xff, 0xfe, 0xfd, 0xfc, 0xfb, 0xfa, 0xf9, 0xf8, 0xf7, 0xf6, 0xf5, 0xf4, 0xf3, 0xf2, 0xf1, 0xf0,
    0xef, 0xee, 0xed, 0xec, 0xeb, 0xea, 0xe9, 0xe8, 0xe7, 0xe6, 0xe5, 0xe4, 0xe3, 0xe2, 0xe1, 0xe0,
    0xdf, 0xde, 0xdd, 0xdc, 0xdb, 0xda, 0xd9, 0xd8, 0xd7, 0xd6, 0xd5, 0xd4, 0xd3, 0xd2, 0xd1, 0xd0,
    0xcf, 0xce, 0xcd, 0xcc, 0xcb, 0xca, 0xc9, 0xc8, 0xc7, 0xc6, 0xc5, 0xc4, 0xc3, 0xc2, 0xc1, 0xc0,
    0xbf, 0xbe, 0xbd, 0xbc, 0xbb, 0xba, 0xb9, 0xb8, 0xb7, 0xb6, 0xb5, 0xb4, 0xb3, 0xb2, 0xb1, 0xb0,
    0xaf, 0xae, 0xad, 0xac, 0xab, 0xaa, 0xa9, 0xa8, 0xa7, 0xa6, 0xa5, 0xa4, 0xa3, 0xa2, 0xa1, 0xa0,
    0x9f, 0x9e, 0x9d, 0x9c, 0x9b, 0x9a, 0x99, 0x98, 0x97, 0x96, 0x95, 0x94, 0x93, 0x92, 0x91, 0x90,
    0x8f, 0x8e, 0x8d, 0x8c, 0x8b, 0x8a, 0x89, 0x88, 0x87, 0x86, 0x85, 0x84, 0x83, 0x82, 0x81, 0x80,
    0x7f, 0x7e, 0x7d, 0x7c, 0x7b, 0x7a, 0x79, 0x78, 0x77, 0x76, 0x75, 0x74, 0x73, 0x72, 0x71, 0x70,
    0x6f, 0x6e, 0x6d, 0x6c, 0x6b, 0x6a, 0x69, 0x68, 0x67, 0x66, 0x65, 0x64, 0x63, 0x62, 0x61, 0x60,
    0x5f, 0x5e, 0x5d, 0x5c, 0x5b, 0x5a, 0x59, 0x58, 0x57, 0x56, 0x55, 0x54, 0x53, 0x52, 0x51, 0x50,
    0x4f, 0x4e, 0x4d, 0x4c, 0x4b, 0x4a, 0x49, 0x48, 0x47, 0x46, 0x45, 0x44, 0x43, 0x42, 0x41, 0x40,
    0x3f, 0x3e, 0x3d, 0x3c, 0x3b, 0x3a, 0x39, 0x38, 0x37, 0x36, 0x35, 0x34, 0x33, 0x32, 0x31, 0x30,
    0x2f, 0x2e, 0x2d, 0x2c, 0x2b, 0x2a, 0x29, 0x28, 0x27, 0x26, 0x25, 0x24, 0x23, 0x22, 0x21, 0x20,
    0x1f, 0x1e, 0x1d, 0x1c, 0x1b, 0x1a, 0x19, 0x18, 0x17, 0x16, 0x15, 0x14, 0x13, 0x12, 0x11, 0x10,
    0x0f, 0x0e, 0x0d, 0x0c, 0x0b, 0x0a, 0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x00
};
typedef void (t_preempt_test) (void);
typedef struct t_sched_test
{
    int interval_h; t_preempt_test *fn_h;
    int interval_m; t_preempt_test *fn_m;
    int interval_l; t_preempt_test *fn_l;
    int regs_2_to_10[9];
} t_sched_test;
#define NULL ((void *)(0))
static t_sched_test tests[] =
{
    {30, preempt_simple_8_to_10, 30, preempt_simple_5_to_7, 30, preempt_simple_2_to_4, { 4, 1, 1, 4, 2, 2, 4, 3, 3 }}, // this test takes about 80k cycles
    {30, preempt_branch_8_to_10, 30, preempt_branch_5_to_7, 30, preempt_branch_2_to_4, { 4, 1, 1, 4, 2, 2, 4, 3, 3 }}, // this test takes about 145k cycles
    {100, preempt_branch_8_to_10, 100, preempt_branch_5_to_7, 100, preempt_branch_2_to_4, { 4, 1, 1, 4, 2, 2, 4, 3, 3 }}, // interval of 100 gives no overlap, 30 is pretty much full - this test takes about 190k cycles
    {70, preempt_branch_8_to_10, 60, preempt_branch_5_to_7, 50, preempt_branch_2_to_4, { 4, 1, 1, 4, 2, 2, 4, 3, 3 }}, // this test takes about 150k cycles
    {40, preempt_alus_8_to_10, 60, preempt_alus_5_to_7, 50, preempt_alus_2_to_4, { 4, 1, 1, 4, 2, 2, 4, 3, 3 }}, // this test takes about 50k cycles
    {40, preempt_mems_8_to_10, 60, preempt_mems_5_to_7, 50, preempt_mems_2_to_4, { 4, 1, 1, 4, 2, 2, 4, 3, 3 }}, // this test takes about 120k cycles
    {60, preempt_mems_8_to_10, 50, preempt_mems_5_to_7, 30, preempt_mems_2_to_4, { 4, 1, 1, 4, 2, 2, 4, 3, 3 }}, // this test takes about 110k cycles
    {60, preempt_alus_8_to_10, 60, preempt_alus_5_to_7, 60, preempt_alus_2_to_4, { 4, 1, 1, 4, 2, 2, 4, 3, 3 }}, // this test takes about 120k cycles
//    {70, preempt_alus_8_to_10, 60, preempt_alus_5_to_7, 60, preempt_alus_2_to_4, { 20, 1, 1, 20, 2, 2, 20, 3, 3 }}, // this test takes about 440k cycles
//    {70, preempt_alus_8_to_10, 60, preempt_alus_5_to_7, 60, preempt_alus_2_to_4, { 200, 0xfedcba98, 0x76543210, 200, 0xdeadbeef, 0xfeedcafe, 200, 0x12345678, 0x23456789 }}, // this test takes about 3.5M cycles
//    {130, preempt_branch_8_to_10, 110, preempt_mems_5_to_7, 60, preempt_alus_2_to_4, { 60, 0xfedcba98, 0x76543210, 40, 0xdeadbeef, 0xfeedcafe, 40, 0x12345678, 0x23456789 }}, // this test takes about 1.3M cycles, and seems balanced
//    {128, preempt_branch_8_to_10, 112, preempt_mems_5_to_7, 58, preempt_alus_2_to_4, { 300, 0xfedcba98, 0x76543210, 200, 0xdeadbeef, 0xfeedcafe, 200, 0x12345678, 0x23456789 }}, // this test takes about 6.2M cycles, and seems balanced
//    {135, preempt_mems_8_to_10, 115, preempt_alus_5_to_7, 60, preempt_branch_2_to_4, { 60, 0xfedcba98, 0x76543210, 40, 0xdeadbeef, 0xfeedcafe, 40, 0x12345678, 0x23456789 }}, // this test takes about 1.3M cycles, and seems balanced
 {0, NULL, 0, NULL, 0, NULL, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }}
};
static void next_test( void )
{
    unsigned int s;

    GIP_TIMER_DISABLE(); // do this now so that the semaphores can be cleanly cleared

    test_number++;
    GIP_EXTBUS_DATA_WRITE( test_number );
    if (tests[test_number].interval_h<=0)
    {
        test_return(0);
    }

    GIP_READ_AND_CLEAR_SEMAPHORES(s,0xffffffff); // Kill all sempahores to start with
    GIP_SET_LOCAL_EVENTS_CFG( 0xc | 0xa0 | 0x800 ); // set config so that timer 1 (event 0) -> high priority thread 4, timer 2 (event 1) -> medium priority thread 2, timer 3 (event 2) -> low priority thread 0
    thread_details[0].semaphores_to_clear = (1<<16); // event 0 for thread 4 sets semaphore 4*4+0 = 16
    thread_details[0].interval            = tests[test_number].interval_h;     // thread 4 interval of 100*4 clocks
    thread_details[1].semaphores_to_clear = (1<<9);  // event 0 for thread 2 sets semaphore 2*4+1 = 9
    thread_details[1].interval            = tests[test_number].interval_m;     // thread 4 interval of 100*4 clocks
    thread_details[2].semaphores_to_clear = (1<<2);  // event 0 for thread 0 sets semaphore 0*4+2 = 2
    thread_details[2].interval            = tests[test_number].interval_l;     // thread 4 interval of 100*4 clocks
    // Set thread entrypoints and sensitivities
    GIP_SET_THREAD(0,tests[test_number].fn_l,0x41);  // set thread 0 startup to be ARM, on semaphore 2 set, and the entry point
    GIP_SET_THREAD(2,tests[test_number].fn_m,0x21);  // set thread 2 startup to be ARM, on semaphore 9 set, and the entry point
    GIP_SET_THREAD(4,tests[test_number].fn_h,0x11); // set thread 4 startup to be ARM, on semaphore 16 set, and the entry point
    // Set scheduling mode
    __asm__ volatile (".word 0xec00c83e \n mov r0, %0" : : "r" (0x40000) ); // set special (4) reg gip_config (3) to 0x40000 (preemptive, prioritized, no privilege, 4 arm trap semaphore)
    // Set registers
    __asm__ volatile(" mov r0, %0 ; ldmia r0, {r2-r10} " : : "r" (tests[test_number].regs_2_to_10) : "r2", "r3", "r4", "r5", "r6", "r7", "r8", "r9", "r10", "r11", "r12", "r14" );
    // atomically enable timer, set semaphores and then deschedule
    __asm__ volatile ( ".word 0xec00723f" ); // We can atomic a long time as we will flush on the deschedule occurring
    __asm__ volatile ( " .word 0xec00c58e ; mov r0, #0<<31 " );
    __asm__ volatile ( " mov r0, #0x10000 ; orr r0, r0, #0x200; orr r0, r0, #4; .word 0xec00c80e ; mov r0, r0 ; .word 0xec0072bf ; .word 0xec00ce08 ; mov r0, r1 ; .word 0xec0072bf " ); // set our semaphores
    __asm__ volatile ( " mov lr, pc ; .word 0xec007305 ; nop ": : : "r14" ); // Deschedule, but recording lr for return here

    // Return here once the test is done!
    GIP_LED_OUTPUT_CFG_WRITE( 0xaaff );
    GIP_EXTBUS_DATA_WRITE( results_store[0] );
    GIP_BLOCK_ALL();
    GIP_EXTBUS_DATA_WRITE( results_store[1] );
    GIP_BLOCK_ALL();
    GIP_EXTBUS_DATA_WRITE( results_store[2] );
    GIP_BLOCK_ALL();
    if ((results_store[0]!=0) ||
        (results_store[1]!=0) ||
        (results_store[2]!=0) )
    {
        test_return( test_number+1 );
    }
    next_test();
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
    GIP_EXTBUS_DATA_WRITE( 0xffff );

    test_number = -1;
    next_test();

}

/*f test_entry_point
  This function preserves all the entry registers in a static location so we can exit cleanly
 */
static int return_regs[15];
extern int test_entry_point( void )
{
    __asm__ volatile (" ldr r0, =return_regs ; stmia r0, {r1-r14} ; b test_internal" );
    return 0;
    test_internal();
}

/*f test_return
  This function recovers the preserved entry registers from a static location and exits with a return code
 */
static int test_return( int result )
{
    __asm__ volatile (" ldr r1, =return_regs ; ldmia r1, {r1-r14} ; mov pc, lr" );
    return 0;
    return_regs[0] = 0;
}
