<?php

include "web_locals.php";
include "${toplevel}web_assist/web_globals.php";

site_set_location($site_location);

$page_title = "GIP Documentation";

include "${toplevel}web_assist/web_header.php";

page_header( "ARM Emulation Microkernel" );

page_sp();
?>

<?php page_section( "outline_code", "Outline code" ); ?>

The microkernel is effectively a state machine. In standard running
there will be an ARM mode code thread running a user process, and
the microkernel will be in 'user' state. From this state it may get a system call from the ARM code, and enter 'system call' state; in 'system call' state it may then receive hardware interrupts, or schedule a different user process, or a return from system call.

The main entry point for the microkernel is entered on a semaphore
going high. This semaphore may be triggered by a SWI or by a hardware
interrupt routine; in either case, it is triggered by another piece of
software running on the same GIP in another thread.

<p>

The first thing the code does is to determine the source of the
interrupt. If it is software then it enters the SWI entry code. If it
is hardware then it has to test if hardware interrupts are disabled;
if so, the interrupt is posted (somehow for now). If they are enabled
then two cases may occur: first, the ARM thread is not running, in
which case the ARM thread should be started at the interrupt vector
for the interrupt, with interrupts disabled, and with the registers
appropriately set. If the ARM thread is running then it must have its
state preserved (registers and flags), be marked as running, and then
the process continues as if the ARM thread had already not been
running. When the hardware interrupt routine may reenable interrupts;
we will ignore that for now. If it does not, then it must eventually
return, and upon doing so it invokes the microkernel thread again,
which will reenable interrupts, and check to see if the ARM thread
needs to be restarted.


<?php page_section( "despatch", "Despatch" ); ?>

This is the code that occurs when the microkernel was idling, and when
either a hardware or software interrupt occurs.  It examines the
source of the interrupt, clearing the indication atomically. It then
despatches to the correct routines until all the interrupt sources are
handled.

<?php code_format( "gip", "code/despatch.s" );?>

<?php page_section( "swi_entry", "SWI entry code" ); ?>

<?php code_format( "gip", "code/swi_entry.s" );?>

<?php
page_ep();

include "${toplevel}web_assist/web_footer.php"; ?>
