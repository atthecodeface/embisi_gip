    .text
	.align 4

    mov sp, #0x20000 // microkernel_init needs a stack

    stmfd sp!, {r0-r12, lr}
    ldr r0, =0x10400 // microkernel_init, make it return here so we can recover r0/r1
    mov lr, pc
    mov pc, r0
    ldr r0, =0x80208000 // linux
    str r0, [sp, #4*13] // replace 'lr' on stack with linux entry point
    ldmfd sp!, {r0-r12, pc}
