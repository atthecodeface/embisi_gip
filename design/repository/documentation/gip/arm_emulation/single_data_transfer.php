<?php

include "web_locals.php";
include "${toplevel}web_assist/web_globals.php";
include "local_header.php";
?>

<?php page_section( "single_data_transfer", "Single data transfer" ); ?>

<p>

ARM single data transfer instructions support preindexed and postindexed addressing, with optional writeback for preindexed, with
an indexing operation of add or subtract an immediate value, a register, or a register shifted by an immediate value. Each of these is examined
in more detail in a separate section, with loads separated from stores.

</p>

<p>

Notes:

<ol>

<li>
Writeback is not permitted with PC as the base address; this is an ARM restriction.

<li>
A read of the PC with a postindexed read should work; we will have to ensure, though, that the completion of the read cannot occur in a single cycle where the pipeline flushes after the PC is read with the writeback internal instruction

<li>
The accumulator tracker is marked as invalid at the end of every load or store instruction

</ol>

</p>

<?php

page_section( "loads", "Loads" );

arm_emulation_table_start();
arm_emulation_table_instruction( "LDR[CC] Rd, [Rn, #+/-imm]", "ILDR[CC]A #0 (Rn, #+/-imm) -> Rd", $rd_not_pc, $repl_rn );
arm_emulation_table_instruction( "LDR[CC] PC, [Rn, #+/-imm]", "ILDR[CC]AF #0 (Rn, #+/-imm) -> PC", "", $repl_rn );
arm_emulation_table_instruction( "LDR[CC] Rd, [Rn, +/-Rm]", "ILDR[CC]A #0 (Rn, +/-Rm) -> Rd", $rd_not_pc, $repl_rn_rm );
arm_emulation_table_instruction( "LDR[CC] PC, [Rn, +/-Rm]", "ILDR[CC]AF #0 (Rn, +/-Rm) -> PC", "", $repl_rn_rm );
arm_emulation_table_instruction( "LDR[CC] Rd, [Rn, +/-Rm, SHF #imm]", "ILSL Rm, #imm<br>ILDR[CC]A #0 (Rn, +/-SHF) -> Rd", $rd_not_pc, $repl_rn_rm );
arm_emulation_table_instruction( "LDR[CC] PC, [Rn, +/-Rm, SHF #imm]", "ILSL Rm, #imm<br>ILDR[CC]AF #0 (Rn, +/-SHF) -> PC", "", $repl_rn_rm );
arm_emulation_table_instruction( "LDR[CC] Rd, [Rn, #+/-imm]!", "IADD[CC]A/ISUB[CC]A Rn, #imm -> Rn<br>ILDRCP #0 (ACC) -> Rd", $rd_rn_not_pc, "$repl_rn<br>" );
arm_emulation_table_instruction( "LDR[CC] PC, [Rn, #+/-imm]!", "IADD[CC]A/ISUB[CC]A Rn, #imm -> Rn<br>ILDRCPF #0 (ACC) -> PC", $rn_not_pc, "$repl_rn<br>" );
arm_emulation_table_instruction( "LDR[CC] Rd, [Rn, +/-Rm]!", "IADD[CC]A/ISUB[CC]A Rn, Rm -> Rn<br>ILDRCP #0 (ACC) -> Rd", $rd_rn_not_pc, "$repl_rn_rm<br>" );
arm_emulation_table_instruction( "LDR[CC] PC, [Rn, +/-Rm]!", "IADD[CC]A/ISUB[CC]A Rn, Rm -> Rn<br>ILDRCPF #0 (ACC) -> PC", $rn_not_pc, "$repl_rn_rm<br>" );
arm_emulation_table_instruction( "LDR[CC] Rd, [Rn, +/-Rm, SHF #imm]!", "ILSL Rm, #imm<br>IADDA/ISUBA Rn, SHF -> Rn<br>ILDR[CC] #0 (ACC) -> Rd", $rd_rn_not_pc, "$repl_rn_rm<br>" );
arm_emulation_table_instruction( "LDR[CC] PC, [Rn, +/-Rm, SHF #imm]!", "ILSL Rm, #imm<br>IADDA/ISUBA Rn, SHF -> Rn<br>ILDR[CC]F #0 (ACC) -> PC", $rn_not_pc, "$repl_rn_rm<br>" );
arm_emulation_table_instruction( "LDR[CC] Rd, [Rn], #+/-imm", "ILDR[CC]A #0 (Rn), #+/-imm -> Rd<br>MOVCP ACC -> Rn", $rd_rn_not_pc, "$repl_rn<br>" );
arm_emulation_table_instruction( "LDR[CC] PC, [Rn], #+/-imm", "ILDR[CC]A #0 (Rn), #+/-imm -> PC<br>MOVCPF ACC -> Rn", $rn_not_pc, "$repl_rn<br>" );
arm_emulation_table_instruction( "LDR[CC] Rd, [Rn], +/-Rm", "ILDR[CC]A #0 (Rn), +/-Rm -> Rd<br>MOVCP ACC -> Rn", $rd_rn_not_pc, "$repl_rn_rm<br>" );
arm_emulation_table_instruction( "LDR[CC] PC, [Rn], +/-Rm", "ILDR[CC]A #0 (Rn), +/-Rm -> PC<br>MOVCPF ACC -> Rn", $rn_not_pc, "$repl_rn_rm<br>" );
arm_emulation_table_instruction( "LDR[CC] Rd, [Rn], +/-Rm, SHF #imm", "ILSL Rm, #imm<br>ILDR[CC]A #0 (Rn), +/-SHF -> Rd<br>MOVCP ACC -> Rn", $rd_rn_not_pc, "$repl_rn_rm<br>" );
arm_emulation_table_instruction( "LDR[CC] PC, [Rn], +/-Rm, SHF #imm", "ILSL Rm, #imm<br>ILDR[CC]A #0 (Rn), +/-SHF -> PC<br>MOVCPF ACC -> Rn", $rn_not_pc, "$repl_rn_rm<br>" );
arm_emulation_table_end();

