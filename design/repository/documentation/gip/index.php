<?php

include "web_locals.php";
include "${toplevel}web_assist/web_globals.php";

site_set_location("gip");

$page_title = "GIP Documentation";

include "${toplevel}web_assist/web_header.php";

page_header( "Overview" );

page_sp();
?>

The GIP documentation currently consists of OpenOffice documents and HTML/PHP documents.

<p>

PHP/HTML documentation is written in Quanta or any text editor, and it contains the next level of design after the lab book details.

<p>

OpenOffice documentation covers the actual final design and functional details. This is collected together and prettied up from the HTML files and lab book details.

<p>

The instruction set is documented in OpenOffice

<?php page_section( "php", "PHP/HTML documentation" );?>

<table border=1>
<tr>
<th>Documentation</th>
<th>Description</th>
</tr>

<tr>
<th><a href="instruction_set/index.php">Instruction set</a></th>
<td>The instruction set and its encoding</td>
</tr>

<tr>
<th><a href="thread/index.php">Threads and scheduling</a></th>
<td>The thread model and hardware and the scheduling mechanisms</td>
</tr>

<tr>
<th><a href="rf/index.php">RF</a></th>
<td>Register file, with information on read ports, write ports, and register scoreboarding</td>
</tr>

<tr>
<th><a href="alu/index.php">ALU</a></th>
<td>ALU capabilities, outline implementation, and use for ARM emulation</td>
</tr>

<tr>
<th><a href="arm_emulation/index.php">ARM emulation</a></th>
<td>Details on the mechanisms used for ARM emulation</td>
</tr>

<tr>
<th><a href="microkernel/index.php">Microkernel</a></th>
<td>Capabilities of the microkernel and its use in ARM emulation</td>
</tr>

</table>

<?php
page_ep();

include "${toplevel}web_assist/web_footer.php"; ?>

