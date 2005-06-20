.text
	.align 4

    .global test_entry_point
  	.type	 test_entry_point,function
test_entry_point:
    mov pc, #0x80000000
