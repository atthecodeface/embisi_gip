<?php

include "web_locals.php";
include "${toplevel}web_assist/web_globals.php";
include "local_header.php";
?>

<?php page_section( "block_data_transfer", "Block data transfer" ); ?>

<p>

ARM block data transfer instructions are multiple word transfers to or
from a register set, identified by a bit mask. The address for the
transfers is specified by a register, and the transfers will occur at
or next to the address and above or below, depending on options
specified in the instruction

</p>

<p>

Notes:

<ol>

<li> As with the ARM, all data transfers occur as a burst from the
lowest address of the transfer upwards

<li> The PC in an ARM STM instruction stores PC+12; the GIP stores PC+8

<li> A conditional store/load could be optimised with an opposite condition ISUB{!CC}F PC, #4 -> PC; this optimization could be conditional on the number of registers to be loaded or stored.

<li> An STM with writeback that stores the address register will always write the updated index (GIP), but only do so on ARM if it is not the first register to be written.

<li> An LDM with writeback that includes the address register on its data register list will differ on the GIP from the ARM if the addressing is 'IA' or 'IB', as the writeback will occur at the end of the operation, not early in the operation as it does with ARM.

</ol>

</p>

<?php

page_section( "loads", "Loads" );

arm_emulation_table_start();
arm_emulation_table_instruction( "LDM[CC]IA Rn, {Ri0, Ri1, ... Rik}", "ILDR[CC]A[S] #k (Rn), #+4 -> Ri0<br>ILDRCPA[S] #k-1 (Acc), #+4 -> Ri1<br>...<br>ILDRCPA[A][F] #0 (Acc), #+4 -> Rik", $rn_not_pc, $repl_rn );
arm_emulation_table_instruction( "LDM[CC]IA Rn!, {Ri0, Ri1, ... Rik}", "ILDR[CC]A[S] #k (Rn), #+4 -> Ri0<br>ILDRCPA[S] #k-1 (Acc), #+4 -> Ri1<br>...<br>ILDRCPA[S] #0 (Acc), #+4 -> Rik<br>IMOVCP[F] Acc -> Rn", $rn_not_pc, $repl_rn );
arm_emulation_table_instruction( "LDM[CC]IB Rn, {Ri0, Ri1, ... Rik}", "ILDR[CC]A[S] #k (Rn, #+4) -> Ri0<br>ILDRCPA[S] #k-1 (Acc, #+4) -> Ri1<br>...<br>ILDRCPA[S][F] #0 (Acc, #+4) -> Rik", $rn_not_pc, $repl_rn );
arm_emulation_table_instruction( "LDM[CC]IB Rn!, {Ri0, Ri1, ... Rik}", "ILDR[CC]A[S] #k (Rn, #+4) -> Ri0<br>ILDRCPA[S] #k-1 (Acc, #+4) -> Ri1<br>...<br>ILDRCPA[S] #0 (Acc, #+4) -> Rik<br>IMOVCP[F] Acc -> Rn", $rn_not_pc, $repl_rn );
arm_emulation_table_instruction( "LDM[CC]DB Rn, {Ri0, Ri1, ... Rik}", "ISUB[CC]A Rn, #k*4<br>ILDRCPA[S] #k (Acc), #+4 -> Ri0<br>ILDRCPA[S] #k-1 (Acc), #+4 -> Ri1<br>...<br>ILDRCPA[S][F] #0 (Acc), #+4 -> Rik", $rn_not_pc, $repl_rn );
arm_emulation_table_instruction( "LDM[CC]DB Rn!, {Ri0, Ri1, ... Rik}", "ISUB[CC]A Rn, #k*4 -> Rn<br>ILDRCPA[S] #k (Acc), #+4 -> Ri0<br>ILDRCPA[S] #k-1 (Acc), #+4 -> Ri1<br>...<br>ILDRCPA[S][F] #0 (Acc), #+4 -> Rik", $rn_not_pc, $repl_rn );
arm_emulation_table_instruction( "LDM[CC]DA Rn, {Ri0, Ri1, ... Rik}", "ISUB[CC]A Rn, #k*4<br>ILDRCPA[S] #k (Acc, #+4) -> Ri0<br>ILDRCPA[S] #k-1 (Acc, #+4) -> Ri1<br>...<br>ILDRCPA[S][F] #0 (Acc, #+4) -> Rik", $rn_not_pc, $repl_rn );
arm_emulation_table_instruction( "LDM[CC]DA Rn!, {Ri0, Ri1, ... Rik}", "ISUB[CC]A Rn, #k*4 -> Rn<br>ILDRCPA[S] #k (Acc, #+4) -> Ri0<br>ILDRCPA[S] #k-1 (Acc, #+4) -> Ri1<br>...<br>ILDRCPA[S][F] #0 (Acc, #+4) -> Rik", $rn_not_pc, $repl_rn );
arm_emulation_table_end();

