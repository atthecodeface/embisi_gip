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

<p>
This area describes the GIP internal instruction set, which is executed by the GIP execution pipeline on behalf of
the native 16-bit GIP instruction set decoder and the ARM 32-bit instruction emulation system.
</p>

<p>
There are a few basic classes of instruction:

<ul>

<li> ALU / shift instructions (including multiply/divide steps)

<li> Memory reads

<li> Memory writes

<li> Coprocessor reads

<li> Coprocessor writes

<li> Coprocessor to memory transfers

<li> Memory to coprocessor transfers

<li> Memory commands (prefetch, flush, and such like)

<li> Coprocessor/memory DMA commands

</ul>

Note that there are no flow control instructions; in fact, there is no specific program counter in the internal instruction set, and it does not issue any fetches, as that is the responsibility of the decoder or emulator feeding internal instructions to the pipeline.

</p>

<?php page_section( "instruction_encoding", "Instruction Encoding" ); ?>

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
</th>
<td></td>
<td>CC</td>
</tr>

<tr>
<th>
IADC[CC][S][A] Rn, Rm/Imm -> Rfw
</th>
<td></td>
<td>CC</td>
</tr>

<tr>
<th>
ISUB[CC][S][A] Rn, Rm/Imm -> Rfw
</th>
<td></td>
<td>CC</td>
</tr>

<tr>
<th>
ISBC[CC][S][A] Rn, Rm/Imm -> Rfw
</th>
<td></td>
<td>CC</td>
</tr>

<tr>
<th>
IRSB[CC][S][A] Rn, Rm/Imm -> Rfw
</th>
<td></td>
<td>CC</td>
</tr>

<tr>
<th>
IRSC[CC][S][A] Rn, Rm/Imm -> Rfw
</th>
<td></td>
<td>CC</td>
</tr>

<tr>
<th>
IADD[S][A] Rn, Rm/Imm -> {cond}
</th>
<td></td>
<td>CC</td>
</tr>

<tr>
<th>
ISUB[S][A] Rn, Rm/Imm -> {cond}
</th>
<td></td>
<td>CC</td>
</tr>

<tr>
<th>
IRSB[S][A] Rn, Rm/Imm -> {cond}
</th>
<td></td>
<td>CC</td>
</tr>

<tr>
<th>
IAND[CC][S][P][A] Rn, Rm/Imm -> Rfw
</th>
<td></td>
<td>CC</td>
</tr>

<tr>
<th>
IORR[CC][S][P][A] Rn, Rm/Imm -> Rfw
</th>
<td></td>
<td>CC</td>
</tr>

<tr>
<th>
IXOR[CC][S][P][A] Rn, Rm/Imm -> Rfw
</th>
<td></td>
<td>CC</td>
</tr>

<tr>
<th>
IBIC[CC][S][P][A] Rn, Rm/Imm -> Rfw
</th>
<td></td>
<td>CC</td>
</tr>

<tr>
<th>
IORN[CC][S][P][A] Rn, Rm/Imm -> Rfw
</th>
<td></td>
<td>CC</td>
</tr>

<tr>
<th>
IMOV[CC][S][P][A] Rm/Imm -> Rfw
</th>
<td></td>
<td>CC</td>
</tr>

<tr>
<th>
IMVN[CC][S][P][A] Rm/Imm -> Rfw
</th>
<td></td>
<td>CC</td>
</tr>

<tr>
<th>
IAND[S][P][A] Rn, Rm/Imm -> {cond}
</th>
<td></td>
<td>CC</td>
</tr>

<tr>
<th>
IORR[S][P][A] Rn, Rm/Imm -> {cond}
</th>
<td></td>
<td>CC</td>
</tr>

<tr>
<th>
IXOR[S][P][A] Rn, Rm/Imm -> {cond}
</th>
<td></td>
<td>CC</td>
</tr>

<tr>
<th>
IBIC[S][P][A] Rn, Rm/Imm -> {cond}
</th>
<td></td>
<td>CC</td>
</tr>

<tr>
<th>
IORN[S][P][A] Rn, Rm/Imm -> {cond}
</th>
<td></td>
<td>CC</td>
</tr>

<tr>
<th>
ILSL[CC][S][P][A] Rn, Rm/Imm -> Rfw
</th>
<td></td>
<td>CC</td>
</tr>

<tr>
<th>
ILSR[CC][S][P][A] Rn, Rm/Imm -> Rfw
</th>
<td></td>
<td>CC</td>
</tr>

<tr>
<th>
IASR[CC][S][P][A] Rn, Rm/Imm -> Rfw
</th>
<td></td>
<td>CC</td>
</tr>

