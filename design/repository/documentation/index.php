<?php

include "web_locals.php";
include "${toplevel}web_assist/web_globals.php";

site_set_location("top");

$page_title = "Embisi Hardware Documentation";

include "${toplevel}web_assist/web_header.php";


page_header( "Overview" );

page_sp();
?>

<a href="gip/index.php">GIP</a>

<p>

<a href="postbus/index.php">Post bus</a>

<?php
page_ep();

include "${toplevel}web_assist/web_footer.php"; ?>

