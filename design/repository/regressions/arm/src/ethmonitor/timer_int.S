/*a Includes
 */
#include "gip_macros.h"

    .macro preserve_gip_scheduling_conditions address
    .word 0xec00c1f0
    mov r0, #\address // r31 = 0x11000
    .word 0xec00de01
    str r0, [r15] // perserve r0 in 11000
    mov r0, #\address
    stmib r0, {r1-r3}
    add r1, r0, #0x10
    .word 0xec00c80e
    str r2, [r1], #4 // store semaphores in storage
    mov r2, #0
0:  
    // set select thread
    .word 0xec00c84e
    mov r0, r2
    // block
    .word 0xec007281
    .word 0xec007281
    .word 0xec007281
    // read thread pc and config, and store
    .word 0xec00c85e
    str r2, [r1], #4
    .word 0xec00c86e
    str r2, [r1], #4
    add r2, r2, #1
    cmp r2, #8
    bne 0b
    ldmia r0, {r0-r3}
    .endm
    
#define LINUX_BUG    
#ifndef LINUX_BUG        
     .text
    .align 4
    .global timer_int
timer_int:  
     .word 0xec00c1f5 // r31 <= periph[24] (timer value)
    mov r0, r8

    .word 0xec00d591
    add r0, r15, #0x30000 // periph[25] = r31+0x30000, and clears passed // 0x40 is too tight

    swi 0xf60201 // clear semaphore 1>>>(2*2)

//__asm__ volatile ( " .word 0xec00c80e ; mov r0, #0x1<<28 " ); // write to spec0 (sems to clr)
//__asm__ volatile ( " .word 0xec007281 ; .word 0xec00c1f8 ; mov r0, r0 " ); // r31 <= spec0 (read and clear semaphores)
//__asm__ volatile ( " .word 0xec00c80e ; mov r0, #0x4<<4 " ); // write to spec0 (sems to set)

    .word 0xec00d111
    orr r1, r1, #1 // take some time setting our interrupt bit (r17 bit 0)

//    __asm__ volatile ( " .word 0xec00c1f8 ; mov r0, r1 " ); // r31 <= spec1 (read and clear semaphores)
//    __asm__ volatile ( " .word 0xec007305 " ); // deschedule
    swi 0xc60040
    mov r0, r0
    mov r0, r0

#else


    .text
    .align 4
    .global timer_int
timer_int:  
    preserve_gip_scheduling_conditions 0x11000
    .word 0xec00c1f5
    mov r0, r8  // r31 <= periph[24] (timer value)

    .word 0xec00d591
    add r0, r15, #0x20000 // periph[25] = timer value+0x80, and clears passed // 0x40 is too tight

    gip_clear_semaphores 1, 28 // clear semaphore from timer 

    .word 0xec00d111
    orr r1, r1, #1 // take some time setting our interrupt bit (r17 bit 0) */

    swi 0xc60040 // don't set r15 or r16, but do set ukernel semaphore and deschedule - this fails with an extra read/write swi before it, provided there is only one nop afterwards. Why do we care about succeeding nops? This does not seem to effect cmd_test
    nop
    .word 0xec00d111
    bic r1, r1, #1
    mov r0, sp
#endif    