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
to support atomic updates of structures.

<p>

With the preemption scheme described, there can be at most three
active threads (one running, two preempted) simultaneously. In some
systems this would entail pushing the state of the threads on to a
stack, or mirroring the entire state. The GIP, however, only preserves
the condition flags on a small register file in the ALU itself. It
does not preserve the state of the main register file, which is
designed to be shared. It does not, either, preserve the accumulator
register or the shifter register. If these are to be preserved over
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

