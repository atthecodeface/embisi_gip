    ;; The microkernel shares R0, R13, R14, R15, R16, R17 with the ARM mode code thread.
    ;; It need not share R8 through R11; this would help in storing off the banked registers. Make them R20 through R23.
    ;; We can user our R9 as the interrupt status register; this can be actual register R21.

    ;; User mode state, entry point
state_user_mode_entry:
    ;; Expected SystemCall, UndefinedInstruction or HardwareInterrupt
    str r0, [r8, #StoredUserR0]
    schedsemrd r0, semaphores
    tstanyclear r0, #SoftSemaphore
    b state_user_mode_look_for_hardware_interrupt ; not a soft semaphore, so look for a hardware interrupt

state_user_mode_handle_soft_call:   
    ;; Clear interrupt enable semaphore
    schedsemclear #InterruptEnableSemaphore
    
    ;; Clear soft semaphore
    schedsemclear #SoftSemaphore

    ;; Save user R13, user R14 in banked register area
    str r13, [r8, #StoredUserSP]
    str r14, [r8, #StoredUserLink]

    ;; Set R14_SVC to the return address
    mov r14, r15

    ;; Recover system R13
    ldr r13, [r8, #StoredSystemSP]

    ;; Change ARM mode code thread to run at system call vectoring routine
    schedthread #ArmModeCodeThread
    ldr r0,
    schedpc #ArmModeCodeSystemCallVectorRoutine
    schedsemset #ArmModeCodeThreadRun

    ;; Get system call number in R0
    mov r0, r16 !!!

    ;; We are now in system state with no interrupt pending, so exit from there
    b state_system_mode_no_interrupt_pending_exit


    ;; Hardware interrupt status register is nonzero, ARM mode code thread is fully scheduled and configured. Handle highest priority interrupt.
state_user_mode_handle_hardware_interrupt:
    mov r10, r9
    cmpeq r10, #0               ; If none, then exit - this should not happen, but it is defensive
    b #1, state_user_mode_exit
    xorfirst r10, #0            ; Find highest priority interrupt

    ;; Clear interrupt enable semaphore
    schedsemclear #InterruptEnableSemaphore
    
    ;; Save user R13, user R14 in banked register area
    str r13, [r8, #StoredUserSP]
    str r14, [r8, #StoredUserLink]

    ;; Get interrupted flags and PC, and save them
    schedpriority #0
    schedrdflags r13
    str r13, [r8, #StoredInterruptedFlags]
    schedrdpc r13
    str r13, [r8, #StoredInterruptedPC]

    ;; Recover system R13
    ldr r13, [r8, #StoredSystemSP]

    ;; Change ARM mode code thread to run at user mode interrrupt handler
    schedthread #ArmModeCodeThread
    ldr r0,
    schedpc #ArmModeCodeUserModeInterruptHandler
    schedsemset #ArmModeCodeThreadRun

    ;; Get interrupt number in R0
    mov r0, r10

    
    ;; User mode state, exit point - also our path for checking hardware interrupts
state_user_mode_look_for_hardware_interrupt:        
state_user_mode_exit:
    ;; About to start ARM mode code thread. Check first for pending interrupts - interrupts must be enabled here anyway (always are in user mode). Must preserve all ARM thread registers, though.
    cmpeq r9, #0
    bne state_user_mode_handle_hardware_interrupt
    schedsemset #InterruptEnableSemaphore ; Should not need to do this, but it is safe enough - interrupts should already be enabled
    schedsignal #SoftSemaphore | InterruptPendingSemaphore
    schedrestart state_user_mode_entry
    desched

    
state_system_mode_no_interrupt_pending_exit:        
        ;; Reschedule on hardware interrupt occurring or

    