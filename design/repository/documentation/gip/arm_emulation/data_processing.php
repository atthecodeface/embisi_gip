<?php

include "web_locals.php";
include "${toplevel}web_assist/web_globals.php";
include "local_header.php";
?>

<?php page_section( "data_processing", "Data processing instructions" ); ?>

<p>

ARM data processing instructions can perform a shift and ALU operation in a single instruction, but the bulk
of the instructions are just ALU operations. The internal instruction set does not support a single instruction
to perform both a shift and an ALU operation, so pairs of internal instructions must be used for those ARM instructions. This
leads to two distinct classes of emulated data processing instructions:

<dl>

<dt>
Unshifted instructions

<dd>
ALU{cc} Rd, Rn, Rm
<br>
ALU{cc} Rd, Rn, #imm
<p>
These instructions do not require a shift operation, and so are emulated with a single internal instruction.
There are a few subclasses of these instructions; comparisons differ from instructions that write to registers,
and some instructions may set the flags while others do not.

<dt>
Shifted instructions

<dd>
ALU{cc} Rd, Rn, Rm, SHF #imm_shift
<br>
ALU{cc} Rd, Rn, Rm, SHF Rs
<p>
These ARM instructions require two internal instructions. The first will generate an intermediate result in the
'SHF' register and a carry out in the 'P' flag, and the second instruction will utilize these results to get
the correct results from the ARM instruction. There are similar subclasses of instruction as for the previous
class, where it should be noted that the main difference between them is that the preliminary internal shift
instruction is performed first.

</dl>

</p>

<?php page_subsection( "", "Unshifted data processing instructions" );?>

<p>

Unshifted data processing operations map to a single internal instruction. There are then a small number of subclasses
of these instructions, formed in part as a matrix derived from a couple of issues: instructions that effect flags
(CMP, CMN, TST, TEQ, and operations with explicit 'S'); instructions that execute conditionally; instructions that effect the PC.
The first set of instructions can be mapped directly to internal instructions that effect the flags, with an internal
instruction modifier of 'S'. The second set of instructions can be mapped directly also to internal instructions that execute conditionally, but
as the instruction may not be executed, the mapping <em>cannot</em> use the 'A' modifier to update the accumulator,
as this renders the accumulator value as useless.

</p>

<p>

So the mapping is relatively simple:

<ol>

<li>
Map the operation from the ARM space to the internal space:

<table>
<tr><th>ARM</th><th>Internal</th><th>ARM</th><th>Internal</th><th>ARM</th><th>Internal</th><th>ARM</th><th>Internal</th></tr>
<tr><th>ADD</th><td>IADD</td><th>ADC</th><td>IADC</td><th>SUB</th><td>ISUB</td><th>SBC</th><td>ISBC</td></tr>
<tr><th>RSB</th><td>IRSB</td><th>RSC</th><td>IRSC</td><th>AND</th><td>IAND</td><th>ORR</th><td>IORR</td></tr>
<tr><th>EOR</th><td>IEOR</td><th>BIC</th><td>IBIC</td><th>MOV</th><td>IMOV</td><th>MVN</th><td>IMVN</td></tr>
<tr><th>CMP</th><td>ISUB</td><th>CMN</th><td>IADD</td><th>TST</th><td>IAND</td><th>TEQ</th><td>IEOR</td></tr>
</table>

<li>
Map the condition code from the ARM space to the internal space (textually they are identical)

<li>
If the ARM instruction has the 'S' modifier (which CMP, CMN, TST and TEQ must) then add the 'S' modifier to the internal instruction.

<li>
For all except CMP, CMN, TST and TEQ, if the ARM instruction is not conditional (i.e. the condition is 'AL') then add the 'A' modifier to the internal instruction.

<li>
For all except CMP, CMN, TST and TEQ, check if Rd is PC; if it is, then add the 'F' modifier to the internal instruction.

<li>
For all except MOV and MVN, check if Rn should be replaced with ACC or PC: if it is R15 then replace it with 'PC', and the instruction address plus 8 will be passed in to the internal pipeline as the PC value; if instead it matches the tracking accumulator indicator then replace it with 'ACC'. If Rn is not replaced, then use it as is.

<li>
For an immediate ARM instruction copy across the immediate value.

<li>
For an ARM instruction utilizing Rm, check if Rm should be replaced with ACC or PC: if it is R15 then replace it with 'PC', and the instruction address plus 8 will be passed in to the internal pipeline as the PC value; if instead it matches the tracking accumulator indicator then replace it with 'ACC'. If Rm is not replaced, then use it as is.

</ol>

</p>

