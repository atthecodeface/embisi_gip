<?php

include "web_locals.php";
include "${toplevel}web_assist/web_globals.php";

site_set_location($site_location);

$page_title = "GIP Documentation";

include "${toplevel}web_assist/web_header.php";

page_header( "ARM Emulation Microkernel" );

page_sp();
?>

<?php page_section( "communication", "Communication primitives" ); ?>

These communication primitives manipulate two basic structures: the
ARM mode code thread, and a local SRAM data block that contains the
ARM 'state', basically the register set (r0 to r12, r14), user mode
stack pointer (r13), flags (zcnv), PC (r15), 'SVC mode' stack pointer
(r13_svc).

<table border=1 class=data>

<tr>

<th>Primitive</th>

<td>GIP operation</td>

<td>Microkernel operation</td>

</tr>


<tr>
<th>System call</th>

<td>

Deschedules ARM mode code thread

<br>

Stores system call number in a useful register

<br>

Sets R15 to be the next address to execute

<br>

Indicates the microkernel to be scheduled

</td>

<td>

Save ARM state in a local SRAM data block

<br>

Move system call number to some register

<br>

May want to recover r13 from local SRAM data block

<br>

Change ARM mode code thread PC to be the SWI vectoring routine

<br>

Enable ARM mode code thread

<br>

Deschedule microkernel

</td>

</tr>


<tr>
<th>Return from system call</th>

<td>

Deschedules ARM mode code thread

<br>

Indicates the microkernel to be scheduled

</td>

<td>

May want to save r13 'svc mode' in local SRAM data block

<br>

Recover full ARM state in a local SRAM data block, except r0 (system call return value)

<br>

Change ARM mode code thread flags to be the recovered flags.

<br>

Change ARM mode code thread PC to be the recovered PC.

<br>

Enable ARM mode code thread

<br>

Deschedule microkernel

</td>

</tr>


<tr>
<th>Return from interrupt</th>

<td>

Indicates the microkernel to be scheduled

</td>

<td>

May want to save r13 'svc mode' in local SRAM data block

<br>

Recover full ARM state in a local SRAM data block

<br>

Change ARM mode code thread flags to be the recovered flags.

<br>

Change ARM mode code thread PC to be the recovered PC.

<br>

Enable ARM mode code thread

<br>

Deschedule microkernel

</td>

</tr>



<tr>
<th>Save ARM state</th>

<td>

</td>

<td>

Transfers ARM state in local SRAM block to a main memory location

</td>

</tr>



<tr>
<th>Recover ARM state</th>

<td>

</td>

<td>

Transfers ARM state from a main memory location to the local SRAM block

</td>

</tr>



</table>

<?php
page_ep();

include "${toplevel}web_assist/web_footer.php"; ?>
