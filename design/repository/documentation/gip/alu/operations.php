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
<th>Class</th>
<th>Operation</th>
<th>Algebraic</th>
<th>Description</th>
</tr>

<tr>
<th>MOV</th>
<th>Logic</th>
<th>Move</th>
<td>Op2</td>
<td>Move an operand</td>
</tr>

<tr>
<th>MVN</th>
<th>Logic</th>
<th>Negate</th>
<td>~Op2</td>
<td>Ones complement an operand</td>
</tr>

<tr>
<th>AND</th>
<th>Logic</th>
<th>And</th>
<td>Op1 & Op2</td>
<td>Logical and two operands</td>
</tr>

<tr>
<th>OR</th>
<th>Logic</th>
<th>Or</th>
<td>Op1 | Op2</td>
<td>Logical or two operands</td>
</tr>

<tr>
<th>XOR</th>
<th>Logic</th>
<th>Exclusive-or</th>
<td>Op1 ^ Op2</td>
<td>Logical exclusive or two operands</td>
</tr>

<tr>
<th>BIC</th>
<th>Logic</th>
<th>Bit clear</th>
<td>Op1 & ~Op2</td>
<td>Clear bits of Op1 that are set in Op2</td>
</tr>

<tr>
<th>ORN</th>
<th>Logic</th>
<th>Or not</th>
<td>Op1 | ~Op2</td>
<td>Set bits of Op1 that are clear in Op2</td>
</tr>

<tr>
<th>ANDX</th>
<th>Logic</th>
<th>AND and then XOR operands</th>
<td>(Op1 & Op2)^Op2</td>
<td>AND Op1 and Op2, then XOR the result with Op2</td>
</tr>

<tr>
<th>ANDC</th>
<th>Logic</th>
<th>AND and count number ones in result</th>
<td>ZC(Op1 ^ Op2)</td>
<td>AND Op1 and Op2<br>&nbsp;and count number of ones in result (0 to 32)</td>
</tr>

<tr>
<th>XORF</th>
<th>Logic</th>
<th>XOR and count zeros from left</th>
<td>LZC(Op1 ^ Op2)</td>
<td>Exclusive-or Op1 and Op2<br>&nbsp;and count number of leading zeros in result (0 to 32)</td>
</tr>

<tr>
<th>BITR</th>
<th>Logic</th>
<th>Bit reverse bottom byte</th>
<td>bitrev8(Op2)</td>
<td>Bit reverse the bottom byte of Op2, use rest of Op2</td>
</tr>

<tr>
<th>BYTR</th>
<th>Logic</th>
<th>Byte reverse</th>
<td>byterev(Op2)</td>
<td>Byte reverse Op2</td>
</tr>

<tr>
<th>RDFL</th>
<th>Logic</th>
<th>Read flags</th>
<td>VCNZ</td>
<td>Read the current flags<br>(bit 3, V to bit 0, Z)</td>
</tr>

<tr>
<th>LSL</th>
<th>Shift</th>
<th>Logical shift left</th>
<td>Op2 LSL Op1</td>
<td>Logical shift left;<br>&nbsp;carry out is last bit shifted out</td>
</tr>

<tr>
<th>LSR</th>
<th>Shift</th>
<th>Logical shift right</th>
<td>Op2 LSR Op1</td>
<td>Logical shift right;<br>&nbsp;carry out is last bit shifted out</td>
</tr>

<tr>
<th>ASR</th>
<th>Shift</th>
<th>Arithmetic shift right</th>
<td>Op2 ASR Op1</td>
<td>Arithmetic shift right;<br>&nbsp;carry out is last bit shifted out</td>
</tr>

<tr>
<th>ROR</th>
<th>Shift</th>
<th>Rotate right</th>
<td>Op2 ROR Op1</td>
<td>Rotate right;<br>&nbsp;carry out is last bit rotated out</td>
</tr>

<tr>
<th>ROR33</th>
<th>Shift</th>
<th>33-bit rotate right</th>
<td>Op2,C ROR #1</td>
<td>Rotate right 33-bit value by one bit;<br>&nbsp;carry out is bit rotated out of
Op2</td>
</tr>

<tr>
<th>ADD</th>
<th>Arithmetic</th>
<th>Add</th>
<td>Op1 + Op2</td>
<td>Add two operands</td>
</tr>

<tr>
<th>ADC</th>
<th>Arithmetic</th>
<th>Add with carry</th>
<td>Op1 + Op2 + C</td>
<td>Add two operands with carry</td>
</tr>

<tr>
<th>SUB</th>
<th>Arithmetic</th>
<th>Subtract</th>
<td>Op1+ ~Op2 + 1</td>
<td>Subtract one operand from the other</td>
</tr>

<tr>
<th>SBC</th>
<th>Arithmetic</th>
<th>Subtract with carry</th>
<td>Op1  + ~Op2 + C</td>
<td>Subtract one operand from the other with carry</td>
</tr>

<tr>
<th>RSUB</th>
<th>Arithmetic</th>
<th>Reverse subtract</th>
<td>~Op1 + Op2 + 1</td>
<td>Subtract one operand from the other</td>
</tr>

<tr>
<th>RSBC</th>
<th>Arithmetic</th>
<th>Reverse subtract with carry</th>
<td>~Op1 + Op2 + C</td>
<td>Subtract one operand from the other with carry</td>
</tr>

<tr>
<th>INIT</th>
<th>Arithmetic</th>
<th>Init a multiplication or division</th>
<td>ACC = Op2<br>SHF = Op1 LSR 0<br>C=0, P=0</td>
<td>Initialize a multiplication or division</td>
</tr>

<tr>
<th>MLA</th>
<th>Arithmetic</th>
<th>Do a multiplication step</th>
<td>ACC+=Op1[*2/1/0/-1]<br>B=Op1<br>SHF=SHF LSR 2<br>C=0/1</td>
<td>Perform a Booths 2-bit multiplication step (taking B from RF stage)<br>B multiplier is SHF[2;0]+P, with 3 as 4-1</td>
</tr>

<tr>
<th>MLB</th>
<th>Arithmetic</th>
<th>Do a multiplication step</th>
<td>ACC+=B[*2/1/0/-1]<br>B=B LSL 2<br>SHF=SHF LSR 2<br>C=0/1</td>
<td>Perform a Booths 2-bit multiplication step (taking B from B LSL 2)<br>B multiplier is SHF[2;0]+P, with 3 as 4-1</td>
</tr>

<tr>
<th>DIVST</th>
<th>Arithmetic</th>
<th>Do a division step</th>
<td>ACC=Acc-?Op1;<br>SHF=SHF|?Op2;<br>B SHR 1;<br>A SHR 1</td>
<td>Perform a single bit of a division step</td>
</tr>

<tr>
<th>WRFL</th>
<th>Arithmetic</th>
<th>Write flags</th>
<td>VCNZ=Op1</td>
<td>Write the flags from an input directly</td>
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

This section describes how some of the more complex ALU operations can be used

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

<?php
page_ep();

include "${toplevel}web_assist/web_footer.php"; ?>

