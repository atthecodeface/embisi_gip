<?php

include "web_locals.php";
include "${toplevel}web_assist/web_globals.php";

site_set_location($site_location);

$page_title = "GIP Documentation";

include "${toplevel}web_assist/web_header.php";

page_header( "GIP Threading and Scheduling" );

page_sp();
?>

<?php page_section( "overview", "Overview" ); ?>

The GIP supports eight threads. They can be run prioritised or
nonprioritised: if they are prioritised then there are three priority
levels (low, medium and high); one low priority, three medium
priority, four high priority.

<p>

The threads may be scheduled with preemption or without. If preemption
is disabled then threads will run to completion, or until they yield;
this is cooperative multithreading. Prioritization may still be used,
and it will effect which thread is scheduled following a yield; if
prioritization is disabled, then a round-robin scheme is used.

<p>

If preemption is enabled then a low priority thread may be preempted
by a medium or high priority thread, and a medium priority thread by a
high priority thread. The threads themselves have the capability of
locally disabling preemption (a little like an interrupt disable flag)
to support atomic updates of structures. This disabling is global; that is, a low priority thread that disables preemption will stop high and medium priority threads from preemtping it - it is not possible to stop just medium priority threads from preempting.

<p>

With the preemption scheme described, there can be at most three
active threads (one running, two preempted) simultaneously. In some
systems this would entail pushing the state of the threads on to a
stack, or mirroring the entire state. The GIP, however, only preserves
the condition flags and accumulator on a small register file in the ALU itself. It
does not preserve the state of the main register file, which is
designed to be shared. It does not, either, preserve the shifter register. If these are to be preserved over
preemption then the software must do that explicitly. Note that ARM
emulation does not require the accumulator or shifter to be preserved,
so ARM mode threads can be run and preempted, and preempt, without
issue.

<p>

Threads are scheduled to run by using four semaphores local to each
thread. A thread is registered as having an interest in the levels of
a number of those semaphores (high or low). When the semaphores are
appropriately set the thread becomes schedulable. The thread is
expected to start running (if it actually gets scheduled) at a
specified PC in a specified mode (GIP or ARM, sticky flags, ...); this
data is stored in the scheduler register file.

<?php page_section( "implementation", "Thread switching implementation" ); ?>

The scheduler communicates with the decode and register file stages of the GIP pipeline to implement thread switching.
The scheduler may request of the decode the following:

<ol>

<li>Schedule thread 'n' with GIP pipeline configuration 'c' at address 'pc' from nothing running

<li>Preempt current thread with thread 'n' with GIP pipeline configuration 'c' at address 'pc'

<li>Resume preempted thread 'n' with GIP pipeline configuration 'c' at address 'pc'

</ol>

The decode is responsible for acknowledging these requests, and indeed
it completely controls actual thread switching. In response to a
preempt request the GIP pipeline will, at some point, if the request
is acknowledged, signal that the preempted thread has been preempted -
the thread may not spontaneosuly deschedule following the request
being acknowledged.

<p>

The GIP pipeline communicates descheduling and preemption events to
the scheduler. It does this from the register file read stage of the
pipeline. The two potential messages that can be given are:

<ol>

<li>Preemption occurred

<li>Thread descheduled

</ol>

The mechanism the scheduler uses for communicating with the decoder is
a bus with the following components and protocol: <dl>

<dt>Request indication 

<dd>1 for request for schedule, 0 for no request (idle)

<dt>Thread number

<dd>Number of thread requesting scheduling; only changes the clock
edge that request goes high

<dt>GIP configuration

<dd>Configuration values for the thread to be scheudled; only changes
on the clock edge that request goes high

<dt>Acknowledge

<dd>From the decode logic to the scheduler, only asserted if the
previous cycle was a schedule request, indicating that the request was
acted upon

</dl>

