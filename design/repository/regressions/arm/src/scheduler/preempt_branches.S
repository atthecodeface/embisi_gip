    // This code is basically designed to decrement regdec while executing every branch (conditional and unconditional) possible
    // We do not corrupt r14 - we preserve it in temp2
    .macro branches, regdec, temp1, temp2
    mov \temp1, lr
    teq \regdec, \regdec // set Z flag
    b 8f                // forward unc branch
0:                      // backward cnd branch not taken
1:                      // backward cnd bl not taken
    sub \regdec, \regdec, #0x1000
2:                      // backward unc b
    add \regdec, \regdec, #0x10
    bne 0b // backward cnd branch not taken
    bl 9f  // forward unc bl
    sub \regdec, \regdec, #0x1000
3:                      // backward unc bl
    add \regdec, \regdec, #0x5
    blne 0b // backward cnd bl not taken
    beq 10f  // forward cnd b taken
    sub \regdec, \regdec, #0x1000
4:                      // backward cnd b taken
    add \regdec, \regdec, #0xf
    bne 6f // forward cnd branch not taken
    bleq 11f  // forward cnd b taken
    sub \regdec, \regdec, #0x1000
5:                      // backward cnd bl taken
    add \regdec, \regdec, #0x7-1
    blne 7f // forward cnd bl not taken
    b 12f  // done
    sub \regdec, \regdec, #0x1000

6:                      // forward cnd branch not taken
7:                      // forward cnd bl not taken
    sub \regdec, \regdec, #0x1000
8:                      // forward unc b
    sub \regdec, \regdec, #0x10
    b 2b  // backward unc branch
    sub \regdec, \regdec, #0x1000
9:                      // forward unc bl
    sub \regdec, \regdec, #0x5
    b 3b  // backward unc bl
    sub \regdec, \regdec, #0x1000
10:                      // forward cnd b taken
    sub \regdec, \regdec, #0xf
    beq 4b  // backward cnd branch taken
    sub \regdec, \regdec, #0x1000
11:                      // forward cnd bl taken
    sub \regdec, \regdec, #0x7
    bleq 5b  // backward cnd bl taken
    sub \regdec, \regdec, #0x1000

