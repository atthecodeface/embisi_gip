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
    stmfd sp!, {r1-r4, r7, r8}
    ldmfd sp!, {r1-r4, r7, r8}
    mov pc, lr
    