<tr>
<th>
IROR[CC][S][P][A] Rn, Rm/Imm -> Rfw
</th>
<td></td>
<td>CC</td>
</tr>

<tr>
<th>
IROR33[CC][S][P][A] Rn -> Rfw
</th>
<td></td>
<td>CC</td>
</tr>

<tr>
<th>
ICPRD[CC] Rm/Imm -> Rfw
</th>
<td></td>
<td>CC</td>
</tr>

<tr>
<th>
ICPWR[CC] Rn, Rm/Imm
</th>
<td></td>
<td>CC</td>
</tr>

<tr>
<th>
ICPCMD[CC] Rm/Imm
</th>
<td></td>
<td>CC</td>
</tr>

<tr>
<th>
ILDR[CC][A] #k (Rn, [-]Rm/Imm) -> Rfw
</th>
<td></td>
<td>CC</td>
</tr>

<tr>
<th>
ILDR[CC][A] #k (Rn), [-]Rm/Imm -> Rfw (?)
</th>
<td></td>
<td>CC</td>
</tr>

<tr>
<th>
ILDR[CC]B[A][L][U] #k (Rn, [-]Rm/Imm) -> Rfw
</th>
<td></td>
<td>CC</td>
</tr>

<tr>
<th>
ILDR[CC]B[A][L][U] #k (Rn), [-]Rm/Imm -> Rfw
</th>
<td></td>
<td>CC</td>
</tr>

<tr>
<th>
ILDR[CC]H[A][L][U] #k (Rn, [-]Rm/Imm) -> Rfw
</th>
<td></td>
<td>CC</td>
</tr>

<tr>
<th>
ILDR[CC]H[A][L][U] #k (Rn), [-]Rm/Imm -> Rfw
</th>
<td></td>
<td>CC</td>
</tr>

<tr>
<th>
ISTR[CC][A] #k (Rn), #-4/0/+4 <- Rm/Imm [(Rn->Rfw)]
</th>
<td></td>
<td>CC</td>
</tr>

<tr>
<th>
ISTR[CC]B[A][L][U] #k (Rn), #-4/0/+4 <- Rm/Imm [(Rn->Rfw)]
</th>
<td></td>
<td>CC</td>
</tr>

<tr>
<th>
ISTR[CC]H[A][L][U] #k (Rn), #-2/0/+2 <- Rm/Imm [(Rn->Rfw)]
</th>
<td></td>
<td>CC</td>
</tr>

</table>

Still missing:
XMC (word, block), XCM (word, block), XORFIRSTONE, XORLASTONE, INIT, MULFIRST, MULSTEP, DIVSTEP, BITCOUNT, MEMCMD (prefetch, writeback, writeback_and_invalidate, flush, fill buffer from address, write buffer to address,
Sticky flags

<table class=data>
<tr>
<th>CC
<th>Meaning
<th>Flags
</tr>

<tr>
<th>AL</th>
<td>Always</td>
<td>1</td>
</tr>

<tr>
<th>EQ</th>
<td>equal</td>
<td>Z</td>
</tr>

<tr>
<th>NE</th>
<td>not equal</td>
<td>!Z</td>
</tr>

<tr>
<th>CS</th>
<td>carry set</td>
<td>C</td>
</tr>

<tr>
<th>CC</th>
<td>carry clear</td>
<td>!C</td>
</tr>

<tr>
<th>MI</th>
<td>negative</td>
<td>N</td>
</tr>

<tr>
<th>PL</th>
<td>positive</td>
<td>!N</td>
</tr>

<tr>
<th>VS</th>
<td>overflow set</td>
<td>V</td>
</tr>

<tr>
<th>VC</th>
<td>overflow clear</td>
<td>!V</td>
</tr>

<tr>
<th>LS</th>
<td>lower or same</td>
<td>!C | Z</td>
</tr>

<tr>
<th>HI</th>
<td>higher</td>
<td>C & !Z</td>
</tr>

<tr>
<th>GT</th>
<td>(C&!V) | (!C&V)</td>
<td></td>
</tr>

<tr>
<th>GE</th>
<td>(C&!V) | (!C&V) | Z</td>
<td></td>
</tr>

<tr>
<th>LT</th>
<td>(!C&V) | (C&V)</td>
<td></td>
</tr>

<tr>
<th>LE</th>
<td>(!C&V) | (C&V) | Z</td>
<td></td>
</tr>

</table>


<?php
page_ep();

include "${toplevel}web_assist/web_footer.php"; ?>

