<?php

include "web_locals.php";
include "${toplevel}web_assist/web_globals.php";

site_set_location("gip.alu.operations");

$page_title = "GIP Documentation";

include "${toplevel}web_assist/web_header.php";

page_header( "GIP ALU and Shifter Operations" );

page_sp();
?>

This page gives details of the operations the ALU and shifter can perform, how they are used, and how ARM emulation mode emulates the ARM instruction set with the ALU and shifter operations.

<?php page_section( "descriptions", "Descriptions" ); ?>

<p>
In the following table Op1 can be A or ACC, Op2 can be B or ACC; C indicates the
carry flag
</p>

<table border=1>

<tr>
<th>Mnemonic</th>
<th>Operation</th>
<th>Algebraic</th>
<th>Description</th>
</tr>

<tr>
<th>MOV</th>
<th>Move</th>
<td>Op2</td>
<td>Move an operand</td>
</tr>

<tr>
<th>MVN</th>
<th>Negate</th>
<td>~Op2</td>
<td>Ones complement an operand</td>
</tr>

<tr>
<th>AND</th>
<th>And</th>
<td>Op1 & Op2</td>
<td>Logical and two operands</td>
</tr>

<tr>
<th>OR</th>
<th>Or</th>
<td>Op1 | Op2</td>
<td>Logical or two operands</td>
</tr>

<tr>
<th>XOR</th>
<th>Exclusive-or</th>
<td>Op1 ^ Op2</td>
<td>Logical exclusive or two operands</td>
</tr>

<tr>
<th>BIC</th>
<th>Bit clear</th>
<td>Op1 & ~Op2</td>
<td>Clear bits of Op1 that are set in Op2</td>
</tr>

<tr>
<th>ORN</th>
<th>Or not</th>
<td>Op1 | ~Op2</td>
<td>Set bits of Op1 that are clear in Op2</td>
</tr>

<tr>
<th>ANDX</th>
<th>AND and then XOR operands</th>
<td>(Op1 & Op2)^Op2</td>
<td>AND Op1 and Op2, then XOR the result with Op2</td>
</tr>

<tr>
<th>ANDC</th>
<th>AND and count number ones in result</th>
<td>ZC(Op1 ^ Op2)</td>
<td>AND Op1 and Op2<br>&nbsp;and count number of ones in result (0 to 32)</td>
</tr>

<tr>
<th>XORF</th>
<th>XOR and count zeros from left</th>
<td>LZC(Op1 ^ Op2)</td>
<td>Exclusive-or Op1 and Op2<br>&nbsp;and count number of leading zeros in result (0 to 32)</td>
</tr>

<tr>
<th>XORL</th>
<th>XOR and count zeros from right</th>
<td>TZC(Op1 ^ Op2)</td>
<td>Exclusive-or Op1 and Op2<br>&nbsp;and count number of trailing zeros in result (0 to 32)</td>
</tr>

<tr>
<th>BITR</th>
<th>Bit reverse bottom byte</th>
<td>bitrev8(Op2)</td>
<td>Bit reverse the bottom byte of Op2, use rest of Op2</td>
</tr>

<tr>
<th>BYTR</th>
<th>Byte reverse</th>
<td>byterev(Op2)</td>
<td>Byte reverse Op2</td>
</tr>

<tr>
<th>LSL</th>
<th>Logical shift left</th>
<td>Op2 LSL Op1</td>
<td>Logical shift left;<br>&nbsp;carry out is last bit shifted out</td>
</tr>

<tr>
<th>LSR</th>
<th>Logical shift right</th>
<td>Op2 LSR Op1</td>
<td>Logical shift right;<br>&nbsp;carry out is last bit shifted out</td>
</tr>

<tr>
<th>ASR</th>
<th>Arithmetic shift right</th>
<td>Op2 ASR Op1</td>
<td>Arithmetic shift right;<br>&nbsp;carry out is last bit shifted out</td>
</tr>

<tr>
<th>ROR</th>
<th>Rotate right</th>
<td>Op2 ROR Op1</td>
<td>Rotate right;<br>&nbsp;carry out is last bit rotated out</td>
</tr>

<tr>
<th>ROR33</th>
<th>33-bit rotate right</th>
<td>Op2,C ROR #1</td>
<td>Rotate right 33-bit value by one bit;<br>&nbsp;carry out is bit rotated out of
Op2</td>
</tr>

<tr>
<th>ADD</th>
<th>Add</th>
<td>Op1 + Op2</td>
<td>Add two operands</td>
</tr>

