<?php

include "web_locals.php";
include "${toplevel}web_assist/web_globals.php";

site_set_location("gip.instruction_set");

$page_title = "GIP Documentation";

include "${toplevel}web_assist/web_header.php";

page_header( "GIP Instruction Set" );

page_sp();
?>

<?php page_section( "overview", "Overview" ); ?>

<table class=data>
<tr>
<th>Field
<th>Size
<th>Description
</tr>

<tr>
<th>Op
<td>[5]
<td>Operation, see table below
</tr>

<tr>
<th>Opts
<td>[3?]
<td>Coded with operation; options for the operation
</tr>

<tr>
<th>CC
<td>[5]
<td>Condition under which to execute the instruction; 00=>always, 01=>last condition test, >02=> see table...
</tr>

<tr>
<th>Rn
<td>[6]
<td>First register for use in the operation; if the top bit is a zero, then the bottom 5 bits code a register source. If not then the bottom 2 bits code an ALU source (ACC or SHF)
</tr>

<tr>
<th>Rm/Imm
<td>[33]
<td>Immediate or second register field; if the top bit is a 1, then the remaining 32 bits are an immediate value. If not, then the bottom 6 bits code a source in the same way as Rn.
</tr>

<tr>
<th>Rfw
<td>[6]
<td>Six bit encoding of register destination for the operation; if the top bit is set then the result is to a register encoded in the bottom 5 bits; if the top bit is clear then the bottom 2 bits encode none (00) or a special target such as the PC.
</tr>

<tr>
<th>S
<td>[1]
<td>Set if the flags should be set by the operation
</tr>

<tr>
<th>P
<td>p[1]
<td>Set if the shifter P flag should be written to the carry flag
</tr>

<tr>
<th>A
<td>[1]
<td>Set if the accumulator should be written with the ALU result
</tr>

<tr>
<th>F
<td>[1]
<td>Flush: set if the instruction should flush the pipeline behind it if it is executed and indicate this to the decoder.
</tr>

</table>

<table class=data>
<tr>
<th>Mnemonic
<th>Op
<th>Opts
<th>CC
<th>Rn
<th>Rm/Imm
<th>Rfw
<th>S
<th>P
<th>A
<th>F
</tr>

<tr>
<th>
IADD[CC][S][A] Rn, Rm/Imm -> Rfw
</tr>

<tr>
<th>
IADC[CC][S][A] Rn, Rm/Imm -> Rfw
</tr>

<tr>
<th>
ISUB[CC][S][A] Rn, Rm/Imm -> Rfw
</tr>

<tr>
<th>
ISBC[CC][S][A] Rn, Rm/Imm -> Rfw
</tr>

<tr>
<th>
IRSB[CC][S][A] Rn, Rm/Imm -> Rfw
</tr>

<tr>
<th>
IRSC[CC][S][A] Rn, Rm/Imm -> Rfw
</tr>

<tr>
<th>
IADD[S][A] Rn, Rm/Imm -> {cond}
</tr>

<tr>
<th>
ISUB[S][A] Rn, Rm/Imm -> {cond}
</tr>

<tr>
<th>
IRSB[S][A] Rn, Rm/Imm -> {cond}
</tr>

<tr>
<th>
IAND[CC][S][P][A] Rn, Rm/Imm -> Rfw
</tr>

<tr>
<th>
IORR[CC][S][P][A] Rn, Rm/Imm -> Rfw
</tr>

<tr>
<th>
IXOR[CC][S][P][A] Rn, Rm/Imm -> Rfw
</tr>

<tr>
<th>
IBIC[CC][S][P][A] Rn, Rm/Imm -> Rfw
</tr>

<tr>
<th>
IORN[CC][S][P][A] Rn, Rm/Imm -> Rfw
</tr>

<tr>
<th>
IMOV[CC][S][P][A] Rm/Imm -> Rfw
</tr>

<tr>
<th>
IMVN[CC][S][P][A] Rm/Imm -> Rfw
</tr>

<tr>
<th>
IAND[S][P][A] Rn, Rm/Imm -> {cond}
</tr>

<tr>
<th>
IORR[S][P][A] Rn, Rm/Imm -> {cond}
</tr>

<tr>
<th>
IXOR[S][P][A] Rn, Rm/Imm -> {cond}
</tr>

<tr>
<th>
IBIC[S][P][A] Rn, Rm/Imm -> {cond}
</tr>

<tr>
<th>
IORN[S][P][A] Rn, Rm/Imm -> {cond}
</tr>

<tr>
<th>
ILSL[CC][S][P][A] Rn, Rm/Imm -> Rfw
</tr>

<tr>
<th>
ILSR[CC][S][P][A] Rn, Rm/Imm -> Rfw
</tr>

<tr>
<th>
IASR[CC][S][P][A] Rn, Rm/Imm -> Rfw
</tr>

<tr>
<th>
IROR[CC][S][P][A] Rn, Rm/Imm -> Rfw
</tr>

<tr>
<th>
IROR33[CC][S][P][A] Rn -> Rfw
</tr>

<tr>
<th>
ICPRD[CC] Rm/Imm -> Rfw
</tr>

<tr>
<th>
ICPWR[CC] Rn, Rm/Imm
</tr>

<tr>
<th>
ICPCMD[CC] Rm/Imm
</tr>

<tr>
<th>
ILDR[CC][A] (Rn, [-]Rm/Imm) -> Rfw
</tr>

<tr>
<th>
ILDR[CC][A] (Rn), [-]Rm/Imm -> Rfw (?)
</tr>

<tr>
<th>
ILDR[CC]B[A][L][U] (Rn, [-]Rm/Imm) -> Rfw
</tr>

<tr>
<th>
ILDR[CC]B[A][L][U] (Rn), [-]Rm/Imm -> Rfw (?)
</tr>

<tr>
<th>
ILDR[CC]H[A][L][U] (Rn, [-]Rm/Imm) -> Rfw
</tr>

<tr>
<th>
ILDR[CC]H[A][L][U] (Rn), [-]Rm/Imm -> Rfw (?)
</tr>

<tr>
<th>
ISTR[CC][A] (Rn) <- Rm/Imm (Rn->Rfw)
</tr>

<tr>
<th>
ISTR[CC]B[A][L][U] (Rn) <- Rm/Imm (Rn->Rfw)
</tr>

<tr>
<th>
ISTR[CC]H[A][L][U] (Rn) <- Rm/Imm (Rn->Rfw)
</tr>

</table>

Still missing:
XMC (word, block), XCM (word, block), XORFIRSTONE, XORLASTONE, INIT, MULFIRST, MULSTEP, DIVSTEP, BITCOUNT, MEMCMD (prefetch, writeback, writeback_and_invalidate, flush, fill buffer from address, write buffer to address,
Sticky flags


<?php
page_ep();

include "${toplevel}web_assist/web_footer.php"; ?>

