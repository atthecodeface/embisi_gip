<?php

include "web_locals.php";
include "${toplevel}web_assist/web_globals.php";

site_set_location("gip.microkernel");

$page_title = "GIP Documentation";

include "${toplevel}web_assist/web_header.php";

page_header( "ARM Emulation Microkernel" );

page_sp();
?>

<?php page_section( "overview", "Overview" ); ?>

<pre>
Two + n threads:

n hardware threads, which are GIP code, which present requests in some
form to the microkernel for particularly pieces of ARM code to be
executed.

Microkernel thread, which is GIP code, which takes requests from
hardware threads and the ARM thread, and despatches execution of ARM
code. Microkernel is responsible for ARM register 'banking'; R13 and
R14 management.

ARM execution thread, which runs OS code, application code, etc. All
the ARM code.



Hardware threads should be equal top priority, nonpreemptable.
Microkernel thread should be second priority, preemptable by hardware
threads.
ARM thread should be lowest priority, preemptable by any of the above.

Software interrupts are explicit communications between the ARm thread
then microkernel thread.  Hardware interrupts as far as the OS is
concerned are effectively internally generated within the microkernel,
using messages from the hardware threads.

So there are two entry points in to the microkernel thread, and as its
a single thread they are mutually exclusive (i.e. not reentrant). As
the hardware only supports a single event to wake up a hardware
thread, we will have to have a differentiating mechanism and a single
mutex/semaphore to wake the microkernel; the suggested mechanism would
be a register with 32 bits which provides for 31 different hardware
interrupt sources and one software interrupt source.

A SWI always deschedules the ARM exeuction thread. 

Hardware interrupts occur through the messaging to the microkernel
entry point, and the microkernel will 'deschedule' the ARM thread,
replace it with the ARM hardware interrupt handler code, and then
return to its entry point waiting for a SWI ONLY (as hardware
interrupts are disabled).




Hardware thread state machine

1. Wait for stuff on a register or something from an external device (or
other GIP)

2. Get stuff from that register; if it has enough, or a packet, or something
then notify the microkernel that there is something to do by setting
a bit in the which interrupts register and the microkernel event occurred thing.

3. return, but keep giving it stuff.


The microkernel thread wakes up on the event occurred.

It clears the event.

It reads and clears the 'which interrupts register' atomically.

It then invokes the ARM thread with each interrupt vector requested
if linux interrupts are enabled. (Question; can we do this instead
with a mutex? We would need to have an atomic read and write
instruction; also useful to have an atomic write. But can the
descheduling be handled well if the requested mutex is not available?
That should only happen for the microkernel thread, as the ARM thread
can only run if the microkernel thread is not running, and the
microkernel thread would then not own the mutex)

What is the prod to the microkernel though? Sounds like a
semaphore. No, its an event

We will have an atomic test/set bit that can be read and written in a
single instruction, and it will have an associated change action
response mechanism that can be dynamically configured, such that
particular edges on the value of the bit can be set to invoke a
corresponding event (which may also be forced to occur by other
sources too including from other processors).



Microkernel on entry from SWI from kernel mode occurs only, we
believe, from init to do an exec; from user mode for all other SWI
calls (probably). From user mode code the portable kernel code has to
be able to modify r13/r14, and visibility of them so that preemptive
code can do a task switch.
Currently the ARM Linux layer, on entry to a SWI or on an interrupt,
pushes all sixteen uesr mode registers (inc PC) on to the kernel
stack. We may want to change that from the kernel stack to save
accesses to SDRAM.





----------------------------------------------------------------------------

So, summary:

We wil use one internal event, and an atomic test/set bit. The event
can be triggered or cleared with a GIP instruction. The atomic
test/set bit can be read, set-and-read, cleared-and-read,
written-and-read, or just written. Additionally an 'interest' may be
registered with the atomic test/set bit such that if the 'interest' is
met (such as rising edge) when a transaction is performed to the bit,
then the event is triggered.

We will have a PC stack/triple in the GIP which holds the PCs of the
three levels of process: high (hardware), medium (microkernel), low (ARM).

The atomic test/set bit is used to disable interrupts; if it is high,
then interrupts are disabled. If it is low, then interrupts are
enabled. Note that this means that the interrupt enable must be
managed appropriately with the internal scheduler...

Undefined ARM instructions will cause the PC to be stored in the PC
stack, a specified internal event to be triggered, and the thread to be descheduled.

The hardware 'interrupt' threads schedule on an external event from hardware,
generate data, put in a buffer, mark a bit in a shared interrupt
register corresponding to their interrupt number, and trigger the
internal event, before descheduling back to the external event.

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

When the event triggers now, in 'interrupt in action', it could be for
one of four reasons:

1. Interrupts being reenabled by hardware interrupt handler. In this
   case other interrupts may have to be looked at and so on; this is
   very similar to return from interrupt, except that the top of the
   interrupt vector stack is not popped.

2. Hardware interrupts from hardware threads; if this occurs and
   interrupts are disabled (as should be the case here) then they are
   ignored, and left for later (when interrupts are reenabled).

3. Return from interrupt, which looks like an undefined
   instruction. This recovers the top interrupt vector stack
   information, and then tries to execute the next highest priority
   interrupt. If none are available, then interrupts are
   reenabled. Control is returned to the ARM using the information
   from the interrupt vector stack, and the microkernel enters its 'no
   interrupt in action' state.

4. SWI or other system call, through an undefined instruction. This
   causes the standard behaviour for 'no interrupt in action'.




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

</pre>

<?php
page_ep();

include "${toplevel}web_assist/web_footer.php"; ?>