<tr>
<th>ADC</th>
<th>Add with carry</th>
<td>Op1 + Op2 + C</td>
<td>Add two operands with carry</td>
</tr>

<tr>
<th>SUB</th>
<th>Subtract</th>
<td>Op1+ ~Op2 + 1</td>
<td>Subtract one operand from the other</td>
</tr>

<tr>
<th>SBC</th>
<th>Subtract with carry</th>
<td>Op1  + ~Op2 + C</td>
<td>Subtract one operand from the other with carry</td>
</tr>

<tr>
<th>RSUB</th>
<th>Reverse subtract</th>
<td>~Op1 + Op2 + 1</td>
<td>Subtract one operand from the other</td>
</tr>

<tr>
<th>RSBC</th>
<th>Reverse subtract with carry</th>
<td>~Op1 + Op2 + C</td>
<td>Subtract one operand from the other with carry</td>
</tr>

<tr>
<th>INIT</th>
<th>Init a multiplication or division</th>
<td>ACC = Op1;<br>SHF = Op2;<br>C=0;<br>Z=(SHF==0)</td>
<td>Initialize a multiplication or division</td>
</tr>

<tr>
<th>MULST</th>
<th>Do a multiplication step</th>
<td>ACC+=A[*2/1/0/-1];<br>A ASL 2;<br>SHF LSR 2;<br>C=0/1;<br> Z=(SHF==0)</td>
<td>Perform a Booths 2-bit multiplication step</td>
</tr>

<tr>
<th>DIVST</th>
<th>Do a division step</th>
<td>ACC=Acc-?Op1;<br>SHF=SHF|?Op2;<br>B SHR 1;<br>A SHR 1</td>
<td>Perform a single bit of a division step</td>
</tr>

</table>

<?php page_section( "overview", "Overview" ); ?>

<p>
The data operations that the ALU and shifter are capable of are, broadly speaking:
</p>

<ol>
<li>Addition and subtraction</li>
<li>AND, OR, NOT, XOR</li>
<li>Left shift by arbitrary amount</li>
<li>Logical right shift by arbitrary amount</li>
<li>Arithmetic right shift by arbitrary amount</li>
<li>Rotate by arbitrary amount</li>
<li>Two-bit step of multiply-accumulate</li>
<li>Single bit step of divide</li>
<li>XOR and zero-bit count (top down)</li>
</ol>

<?php page_section( "use", "Operation use" ); ?>

<p>

The use of the operations of the ALU and shifter is most importantly tied to the 
ARM instruction set. This section covers how the ALU is used
in those instructions, plus some additional information on extended instructions
for the GIP 16-bit instruction set for other operations not
supported by the ARM.

</p>

<?php page_subsection( "multiplication", "Multiplication" ); ?>

<p>A multiply instruction can be implemented with Booths algorithm calculating 
two bits of result at a time. Say a calculation of the form
r=x*y+z needs to be performed. The basic multiply step of 2 bits can be
performed as follows:

<ol>
<li>Take the bottom two bits of multiplier</li>
<li>If there is a carry from the previous stage, add that to the bottom 2 bits
of the multiplier (held in A) (so we now have a number from 0 to 4)</li>
<li>If the number is zero then leave the accumulator alone, and clear the
carry</li>
<li>If the number is one then add the current multiplicand to the accumulator,
and clear the carry</li>
<li>If the number is two then add the current multiplicand left shifted by one
to the accumulator, and clear the carry</li>
<li>If the number is three then subtract the current multiplicand from the
accumulator, and set the carry</li>
<li>If the number is four then leave the accumulator alone, and set the
carry</li>
<li>Shift the mutliplier right by two bits (logical, not arithmetic)</li>
<li>Shift the multiplicand left by two bits</li>
</ol>

The multiplication can be terminated when the multiplier is all zero and the
carry is zero, or when the multiplicand is zero.

<br>

Initialization requires the accumulator to be set to the value 'z' and the
multiplier and multiplicand to be stored in the correct places.

</p>

<?php page_subsection( "division", "Division" ); ?>

<p>
A divide instruction can be implemented with the divide step instruction, one 
bit at a time. Say a calculation of the form r=x/y needs to be performed.
The basic divide step per bit can be performed as follows:

<ol>
<li>
Subtract B from ACC as the ALU result, generating the carry
</li>
<li>OR SHF with A as the logical result
</li>
<li>If B less than or equal to ACC (carry=1) then store the ALU result in ACC,
and the logical result in SHF</li>
<li>LSR B and LSR A</li>
</ol>

