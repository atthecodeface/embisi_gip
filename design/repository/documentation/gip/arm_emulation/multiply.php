<?php

include "web_locals.php";
include "${toplevel}web_assist/web_globals.php";
include "local_header.php";
?>

<?php page_section( "multiply", "Multiply instructions" ); ?>

<p>

ARM multiply instructions are basically plain multiplies or multiply
with accumulate. They are emulated with three different instructions:
INIT, IMLA, IMLB. For simplicity a multiply instruction always takes
16 cycles; early termination is not supported.

INIT[CC]A Rm, Rn/0
IMLACPA Acc, Rs
IMLBCPA Acc, #0 14 times
IMLBCPA[S] Acc, #0 -> Rd

</p>

<?php

arm_emulation_table_start();
arm_emulation_table_instruction( "MUL[CC] Rd, Rm, Rs", "
    INIT[CC]A Rm, #0<br>
    IMLACPA Acc, Rs<br>
    IMLBCPA Acc, #0 <i>(14 times)</i><br>
    IMLBCPA[F] Acc, #0 -> Rd", "No restrictions", "" );
arm_emulation_table_instruction( "MLA[CC] Rd, Rm, Rs, Rn", "
    INIT[CC]A Rm, Rn<br>
    IMLACPA Acc, Rs<br>
    IMLBCPA Acc, #0 <i>(14 times)</i><br>
    IMLBCPA[F] Acc, #0 -> Rd", "No restrictions", "" );
arm_emulation_table_instruction( "MUL[CC]S Rd, Rm, Rs", "
    INIT[CC]A Rm, #0<br>
    IMLACPA Acc, Rs<br>
    IMLBCPA Acc, #0 <i>(14 times)</i><br>
    IMLBCPAS[F] Acc, #0 -> Rd", "No restrictions", "<em>Differs from ARM - V is corrupted</em>" );
arm_emulation_table_instruction( "MLA[CC]S Rd, Rm, Rs, Rn", "
    INIT[CC]A Rm, Rn<br>
    IMLACPA Acc, Rs<br>
    IMLBCPA Acc, #0 <i>(14 times)</i><br>
    IMLBCPAS[F] Acc, #0 -> Rd", "No restrictions", "<em>Differs from ARM - V is corrupted</em>" );
arm_emulation_table_end();

?>

<?php
page_ep();

include "${toplevel}web_assist/web_footer.php"; ?>