?>

<?php

page_section( "stores", "Stores" );

arm_emulation_table_start();
arm_emulation_table_instruction( "STR[CC] Rd, [Rn, #0]", "ISTR[CC]A[S] #0 (Rn, #0) <- Rd", "", $repl_rn );
arm_emulation_table_instruction( "STR[CC] Rd, [Rn, #+/-imm]", "IADDA/ISUBA Rn, #imm<br>ISTR[CC]A[S] #0 (ACC, #0) <- Rd", "", $repl_rn );
arm_emulation_table_instruction( "STR[CC] Rd, [Rn, +/-Rm]", "IADDA/ISUBA Rn, Rm<br>ISTR[CC]A[S] #0 (ACC, #0) <- Rd", "", $repl_rn_rm );
arm_emulation_table_instruction( "STR[CC] Rd, [Rn, +/-Rm, SHF #imm]", "ILSL Rm, #imm<br>ISTR[CC]A[S] #0 (Rn, +/-SHF) <- Rd", "", $repl_rn_rm );
arm_emulation_table_instruction( "STR[CC] Rd, [Rn, #0]!", "ISTR[CC]A[S] #0 (Rn, #0) <- Rd -> Rn", "", $repl_rn );
arm_emulation_table_instruction( "STR[CC] Rd, [Rn, #+/-imm]!", "IADDA/ISUBA Rn, #imm<br>ISTR[CC]A[S] #0 (ACC, #0) <- Rd -> Rn", "", $repl_rn );
arm_emulation_table_instruction( "STR[CC] Rd, [Rn, +/-Rm]!", "IADDA/ISUBA Rn, Rm<br>ISTR[CC]A[S] #0 (ACC, #0) <- Rd -> Rn", "", $repl_rn_rm );
arm_emulation_table_instruction( "STR[CC] Rd, [Rn, +/-Rm, SHF #imm]!", "ILSL Rm, #imm<br>ISTR[CC]A[S] #0 (Rn, +/-SHF) <- Rd -> Rn", "", $repl_rn_rm );
arm_emulation_table_instruction( "STR[CC] Rd, [Rn], #+/-imm", "ISTR[CC]A[S] #0 (Rn, #0) <- Rd<br>IADDCPA/ISUBCPA Rn, #imm -> Rn", "", $repl_rn );
arm_emulation_table_instruction( "STR[CC] Rd, [Rn], +/-Rm",  "ISTR[CC]A[S] #0 (Rn, #0) <- Rd<br>IADDCPA/ISUBCPA Rn, Rm -> Rn", "", $repl_rn_rm );
arm_emulation_table_instruction( "STR[CC] Rd, [Rn], +/-Rm, SHF #imm",  "ISTR[CC]A[S] #0 (Rn, #0) <- Rd<br>ILSLCP Rm, #imm<br>IADDCPA/ISUBCPA Rn, SHF -> Rn", "", $repl_rn_rm );
arm_emulation_table_end();

?>

<?php
page_ep();

include "${toplevel}web_assist/web_footer.php"; ?>

