    ;; Simple ALU test

memcpy_16_bytes:  
    ldr r4, [r1], #4
    str r4, [r0], #4

    ldr r4, [r1], #4
    ldr r5, [acc], #4
    ldr r6, [acc], #4
    mov r1, acc
    str r4, [r0], #4
    str r5, [acc], #4
    str r6, [acc], #4
    mov r0, acc                 ;  This could possibly be done with acc/register detection
    mov pc, r14


    