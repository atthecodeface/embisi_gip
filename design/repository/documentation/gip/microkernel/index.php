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

This documentation contains a number of pages:

<ul>

<li>This page includes a very basic summary of the microkernel operation.

<li>The basic requirements of the microkernel are derived from the way the <a href="linux.php">ARM port of Linux uses processor modes</a> 

<li>The operation of the microkernel is <a href="detailed_operation.php">described in detail</a>

<li>The <a href="communication.php">communication primitives</a> required of the microkernel supply the mechanisms for interaction between ARM code and the microkernel.


<?php page_section( "overview", "Overview" ); ?>

<p>

The microkernel provides the capability of a GIP to support a full-blown OS with hardware interrupts.

<p>

The basic implementation requires three classes of thread:

<ul>

<li>Microkernel thread (one instance)

<li>ARM mode code thread (one instance for all user and supervisor code)

<li>Hardware interrupt code threads (one instance per hardware interrupt type)

</ul>

The operation can be summarized fairly simply. Assume the GIP is running a user application.

<ol>

<li>A hardware interrupt comes in, starting a hardware interrupt thread. This thread
handles the interrupt provisionally, and signals the microkernel thread to schedule.

<li>The microkernel thread schedules, and notices a hardware interrupt has occurred. It preserves the state of the ARM mode code thread, sets that up to run the interrupt code, manages some local interrupt
enable masks, allows the ARM mode code thread to be scheduled, and then deschedules itself.

<li>This lets the ARM mode code thread schedule, and it runs the interrupt driver code. On completion
this code requests a restart of the user mode code with interrupts reenabled, by messaging the microkernel thread and descheduling.

<li>The microkernel thread gets scheduled, clears interrupt masks, resets the ARM mode code thread to have the user application registers and flags, sets the thread to run, and deschedules.

<li>The ARM mode code thread schedules again, and continues the user application.

</ol>

</p>

<p>

So the basic concept is that the ARM mode code thread can indicate to
the microkernel that a system call is required, or the hardware
interrupt threads indicate that an interrupt is required, and in
either case effectively a software interrupt occurs in the ARM mode
code thread. The microkernel saves and restores the ARM mode code
thread state; replacing the user application with some kernel
code. The kernel code runs, and completes. The microkernel can restore
the user application state in the ARM mode code thread, and it will
run again.

</p>

<?php
page_ep();

include "${toplevel}web_assist/web_footer.php"; ?>