<br>

Initialization requires the leading zeros in the denominator (y) to be counted,
then the denominator shifted left by that and placed in B, and one shifted left
by that to be placed in A.
The accumulator should have the numerator value (x), and SHF should be zeroed.

</p>

<?php page_section( "arm_emulation" , "ARM emulation" ); ?>

<p>
The ARM instruction classes in general are as follows:
</p>

<table border=1>
<tr>
<th>Class</th>
<th>Description</th>
<th>ALU note</th>
</tr>

<tr>
<th>Data processing</th>
<td>ALU, Shift, and combined instructions</td>
<td>Varied emulation issues; see below</td>
</tr>

<tr>
<th>Multiply</th>
<td>Multiply and multiply accumulate</td>
<td>Utilizes INIT an MULST</td>
</tr>

<tr>
<th>Single Data Swap</th>
<td><i>Not supported</i></td>
<td>No ALU issues</td>
</tr>

<tr>
<th>Single Data Transfer</th>
<td>Load or store from register plus offset, possibly shifted</td>
<td>Access and/or writeback address calculation</td>
</tr>

<tr>
<th>Undefined</th>
<td><i>Not supported</i></td>
<td>No ALU issues</td>
</tr>

<tr>
<th>Block data transfer</th>
<td>Load or Store multiple</td>
<td>Base and writeback address calculation</td>
</tr>

<tr>
<th>Branch</th>
<td>Branch with or without link</td>
<td>Write path for link register in BL</td>
</tr>

<tr>
<th>Coproc Data Transfer</th>
<td><i>Not supported</i></td>
<td>No ALU issues</td>
</tr>

<tr>
<th>Coproc Data Operation</th>
<td><i>Not supported</i></td>
<td>No ALU issues</td>
</tr>

<tr>
<th>Coproc Register Transfer</th>
<td><i>Not supported</i></td>
<td>No ALU issues</td>
</tr>

<tr>
<th>Software interrupt</th>
<td>Emulated trap</td>
<td>No ALU issues</td>
</tr>

</table>

<p>

For ARM emulation and the shifter/ALU we are concerned with the data
processing, multiply, load/store, and branch ARM instructions. They can be
emulated as follows:

<dl>

<dt>ADD/ADC/SUB/SBC/RSB/RSC{cond}[S] Rd, Rn, Rm (Rd not PC)</dt>
<dd>These map to a single conditional ALU operation, with Op1=Rn, Op2=Rm, and 
the N, Z, V, C flags being set based on the result
 if the condition presented is already passed by the current flags. The 
accumulator is set with the result if the condition passed.
</dd>

<dt>AND/ORR/BIC/EOR{cond}[S] Rd, Rn, Rm (Rd not PC)</dt>
<dd>These map to a single conditional ALU operation, with Op1=Rn, Op2=Rm, and
the N, Z flags being set based on the result if the condition
presented is already passed by the current flags. The accumulator is set with 
the result if the condition passed.
</dd>

<dt>MOV/MVN{cond}[S] Rd, Rm (Rd not PC)</dt>
<dd>These map to a single conditional ALU operation, with Op1=Rn, Op2=Rm, and 
the N, Z flags being set based on the result if
the condition presented is already passed by the current flags. The accumulator
is set with the result if the condition passed.
</dd>

<dt>CMP/CMN{cond} Rn, Rm</dt>
<dd>These map to a single conditional ALU operation, (SUB or ADD respectively),
with Op1=Rn, Op2=Rm, and the N, Z, V, C flags being set based on the result
 if the condition presented is already passed by the current flags.
</dd>

<dt>TST/TEQ{cond} Rn, Rm</dt>
<dd>These map to a single conditional ALU operation, (AND or XOR respectively) 
with Op1=Rn, Op2=Rm, and the N, Z flags being set based on the result if the
condition
presented is already passed by the current flags.
</dd>

<dt>ADD/ADC/SUB/SBC/RSB/RSC{cond}[S] Rd, Rn, #I (Rd not PC)</dt>
<dd>These map to a single conditional ALU operation, with Op1=Rn, Op2=I, and the 
N, Z, V, C flags being set based on the result
 if the condition presented is already passed by the current flags. The
accumulator is set with the result if the condition passed.
</dd>

