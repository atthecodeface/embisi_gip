<?php

include "web_locals.php";
include "${toplevel}web_assist/web_globals.php";

site_set_location($site_location);

$page_title = "GIP Documentation";

include "${toplevel}web_assist/web_header.php";

page_header( "ARM Emulation Microkernel" );

page_sp();
?>

<?php page_section( "summary", "Summary" ); ?>

<p>
We wil use one internal event, and an atomic test/set bit. The event
can be triggered or cleared with a GIP instruction. The atomic
test/set bit can be read, set-and-read, cleared-and-read,
written-and-read, or just written. Additionally an 'interest' may be
registered with the atomic test/set bit such that if the 'interest' is
met (such as rising edge) when a transaction is performed to the bit,
then the event is triggered.
</p>

<p>
We will have a PC stack/triple in the GIP which holds the PCs of the
three levels of process: high (hardware), medium (microkernel), low (ARM).
</p>

<p>
The atomic test/set bit is used to disable interrupts; if it is high,
then interrupts are disabled. If it is low, then interrupts are
enabled. Note that this means that the interrupt enable must be
managed appropriately with the internal scheduler...
</p>

<p>
Undefined ARM instructions will cause the PC to be stored in the PC
stack, a specified internal event to be triggered, and the thread to be descheduled.
</p>

<p>
The hardware 'interrupt' threads schedule on an external event from hardware,
generate data, put in a buffer, mark a bit in a shared interrupt
register corresponding to their interrupt number, and trigger the
internal event, before descheduling back to the external event.
</p>

<p>
The microkernel operates in two states: interrupt in action, and no
interrupt in action. Initially no interrupt is in action. In this
state it waits for the internal event to trigger. When this occurs, it
first checks to see if an undefined ARM instruction is pending. (How
does it do this?) If it is, then the action for that undefined
instruction is taken (more below ?). Then the shared interrupt
register is read and cleared atomically (with a block scheduling
instruction), and the internal trigger event is reset. If there are
any hardware interrupts to be handled (mask the read shared interrupt
register) then they are handled one at a time. This is done by first
setting the atomic test/set bit, to mark interrupts as disabled. Then
the ARM thread is restarted with the hardware vector number for the
relevant interrupt, with the previous ARM registers (including flags
and PC) pushed on to a stack of some form. The microkernel then
configures the atomic test/set bit interest to be 'interrupt being
enabled', pushes the vector information on to the interrupt vector stack, and
deschedules, now in its second state 'interrupt in action'.
</p>

<p>
When the event triggers now, in 'interrupt in action', it could be for
one of four reasons:

<ol>
<li>
Interrupts being reenabled by hardware interrupt handler. In this
   case other interrupts may have to be looked at and so on; this is
   very similar to return from interrupt, except that the top of the
   interrupt vector stack is not popped.

<li>
Hardware interrupts from hardware threads; if this occurs and
   interrupts are disabled (as should be the case here) then they are
   ignored, and left for later (when interrupts are reenabled).

<li>
Return from interrupt, which looks like an undefined
   instruction. This recovers the top interrupt vector stack
   information, and then tries to execute the next highest priority
   interrupt. If none are available, then interrupts are
   reenabled. Control is returned to the ARM using the information
   from the interrupt vector stack, and the microkernel enters its 'no
   interrupt in action' state.

<li>
SWI or other system call, through an undefined instruction. This
   causes the standard behaviour for 'no interrupt in action'.

</ol>

</p>

<?php
page_ep();

include "${toplevel}web_assist/web_footer.php"; ?>
