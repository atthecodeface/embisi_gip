<?php

include "web_locals.php";
include "${toplevel}web_assist/web_globals.php";

site_set_location("gip.alu.implementation");

$page_title = "GIP Documentation";

include "${toplevel}web_assist/web_header.php";

page_header( "GIP ALU and Shifter Implementation" );

page_sp();
?>

This documentation matches version 1.9 of the gip_alu source code

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
<td>rfr_inst</td>
<td>t_gip_instruction_rf</td>
<td>
Instruction to execute in next cycle, if alu_accepting_rfr_instruction is asserted (else the ALU is blocked) <br>
Relevant fields are:<br>
valid, gip_ins_cc, gip_ins_class, gip_ins_subclass, gip_ins_rn (for acc), rm_is_imm, gip_ins_rm (for acc and shf), p_or_offset_is_shift, gip_ins_rd, s_or_stack, a, f, tag, ?k? for alu_mem_burst
</td>
</tr>

<tr>
<td>rf_read_port_0</td>
<td>t_gip_word</td>
<td>
ALU Op0 data input (32-bit word) for the instruction to execute in the next cycle
</td>
</tr>

<tr>
<td>rf_read_port_1</td>
<td>t_gip_word</td>
<td>
ALU Op1 data input (32-bit word) for the instruction to execute in the next cycle
</td>
</tr>

<tr>
<td>alu_accepting_rfr_instruction</td>
<td>bit</td>
<td>
Asserted if the ALU should accept the RFR instruction - if this is deasserted and the ALU has a valid instruction, then it should repeat that instruction <br>
Note that this means writing the flags, accumulator and shifter value should NOT occur if this is deasserted
</td>
</tr>

<tr>
<td>special_cp_trail_2</td>
<td>bit</td>
<td>
Asserted if any condition of 'cp' should use the AND of the old condition passed signal with the current condition passed signal; basically, this provides for some native GIP tricks. <br>
This is generally deasserted, and may not be used.
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
<th>Timing</th>
<th>Details</th>
</tr>

<tr>
<td>alu_accepting_rfr_instruction_always</td>
<td>bit</td>
<td>Condition evaluation</td>
<td>
Asserted ONLY if the condition for the current instruction is not met. If the current instruction is not valid, then its condition is deemd not met, and so this will be asserted<br>
This signal should be combined with other pipeline stage signals to determine if the ALU should block, driving the alu_accepting_rfr_instruction.
</td>
</tr>

<tr>
<td>alu_accepting_rfr_instruction_if_mem_does</td>
<td>bit</td>
<td>Condition evaluation</td>
<td>
Asserted ONLY if the current instruction is valid, passes its condition test, and if the instruction is a load or store that will be outputting to the memory stage.<br>
This signal should be combined with other pipeline stage signals to determine if the ALU should block, driving the alu_accepting_rfr_instruction.
</td>
</tr>

<tr>
<td>alu_accepting_rfr_instruction_if_rfw_does</td>
<td>bit</td>
<td>Condition evaluation</td>
<td>
Asserted ONLY if the current instruction is valid, passes its condition test, and if the instruction is a logical, arithmetic, shift or store that MIGHT be outputting to the memory stage.<br>
This signal should be combined with other pipeline stage signals to determine if the ALU should block, driving the alu_accepting_rfr_instruction.
</td>
</tr>

<tr>
<td>alu_rd</td>
<td>t_gip_ins_r</td>
<td>Condition evaluation</td>
<td>
The register to write in the register file, if the ALU stage is not blocked. If ALU is blocked then no write should take place.
</td>
</tr>

<tr>
<td>alu_use_shifter</td>
<td>bit</td>
<td>simple gip_ins_class decode</td>
<td>
Mux select for which ALU data output to use to write in to the register file - asserted if alu_shifter_result should be used, deasserted if alu_arith_logic_result should be used.
</td>
</tr>

<tr>
<td>alu_arith_logic_result</td>
<td>t_gip_word</td>
<td>Full ALU path</td>
<td>
Data to write to register file from the ALU stage, from logic and arithmetic instructions; only used if alu_rd indicates a register to write, 
</td>
</tr>

<tr>
<td>alu_shifter_result</td>
<td>t_gip_word</td>
<td>Full shifter path</td>
<td>
Data to write to register file from the ALU stage, from shifter instructions; only used if alu_rd indicates a register to write, 
</td>
</tr>

<tr>
<td>alu_mem_op</td>
<td>t_gip_mem_op</td>
<td>Condition evaluation and instruction decode</td>
<td>
Memory operation to perform in the next cycle if the ALU stage is not blocked - should only be used if alu_accepting_rfr_instruction is asserted, else a NOP to the memory should be assumed
</td>
</tr>

<tr>
<td>alu_mem_options</td>
<td>t_gip_mem_options</td>
<td>simple gip_ins_class decode</td>
<td>
Options for the memory operation to perform in the next cycle - includes items such as 'signed', and 'bigendian'
</td>
</tr>

<tr>
<td>alu_mem_rd</td>
<td>t_gip_ins_r</td>
<td>Condition evaluation and simple instruction decode</td>
<td>
Register to place memory read data into if the ALU stage is not blocked - should only be used if alu_accepting_rfr_instruction is asserted, else NONE should be assumed;
</td>
</tr>

<tr>
<td>alu_mem_address</td>
<td>t_gip_word</td>
<td>Mux of arithmetic result and arith operation input</td>
<td>
Address of memory operation, if one is indicated
</td>
</tr>

<tr>
<td>alu_mem_write_data</td>
<td>t_gip_word</td>
<td>Arith operation input</td>
<td>
Data for a memory write transaction, aligned to bit 0; the memory stage is responsible for placing byte and half-word writes in the correct bytelanes for the memory itself
</td>
</tr>

<tr>
<td>alu_mem_burst</td>
<td>bit[4]</td>
<td>From the instruction</td>
<td>
Size of a burst transaction remaining; if this is non-zero, then the following cycles shall be successive identical memory operations to successive memory words, and the number of such successive operations is indicated in the burst. Should be qualified by the ALU blocking indication
</td>
</tr>

<tr>
<td>gip_pipeline_flush</td>
<td>bit</td>
<td>Condition evaluation</td>
<td>
Asserted if the ALU stage has a valid instruction, being executed (i.e. its condition is met), and if that instruction has the 'f' bit asserted; this signal only occurs in the first cycle of an instruction, if it should be held due to blocking.
</td>
</tr>

<tr>
<td>gip_pipeline_tag</td>
<td>bit</td>
<td>From register</td>
<td>
Mirrors the contents of the instruction tag field for the instruction currently in the ALU stage; this can be used for whatever purposes the decode deems worthwhile.
</td>
</tr>

<tr>
<td>gip_pipeline_executing</td>
<td>bit</td>
<td>Condition evaluation</td>
<td>
This bit indicates that the ALU has a valid instruction and that instruction is being executed as its condition is met; this signal is only asserted in the first cycle of an instruction, if it should be held due to blocking.
</td>
</tr>

</table>

KNOWN BUGS

memory uses operation even if ALU is blocked
RFW uses ALU rd even if ALU is blocked
tag is probably used unqualified by executing
alu_mem_burst is never driven

<?php
page_ep();

include "${toplevel}web_assist/web_footer.php"; ?>

