    .macro gip_read_and_clear_semaphores, semaphores, reg
    gip_atomic 31
    gip_mov_full_imm 0x80, #\semaphores
    gip_atomic_block 1
    gip_extrm 0x80
    mov \reg, r0 // read and clear semaphores reading them in to \reg
    .endm

    .macro gip_read_and_set_semaphores, semaphores, reg
    gip_atomic 31
    gip_mov_full_imm 0x80, #\semaphores
    gip_atomic_block 1
    gip_extrm 0x80
    mov \reg, r1 // read and set semaphores reading them in to \reg
    .endm

    .macro restart_at_int_dis_system
    .word 0xec00c86e
    mov r0, #((TRAP_SEM | INTEN_SEM)<<4)|1 // set our flag dependencies and restart mode
    .word 0xec007000 | ( (mkt_int_dis_system_mode - (.+8)) & 0xffe )
    .endm

    .macro restart_at_basic_system
    .word 0xec00c86e
    mov r0, #((TRAP_SEM | HWINT_SEM)<<4)|1 // set our flag dependencies and restart mode
    .word 0xec007000 | ( (mkt_basic_system_mode - (.+8)) & 0xffe )
    .endm

    .macro gip_atomic, num_inst
    .word 0xec007201 | (\num_inst<<1)
    .endm
    
    .macro gip_atomic_block, num_inst
    .word 0xec007281 | (\num_inst<<1)
    .endm
    
    .macro gip_extrd, full_reg
    .word 0xec00c00e | (\full_reg<<4)
    .endm
    
    .macro gip_extrm, full_reg
    .word 0xec00ce00 | (\full_reg>>4)
    .endm
    
    .macro gip_extrn, full_reg
    .word 0xec00d00e | (\full_reg<<4)
    .endm
    
    .macro gip_mov_full_reg, full_reg, reg
    gip_extrd \full_reg
    mov r0, \reg
    .endm
    
    .macro gip_mov_full_imm, full_reg, immediate
    gip_extrd \full_reg
    mov r0, \immediate
    .endm

    .macro gip_deschedule_block
    .word 0xec007385
    nop
    .endm

    .macro gip_restart_at, label
//    .ifgt (\label-(.+8))-0x7ff
//    b .+0x80000000
    //    .err "gip restart at out of range"
//    .endif
//    .iflt (\label-(.+8))+0x7ff
//    b .+0x80000000
    //    .err "gip restart at out of range"
//    .endif
    .word 0xec007000 | ( (\label - (.+8)) & 0xffe )
    .endm
                    
    .macro gip_utximm c
    .word 0xec00c40e
    ldr r0, = 0x16000096 + (\c<<8)
    .endm
    
    .macro gip_utx c, tmp            
    mov \tmp, \c, lsl #8
    orr \tmp, \tmp, #0x96
    orr \tmp, \tmp, #0x16000000
    .word 0xec00c40e
    mov \tmp, \tmp
    .endm
