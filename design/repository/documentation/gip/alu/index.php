<?php

include "web_locals.php";
include "${toplevel}web_assist/web_globals.php";

site_set_location("gip.alu");

$page_title = "GIP Documentation";

include "${toplevel}web_assist/web_header.php";

page_header( "GIP ALU and Shifter" );

page_sp();
?>

<?php page_section( "overview", "Overview" ); ?>

<p>
The GIP ALU and shifter are responsible for implementing all of the
data operations of the GIP. 
</p>

<p>The unit takes an operation and two 32-bit registered data items 
as its inputs, and it maintains two 32-bit internal stores; one is an accumulator, and 
the other is a shifter result. Also five flags are maintained;
a carry flag, a zero flag, a negative flag, an overflow flag, and a shifter
flag.

</p>

<?php page_section( "conditional", "Conditional execution" ); ?>
<p>

The ALU generates a 'condition passed' indication from either current flags or from the stored flags. 
An ALU and data shifter instruction can store results and flags conditionally on this value, if desired.

<br>
The flags may be configured as 'sticky'; that is they may be set if desired due 
to an operation, but not cleared

</p>

<?php page_section( "operands", "Operands to registers" ); ?>

<p>The ALU contains four potential operand sources: input register A, input register B, ALU accumulator ACC, and shifter result SHF.
</p>

<?php page_section( "further_details", "Further details" ); ?>

<p>The <a href="operations.php">operations</a> document describes the operational capabilities required of the ALU and shifter, and discusses
how these requirements are derived from the ARM emulation. The <a href="dataflow.php">dataflow</a> document takes this information in another form, presenting
details as to how the data flows for each operation and what the shifter and ALU do, and how that ties back to ARM emulation. The <a href="implementation.php">implementation</a> document then gives details on the module and its implementation.
</p>

<?php
page_ep();

include "${toplevel}web_assist/web_footer.php"; ?>

