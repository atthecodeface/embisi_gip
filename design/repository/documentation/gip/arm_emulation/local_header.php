<?php

$repl_rn = "Rn may be replaced with ACC if tracking indicates accumulator contains Rn<br>Rn may be replaced with PC if it indicates R15; the value the ARM emulator decoder passes to the pipeline as PC is the address of the instruction plus 8";
$repl_rn_rm = "Rn and/or Rm may be replaced with ACC if tracking indicates accumulator contains Rn/Rm<br>Rn/Rm may be replaced with PC if it indicates R15; the value the ARM emulator decoder passes to the pipeline as PC is the address of the instruction plus 8";
$repl_rs = "Rs may be replaced with ACC if tracking indicates accumulator contains Rs<br>Rs may be replaced with PC if it indicates R15; the value the ARM emulator decoder passes to the pipeline as PC is the address of the instruction plus 8";
$repl_rn_rs = "Rn and/or Rs may be replaced with ACC if tracking indicates accumulator contains Rn/Rs<br>Rn/Rs may be replaced with PC if it indicates R15; the value the ARM emulator decoder passes to the pipeline as PC is the address of the instruction plus 8";
$repl_shf = "LSL may be replaced with LSR, ASR, ROR (or RRX/ROR33)";
$rd_not_pc = "Rd not PC";
$rd_rn_not_pc = "Rd not PC, Rn not PC (illegal in ARM)";
$rn_not_pc = "Rn not PC (illegal in ARM)";
$arith_logic_ops = "ADD may be replaced with ADC, SUB, SBC, RSB, RSC, AND, EOR, ORR, BIC";
$move_ops = "MOV may be replaced with MVN";
$arith_ops = "ADD may be replaced with ADC, SUB, SBC, RSB, RSC";
$logic_ops = "BIC may be replaced with AND, ORR, EOR";
$compare_ops = "CMP/ISUB may be replaced with CMN/IADD";
$test_ops = "TST/IAND may be replaced with TEQ/IXOR";
$conds = "EQ may be replaced with NE, MI, PL, CC, CS, VC, VS, GT, GE, LT, LE, HI, LS";
$effects_nothing = "Does not effect flags, accumulator marked as invalid";
$effects_acc = "Effects no flags, sets accumulator";
$effects_acc_zcvn = "Sets accumulator, updates Z, C, V and N";
$effects_zcvn = "Updates Z, C, V and N";
$effects_acc_zn = "Sets accumulator, updates Z and N";
$effects_zn = "Updates Z and N";
$differs = "<b>Differs from ARM architecture</b>";

function arm_emulation_table_start( )
{
    echo "<table class=data border=1><tr><th>ARM instruction</th><th>Internal instruction</th><th>Notes</th></tr>\n";
}

function arm_emulation_table_instruction( $arm, $internal, $restrictions, $notes )
{
    echo "<tr>\n";
    echo "<th align=left valign=top>$arm</th>\n";
    echo "<td align=left valign=top><em>$internal</em></td>";
    echo "<td align=left valign=top>";
    if ($restrictions != "")
    {
        echo "<em>$restrictions</em><br>";
    }
    if ($notes != "")
    {
        echo "$notes<br>\n";
    }
    echo "</td></tr>\n";
}

function arm_emulation_table_end( )
{
    echo "</table>\n";
}

site_set_location("gip.arm_emulation");

$page_title = "GIP Documentation";

include "${toplevel}web_assist/web_header.php";

page_header( "ARM Emulation in the GIP" );

page_sp();

?>
