<?php

include "web_locals.php";
include "${toplevel}web_assist/web_globals.php";

site_set_location($site_location);

$page_title = "GIP Documentation";

include "${toplevel}web_assist/web_header.php";

page_header( "ARM Emulation Microkernel" );

page_sp();
?>

<?php page_section( "communication", "Communication primitives" ); ?>

These communication primitives manipulate two basic structures: the
ARM mode code thread, and a local SRAM data block that contains the
ARM 'state', basically the banked registers (user mode
                                             stack pointer (r13) and link register (r14), system stack pointer, interrupted flags (zcnv), and interrupted PC (r15)).

<p>

For the interthread management to work the registers R0-R15 must be
the same for the microkernel and the ARM mode code thread, and they
cannot be effected by any of the other threads running on the GIP.

<table border=1>

<tr><th>State</th><th>Name</th><th>Description</th></tr>

<tr><th>User R13</th>
<th>StoredUserSP</th>
<td>ARM register R13_USR; written on entry to a system call and on handling an interrupt. Read by microkernel on return from system call or interrupt to user mode.
</td>
</tr>

<tr><th>System R13</th>
<th>StoredSystemSP</th>
<td>ARM register R13_SVC; written on exit from a system call. Read by microkernel on entry to a system call or handling an interrupt from user mode.
</td>
</tr>

<tr><th>User R14</th>
<th>StoredUserLink</th>
<td>ARM register R14_USR; written on entry to a system call and on handling an interrupt. Read by microkernel on return from system call or interrupt to user mode.
</td>
</tr>

<tr><th>Interrupted PC</th>
<th>StoredInterruptedPC</th>
<td>ARM register R14_IRQ; written on handling an interrupt. Read by microkernel on return from interrupt to user mode or system mode.
</td>
</tr>

<tr><th>Interrupted Flags</th>
<th>StoredInterruptedFlags</th>
<td>Vague approximation to ARM register SPSR; written on handling an interrupt. Read by microkernel on return from interrupt to user mode or system mode.
</td>
</tr>

</table>

<p>

In order to schedule the microkernel thread, one of the microkernels
semaphores must be set by some primitives. This semaphore is actually
configurable in the GIP, but the following table assumes the
microkernel is thread 1, and that its semaphore 0 (i.e. global
semaphore 4) is used for the purposes of scheduling. Additionally its
semaphore 1 is used as the interrupt enable semaphore. Similarly the
ARM mode code thread is thread 0 (low priority), and its semaphore 0
is used to wake it up (global sempahore 0).

<br>

Semaphore 6 (microkernel semaphore 2) is used to indicate a hardware
interrupt is pending. It is set by hardware interrupt routines, and
cleared by the microkernel. Is it used for scheduling the microkernel.

<table border=1 class=data>

<tr>

<th>Primitive</th>

<td>Pertinent state</td>

<td>GIP operation</td>

<td>Microkernel operation</td>

<td>Notes</td>

</tr>


<tr>
<th>System call
</th>

<td>Microkernel must be in 'user' mode
</td>

<td>

ARM mode code thread executes SWI instruction with 'full handling' bit set and system call number in bits 0 through 9

<br>

Deschedules ARM mode code thread

<br>

Stores system call number in a useful register

<br>

Sets R15 to be the next address that would have been executed

<br>

Indicates the microkernel to be scheduled (sets semaphore 4)

</td>

<td>

Clears interrupt enable semaphore (semaphore 5)

<br>

Clears microkernel invoking semaphore (semaphore 4)

<br>

Save ARM R0 in StoredUserR0

<br>

Save ARM R13 in StoredUserSP

<br>

Save ARM R14 in StoredUserLink

<br>

Move R15 to R14

<br>

Read R13 (stack pointer) from StoredSystemSP

<br>

Move system call number to R0

<br>

Change microkernel state to 'system' mode

<br>

Change ARM mode code thread PC to be the SWI vectoring routine

<br>

Enable ARM mode code thread

<br>

Deschedule microkernel

</td>

<td>

SWI vector despatch routine should enable interrupts when it knows
that the StoredUser registers are no longer required - they should
probably be pushed on to the system stack by the vectoring routine.

<br>

The SWI vectoring routine is entered with R0 containing the system
call number, with R13 set to the system stack, and R14 set to the
restart address of the calling function. The original R0 is in
StoredUserR0. The user stack pointer is in StoredUserSP.

<br>

The SWI vectoring routine can return to the calling function with a return value in R0, by invoking the ReturnFromSystemCall primitive.

</td>

</tr>


<tr>
<th>Return from system call
</th>

