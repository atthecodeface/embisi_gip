<?php

include "web_locals.php";
include "${toplevel}web_assist/web_globals.php";
include "local_header.php";
?>

<?php page_section( "prefetch", "Prefetching in ARM emulation" ); ?>

<p>

The ARM emulation mode is designed to emulate ARM instructions at
around 1.5 clocks per instruction for data processing, loads and
stores, with obviously higher CPI for bulk transfers and
multiplies. It is important at these rates to keep the instruction
pipeline fed. This is particularly important as the distance to the
main ROM on a GIP system is many cycles (about ten), and there is no
level 1 cache. The prefetch unit performs speculative fetch of the
'next' instruction line to be used; this means that when instruction
at address 'n' is executed the prefetch unit will speculatively fetch
'n' plus one line, and as each line is 8 instructions this means
'n'+8. With 10 cycles of latency, instruction 'n'+8 should be ready in
about 10 cycles, whereas at 1.5 CPI it will be needed in about 12
cycles, so all is well. However, when a branch is taken there will be
a long penalty; unconditional branches will see a 10 cycle penalty,
for example. Compare this to the ARM, though, where the branch is no
detected until the execute stage (i.e. 2 cycles later), then the
penalty is slightly less, but not considerably. The worst effect,
though, is on returning from a branch, as this cannot be concretely
evaluated until the register file write stage, which would mean a
delay of 12 cycles. To alleviate this subroutine calls are
speculatively detected and 'stacked', and subroutine returns
speculatively fetch the return address from the top of the 'stack'. The
'stack' is a local 3-deep 'stack' that is pushed on every 'bl'
instruction handled in to the pipeline, and popped on every ldm of the
pc and mov(s) pc, lr. Question: when it gets out of sync, can we detect this quickly? When a RFW of the PC does not match the last speculatively fetched address, maybe? Then pop 2?

<p>

The actual performance comparison desired is with an ARM9 system with
4kB of shared instruction/data cache; the GIP plus its data and
instruction subsystems should achieve about half of the performance of
such a system, but supply full symmetric shared memory amongst
multiple CPUs, with coprocessor for performance enhancement.

</p>

<?php
page_ep();

include "${toplevel}web_assist/web_footer.php"; ?>

