<?php

include "web_locals.php";
include "${toplevel}web_assist/web_globals.php";

site_set_location("gip.alu.implementation");

$page_title = "GIP Documentation";

include "${toplevel}web_assist/web_header.php";

page_header( "GIP ALU and Shifter Implementation" );

page_sp();
?>

<?php page_section( "module_definition", "Module definition" ); ?>

<p>
The ALU stage, then, requires the following controls and input data, which can be described as 'ports' to the ALU stage
</p>

<?php page_subsection( "inputs", "Inputs" ); ?>

<table border=1>
<tr>
<th>Port</th>
<th>Type</th>
<th>Details</th>
</tr>

<tr>
<td>alu_a_in</td>
<td>t_word</td>
<td>
ALU A data input (32-bit word)
</td>
</tr>

<tr>
<td>alu_b_in</td>
<td>t_word</td>
<td>
ALU B data input (32-bit word)
</td>
</tr>

<tr>
<td>alu_op</td>
<td>t_alu_op</td>
<td>
Operation for the logical or arithmetic unit to perform. One of: mov, not, and, or, xor, bic, orn, xorcnt, init, add, adc, sub, sbc, rsb, rsc, mul, div.
</td>
</tr>

<tr>
<td>shift_op</td>
<td>t_shift_op</td>
<td>
Operation for the shifter to perform. One of: lsl, lsr, asr, ror, rrx, init, mul, div.
</td>
</tr>

<tr>
<td>condition</td>
<td>t_alu_condition</td>
<td>
Condition to be checked. One of: CS, CC, EQ, NE, VC, VS, GT, GE, LT, LE, LS, HI, MI, PL, AL, NV
</td>
</tr>

<tr>
<td>op1_source</td>
<td>t_alu_op1_source</td>
<td>
Source for op1. One of A or ACC.
</td>
</tr>

<tr>
<td>op2_source</td>
<td>t_alu_op2_source</td>
<td>
Source for op2. One of B, ACC or SHF
</td>
</tr>

<tr>
<td>alu_op_conditional</td>
<td>bit</td>
<td>
Asserted if the ALU operation is conditional.
If conditional, then stored flags are used to indicate for testing the condition to be checked.
If not conditional, then the combinatorial flags are used for testing the condition to be checked.
</td>
</tr>

<tr>
<td>alu_write_accumulator</td>
<td>bit</td>
<td>
Assert if the accumulator should be written if the instruction is not conditional (<i>alu_op_conditional</i> is clear) or if the condition is met.
</td>
</tr>

<tr>
<td>alu_write_flags</td>
<td>bit</td>
<td>
Assert if the flags should be written if the instruction is not conditional (<i>alu_op_conditional</i> is clear) or if the condition is met.
</td>
</tr>

</table>

<?php page_subsection( "outputs", "Outputs" ); ?>

<p>
The ALU stage will also generate outputs: again, 'ports' from the ALU stage
</p>

<table border=1>
<tr>
<th>Port</th>
<th>Type</th>
<th>Details</th>
</tr>

<tr>
<td>alu_condition_met</td>
<td>bit</td>
<td>
Asserted if the condition requested (indicated by the <i>condition</i> input and the <i>alu_op_conditional</i> input)
is met.
</td>
</tr>

</table>

<?php
page_ep();

include "${toplevel}web_assist/web_footer.php"; ?>

