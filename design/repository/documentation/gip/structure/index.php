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
Register file

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

<?php
page_ep();

include "${toplevel}web_assist/web_footer.php"; ?>







