<?php

include "web_locals.php";
include "${toplevel}web_assist/web_globals.php";

site_set_location($site_location);

$page_title = "GIP Documentation";

include "${toplevel}web_assist/web_header.php";

page_header( "ARM Emulation Microkernel" );

page_sp();
?>

<?php page_section( "states", "States" ); ?>

The microkernel has six states, and transitions between them according to the following table:

<table border=1>
<tr><th>State</th><th>Primitive / event</th><th>Action</th><th>Next state</th></tr>

<tr>
<th>User state</th>
<td>SystemCall</td>
<td>Disable interrupts, save R0, R13, R14, set R13 to system mode R13, R14 to return address, R0 to system call number, change to SystemState, set ARM mode code thread PC to system call vectoring routine</td>
<td>System state, interrupts disabled</td>
</tr>

<tr>
<th>User state</th>
<td>UndefinedInstruction</td>
<td>Disable interrupts, save R0, R13, R14, set R13 to system mode R13, R14 to return address, R0 to instruction, change to SystemState, set ARM mode code thread PC to undefined instruction routine</td>
<td>System state, interrupts disabled</td>
</tr>

<tr>
<th>User state</th>
<td>HardwareInterrupt</td>
<td>Disable interrupts, save R0, R13, R14, PC, set R13 to system mode R13, R0 to interrupt number, change to SystemState, set ARM mode code thread PC to user mode interrupt routine</td>
<td>System state, interrupts disabled</td>
</tr>

<tr>
<th>System state, interrupts enabled</th>
<td>HardwareInterrupt</td>
<td>Disable interrupts, save R0, R13, R14, PC, set R0 to interrupt number, set ARM mode code thread PC to system mode interrupt routine</td>
<td>System state, interrupts disabled</td>
</tr>

<tr>
<th>System state, interrupts disabled</th>
<td>HardwareInterrupt</td>
<td>Do nothing</td>
<td>System state, interrupts disabled</td>
</tr>

<tr>
<th>System state, interrupts disabled</th>
<td>Interrupts reenabled, hardware interrupt pending</td>
<td>Disable interrupts, save R0, R13, R14, PC, set R0 to interrupt number, set ARM mode code thread PC to system mode interrupt routine</td>
<td>System state, interrupts disabled</td>
</tr>

<tr>
<th>System state, interrupts disabled</th>
<td>Interrupts reenabled, no hardware interrupt pending</td>
<td>Do nothing</td>
<td>System state, interrupts enabled</td>
</tr>

<tr>
<th>System state, interrupts enabled</th>
<td>ReturnFromInterruptToUser</td>
<td>Recover R13, R14 from user bank; set ARM mode code thread PC and flags to interrupted PC and flags, change to UserState</td>
<td>User state</td>
</tr>

<tr>
<th>System state, interrupts disabled</th>
<td>ReturnFromInterruptToUser</td>
<td>Recover R13, R14 from user bank; set ARM mode code thread PC and flags to interrupted PC and flags, change to UserState, enable interrupts</td>
<td>User state</td>
</tr>

<tr>
<th>System state, interrupts enabled</th>
<td>ReturnFromInterruptToSystem</td>
<td>Set ARM mode code thread PC and flags to interrupted PC and flags</td>
<td>System state, interrupts enabled</td>
</tr>

<tr>
<th>System state, interrupts disabled</th>
<td>ReturnFromInterruptToSystem</td>
<td>Set ARM mode code thread PC and flags to interrupted PC and flags, enable interrupts</td>
<td>System state, interrupts enabled</td>
</tr>

</table>

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
