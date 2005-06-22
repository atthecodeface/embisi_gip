#define MICROKERNEL_THREAD (1)
#define TRAP_SEM (0x1)
#define INTEN_SEM (0x2)
#define HWINT_SEM (0x4)

#define TRAP_SEM_IN_READ (TRAP_SEM<<(4*MICROKERNEL_THREAD))
#define INTEN_SEM_IN_READ (INTEN_SEM<<(4*MICROKERNEL_THREAD))
#define HWINT_SEM_IN_READ (HWINT_SEM<<(4*MICROKERNEL_THREAD))

#define Trap_ReturnFromInterruptToUser (0)
#define Trap_ReturnFromInterruptToSystem (1)
#define Trap_EnterUserMode (2)

    // This microkernel thread uses its semaphore 0 as its ARM trap semaphore, semaphore 1 as interrupts enable, sempahore 2 as hardware interrupt
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

    .macro preserve_registers
    .word 0xec007209 // atomic for 4 instructions
    .word 0xec00c12e
    mov r2, r0 // r18 <= r0
    ldr r0, = microkernel_thread_register_store
    stmib r0, {r1-r5}
    .word 0xec00c12e // for stores, rd is the data to store, so extend that
    str r2, [r0]
    .endm
    
    .macro recover_registers
    ldr r0, = microkernel_thread_register_store
    ldmia r0, {r0-r5}
    .endm
    

    
    
    
            
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
    
    // clear the semaphores that effect us - this clears interrupt enable and hw interrupt (as well as trap)
    .word 0xec00723f // atomic 
    .word 0xec00c80e // write semaphores to clear
    mov r0, #0xf << (MICROKERNEL_THREAD*4)
    .word 0xec0072bf // atomic block
    .word 0xec00ce08
    mov r0, r0 // read and clear semaphores, corrupting r0

    // clear any pending interrupts
    .word 0xec00c11e
    mov r1, #0 // r17 <= 0, clear interrupt sources
        
    // make us depend on trap semaphore, but not interrupt as interrupts are disabled by default
    .word 0xec00c85e
    adr r0, mkt_basic_system_mode // set our restart address
    .word 0xec00c86e
    mov r0, #((TRAP_SEM | HWINT_SEM)<<4)|1 // set our flag dependencies and restart mode

    // recover r0
    ldmfd sp!, {r0}

    // deschedule, blocking first for cleanliness
    .word 0xec007385
    nop


    
    
    
        
    // Code for semaphore firing with interrupts possibly enabled or disabled
    // This can be entered when the trap semaphore goes off OR when a hardware interrupt occurs
    // If the trap semaphore has occurred then it MUST be handled first - the emulated code will have descheduled
mkt_basic_system_mode:
    preserve_registers // we do not yet have register mapping
    .word 0xec00723f // atomic 
    .word 0xec00c80e // write semaphores to clear
    mov r0, #TRAP_SEM_IN_READ
    .word 0xec007285 // atomic block
    .word 0xec00ce08
    mov r0, r0 // read and clear semaphores reading them in to r0
    tst r0, #TRAP_SEM_IN_READ
    beq mkt_basic_system_mode_not_trap

mkt_basic_system_mode_trap: 
    // trap semaphore fired when in system mode
    // can be:  
    // ReturnFromInterruptToUser
    // ReturnFromInterruptToSystem
    // EnterUserMode
    // Trap number is in R16
    .word 0xec00d10e
    and r1, r0, #0xff // r1 = r16 & 0xff
    cmp r1, #Trap_ReturnFromInterruptToUser
    beq mkt_basic_system_mode_trap_return_from_interrupt_to_user
    cmp r1, #Trap_ReturnFromInterruptToSystem
    beq mkt_basic_system_mode_trap_return_from_interrupt_to_system
    cmp r1, #Trap_EnterUserMode
    bne mkt_unknown_trap
// fall through
mkt_basic_system_mode_trap_enter_user_mode:
    // r15 = pc to return to, other registers were all set
    // we need to point emulation thread at r15, and it can then run
    .word 0xec007205 // Atomic while we play with spec4/5
    .word 0xec00c84e
    mov r0, #EMULATION_THREAD // spec4 <= EMULATION_THREAD
    .word 0xec00c85e
    .word 0xec00d0fe
    orr r0, r15, #1 // spec5 <= pc to return to | 1
    // emulation thread is now schedulable with the correct address - mkt trap semaphore needs to be cleared
    b mkt_basic_user_mode_not_trap

    
    // System mode, interrupts possibly enabled or disabled, trap to return from interrupt to system mode
    // emulation thread is descheduled, pc and flags are in the interrupt storage space
    // set our return point to be mkt_basic_system_mode
mkt_basic_system_mode_trap_return_from_interrupt_to_system:
    .word 0xec007000 | ( (mkt_basic_system_mode - (.+8)) & 0xffe )
    b mkt_return_from_interrupt

    
    // System mode, interrupts possible enabled or disabled, trap to return from interrupt to user mode