12:     // Done
    mov lr, \temp1
    .endm


    // This code is basically designed to decrement regdec and manipulate temp1 and temp2
    // It basically does regdec += temp1, regdec+1 if CS, regdec-- if CS, regdec-=temp2, regdec=-regdec if mi, regdec =-regdec if mi, regdec-=1, regdec-=temp1, regdec+=temp2
    .macro alus, regdec, temp1, temp2
    adds \regdec, \regdec, \temp1
    adc \regdec, \regdec, #0
    subcs \regdec, \regdec, #1
    subs \regdec, \regdec, \temp2, ror #14
    eormi \regdec, \regdec, #0x80000000
    sub \regdec, \regdec, #1
    eormi \regdec, \regdec, #0x80000000
    sub \regdec, \regdec, \temp1
    adds \regdec, \regdec, \temp2, ror #14
    add \temp1, \temp1, \temp1, lsr #20
    mov \temp1, \temp1, ror #1
    add \temp2, \temp2, \temp2, lsr #27
    mov \temp2, \temp2, ror #3
    .endm

        
    // This code is basically designed to decrement regdec and manipulate temp1 and temp2, utilizing a decrementing byte memory space to do some manipulations
    // It basically does regdec += temp1, regdec+1 if CS, regdec-- if CS, regdec-=temp2, regdec=-regdec if mi, regdec =-regdec if mi, regdec-=1, regdec-=temp1, regdec+=temp2
    .macro mems, regdec, temp1, temp2
    ldr  \temp1, =inverse_byte_table
    and \temp2, \regdec, #0xff
    ldrb \temp2, [\temp1, \temp2]
    eor \temp2, \temp2, \regdec, ROR #8
    str \temp2, [sp, #-4]!
    mvn \temp2, \temp2
    ldmia sp!, {\temp1}
    eor \temp1, \temp1, #0xff
    eor \temp2, \temp1, \regdec, ROR #8 // temp2 should now be the bottom byte of regdec
    bic \regdec, \regdec, #0xff
    sub \temp2, \temp2, #1
    add \regdec, \temp2, \regdec
    .endm

        
    .global thread_details

    // Clear the semaphore that started this thread
    // Read the timer, add an amount, write back comparator
    .macro clear_semaphore_restart_timer, offset, timer
    .word 0xec00720d       // atomic for 6 ARM instructions
    ldr r1, =thread_details
    .word 0xec00c80e       // extrd spec0
    ldr r0, [r1, #\offset]! // spec0 (semclr) = [r1+offset], r1+=offset
    .word 0xec00ce05       // extrm periph(16-31)
    mov r0, r8             // r0 <= periph[24] (timer value)
    ldr r1, [r1, #4]       // r1 = [r1+4]
    .word 0xec00c59e+(\timer<<4)       // extrd periph[25]/26/27
    add r0, r0, r1
    .word 0xec00de08 // extrm spec0(0-15)
    mov r0, r0       // r0 <= spec0 (read and clear semaphores)
    .endm
    
    .text
	.align 4

  	.global preempt_simple_8_to_10
preempt_simple_8_to_10:
    clear_semaphore_restart_timer 0, 0
    subs r8, r8, #1
    .word 0xcc007305            // Deschedule if GT, atomic for 2, not blocking
    b preempt_end_8_to_10       // Atomic with the deschedule so that preempt cannot occur during the deschedule

	.global preempt_simple_5_to_7
preempt_simple_5_to_7:  
    clear_semaphore_restart_timer 16, 1
    subs r5, r5, #1
    .word 0xcc007305            // Deschedule if GT, atomic for 2, not blocking
    b preempt_end_5_to_7        // Atomic with the deschedule so that preempt cannot occur during the deschedule

	.global preempt_simple_2_to_4
preempt_simple_2_to_4:  
    clear_semaphore_restart_timer 32, 2
    subs r2, r2, #1
    .word 0xcc007305            // Deschedule if GT, atomic for 2, not blocking
    b preempt_end_2_to_4        // Atomic with the deschedule so that preempt cannot occur during the deschedule



    
  	.global preempt_branch_8_to_10
preempt_branch_8_to_10:
    clear_semaphore_restart_timer 0, 0
    branches r8, r9, r10
    cmp r8, #0
    .word 0xcc007305            // Deschedule if GT, atomic for 2, not blocking
    b preempt_end_8_to_10       // Atomic with the deschedule so that preempt cannot occur during the deschedule

	.global preempt_branch_5_to_7
preempt_branch_5_to_7:  
    clear_semaphore_restart_timer 16, 1
    branches r5, r6, r7
    cmp r5, #0
    .word 0xcc007305            // Deschedule if GT, atomic for 2, not blocking
    b preempt_end_5_to_7        // Atomic with the deschedule so that preempt cannot occur during the deschedule

	.global preempt_branch_2_to_4
preempt_branch_2_to_4:  
    clear_semaphore_restart_timer 32, 2
    branches r2, r3, r4
    cmp r2, #0
    .word 0xcc007305            // Deschedule if GT, atomic for 2, not blocking
    b preempt_end_2_to_4        // Atomic with the deschedule so that preempt cannot occur during the deschedule


    
    
    .global preempt_alus_8_to_10
preempt_alus_8_to_10:
    clear_semaphore_restart_timer 0, 0
    alus r8, r9, r10
    cmp r8, #0
    .word 0xcc007305            // Deschedule if GT, atomic for 2, not blocking
    b preempt_end_8_to_10       // Atomic with the deschedule so that preempt cannot occur during the deschedule

	.global preempt_alus_5_to_7
preempt_alus_5_to_7:  
    clear_semaphore_restart_timer 16, 1
    alus r5, r6, r7
    cmp r5, #0
    .word 0xcc007305            // Deschedule if GT, atomic for 2, not blocking
    b preempt_end_5_to_7        // Atomic with the deschedule so that preempt cannot occur during the deschedule

	.global preempt_alus_2_to_4
preempt_alus_2_to_4:  
    clear_semaphore_restart_timer 32, 2
    alus r2, r3, r4
    cmp r2, #0
    .word 0xcc007305            // Deschedule if GT, atomic for 2, not blocking
    b preempt_end_2_to_4        // Atomic with the deschedule so that preempt cannot occur during the deschedule


    
            
    .global preempt_mems_8_to_10
preempt_mems_8_to_10:
    clear_semaphore_restart_timer 0, 0
    mems r8, r9, r10
    cmp r8, #0
    .word 0xcc007305            // Deschedule if GT, atomic for 2, not blocking
    b preempt_end_8_to_10       // Atomic with the deschedule so that preempt cannot occur during the deschedule

	.global preempt_mems_5_to_7
preempt_mems_5_to_7:  
    clear_semaphore_restart_timer 16, 1
    mems r5, r6, r7
    cmp r5, #0
    .word 0xcc007305            // Deschedule if GT, atomic for 2, not blocking
    b preempt_end_5_to_7        // Atomic with the deschedule so that preempt cannot occur during the deschedule

	.global preempt_mems_2_to_4
preempt_mems_2_to_4:  
    clear_semaphore_restart_timer 32, 2
    mems r2, r3, r4
    cmp r2, #0
    .word 0xcc007305            // Deschedule if GT, atomic for 2, not blocking
    b preempt_end_2_to_4        // Atomic with the deschedule so that preempt cannot occur during the deschedule
