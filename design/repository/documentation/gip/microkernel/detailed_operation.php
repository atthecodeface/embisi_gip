<?php

include "web_locals.php";
include "${toplevel}web_assist/web_globals.php";

site_set_location($site_location);

$page_title = "GIP Documentation";

include "${toplevel}web_assist/web_header.php";

page_header( "ARM Emulation Microkernel" );

page_sp();
?>

<?php page_section( "modes", "Modes" ); ?>

The microkernel has a concept of which 'mode' the ARM is running in, but it is primitive. It has a flag that it keeps in memory (or register?) that is 1 for SVC mode, 0 for user mode.

<?php page_section( "flags", "Flags" ); ?>

The microkernel maintains a single flag for interrupt enable. This is a flag which is in the scheduler, which may cause the microkernel to be scheduled when it changes from 0 (interrupts disabled) to 1 (interrupts enabled).

<?php page_section( "primitives", "Primitives" ); ?>

The microkernel supports the following primitives:

<dl>

<dt>SWI invoked (r10-r14 = user mode register values, r15 = calling PC+8, r16 = instruction)

<dd>The highest-priority primitve; it is invoked generally from a hardware decode, and the microkernel expects r0 to r15 to be the requesting arguments and r16 to be the instruction that caused the invocation, so that it may be decoded from a table for despatch. The microkernel will preserve r0 to r15 in a fixed region of memory, and restart the ARM with r14 equal to the given r15, and use a depatch table for the start value based on r16. It will also mark itself as in 'SVC' mode, and clear the interrupts enabled flag.

<dt>Enter USER mode at address (r10-r14 = user mode register values, r15 = restart PC, r16 = instruction)

<dd>This is invoked by ARM code explicitly, and is effectively a kind of SWI. It takes a single argument, that of the address to start the ARM at. The microkernel will clear its 'SVC' mode flag, and restart the ARM mode code thread at the value of r15.

<dt>Handle interrupt

<dd>This is invoked by hardware interrupt threads. It has a lower priority than SWIs being invoked - if a SWI and a hardware interrupt occur simultaneously (as far as the microkernel is concerned) the SWI invocation must be handled first. The interrupt code examines the interrupt enable flag; if it is low, then the microkernel register an interest in it rising, then deschedules with no interest in messages from hardware interrupt primitives. If it is high (interrupts enabled), or when the rising-interrupt-enabled interest is detected then r0 to r15 are preserved in a fixed region of memory, and the ARM is restarted at a PC of 0x18.

</dl>


<p>
<?php
page_ep();

include "${toplevel}web_assist/web_footer.php"; ?>
