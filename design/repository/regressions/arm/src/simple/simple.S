.text
	.align 4
	.global test_entry_point
test_entry_point:
    mov r0, #0
    mov r1, #0
    mov r2, #1
    mov r3, #2

    adds r4, r1, r0

    adds r4, r2, r3

    subs r5, r4, r4

    adds r4, r4, #0x80000000

    movs r5, r4, lsl #1
    
    mov pc, lr
	