<?php

arm_emulation_table_start();
arm_emulation_table_instruction( "MOV    Rd, #imm", "IMOVA   #imm -> Rd", $rd_not_pc, "$move_ops<br>$effects_acc" );
arm_emulation_table_instruction( "ADD    Rd, Rn, #imm", "IADDA   Rn, #imm -> Rd", $rd_not_pc, "$repl_rn<br>$arith_logic_ops<br>$effects_acc" );
arm_emulation_table_instruction( "MOV    PC, #imm", "IMOVAF  #imm -> PC", "", "$move_ops<br>$effects_acc" );
arm_emulation_table_instruction( "ADD    PC, Rn, #imm", "IADDAF  Rn, #imm -> PC", "", "$repl_rn<br>$arith_logic_ops<br>$effects_nothing" );

arm_emulation_table_instruction( "MOVS   Rd, #imm", "IMOVSA   #imm -> Rd", $rd_not_pc, "$move_ops<br>$effects_acc_zn" );
arm_emulation_table_instruction( "CMP[S] Rn, #imm", "ISUBS   Rn, #imm", "", "$repl_rn<br>$compare_ops<br>$effects_zcvn" );
arm_emulation_table_instruction( "TST[S] Rn, #imm", "IANDS   Rn, #imm", "", "$repl_rn<br>$compare_ops<br>$effects_zn" );
arm_emulation_table_instruction( "ADDS   Rd, Rn, #imm", "IADDSA  Rn, #imm -> Rd", $rd_not_pc, "$repl_rn<br>$arith_ops<br>$effects_acc_zcvn" );
arm_emulation_table_instruction( "BICS   Rd, Rn, #imm", "IBICSA  Rn, #imm -> Rd", $rd_not_pc, "$repl_rn<br>$logic_ops<br>$effects_acc_zn" );
arm_emulation_table_instruction( "MOVS   PC, #imm", "IMOVSAF  #imm -> PC", "", "$move_ops<br>$effects_acc_zn<br>$differs" );
arm_emulation_table_instruction( "ADDS   PC, Rn, #imm", "IADDSAF Rn, #imm -> PC", "", "$repl_rn<br>$arith_ops<br>$effects_acc_zcvn<br>$differs" );
arm_emulation_table_instruction( "BICS   PC, Rn, #imm", "IBICSAF Rn, #imm -> PC", "", "$repl_rn<br>$logic_ops<br>$effects_acc_zn<br>$differs" );

arm_emulation_table_instruction( "ADDEQ  Rd, Rn, #imm", "IADDEQ  Rn, #imm -> Rd", $rd_not_pc, "$repl_rn<br>$arith_logic_ops<br>$effects_nothing<br>$conds" );
arm_emulation_table_instruction( "ADDEQ  PC, Rn, #imm", "IADDEQF Rn, #imm -> PC", "", "$repl_rn<br>$arith_logic_ops<br>$effects_nothing<br>$conds" );
arm_emulation_table_instruction( "ADDEQS Rd, Rn, #imm", "IADDEQS Rn, #imm -> Rd", $rd_not_pc, "$repl_rn<br>$arith_ops<br>$effects_acc_zcvn<br>$conds" );
arm_emulation_table_instruction( "ADDEQS PC, Rn, #imm", "IADDEQSF Rn, #imm -> PC", "", "$repl_rn<br>$arith_ops<br>$effects_acc_zcvn<br>$conds<br>$differs" );

arm_emulation_table_instruction( "ADDEQ  Rd, Rn, Rm", "IADDEQ  Rn, Rm -> Rd", $rd_not_pc, "$repl_rn_rm<br>$arith_logic_ops<br>$effects_nothing<br>$conds" );
arm_emulation_table_instruction( "ADDEQ  PC, Rn, Rm", "IADDEQF Rn, Rm -> PC", "", "$repl_rn_rm<br>$arith_logic_ops<br>$effects_nothing<br>$conds" );
arm_emulation_table_instruction( "ADDEQS Rd, Rn, Rm", "IADDEQS Rn, Rm -> Rd", $rd_not_pc, "$repl_rn_rm<br>$arith_ops<br>$effects_acc_zcvn<br>$conds" );
arm_emulation_table_instruction( "ADDEQS PC, Rn, Rm", "IADDEQSF Rn, Rm -> PC", "", "$repl_rn_rm<br>$arith_ops<br>$effects_acc_zcvn<br>$conds<br>$differs" );
arm_emulation_table_end();

?>

<?php page_subsection( "shifted", "Shifted data processing instructions" );?>

<p>

