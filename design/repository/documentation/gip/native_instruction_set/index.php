<?php

include "web_locals.php";
include "${toplevel}web_assist/web_globals.php";

site_set_location("gip.instruction_set");

$page_title = "GIP Documentation";

include "${toplevel}web_assist/web_header.php";

page_header( "GIP Native Instruction Set" );

page_sp();
?>

<?php page_section( "summary", "Summary" ); ?>

<p>

This area describes the 16-bit GIP native instruction set, which is
mapped by the 16-bit instruction decoder in to GIP internal
instructions. Some instruction encodings are modal; that is they map
to different internal instructions depending on the mode of the
thread, which may be adjusted by another 16-bit instruction. Another
item to note is the instruction set is a 2-register instruction set,
with separated memory and operation instructions, much like simple
RISC processors.

</p>

<?php page_section( "summary", "Summary" ); ?>

<p>

This documentation starts with <a href="overview.php">the overview</a>; pages then give <a href="encoding.php">details of the encodings</a>, and <a href="examples.php">some examples of use</a>.

</p>

<?php
page_ep();

include "${toplevel}web_assist/web_footer.php"; ?>

