.text
	.align 4

    .global wrapper_entry_point
  	.type	 wrapper_entry_point,function
wrapper_entry_point:

  	mov sp, #0x20000
	bl	test_entry_point
	cmp r0, #0
	bne test_failed
    b test_passed

done:
    b done
    
test_passed:
    mov r4, #0x100
    .word 0xec00c48e
    orr r4, r4, #0x11
    mov r0, r0
    mov r0, r0
    mov r4, #0xc0000000
    .word 0xec00c4ae
    add r4, r4, #0
    mov r0, r0
    mov r0, r0
    .word 0xec00c49e
    mov r0, r0
    b done

test_failed:	
    mov r4, #0x100
    .word 0xec00c48e
    orr r4, r4, #0x11
    mov r0, r0
    mov r0, r0
    mov r4, #0xc0000000
    .word 0xec00c4ae
    add r4, r4, #1
    mov r0, r0
    mov r0, r0
    .word 0xec00c49e
    mov r0, r0
    b done

.Lfe1:
	.size	 wrapper_entry_point,.Lfe1-wrapper_entry_point
