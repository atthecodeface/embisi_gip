    ;; Sample GIP code
    ;; An ARM SWI instruction stores the PC+8 in R15, and the instruction in R16.
    ;; We need to reschedule the ARM code to start at an appropriate point, first preserving r14 and r13,
    ;; and moving the supervisor stack pointer to r13.
    ;; The appropriate start point comes from a vector table, indexed by
    ;; the SWI number.
    ;; Do we need to preserve the flags?
    ;; Are SWIs reentrant? Not here
    ;; Can an interrupt call a SWI - that would require reentrancy
despatch:
    ;; atomically read the sources and clear the highest priority (SWI first, then interrupt)

    ;; if a source was a SWI, go there
    tst source_is_swi
    b #1, handle_swi

    ;; else it is a hardware interrupt
    b #0, handle_irq          ;  Note that this instruction is in the shadow of the conditional branch, but as it is a branch it will not be executed if the condition is true
