<?php

include "web_locals.php";
include "${toplevel}web_assist/web_globals.php";
include "local_header.php";
?>

<?php page_section( "branch", "Branch instructions" ); ?>

<p>

ARM branch instructions may either be direct branches or subroutine
calls using a branch with a link store to pass a return address to the
subroutine. Additionally branches may be conditional or not; this
leads to five basic cases of branches in emulation:

<ul>

<li>
Guaranteed branch

<li>
Guaranteed branch with link

<li>
Predicted conditional branch (branch where condition is predicted to be true)

<li>
Unpredicted conditional branch (branch where condition is predicted to be false)

<li>
Conditional branch with link (all conditional branches with link are predicted to be taken)

</ul>

Note the use of 'guaranteed', 'predicted' and 'unpredicted'. An
unconditional branch is guaranteed to occur; a conditional branch may
be predicted correctly ('predicted') or incorrectly ('unpredicted').

</p>

<?php page_section( "guaranteed", "Guaranteed branch instructions" ); ?>

<p>

A guaranteed branch instruction is an unconditional branch, or a
conditional branch whose condition is satisfied and where the pipeline
(up to the ALU stage) does not contain instructions that can effect
the condition codes.

</p>

<p>

Guaranteed branches are emulated without any internal
instructions. The ARM emulation decoder implements a branch target
address calculation, and the program counter is updated with the
target address.

</p>

<?php page_section( "guaranteed_link", "Guaranteed branch with link instructions" ); ?>

<p>

A guaranteed branch with link instruction is an unconditional branch with
link or a conditional branch with link whose condition is guaranteed to be satisfied; this only occurs when the
pipeline (up to the ALU stage) contains no instructions that can effect
the condition codes.

</p>

<p>

Guaranteed branches with link are emulated by inserting one internal
instructions in to the pipeline, to set the link
register to the current PC+8-4. The program counter is also
updated with the branch target address.

</p>

<?php page_section( "predicted", "Predicted branch instructions" ); ?>

<p>

A predicted branch instruction is a conditional branch whose condition
may not be satisfied; this only occurs when the pipeline (up to the
ALU stage) contains instructions that can effect the condition
codes. The branch is predicted to occur, if it is a backward branch. A
forward branch is not predicted.

</p>

<p>

Predicted branches are emulated by inserting an internal instruction
in to the pipeline with the reverse condition to that in the
instruction. The program counter is changed to the calculated branch
target address, but the current program counter+8 is given in the
internal instruction. If the internal instruction is executed then it
will cause a pipeline flush and change of program counter to the given
current program counter+8 minus 4, i.e. the address of the next
instruction to actually execute.

</p>

<?php page_section( "unpredicted", "Unpredicted branch instructions" ); ?>

<p>

An unpredicted branch instruction is a forward conditional branch whose condition
may not be satisfied; this only occurs when the pipeline (up to the
ALU stage) contains instructions that can effect the condition
codes.

</p>

<p>

Unpredicted branches are emulated by inserting an internal instruction
in to the pipeline with the condition in the instruction to set the program counter to the target of the branch and flush the pipeline.
If the internal instruction is executed then it
will cause a pipeline flush and change of program counter to the branch target.

</p>

<?php page_section( "conditional_link", "Conditional branch with link instructions" ); ?>

<p>

A conditional branch with link instruction is a conditional branch with
link whose condition may not be satisfied; this only occurs when the
pipeline (up to the ALU stage) contains instructions that can effect
the condition codes. Such conditional branches are always predicted to
occur, whether they are forwards or backwards.

</p>

<p>

Conditional branches with link are emulated by inserting two internal
instructions in to the pipeline; one with the reverse condition of the
instruction with flush to force a branch to PC+8-4 if the condition is
not met, and the other with the current condition to set the link
register on a correctly predicted branch. The program counter is also
updated with the branch target address.

</p>

<?php page_section( "emulation_details", "Emulation details" );

arm_emulation_table_start();

arm_emulation_table_instruction( "B {offset}", "", "Guaranteed branch", "Changes PC to PC+8+offset" );
arm_emulation_table_instruction( "B[CC] {offset}", "", "CC will be met<br>Guaranteed branch", "Changes PC to PC+8+offset" );
arm_emulation_table_instruction( "B[CC] {negative offset}", "SUB{!CC}F PC, #4 -> PC", "CC may not be met<br>Predicted branch", "Changes PC to PC+8+offset<br>If mispredicted then instruction will execute and reset PC to the correct path" );
arm_emulation_table_instruction( "B[CC] {positive offset}", "MOV{CC}F #target -> PC", "CC may not be met<br>Unpredicted branch", "If mispredicted then instruction will execute and set PC to branch target" );
arm_emulation_table_instruction( "BL {offset}", "SUB PC, #4 -> R14", "Guaranteed branch with link", "Changes PC to PC+8+offset" );
arm_emulation_table_instruction( "BL[CC] {offset}", "SUB PC, #4 -> R14", "CC will be met<br>Guaranteed branch with link", "Changes PC to PC+8+offset" );
arm_emulation_table_instruction( "BL[CC] {offset}", "SUB{!CC}F PC, #4 -> PC<br>SUB PC, #4 -> R14", "CC may not be met<br>Conditional branch with link", "Changes PC to PC+8+offset<br>If mispredicted then instruction will execute and reset PC to the correct path, and R14 will not be written" );

arm_emulation_table_end();

?>
<?php
page_ep();

include "${toplevel}web_assist/web_footer.php"; ?>

