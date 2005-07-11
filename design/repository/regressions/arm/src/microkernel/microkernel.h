#include "gip_support.h"

/*a Defines
  A trap is (atomic)
  set trap semaphore
  set r16 to be trap number
  (optionally set r15 to be something?)
  deschedule
 */
#define MICROKERNEL_THREAD (1)
#define MK_TRAP_RETURN_TO_STACK_FRAME (1024+0)
#define MK_TRAP_GET_IRQ_STORES (1024+3)
#define MK_TRAP_SET_IRQ_HANDLER (1024+4)
#define MK_TRAP_SET_SWI_HANDLER (1024+5)
#define MK_TRAP(a) { GIP_ATOMIC_MAX(); GIP_SET_SEMAPHORES_ATOMIC(1<<(4*MICROKERNEL_THREAD)); __asm__ volatile (" .word 0xec00c10e ; mov r0, %0 ; .word 0xec00c0fe ; mov r0, pc ; .word 0xec007305 " : : "r" (a) ); NOP; }
#define MK_TRAP2(a,b,c) { GIP_ATOMIC_MAX(); GIP_SET_SEMAPHORES_ATOMIC(1<<(4*MICROKERNEL_THREAD)); __asm__ volatile (".word 0xec00c10e ; mov r0, %0 ; " : : "r" (a) ); __asm__ volatile ( " mov r0, %0 ; mov r1, %1 ; .word 0xec00c0fe ; mov r0, pc ; .word 0xec007305 " : : "r" (b), "r"(c) : "r0", "r1" ); NOP; }
#define MK_INT_DIS() { __asm__ volatile ( " .word 0xec00723f ; .word 0xec00c80e ; mov r0, #0x20 ; .word 0xec0072bf ; .word 0xec00ce08 ; mov r0, r0 ; .word 0xec007281 " : : : "r0" ); }
#define MK_INT_EN() { __asm__ volatile ( " .word 0xec00723f ; .word 0xec00c80e ; mov r0, #0x20 ; .word 0xec0072bf ; .word 0xec00ce08 ; mov r0, r1 ; .word 0xec007281 " : : : "r0" ); }
#define MK_INT_DIS_GET(s) { __asm__ volatile ( " .word 0xec00723f ; .word 0xec00c80e ; mov r0, #0x20 ; .word 0xec0072bf ; .word 0xec00ce08 ; mov %0, r0 ; .word 0xec007281 " : "=r" (s) ); }
#define MK_INT_EN_GET(s) { __asm__ volatile ( " .word 0xec00723f ; .word 0xec00c80e ; mov r0, #0x20 ; .word 0xec0072bf ; .word 0xec00ce08 ; mov %0, r1 ; .word 0xec007281 " : "=r"(s) ); }
#define MK_INT_GET(s) { __asm__ volatile ( " .word 0xec00723f ; .word 0xec00ce08 ; mov %0, r0 ; .word 0xec007281 " : "=r"(s) ); }
#define MK_RETURN_TO_STACK_FRAME() { MK_TRAP(MK_TRAP_RETURN_TO_STACK_FRAME); }
#define MK_INT_CLR(a) { __asm__ volatile (" .word 0xec00c11e ; .word 0xec00d11e ; bic r1, r1, %0" : : "r" (1<<a) ); }

/*a Functions
 */
typedef void (microkernel_int_fn)( void );
extern void microkernel_init( void );
extern void microkernel_int_register( int vector, microkernel_int_fn fn );
