<?php

include "web_locals.php";
include "${toplevel}web_assist/web_globals.php";

site_set_location("gip.alu.dataflow");

$page_title = "GIP Documentation";

include "${toplevel}web_assist/web_header.php";

page_header( "GIP ALU and Shifter Dataflow" );

page_sp();
?>

<p>
The data flow for the ALU is from one of the registers (SHF, ACC, A input or B input) through the shifter, adder and lofical units,
through to the outputs (ALU result, passthrough result) 
and the next ACC and SHF values (and also A and B when doing multiplies and divides). This section summarizes those flows from an
analysis of the previous sections to show what sources are required for what, and when.
</p>

<?php page_section( "Op1", "Op1" ); ?>

<table border=1>
<tr>
<th>ARM source</th>
<th>Actual source</th>
<th>Relevant ARM instructions</th>
</tr>

<tr>
<td>Rn</td>
<td>A or ACC</td>
<td>
ALU Rd, Rm, Rn cycle 1
<br>
ALU Rd, Rm, #I cycle 1
<br>
ALU Rd, Rm, Rn, SHF Rs cycle 2
<br>
ALU Rd, Rm, Rn, SHF #S cycle 2
<br>
MLA cycle 1
<br>
LDM/STM cycles 1 and 2
<br>
LDR|STR Rd, [Rn, +/-Rm]{!} cycle 1
<br>
LDR|STR Rd, [Rn], +/-Rm cycle 1
<br>
LDR|STR Rd, [Rn, #+/-I]{!} cycle 1
<br>
LDR|STR Rd, [Rn], #+/-I cycle 1
<br>
LDR|STR Rd, [Rn, +/-Rm, SHF #S]{!} cycle 2
<br>
LDR|STR Rd, [Rn], +/-Rm, SHF #S cycle 2
</td>
</tr>

<tr>
<td>Rs</td>
<td>A or ACC</td>
<td>
ALU Rd, Rm, Rn, SHF Rs cycle 1
</td>
</tr>

<tr>
<td>PC+8</td>
<td>A</td>
<td>BL</td>
</tr>

<tr>
<td>S (imm shift)</td>
<td>A</td>
<td>
ALU Rd, Rm, Rn, SHF #S cycle 1
<br>
LDR|STR Rd, [Rn, +/-Rm, SHF #S]{!} cycle 1
<br>
LDR|STR Rd, [Rn], +/-Rm, SHF #S cycle 1
</td>
</tr>

<tr>
<td>0</td>
<td>A</td>
<td>MUL cycle 1</td>
</tr>

<tr>
<td>A</td>
<td>A</td>
<td>MUL|MLA cycles 2 and beyond
<br>
(Also divide)
</td>
</tr>

</table>

<p>Where A or ACC is specified the ARM emulator will indicate which, using its register scoreboarding techniques.
<br>
Note also that A must be loaded with Rs when the first MULST is presented (i.e. when the MULIN has been accepted), as the MULST
steps require Rs to be present and then rotate it; A is generally loaded on every cycle, EXCEPT those where a MULST is in the ALU stage.
</p>

<?php page_section( "Op2", "Op2" ); ?>

<table border=1>
<tr>
<th>ARM source</th>
<th>Actual source</th>
<th>Relevant ARM instructions</th>
</tr>

<tr>
<td>Rm</td>
<td>B or ACC</td>
<td>
ALU Rd, Rm, Rn cycle 1
<br>
ALU Rd, Rm, Rn, SHF Rs cycle 1
<br>
ALU Rd, Rm, Rn, SHF #S cycle 1
<br>
MUL|MLA cycle 1
<br>
LDM/STM cycles 1 and 2
<br>
LDR|STR Rd, [Rn, +/-Rm]{!} cycle 1
<br>
LDR|STR Rd, [Rn], +/-Rm cycle 1
<br>
LDR|STR Rd, [Rn, +/-Rm, SHF #S]{!} cycle 1
<br>
LDR|STR Rd, [Rn], +/-Rm, SHF #S cycle 1
</td>
</tr>

<tr>
<td>SHF</td>
<td>SHF</td>
<td>
ALU Rd, Rm, Rn, SHF Rs cycle 2
<br>
ALU Rd, Rm, Rn, SHF #S cycle 2
<br>
LDR|STR Rd, [Rn, +/-Rm, SHF #S]{!} cycle 1
<br>
LDR|STR Rd, [Rn], +/-Rm, SHF #S cycle 1
</td>
</tr>

<tr>
<td>I (immediate)</td>
<td>B</td>
<td>
ALU Rd, Rm, #I cycle 1
<br>
ALU Rd, Rm, Rn, SHF Rs cycle 2
<br>
ALU Rd, Rm, Rn, SHF #S cycle 2
<br>
LDR|STR Rd, [Rn, #+/-I]{!} cycle 1
<br>
LDR|STR Rd, [Rn], #+/-I cycle 1
</td>
</tr>

<tr>
<td>0 (Zero)</td>
<td>B</td>
<td>
LDM|STMIA cycle 1
</td>
</tr>

<tr>
<td>4 (Four)</td>
<td>B</td>
<td>
LDM|STMIB cycle 1
<br>
BL cycle 1
</td>
</tr>

<tr>
<td>4*(number registers)</td>
<td>B</td>
<td>
LDM|STMDB cycle 1
<br>
LDM|STM cycle 2
</td>
</tr>

<tr>
<td>4*(number registers-1)</td>
<td>B</td>
<td>
LDM|STMDA cycle 1
</td>
</tr>

<tr>
<td>ACC</td>
<td>ACC</td>
<td>MUL|MLA cycles 2 and beyond
<br>
(Also divide)
</td>
</tr>

</table>

<p>Where B or ACC is specified the ARM emulator will indicate which, using its register scoreboarding techniques.
</p>


<?php page_section( "Shifter operation and result", "Shifter operation and result" ); ?>

<table border=1>
<tr>
<th>Internal instruction</th>
<th>Operation</th>
<th>Details</th>
<th>Relevant ARM instructions</th>
</tr>

<tr>
<th>ILSL</th>
<td>Op2 &lt;&lt; Op1[8;0]</td>
<td>
Take bottom 8 bits of Op1
<br>
If zero, carry out is zero, result is Op2
<br>
If one to thirtytwo, result is Op2 shifted left by that amount, carry out is last bit shifted out
<br>
If greater than thirtytwo, result is zero, carry out is zero
</td>
<td>
ALU Rd, Rn, Rm, LSL #S
<br>
ALU Rd, Rn, Rm, LSL Rs
<br>
LDR|STR Rd, [Rn, +-Rm, LSL #S]{!}
<br>
LDR|STR Rd, [Rn], +-Rm, LSL #S
</td>
</tr>

<tr>
<th>ILSR</th>
<td>Op2 &gt;&gt; Op1[8;0]</td>
<td>
Take bottom 8 bits of Op1
<br>
If zero, carry out is zero, result is Op2
<br>
If one to thirtytwo, result is Op2 shifted right by that amount, carry out is last bit shifted out
<br>
If greater than thirtytwo, result is zero, carry out is zero
</td>
<td>
ALU Rd, Rn, Rm, LSR #S
<br>
ALU Rd, Rn, Rm, LSR Rs
<br>
LDR|STR Rd, [Rn, +-Rm, LSR #S]{!}
<br>
LDR|STR Rd, [Rn], +-Rm, LSR #S
</td>
</tr>

<tr>
<th>IASR</th>
<td>Op2 &gt;&gt;&gt; Op1[8;0]</td>
<td>
Take bottom 8 bits of Op1
<br>
If zero, carry out is zero, result is Op2
<br>
If one to thirtytwo, result is Op2 arithmetically shifted right by that amount, carry out is last bit shifted out
<br>
If greater than thirtytwo, result is 32 copies of Op2[31], carry out is Op2[31]; the sign bit of Op2.
</td>
<td>
ALU Rd, Rn, Rm, ASR #S
<br>
ALU Rd, Rn, Rm, ASR Rs
<br>
LDR|STR Rd, [Rn, +-Rm, ASR #S]{!}
<br>
LDR|STR Rd, [Rn], +-Rm, ASR #S
</td>
</tr>

<tr>
<th>IROR</th>
<td>Op2 rotate right Op1[8;0]</td>
<td>
Take bottom 8 bits of Op1
<br>
If zero, carry out is zero, result is Op2
<br>
Else take bottom five bits of Op1; if zero, use thirytwo; result is Op2 rotated right by this number; carry out is last bit rotated
</td>
<td>
ALU Rd, Rn, Rm, ROR #S
<br>
ALU Rd, Rn, Rm, ROR Rs
<br>
LDR|STR Rd, [Rn, +-Rm, ROR #S]{!}
<br>
LDR|STR Rd, [Rn], +-Rm, ROR #S
</td>
</tr>

<tr>
<th>IRRX</th>
<td>Op2,C rotate right 1</td>
<td>
Shift Op2 right by one bit as the result, but make bit 31 of the result equal to C
<br>
Make carry out equal to Op2[0].
</td>
<td>
ALU Rd, Rn, Rm, RRX
<br>
LDR|STR Rd, [Rn, +-Rm, RRX]{!}
<br>
LDR|STR Rd, [Rn], +-Rm, RRX
</td>
</tr>

<tr>
<th>IINIT</th>
<td>Op2</td>
<td>
Result is Op2, carry out is zero
</td>
<td>MUL|MLA</td>
</tr>

<tr>
<th>IMUL</th>
<td>SHF LSR 2</td>
<td>
Result is SHF logically shifted right by two bits
<br>
Carry out is zero unless: SHF[2;0] is two and carry in is 1 or SHF[2;0] is three.
</td>
<td>MUL|MLA</td>
</tr>

<tr>
<th>IDIV</th>
<td>SHF ?OR Op2</td>
<td>
If current ALU carry is 1 then result is SHF logically ORred with Op2, else just SHF. Carry out is undefined.
</td>
<td>&nbsp;</td>
</tr>

</table>

<p>
The shifter carry is written whenever it performs an operation; the result is also always written to SHF.
</p>

<?php page_section( "Logical operation and result", "Logical operation and result" ); ?>

<table border=1>
<tr>
<th>Internal instruction</th>
<th>Operation</th>
<th>Details</th>
<th>Relevant ARM instructions</th>
</tr>

<tr>
<th>IMOV</th>
<td>Op2</td>
<td>
Op2
</td>
<td>
MOV
</td>
</tr>

<tr>
<th>INOT</th>
<td>~Op2</td>
<td>
Binary not Op2
</td>
<td>
MVN
</td>
</tr>

<tr>
<th>IAND</th>
<td>Op1 & Op2</td>
<td>
Binary AND of Op1 and Op2
</td>
<td>
AND
<br>
TST
</td>
</tr>

<tr>
<th>IOR</th>
<td>Op1 | Op2</td>
<td>
Binary OR of Op1 and Op2
</td>
<td>
ORR
</td>
</tr>

<tr>
<th>IXOR</th>
<td>Op1 ^ Op2</td>
<td>
Binary XOR of Op1 and Op2
</td>
<td>
EOR
<br>
TEQ
</td>
</tr>

<tr>
<th>IBIC</th>
<td>Op1 & ~Op2</td>
<td>
Binary AND of Op1 and binary not of Op2
</td>
<td>
BIC
</td>
</tr>

<tr>
<th>IORN</th>
<td>Op1 | ~Op2</td>
<td>
Binary OR of Op1 and binary not of Op2
</td>
<td>
&nbsp;
</td>
</tr>

<tr>
<th>IANDXOR</th>
<td>(Op1 & Op2) ^ Op1</td>
<td>
Binary XOR of Op1 and (Op2 binary AND Op1)
<br>
Use for testing that all bits of Op1 are set in Op2
</td>
<td>
&nbsp;
</td>
</tr>

<tr>
<th>IXORCNT</th>
<td>ZC(Op1 ^ Op2)</td>
<td>
Count leading zeros of the result of Op1 binary exclusive or Op2
<br>
This is a result from zero to 32; it is useful for divide, networking, floating point, and more.
</td>
<td>
&nbsp;
</td>
</tr>

<tr>
<th>IINIT</th>
<td>Op1</td>
<td>
Pass Op1 to ACC
</td>
<td>
MUL, MLA
<br>
Also divide
</td>
</tr>

</table>

<p>
The logical unit does not produce a carry or overflow; it produces a zero flag indicating its result is zero, and a negative flag indicating the top bit of its result is set.
</p>

<?php page_section( "Arithmetic operation and result", "Arithmetic operation and result" ); ?>

<table border=1>
<tr>
<th>Internal instruction</th>
<th>Operation</th>
<th>Details</th>
<th>Relevant ARM instructions</th>
</tr>

<tr>
<th>IADD</th>
<td>Op1 + Op2</td>
<td>
Op1 + Op2
</td>
<td>
ADD
<br>
CMN
</td>
</tr>

<tr>
<th>IADC</th>
<td>Op1 + Op2 + C</td>
<td>
Op1 + Op2 + C
</td>
<td>
ADC
</td>
</tr>

<tr>
<th>ISUB</th>
<td>Op1 - Op2</td>
<td>
Op1 + ~Op2 + 1
</td>
<td>
SUB
<br>
CMP
</td>
</tr>

<tr>
<th>ISUBC</th>
<td>Op1 - Op2 plus carry</td>
<td>
Op1 + ~Op2 + C
</td>
<td>
SBC
</td>
</tr>

<tr>
<th>IRSUB</th>
<td>-Op1 + Op2</td>
<td>
~Op1 + Op2 + 1
</td>
<td>
RSB
</td>
</tr>

<tr>
<th>IRSUBC</th>
<td>-Op1 + Op2 plus carry</td>
<td>
~Op1 + Op2 + C
</td>
<td>
RSC
</td>
</tr>

<tr>
<th>IMUL</th>
<td>(~)Op1(*2) + Op2 (+1)</td>
<td>
Determine SHF[2;0]+shifter carry.
<ul>
<li>If 0 or 4 return Op2</li>
<li>If 1, return Op1 + Op2 + 0</li>
<li>If 2, return (Op1*2) + Op2 + 0</li>
<li>If 3, return ~Op1 + Op2 + 1</li>
</ul>
</td>
<td>
MUL, MLA
</td>
</tr>

<tr>
<th>IDIV</th>
<td>-Op1 + Op2, or Op2</td>
<td>
~Op1 + Op2 + 1, generating carry;
If carry is one then return that as result, else return Op2
</td>
<td>
&nbsp;
</td>
</tr>

</table>

<p>
The arithmetic unit produces a carry (carry out of the adder),
overflow (from the adder), a zero flag indicating its result is zero, and a negative flag indicating the top bit of its result is set.
</p>

<?php page_section( "Result values", "Result values" ); ?>

<p>
Conditional execution may block execution; no effects will occur if a conditional operation is performed and its condition is not met.
<br>
With that in mind:
<ol>
<li>
The ALU result is the result of the logical or arithmetic operation performed: if a logical operation was performed then
the result comes from the logic unit (as do the N and Z flags; V is unchanged; C may come from the shifters last carry out);
if an arithmetic operation was performed then the result comes from the arithmetic unit (as do N, Z, V, C). Note that for shifter result
to be seen it must be moved through the logical or arithmetic path in a second instruction, as it is not muxed through to the output.
</li>
<li>
The shifter result is always written to the SHF register on execution
</li>
<li>
The accumulator may be written with the ALU result if the instruction indicates
</li>
<li>
The ALU 'A' register is clocked in from the previous pipeline stage on execution, <i>except</i> when an IMUL or IDIV instruction is executed. In these cases
the value of the A register is logically shifted left by 2 (for a multiply) or right by 1 (for a divide).
</li>
<li>
The ALU 'B' register is clocked in from the previous pipeline stage on execution, <i>except</i> when an IDIV instruction is executed. In this case
the value of the B register is logically shifted right by 1 (for a divide).
</li>
</ol>
</p>



<?php
page_ep();

include "${toplevel}web_assist/web_footer.php"; ?>

