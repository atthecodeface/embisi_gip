<?php

include "web_locals.php";
include "${toplevel}web_assist/web_globals.php";

site_set_location("gip.arm_emulation");

$page_title = "GIP Documentation";

include "${toplevel}web_assist/web_header.php";

page_header( "ARM Emulation in the GIP" );

page_sp();
?>

<?php page_section( "overview", "Overview" ); ?>

<p>
The ARM instruction classes in general are as follows:

<table border=1>
<tr>
<th>Class</th>
<th>Description</th>
<th>Emulation restrictions</th>
</tr>

<tr>
<th>Data processing</th>
<td>ALU, Shift, and combined instructions</td>
<td>Operations with a register-speciifed shift may not use the PC for any register other than the destination register</td>
</tr>

<tr>
<th>Multiply</th>
<td>Multiply and multiply accumulate</td>
<td>&nbsp;</td>
</tr>

<tr>
<th>Single Data Swap</th>
<td>&nbsp;</td>
<td><i>Not supported</i></td>
</tr>

<tr>
<th>Single Data Transfer</th>
<td>Load or store from register plus offset, possibly shifted</td>
<td>&nbsp;</td>
</tr>

<tr>
<th>Undefined</th>
<td>&nbsp;</td>
<td><i>Not supported</i></td>
</tr>

<tr>
<th>Block data transfer</th>
<td>Load or Store multiple</td>
<td>&nbsp;</td>
</tr>

<tr>
<th>Branch</th>
<td>Branch with or without link</td>
<td>&nbsp;</td>
</tr>

<tr>
<th>Coproc Data Transfer</th>
<td>&nbsp;</td>
<td><i>Not supported</i></td>
</tr>

<tr>
<th>Coproc Data Operation</th>
<td>&nbsp;</td>
<td><i>Not supported</i></td>
</tr>

<tr>
<th>Coproc Register Transfer</th>
<td>&nbsp;</td>
<td><i>Not supported</i></td>
</tr>

<tr>
<th>Software interrupt</th>
<td>&nbsp;</td>
<td><i>Traps</i></td>
</tr>

</table>
</p>

<p>
In addition, there are some instructions that the GIP implements in addition to the standard 32-bit
ARM instruction set, to support the microkernel and software emulation features required for the OS.

<table>

<tr>
<th>Instruction</th>
<th>Description</th>
</tr>

<tr>
<th></th>
<td></td>
</tr>

<tr>
<th></th>
<td></td>
</tr>

<tr>
<th></th>
<td></td>
</tr>

<tr>
<th></th>
<td></td>
</tr>

<tr>
<th></th>
<td></td>
</tr>

<tr>
<th></th>
<td></td>
</tr>

</table>

</p>

<?php
page_ep();

include "${toplevel}web_assist/web_footer.php"; ?>

