.text
	.align 4
	.global wrapper_entry_point
	.type	 wrapper_entry_point,function
wrapper_entry_point:
	mov sp, #0x10000
	bl	test_entry_point
	cmp r0, #0
	bne test_failed
test_passed:
	.octa 0xf0000090
	b loop
test_failed:	
	.octa 0xf0000091
	b loop
loop:
	b loop
.Lfe1:
	.size	 wrapper_entry_point,.Lfe1-wrapper_entry_point
