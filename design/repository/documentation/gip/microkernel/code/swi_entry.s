    ;; Sample GIP code
    ;; An ARM SWI instruction stores the PC+8 in R15, and the instruction in R16.
    ;; We need to reschedule the ARM code to start at an appropriate point, first preserving r14 and r13,
    ;; and moving the supervisor stack pointer to r13.
    ;; The appropriate start point comes from a vector table, indexed by
    ;; the SWI number.
    ;; Do we need to preserve the flags?
    ;; Are SWIs reentrant? Not here
    ;; Can an interrupt call a SWI - that would require reentrancy
swi_entry:
    