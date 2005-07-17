/*a Includes
 */
#include "gip_macros.h"
    
/*a Macros
 */
    .macro cmp_reg reg, reg_num
    ldr r0, =stable_regs_table
    ldr r0, [r0, #\reg_num*4-4]
    cmp r0, \reg
    movne r1, \reg
    movne r2, r0
    movne r0, #\reg_num
    bne failure
    .endm
    
/*a Main routine
 */
     .text
    .align 4
    .global test_regs_stable
stable_regs_table:
    // r1 top bit set indicates do test in system mode
    // r1 bottom bits indicate which test
    // 00->regs
    // 01->regs with interrupt enable/disable
    // 02->stack pointer with interrupt enable/disable
    // 03->flags with interrupt enable/disable
test_regs_stable:

    tst r1, #1<<31
    bne user_entry_point

// Do the next two to make the system stop after 2 user returns
//    gip_extrd 0x14
//    mov r4, #2
    // First enter user mode - r13 must point to r0-15,flags
    mov sp,     #0x10000
    add sp, sp, #0x0fe00
    sub sp, sp, #17*4
    mov r0, #0xc0000000
    str r0, [sp, #16*4]
    adr r0, user_entry_point
    str r0, [sp, #15*4]
    mov r0,     #0x10000
    add r0, r0, #0x0ff00
    str r0, [sp, #13*4]
    str r1, [sp, #4]
    
    gip_atomic_block 31
    gip_read_and_set_semaphores 0x10, r0
    gip_mov_full_imm 0x80, #0x10
    gip_atomic_block 31
    gip_extrm 0x80
    mov r0, r1 // read and clear semaphores reading them in to \reg
    gip_extrd 0x10
    mov r0, #0x400
    gip_extrd 0x0f
    mov r0, pc
    .word 0xec007305

user_entry_point:
    ands r1, r1, #255
    beq test_regs_held_no_interrupt_mgmt
    cmp r1, #2
    blt test_regs_held_with_interrupt_mgmt
    beq test_sp_with_interrupt_mgmt
    b test_flags_with_interrupt_mgmt

/*a Register test functions
 */
/*f test_regs_held_no_interrupt_mgmt
 */    
test_regs_held_no_interrupt_mgmt:     
    ldr r0, =stable_regs_table
    ldmia r0, {r1-r12, r14}

0:  
    cmp_reg r5, 5
    cmp_reg r6, 6
    cmp_reg r7, 7
    cmp_reg r8, 8
    cmp_reg r9, 9
    cmp_reg r10, 10
    cmp_reg r11, 11
    cmp_reg r12, 12
    cmp_reg r14, 13
    cmp_reg r1, 1
    cmp_reg r2, 2
    cmp_reg r3, 3
    cmp_reg r4, 4

    b 0b
    
/*f test_regs_held_with_interrupt_mgmt
 */    
test_regs_held_with_interrupt_mgmt:     
    ldr r0, =stable_regs_table
    ldmia r0, {r1-r12, r14}

0:  
    cmp_reg r5, 5
    cmp_reg r6, 6
    cmp_reg r7, 7
    cmp_reg r8, 8
    cmp_reg r9, 9
    cmp_reg r10, 10
    cmp_reg r11, 11
    cmp_reg r12, 12
    cmp_reg r14, 13
    cmp_reg r1, 1
    cmp_reg r2, 2
    cmp_reg r3, 3
    cmp_reg r4, 4

    .word 0xec00723f
    .word 0xec00c80e
    mov r0, #0x20
    .word 0xec0072bf
    .word 0xec00ce08
    mov r0, r1 // enable interrupts
    .word 0xec007283

    cmp_reg r5, 5
    cmp_reg r6, 6
    cmp_reg r7, 7
    cmp_reg r8, 8
    cmp_reg r9, 9
    cmp_reg r10, 10
    cmp_reg r11, 11
    cmp_reg r12, 12
    cmp_reg r14, 13
    cmp_reg r1, 1
    cmp_reg r2, 2
    cmp_reg r3, 3
    cmp_reg r4, 4

    .word 0xec00723f
    .word 0xec00c80e
    mov r0, #0x20
    .word 0xec0072bf
    .word 0xec00ce08
    mov r0, r0 // disable interrupts
    .word 0xec007283

        b 0b

/*a SP test function
 */
test_sp_with_interrupt_mgmt:    
    mov r2, sp
0:  
    .word 0xec00723f
    .word 0xec00c80e
    mov r0, #0x20
    .word 0xec0072bf
    .word 0xec00ce08
    mov r0, r1 // enable interrupts
    .word 0xec007283

    add sp, sp, #16
    sub r1, sp, r2
    cmp r1, #16
    bne failure

    add sp, sp, #16
    sub r1, sp, r2
    cmp r1, #32
    bne failure

    sub sp, sp, #48
    sub r1, sp, r2
    cmp r1, #-16
    bne failure

    add sp, sp, #16
    cmp sp, r2
    bne failure

    .word 0xec00723f
    .word 0xec00c80e
    mov r0, #0x20
    .word 0xec0072bf
    .word 0xec00ce08
    mov r0, r0 // disable interrupts
    .word 0xec007283

    add sp, sp, #16
    sub r1, sp, r2
    cmp r1, #16
    bne failure

    add sp, sp, #16
    sub r1, sp, r2
    cmp r1, #32
    bne failure

    sub sp, sp, #48
    sub r1, sp, r2
    cmp r1, #-16
    bne failure

    add sp, sp, #16
    cmp sp, r2
    bne failure

    b 0b

/*a Flags test function
 */
test_flags_with_interrupt_mgmt:    
    mov r2, #0
    mov r1, #0
0:
    cmp r1, #0
    bne 1f

    .word 0xec00723f
    .word 0xec00c80e
    mov r0, #0x20
    .word 0xec0072bf
    .word 0xec00ce08
    mov r0, r1 // enable interrupts
    .word 0xec007283
    b 2f

1:  
    .word 0xec00723f
    .word 0xec00c80e
    mov r0, #0x20
    .word 0xec0072bf
    .word 0xec00ce08
    mov r0, r0 // disable interrupts
    .word 0xec007283
2:
    
    gip_atomic_block 31
    gip_mov_full_imm 0x88, #3
    gip_atomic_block 31
    gip_atomic_block 31
    .word 0xec00cc0e // extrdrm rd=none
    .word 0xec000f02 // alureg op15 rd=none, rm=2 - ttttooooddddmmmm type=0, op=0xf, d=0, m=2
    gip_mov_full_imm 0x88, #0
    gip_atomic_block 1
    gip_atomic_block 1

    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    
    mov r3, #0
    orreq r3, r3, #1
    orrmi r3, r3, #2
    orrcs r3, r3, #4
    orrvs r3, r3, #8
    cmp r3, r2
    bne failure

    add r1, r1, #1
    ands r1, r1, #1
    bne 0b
            
    add r2, r2, #1
    and r2, r2, #15
    b 0b

/*a Failure routine
 */
failure:
    mov r0, #0x11000
    add r0, r0, #0x200
    stmia r0, {r0-r14}
    gip_extrd 15
    str r0, [r0, #15*4]
    gip_extrd 16
    str r0, [r0, #16*4]
    gip_extrd 17
    str r0, [r0, #17*4]
    gip_extrd 18
    str r0, [r0, #18*4]
    gip_extrd 19
    str r0, [r0, #19*4]
    gip_extrd 20
    str r0, [r0, #20*4]
    gip_extrd 21
    str r0, [r0, #11*4]
    gip_extrd 22
    str r0, [r0, #22*4]
    gip_extrd 23
    str r0, [r0, #23*4]
    gip_extrd 24
    str r0, [r0, #24*4]
    gip_extrd 25
    str r0, [r0, #25*4]
    gip_extrd 26
    str r0, [r0, #26*4]
    gip_extrd 27
    str r0, [r0, #27*4]
    gip_extrd 28
    str r0, [r0, #28*4]
    gip_extrd 29
    str r0, [r0, #29*4]
    gip_extrd 30
    str r0, [r0, #30*4]
    gip_extrd 31
    str r0, [r0, #31*4]
    gip_utximm 'M'
    gip_utximm 'K'
    gip_utximm 'T'
    gip_utximm ' '
    gip_utximm 'U'
    gip_utximm 'N'
    gip_utximm 'K'
    gip_utximm '\n'

    .word 0xec00c58e // disable the timer
     mov r0, #1<<31 

     mov pc, #0
