.text
	.align 4

    .global wrapper_entry_point
  	.type	 wrapper_entry_point,function
wrapper_entry_point:
    mov r8, #0x2100 // gjs changed this to 2100 at 1115pm Weds, back again at 1155 as that seems to have broken it?
    orr r8, r8, #0x96
    orr r8, r8, #0x16000000
    .word 0xec00c40e
    mov r8, r8
    
    mov r8, #0x0a00
    orr r8, r8, #0x96
    orr r8, r8, #0x16000000
    .word 0xec00c40e // extrdrm periph.0, none
    mov r8, r8

 	mov sp, #0x20000
    mov r9, sp
    sub sp, sp, #8
    str r8, [sp]
    mov r0, r0
    mov r0, r0
    mov r0, r0
    ldr r9, [sp]

    mov r9, r9, ror #24
    and r8, r9, #0xff
    mov r8, r8, lsl #8
    orr r8, r8, #0x96
    orr r8, r8, #0x16000000
    .word 0xec00c40e
    mov r8, r8
    
    mov r9, r9, ror #24
    and r8, r9, #0xff
    mov r8, r8, lsl #8
    orr r8, r8, #0x96
    orr r8, r8, #0x16000000
    .word 0xec00c40e
    mov r8, r8
    
    mov r9, r9, ror #24
    and r8, r9, #0xff
    mov r8, r8, lsl #8
    orr r8, r8, #0x96
    orr r8, r8, #0x16000000
    .word 0xec00c40e
    mov r8, r8
    
    mov r9, r9, ror #24
    and r8, r9, #0xff
    mov r8, r8, lsl #8
    orr r8, r8, #0x96
    orr r8, r8, #0x16000000
    .word 0xec00c40e
    mov r8, r8

    bl delay_char
    bl delay_char
    bl delay_char
    bl delay_char
    bl delay_char
    bl delay_char
    
	mov sp, #0x20000
	bl	test_entry_point
	cmp r0, #0
	bne test_failed

test_passed:
    mov r0, r0
    mov r0, r0
    mov r0, r0
    mov r0, r0
	.word 0xf0000090
	b loop

    .word 0
    .word 0
    .word 0

test_failed:	
	.word 0xf0000091
	b loop

    .word 0
    .word 0
    .word 0

loop:
	b loop
    .word 0
    .word 0
    .word 0
    .word 0

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
    
.Lfe1:
	.size	 wrapper_entry_point,.Lfe1-wrapper_entry_point
