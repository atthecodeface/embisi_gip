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

<li> The address is written back to the indexing register at the end
of a transfer of incrementing addresses, and the beginning of a
transfer for decrementing addresses. If a load or store includes the
indexing register then there may be some difference between ARM and
GIP execution of the instruction.

</ol>

</p>

<?php

page_section( "loads", "Loads" );

arm_emulation_table_start();
arm_emulation_table_instruction( "LDR[CC] Rd, [Rn, #+/-imm]", "ILDR[CC]A #0 (Rn, #+/-imm) -> Rd", $rd_not_pc, $repl_rn );
arm_emulation_table_end();

?>

<?php

page_section( "stores", "Stores" );

arm_emulation_table_start();
arm_emulation_table_instruction( "STR[CC] Rd, [Rn, #0]", "ISTR[CC]A[S] #0 (Rn, #0) <- Rd", "", $repl_rn );
arm_emulation_table_end();

?>

<?php
page_ep();

include "${toplevel}web_assist/web_footer.php"; ?>