mkt_basic_system_mode_trap_return_from_interrupt_to_user:   
    b mkt_unknown_trap

    
    // System mode, not a trap - should be a hardware interrupt: check if interrupts are disabled, then check interrupt sources
mkt_basic_system_mode_not_trap:
    tst r0, #INTEN_SEM_IN_READ
    beq mkt_basic_system_mode_not_trap_interrupts_disabled
    .word 0xec00ce01
    movs r0, r1 // r0 <= r17, interrupt sources
    beq mkt_basic_system_mode_not_trap_no_interrupts
    // Okay, we have interrupts enabled and a pending interrupt and we preempted the emulation thread
    // Copy out the emulated thread's PC and flags for later returns
    ldr r0, = microkernel_thread_interrupted_data+8 // r0 and r1 are already corrupted, and in our reserve space
    stmia r0, {r2-r14}
    ldr r2, = microkernel_thread_register_store // recover r0 and r1 into r1 and r2
    ldmia r2, {r1-r2}
    stmdb r0!, {r1-r2} // store original r0 and r1
    .word 0xec00c8ee // rd is used to store data in stores, so extend that to spec14 (preempted flags l)
    str r1, [r0, #15*4]
    .word 0xec00c8ce // rd is used to store data in stores, so extend that to spec12 (preempted pc l)
    str r1, [r0, #16*4]
    // Change the emulated thread's PC to be the interrupt vector
    ldr r0, = microkernel_thread_vectors
    ldr r0, [r0] // ignore which status bit for now - only use vector 0
    .word 0xec007205 // atomic block
    .word 0xec00c84e 
    mov r0, #0 // select emulation thread (spec4)
    .word 0xec00c85e
    orr r0, r0, #1 // set restart address for selected thread
    // change to interrupt disabled mode - 
    .word 0xec00723f // atomic 
    .word 0xec00c80e // write semaphores to clear (inten)
    mov r0, #INTEN_SEM_IN_READ
    .word 0xec007203 // atomic block
    .word 0xec00ce08
    mov r0, r0 // read and clear semaphores reading them in to r0
    .word 0xec00c86e
    mov r0, #((TRAP_SEM | INTEN_SEM)<<4)|1 // set our flag dependencies and restart mode
    .word 0xec007000 | ( (mkt_int_dis_system_mode - (.+8)) & 0xffe )
    recover_registers
    // deschedule, blocking first for cleanliness
    .word 0xec007385
    nop
    
    
    // System mode, not a trap, no interrupt sources claim to be pending, so we can clear the hardware interrupt, check sources again (in case one came in since between reading the hwint bit and clearing it)
mkt_basic_system_mode_not_trap_no_interrupts:   
    .word 0xec00723f // atomic 
    .word 0xec00c80e // write semaphores to clear
    mov r0, #HWINT_SEM_IN_READ | TRAP_SEM_IN_READ
    .word 0xec0072bf // atomic block
    .word 0xec00ce08
    mov r0, r0 // read and clear semaphores reading them in to r0
    .word 0xec007283 // block, ending atomic
    .word 0xec00ce01
    movs r1, r1 // r1 <= r17, interrupt sources
    bne mkt_basic_system_mode_not_trap // system mode, not a trap, hw interrupt still pending - check interrupt enable and handle interrupts
    // System mode, all interrupts handled, semaphores cleared, not a trap (we came from the emulation thread so we must have preempted it and can just deschedule - but recover registers first)
    recover_registers
    // deschedule, blocking first for cleanliness
    .word 0xec007385
    nop

    
    // System mode, not a trap, probably hw interrupt, but interrupts are now disabled - set scheduling dependency to be trap semaphore and inten semaphore, set restart at ints disabled mode, then recover registers and continue emulation thread
mkt_basic_system_mode_not_trap_interrupts_disabled:
    .word 0xec00c86e
    mov r0, #((TRAP_SEM | INTEN_SEM)<<4)|1 // set our flag dependencies and restart mode
    .word 0xec007000 | ( (mkt_int_dis_system_mode - (.+8)) & 0xffe )
    recover_registers
    // deschedule, blocking first for cleanliness
    .word 0xec007385
    nop


    
    // System mode interrupts disabled, either a trap or interrupts have been reenabled - if its a trap, emulation thread is descheduled: if interrupts are reenabled then emulation thread must have been preempted
mkt_int_dis_system_mode:
    preserve_registers // we do not yet have register mapping
    .word 0xec00723f // atomic 
    .word 0xec00c80e // write semaphores to clear
    mov r0, #TRAP_SEM_IN_READ
    .word 0xec007285 // atomic block
    .word 0xec00ce08
    mov r0, r0 // read and clear semaphores reading them in to r0
    tst r0, #TRAP_SEM_IN_READ
    beq mkt_int_dis_system_mode_not_trap // well if it ain't the trap, it must be the interrupt enable
    // trap semaphore fired when in system mode
    // can be:  
    // ReturnFromInterruptToUser
    // ReturnFromInterruptToSystem
    // EnterUserMode
    // Trap number is in R16
    .word 0xec00d10e
    and r1, r0, #0xff // r1 = r16 & 0xff
    cmp r1, #Trap_ReturnFromInterruptToUser
    beq mkt_basic_system_mode_trap_return_from_interrupt_to_user
    cmp r1, #Trap_ReturnFromInterruptToSystem
    beq mkt_basic_system_mode_trap_return_from_interrupt_to_system
    cmp r1, #Trap_EnterUserMode
    bne mkt_unknown_trap
    b mkt_unknown_trap

        
    // System mode interrupts disabled, not a trap - should be interrupts enabled - check, and if so we can go back to basic system mode
mkt_int_dis_system_mode_not_trap:   
    tst r0, #INTEN_SEM_IN_READ
    beq mkt_unknown_trap // this should never happen - we were scheduled, after all
    // To go back to basic mode we need to change the flag dependencies and restart address (for now - this may be overridden later anyway)
    .word 0xec00c86e
    mov r0, #((TRAP_SEM | HWINT_SEM)<<4)|1 // set our flag dependencies and restart mode
    .word 0xec007000 | ( (mkt_basic_system_mode - (.+8)) & 0xffe )
    b mkt_basic_system_mode_not_trap

mkt_basic_int_dis_mode_not_trap_interrupts_disabled:   
    // Cannot imagine how we got here - scheduled based on one of two semaphores, both of which are clear and only we should clear.
    b mkt_unknown_trap
    
    // User mode, not a trap, emulation thread not preempted, trap semaphore may be set, Hm. Bad call.
mkt_basic_user_mode_not_trap:   
    b mkt_unknown_trap


    

    
    // This code is called as part of the mkt to return from an interrupt - it gets the flags (to r16) and pc (to r15)
    // stops mkt from being scheduled by removing dependencies, then restarts the emulated thread at the emt_return_from_interrupt
    // which makes everything return cleanly
mkt_return_from_interrupt:
    // enable interrupts
    .word 0xec00723f // atomic 
    .word 0xec00c80e // write semaphores to set
    mov r0, #INTEN_SEM_IN_READ
    .word 0xec007285 // atomic block
    .word 0xec00ce08
    mov r0, r1 // read and set semaphores reading them in to r0
    // stop mkt from scheduling
    .word 0xec00c86e
    mov r0, #1 // set our flag dependencies to nothing (just ARM mode)
    // set emulation thread to restart at emt_return_from_interrupt
    .word 0xec007205 // Atomic while we play with spec4/5
    .word 0xec00c84e
    mov r0, #EMULATION_THREAD // spec4 <= EMULATION_THREAD
    .word 0xec00c85e
    add r0, pc, #emt_return_from_interrupt - (.+8) + 1 // spec5:    add 1 so that the restart address is for the selected thread, not mkt
    // recover flags and pc, then the remaining registers
    ldr r0, = microkernel_thread_interrupted_data
    .word 0xec00c10e
    ldr r0, [r0, #15*4] // r16 = flags
    .word 0xec00c0fe
    ldr r0, [r0, #16*4] // r15 = pc
    ldmia r0, {r0-r14}
    // deschedule, blocking first for cleanliness
    .word 0xec007385
    nop
   
    // This code is scheduled for the emulation thread to return from interrupt
    // It must recover the flags and pc from storage, then set the correct flag dependencies for the mkt
    // The mkt MUST be dependent on nothing when this code is scheduled
    // If interrupts occur between the mkt descheduling and this code running there is no effect
    // basically this code is an extension of mkt but running in the emulation thread - we effectively single-thread it
emt_return_from_interrupt:
    // atomic max - we flush at the end anyway
    .word 0xec00723f
    // atomically set the flag dependencies for mkt to be hardware int and trap
    .word 0xec00c84e
    mov r0, #MICROKERNEL_THREAD // spec4 = thread
    .word 0xec00c86e
    ldr r0, emt_restart_flags  // spec6 = flags
    // set the flags from storage (r16)     writeflags r16
    .word 0xec00cf01 // extrdrm rd=none, rm=16-31
    .word 0xec00ee30 // extcmd 0110ccccsaoobbbb - c condition, s sign, a acc, o op (3 for gp mode), b burst
    .word 0xec000f00 // alureg op15 rd=none, rm=16
    // set the pc from storage (r15)
    .word 0xec00ce00 // extrdrm rd=none, rm=15
    mov pc, r15
    nop

emt_restart_flags:  .word    0x100 | ((TRAP_SEM | HWINT_SEM)<<4) | 1

    
    
    
    
                        
   // fatal unknown trap - reboot!
mkt_unknown_trap:
    b mkt_unknown_trap
    mov pc, #0
