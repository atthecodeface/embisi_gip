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
// r0 -> string
// r1, r2, r3 = args
dprintf_loop:       
    ldrb r4, [r0], #1
    cmp r4, #0
    beq dprintf_done

    cmp r4, #'%'
    bne dprintf_do_char

dprintf_do_char:    
    and r8, r4, #0xff
    mov r8, r8, lsl #8
    orr r8, r8, #0x96
    orr r8, r8, #0x16000000
    .word 0xec00c40e
    mov r8, r8
    b dprintf_loop

dprinf_done
    ldmfd sp!, {r4}
    mov pc, lr
    