The scheduler may present requests whenever it chooses, but if it
decides to change the presented request (perhaps because a higher
priority thread becomes available when a previous request has not been
acknowledged) then it must first go idle (remove its request): in the
idle cycle (as can be seen from the protocol above) the request
content must not have changed, so if the race condition of an
acknowledge occurring simultaneously with a request change happens
then the decoder and scheduler will not have to backtrack state.

<?php page_section( "round_robin", "Round robin mechanism" ); ?>

The round robin mechanism is not a single cycle round robin
resolver. It uses multiple cycles to reduce the circuitry required. On
each clock edge the mechanism basically examines the next two threads
after the one it last examined to determine if either of them is
ready; it selects the appropriate one if so. If neither is then on the
next cycle it examines the next pair of threads, and so on. This means
that there is an extra delay when scheduling round robin, particularly
if all the threads are not used. However, the overall performance
impact should not be that high.

<?php page_section( "state", "State" ); ?>

The GIP contains the following per-thread state:

<table border=1>

<tr><th>Size</th><th>Description</th></tr>

<tr><th>4</th><td>Signalling semaphores</td></tr>
<tr><th>1</th><td>Restart mode - 0 for native, 1 for ARM mode</td></tr>
<tr><th>32</th><td>Program counter to restart at (full 32 bits seems over the top)</td></tr>
<tr><th>8</th><td>Restart interest - which of the four signalling flags are interesting, and what their values should be</td></tr>
<tr><th>1</th><td>Running</td></tr>
<tr><th>?</th><td>Pipeline configuration - sticky flags, native GIP decode mode, etc; not too many, and not highly valued</td></tr>
</table>

The signalling flags can be viewed as a whole as a 32-bit register,
known as SchedulingSignallingSemaphores. As such the flags are numbered
from 0 to 31, with 0 to 3 being for thread 0, 4 to 7 for thread 1, and
so on. Individual flags may be signalled using GIP instructions.

<p>

Furthermore the scheduler maintains a round-robin state of 3 bits of
the next thread to run; this state is used when the current thread is
accepted for running by the GIP, the scheduler works on scheduling the
next thread. It will start with the round-robin thread number given.

<p>

Also the scheduler has a bit indicating whether it is running preempting or cooperatively.

<p>

The scheduler maintains the knowledge of the currently running thread.

<p>

The scheduler also determines what thread to run next, and registers that.

<?php page_section( "operations", "Operations" ); ?>

<dl>

<dt>Clear flag

<dd>

ISCHDSEMBIC[CC][A] Rm/Imm [-> Rd]

<br>

This uses the semaphore flags in the mode currently specified for the
system, reads them, performs a BIC operation with the specified
operand, and can put the result in Rd. To clear a particular sempahore based on a register
set the accumulator to 1 shl semaphore number and do ISCHSEMBIC Acc; to do a particular semaphore from an immediate value do ISCHSEMBIC #1 shl semaphore.

<dt>Toggle flag

<dd>

Use ISCHSEMXOR, similar to clearing flags

<dt>Set flag (flag absolute, register based, relative to thread 'n')

<dd>

Use ISCHSEMOR, similar to clearing flags

<dt>Read and set/clear/toggle flag

<dd>

Use an atomic pair with a read followed by a set/clear/toggle

<dt>Read all signalling flags

<dd>

Use ISCHSEMOR #0 -> Rd

<dt>Specify thread to operate on

<dd>

May specify current thread or other thread. Takes 2 clock ticks to take effect.

<dt>Read restart info for operation thread

<dd>

ISCHINFO -> Rd

<dt>Read restart program counter for operation thread

<dd>

ISCHPC -> Rd

<dt>Write restart/running info for operation thread

<dd>

ISCHINFO Rm/Imm

<dt>Write restart program counter for operation thread

<dd>

ISCHPC Rm/Imm

<br>

If the thread specified is currently preempted then that thread is descheduled.

<dt>Write restart info and program counter for running thread and deschedule (clears running bit)

<dd>

ISCHRESTART Rm/Imm + PC somehow


</dl>

<?php
page_ep();

include "${toplevel}web_assist/web_footer.php"; ?>

