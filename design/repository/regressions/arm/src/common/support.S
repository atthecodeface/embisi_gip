    .global delay_char
    .type delay_char, function
delay_char: 
    mov r0, #0
tight_loop: 
    add r0, r0, #1
    cmp r0, #8192*6
//     bne tight_loop
    mov pc, lr
        

    .global dprintf
	.type	 dprintf,function
    .word 0x3c000
dprintf:
    stmfd sp!, {r4}
    mov r4, #0x3c000
    add r4, r4, #16
    stmia r4, {r1, r2,r3}
    mov r4, #0x3c000
    str r0, [r4]
        ;;     str r0, [r4, #-16]
    ldmfd sp!, {r4}
    mov pc, lr

    mov r0, r0
    mov r1, r1
    mov r2, r2
    mov r3, r3
    .word 0xf00000a0
    mov pc, lr
    