Shifted data processing operations map to two internal instructions: a shift operation whose results
are placed in 'SHF' and 'P', and a following ALU operation utilizing that shift operation. The
second ALU operation is very similar to an unshifted data processing ARM instruction mapping, except 'Rm' is
replaced with 'SHF', and 'P' is appended to any instruction that includes the 'S' modifier.

</p>

<p>
ARM instructions with a shift are of the form: ADD Rd, Rn, Rm, LSL Rs or ADD Rd, Rn, Rm, LSL #5. The first
internal shift operation instruction implements the 'Rm LSL Rs' or 'Rm LSL #5' portion of the instructions.
</p>

<p>

The shift is handled using the following simple mapping:

<ol>

<li>
Map the shift operation from the ARM space to the internal space:

<table>
<tr><th>ARM</th><th>Internal</th></tr>
<tr><th>LSL</th><td>ILSL</td></tr>
<tr><th>LSR</th><td>ILSR</td></tr>
<tr><th>ASR</th><td>IASR</td></tr>
<tr><th>ROR</th><td>IROR</td></tr>
<tr><th>RRX</th><td>IROR33</td></tr>
</table>

<li>
Check if Rm should be replaced with ACC or PC: if it is R15 then replace it with 'PC', and the instruction address plus 8 will be passed in to the internal pipeline as the PC value; if instead it matches the tracking accumulator indicator then replace it with 'ACC'. If Rm is not replaced, then use it as is. This will be the first operand of the first internal instruction.

<li>
If the shift is by register Rs, then check if it should be replaced with ACC or PC: if it is R15 then replace it with 'PC', and the instruction address plus 8 will be passed in to the internal pipeline as the PC value; if instead it matches the tracking accumulator indicator then replace it with 'ACC'. If Rs is not replaced, then use it as is. This will be the second operand of the first internal instruction.

<li>
If the shift is immediate then use that as the second operand of the first internal instruction.

</ol>

<?php
arm_emulation_table_start();

arm_emulation_table_instruction( "MOV    Rd, Rm, LSL #imm", "ILSL Rm, #imm<br>IMOVA   SHF -> Rd", $rd_not_pc, "$repl_shf<br>$move_ops<br>$effects_acc" );
arm_emulation_table_instruction( "ADD    Rd, Rn, Rm, LSL Rs", "ILSL Rm, Rs<br>IADDA   Rn, SHF -> Rd", $rd_not_pc, "$repl_shf<br>$repl_rn_rs<br>$arith_logic_ops<br>$effects_acc" );
arm_emulation_table_instruction( "MOV    PC, Rm, LSL Rs", "ILSL Rm, Rs<br>IMOVAF  SHF -> PC", "", "$repl_shf<br>$repl_rs<br>$move_ops<br>$effects_acc" );
arm_emulation_table_instruction( "ADD    PC, Rn, Rm, LSL #imm", "ILSL Rm, #imm<br>IADDAF  Rn, SHF -> PC", "", "$repl_shf<br>$repl_rn<br>$arith_logic_ops<br>$effects_nothing" );

arm_emulation_table_instruction( "MOVS   Rd, Rm, LSL #imm", "ILSL Rm, #imm<br>IMOVSPA   SHF -> Rd", $rd_not_pc, "$repl_shf<br>$move_ops<br>$effects_acc_zn" );
arm_emulation_table_instruction( "CMP[S] Rn, Rm, LSL #imm", "ILSL Rm, #imm<br>ISUBSP   Rn, SHF", "", "$repl_shf<br>$repl_rn<br>$compare_ops<br>$effects_zcvn" );
arm_emulation_table_instruction( "TST[S] Rn, Rm, LSL #imm", "ILSL Rm, #imm<br>IANDSP   Rn, SHF", "", "$repl_shf<br>$repl_rn<br>$compare_ops<br>$effects_zn" );
arm_emulation_table_instruction( "ADDEQS   Rd, Rn, Rm, LSL #imm", "ILSL Rm, #imm<br>IADDEQSP  Rn, SHF -> Rd", $rd_not_pc, "$repl_shf<br>$repl_rn<br>$arith_ops<br>$effects_acc_zcvn<br>$conds" );
arm_emulation_table_instruction( "BICS   Rd, Rn, Rm, LSL #imm", "ILSL Rm, #imm<br>IBICSPA  Rn, SHF -> Rd", $rd_not_pc, "$repl_shf<br>$repl_rn<br>$logic_ops<br>$effects_acc_zn" );

arm_emulation_table_end();
?>

</p>

<?php
page_ep();

include "${toplevel}web_assist/web_footer.php"; ?>

