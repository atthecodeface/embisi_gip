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

<p> The microkernel is effectively a state machine. In standard
running there is an ARM mode code thread running a user process
(thread 0), and the microkernel (thread 1) will be in 'user'
state. From this state it may get a system call from the ARM code, and
enter 'system call' state, handle that call and then return to 'user'
state; it may also receive hardware interrupts, which are handled in
'system' state; process scheduling may be performed quite readily with
the given primitives.

<p>

The microkernel thread utilizes a single semaphore for interaction with the ARM mode code thread. The semaphore is set by the ARM mode code thread, and cleared by the microkernel thread.

<p>

The microkernel thread utilizes a single semaphore for interaction
with the hardware interrupt threads, and an interrupt status
register. The semaphore is set by any of the hardware interrupt
threads to indicate a pending interrupt, after the hardware interrupt
thread sets its bit in the interrupt status register. The semaphore is
cleared by the microkernel when it reads the interrupt status register
to determine what hardware interrupt to handle next.

<p>

The microkernel thread utilizes a single semaphore for
enabling/disabling interrupts. This semaphore may be set and cleared
at will by the ARM mode code thread. It is set and cleared sometimes
by the microkernel thread (see the <a
href="communication.php">communication primitives page</a> for more
details). If a hardware interrupt is requested with the hardware
interrupt semaphore while interrupts are marked as disabled then the
microkernel will watch for the interrupt enable bit semaphore to
be set; if a hardware interrupt is requested while the interrupts are
not disabled, or if a hardware interrupt is pending when the
microkernel schedules on the interrupt enable being set then it is just handled
normally.

</p>

<p>
The GIP contains a PC stack/triple in the GIP which holds the PCs of the
three levels of process: high (hardware), medium (microkernel), low (ARM).
</p>

<p>
ARM SWI instructions map on to two different microkernel call instructions in the GIP system, identified with one of the bits in the system call number. Standard system call invoking SWIs set R15 to a return address and another register to the system call number, set the microkernel threads soft semaphore, and deschedule the ARM mode code thread. Other microkernel invoking SWIs do the same except they do not set R15 to a return address.

<p>
Undefined ARM instructions act similarly to microkernel invoking SWIs; the system call number, though, will contain the full ARM instruction, and thus will be an out-of-range microkernel call number.
</p>

<p>
The hardware 'interrupt' threads schedule on an external event from hardware,
generate data, put in a buffer, mark a bit in the shared interrupt
status register corresponding to their interrupt number, and set the microkernel hardware interrupt pending semaphore, before descheduling back to the external event.
</p>

<p>
So the microkernel operates in six states:

<ol>

<li>

User mode, no interrupt pending

<li>

System mode, no interrupt pending, interrupts enabled

<li>

System mode, no interrupt pending, interrupts disabled

<li>

User mode, interrupt pending

<li>

System mode, interrupt pending, interrupts enabled

<li>

System mode, interrupt pending, interrupts disabled

</ol>

Initially we will assume the microkernel is in user mode with no
interrupt pending.  In this state it waits for either the soft
semaphore or the interrupt pending semaphore to go high. When the
microkernel is scheduled it checks first if the soft semaphore is high
- this is the higher priority path. If it is, then the type of call is
determined, and the appropriate action taken. This action will result
in the ARM mode code thread being scheduled. However, if there is an
interrupt pending as it is finally scheduled (i.e. when the
microkernel would deschedule) then the pending interrupt is
handled. Note that the soft semaphore may have caused a change of
state in the microkernel!

<br>

The pending interrupts are handled differently depending on the microkernel state:

<dl>

<dt>System mode, interrupt pending, interrupts disabled

<dd>
In this case the
interrupt cannot be handled yet. The ARM mode code thread is let to
run, and the microkernel waits on one of two events: interrupts being
enabled, or the ARM mode code thread setting the soft call
semaphore. Soft semaphores are handled at high priority as above. If
the interrupt enable semaphore has become set, then the microkernel will
enter its 'interrupt pending, interrupts enabled' state and handle the interrupt as below.

<dt>System mode, interrupt pending, interrupts enabled

<dd>

Here the interrupt may be handled. The microkernel reads the hardware interrupt status register, clears the hardware
interrupt pending semaphore, and determines which interrupt to
handle. The microkernel will then execute the 'handle hardware
interrupt' code, disabling interrupts, and run the ARM mode code
thread.

</dl>

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
interrupts are disabled). Note that this permits hardware interrupt
code in the ARM mode to perform SWIs. This is not readily supported on
the ARM, as the entry mechanism for SWIs on the ARM is not reentrant
(if a hardware interrupt occurs simultaneously with a user mode SWI
call then the hardware interrupt would have to preserve r14_svc prior
to calling the SWI; also, all SWIs would have to support reentrancy)

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

<p>

The ARM mode code thread communicates with the microkernel in four basic ways:

<ol>

<li>
It can set and clear the interrupt enable semaphore (this should only really be done from 'system' mode)

<li>
It can read and write the (fixed address) saved user and system registers and interrupted flags and PC

<li>

It can invoke a microkernel system call (from 'user' mode) to force the ARM mode code execution to go in to system mode at the system call vectoring location

<li>

It can invoke a microkernel functional call (from 'system' mode generally) to force the ARM mode code execution to return to user mode at a specified location

</ol>

<?php page_section( "interrupt_threads", "Hardware interrupt threads" ); ?>

<p>
The basic operation of a hardware interrupt thread is:

<ol>
<li>Wait for stuff on a register or something from an external device (or
other GIP)

<li>Get stuff from that register; if it has enough, or a packet, or something
then notify the microkernel that there is something to do by setting
a bit in the interrupt status register and by setting the hardware interrupt pending semaphore.

<li>Return, but keep giving it stuff.

</ol>


</p>

<?php page_section( "thread_priorities", "Thread priorities" ); ?>

<p>
Hardware threads should be equal top priority, nonpreemptable.
Microkernel thread should be second priority, preemptable by hardware
threads.
ARM thread should be lowest priority, preemptable by any of the above.
</p>

<?php
page_ep();

include "${toplevel}web_assist/web_footer.php"; ?>
