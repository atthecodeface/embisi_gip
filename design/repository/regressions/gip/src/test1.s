    ;; First comment
    ;; Second comment
    extimm #6553600             ; Comment this line
    mov r0, #0
    
    mov r1, #3
    mov r2, #15
    mov r3, #10
loop:
    extrd #130
    mov r0, r3                  ; actually semaphore clear
    lsl r3, #1
    add r1, #1
    extrd #129
    mov r0, r3                  ; actually semaphore set
    cmpne r1, r2
    b loop

    mov r0, r0
    mov r0, r0
    mov r0, r0

pass:   
    dch #0xf090
    dch #0xf090
    dch #0xf090
fred:
    b fred
    
next_loop:  
        
    