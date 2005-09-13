    #include "gip_macros.h"
    
#define EMULATION_THREAD (0)
#define MICROKERNEL_THREAD (1)
#define TRAP_SEM (0x1)
#define INTEN_SEM (0x2)
#define HWINT_SEM (0x4)

#define TRAP_SEM_IN_READ (TRAP_SEM<<(4*MICROKERNEL_THREAD))
#define INTEN_SEM_IN_READ (INTEN_SEM<<(4*MICROKERNEL_THREAD))
#define HWINT_SEM_IN_READ (HWINT_SEM<<(4*MICROKERNEL_THREAD))

#define Trap_ReturnToStackFrame (1024+0)
#define Trap_SetIrqHandler (1024+4)
#define Trap_SetSwiHandler (1024+5)

#define STACK_FRAME_SIZE (17*4)

#if 0
#define DEBUG(a) gip_utximm a
#else
#define DEBUG(a) 
#endif
        
    // This microkernel thread uses its semaphore 0 as its ARM trap semaphore, semaphore 1 as interrupts enable, sempahore 2 as hardware interrupt
    // It also uses r15/r16 for trap handling, r17 for hardware interrupt status, and r18 for the system stack pointer
    
    // The most common actions to occur are:    
    // user mode emulation (interrupts always enabled):  ARM trap semaphore, hardware interrupt
    // basic system mode emulation (interrupts possibly enabled):   ARM trap semaphore (system exit), interrupts disable, interrupts enable, interrupts save and disable, interrupts restore and enable, hardware interrupt
    // system mode emulation (interrupts definitely disabled, hardware was pending):   interrupts save and disable, interrupts restore disabled, interrupts restore enabled, interrupts enable, hardware interrupt

    // Interrupts are disabled by the emulation thread by clearing the interrrupt enable semaphore
    // Interrupts are enabled by the emulation thread by setting the interrupt enable semaphore
    // A hardware interrupt is pending if the hardware interrupt semaphore is set

    // In user mode emulation the microkernel is watching the ARM trap semaphore and the hardware interrupt semaphore

    // In system mode emulation (interrupts possibly enabled) the microkernel is watching the ARM trap semaphore and the hardware interrupt semaphore.
    // Disabling interrupts (and save/disable) is done by clearing the interrrupt enable semaphore
    // If a hardware interrupt occurs the microkernel checks the interrupt enable semaphore and transitions to disabled if it is clear

    // In system mode emulation (interrupts disabled, hardware may be pending) the microkernel is watching the ARM trap semaphore and the interrupt enable semaphore.
    // If the interrupt enable semaphore becomes set then

    // If the microkernel believes interrupts are enabled and a hardware interrupt goes off then it checks the disable semaphore:  if they are disabled, then the interrupt is left pending and the microkernel enters disabled mode
    // If the microkernel believes interrupts are disabled then it should not schedule on a hardware interrupt going off

    // The basic mechanism the microkernel uses for entering the SWI handler is:
    // r18 -> caller r0-r14, return pc (from r15 in trap), flags (bit 31=> set means from user, bit 30=> set means interrupts were enabled, others ignored)
    // r0-r7 are from the caller
    // r8 = SWI number (from r16 in trap)
    // r9-r12 are from the caller
    // r13 = r18
    // r14 is from the caller
    // pc is the SWI handler
    // interrupts DISABLED
    
    // The basic mechanism the microkernel uses for entering the interrupt handler is: 
    // r18 -> preempted r0-r14, pc, flags (bit 31=> from user, bit 30=> set as interrupts must have been enabled, bottom 3 bits preempted flags)
    // r0-r14 are from the preempted process
    // pc is the interrupt handler

    // Return from system call/irq takes:   
    // r13->register store
    //  if return to system the r18 <= sp from the store
    //  if return to system then interrupt enable/disable taken from flags
    //  if return to user the interrupts are enabled
    // Execution continues at register store r15


    .text
	.align 4

    // The microkernel thread starts here when it is initialized
    // It must clear its starting semaphore and then mark the emulation thread as running with interrupts disabled and in 'system' mode
    // Interrupts are enabled if the microkernel can kick off on the interrupt semaphore (i.e. if it will be scheduled when that is set)
    // The microkernel is always watching for the trap semaphore from the main emulation thread

   	.global microkernel_thread_start
