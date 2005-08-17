/*a
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

/*a Defines
 */
//#define DEBUG
    
#define ENTRY(name) \
  .globl name; \
  name:

/*a Includes
 */
#define GIP_INCLUDE_FROM_ASM
#ifdef REGRESSION
#include <gip_macros.h>
#include <gip_system.h>
#include <postbus_asm.h>
#include "gip_video_capture.h"
#else
#include <asm/gip_macros.h>
#include <asm/gip_system.h>
#include <asm/postbus_asm.h>
#include "gip_video_capture.h"
#endif    

#ifdef DEBUG    
#define debug_ch(a) gip_utximm a
#else
#define debug_ch(a)
#endif    

/*a GIP video capture hardware thread code
 */
/*f gip_video_capture_hw_thread
    Runs as thread 5
    Uses r27
    r27 -> t_gip_vfc structure for the driver
    Semaphore 20 is virtual par status fifo ne
    Semaphore 21 is virtual rx data fifo nne
    Semaphore 22 is 'work to be done' from the device driver
    Semaphore 23 is the postbus txd0/comm0 access semaphore

    The hardware thread basically has two tasks
    It sets up a frame start taking configuration - this is basically done by setting up a structure and hitting the soft ISR
    It takes received data and status and stores it in a buffer that it has been given - if the buffer overflows then data is thrown away
    When the frame completes the ISR is informed by clearing the buffer pointer and asserting the microkernel interrupt
*/
ENTRY(gip_video_capture_hw_thread)

    /*b Preserve registers
    */
 debug_ch('P')    
    gip_extrn VFC_PRIVATE_REG_F
    str r0, [VFC_PRIVATE_REG_R, #t_gip_vfc__regs]
    gip_extrn VFC_PRIVATE_REG_F
    add r0, VFC_PRIVATE_REG_R, #t_gip_vfc__regs
    stmib r0, {r1-r14}

    /*b Read and clear semaphores 20,22,23 - don't clear 21 unless we take data from the rx fifo
    */
    mov r12, #0
    gip_read_and_clear_semaphores (SEM_VFC_STATUS_VNE | SEM_VFC_SOFT_IRQ | SEM_VFC_POSTBUS )>>(4*VFC_THREAD_NUM), (4*VFC_THREAD_NUM), 13
    tst r13, #SEM_VFC_RX_DATA_VWM
    blne hw_thread_handle_rx_data // take data out of the rx fifo if there is a watermark lump there
    tst r13, #SEM_VFC_STATUS_VNE
    blne hw_thread_handle_par_status
    tst r13, #SEM_VFC_SOFT_IRQ
    blne hw_thread_handle_soft_irq

    /*b Set microkernel interrupt if required
    */
    tst r12, #1
    beq 0f

    gip_extrdrn 0x11, 0x11
    orr r1, r1, #MK_INT_BIT_VFC
    gip_set_semaphores 1, SEM_NUM_MICROKERNEL_HW_INT

0:

    /*b Recover registers
    */
    gip_extrn VFC_PRIVATE_REG_F
    add r0, VFC_PRIVATE_REG_R, #t_gip_vfc__regs
    ldmia r0, {r0-r14}

    /*b Exit
    */
    nop
    nop
 debug_ch('D')
    nop
    nop
 debug_ch('\n')
    nop
    nop
    .word 0xec007305  // deschedule */
    nop

/*f hw_thread_handle_rx_data
    on entry - r12 = need to set soft irq
    preserve r13, can set (but not clear) r12

    There is at least 64*4 bytes in the FIFO
    Take out 64*3 bytes and either store or discard
 */
hw_thread_handle_rx_data:    

    debug_ch('R')
    
    /*b Define registers - r12, r13, r14 must not be used
    */
rxd_temp       .req r3
rxd_temp2      .req r4
rxd_burst      .req r5
rxd_buffer_store   .req r9  // note these 3 must be in order
rxd_buffer_length  .req r10 // note these 3 must be in order
rxd_data_length    .req r11 // note these 3 must be in order

    /*b Get rx data from storage
    */
    gip_extrn VFC_PRIVATE_REG_F
    add rxd_temp, VFC_PRIVATE_REG_R, #t_gip_vfc__rxd_buffer_store
    ldmia rxd_temp, {rxd_buffer_store, rxd_buffer_length, rxd_data_length}

    /*b Request data in 16 word lumps - we fetch 3 bursts of 16, so that the vne flag works with data arriving simultaneously to our removing it
    */
    mov rxd_burst, #3
rxd_burst_fetch:        
    gip_clear_semaphores (SEM_VFC_RX_DATA_VWM | SEM_VFC_POSTBUS )>>(4*VFC_THREAD_NUM), (4*VFC_THREAD_NUM)
    gip_extrd 0x60    // extrd postbus tx d fifo 0
    ldr r0, read_rx_data_16_data
    gip_extrd 0x6c    // extrd postbus tx c 0
    ldr r0, read_rx_data_16_cmd

    /*b Decide if there is room - rxd_temp is where to put the data, 0 for discard
    */
    add rxd_temp, rxd_buffer_store, rxd_data_length
    cmp rxd_buffer_store, #0
    moveq rxd_temp, #0
    add rxd_temp2, rxd_data_length, #16*4
    cmp rxd_temp2, rxd_buffer_length
    movgt rxd_temp, #0

    /*b Wait for data
    */
0:
    gip_test_semaphores 1, SEM_NUM_VFC_POSTBUS
    beq 0b
    gip_clear_semaphores 1, SEM_NUM_VFC_POSTBUS

    /*b Copy the data to the buffer - writes do not occur if condition fails, but the RF reads do so the postbus FIFO is emptied either way
    */
    cmp rxd_temp, #0
    gip_extrd 0x60
    strne r0, [rxd_temp], #4
    gip_extrd 0x60
    strne r0, [rxd_temp], #4
    gip_extrd 0x60
    strne r0, [rxd_temp], #4
    gip_extrd 0x60
    strne r0, [rxd_temp], #4
    gip_extrd 0x60
    strne r0, [rxd_temp], #4
    gip_extrd 0x60
    strne r0, [rxd_temp], #4
    gip_extrd 0x60
    strne r0, [rxd_temp], #4
    gip_extrd 0x60
    strne r0, [rxd_temp], #4

    gip_extrd 0x60
    strne r0, [rxd_temp], #4
    gip_extrd 0x60
    strne r0, [rxd_temp], #4
    gip_extrd 0x60
    strne r0, [rxd_temp], #4
    gip_extrd 0x60
    strne r0, [rxd_temp], #4
    gip_extrd 0x60
    strne r0, [rxd_temp], #4
    gip_extrd 0x60
    strne r0, [rxd_temp], #4
    gip_extrd 0x60
    strne r0, [rxd_temp], #4
    gip_extrd 0x60
    strne r0, [rxd_temp], #4

    /*b Update data length
    */
    add rxd_data_length, rxd_data_length, #16*4

    /*b Do another burst if required
    */
    sub rxd_burst, rxd_burst, #1
    bne rxd_burst_fetch

    /*b Store registers back
    */
    gip_extrn VFC_PRIVATE_REG_F
    add rxd_temp, VFC_PRIVATE_REG_R, #t_gip_vfc__rxd_buffer_store
    stmia rxd_temp, {rxd_buffer_store, rxd_buffer_length, rxd_data_length}

    /*b Done
    */
    mov pc, lr

    /*b Numbers we need
    */
read_rx_data_16_data:      
    .word (0<<postbus_command_source_io_cmd_op_start) | (16<<postbus_command_source_io_length_start) | (SEM_NUM_VFC_POSTBUS<<postbus_command_target_gip_rx_semaphore_start)
read_rx_data_16_cmd: 
    .word    ((0)<<postbus_command_route_start) | ((1-1)<<postbus_command_source_gip_tx_length_start) | ((0)<<postbus_command_source_gip_tx_signal_start) | ((4|IO_A_SLOT_PARALLEL_0)<<postbus_command_target_io_dest_start) | (0<<postbus_command_target_io_dest_type_start)

/*f hw_thread_handle_soft_irq
    on entry - r12 = need to set soft irq
    preserve r13, can set (but not clear) r12

    do an array of IO slot configurations
 */
hw_thread_handle_soft_irq:

    debug_ch('I')

    /*b Define registers - r12, r13, r14 must not be used
    */
si_temp       .req r2
si_temp2      .req r3
si_slot                  .req r9 // note these 3 must be in order
si_action_array          .req r10 // note these 3 must be in order
si_action_array_length   .req r11 // note these 3 must be in order

    /*b Get action and despatch
    */
    gip_extrn VFC_PRIVATE_REG_F
    add si_temp, VFC_PRIVATE_REG_R, #t_gip_vfc__si_slot
    ldmia si_temp, {si_slot, si_action_array, si_action_array_length}
    mov si_temp2, #0
    str si_temp2, [si_temp, #t_gip_vfc__si_action_array_length - t_gip_vfc__si_slot]

    /*b If action array has all been done then bail
    */
1:      
    debug_ch('A')
    subs si_action_array_length, si_action_array_length, #1
    orrlt r12, r12, #1
    movlt pc, lr

    /*b Do next action
        on entry:   si_slot, si_action_array, si_action_array_length
        on exit:    si_slot, si_action_array, si_action_array_length
    */
    gip_clear_semaphores 1, SEM_NUM_VFC_POSTBUS
    ldr si_temp2, [si_action_array], #4
    gip_extrd 0x60
    mov r0, #0x01000000 // send command immediately
    gip_extrd 0x60
    mov r0, si_temp2
    ldr si_temp, write_tx_cmd_cmd
    orr si_temp, si_temp, si_slot, lsl #postbus_command_target_io_fifo_start
    gip_extrd 0x6c    // extrd postbus tx c 0
    mov r0, si_temp
2:  
    gip_test_semaphores 1, SEM_NUM_VFC_POSTBUS
    beq 2b

    b 1b

    /*b Numbers we need
    */
write_tx_cmd_cmd: 
    .word    ((0)<<postbus_command_route_start) | ((2-1)<<postbus_command_source_gip_tx_length_start) | ((SEM_NUM_VFC_POSTBUS)<<postbus_command_source_gip_tx_signal_start) | ((8)<<postbus_command_target_io_fifo_start) | (2<<postbus_command_target_io_dest_type_start)
    

/*f hw_thread_handle_par_status
    on entry - r12 = need to set soft irq
    preserve r13, can set (but not clear) r12

    basically a status indicates some amount of data is ready
    get the status
    if we have not yet gathered that much data then get it (down to the word)
    then handle the status
*/
hw_thread_handle_par_status: 
    
    /*b Define registers - r12, r13, r14 must not be used
    */
sts_temp       .req r0
sts_temp2      .req r1        
sts_time       .req r2
sts_status     .req r3
sts_block_size .req r4
sts_burst_size .req r5
sts_buffer_store     .req r8 // note these 3 must be in order
sts_buffer_length    .req r9 // note these 3 must be in order
sts_data_length      .req r10 // note these 3 must be in order

    /*b Read the status (time/status) pair - issue the command to the postbus, wait for it to come back, then read the time and status
    on entry - none
    on exit - time, status, next_buffer_to_reuse, buffer_in_hand, length
    */
 debug_ch('S')    

    gip_extrd 0x60    // extrd postbus tx d fifo 0
    ldr r0, read_status_data
    gip_extrd 0x6c    // extrd postbus tx c 0
    ldr r0, read_status_cmd

    gip_extrn VFC_PRIVATE_REG_F
    add sts_temp, VFC_PRIVATE_REG_R, #t_gip_vfc__rxd_buffer_store
    ldmia sts_temp, {sts_buffer_store, sts_buffer_length, sts_data_length}

0:
    gip_test_semaphores 1, SEM_NUM_VFC_POSTBUS
    beq 0b
    
    gip_extrm 0x60 // postbus rx d 0
    mov sts_time, r0 // time
    gip_extrm 0x60 // postbus rx d 0
    mov sts_status, r0 // status
    gip_clear_semaphores 1, SEM_NUM_VFC_POSTBUS

#define PIXELS_PER_BYTE 4
#define LOG_PIXELS_PER_BYTE 2    
    /*b Check for amount of data received - the status has number of captures, which is bytes/(pixels per byte)
    */
    bic sts_block_size, sts_status, #0xff000000
    add sts_block_size, sts_block_size, #PIXELS_PER_BYTE-1
    mov sts_block_size, sts_block_size, lsr #LOG_PIXELS_PER_BYTE
    sub sts_block_size, sts_block_size, sts_data_length
    ble sts_all_data_received

    /*b Now receive 'sts_block_size' bytes, rounded up to words (or discard them if we haven't got room)
    */
sts_read_block_size_bytes:      
    debug_ch('R')
    ldr sts_temp, read_rx_data_data // request 'n' words from the rx data fifo, with 'n' coming in postbus_command_source_io_length_start
    add sts_burst_size, sts_block_size, #3
    mov sts_burst_size, sts_burst_size, lsr #2 // ((block_size(bytes)+3)/4) = block_size in words

    // Now watch out - we cannot read more than 16 words at a time!
#define BURST_READ_SIZE (12)
    cmp sts_burst_size, #BURST_READ_SIZE
    movgt sts_burst_size, #BURST_READ_SIZE
    gip_extrd 0x60    // extrd postbus tx d fifo 0
    orr r0, sts_temp, sts_burst_size, lsl #postbus_command_source_io_length_start
    gip_extrd 0x6c    // extrd postbus tx c 0
    ldr r0, read_rx_data_cmd

    /*b Now figure out what to do with the data while it comes
    on entry - time, status, block_size, next_buffer_to_reuse, buffer_in_hand, length
    on exit - status, block_size, dest, next_buffer_to_reuse, buffer_in_hand, length (updated)
    dest = 0 -> discard, buffer_in_hand must be 0
    buffer_in_hand = 0 -> length invalid
     */
 debug_ch('D')    
    add sts_temp, sts_buffer_store, sts_data_length
    cmp sts_buffer_store, #0
    moveq sts_temp, #0
    add sts_data_length, sts_data_length, sts_burst_size, lsl #2 // update data_length - if the buffer is invalid, then we discard anyway
    sub sts_block_size, sts_block_size, sts_burst_size, lsl #2 // take this burst from the block size
    cmp sts_data_length, sts_buffer_length
    movgt sts_temp, #0

    /*b Wait for any new data and discard or copy to buffer
    on entry - status, block_size, dest, next_buffer_to_reuse, buffer_in_hand, length
    on exit - status, next_buffer_to_reuse, buffer_in_hand, length
     */
 debug_ch('W')    
2:  
    gip_test_semaphores 1, SEM_NUM_VFC_POSTBUS
    beq 2b

    gip_clear_semaphores 1, SEM_NUM_VFC_POSTBUS
    cmp sts_burst_size, #BURST_READ_SIZE
    blt sts_read_smaller_burst

    cmp sts_temp, #0
    gip_extrd 0x60
    str r1, [sts_temp], #4
    gip_extrd 0x60
    str r1, [sts_temp], #4
    gip_extrd 0x60
    str r1, [sts_temp], #4
    gip_extrd 0x60
    str r1, [sts_temp], #4
    gip_extrd 0x60
    str r1, [sts_temp], #4
    gip_extrd 0x60
    str r1, [sts_temp], #4

    gip_extrd 0x60
    str r1, [sts_temp], #4
    gip_extrd 0x60
    str r1, [sts_temp], #4
    gip_extrd 0x60
    str r1, [sts_temp], #4
    gip_extrd 0x60
    str r1, [sts_temp], #4
    gip_extrd 0x60
    str r1, [sts_temp], #4
    gip_extrd 0x60
    str r1, [sts_temp], #4

    cmp sts_block_size, #0
    bgt sts_read_block_size_bytes
    b sts_all_data_received

sts_read_smaller_burst: 
3:  
    cmp sts_temp, #0
    gip_extrd 0x60
    strne r1, [sts_temp], #4
    subs sts_burst_size, sts_burst_size, #1
    bgt 3b

    cmp sts_block_size, #0
    bgt sts_read_block_size_bytes

    /*b Now check for final status - that means the buffer is ready
    */
sts_all_data_received:      
    tst sts_status, #0x80000000
    orreq r12, r12, #1
    moveq sts_buffer_store, #0
    
    /*b Push registers back to the data structure
    on entry - next_buffer_to_reuse, buffer_in_hand, length
    on exit - 
     */
 debug_ch('E')    
    gip_extrn VFC_PRIVATE_REG_F
    add sts_temp, VFC_PRIVATE_REG_R, #t_gip_vfc__rxd_buffer_store
    stmia sts_temp, {sts_buffer_store, sts_buffer_length, sts_data_length}

    mov pc, lr

    /*b Numbers we need
    */
read_status_data: 
    .word    (0<<postbus_command_source_io_cmd_op_start) | (2<<postbus_command_source_io_length_start) | (SEM_NUM_VFC_POSTBUS<<postbus_command_target_gip_rx_semaphore_start)
read_status_cmd: 
    .word    ((0)<<postbus_command_route_start) | ((1-1)<<postbus_command_source_gip_tx_length_start) | ((0)<<postbus_command_source_gip_tx_signal_start) | ((12|IO_A_SLOT_PARALLEL_0)<<postbus_command_target_io_dest_start) | (0<<postbus_command_target_io_dest_type_start)
read_rx_data_data:      
    .word (0<<postbus_command_source_io_cmd_op_start) | (SEM_NUM_VFC_POSTBUS<<postbus_command_target_gip_rx_semaphore_start)
read_rx_data_cmd: 
    .word    ((0)<<postbus_command_route_start) | ((1-1)<<postbus_command_source_gip_tx_length_start) | ((0)<<postbus_command_source_gip_tx_signal_start) | ((4|IO_A_SLOT_PARALLEL_0)<<postbus_command_target_io_dest_start) | (0<<postbus_command_target_io_dest_type_start)

/*a GIP video capture ISR code
 */
/*f gip_video_capture_isr_asm
    This is called with the return stack frame (ptregs structure) on the stack
    Basically we clear the interrupt bit in r17, call our handler, then return from interrupt
    */
ENTRY(gip_video_capture_isr_asm)
 debug_ch('[')    
    .word 0xec00d111
    bic r1, r1, #MK_INT_BIT_VFC
	bl gip_video_capture_isr
 debug_ch(']')    
    gip_trap 0, r0 // trashing r0 is fine, as this returns to frame

        
        
   // fatal unknown trap - reboot!
mkt_unknown_trap:
    ldr r0, =0x7f999990
    strb r0, [r0]
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
    gip_extrd 27
    str r0, [r0, #27*4]
    gip_utximm 'V'
    gip_utximm 'F'
    gip_utximm 'C'
    gip_utximm ' '
    gip_utximm 'U'
    gip_utximm 'N'
    gip_utximm 'K'
    gip_utximm '\n'

    .word 0xec00c58e // disable the timer
     mov r0, #1<<31 

     mov pc, #0

