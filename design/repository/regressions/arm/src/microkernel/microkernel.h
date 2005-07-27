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
#define MK_TRAP(a) { __asm__ volatile (" swi %1" : : "i" (a) ); }
#define MK_TRAP2(a,b,c) { register long __r0 __asm__("r0") = (long)(b); register long __r1 __asm__("r1") = (long)(c); register long __res_r0 __asm__("r0"); \
                        long __res; __asm__ __volatile__ ( "swi %1" : "=r" (__res_r0) : "i" (a), "r" (__r0),"r" (__r1) : "lr");__res = __res_r0; }
#define MK_INT_DIS() { __asm__ volatile ( " swi 0xf60020 " ); }
#define MK_INT_EN()  { __asm__ volatile ( " swi 0xe60020 " ); }
#define MK_INT_DIS_GET(s) { __asm__ volatile ( " swi 0xfe0020 ; mov %0, r0 " : "=r" (s) : : "r0" ); }
#define MK_INT_EN_GET(s) { __asm__ volatile ( " swi 0xee0020 ; mov %0, r0 " : "=r" (s) : : "r0" ); }
#define MK_INT_GET(s) { __asm__ volatile ( " .word 0xec00ce08 ; mov %0, r0 " : "=r"(s) ); }
#define MK_RETURN_TO_STACK_FRAME() { __asm__ volatile ( " swi 0x800400 "); }
#define MK_INT_CLR(a) { __asm__ volatile (" .word 0xec00d111 ; bic r1, r1, %0" : : "r" (1<<a) ); }

//#define MK_TRAP(a) { GIP_ATOMIC_MAX(); GIP_SET_SEMAPHORES_ATOMIC(1<<(4*MICROKERNEL_THREAD)); __asm__ volatile (" .word 0xec00c10e ; mov r0, %0 ; .word 0xec00c0fe ; mov r0, pc ; .word 0xec007305 " : : "r" (a) ); NOP; }
//#define MK_TRAP2(a,b,c) { GIP_ATOMIC_MAX(); GIP_SET_SEMAPHORES_ATOMIC(1<<(4*MICROKERNEL_THREAD)); __asm__ volatile (".word 0xec00c10e ; mov r0, %0 ; " : : "r" (a) ); __asm__ volatile ( " mov r0, %0 ; mov r1, %1 ; .word 0xec00c0fe ; mov r0, pc ; .word 0xec007305 " : : "r" (b), "r"(c) : "r0", "r1" ); NOP; }
//#define MK_INT_DIS() { __asm__ volatile ( " .word 0xec00723f ; .word 0xec00c80e ; mov r0, #0x20 ; .word 0xec0072bf ; .word 0xec00ce08 ; mov r0, r0 ; .word 0xec007281 " : : : "r0" ); }
//#define MK_INT_EN() { __asm__ volatile ( " .word 0xec00723f ; .word 0xec00c80e ; mov r0, #0x20 ; .word 0xec0072bf ; .word 0xec00ce08 ; mov r0, r1 ; .word 0xec007281 " : : : "r0" ); }
//#define MK_INT_DIS_GET(s) { __asm__ volatile ( " .word 0xec00723f ; .word 0xec00c80e ; mov r0, #0x20 ; .word 0xec0072bf ; .word 0xec00ce08 ; mov %0, r0 ; .word 0xec007281 " : "=r" (s) ); }
//#define MK_INT_EN_GET(s) { __asm__ volatile ( " .word 0xec00723f ; .word 0xec00c80e ; mov r0, #0x20 ; .word 0xec0072bf ; .word 0xec00ce08 ; mov %0, r1 ; .word 0xec007281 " : "=r"(s) ); }
//#define MK_INT_GET(s) { __asm__ volatile ( " .word 0xec00723f ; .word 0xec00ce08 ; mov %0, r0 ; .word 0xec007281 " : "=r"(s) ); }
//#define MK_RETURN_TO_STACK_FRAME() { MK_TRAP(MK_TRAP_RETURN_TO_STACK_FRAME); }
//#define MK_INT_CLR(a) { __asm__ volatile (" .word 0xec00d111 ; bic r1, r1, %0" : : "r" (1<<a) ); }

/*a Functions
 */
typedef void (microkernel_int_fn)( void );
extern void microkernel_init( void );
extern void microkernel_int_register( int vector, microkernel_int_fn fn );
