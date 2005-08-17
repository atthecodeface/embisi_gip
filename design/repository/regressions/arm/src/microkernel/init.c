/*a Includes
 */
#include "gip_support.h"
#include "microkernel.h"
#include "microkernel_thread.h"

/*a Defines
 */
#define __init          __attribute__ ((__section__ (".init.text")))

/*a Types
 */
/*t t_data_fn, t_data_init_fn
 */
typedef void (t_data_init_fn)(unsigned int *ptr, unsigned int size );
typedef unsigned int (t_data_fn)(void);

/*t t_data_type
 */
typedef struct t_data_type
{
    t_data_init_fn *init_fn;
    t_data_fn *data_fn;
} t_data_type;

/*a Global data storage
 */
unsigned int microkernel_thread_vectors[33]; // 0-31 are interrupt vectors, 32 is SWI vector

/*a External functions
 */
/*f microkernel_init
  Input:
  Should be called as thread 0 in ARM mode with all other threads disabled

  Output:
  None

  Effects:
  Sets scheduling mode to be preemptive priority
  Clears events
  Clears semaphores
  Spawns the microkernel thread (thread 1)
  Starts the microkernel thread with its semaphore
  Returns

 */
extern void mkt_unknown_trap( void );
extern void __init microkernel_init( void )
{
    int i;
    unsigned int s;

    // make emulation thread depend on semaphore 0 and set that semaphore
    __asm__ volatile ( " .word 0xec00c86e ; mov r0, #0x11 " );
    GIP_READ_AND_SET_SEMAPHORES( s, 1 ); // set semaphore 0
         
    GIP_ATOMIC_MAX();
    GIP_SET_LOCAL_EVENTS_CFG(0);
    GIP_ATOMIC_MAX_BLOCK();
    GIP_SET_SCHED_CFG( 0x40000 );
    GIP_ATOMIC_MAX_BLOCK();
    GIP_CLEAR_SEMAPHORES(0xf<<(4*MICROKERNEL_THREAD)); // clear microkernels semaphores
    GIP_SET_THREAD( MICROKERNEL_THREAD, microkernel_thread_start, MICROKERNEL_THREAD_INIT_FLAGS); // microkernel thread kicks off on any of its semaphores in ARM mode
    GIP_READ_AND_SET_SEMAPHORES( s, 2<<(4*MICROKERNEL_THREAD) );
    GIP_BLOCK_ALL(); // ends the atomic section and blocks until the semaphores are written

    for (i=0; i<32; i++)
        microkernel_thread_vectors[i] = (unsigned int)(mkt_unknown_trap);

}

extern void microkernel_int_register( int vector, microkernel_int_fn fn )
{
    microkernel_thread_vectors[vector&0x1f] = (unsigned int)fn;
}
