    ;; First comment
    ;; Second comment
    extimm #6553600             ; Comment this line
    mov r0, #0
    
    mov r1, #3
    mov r2, #5
loop:   
    add r1, #1
    cmpne r1, r2
    b loop

    mov r0, r0
    mov r0, r0
    mov r0, r0
fred:
    b fred
    
next_loop:  
        
    