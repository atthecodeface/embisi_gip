//#define DEBUG
//#define VERBOSE

.text
	.align 4
	.global alu_call
alu_call:
                           /* r0 points to a test vector structure, r1 points to a results structure */
    stmfd r13!, {r0-r12, lr}
        /* We must set the flags Z N C V (r2 thru r5), then set R0=a_in(r6), R1=b_in(r7), then call the function
        Then store the flags and results in r9 (which preserved incoming r1)
        */
    mov r9, r1

    ldmia r0, { r2, r3, r4, r5,  r6, r7,  r8 }

    /*b Turn verbose on
    */
#ifdef VERBOSE
    .word 0xf00000a2
#endif
    
    /*b First clear all flags, get r0 so we can set overflow if we need to
        */
    mov r0, #0
    adds r0, r0, #0x40000000
    teq r5, #0
    addnes r0, r0, r0 /* Overflow r0 if required */
    mvn r0, #0
    teq r4, #0
    movnes r0, r0, lsl #1 /* Set carry if required, don't touch overflow */
    teq r3, #0
    bne set_n
    /* Okay, we want a clear n, and maybe z. Seems easy enough. If r2 is 0 then we want a movs r2, #1. If r2 is nonzero we want a movs r2, #0. So prep r0, then do a mov */
    mov r0, #1
    teq r2, #0
    movne r0, #0
    movs r0, r0
    b flags_ready

set_n:
    /* Okay, we want to set n. Cannot see a way for N to be set with Z set:  how can it be zero and negative? So all we can do is set N, clear Z */
    movs r0, #0x80000000

flags_ready:        
    mov r0, r6
    mov r1, r7

    /*b Now call the function, with link so we get back here
    */
    mov lr, pc
    mov pc, r8

    /*b nop four times to ensure we have a clean pipeline,  then turn verbose off
    */
#ifdef VERBOSE
    .word 0xf00000a4
    .word 0xf00000a4
    .word 0xf00000a4
    .word 0xf00000a4
    .word 0xf00000a3
#endif
 
    /*b Now preserve the flags
    */
    mov r2, #0
    mov r3, #0
    mov r4, #0
    mov r5, #0
    moveq r2, #1
    movmi r3, #1
    movcs r4, #1
    movvs r5, #1

    /*b Get result in r6 for store
    */
    mov r6, r0

    /*b Store results
    */
    stmia r9, { r2, r3, r4, r5,  r6 }
        
    /*b dump regs - pipeline is always clean enough for this
    */
#ifdef DEBUG
    .word 0xf00000a1
#endif
        
    ldmfd r13!, {r0-r12, pc}
