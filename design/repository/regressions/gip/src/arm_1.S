    extimm #0
    mov r0, #0
    extimm #256
    mov r1, #256

    mov r2, #0
    mov r3, #0

loop:
        ;;      ldr rm, [rn] (and to use an offset of a register extend with rd; extend immediate for immediate offset)
    ldr r4, [r0]
    add r0, #4
    str r4, [r1]
    add r1, #4
    extimm #0xc0                ; Note that our extimm mnemonic works not for loads, but for immediates; therefore it assumes you are setting bits 4 upwards. Hm
    ldr r4, [r0]
    b loop
    