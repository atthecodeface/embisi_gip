<?php

include "web_locals.php";
include "${toplevel}web_assist/web_globals.php";

site_set_location($site_location);

$page_title = "GIP Documentation";

include "${toplevel}web_assist/web_header.php";

page_header( "ARM Emulation Microkernel" );

page_sp();
?>

<?php page_section( "linux", "Linux operation" ); ?>

<p>

Linux supports a preempting kernel. In this model hardware interrupts
can occur at any point in time, including during system calls; system calls may then be preempted by the interrupt routines.

<p>

It is worth examining the aim of the kernel architecture to determine
how to manage hardware interrupts and system calls from a processor
emulation perspective. This document does this in more detail in the sections below, but summarized first.

<p>

<?php page_subsection( "summary", "Summary of operation" ); ?>

The processor modes in the ARM are used for four purposes (other than virtual memory, which the GIP does not support):

<dl>

<dt>SWI call for system call from user application

<dd>Preserve return address, flags and registers on applications SVC stack, get call number, call system call, recover registers except r0 (return value from system call), return to user code

<dt>Hardware interrupt during user application

<dd>Save return address, flags and registers on applications SVC stack, call interrupt handler code, recover all registers, return to user code

<dt>Hardware interrupt during system call

<dd>Save return address, flags and registers on applications SVC stack, call interrupt handler code, recover all registers, return to system code

<dt>Hardware interrupt simultaneous with system call entry/exit

<dd>This is effectively a hardware interrupt during a system call, but it must be emulated with care: this is the only mechanism that uses three ARM processor moded simultaneously.

</dl>

<?php page_subsection( "irq_code", "Linux ARM IRQ code" ); ?>

<p>

 When a
hardware interrupt occurs (in the ARM port of Linux) the code in
arch/arm/kernel/entry-armv.S is called, in particular the routine
vector_IRQ. The code operates in the following manner:

<ol>

<li>
The program counter that was interrupted (kept in lr_IRQ) is saved in a global location (__temp_irq).

<li>The processor mode and flags that was interrupted (which must be SVC or user) is saved in a global location (__temp_irq+4).

<li>If svc mode was interrupted, then (__irq_svc) in SVC mode:

<ol>

<li>Save registers r0-r12 on the SVC stack

<li>Save registers r13_svc (before r0-r12 were pushed), r14_svc, interrupted pc, interrupted mode/flags, and a rubbish value on the stack also

<li>Get register r0 to be the interrupt number

<li>Get r1 to point to a struct pt_regs *. What is this?

<li>Handle the interrupt with asm_do_IRQ

<li>If the kernel is preemptible and preemption is needed here, then enable IRQs, schedule, and disable IRQs. Dont know how this works.

<li>Recover the interrupted mode/flags from the stack

<li>Grab r0-r12, r13_svc, r14_svc, interrupt pc from the stack, shift back to interrupted mode and its flags

</ol>

<li>If user mode was interrupted, then (__irq_usr) in SVC mode:

<ol>

<li>Save registers r0-r12 on the SVC stack

<li>Save registers interrupted pc, interrupted mode/flags, and a rubbish value on the stack also

<li>Save registers r13_usr, r14_usr, on the stack below the interrupted pc and mode/flags

<li>Get register r0 to be the interrupt number

<li>Get r1 to point to a struct pt_regs *. What is this?

<li>Handle the interrupt with asm_do_IRQ

<li>If the kernel is preemptible and preemption is needed here, then enable IRQs, schedule, and disable IRQs. Dont know how this works.

<li>Call ret_to_user, in entry-common.S, with 'why' of 0.

</ol>

</ol>

</p>

<?php page_subsection( "swi_code", "Linux ARM SWI code" ); ?>

<p>

A system call, i.e. a SWI, vectors through vector_swi, in entry-common.S. This code (obviously entered in SVC mode):

<ol>

<li>Save user registers

<ol>

<li>Save calling r0-r12 on SVC stack

<li>Save r13_usr, r14_usr on SVC stack

<li>Put r14_svc (return address) on SVC stack

<li>Put calling mode/flags (which should be user mode) on SVC stack

<li>Put R0 on SVC stack

</ol>

<li>get_scno gets system call number from the invoking SWI instruction

<li>enable interrupts (how were they disabled? Were they disabled? Why do this? I suppose they may have been disabled)

<li>If tracing syscall, then trace them; assume not, as it doesnt matter too much here

<li>Get return address for system call in lr: ret_fast_syscall

<li>Use a vector jump table to call the code, which will return to ret_fast_syscall

<li>at ret_fast_syscall, disable interrupts

<li>fast_restore_user_regs

<ol>

<li>get original calling cpsr and pc from the SVC stack

<li>get user mode r1 to lr from the SVC stack

<li>pop the stack frame

<li>Return to the calling PC and reset mode and flags

</ol>

</ol>
<?php
page_ep();

include "${toplevel}web_assist/web_footer.php"; ?>