microkernel_thread_start:
    // preserve r0
    stmfd sp!, {r0}

    mov r0, #0x11000
    gip_extrd 0x13
    mov r3, r0

    gip_extrd 0x14
    mov r4, #0

    // clear the semaphores that effect us - this clears interrupt enable and hw interrupt (as well as trap)
    gip_clear_semaphores 0xf, MICROKERNEL_THREAD*4

    // clear any pending interrupts
    gip_mov_full_imm 0x11, #0 // r17 <= 0, clear interrupt sources
        
    // make us depend on trap semaphore, but not interrupt as interrupts are disabled by default
    gip_extrd 0x85
    adr r0, mkt_basic_system_mode // set our restart address
    gip_mov_full_imm 0x86, #((TRAP_SEM | HWINT_SEM)<<4)|1 // set our flag dependencies and restart mode

    // recover r0
    ldmfd sp!, {r0}

    // deschedule, blocking first for cleanliness
    gip_deschedule_block




    
    // Macro called to preserve r8 from any system entry point
    .macro system_preserve_r8
    str r8, [r13, #-4] // put r8 on to next system stack entry - we cannot be preempted by any users of the stack, so we need not change r13
    .endm

    // Macro called to recover r8 from any system entry point where no stack frames were used
    .macro system_recover_r8
    ldr r8, [r13, #-4]
    .endm

    // Macro called to preserve r8 from any user entry point - we need to put it on the system stack (which should be unchanged from last return to user mode)
    .macro user_preserve_r8
    gip_extrn 18
    str r8, [r2, #-4] // put r8 on to next system stack entry - we cannot be preempted by any users of the stack, so we need not change r18 - indeed, doing so would mess up stack frame building
    .endm

    // Macro called to recover r8 from an exit where no stack frames were used
    .macro user_recover_r8
    gip_extrn 18
    ldr r8, [r2, #-4]
    .endm

    // Macro to set restart to be for disabled interrupts, system mode
    .macro restart_at_int_dis_system
    gip_extrd 0x86
    mov r0, #((TRAP_SEM | INTEN_SEM)<<4)|1 // set our flag dependencies and restart mode
    gip_restart_at mkt_int_dis_system_mode
// - (.+8)) & 0xffe ) .word 0xec007000 | ( (    
    .endm

    // Macro to set restart to be for enabled interrupts, system mode
    .macro restart_at_basic_system
    gip_extrd 0x86
    mov r0, #((TRAP_SEM | HWINT_SEM)<<4)|1 // set our flag dependencies and restart mode
    gip_restart_at mkt_basic_system_mode
    .endm

    // Macro to set restart to be for enabled interrupts, user mode
    .macro restart_at_user
    gip_extrd 0x86
    mov r0, #((TRAP_SEM | HWINT_SEM)<<4)|1 // set our flag dependencies and restart mode
    gip_restart_at mkt_user_mode
    .endm

    .macro circular_buffer reg, tmp
    gip_extrm 0x13
    mov \tmp, r3 // \tmp = r19
    str \reg, [\tmp], #4
    bic \tmp, \tmp, #0x200
    gip_extrd 0x13
    mov r3, \tmp // r19 = \tmp
    .endm
        
    
    
        
    // Code for semaphore firing with interrupts possibly enabled or disabled
    // This can be entered when the trap semaphore goes off OR when a hardware interrupt occurs
    // If the trap semaphore has occurred then it MUST be handled first - the emulated code will have descheduled
mkt_basic_system_mode:
    DEBUG('S')
    system_preserve_r8
    gip_gpio_instat r8
    tst r8, #2
    bne mkt_unknown_trap
    gip_read_and_clear_semaphores TRAP_SEM, 4*MICROKERNEL_THREAD, 8
    tst r8, #TRAP_SEM_IN_READ
    beq mkt_basic_system_mode_interrupt

mkt_basic_system_mode_trap: // trap semaphore fired when in system mode (interrupts enabled or disable) - r8 = semaphores, [r13,#-4] = original r8
    // about half the traps are system calls, the rest are return from system calls or return from interrupt
    // must set r15 and mode in return block
    gip_extrn 16
    tst r0, #0x400 // test trap number in r16
    beq mkt_basic_system_mode_trap_swi // if a SWI, enter that handler
    // fall through
    DEBUG('T')
mkt_basic_system_mode_trap_mk:  // microkernel trap occurred - handle appropriately - r8 = semaphores, [r13, #-4] = original r8
    gip_extrn 16
    ands r8, r0, #0xff // r8 = r16 & 0xff
    bne mkt_basic_system_mode_trap_despatch
    // fall through
mkt_basic_system_mode_trap_return: // microkernel trap to return to given frame in r13, with correct mode and enables - trap semaphore clear, int enable semaphore high or low
    // if interrupts are to be reenabled and there is an interrupt pending then we have a stack frame in r13 ready and we can just handle the next interrrupt
    // r13 -> return frame
    //  frame is r0-r12, r13, r14, pc, flags
    //  if flags indicates user, then enable interrupts (set semaphore), recover sp from stack, put r13 after stack pop in to r18, and restart at user
    //  if flags indicates system, then enable or disable interrupts based on flags, set r13 valid after stack pop, and restart at system

    DEBUG('r')
   
    // disable interrupts
    gip_clear_semaphores INTEN_SEM, 4*MICROKERNEL_THREAD
    ldr r1, [r13, #16*4] // get flags
    // enable interrupts if required
    movs r2, r1, lsr #30 // r2 = 0 for system ints disable, 1 for system ints enable, else user
    beq mkt_basic_system_mode_trap_return_ints_disabled
    gip_extrn 0x11
    cmp r1, #0
    bne mkt_take_interrupt
    gip_set_semaphores INTEN_SEM, 4*MICROKERNEL_THREAD
mkt_basic_system_mode_trap_return_ints_disabled: // semaphores correct, r1=flags, r2 = 0/1 for system else user, r13->return frame
    tst r2, #2
    bne mkt_basic_system_mode_trap_return_user_mode // handle user mode and system mode differently - they return to different code for the microkernel thread, and user mode sets r18=sp, system puts sp in frame so it recovers cleanly
    restart_at_basic_system
    add r3, r13, #STACK_FRAME_SIZE // r3 = system stack pointer after we pop off our frame - store it in the frame so it pops correctly
    str r3, [r13, #13*4]
    b mkt_basic_system_mode_trap_return_exit
mkt_basic_system_mode_trap_return_user_mode:    
    gip_extrd 18
    add r2, r13, #STACK_FRAME_SIZE // r18 = system stack pointer, after we pop off our frame - only needed for user mode, as otherwise r13 is valid
    ldr r0, [r13, #15*4]
    //tst r0, #0x6000000
    //blne mkt_unknown_trap
    circular_buffer r0, r8
    circular_buffer r13, r8
    gip_extrm 20
    mov r0, r4
    cmp r0, #0
    beq 0f
    subs r0, r0, #1
    gip_extrd 32 // global APB 0 - analyzer config
    moveq r0, #2
    //bleq mkt_unknown_trap
    gip_extrd 20
    mov r0, r0
0:      
    restart_at_user
mkt_basic_system_mode_trap_return_exit:     
    // stop mkt from scheduling
    gip_mov_full_imm 0x86, #1 // set our flag dependencies to nothing (just ARM mode)
    // set emulation thread to restart at emt_return_to_frame
    gip_atomic 2
    gip_mov_full_imm 0x84, #EMULATION_THREAD // spec4 <= EMULATION_THREAD
    gip_extrd 0x85
    add r0, pc, #emt_return_to_frame - (.+8) + 1 // spec5:    add 1 so that the restart address is for the selected thread, not mkt
    // deschedule, blocking first for cleanliness
    gip_deschedule_block
   
    // This code is scheduled for the emulation thread to return to a stack frame
    // r1 = flags (in bottom 4 bits)
    // r13 -> stack frame with r0-r14,pc
    // If interrupts occur between the mkt descheduling and this code running there is no effect
    // basically this code is an extension of mkt but running in the emulation thread - we effectively single-thread it
emt_return_to_frame:    
    // atomic max - we flush at the end anyway
    gip_atomic 31
    // atomically set the flag dependencies for mkt to be hardware int and trap
    gip_mov_full_imm 0x84, #MICROKERNEL_THREAD // spec4 = thread
    gip_extrd 0x86
    ldr r0, emt_frame_restart_flags  // spec6 = flags
    // set the flags from r1     writeflags r16
//    // WORKAROUND FOR EXTCMD NOT WORKING:   set us as gp mode (special_alu_mode bottom 2 bits set, which means same for repeat count)
       gip_mov_full_imm 0x88, #3
       gip_atomic_block 31
       gip_atomic_block 31 // have to block twice as it blocks at rf stage, and we want the decode to be blocked
    .word 0xec00cc0e // extrdrm rd=none
//    .word 0xec00ee30 // extcmd 1110ccccsaoobbbb - c condition, s sign, a acc, o op (3 for gp mode), b burst
    .word 0xec000f01 // alureg op15 rd=none, rm=1 - ttttooooddddmmmm type=0, op=0xf, d=0, m=1
       gip_mov_full_imm 0x88, #0
       gip_atomic_block 31
    // recover r0-r14, pc from the stack
    ldmfd sp, {r0-r15}
    nop

emt_frame_restart_flags:  .word    0x100 | ((TRAP_SEM | HWINT_SEM)<<4) | 1
    //  

       
mkt_basic_system_mode_trap_despatch:  // microkernel trap occurred - handle appropriately - r8 = trap number&0xff, [r13, #-4] = original r8, trap semaphore clear
    cmp r8, #Trap_SetIrqHandler-1024
    beq mkt_basic_system_mode_trap_set_irq_handler
    cmp r8, #Trap_SetSwiHandler-1024
    beq mkt_basic_system_mode_trap_set_swi_handler
    blne mkt_unknown_trap

    // System mode, interrupts possibly enabled or disabled, trap to set irq (r0) handler to r1 - return PC in R15
mkt_basic_system_mode_trap_set_irq_handler:
    and r0, r0, #0x1f
    ldr r8, = microkernel_thread_vectors
    str r1, [r8, r0, lsl #2]
    system_recover_r8
    b mkt_basic_system_mode_trap_done_return_to_caller

    // System mode, interrupts possibly enabled or disabled, trap to set irq (r0) handler to r1 - return PC in R15
mkt_basic_system_mode_trap_set_swi_handler:
    ldr r8, = microkernel_thread_vectors
    str r0, [r8, #128]
    system_recover_r8
    b mkt_basic_system_mode_trap_done_return_to_caller

   

    // System mode (or user mode), interrupts possibly enabled or disabled, microkernel trap performed - r15 = pc to return to, other registers for return
mkt_basic_system_mode_trap_done_return_to_caller:    
    DEBUG('R')
    // Note that if we get an interrupt at this point that forces us to be scheduled, then we will NOT be preempting the emulation thread!!!

    // SO... stop set mkt to restart in the right place, then stop mkt from scheduling
    gip_restart_at mkt_basic_system_mode
    gip_mov_full_imm 0x86, #1 // set our flag dependencies to nothing (just ARM mode)
    // set emulation thread to restart at emt_return_to_caller
    gip_atomic 2
    gip_mov_full_imm 0x84, #EMULATION_THREAD // spec4 <= EMULATION_THREAD
    gip_extrd 0x85
    add r0, pc, #emt_return_to_caller - (.+8) + 1 // spec5:    add 1 so that the restart address is for the selected thread, not mkt
    // deschedule, blocking first for cleanliness
    gip_deschedule_block
    
    // This code is scheduled for the emulation thread to return to a stack frame
    // r15 = pc to return to, other registers were all set
    // If interrupts occur between the mkt descheduling and this code running there is no effect
    // basically this code is an extension of mkt but running in the emulation thread - we effectively single-thread it
emt_return_to_caller:   
    // atomic max - we flush at the end anyway
    gip_atomic 31
    // atomically set the flag dependencies for mkt to be hardware int and trap
    gip_mov_full_imm 0x84, #MICROKERNEL_THREAD // spec4 = thread
    gip_extrd 0x86
    ldr r0, emt_caller_restart_flags  // spec6 = flags
    // Now jump to the correct location
    gip_extrm 0x0f
    mov pc, r15

emt_caller_restart_flags:  .word    0x100 | ((TRAP_SEM | HWINT_SEM)<<4) | 1


    
    // System mode, interrupts possibly enabled or disabled, build stack frame using r13 and call SWI handler - [r13, #-4] = original r8
mkt_basic_system_mode_trap_swi:
    DEBUG('W')
    ldr r8, [r13, #-4]
    str r13, [r13, #-STACK_FRAME_SIZE+13*4] // preserve r13 on the stack frame
    sub r13, r13, #STACK_FRAME_SIZE // r13 <= r13-stack_frame_size (after all, we are now in system mode(!))
    stmia r13, {r0-r12} // preserve r0-r12
    str r14, [r13, #14*4] // preserve r14
    gip_extrd 15 // rd is used to store data in stores, so extend that to r15
    str r15, [r13, #15*4] // preserve r15

    gip_extrn 0x80
    tst r0, #INTEN_SEM_IN_READ // test for interrupts enabled
    mov r8, #0<<31
//    orrne r8, r8, #1<<30 // set bit 30 to indicate interrupts enabled
    orr r8, r8, #1<<30 // set bit 30 to indicate interrupts enabled
    str r8, [r13, #16*4] // preserve flags

    ldr r8, = microkernel_thread_vectors
    gip_extrd 15
    ldr r0, [r8, #128] // r15 = swi handler vector

    gip_extrm 16
    mov r7, r0 // r7 = SWI number

    b mkt_basic_system_mode_trap_done_return_to_caller


    
    
        
    // System mode, not a trap - must be a hardware interrupt: check if interrupts are disabled, then check interrupt sources - r8 = semaphores
mkt_basic_system_mode_interrupt:
    tst r8, #INTEN_SEM_IN_READ
    beq mkt_basic_system_mode_interrupt_interrupts_disabled
    gip_extrm 0x11
    movs r8, r1 // r8 <= r17, interrupt sources
    beq mkt_basic_system_mode_interrupt_no_interrupts
    DEBUG('I')

    // interrupts enabled, interrupt to service - create stack frame
    ldr r8, [r13, #-4]
    str r13, [r13, #-STACK_FRAME_SIZE+13*4] // preserve r13 on the stack frame
    sub r13, r13, #STACK_FRAME_SIZE // r13 <= r13-stack_frame_size
    stmia r13, {r0-r12} // preserve r0-r12
    str r14, [r13, #14*4] // preserve r14
    gip_extrd 0x8c // rd is used to store data in stores, so extend that to spec12 (preempted pc l)
    str r12, [r13, #15*4]

    gip_extrn 0x8e // spec14 (preempted flags l)
    orr r8, r14, #1<<30 // system mode, interrupts enabled
    str r8, [r13, #16*4] // preserve flags

    b mkt_take_interrupt


    
    // System mode, not a trap, no interrupt sources claim to be pending, so we can clear the hardware interrupt, check sources again (in case one came in since between reading the hwint bit and clearing it)
mkt_basic_system_mode_interrupt_no_interrupts:
    DEBUG('N')
    gip_clear_semaphores (HWINT_SEM | TRAP_SEM), 4*MICROKERNEL_THREAD
    gip_atomic_block 1
    gip_extrm 0x11
    movs r8, r1 // r8 <= r17, interrupt sources
    bne mkt_basic_system_mode_interrupt // system mode, not a trap, hw interrupt still pending - check interrupt enable and handle interrupts
    restart_at_basic_system
    // System mode, all interrupts handled, semaphores cleared, not a trap (we came from the emulation thread so we must have preempted it and can just deschedule - but recover r8 first)
    system_recover_r8
    // deschedule, blocking first for cleanliness
    gip_deschedule_block
    nop

    
    // System mode, not a trap, probably hw interrupt, but interrupts are now disabled - set scheduling dependency to be trap semaphore and inten semaphore, set restart at ints disabled mode, then recover r8 and continue emulation thread
mkt_basic_system_mode_interrupt_interrupts_disabled:
    DEBUG('D')
    restart_at_int_dis_system
    system_recover_r8
    // deschedule, blocking first for cleanliness
    gip_deschedule_block
    nop



    
    
    // System mode interrupts disabled, either a trap or interrupts have been reenabled - if its a trap, emulation thread is descheduled: if interrupts are reenabled then emulation thread must have been preempted
mkt_int_dis_system_mode:
    DEBUG('s')
    system_preserve_r8

    gip_gpio_instat r8
    tst r8, #2
    bne mkt_unknown_trap

    gip_read_and_clear_semaphores TRAP_SEM, 4*MICROKERNEL_THREAD, 8
    tst r8, #TRAP_SEM_IN_READ
    bne mkt_basic_system_mode_trap // for now we go back to the system mode handler, as it copes with interrupts enabled or disabled
//fall through        
    // System mode interrupts disabled, not a trap - interrupts must have been enabled
mkt_int_dis_system_mode_interrupts_enabled: 
    // To go back to basic mode we need to change the flag dependencies and restart address (for now - this may be overridden later anyway)
    b mkt_basic_system_mode_interrupt


    
    
    // Code for semaphore firing in user mode - interrupts should be enabled
    // This can be entered when the trap semaphore goes off OR when a hardware interrupt occurs
    // If the trap semaphore has occurred then it MUST be handled first - the emulated code will have descheduled
mkt_user_mode:
    DEBUG('U')
    user_preserve_r8

    gip_gpio_instat r8
    tst r8, #2
    bne mkt_unknown_trap

    gip_read_and_clear_semaphores TRAP_SEM, 4*MICROKERNEL_THREAD, 8
    tst r8, #TRAP_SEM_IN_READ
    beq mkt_user_mode_interrupt

mkt_user_mode_trap: // trap semaphore fired when in user mode (*must be SWI*) - r8 = semaphores, [r18,#-4] = original r8
    DEBUG('T')
    gip_extrn 18
    str r13, [r2, #-STACK_FRAME_SIZE+13*4] // preserve r13 (user) on the stack frame
    gip_extrn 18
    sub r13, r2, #STACK_FRAME_SIZE // r13 (system) <= r18-stack_frame_size (after all, we are now in system mode(!))
    user_recover_r8
    stmia r13, {r0-r12} // preserve r0-r12
    str r14, [r13, #14*4] // preserve r14
    gip_extrd 15 // rd is used to store data in stores, so extend that to r15
    str r15, [r13, #15*4] // preserve r15

    mov r8, #3<<30 // set flags to be user mode, interrupts enabled
    str r8, [r13, #16*4]

    ldr r8, = microkernel_thread_vectors
    gip_extrd 15
    ldr r0, [r8, #128] // r15 = swi handler vector
    
    gip_extrm 16
    mov r7, r0 // r7 = SWI number
    
    b mkt_basic_system_mode_trap_done_return_to_caller // we can return as if from system mode, coz we now are





    
    
        
    // User, not a trap - must be a hardware interrupt, so check interrupt sources - r8 = semaphores, [r18,#-4] = orig r8
mkt_user_mode_interrupt:
    gip_extrm 0x11
    movs r8, r1 // r8 <= r17, interrupt sources
    beq mkt_user_mode_interrupt_no_interrupts

    DEBUG('I')
    // interrupts enabled, interrupt to service - create stack frame
    gip_extrn 18
    str r13, [r2, #-STACK_FRAME_SIZE+13*4] // preserve r13 on the stack frame
    gip_extrn 18
    sub r13, r2, #STACK_FRAME_SIZE // r13 <= r18-stack_frame_size (after all, we are now in system mode(!))
    user_recover_r8
    stmia r13, {r0-r12} // preserve r0-r12
    str r14, [r13, #14*4] // preserve r14
    gip_extrd 0x8c // rd is used to store data in stores, so extend that to spec12 (preempted pc l)
    str r12, [r13, #15*4]

    gip_extrn 0x8e // spec14 (preempted flags l)
    orr r8, r14, #3<<30 // user mode, interrupts enabled
    str r8, [r13, #16*4] // preserve flags

    b mkt_take_interrupt
    
    // User mode, not a trap, no interrupt sources claim to be pending, so we can clear the hardware interrupt, check sources again (in case one came in since between reading the hwint bit and clearing it)
mkt_user_mode_interrupt_no_interrupts:
    DEBUG('N')
    gip_clear_semaphores (HWINT_SEM | TRAP_SEM), 4*MICROKERNEL_THREAD
    gip_atomic_block 1
    gip_extrm 0x11
    movs r8, r1 // r8 <= r17, interrupt sources
    bne mkt_user_mode_interrupt // user mode, not a trap, hw interrupt still pending - handle interrupts
    restart_at_user
    // User mode, no interrupts to be handled, semaphores cleared, preempted
    user_recover_r8
    // deschedule, blocking first for cleanliness
    gip_deschedule_block
    nop

    
    
    


    

    // r17 is the interrupts, nonzero
    // r13 (system sp) contains a complete stack frame ready to return to
mkt_take_interrupt:
    // Change the emulated thread's PC to be the interrupt vector
    gip_extrm 0x11
    mov r9, r1
    ldr r8, = microkernel_thread_vectors
0:  
    ldr r7, [r8], #4 // vector 0
    movs r9, r9, lsr #1
    bcc 0b

    gip_atomic 2
    gip_mov_full_imm 0x84, #EMULATION_THREAD // spec4 <= EMULATION_THREAD
    gip_extrd 0x85
    orr r0, r7, #1 // set restart address for emulated thread, spec5 <= pc | 1

    // change to interrupt disabled mode - clear INTEN semaphore, set restart to be mkt_int_dis_system_mode dependent on trap and inten
    gip_clear_semaphores (HWINT_SEM | INTEN_SEM), 4*MICROKERNEL_THREAD
    restart_at_int_dis_system

    // deschedule, blocking first for cleanliness
    gip_deschedule_block
    nop



        
        
   	.global mkt_unknown_trap
   // fatal unknown trap - reboot!
mkt_unknown_trap:
    ldr r0, =0x0
    strb r0, [r0]
    mov r0, #0x11000
    add r0, r0, #0x200
    stmia r0, {r0-r14}
    gip_extrd 15
    str r0, [r0, #15*4]
    gip_extrd 16
    str r0, [r0, #16*4]
    gip_extrd 17
    str r0, [r0, #17*4]
    gip_extrd 18
    str r0, [r0, #18*4]
    gip_extrd 19
    str r0, [r0, #19*4]
    gip_extrd 20
    str r0, [r0, #20*4]
    gip_extrd 21
    str r0, [r0, #11*4]
    gip_extrd 22
    str r0, [r0, #22*4]
    gip_extrd 23
    str r0, [r0, #23*4]
    gip_extrd 24
    str r0, [r0, #24*4]
    gip_extrd 25
    str r0, [r0, #25*4]
    gip_extrd 26
    str r0, [r0, #26*4]
    gip_extrd 27
    str r0, [r0, #27*4]
    gip_extrd 28
    str r0, [r0, #28*4]
    gip_extrd 29
    str r0, [r0, #29*4]
    gip_extrd 30
    str r0, [r0, #30*4]
    gip_extrd 31
    str r0, [r0, #31*4]
    gip_utximm 'M'
    gip_utximm 'K'
    gip_utximm 'T'
    gip_utximm ' '
    gip_utximm 'U'
    gip_utximm 'N'
    gip_utximm 'K'
    gip_utximm '\n'

    .word 0xec00c58e // disable the timer
     mov r0, #1<<31 

     mov pc, #0