<dt>AND/ORR/BIC/EOR{cond}[S] Rd, Rn, #I (Rd not PC)</dt>
<dd>These map to a single conditional ALU operation, with Op1=Rn, Op2=I and the
N, Z flags being set based on the result if the condition
presented is already passed by the current flags. The accumulator is set with
the result if the condition passed.
</dd>

<dt>MOV/MVN{cond}[S] Rd, #I (Rd not PC)</dt>
<dd>These map to a single conditional ALU operation, with Op1=Rn, Op2=I, and the
N, Z flags being set based on the result if
the condition presented is already passed by the current flags. The accumulator
is set with the result if the condition passed.
</dd>

<dt>CMP/CMN{cond} Rn, #I</dt>
<dd>These map to a single conditional ALU operation, (SUB or ADD respectively),
with Op1=Rn, Op2=I, and the N, Z, V, C flags being set based on the result
 if the condition presented is already passed by the current flags.
</dd>

<dt>TST/TEQ{cond} Rn, #I</dt>
<dd>These map to a single conditional ALU operation, (AND or XOR respectively)
with Op1=Rn, Op2=I, and the N, Z flags being set based on the result if the
condition
presented is already passed by the current flags.
</dd>

<dt>ADD/ADC/SUB/SBC/RSB/RSC{cond}[S] Rd, Rn, Rm, SHIFT #S/Rs (Rd not PC)</dt>
<dd>These map to two conditional operations, a shift and an ALU operation. The
first operation is Op1=S or Rs, Op2=Rm, and the shifter produces a result which
is always stored in SHF and a carry that is stored in the shifter carry; the
second operation is Op1=Rn, Op2=SHF, and the N, Z, V, C flags are set based on
the result  if the condition presented is already passed by the current flags.
The accumulator is set with the result if the condition passed.
</dd>

<dt>AND/ORR/BIC/EOR{cond}[S] Rd, Rn, Rm, SHIFT #S/Rs (Rd not PC)</dt>
<dd>These map to two conditional operations, a shift and an ALU operation. The
first operation is Op1=S or Rs, Op2=Rm, and the shifter produces a result which
is always stored in SHF and a carry that is stored in the shifter carry; the
second operation is Op1=Rn, Op2=SHF, and the N, Z flags being set based on the
result if the condition
presented is already passed by the current flags. The accumulator is set with
the result if the condition passed.
</dd>

<dt>MOV/MVN{cond}[S] Rd, Rm, SHIFT #S/Rs (Rd not PC)</dt>
<dd>These map to two conditional operations, a shift and an ALU operation. The
first operation is Op1=S or Rs, Op2=Rm, and the shifter produces a result which
is always stored in SHF and a carry that is stored in the shifter carry; the
second operation is Op1=Rn, Op2=SHF, and the N, Z flags being set based on the
result if
the condition presented is already passed by the current flags. The accumulator
is set with the result if the condition passed.
</dd>

<dt>CMP/CMN{cond} Rn, Rm, SHIFT #S/Rs</dt>
<dd>These map to two conditional operations, a shift and an ALU operation. The
first operation is Op1=S or Rs, Op2=Rm, and the shifter produces a result which
is always stored in SHF and a carry that is stored in the shifter carry; the
second operation is Op1=Rn, Op2=SHF, (SUB or ADD respectively), and the N, Z, V,
C flags being set based on the result  if the condition presented is already
passed by the current flags.
</dd>

<dt>TST/TEQ{cond} Rn, Rm, SHIFT #S/Rs</dt>
<dd>These map to two conditional operations, a shift and an ALU operation. The
first operation is Op1=S or Rs, Op2=Rm, and the shifter produces a result which
is always stored in SHF and a carry that is stored in the shifter carry; the
second operation is Op1=Rn, Op2=SHF, (AND or XOR respectively), and the N, Z
flags being set based on the result if the condition
presented is already passed by the current flags.
</dd>

<dt>MOV{cond}[S] PC, Rm</dt>
<dd>This instruction is emulated as a two-cycle instruction, the first of which
is as a conditional MOV ALU operation, with no results written back. If the
condition passes then execution
flow will change to the result of the ALU path; if it fails then the next
instruction is performed. A speculative fetch will be performed of the address
in Rm when it is read from the
register file. The 'S' field is always ignored.
</dd>

<dt>ADD/ADC/SUB/SBC/RSB/RSC{cond}[S] PC, Rn, Rm</dt>
<dd>These instructions are emulated as multi-cycle instructions, the first of
which is a conditional ALU operation with Op1=Rn, Op2=Rm, with no results
written back. If the condition passes then execution
flow will change to the result of the ALU operation; if it fails then the next
instruction is performed after a single cycle of delay. The 'S' field is always
ignored.
</dd>

