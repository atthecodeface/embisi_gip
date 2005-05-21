/*a Includes
 */
#include "gip_support.h"
#include "../common/wrapper.h"

/*a Defines
 */

/*a Test entry point
 */
static void thread_1_entry( void )
{
    unsigned int s;
    GIP_READ_AND_CLEAR_SEMAPHORES(s,1<<7);
    GIP_DESCHEDULE();
}

static void thread_2_entry( void )
{
    unsigned int s;
    GIP_READ_AND_CLEAR_SEMAPHORES(s,1<<8);
    GIP_DESCHEDULE();
}

static void thread_3_entry( void )
{
    unsigned int s;
    GIP_READ_AND_CLEAR_SEMAPHORES(s,1<<12);
    GIP_DESCHEDULE();
}

static void thread_4_entry( void )
{
    unsigned int s;
    GIP_READ_AND_CLEAR_SEMAPHORES(s,1<<16);
    GIP_DESCHEDULE();
}

static void thread_5_entry( void )
{
    unsigned int s;
    GIP_READ_AND_CLEAR_SEMAPHORES(s,1<<20);
    GIP_DESCHEDULE();
}

static void thread_6_entry( void )
{
    unsigned int s;
    GIP_READ_AND_CLEAR_SEMAPHORES(s,1<<24);
    GIP_DESCHEDULE();
}

static void thread_7_entry( void )
{
    unsigned int s;
    GIP_READ_AND_CLEAR_SEMAPHORES(s,1<<28);
    GIP_DESCHEDULE();
}

/*f test_entry_point
 */
extern int test_entry_point()
{
    unsigned int s;
    GIP_READ_AND_CLEAR_SEMAPHORES(s,1<<0);

    __asm__ volatile (".word 0xec00c83e \n mov r0, %0" : : "r" (0x40001) ); // set special (4) reg gip_config (3) to 0x40001 (cooperative, prioritized, no privilege, 4 arm trap semaphore)
    __asm__ volatile (".word 0xec00c83e \n mov r0, %0" : : "r" (0x40000) ); // set special (4) reg gip_config (3) to 0x40000 (preemptive, prioritized, no privilege, 4 arm trap semaphore)


    GIP_SET_THREAD(1,thread_1_entry,0x81); // set thread 1 startup to be ARM, on semaphore 7 set, and the entry point
    GIP_SET_THREAD(2,thread_2_entry,0x11); // set thread 2 startup to be ARM, on semaphore 8 set, and the entry point
    GIP_SET_THREAD(3,thread_3_entry,0x11); // set thread 3 startup to be ARM, on semaphore 12 set, and the entry point
    GIP_SET_THREAD(4,thread_4_entry,0x11); // set thread 4 startup to be ARM, on semaphore 16 set, and the entry point
    GIP_SET_THREAD(5,thread_5_entry,0x11); // set thread 5 startup to be ARM, on semaphore 20 set, and the entry point
    GIP_SET_THREAD(6,thread_6_entry,0x11); // set thread 6 startup to be ARM, on semaphore 24 set, and the entry point
    GIP_SET_THREAD(7,thread_7_entry,0x11); // set thread 7 startup to be ARM, on semaphore 28 set, and the entry point

    GIP_READ_AND_SET_SEMAPHORES(s,(1<<7) | (1<<8) | (1<<12) | (1<<16) | (1<<20) | (1<<24) | (1<<28) );

//    GIP_READ_AND_SET_SEMAPHORES(s,1<<7);
//    GIP_READ_AND_SET_SEMAPHORES(s,1<<8);
//    GIP_READ_AND_SET_SEMAPHORES(s,1<<12);
//    GIP_READ_AND_SET_SEMAPHORES(s,1<<16;
//    GIP_READ_AND_SET_SEMAPHORES(s,1<<20);
//    GIP_READ_AND_SET_SEMAPHORES(s,1<<24);
//    GIP_READ_AND_SET_SEMAPHORES(s,1<<28);

    GIP_DESCHEDULE();

}