<td>Microkernel must be in 'system' mode</td>

<td>

ARM mode code thread executes SWI instruction with 'full handling' bit clear and bits 0 through 9 indicating ReturnFromSystemCall

<br>

Deschedules ARM mode code thread

<br>

Indicates the microkernel to be scheduled

</td>

<td>

Clears microkernel invoking semaphore (semaphore 4)

<br>

Change ARM mode code thread PC to be value of R14

<br>

Write ARM R13 to StoredSystemSP

<br>

Read ARM R13 from StoredUserSP

<br>

Read ARM R14 from StoredUserLink

<br>

Sets interrupt enable semaphore (semaphore 5)

<br>

Change microkernel state to 'user' mode

<br>

Enable ARM mode code thread

<br>

Deschedule microkernel

</td>

<td>

</td>

</tr>


<tr>
<th>Take hardware interrupt
</th>

<td>Interrupts enabled, microkernel in 'user' mode</td>

<td>

Hardware interrupt thread sets its bit in interrupt pending register

<br>

Hardware interrupt thread sets interrupt pending semaphore (semaphore 6)

<br>

Hardware interrupt thread descehdules

</td>

<td>

Clear interrupt enable semaphore (semaphore 5)

<br>

Saves registers R13, R14 in StoredUserSP, StoredUserLink

<br>

Saves interrupted flags in StoredInterruptedFlags

<br>

Saves interrupted PC in StoredInterruptedPC

<br>

Reads ARM R13 from StoredSystemSP

<br>

Gets R0 to be the hardware interrupt number

<br>

Change microkernel state to 'system' mode

<br>

Change ARM mode code thread PC to be the user mode interrupt handler

<br>

Enable ARM mode code thread

<br>

Deschedule microkernel

</td>

<td>

User mode interrupt handler responsible for reenabling interrupts after appropriate preservation of state, if reentrancy is supported.

</td>

</tr>


<tr>
<th>Take hardware interrupt
</th>

<td>Interrupts enabled, microkernel in 'system' mode</td>

<td>

Hardware interrupt thread sets its bit in interrupt pending register

<br>

Hardware interrupt thread sets interrupt pending semaphore (semaphore 6)

<br>

Hardware interrupt thread descehdules

</td>

<td>

Clear interrupt enable semaphore (semaphore 5)

<br>

Saves interrupted flags in StoredInterruptedFlags

<br>

Saves interrupted PC in StoredInterruptedPC

<br>

Gets R0 to be the hardware interrupt number

<br>

Change ARM mode code thread PC to be the system mode interrupt handler

<br>

Enable ARM mode code thread

<br>

Deschedule microkernel

</td>

<td>

System mode interrupt handler responsible for saving R0 through R14

</td>

</tr>


<tr>
<th>Return from interrupt to user mode
</th>

<td></td>

<td>

ARM mode code thread executes SWI instruction with 'full handling' bit clear and bits 0 through 9 indicating ReturnFromInterruptToUser

<br>

Deschedules ARM mode code thread

<br>

Indicates the microkernel to be scheduled

</td>

<td>

Clears microkernel invoking semaphore (semaphore 4)

<br>

Recover ARM R13, R14 from StoredUserSP, StoredUserLink

<br>

Change microkernel state to 'system' mode

<br>

Change ARM mode code thread PC to be the recovered PC.

<br>

Sets interrupt enable semaphore (semaphore 5)

<br>

Change microkernel state to 'user' mode

<br>

Enable ARM mode code thread

<br>

Deschedule microkernel

</td>

<td>

Microkernel does not write StoredSystemSP - this is generally unnecessary as it has not changed. If it has it is the responsibility of the caller to set it appropriately.

</td>

</tr>


<tr>
<th>Return from interrupt to system mode
</th>

<td>Microkernel in 'system' mode</td>

<td>

ARM mode code thread executes SWI instruction with 'full handling' bit clear and bits 0 through 9 indicating ReturnFromInterruptToSystem

<br>

Deschedules ARM mode code thread

<br>

Indicates the microkernel to be scheduled

</td>

<td>

Clears microkernel invoking semaphore (semaphore 4)

<br>

Change ARM mode code thread flags to be StoredInterruptedFlags

<br>

Change ARM mode code thread PC to be StoredInterruptedPC

<br>

Enable ARM mode code thread

<br>

Deschedule microkernel

</td>

<td>

The calling routine is responsible for restoring saved values of R0 through R12 and R14. R13 is the system stack, and it should be unchanged from when the interrupt occurred.

</td>

</tr>



</table>

<?php
page_ep();

include "${toplevel}web_assist/web_footer.php"; ?>
