<?php

include "web_locals.php";
include "${toplevel}web_assist/web_globals.php";

site_set_location($site_location);

$page_title = "GIP Documentation";

include "${toplevel}web_assist/web_header.php";

page_header( "ARM Emulation Microkernel" );

page_sp();
?>

<?php page_section( "overview", "Overview" ); ?>

<p>

The microkernel provides the capability of a GIP to support a full-blown OS with hardware interrupts.
</p>

<p>

The basic implementation requires three classes of thread:

<ul>
<li>Microkernel thread (one instance)
<li>ARM mode code thread (one instance for all user and supervisor code)
<li>Hardware interrupt code threads (one instance per hardware interrupt type)
</ul>

The operation can be summarized fairly simply. Assume the GIP is totally idle.
A hardware interrupt comes in, starting a hardware interrupt thread. This thread
handles the interrupt provisionally, and signals the microkernel thread to schedule.
The microkernel thread schedules, notices a hardware interrupt has occurred, then
it arranges the parameters for the ARM mode code thread, manages some local interrupt
enable masks, allows the ARM mode code thread to be scheduled, and then deschedules itself.
This lets the ARM mode code thread schedule, and it runs the interrupt driver code. On completion
it 'exits from IRQ mode', by messaging the microkernel thread and then descheduling.
The microkernel thread gets scheduled, notes what has happened, clears interrupt masks, and everything
continues from the start.

</p>

<p>
The microkernel is basically responsible for scheduling the ARM mode code thread, and if any
preemption is required, or mode switching (on a system call, for example) then the microkernel is
responsible for that. The basic communication between them is scheduling management; this is akin to interrupt
management, except it contains more information than 'disable', 'restore', 'enter' and 'exit' from interrupt routines, which
is what interrupt masking generally provides for.
</p>


<?php page_section( "microkernel_thread", "Microkernel Thread" ); ?>

<p>
The microkernel thread, which is written in GIP 16-bit code, takes requests from
hardware threads and the ARM thread, and despatches execution of ARM
code. The microkernel thread is responsible for ARM register 'banking': R13 and
R14 management.
</p>

<p>
So there are two entry points in to the microkernel thread, and as it is
a single thread they are mutually exclusive (i.e. not reentrant). As
the hardware only supports a single event to wake up a hardware
thread, we will have to have a differentiating mechanism and a single
mutex/semaphore to wake the microkernel; the suggested mechanism would
be a register with 32 bits which provides for 31 different hardware
interrupt sources and one software interrupt source.
</p>

<p>
A SWI always deschedules the ARM execution thread. 
</p>

<p>
Hardware interrupts occur through the messaging to the microkernel
entry point, and the microkernel will 'deschedule' the ARM thread,
replace it with the ARM hardware interrupt handler code, and then
return to its entry point waiting for a SWI ONLY (as hardware
interrupts are disabled).
</p>

<p>
The microkernel thread wakes up on the event occurred.
</p>

<p>
It clears the event.
</p>

<p>
It reads and clears the 'which interrupts register' atomically.
</p>

<p>
It then invokes the ARM thread with each interrupt vector requested
if linux interrupts are enabled. (Question; can we do this instead
with a mutex? We would need to have an atomic read and write
instruction; also useful to have an atomic write. But can the
descheduling be handled well if the requested mutex is not available?
That should only happen for the microkernel thread, as the ARM thread
can only run if the microkernel thread is not running, and the
microkernel thread would then not own the mutex)
</p>

<p>
What is the prod to the microkernel though? Sounds like a
semaphore. No, its an event
</p>

<p>
We will have an atomic test/set bit that can be read and written in a
single instruction, and it will have an associated change action
response mechanism that can be dynamically configured, such that
particular edges on the value of the bit can be set to invoke a
corresponding event (which may also be forced to occur by other
sources too including from other processors).
</p>

<p>
Microkernel on entry from SWI from kernel mode occurs only, we
believe, from init to do an exec; from user mode for all other SWI
calls (probably). From user mode code the portable kernel code has to
be able to modify r13/r14, and visibility of them so that preemptive
code can do a task switch.
Currently the ARM Linux layer, on entry to a SWI or on an interrupt,
pushes all sixteen uesr mode registers (inc PC) on to the kernel
stack. We may want to change that from the kernel stack to save
accesses to SDRAM.
</p>

<?php page_section( "arm_code_thread", "ARM Mode Code Thread" ); ?>

<p>
ARM execution thread, which runs OS code, application code, etc. All
the ARM code.
</p>

<?php page_section( "interrupt_threads", "Hardware interrupt threads" ); ?>

<p>
The basic operation of a hardware interrupt thread is:

<ol>
<li>Wait for stuff on a register or something from an external device (or
other GIP)

<li>Get stuff from that register; if it has enough, or a packet, or something
then notify the microkernel that there is something to do by setting
a bit in the which interrupts register and the microkernel event occurred thing.

<li>Return, but keep giving it stuff.

</ol>


</p>

<?php page_section( "system_calls", "System calls" ); ?>

<p>
Software interrupts are explicit communications between the ARm thread
then microkernel thread.  Hardware interrupts as far as the OS is
concerned are effectively internally generated within the microkernel,
using messages from the hardware threads.
</p>

<?php page_section( "thread_priorities", "Thread priorities" ); ?>

<p>
Hardware threads should be equal top priority, nonpreemptable.
Microkernel thread should be second priority, preemptable by hardware
threads.
ARM thread should be lowest priority, preemptable by any of the above.
</p>

<?php page_section( "communication_primitives", "Communication primitives" ); ?>




----------------------------------------------------------------------------
Thunking dynamic libraries
We can use r17 or some other register to contain a base address of
dynamic library thunking table assists; the dynamic mapping of
registers to support this in this particular way is patentable.

Best method is to have a small table of static data pointers whose
base address is in r17 indexed by local library number, and a global
table of entry points for functions in the libraries indexed by global
entry point number whose base is in r18
We can have one instruction that loads 'r12' with 'r17, #...' and pc
with 'r18, #entryptr<<2' - we can use a quarter of the SWI instruction
decode.

<?php
page_ep();

include "${toplevel}web_assist/web_footer.php"; ?>
