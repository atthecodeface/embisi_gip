<?php

include "web_locals.php";
include "${toplevel}web_assist/web_globals.php";
include "local_header.php";
?>

<?php page_section( "enhancements", "Enhancements to the ARM instruction set" ); ?>

<p>

<ul>

<li>Ability to execute native 16-bit GIP instructions (no condition) (single cycle decode, single instruction)

<li>Access (for MOV and test/set, test/clear, etc) unconditionally to all 32 registers (GIP instruction?)

<li>Ability to hold off descheduling for a number of ARM instructions (GIP instruction?)

<li>Ability to deschedule the current process (GIP instruction?)

<li>Ability to do a thunking call (multicycle decode, single instruction)

<li>Ability to do a SWI (couple of moves, deschedule, assert event) (multicycle decode, single instruction)

<li>Ability to make return from interrupt happen... How?

<li>Force enable of hardware interrupts (macro)

<li>Restore interrupt enable (macro)

<li>Disable interrupts, returning previous state (macro)

</ul>

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
