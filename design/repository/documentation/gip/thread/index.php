<?php

include "web_locals.php";
include "${toplevel}web_assist/web_globals.php";

site_set_location($location);

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

<?php
page_ep();

include "${toplevel}web_assist/web_footer.php"; ?>

