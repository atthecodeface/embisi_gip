<?php

include "web_locals.php";
include "${toplevel}web_assist/web_globals.php";

site_set_location("gip.instruction_set");

$page_title = "GIP Documentation";

include "${toplevel}web_assist/web_header.php";

page_header( "GIP Native Instruction Set" );

page_sp();
?>

<?php page_section( "encoding", "Encoding" ); ?>

<?php
page_ep();

include "${toplevel}web_assist/web_footer.php"; ?>

