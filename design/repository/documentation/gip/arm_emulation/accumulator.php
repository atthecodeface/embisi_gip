<?php

include "web_locals.php";
include "${toplevel}web_assist/web_globals.php";
include "local_header.php";
?>

<?php page_section( "accumulator", "Use of the accumulator" ); ?>

<p>

The accumulator is used for two purposes in the ARM emulation system. Firstly it
is used to create intermediate results for instructions, such as addresses of loads and stores, 
without requiring an actual register. Secondly it is used for optimization of data processing,
in effect as a forwarding path within the ALU.

</p>

<?php page_section( "alu_forwarding", "ALU forwarding" ); ?>

<p>

The accumulator in the internal GIP pipeline is utilized to enhance the
performance of ARM emulation. Simply put, it maintains a local copy of
a single ARM register, and the register number that it contains is tracked
by the ARM emulation hardware unit. Then, instead of issuing internal instructions which
use the accumulator for a register field, the internal instruction may use the accumulator
instead. Note that the value in the accumulator is very volatile, and may be used by
just a few instructions, and will be corrupted by many instructions, so the net
usage is not likely to be more than 20% of instructions. However, it may only
achieve a performance benefit in back-to-back instructions, as
with more spacing the register value is likely to be ready in the register file path, and the
'forwarding' path through the accumulator does not save cycles.

</p>

<?php page_subsection( "tracking", "Tracking the accumulator" ); ?>

The ARM emulation hardware maintains a 4-bit value which contains all ones (15) if the accumulator
does not contain a useful register, otherwise it contains the register number that the accumulator holds. This value
is updated every time an instruction is inserted into the pipeline. If an instruction requires a register
whose value is in the accumulator (i.e. if the register number matches the 4-bit value) then the accumulator can be
used as the source of the value instead of the register number itself. Note that the key word is 'can', and not shall. Some
instructions do not use the accumulator as a source; this simplifies the logic and reduces corner cases.

<br>

Note that the concept of the accumulator 'holding' a value is not effected by the pipeline; the emulation
hardware ignores the pipeline entirely, and just feeds instructions in at the start of the pipeline, so
'holds' relates to what the accumulator will hold when the instruction just being placed into the pipeline actually is 
executed. So, following the insertion of an 'IADDA' instruction into the accumulator will be though to hold
the result of the addition, even though the actual instruction is executed some cycles later.

<?php page_subsection( "setting", "Setting the accumulator" ); ?>

The accumulator can only be set by ALU instructions; that is its function. Mirroring this, it is explicitly set
(for ALU forwarding) only by ARM data processing instructions that have a result (i.e. ADD, ADC, SUB, SBC, RSB, RSC, AND, ORR, XOR, BIC, MOV, MVN).
A conditional instruction, as it may or may not be executed, <em>does not</em> set the accumulator.
Any internal instruction that is inserted into the pipeline that does set the accumulator from a data processing instruction
marks the accumulator as valid containing the target register. Any other internal instruction that is inserted into the
pipeline marks the accumulator as invalid.

<p>
<?php
page_ep();

include "${toplevel}web_assist/web_footer.php"; ?>

