    ;; The microkernel shares R0, R13, R14, R15, R16, R17 with the ARM mode code thread.
    ;; It need not share R8 through R11; this would help in storing off the banked registers. Make them R20 through R23.
    ;; We can use our R9 as the interrupt status register; this can be actual register R21.

struct t_banked_registers:
    word StoredUserR0 1         ;  preserve the order of the first 5 for easy storage in user mode
    word StoredUserSP
    word StoredUserLink
    word StoredInterruptedFlags
    word StoredInterruptedPC
    word StoredSystemSP
    end                         ;  struct

storage t_banked_registers banked_registers @StoredRegistersStart    
    asmmode auto_extimm auto_extregs

    ;; User mode state, entry point
state_user_mode_entry:
    ;; Expected SystemCall, UndefinedInstruction or HardwareInterrupt
    mov r8, #&banked_registers
    use r8 banked_registers
    assert (offset(StoredUserR0)==0)
    str r0, banked_registers->StoredUserR0, #4 ; StoredUserR0
;;;     use add banked_registers 4 implied
    tstanyclear semaphore_reg, #SoftSemaphore
    b state_user_mode_look_for_hardware_interrupt ; not a soft semaphore, so look for a hardware interrupt

state_user_mode_handle_soft_call: ; Also undefined instruction, same effect
    ;; Clear interrupt enable semaphore and soft semaphore
    mov semaphore_clear_reg,  #InterruptEnableSemaphore | SoftSemaphore

    ;; Save user R13, user R14 in banked register area
    str r13, banked_registers->StoredUserSP, #4
;;;     use add banked_registers 4 implied
    str r14, banked_registers->StoredUserLink, #4
;;;     use add banked_registers 4 implied

    ;; Set 'R14_SVC' to the return address
    mov r14, $r15

    ;; Recover 'R13_SVC'
    ldr r13, banked_registers->StoredSystemSP

    ;; Change ARM mode code thread to run at system call vectoring routine
    mov sched_thread, #ArmModeCodeThread
    mov sched_pc, #ArmModeCodeSystemCallVectorRoutine
    mov semaphore_set_reg, #ArmModeCodeThreadRun

    ;; Get system call number in R0
    mov r0, $r16

    ;; We are now in system state, interrupts disabled, with no interrupt pending, so exit from there
    b state_system_mode_interrupts_disabled_no_interrupt_pending_exit


    ;; Hardware interrupt status register is nonzero, ARM mode code thread is fully scheduled and configured. Handle highest priority interrupt.
    ;; Note that r8 points to the banked register area + 4, and user R0 is already saved
    use r8 banked_registers
    use add banked_registers 4
state_user_mode_handle_hardware_interrupt:
    mov r10, r9
    cmpeq r10, #0               ; If none, then exit - this should not happen, but it is defensive
    b state_user_mode_exit
    xorf r10, #0            ; Find highest priority interrupt

    ;; Clear interrupt enable semaphore (disabled interrupts)
    mov semaphore_clear_reg,  #InterruptEnableSemaphore
    
    ;; Save user R13, user R14 in banked register area
    str r13, banked_registers->StoredUserSP, #4
;;;     use add banked_registers 4 implied
    str r14, banked_registers->StoredUserLink, #4
;;;     use add banked_registers 4 implied

    ;; Get interrupted flags and PC, and save them
    str preempted_flags, banked_registers->StoredInterruptedFlags, #4
;;;     use add banked_registers 4 implied
    str preempted_pc_l, banked_registers->StoredInterruptedPC, #4
;;;     use add banked_registers 4 implied

    ;; Recover system R13
    ldr r13, banked_registers->StoredSystemSP

    ;; Change ARM mode code thread to run at user mode interrrupt handler
    mov sched_thread, #ArmModeCodeThread
    mov sched_pc, #ArmModeCodeUserModeInterruptHandler | 1 ; OR in 1 so that the selected thread is used
    mov semaphore_set_reg, #ArmModeCodeThreadRun

    ;; Get interrupt number in R0
    mov r0, r10

    ;; Reschedule us to run when the ARM mode code issues reenable interrupts or ReturnFromInterruptsToUser or ReturnFromInterruptsToSystem, at the continue testing interrupts section
    mov thread_data, #InterruptEnableSemaphore | SoftSemaphore ; schedule us on interrupt enable or soft semaphore
    restart state_system_mode_interrupts_disabled_entry ;  deschedule and restart us at state_system_mode_interrupts_disabled_entry

    
    ;; User mode state, exit point - also our path for checking hardware interrupts
state_user_mode_look_for_hardware_interrupt:        
state_user_mode_exit:
    ;; About to start ARM mode code thread. Check first for pending interrupts - interrupts must be enabled here anyway (always are in user mode). Must preserve all ARM thread registers, though.
    cmpne r9, #0
    b state_user_mode_handle_hardware_interrupt ;  except the flags and PC may not be there, as we may get here from a non-preemption!
    mov semaphore_set_reg, #InterruptEnableSemaphore ; Should not need to do this, but it is safe enough - interrupts should already be enabled
    mov thread_data, #SoftSemaphore | InterruptPendingSemaphore ; schedule us on interrupt pending or soft semaphore
    restart state_user_mode_entry ;  deschedule and restart us at state_user_mode_entry
    

    
