/*
 *  linux/arch/arm/kernel/entry-armv.S
 *
 *  Copyright (C) 1996,1997,1998 Russell King.
 *  Copyright (C) 2004 Embisi Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *  Low-level vector interface routines
 *
 */

#define ENTRY(name) \
  .globl name; \
  name:
#include <gip_macros.h>
#include <postbus_asm.h>

#define t_gipeth__rx_next_buffer_to_reuse (0*4)
#define t_gipeth__rx_buffer_in_hand       (1*4)
#define t_gipeth__rx_length_so_far        (2*4)
#define t_gipeth__tx_next_buffer_to_tx    (3*4)
#define t_gipeth__tx_buffer_in_hand       (4*4)
#define t_gipeth__tx_length               (5*4)
#define t_gipeth__regs                    (6*4)

#define t_gipeth_buffer__next_in_list     (0*4)
#define t_gipeth_buffer__data             (1*4)
#define t_gipeth_buffer__length           (2*4)
#define t_gipeth_buffer__ready            (3*4)
#define t_gipeth_buffer__done             (4*4)

#define GIPETH_BUFFER_LENGTH (2048)
    
/*a GIP ethernet hardware thread code
 */
/*f gipeth_hw_thread
    Runs as thread 6
    Uses r28-30 and timer 1
    r28 -> t_gipeth structure for the driver
    Semaphore 23 is the postbus txd0/comm0 access semaphore
    Semaphore 24 is virtual rx status fifo ne
    Semaphore 25 is virtual tx status fifo ne
    Semaphore 26 is 'work to be done' from the device driver
    Semaphore 27 is virtual tx data fifo nnf
*/
ENTRY(gipeth_hw_thread)

    /*b Preserve registers
    */
 gip_utximm 'P'    
    gip_extrn 28
    str r0, [r12, #t_gipeth__regs]
    gip_extrn 28
    add r0, r12, #t_gipeth__regs
    stmib r0, {r1-r14}

    /*b Read and clear semaphores 23-26 - don't clear 27 unless we add to the tx data fifo, as otherwise it is invalid
    */
    mov r12, #0
    gip_read_and_clear_semaphores 0xf, 23, 13
    tst r13, #1<<24
    blne gipeth_hw_thread_handle_rx
    tst r13, #1<<25
    blne gipeth_hw_thread_handle_tx_status
    tst r13, #1<<26
    blne gipeth_hw_thread_handle_soft_irq
    tst r13, #1<<27
    blne gipeth_hw_thread_handle_tx_data // now if there is room in the tx fifo and we have data for tx, then transmit it    

    /*b If we care have data to transmit then wait for space in the FIFO, else don't
    */
    tst r12, #2
    mov r0, #((0x7)<<4)|1   // set our flag dependencies and restart mode - don't depend on watermark semaphore if no data to send
    movne r0, #((0xf)<<4)|1 // set our flag dependencies and restart mode - depend on watermark semaphore if data to send
    gip_extrd 0x86
    mov r0, r0

    /*b Set microkernel interrupt if required
    */
    tst r12, #1
    beq 0f

    gip_extrdrn 0x11, 0x11
    orr r1, r1, #2 // take some time setting our interrupt bit (r17 bit 2) */
    gip_set_semaphores 4, 4

0:

    /*b Recover registers
    */
    gip_extrn 28
    add r0, r12, #t_gipeth__regs
    ldmia r0, {r0-r14}

    /*b Exit
    */
    nop
    nop
 gip_utximm 'D'
    nop
    nop
 gip_utximm '\n'
    nop
    nop
    .word 0xec007305  // deschedule */
    nop

/*f gipeth_hw_thread_handle_tx_status
    on entry - r12 = need to set soft irq
    preserve r13, can set (but not clear) r12

    There is status for us - get it, and handle it
    Most status just tells us there is more room in the tx fifo
 */
gipeth_hw_thread_handle_tx_status:

    gip_utximm 'T'
    gip_atomic_block 1
    gip_utximm 'X'    
    gip_atomic_block 1
    gip_utximm 'S'    
    gip_atomic_block 1
    
    /*b Define registers - r12, r13, r14 must not be used
    */
txs_temp       .req r0
txs_temp2      .req r1        
txs_time       .req r2
txs_status     .req r3

    /*b Read the status (time/status) pair - issue the command to the postbus, wait for it to come back, then read the time and status
    on entry - none
    on exit - time, status, next_buffer_to_reuse, buffer_in_hand, length
    */
 gip_utximm 'S'    
    gip_extrd 0x60    // extrd postbus tx d fifo 0
    ldr r0, read_tx_status_data
    gip_extrd 0x6c    // extrd postbus tx c 0
    ldr r0, read_tx_status_cmd

 gip_utximm 'W'    
0:
    gip_test_semaphores 1, 23
    beq 0b
    
 gip_utximm 'G'    
    gip_extrm 0x60 // postbus rx d 0
    mov txs_time, r0 // time
    gip_extrm 0x60 // postbus rx d 0
    mov txs_status, r0 // status

    gip_clear_semaphores 1, 23

    mov pc, lr

    /*b Numbers we need
    */
read_tx_status_data: 
    .word    (0<<postbus_command_source_io_cmd_op_start) | (2<<postbus_command_source_io_length_start) | (23<<postbus_command_target_gip_rx_semaphore_start)
read_tx_status_cmd: 
    .word    ((0)<<postbus_command_route_start) | ((1-1)<<postbus_command_source_gip_tx_length_start) | ((0)<<postbus_command_source_gip_tx_signal_start) | ((13)<<postbus_command_target_io_dest_start) | (0<<postbus_command_target_io_dest_type_start)

/*f gipeth_hw_thread_handle_soft_irq
    on entry - r12 = need to set soft irq
    preserve r13, can set (but not clear) r12

    check if the current tx data buffer is to be sent and if we are sending nothing
    if so, we can take the data from that buffer and it will be sent
    it gets freed when its last data block is transmitted
 */
gipeth_hw_thread_handle_soft_irq:

    gip_utximm 'S'
    gip_atomic_block 1
    gip_utximm 'O'    
    gip_atomic_block 1
    gip_utximm 'F'    
    gip_atomic_block 1
    

    /*b Define registers - r12, r13, r14 must not be used
    */
si_temp       .req r0
si_temp2      .req r1        
si_next_buffer_to_tx    .req r9 // note these 3 must be in order
si_buffer_in_hand       .req r10 // note these 3 must be in order
si_length               .req r11 // note these 3 must be in order

    /*b Get tx data from storage
    */
    gip_extrn 28
    add si_temp, r12, #t_gipeth__tx_next_buffer_to_tx
    ldmia si_temp, {si_next_buffer_to_tx, si_buffer_in_hand, si_length}

    /*b If there is data already in hand, or if there is no new data anyway, then return
    */
    cmp si_buffer_in_hand, #0
    movne pc, lr
    ldr si_temp2, [si_next_buffer_to_tx, #t_gipeth_buffer__ready]
    cmp si_temp2, #0
    moveq pc, lr

    /*b Put tx data back in storage and return
    */
    ldr si_buffer_in_hand, [si_next_buffer_to_tx, #t_gipeth_buffer__data]
    ldr si_length, [si_next_buffer_to_tx, #t_gipeth_buffer__length]
    orr r12, r12, #2
    stmia si_temp, {si_next_buffer_to_tx, si_buffer_in_hand, si_length}
    mov pc, lr

/*f gipeth_hw_thread_handle_tx_data
    on entry - r12 = need to set soft irq
    preserve r13, can set (but not clear) r12

    There is room in the FIFO for at least 512 bytes
    Most status just tells us there is more room in the tx fifo
 */
gipeth_hw_thread_handle_tx_data:    

    gip_utximm 'T'
    gip_atomic_block 1
    gip_utximm 'X'    
    gip_atomic_block 1
    gip_utximm 'D'    
    gip_atomic_block 1
    
    /*b Define registers - r12, r13, r14 must not be used
    */
txd_temp       .req r3
txd_temp2      .req r4
txd_burst      .req r5
txd_next_buffer_to_tx    .req r9  // note these 3 must be in order
txd_buffer_in_hand       .req r10 // note these 3 must be in order
txd_length               .req r11 // note these 3 must be in order

    /*b Get tx data from storage
    */
    gip_extrn 28
    add txd_temp, r12, #t_gipeth__tx_next_buffer_to_tx
    ldmia txd_temp, {txd_next_buffer_to_tx, txd_buffer_in_hand, txd_length}

    /*b If there is no data to send, then just bail
    */
    cmp txd_buffer_in_hand, #0
    moveq pc, lr
 
    /*b Data to send and room for 512 bytes
        Send up to 512 bytes from the current packet - do no more than the current packet
    */
    gip_utximm 'D'    
    bic txd_temp, txd_length, #0xff000000 // mask out flags
    cmp txd_temp, #512
    movge txd_temp, #512
    movs txd_burst, txd_temp
    mov txd_temp2, #8
    beq not_first_burst_of_packet // only if we have 0 left to go, which is a bug in the caller

txd_send_data_word: // txd_buffer_in_hand->data, txd_temp2 is number of words left in lump, txd_temp is number of bytes left in burst
    gip_extrd 0x60    // extrd postbus tx d fifo 0
    ldr r0, [txd_buffer_in_hand], #4
    subs txd_temp2, txd_temp2, #1
    beq txd_start_burst
txd_sent_data_word:    
    subs txd_temp, txd_temp, #4
    bgt txd_send_data_word
    b txd_sent_burst

txd_start_burst:  
    gip_utximm 'B'    
    gip_clear_semaphores 1, 27
    gip_extrd 0x6c    // extrd postbus tx c 0
    ldr r0, write_tx_data_8_cmd
4:  
    gip_test_semaphores 1, 23
    beq 4b
    gip_clear_semaphores 1, 23
    mov txd_temp2, #8
    b txd_sent_data_word

txd_sent_burst:
    rsbs txd_temp2, txd_temp2, #7 // txd_temp2 = (8-number of words pushed)-1
    blt txd_no_final_partial_burst
    gip_utximm 'P'    
    gip_clear_semaphores 1, 27
    ldr txd_temp, write_tx_data_n_cmd
    gip_extrd 0x6c    // extrd postbus tx c 0
    orr r0, txd_temp, txd_temp2, lsl #postbus_command_source_gip_tx_length_start

4:  
    gip_test_semaphores 1, 23
    beq 4b
    gip_clear_semaphores 1, 23
    
txd_no_final_partial_burst: 

    /*b Sent a burst of data - send 'tx packet' command if this is first burst of a packet
    */
    gip_utximm 'T'    
    tst txd_length, #1<<31
    bne not_first_burst_of_packet

    gip_utximm 'C'    

    gip_extrd 0x60    // extrd postbus tx d fifo 0
    mov r0, #0x01000000 // send packet immediately
    ldr txd_temp, =0x21800000 // full duplex, ipg etc,
    gip_extrd 0x60    // extrd postbus tx d fifo 0
    orr r0, txd_temp, txd_length
    gip_extrd 0x6c    // extrd postbus tx c 0
    ldr r0, write_tx_cmd_cmd

4:  
    gip_test_semaphores 1, 23
    beq 4b
    gip_clear_semaphores 1, 23

not_first_burst_of_packet:

    /*b Reduce length to send - if packet completes, then move to next packet
    */
    gip_utximm 'L'    
    sub txd_length, txd_length, txd_burst
    bics txd_temp, txd_length, #1<<31 // ignore top bit and test for zero
    orr txd_length, txd_length, #1<<31
    bne packet_still_in_progress

    gip_utximm 'P'    

    mov txd_temp, #0 // clear ready so we don't pick it up again, and then set done so isr picks it up
    str txd_temp, [txd_next_buffer_to_tx, #t_gipeth_buffer__ready]
    str txd_next_buffer_to_tx, [txd_next_buffer_to_tx, #t_gipeth_buffer__done]
    ldr txd_next_buffer_to_tx, [txd_next_buffer_to_tx, #t_gipeth_buffer__next_in_list]
    mov txd_buffer_in_hand, #0
    ldr txd_temp, [txd_next_buffer_to_tx, #t_gipeth_buffer__ready] // is next one ready?
    cmp txd_temp, #0
    ldrne txd_buffer_in_hand, [txd_next_buffer_to_tx, #t_gipeth_buffer__data]
    ldrne txd_length, [txd_next_buffer_to_tx, #t_gipeth_buffer__length]

packet_still_in_progress:   
    
    /*b Put tx data from storage
    */
    gip_utximm 'S'    
    cmp txd_buffer_in_hand, #0
    orrne r12, r12, #2
    gip_extrn 28
    add txd_temp, r12, #t_gipeth__tx_next_buffer_to_tx
    stmia txd_temp, {txd_next_buffer_to_tx, txd_buffer_in_hand, txd_length}

    /*b Done
    */
    mov pc, lr

    /*b Numbers we need
    */
write_tx_data_8_cmd: 
    .word    ((0)<<postbus_command_route_start) | ((8-1)<<postbus_command_source_gip_tx_length_start) | ((23)<<postbus_command_source_gip_tx_signal_start) | ((0)<<postbus_command_target_io_fifo_start) | (2<<postbus_command_target_io_dest_type_start)
write_tx_data_n_cmd: 
    .word    ((0)<<postbus_command_route_start) | ((0)<<postbus_command_source_gip_tx_length_start) | ((23)<<postbus_command_source_gip_tx_signal_start) | ((0)<<postbus_command_target_io_fifo_start) | (2<<postbus_command_target_io_dest_type_start)
write_tx_cmd_cmd: 
    .word    ((0)<<postbus_command_route_start) | ((2-1)<<postbus_command_source_gip_tx_length_start) | ((23)<<postbus_command_source_gip_tx_signal_start) | ((8)<<postbus_command_target_io_fifo_start) | (2<<postbus_command_target_io_dest_type_start)

/*f gipeth_hw_thread_handle_rx
    on entry - r12 = need to set soft irq
    preserve r13, can set (but not clear) r12
*/
gipeth_hw_thread_handle_rx: 
    
    gip_utximm 'R'
    gip_atomic_block 1
    gip_utximm 'X'    
    gip_atomic_block 1
    
    /*b Define registers - r12, r13, r14 must not be used
    */
rx_temp       .req r0
rx_temp2      .req r1        
rx_time       .req r2
rx_status     .req r3
rx_block_size .req r4
rx_dest       .req r5
rx_size_so_far .req r6 // could overlap with temp    
rx_next_buffer_to_reuse .req r8 // note these 3 must be in order
rx_buffer_in_hand       .req r9 // note these 3 must be in order
rx_length               .req r10 // note these 3 must be in order

    /*b Read the status (time/status) pair - issue the command to the postbus, wait for it to come back, then read the time and status
    on entry - none
    on exit - time, status, next_buffer_to_reuse, buffer_in_hand, length
    */
 gip_utximm 'S'    

    gip_extrd 0x60    // extrd postbus tx d fifo 0
    ldr r0, read_rx_status_data
    gip_extrd 0x6c    // extrd postbus tx c 0
    ldr r0, read_rx_status_cmd

 gip_utximm 'W'    

    gip_extrn 28
    add rx_temp, r12, #t_gipeth__next_buffer_to_reuse
    ldmia rx_temp, {rx_next_buffer_to_reuse, rx_buffer_in_hand, rx_length}

0:
    gip_test_semaphores 1, 23
    beq 0b
    
 gip_utximm 'G'    
    gip_extrm 0x60 // postbus rx d 0
    mov rx_time, r0 // time
    gip_extrm 0x60 // postbus rx d 0
    mov rx_status, r0 // status
    gip_clear_semaphores 1, 23

    /*b Break out the status; request data from rx fifo
        reason = (status>>24)&7;
        r2 = block_size = (status>>16)&0xff; // read this (number of bytes+3)/4 words from the data FIFO unless there was overflow!
        size_so_far = (status&0xffff);
    on entry - time, status, next_buffer_to_reuse, buffer_in_hand, length
    on exit - time, status, block_size, next_buffer_to_reuse, buffer_in_hand, length
     */
 gip_utximm 'B'    
    mov rx_block_size, rx_status, lsr #16
    ands rx_block_size, rx_block_size, #0xff // number of words in rx data FIFO indicated by this status
    beq 1f

    ldr rx_temp, read_rx_data_data // request 'n' words from the rx data fifo, with 'n' coming in postbus_command_source_io_length_start
    add rx_temp2, rx_block_size, #3
    mov rx_temp2, rx_temp2, lsr #2 // ((block_size(bytes)+3)/4) = block_size in words
    gip_extrd 0x60    // extrd postbus tx d fifo 0
    orr r0, rx_temp, rx_temp2, lsl #postbus_command_source_io_length_start
    gip_extrd 0x6c    // extrd postbus tx c 0
    ldr r0, read_rx_data_cmd

1:  

    /*b Now figure out what to do with the data while it comes
    // 0 => fcs ok; 1=>fcs bad; 2=> odd nybbles; 3=> complete block; 4=>fifo overrun!; 5=>framing error
    // we get 03400040, 03400080, 00120092 for a 146 byte packet
    on entry - time, status, block_size, next_buffer_to_reuse, buffer_in_hand, length
    on exit - status, block_size, dest, next_buffer_to_reuse, buffer_in_hand, length (updated)
    dest = 0 -> discard, buffer_in_hand must be 0
    buffer_in_hand = 0 -> length invalid
     */
 gip_utximm 'D'    
    mov rx_dest, #0 // default is to discard
    bic rx_size_so_far, rx_status, #0xff000000
    bic rx_size_so_far, rx_size_so_far, #0x00ff0000
    cmp rx_size_so_far, #64
    bgt middle_of_packet

    // start of packet (size_so_far is from the status we just read)
    mov rx_length, rx_size_so_far
    movs rx_dest, rx_buffer_in_hand
    strne rx_time, [rx_buffer_in_hand, #-4]
    bne receive_data  // start of packet with an active packet already - just restart the buffer

    // else get first buffer from list - if it is not done, then use it
    ldr rx_temp, [rx_next_buffer_to_reuse, #t_gipeth_buffer__done]
    cmp rx_temp, #0
    beq receive_data // if buffer is not ready then dump the data

    mov rx_temp, #0
    ldr rx_dest, [rx_next_buffer_to_reuse, #t_gipeth_buffer__data]
    str rx_temp, [rx_next_buffer_to_reuse, #t_gipeth_buffer__done]
    mov rx_buffer_in_hand, rx_dest
    b receive_data

middle_of_packet:
    cmp rx_buffer_in_hand, #0 // if no buffer in hand (i.e. unexpected middle packet), then just throw the data away
    beq receive_data

    add rx_dest, rx_buffer_in_hand, rx_length
    add rx_length, rx_length, rx_block_size
    cmp rx_length, #GIPETH_BUFFER_LENGTH
    ble receive_data // if we have room for the data, receive it

    mov rx_dest, #0 // if we do not have room for the data, throw it away!
    mov rx_buffer_in_hand, #0
    b receive_data

receive_data:   

    /*b Wait for any new data and discard or copy to buffer
    on entry - status, block_size, dest, next_buffer_to_reuse, buffer_in_hand, length
    on exit - status, next_buffer_to_reuse, buffer_in_hand, length
     */
 gip_utximm 'W'    
    cmp rx_block_size, #0
    beq check_status

2:  
    gip_test_semaphores 1, 23
    beq 2b

    cmp rx_dest, #0
    beq discard_data

3:  
    gip_extrd 0x60
    str r0, [rx_dest], #4
    subs rx_block_size, rx_block_size, #4
    bgt 3b
    b check_status

discard_data:     
 gip_utximm 'D'    
  
3:  
    gip_extrm 0x60
    mov rx_temp, r0
    subs rx_block_size, rx_block_size, #4
    bgt 3b
    b check_status

    /*b Handle status of the block
    on entry - status, next_buffer_to_reuse, buffer_in_hand, length
    on exit - next_buffer_to_reuse, buffer_in_hand, length
     */
check_status:
 gip_utximm 'S'    
    movs rx_status, rx_status, lsr #24
    beq handle_fcs_ok
    cmp rx_status, #3
    bne handle_error_status
    // here the status is 3 - block received, not end of packet - so we are done

    /*b Block receive complete - push registers back to the data structure
    on entry - next_buffer_to_reuse, buffer_in_hand, length
    on exit - 
     */
block_done: 
 gip_utximm 'E'    
    gip_extrn 28
    add rx_temp, r12, #t_gipeth__next_buffer_to_reuse
    stmia rx_temp, {rx_next_buffer_to_reuse, rx_buffer_in_hand, rx_length}
    mov pc, lr

    /*b FCS_OK - Packet received - set its ready, set interrupt for microkernel, move on to next rx buffer
    on entry - status, next_buffer_to_reuse, buffer_in_hand, length
    on exit - next_buffer_to_reuse, buffer_in_hand, length
    */
handle_fcs_ok:
    cmp rx_buffer_in_hand, #0
    beq packet_disarded
 gip_utximm 'F'    
    str rx_next_buffer_to_reuse, [rx_next_buffer_to_reuse, #t_gipeth_buffer__ready]
    str rx_length, [rx_next_buffer_to_reuse, #t_gipeth_buffer__length]
    ldr rx_next_buffer_to_reuse, [rx_next_buffer_to_reuse, #t_gipeth_buffer__next_in_list]
    mov rx_buffer_in_hand, #0
    orr r12, r12, #1
    b block_done
packet_disarded:
 gip_utximm 'D'    
    b block_done

    /*b Bad status (FCS bad, odd nybbles, FIFO overrun, framing error) - for now just discard any data we have
    on entry - status, next_buffer_to_reuse, buffer_in_hand, length
    on exit - next_buffer_to_reuse, buffer_in_hand, length
    */
handle_error_status:        
 gip_utximm 'E'    
    mov rx_buffer_in_hand, #0
    b block_done

    /*b Numbers we need
    */
read_rx_status_data: 
    .word    (0<<postbus_command_source_io_cmd_op_start) | (2<<postbus_command_source_io_length_start) | (23<<postbus_command_target_gip_rx_semaphore_start)
read_rx_status_cmd: 
    .word    ((0)<<postbus_command_route_start) | ((1-1)<<postbus_command_source_gip_tx_length_start) | ((0)<<postbus_command_source_gip_tx_signal_start) | ((12)<<postbus_command_target_io_dest_start) | (0<<postbus_command_target_io_dest_type_start)
read_rx_data_data:      
    .word (0<<postbus_command_source_io_cmd_op_start) | (23<<postbus_command_target_gip_rx_semaphore_start)
read_rx_data_cmd: 
    .word    ((0)<<postbus_command_route_start) | ((1-1)<<postbus_command_source_gip_tx_length_start) | ((0)<<postbus_command_source_gip_tx_signal_start) | ((4)<<postbus_command_target_io_dest_start) | (0<<postbus_command_target_io_dest_type_start)

/*a GIP ethernet ISR code
 */
/*f gipeth_isr_asm
    This is called with the return stack frame (ptregs structure) on the stack
    Basically we clear the interrupt bit in r17, call our handler, then return from interrupt
    */
ENTRY(gipeth_isr_asm)
 gip_utximm '<'    
    .word 0xec00d111
    bic r1, r1, #2
    mov r0, sp
	bl gipeth_isr
 gip_utximm '>'    
    gip_trap 0, r0 // trashing r0 is fine, as this returns to frame

        
        
   // fatal unknown trap - reboot!
mkt_unknown_trap:
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
    gip_utximm 'M'
    gip_utximm 'K'
    gip_utximm 'T'
    gip_utximm ' '
    gip_utximm 'U'
    gip_utximm 'N'
    gip_utximm 'K'
    gip_utximm '\n'

    .word 0xec0058e // disable the timer
     mov r0, #1<<31 

     mov pc, #0
