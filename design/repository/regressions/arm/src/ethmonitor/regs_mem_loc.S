// List of failing locations
//#define CODE_TEST_LOCATION 0x8020dd3c
//#define CODE_TEST_LOCATION 0x8000dd3c    
//#define CODE_TEST_LOCATION 0x8000dd40
#define CODE_TEST_LOCATION 0x8020dd40
//#define CODE_TEST_LOCATION 0x80200000
    
    .text
    .align 4
    .global test_regs_stable
test_regs_stable:
    .word 0xec00c52e
    mov r0, #0

        ldr r1, =0x11000
    ldr r5, =0x12345678
    str r5, [r1, #772]
    ldr r2, = code_to_copy
    ldr r9, = CODE_TEST_LOCATION
    ldmia r2, {r4,r5,r6,r7,r8,r10}
    stmia r9, {r4,r5,r6,r7,r8,r10}
    mov r8, #0x100
0:
    add r8, r8, #1
    str r8, [r1, #128]

    bl outer_subroutine
    ldr r4, [r3]
    cmp r2, r4
    blne reg_differs

    b 0b



code_to_copy:   // fails at 0820dd3c
  .word    0xe5942304       // ldr     r2, [r4, #772]
  .word    0xe59f3008       // ldr     r3, [pc, #8]    ; 8020dd50 <free_area_init+0x48>
  .word    0xe5832000       // str     r2, [r3]
  .word    0xe91ba810       // ldmdb   fp, {r4, fp, sp, pc}
  .word    0x803699e4
  .word    0x8038a0cc
    
outer_subroutine:
    mov r12, sp
    stmfd sp!, {r4,r11,r12,lr,pc}
    sub r11, r12, #4
    ldr r3, =0x8038a0cc
    str r3, [r3]
    mov r4, r1
    mov pc, r9


reg_differs:
  
    mov r0, #0x11000
    stmia r0, {r0-r15}

    .word 0xec00c52e
    mov r0, #0xff

    add r0, r0, #65
    mov r8, r0, lsl #8
    orr r8, r8, #0x96
    orr r8, r8, #0x16000000
    .word 0xec00c40e 
    mov r8, r8

    b test_regs_stable
    mov pc, #0
    