state_system_mode_interrupts_disabled_no_interrupt_pending_exit:        
    ;; Reschedule on hardware interrupt occurring or soft Semaphore - ARM thread should already be set up
    mov thread_data, #SoftSemaphore | InterruptPendingSemaphore ; schedule us on interrupt pending or soft semaphore
    restart state_system_mode_interrupts_disabled_entry ;  deschedule and restart us

state_system_mode_interrupts_disabled_interrupt_pending_exit:        
    ;; Reschedule on interrupt enable occurring or soft Semaphore - ARM thread should already be set up
    mov thread_data, #SoftSemaphore | InterruptEnableSemaphore ; schedule us on interrupt enable or soft semaphore
    restart state_system_mode_interrupts_disabled_entry ;  deschedule and restart us


    
    ;; System mode state with interrupts expected to be disabled, entry point
state_system_mode_interrupts_disabled_entry:
    ;; If interrupts are now enabled then go to state_system_mode_interrupts_enabled_entry
    tstanyset semaphore_reg, #InterruptEnableSemaphore
    b state_system_mode_interrupts_enabled_entry:
    ;; Okay, wer are in system mode, interrupts disabled, and something woke us up
    ;; That could be a soft semaphore, or a hardware interrupt, or just nothing (defensively)
    ;; First, handle the soft semaphore (as it is highest priority)
    ;; If a soft semaphore occurred, it could be ReturnFromInterruptToUser, or ReturnFromInterruptToSystem, SystemCall(?), or EnterUserMode
    ;; ReturnFromInterruptToUser should enable interrupts, recover R13/R14 from user bank, set ARM mode code thread PC and flags to interrupted PC and flags, and restart the ARM thread, exiting to state_user_mode_exit
    ;; ReturnFromInterruptToSystem should enable interrupts, set ARM mode code thread PC and flags to interrupted PC and flags, and restart the ARM thread, exiting to state_system_mode_interrupts_enabled_exit
    ;; EnterUserMode should enable interrupted, set ARM mode code thread PC to R15, and restart the ARM thread, exiting to state_user_mode_exit
    ;; If no soft semaphore occurred and a hardware interrupt did occur, we should go to system state, interrupts disabled, interrupt pending, and not touch the ARM at all (no ARM thread interaction required!). This is a lot like doing nothing.
    ;; If nothing happened, just reenter when something does
    b state_system_mode_interrupts_disabled_exit

state_system_mode_interrupts_disabled_exit: 
    cmpne r9, #0
    b state_system_mode_interrupts_disabled_interrupt_pending_exit
    b state_system_mode_interrupts_disabled_no_interrupt_pending_exit
    
    
    ;; System mode state with interrupts expected to be enabled, entry point
state_system_mode_interrupts_enabled_entry:
    ;; If interrupts are now disabled then go to state_system_mode_interrupts_disabled_entry
    tstanyclear semaphore_reg, #InterruptEnableSemaphore
    b state_system_mode_interrupts_disabled_entry:
    ;; Okay, wer are in system mode, interrupts enabled, and something woke us up
    ;; That could be a soft semaphore, or a hardware interrupt, or just nothing (defensively)
    ;; First, handle the soft semaphore (as it is highest priority)
    ;; If a soft semaphore occurred, it could be ReturnFromInterruptToUser, or ReturnFromInterruptToSystem, SystemCall(?), or EnterUserMode
    ;; ReturnFromInterruptToUser should enable interrupts, recover R13/R14 from user bank, set ARM mode code thread PC and flags to interrupted PC and flags, and restart the ARM thread, exiting to state_user_mode_exit
    ;; ReturnFromInterruptToSystem should enable interrupts, set ARM mode code thread PC and flags to interrupted PC and flags, and restart the ARM thread, exiting to state_system_mode_interrupts_enabled_exit
    ;; EnterUserMode should enable interrupts, set ARM mode code thread PC to R15, and restart the ARM thread, exiting to state_user_mode_exit
    ;; If no soft semaphore occurred and a hardware interrupt did occur, we should handle that interrupt
    ;; If nothing happened, just reenter when something does
    b state_system_mode_interrupts_enabled_exit

state_system_mode_interrupts_enabled_exit:
    cmpne r9, #0
    b state_system_mode_handle_hardware_interrupt ;  except the flags and PC may not be there, as we may get here from a non-preemption!
    mov thread_data, #SoftSemaphore | InterruptPendingSemaphore ; schedule us on interrupt pending or soft semaphore
    restart state_system_mode_interrupts_enabled_entry ;  deschedule and restart us
    
        