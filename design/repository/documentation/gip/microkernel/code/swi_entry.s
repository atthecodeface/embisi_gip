    ;; Sample GIP code
    ;; This code is invoked when an ARM mode code thread runs a SWI.
    ;; It is effectively a microkernel call of 'enter SVC mode and branch-with-link through vector table'
    ;; Later the ARM mode code may give a microkernel call of 'exit SVC mode, restart user mode at address x'
    ;; Of course, the microkernel does not care if the kernel thinks it is in SVC mode or not - it has no concept of the mode.
    ;; Or if the ARM does a reschedule
    
    ;; An ARM SWI instruction stores the PC+8 in R15, and the instruction in R16.
    ;; We need to reschedule the ARM code to start at an appropriate point, first preserving r14 and r13,
    ;; and moving the supervisor stack pointer to r13.
    ;; The appropriate start point comes from a vector table, indexed by
    ;; the SWI number.
    ;; Do we need to preserve the flags?
    ;; Are SWIs reentrant? Not here
    ;; Can an interrupt call a SWI - that would require reentrancy
swi_entry:
    ;; Grab the SWI number
    extimm #16                  ;  how does this effect register-to-register operations?
    mov ?, ?
    ;; Preserve r13 and r14 somewhere
    ;; Get ARM start address from vector table
    ;; Schedule ARM thread as ARM mode, start address as given in the vector table
    ;; Look further at the actual despatch code for Linux - we want to support (but not implement necessarily - this code should not be GPLed) this
    