?>

<?php

page_section( "stores", "Stores" );

arm_emulation_table_start();
arm_emulation_table_instruction( "STM[CC]IA Rn, {Ri0, Ri1, ... Rik}", "ISTR[CC]A[S] #k (Rn), #+4 <- Ri0<br>ISTRCPA[S] #k-1 (Acc), #+4 <- Ri1<br>...<br>ISTRCPA[S] #0 (Acc), #+4 <- Rik", $rn_not_pc, $repl_rn );
arm_emulation_table_instruction( "STM[CC]IA Rn!, {Ri0, Ri1, ... Rik}", "ISTR[CC]A[S] #k (Rn), #+4 <- Ri0<br>ISTRCPA[S] #k-1 (Acc), #+4 <- Ri1<br>...<br>ISTRCPA[S] #0 (Acc), #+4 <- Rik -> Rn", $rn_not_pc, $repl_rn );
arm_emulation_table_instruction( "STM[CC]IB Rn, {Ri0, Ri1, ... Rik}", "ISTR[CC]A[S] #k (Rn, #+4) <- Ri0<br>ISTRCPA[S] #k-1 (Acc, #+4) <- Ri1<br>...<br>ISTRCPA[S] #0 (Acc, #+4) <- Rik", $rn_not_pc, $repl_rn );
arm_emulation_table_instruction( "STM[CC]IB Rn!, {Ri0, Ri1, ... Rik}", "ISTR[CC]A[S] #k (Rn, #+4) <- Ri0<br>ISTRCPA[S] #k-1 (Acc, #+4) <- Ri1<br>...<br>ISTRCPA[S] #0 (Acc, #+4) <- Rik -> Rn", $rn_not_pc, $repl_rn );

arm_emulation_table_instruction( "STM[CC]DB Rn, {Ri0, Ri1, ... Rik}", "ISUB[CC]A Rn, #k*4<br>ISTRCPA[S] #k (Acc), #+4 <- Ri0<br>ISTRCPA[S] #k-1 (Acc), #+4 <- Ri1<br>...<br>ISTRCPA[S] #0 (Acc), #+4 <- Rik", $rn_not_pc, $repl_rn );
arm_emulation_table_instruction( "STM[CC]DB Rn!, {Ri0, Ri1, ... Rik}", "ISUB[CC]A Rn, #k*4 -> Rn<br>ISTRCPA[S] #k (Acc), #+4 <- Ri0<br>ISTRCPA[S] #k-1 (Acc), #+4 <- Ri1<br>...<br>ISTRCPA[S] #0 (Acc), #+4 <- Rik", $rn_not_pc, $repl_rn );
arm_emulation_table_instruction( "STM[CC]DA Rn, {Ri0, Ri1, ... Rik}", "ISUB[CC]A Rn, #k*4<br>ISTRCPA[S] #k (Acc, #+4) <- Ri0<br>ISTRCPA[S] #k-1 (Acc, #+4) <- Ri1<br>...<br>ISTRCPA[S] #0 (Acc, #+4) <- Rik", $rn_not_pc, $repl_rn );
arm_emulation_table_instruction( "STM[CC]DA Rn!, {Ri0, Ri1, ... Rik}", "ISUB[CC]A Rn, #k*4 -> Rn<br>ISTRCPA[S] #k (Acc, #+4) <- Ri0<br>ISTRCPA[S] #k-1 (Acc, #+4) <- Ri1<br>...<br>ISTRCPA[S] #0 (Acc, #+4) <- Rik", $rn_not_pc, $repl_rn );
arm_emulation_table_end();

?>

<?php
page_ep();

include "${toplevel}web_assist/web_footer.php"; ?>