<dt>AND/ORR/BIC/EOR{cond}[S] PC, Rn, Rm</dt>
<dd>
These instructions are emulated as multi-cycle instructions, the first of
which is a conditional ALU operation with Op1=Rn, Op2=Rm, with no results
written back. If the condition passes then execution
flow will change to the result of the ALU operation; if it fails then the next
instruction is performed after a single cycle of delay. The 'S' field is always
ignored.
</dd>

<dt>MVN{cond}[S] PC, Rm</dt>
<dd>
These instructions are emulated as multi-cycle instructions, the first of
which is a conditional ALU operation with Op1=Rn, Op2=Rm, with no results
written back. If the condition passes then execution
flow will change to the result of the ALU operation; if it fails then the next
instruction is performed after a single cycle of delay. The 'S' field is always
ignored.
</dd>

<dt>ADD/ADC/SUB/SBC/RSB/RSC{cond}[S] PC, Rn, #I</dt>
<dd>
These instructions are emulated as multi-cycle instructions, the first of
which is a conditional ALU operation with Op1=Rn, Op2=I, with no results
written back. If the condition passes then execution
flow will change to the result of the ALU operation; if it fails then the next
instruction is performed after a single cycle of delay. The 'S' field is always
ignored.
</dd>

<dt>AND/ORR/BIC/EOR{cond}[S] PC, Rn, #I</dt>
<dd>
These instructions are emulated as multi-cycle instructions, the first of
which is a conditional ALU operation with Op1=Rn, Op2=I, with no results
written back. If the condition passes then execution flow will change to the
result of the ALU operation; if it fails then the next instruction is performed
after a single cycle of delay. The 'S' field is always ignored.
</dd>

<dt>MOV/MVN{cond}[S] PC, #I</dt>
<dd>
These instructions are emulated as multi-cycle instructions, the first of
which is a conditional ALU operation with Op1=Rn, Op2=I, with no results
written back. If the condition passes then execution
flow will change to the result of the ALU operation; if it fails then the next
instruction is performed after a single cycle of delay. The 'S' field is always
ignored.
</dd>

<dt>ADD/ADC/SUB/SBC/RSB/RSC{cond}[S] PC, Rn, Rm, SHIFT #I/Rs</dt>
<dd>
These instructions are emulated as multi-cycle instructions.
The first operation is a conditional shift with Op1=I or Rs, Op2=Rm, and the
shifter produces a  result which is always stored in SHF and a carry that is
stored in the shifter carry. The second operation is an ALU operation with
Op1=Rn, Op2=SHF, with no results
written back. If the first cycle condition passes then an additional
delay cycle is inserted, and execution
flow will change to the result of the ALU operation; if it fails then the next
instruction is performed.The 'S' field is always
ignored.
</dd>

<dt>AND/ORR/BIC/EOR{cond}[S] PC, Rn, Rm, SHIFT #I/Rs</dt>
<dd>
These instructions are emulated as multi-cycle instructions.
The first operation is a conditional shift with Op1=I or Rs, Op2=Rm, and the
shifter produces a  result which is always stored in SHF and a carry that is
stored in the shifter carry. The second operation is an ALU operation with
Op1=Rn, Op2=SHF, with no results
written back. If the first cycle condition passes then an additional
delay cycle is inserted, and execution
flow will change to the result of the ALU operation; if it fails then the next
instruction is performed.The 'S' field is always
ignored.
</dd>

<dt>MOV/MVN{cond}[S] PC, Rm, SHIFT #I/Rs</dt>
<dd>
These instructions are emulated as multi-cycle instructions.
The first operation is a conditional shift with Op1=I or Rs, Op2=Rm, and the
shifter produces a  result which is always stored in SHF and a carry that is
stored in the shifter carry. The second operation is an ALU operation with
Op1=Rn, Op2=SHF, with no results
written back. If the first cycle condition passes then  an additional
delay cycle is inserted, and execution
flow will change to the result of the ALU operation; if it fails then the next
instruction is performed.The 'S' field is always
ignored.
</dd>

