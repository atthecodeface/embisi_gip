.text
	.align 4

    .global wrapper_entry_point
  	.type	 wrapper_entry_point,function
wrapper_entry_point:

#ifdef TEST_MEM_RD    
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

#endif

  	mov sp, #0x20000
	bl	test_entry_point
	cmp r0, #0
	bne test_failed

test_passed:
    mov r8, #0x5000
    orr r8, r8, #0xa6
    .word 0x3c00c40e
    mov r8, r8
    mov pc, #0

test_failed:	
    mov r8, #0x4600
    orr r8, r8, #0xa6
    .word 0x3c00c40e
    mov r8, r8
    mov pc, #0

.Lfe1:
	.size	 wrapper_entry_point,.Lfe1-wrapper_entry_point
