<?php

include "web_locals.php";
include "${toplevel}web_assist/web_globals.php";

site_set_location($location);

$page_title = "GIP Documentation";

include "${toplevel}web_assist/web_header.php";

page_header( "GIP Register File" );

page_sp();
?>

<?php page_section( "overview", "Overview" ); ?>

<p> The GIP register file module contains both the actual register
file and registering of the control signals required to access the
register file, plus the register scoreboarding. It implements both the
register file read pipeline stage and the register file write pipeline
stage. It also supplies a same-cycle interface to the special registers (DAGs, semaphores, repeat count, zol ctrs, etc) and the postbus register file/commands </p>

<p> In order to support use of the PC as an operand in instructions,
and as the register file does not hold the PC, there is an input to
the register file read stage which is used as a PC value (or PC+offset
- this depends on the decode/emulation), and the register file read
stage can multiplex this value onto either of its read outputs if the
PC is desired by the ALU as opposed to a true register file value.</p>

<p>

The register file is given register numbers that correspond to one of the following:

<ul>

<li>None - a physical register may be read anyway

<li>A physical register (0 to 31)

<li>An internal register (PC, ACC, SHF, or condition)

<li>A special register (data address generator, repeat count, etc)

<li>A postbus register (numbered 0 to 63; 0-3 are command/FIFO 0, 4-7 are command/FIFO 1: 0 is command/status, 1 is FIFO control/status, 2 is FIFO read data, 3 is FIFO write data, 32 through 63 are direct access to the register file)

<li>A peripheral (numbered 0 to 63; reads and writes of peripheral space actually cause the register file to block reading/writing, and they take 3 cycles to complete)

</ul>

</p>

A vital note is that there is a single read port to the postbus, and a
single read port of the special registers. An instruction that reads
either of these twice will get garbage in both values; however, they
may both be read in the same instruction. (i.e. reading a special and
a post bus register works; reading two special registers does not).

<?php page_ep();

include "${toplevel}web_assist/web_footer.php"; ?>

