.text
	.align 4

    .global wrapper_entry_point
  	.type	 wrapper_entry_point,function
wrapper_entry_point:

//    mov r8, #0x2100
//    orr r8, r8, #0x96
//    orr r8, r8, #0x16000000
//    .word 0xec00c40e
//    mov r8, r8
    
//    mov r8, #0x0a00
//    orr r8, r8, #0x96
//    orr r8, r8, #0x16000000
//    .word 0xec00c40e // extrdrm periph.0, none
//    mov r8, r8


#ifdef TEST_EBC    
//    mov r8, #0x11 // 2 cycles of address, strobe
//    orr r8, r8, #0xc00 // 1 cycle of hold, increment by 4
//    .word 0xec00c48e // extrdrm periph.8, none (8 is ext bus config, 9 is ext bus data, 10 is ext bus address)
//    mov r8, r8
    
//    mov r8, #0x34 // address 0x1234
//    orr r8, r8, #0x1200 
//    .word 0xec00c4ae // extrdrm periph.a, none (8 is ext bus config, 9 is ext bus data, 10 is ext bus address)
//    mov r8, r8

//    mov r8, #0xef // data 0xbeef
//    orr r8, r8, #0xbe00 
//    .word 0xec00c49e // extrdrm periph.9, none (8 is ext bus config, 9 is ext bus data, 10 is ext bus address)
//    mov r8, r8
//    orr r8, r8, r8, lsl #16 // need at least 2 cycles gap between writes
//    .word 0xec00c49e // extrdrm periph.9, none (8 is ext bus config, 9 is ext bus data, 10 is ext bus address)
//    mov r8, r8

//    mov r8, r8
//    mov r8, r8
//    mov r8, r8
//    mov r8, r8
//    mov r8, r8
//    mov r8, r8
//    .word 0xec00ce04 // extrdrm none, periph.9 (8 is ext bus config, 9 is ext bus data, 10 is ext bus address)
//    mov r8, r9
#endif
    
#ifdef TEST_MEM_RD    
    mov r8, #0x2100
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
#endif
        
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

.Lfe1:
	.size	 wrapper_entry_point,.Lfe1-wrapper_entry_point
