<?php

include "web_locals.php";
include "${toplevel}web_assist/web_globals.php";
include "local_header.php";
?>

<?php page_section( "enhancements", "Enhancements to the ARM instruction set" ); ?>

<p>

<ul>

<li>Ability to execute native 16-bit GIP instructions (no condition) (single cycle decode, single instruction)

<li>Access (for MOV and test/set, test/clear, etc) unconditionally to all 32 registers (GIP instruction?)

<li>Ability to hold off descheduling for a number of ARM instructions (GIP instruction?)

<li>Ability to deschedule the current process (GIP instruction?)

<li>Ability to do a thunking call (multicycle decode, single instruction)

<li>Ability to do a SWI (couple of moves, deschedule, assert event) (multicycle decode, single instruction)

<li>Ability to make return from interrupt happen... How?

<li>Force enable of hardware interrupts (macro)

<li>Restore interrupt enable (macro)

<li>Disable interrupts, returning previous state (macro)

</ul>

<?php page_section( "thunking_libraries", "Thunking libraries" ); ?>

We can use r17 or some other register to contain a base address of
dynamic library thunking table assists; the dynamic mapping of
registers to support this in this particular way is patentable.

<p>

Best method is to have a small table of static data pointers whose
base address is in r17 indexed by local library number, and a global
table of entry points for functions in the libraries indexed by global
entry point number whose base is in r18
We can have one instruction that loads 'r12' with 'r17, #...' and pc
with 'r18, #entryptr<<2' - we can use a quarter of the SWI instruction
decode.

<?php
page_ep();

include "${toplevel}web_assist/web_footer.php"; ?>
