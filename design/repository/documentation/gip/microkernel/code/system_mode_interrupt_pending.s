    ;; The microkernel shares R0, R13, R14, R15, R16, R17 with the ARM mode code thread.
    ;; It need not share R8 through R11; this would help in storing off the banked registers. Make them R20 through R23.
    ;; We can user our R9 as the interrupt status register; this can be actual register R21.

    ;; User mode state, entry point
    
state_system_mode_interrupt_pending_exit:        
        ;; Reschedule on hardware interrupt occurring or

    