<?php

include "web_locals.php";
include "${toplevel}web_assist/web_globals.php";

site_set_location("gip.structure");

$page_title = "GIP Documentation";

include "${toplevel}web_assist/web_header.php";

page_header( "GIP Structure" );

page_sp();
?>

<?php page_section( "overview", "Overview" ); ?>

The GIP has the following logical components:

<ul>

<li>
Prefetch unit

<li>
Decode unit (16-bit and ARM emulation here)

<li>
Register file read stage

<li>
ALU pipeline stage

<li>
Coprocessor read stage

<li>
Memory section (effectively two stages of pipeline)

<li>
Coprocessor write stage

<li>
Register file write stage

</ul>

<?php page_section( "Prefetch", "Prefetch stage" ); ?>

<p>
The prefetch stage takes addresses and requests from the decode stage, and presents instructions to the decode stage. Effectively the interface between
the two is somewhat burst-memory-like; address requests or sequential requests are presented, data is presented in response, and the prefetch unit
marries up the data it presents with the addresses.
</p>

<p>
One extra bell in the interface between the decode unit and the prefetch unit, though, is the speculative fetch; the prefetch unit
is assumed to have some form of local read-ahead capability, and the decode unit may request an address be speculatively fetched in to 
the local read-ahead in case a branch is to be taken or a return from a function call is to be executed.
</p>

<p>
The prefetch unit will actually have three lines of data, each one with an address; effectively this is a fully associative cache. The prefetch unit will automatically read-ahead as sequential instructions are presented to the decode unit, and also fill a line on a speculative request, and also (of course) on 
a demand fetch (i.e. non-speculative address request).
</p>


<?php page_section( "Decode", "Decode stage" ); ?>

<p>
The decode stage of the GIP contains two decoders; the first is a native 16-bit instruction decoder, the second is an ARM 32-bit instruction emulator.
These decoders effectively run in parallel; the GIP will be in one of three operating modes (native 16-bit, ARM 32-bit emulation, or idle) and the
internal instructions and prefetch decodes for the appropriate mode will be handled appropriately.
</p>

<p>
The decode unit talks to the prefetch stage, the register file read stage, and the local scheduler; the prefetch stage interface is discussed above,
and that leaves the register file read stage and the local scheduler.
</p>

<p>
The decode unit talks to the scheduler by taking schedule requests from the scheduler and responding with an acknowledgement. The decode unit may block
schedule requests to support atomic sections of a program, and may also block until an actual deschedule occurs if the thread it is executing
is running in a cooperative mode (i.e. cannot be preempted).
</p>

<p>
The decode unit talks to the register file read stage by presenting a combinatorial decode of instruction data (which it has registered on input),
selected from the appropriate decode mode, and taking an acknowledgement back from the stage. Additionally the register file read stage may
present a 'flush' indication; this should be monitored, and it indicates that an instruction has been executed that requested a pipeline flush. Such
an instruction is expected to force a write of the program counter with some value on a cycle following the flush, and instruction execution is
expected to continue from that location.
</p>

<p>
Thread descheduling through preemption requires the ALU accumulator, ALU shifter, and the ALU flags to be preserved. A cooperative deschdule does not
preserve these items.
</p>

<?php
page_ep();

include "${toplevel}web_assist/web_footer.php"; ?>







