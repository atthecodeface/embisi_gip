    ;; First comment
    ;; Second comment
    extimm #6553600             ; Comment this line
loop:   
    mov r1, #3
    add r1, #3
    bic r1, #3
    lsl r1, #3
    lsr r1, r2
    cmpeq r2, r2
    b loop
fred:
    
next_loop:  
        
    