<dt>MUL{cond}[S] Rd, Rm, Rs (Rd may not be PC: ARM restriction)</dt>
<dt>MLA{cond}[S] Rd, Rm, Rs, Rn (Rd may not be PC: ARM restriction)</dt>
<dd>
These instructions are emulated as multi-cycle instructions.
The first operation is a conditional MULIN with Op1=Rn or 0 and Op2=Rm. The second and further operations
are MULST, with the A input set to Rs.
If the conditional first instruction fails then the instruction terminates early.
If the ALU indicates that the termination condition is met then one more MULST operation will take
place (with register file write details), then the multiply will complete. In any case at most 16 MULSTs will be issued,
and the last will always have register file write enabled.
If the S flag is set then the final MULST will write the Z and the N flags.
<br>
Note that due to the register forwarding structure a multiply operation must block until all its component registers are present in the register file; it cannot
use the value in the accumulator for any purpose.
</dd>

<dt>LDR|STR{cond} Rd, [Rn, #+/-I]{!}</dt>
<dt>LDR|STR{cond} Rd, [Rn, +/-Rm]{!}</dt>
<dd>
The ALU performs the address calculation for the data read (and possible writeback) by being issued an addition or subtraction operation
with Op1=Rn, Op2=I or Rm
</dd>

<dt>LDR|STR{cond} Rd, [Rn], #+/-I</dt>
<dt>LDR|STR{cond} Rd, [Rn], +/-Rm</dt>
<dd>
The ALU performs the address calculation for writeback by being issued an addition or subtraction operation
with Op1=Rn, Op2=I or Rm
</dd>

<dt>LDR|STR{cond} Rd, [Rn, +/-Rm, SHF #S]{!}</dt>
<dd>
The ALU performs two instructions; the first is a shift with Op2=Rm, Op1=S, putting the result in the SHF register.
The second is the address calculation for the data read (and possible writeback) by being issued an addition or subtraction operation
with Op1=Rn, Op2=SHF
</dd>

<dt>LDR|STR{cond} Rd, [Rn], +/-Rm, SHF #S</dt>
<dd>
The ALU performs two instructions; the first is a shift with Op2=Rm, Op1=S, putting the result in the SHF register.
The second is the address calculation for writeback by being issued an addition or subtraction operation
with Op1=Rn, Op2=SHF
</dd>

<dt>LDM|STM{cond}IB Rn[!], Rlist (Rn may not be in Rlist if writeback is specified)</dt>
<dd>
The ALU performs the base address calculation by being issued an addition operation
with Op1=Rn, Op2=4, and the writeback address with an addition operation with
Op1=Rn, Op2=4*(number of registers in Rlist)
<br>
If the PC is specified in Rlist and the operation is an LDM then more cycles are taken by the instruction; the ALU is issued with NOPs on those cycles.
</dd>

<dt>LDM|STM{cond}IA Rn[!], Rlist (Rn may not be in Rlist if writeback is specified)</dt>
<dd>
The ALU performs the base address calculation by being issued an addition operation
with Op1=Rn, Op2=0, and the writeback address with an addition operation with
Op1=Rn, Op2=4*(number of registers in Rlist)
<br>
If the PC is specified in Rlist and the operation is an LDM then more cycles are taken by the instruction; the ALU is issued with NOPs on those cycles.
</dd>

<dt>LDM|STM{cond}DB Rn[!], Rlist (Rn may not be in Rlist if writeback is specified)</dt>
<dd>
The ALU performs the base address calculation by being issued a subtraction operation
with Op1=Rn, Op2=4*(number of registers in Rlist), and the writeback address with a subtraction operation with
Op1=Rn, Op2=4*(number of registers in Rlist)
<br>
If the PC is specified in Rlist and the operation is an LDM then more cycles are taken by the instruction; the ALU is issued with NOPs on those cycles.
</dd>

<dt>LDM|STM{cond}DA Rn[!], Rlist (Rn may not be in Rlist if writeback is specified)</dt>
<dd>
The ALU performs the base address calculation by being issued a subtraction operation
with Op1=Rn, Op2=4*(number of registers in Rlist-1), and the writeback address with a subtraction operation with
Op1=Rn, Op2=4*(number of registers in Rlist)
<br>
If the PC is specified in Rlist and the operation is an LDM then more cycles are taken by the instruction; the ALU is issued with NOPs on those cycles.
</dd>

<dt>BL{cond} offset</dt>
<dd>
The ALU performs the return address calculation by being issued a subtraction operation
with Op1=PC+8, Op2=4
</dd>

</dl>

</p>

<?php
page_ep();

include "${toplevel}web_assist/web_footer.php"; ?>

