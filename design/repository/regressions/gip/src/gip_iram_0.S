    mov r0, #0
    mov r1, #0
    mov r2, #0
    mov r3, #0

loop:
    add r0, #1
    add r1, r0
    add r2, r1
    b loop
    