#include <gip_macros.h>
#include <gip_system.h>
#include <postbus_asm.h>

.text
	.align 4

.text
	.align 4

    .global wrapper_entry_point
  	.type	 wrapper_entry_point,function
wrapper_entry_point:

    gip_extrd 0x60
    mvn r0, #0 // 32 1s
    gip_extrd 0x6c
    mov r0, #0x0c000000

    gip_atomic_block 1
    gip_atomic_block 1
        
    gip_extrd 0x60 
    mov r0, #0x01000000
    gip_extrd 0x60
    ldr r0, mdio_write_cmd_data
    gip_extrd 0x6c
    ldr r0, mdio_write_cmd_cmd

    gip_atomic_block 1
    gip_atomic_block 1

    gip_extrd 0x60
    ldr r0, mdio_write_eth_phy_data
    gip_extrd 0x6c
    mov r0, #0x0c000000
    
    gip_atomic_block 1
    gip_atomic_block 1

    gip_extrd 0x60 
    mov r0, #0x01000000
    gip_extrd 0x60
    ldr r0, mdio_write_cmd_data
    gip_extrd 0x6c
    ldr r0, mdio_write_cmd_cmd

    mov pc, lr
    ldr pc, return_address

// return_address:  .word 0x8000489c
return_address:  .word 0x242c // go back to the polling loop start from the outside... it should be okay
    
mdio_write_eth_phy_data:    
    .word 0x51820100
mdio_write_cmd_data:    
    .word 0x10700a20
mdio_write_cmd_cmd:
    .word 0x4c000100

.Lfe1:
	.size	 wrapper_entry_point,.Lfe1-wrapper_entry_point
