<?php

include "web_locals.php";
include "${toplevel}web_assist/web_globals.php";
include "local_header.php";
?>

<?php page_section( "flushing", "Pipeline flushing" ); ?>

<p>

Pipeline flushing is used when an ARM instruction stream is being
emulated where the predicted flow of instructions turns out to be
incorrect, and a different flow is required. A pipeline flush must
always be accompanied, in ARM emulation, with a corresponding write to
the program counter from the end of the pipeline. The order of the
write to the PC and the flush may vary depending on the contents of
the pipeline (in particular a load may complete after a following ALU
operation with flush), so the decoder must keep track of the flush and
write, and pair them up.

<p>

The actual operations that may invoke a flush are as follows:

<table class=data border=1>

<tr>

<th>Type</th>
<th>Example</th>
<th>Comment</th>

</tr>

<tr>

<th>ALU immediate</th>

<td>MOV PC, #5</td>

<td>Converts to a single internal instruction; flush will occur prior to write of PC; if conditional, either neither will occur or both will</td>

</tr>


<tr>

<th>ALU reg, reg</th>

<td>MOV PC, lr</td>

<td>Converts to a single internal instruction; flush will occur prior to write of PC; if conditional, either neither will occur or both will</td>

</tr>


<tr>

<th>ALU reg, reg, SHF #s</th>

<td>ADD PC, r0, r1, LSL #2</td>

<td>Converts to two internal instructions, second of which writes PC and has flush; flush will occur prior to write of PC; if conditional, either neither will occur or both will</td>

</tr>


<tr>

<th>ALU reg, reg, SHF reg</th>

<td>ADD PC, r0, r1, LSL r2</td>

<td>Converts to two internal instructions, second of which writes PC and has flush; flush will occur prior to write of PC; if conditional, either neither will occur or both will</td>

</tr>


<tr>

<th>Unconditional branch</th>

<td>B 0x100</td>

<td>No internal instructions; does not count as a write of the PC. There will be no flush, and no write of the PC.</td>

</tr>


<tr>

<th>Conditional forward branch</th>

<td>BEQ 0x100</td>

<td>Converts to a conditional internal instruction with flush that writes PC; if condition is met then flush will occur prior to write of PC, else neither will occur.</td>

</tr>


<tr>

<th>Conditional backward branch</th>

<td>BEQ 0x100</td>

<td>Converts to a conditional internal instruction with flush that writes PC; if condition is met then flush will occur prior to write of PC, else neither will occur.</td>

</tr>


<tr>

<th>Call subroutine</th>

<td>BL 0x100</td>

<td>Converts to a single instruction that does not effect the PC, and does not have flush.</td>

</tr>


<tr>

<th>Conditional subroutine call</th>

<td>BLVS 0x100</td>

<td>Converts to two instructions; the first has flush and writes the
PC, and so will issue a flush prior to the write of the PC; the second
will either be flushed by the first instruction being executed, or it
will execute (with no flush and no PC write), and it does not flush
nor write the PC itself, so neither flush nor PC write will
occur.</td>

</tr>


<tr>

<th>Load PC, preindex no writeback reg plus unshifted</th>

<td>LDRMI PC, [R0, #4]</td>

<td>Converts to a single internal instruction with flush and writing of PC; if condition is met then flush will occur prior to write of PC, else neither will occur.</td>

</tr>


<tr>

<th>Load PC, preindex no writeback reg plus shifted</th>

<td>LDRMI PC, [R0, R1, LSL #2]</td>

<td>Converts to two internal instructions, second of which has flush and writing of PC; if condition is met then flush will occur prior to write of PC, else neither will occur.</td>

</tr>


<tr>

<th>Load PC, preindex, writeback reg plus unshifted</th>

<td>LDRMI PC, [R0, #4]!</td>

<td>Converts to two internal instructions, second of which has flush and writing of PC; if condition is met then flush will occur prior to write of PC, else neither will occur.</td>

</tr>


<tr>

<th>Load PC, preindex, writeback reg plus shifted</th>

<td>LDRMI PC, [R0, R1, LSL #2]!</td>

<td>Converts to three internal instructions, third of which has flush and writing of PC; if condition is met then flush will occur prior to write of PC, else neither will occur.</td>

</tr>


<tr>

<th>Load PC, postindex, reg plus unshifted</th>

<td>LDRMI PC, [R0], #4]</td>

<td>Converts to two internal instructions, first of which writes PC, second of which has flush; if condition is met then flush and PC write will occur, but in arbitrary order.</td>

</tr>


<tr>

<th>Load PC, postindex, reg plus shifted</th>

<td>LDRMI PC, [R0], R1, LSL #2</td>

<td>Converts to three internal instructions, second of which writes PC, third of which has flush; if condition is met then flush and PC write will occur, but in arbitrary order.</td>

</tr>


<tr>

<th>Single stores</th>

<td>STR R0, [PC, #1000]</td>

<td>Stores are not permitted (by ARM) to effect the PC, so do not write the PC and do not have flush</td>

</tr>


<tr>

<th>Block loads without writeback</th>

<td>LDMVCIA R13, {R0, R2, R4, PC}</td>

<td>Converts to a number of internal instructions, the last of which has flush and writes the PC. If the condition is met then flush will occur prior to write of PC.</td></td>

</tr>


<tr>

<th>Block loads with writeback</th>

<td>LDMVCIA R13!, {R0, R2, R4, PC}</td>

<td>Converts to a number of internal instructions, the last but one of which writes the PC, and the last of which has flush. If the condition is met then flush and PC write will occur, but in arbitrary order.</td></td>

</tr>


<tr>

<th>Block stores</th>

<td>STMDB R13!, {R0, R2, R4, PC}</td>

<td>Stores are not permitted (by ARM) to effect the PC, so do not write the PC and do not have flush</td>

</tr>



</table>

</p>

<p>
<?php
page_ep();

include "${toplevel}web_assist/web_footer.php"; ?>

