    ;; First comment
    ;; Second comment
;;;    dch #0xf0a2

    extrd #0x71                 ;  So we do postbus FIFO configuration; postbus is 96 to 127; tx fifo config is reg 17
    extimm #0                   ;  Base 0, end 0, read 0, write 0 - all okay
    mov r1, #0                  ;  Actually writing postbus FIFO configuration

    extrd #0x70                 ;  Postbus rx fifo configuration, reg 16
    extimm #0                   ;  Base 0, end 0, read 0, write 0 - all okay
    mov r1, #0                  ;  Actually writing postbus FIFO configuration

send_burst:     
    extrd #0x62                 ;  Postbus Tx FIFO
    mov r1, #1
    extrd #0x62
    mov r1, #2
    extrd #0x62
    mov r1, #3
    extrd #0x62
    mov r1, #4
    extrd #0x62
    mov r1, #5

    extrd #0x61
    extimm #0x10400401             ; set semaphore 1 when tx complete, semaphore 2 when rx complete; length is 4+1; first is not last
    mov r1, #0x8

    mov r1, #100
loop:
    extrn #0x80                 ; special registers, semaphore is 0
    tstanyset r0, #2
    b       tx_complete
    sub r1, #1
    cmpne r1, #0
    b loop

    mov r1, r1
    b fail

tx_complete:    
    mov r1, #100
loop2:
    extrn #0x80                 ; special registers, semaphore is 0
    tstanyset r0, #4
    b       rx_complete
    sub r1, #1
    cmpne r1, #0
    b loop2
    mov r1, r1
    b fail

rx_complete:
    extrd #0x80
    mov r2, #0x6                ; Clear semaphored 1 and 2
    b send_burst
    mov r1, r1
    mov r1, r1
    
pass:   

    dch #0xf090
    dch #0xf090
    dch #0xf090

inf_loop:   
    b inf_loop
    mov r1, r1
    mov r1, r1
    mov r1, r1
    
fail:   

    dch #0xf091
    dch #0xf091
    dch #0xf091
    b inf_loop
        
next_loop:  
        
    