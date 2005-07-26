<?php page_section( "arm_emulation" , "ARM emulation" ); ?>

<p>
The ARM instruction classes in general are as follows:
</p>

<table border=1>
<tr>
<th>Class</th>
<th>Description</th>
<th>ALU note</th>
</tr>

<tr>
<th>Data processing</th>
<td>ALU, Shift, and combined instructions</td>
<td>Varied emulation issues; see below</td>
</tr>

<tr>
<th>Multiply</th>
<td>Multiply and multiply accumulate</td>
<td>Utilizes INIT an MULST</td>
</tr>

<tr>
<th>Single Data Swap</th>
<td><i>Not supported</i></td>
<td>No ALU issues</td>
</tr>

<tr>
<th>Single Data Transfer</th>
<td>Load or store from register plus offset, possibly shifted</td>
<td>Access and/or writeback address calculation</td>
</tr>

<tr>
<th>Undefined</th>
<td><i>Not supported</i></td>
<td>No ALU issues</td>
</tr>

<tr>
<th>Block data transfer</th>
<td>Load or Store multiple</td>
<td>Base and writeback address calculation</td>
</tr>

<tr>
<th>Branch</th>
<td>Branch with or without link</td>
<td>Write path for link register in BL</td>
</tr>

<tr>
<th>Coproc Data Transfer</th>
<td><i>Not supported</i></td>
<td>No ALU issues</td>
</tr>

<tr>
<th>Coproc Data Operation</th>
<td><i>Not supported</i></td>
<td>No ALU issues</td>
</tr>

<tr>
<th>Coproc Register Transfer</th>
<td><i>Not supported</i></td>
<td>No ALU issues</td>
</tr>

<tr>
<th>Software interrupt</th>
<td>Emulated trap</td>
<td>No ALU issues</td>
</tr>

</table>

<p>

For ARM emulation and the shifter/ALU we are concerned with the data
processing, multiply, load/store, and branch ARM instructions. They can be
emulated as follows:

<dl>

<dt>ADD/ADC/SUB/SBC/RSB/RSC{cond}[S] Rd, Rn, Rm (Rd not PC)</dt>
<dd>These map to a single conditional ALU operation, with Op1=Rn, Op2=Rm, and 
the N, Z, V, C flags being set based on the result
 if the condition presented is already passed by the current flags. The 
accumulator is set with the result if the condition passed.
</dd>

<dt>AND/ORR/BIC/EOR{cond}[S] Rd, Rn, Rm (Rd not PC)</dt>
<dd>These map to a single conditional ALU operation, with Op1=Rn, Op2=Rm, and
the N, Z flags being set based on the result if the condition
presented is already passed by the current flags. The accumulator is set with 
the result if the condition passed.
</dd>

<dt>MOV/MVN{cond}[S] Rd, Rm (Rd not PC)</dt>
<dd>These map to a single conditional ALU operation, with Op1=Rn, Op2=Rm, and 
the N, Z flags being set based on the result if
the condition presented is already passed by the current flags. The accumulator
is set with the result if the condition passed.
</dd>

<dt>CMP/CMN{cond} Rn, Rm</dt>
<dd>These map to a single conditional ALU operation, (SUB or ADD respectively),
with Op1=Rn, Op2=Rm, and the N, Z, V, C flags being set based on the result
 if the condition presented is already passed by the current flags.
</dd>

<dt>TST/TEQ{cond} Rn, Rm</dt>
<dd>These map to a single conditional ALU operation, (AND or XOR respectively) 
with Op1=Rn, Op2=Rm, and the N, Z flags being set based on the result if the
condition
presented is already passed by the current flags.
</dd>

<dt>ADD/ADC/SUB/SBC/RSB/RSC{cond}[S] Rd, Rn, #I (Rd not PC)</dt>
<dd>These map to a single conditional ALU operation, with Op1=Rn, Op2=I, and the 
N, Z, V, C flags being set based on the result
 if the condition presented is already passed by the current flags. The
accumulator is set with the result if the condition passed.
</dd>

<dt>AND/ORR/BIC/EOR{cond}[S] Rd, Rn, #I (Rd not PC)</dt>
<dd>These map to a single conditional ALU operation, with Op1=Rn, Op2=I and the
N, Z flags being set based on the result if the condition
presented is already passed by the current flags. The accumulator is set with
the result if the condition passed.
</dd>

<dt>MOV/MVN{cond}[S] Rd, #I (Rd not PC)</dt>
<dd>These map to a single conditional ALU operation, with Op1=Rn, Op2=I, and the
N, Z flags being set based on the result if
the condition presented is already passed by the current flags. The accumulator
is set with the result if the condition passed.
</dd>

<dt>CMP/CMN{cond} Rn, #I</dt>
<dd>These map to a single conditional ALU operation, (SUB or ADD respectively),
with Op1=Rn, Op2=I, and the N, Z, V, C flags being set based on the result
 if the condition presented is already passed by the current flags.
</dd>

<dt>TST/TEQ{cond} Rn, #I</dt>
<dd>These map to a single conditional ALU operation, (AND or XOR respectively)
with Op1=Rn, Op2=I, and the N, Z flags being set based on the result if the
condition
presented is already passed by the current flags.
</dd>

<dt>ADD/ADC/SUB/SBC/RSB/RSC{cond}[S] Rd, Rn, Rm, SHIFT #S/Rs (Rd not PC)</dt>
<dd>These map to two conditional operations, a shift and an ALU operation. The
first operation is Op1=S or Rs, Op2=Rm, and the shifter produces a result which
is always stored in SHF and a carry that is stored in the shifter carry; the
second operation is Op1=Rn, Op2=SHF, and the N, Z, V, C flags are set based on
the result  if the condition presented is already passed by the current flags.
The accumulator is set with the result if the condition passed.
</dd>

<dt>AND/ORR/BIC/EOR{cond}[S] Rd, Rn, Rm, SHIFT #S/Rs (Rd not PC)</dt>
<dd>These map to two conditional operations, a shift and an ALU operation. The
first operation is Op1=S or Rs, Op2=Rm, and the shifter produces a result which
is always stored in SHF and a carry that is stored in the shifter carry; the
second operation is Op1=Rn, Op2=SHF, and the N, Z flags being set based on the
result if the condition
presented is already passed by the current flags. The accumulator is set with
the result if the condition passed.
</dd>

<dt>MOV/MVN{cond}[S] Rd, Rm, SHIFT #S/Rs (Rd not PC)</dt>
<dd>These map to two conditional operations, a shift and an ALU operation. The
first operation is Op1=S or Rs, Op2=Rm, and the shifter produces a result which
is always stored in SHF and a carry that is stored in the shifter carry; the
second operation is Op1=Rn, Op2=SHF, and the N, Z flags being set based on the
result if
the condition presented is already passed by the current flags. The accumulator
is set with the result if the condition passed.
</dd>

<dt>CMP/CMN{cond} Rn, Rm, SHIFT #S/Rs</dt>
<dd>These map to two conditional operations, a shift and an ALU operation. The
first operation is Op1=S or Rs, Op2=Rm, and the shifter produces a result which
is always stored in SHF and a carry that is stored in the shifter carry; the
second operation is Op1=Rn, Op2=SHF, (SUB or ADD respectively), and the N, Z, V,
C flags being set based on the result  if the condition presented is already
passed by the current flags.
</dd>

<dt>TST/TEQ{cond} Rn, Rm, SHIFT #S/Rs</dt>
<dd>These map to two conditional operations, a shift and an ALU operation. The
first operation is Op1=S or Rs, Op2=Rm, and the shifter produces a result which
is always stored in SHF and a carry that is stored in the shifter carry; the
second operation is Op1=Rn, Op2=SHF, (AND or XOR respectively), and the N, Z
flags being set based on the result if the condition
presented is already passed by the current flags.
</dd>

<dt>MOV{cond}[S] PC, Rm</dt>
<dd>This instruction is emulated as a two-cycle instruction, the first of which
is as a conditional MOV ALU operation, with no results written back. If the
condition passes then execution
flow will change to the result of the ALU path; if it fails then the next
instruction is performed. A speculative fetch will be performed of the address
in Rm when it is read from the
register file. The 'S' field is always ignored.
</dd>

<dt>ADD/ADC/SUB/SBC/RSB/RSC{cond}[S] PC, Rn, Rm</dt>
<dd>These instructions are emulated as multi-cycle instructions, the first of
which is a conditional ALU operation with Op1=Rn, Op2=Rm, with no results
written back. If the condition passes then execution
flow will change to the result of the ALU operation; if it fails then the next
instruction is performed after a single cycle of delay. The 'S' field is always
ignored.
</dd>

<dt>AND/ORR/BIC/EOR{cond}[S] PC, Rn, Rm</dt>
<dd>
These instructions are emulated as multi-cycle instructions, the first of
which is a conditional ALU operation with Op1=Rn, Op2=Rm, with no results
written back. If the condition passes then execution
flow will change to the result of the ALU operation; if it fails then the next
instruction is performed after a single cycle of delay. The 'S' field is always
ignored.
</dd>

<dt>MVN{cond}[S] PC, Rm</dt>
<dd>
These instructions are emulated as multi-cycle instructions, the first of
which is a conditional ALU operation with Op1=Rn, Op2=Rm, with no results
written back. If the condition passes then execution
flow will change to the result of the ALU operation; if it fails then the next
instruction is performed after a single cycle of delay. The 'S' field is always
ignored.
</dd>

<dt>ADD/ADC/SUB/SBC/RSB/RSC{cond}[S] PC, Rn, #I</dt>
<dd>
These instructions are emulated as multi-cycle instructions, the first of
which is a conditional ALU operation with Op1=Rn, Op2=I, with no results
written back. If the condition passes then execution
flow will change to the result of the ALU operation; if it fails then the next
instruction is performed after a single cycle of delay. The 'S' field is always
ignored.
</dd>

<dt>AND/ORR/BIC/EOR{cond}[S] PC, Rn, #I</dt>
<dd>
These instructions are emulated as multi-cycle instructions, the first of
which is a conditional ALU operation with Op1=Rn, Op2=I, with no results
written back. If the condition passes then execution flow will change to the
result of the ALU operation; if it fails then the next instruction is performed
after a single cycle of delay. The 'S' field is always ignored.
</dd>

<dt>MOV/MVN{cond}[S] PC, #I</dt>
<dd>
These instructions are emulated as multi-cycle instructions, the first of
which is a conditional ALU operation with Op1=Rn, Op2=I, with no results
written back. If the condition passes then execution
flow will change to the result of the ALU operation; if it fails then the next
instruction is performed after a single cycle of delay. The 'S' field is always
ignored.
</dd>

<dt>ADD/ADC/SUB/SBC/RSB/RSC{cond}[S] PC, Rn, Rm, SHIFT #I/Rs</dt>
<dd>
These instructions are emulated as multi-cycle instructions.
The first operation is a conditional shift with Op1=I or Rs, Op2=Rm, and the
shifter produces a  result which is always stored in SHF and a carry that is
stored in the shifter carry. The second operation is an ALU operation with
Op1=Rn, Op2=SHF, with no results
written back. If the first cycle condition passes then an additional
delay cycle is inserted, and execution
flow will change to the result of the ALU operation; if it fails then the next
instruction is performed.The 'S' field is always
ignored.
</dd>

<dt>AND/ORR/BIC/EOR{cond}[S] PC, Rn, Rm, SHIFT #I/Rs</dt>
<dd>
These instructions are emulated as multi-cycle instructions.
The first operation is a conditional shift with Op1=I or Rs, Op2=Rm, and the
shifter produces a  result which is always stored in SHF and a carry that is
stored in the shifter carry. The second operation is an ALU operation with
Op1=Rn, Op2=SHF, with no results
written back. If the first cycle condition passes then an additional
delay cycle is inserted, and execution
flow will change to the result of the ALU operation; if it fails then the next
instruction is performed.The 'S' field is always
ignored.
</dd>

<dt>MOV/MVN{cond}[S] PC, Rm, SHIFT #I/Rs</dt>
<dd>
These instructions are emulated as multi-cycle instructions.
The first operation is a conditional shift with Op1=I or Rs, Op2=Rm, and the
shifter produces a  result which is always stored in SHF and a carry that is
stored in the shifter carry. The second operation is an ALU operation with
Op1=Rn, Op2=SHF, with no results
written back. If the first cycle condition passes then  an additional
delay cycle is inserted, and execution
flow will change to the result of the ALU operation; if it fails then the next
instruction is performed.The 'S' field is always
ignored.
</dd>

<dt>MUL{cond}[S] Rd, Rm, Rs (Rd may not be PC: ARM restriction)</dt>
<dt>MLA{cond}[S] Rd, Rm, Rs, Rn (Rd may not be PC: ARM restriction)</dt>
<dd>
These instructions are emulated as multi-cycle instructions.
The first operation is a conditional MULIN with Op1=Rn or 0 and Op2=Rm. The second and further operations
are MULST, with the A input set to Rs.
If the conditional first instruction fails then the instruction terminates early.
If the ALU indicates that the termination condition is met then one more MULST operation will take
place (with register file write details), then the multiply will complete. In any case at most 16 MULSTs will be issued,
and the last will always have register file write enabled.
If the S flag is set then the final MULST will write the Z and the N flags.
<br>
Note that due to the register forwarding structure a multiply operation must block until all its component registers are present in the register file; it cannot
use the value in the accumulator for any purpose.
</dd>

<dt>LDR|STR{cond} Rd, [Rn, #+/-I]{!}</dt>
<dt>LDR|STR{cond} Rd, [Rn, +/-Rm]{!}</dt>
<dd>
The ALU performs the address calculation for the data read (and possible writeback) by being issued an addition or subtraction operation
with Op1=Rn, Op2=I or Rm
</dd>

<dt>LDR|STR{cond} Rd, [Rn], #+/-I</dt>
<dt>LDR|STR{cond} Rd, [Rn], +/-Rm</dt>
<dd>
The ALU performs the address calculation for writeback by being issued an addition or subtraction operation
with Op1=Rn, Op2=I or Rm
</dd>

<dt>LDR|STR{cond} Rd, [Rn, +/-Rm, SHF #S]{!}</dt>
<dd>
The ALU performs two instructions; the first is a shift with Op2=Rm, Op1=S, putting the result in the SHF register.
The second is the address calculation for the data read (and possible writeback) by being issued an addition or subtraction operation
with Op1=Rn, Op2=SHF
</dd>

<dt>LDR|STR{cond} Rd, [Rn], +/-Rm, SHF #S</dt>
<dd>
The ALU performs two instructions; the first is a shift with Op2=Rm, Op1=S, putting the result in the SHF register.
The second is the address calculation for writeback by being issued an addition or subtraction operation
with Op1=Rn, Op2=SHF
</dd>

<dt>LDM|STM{cond}IB Rn[!], Rlist (Rn may not be in Rlist if writeback is specified)</dt>
<dd>
The ALU performs the base address calculation by being issued an addition operation
with Op1=Rn, Op2=4, and the writeback address with an addition operation with
Op1=Rn, Op2=4*(number of registers in Rlist)
<br>
If the PC is specified in Rlist and the operation is an LDM then more cycles are taken by the instruction; the ALU is issued with NOPs on those cycles.
</dd>

<dt>LDM|STM{cond}IA Rn[!], Rlist (Rn may not be in Rlist if writeback is specified)</dt>
<dd>
The ALU performs the base address calculation by being issued an addition operation
with Op1=Rn, Op2=0, and the writeback address with an addition operation with
Op1=Rn, Op2=4*(number of registers in Rlist)
<br>
If the PC is specified in Rlist and the operation is an LDM then more cycles are taken by the instruction; the ALU is issued with NOPs on those cycles.
</dd>

<dt>LDM|STM{cond}DB Rn[!], Rlist (Rn may not be in Rlist if writeback is specified)</dt>
<dd>
The ALU performs the base address calculation by being issued a subtraction operation
with Op1=Rn, Op2=4*(number of registers in Rlist), and the writeback address with a subtraction operation with
Op1=Rn, Op2=4*(number of registers in Rlist)
<br>
If the PC is specified in Rlist and the operation is an LDM then more cycles are taken by the instruction; the ALU is issued with NOPs on those cycles.
</dd>

<dt>LDM|STM{cond}DA Rn[!], Rlist (Rn may not be in Rlist if writeback is specified)</dt>
<dd>
The ALU performs the base address calculation by being issued a subtraction operation
with Op1=Rn, Op2=4*(number of registers in Rlist-1), and the writeback address with a subtraction operation with
Op1=Rn, Op2=4*(number of registers in Rlist)
<br>
If the PC is specified in Rlist and the operation is an LDM then more cycles are taken by the instruction; the ALU is issued with NOPs on those cycles.
</dd>

<dt>BL{cond} offset</dt>
<dd>
The ALU performs the return address calculation by being issued a subtraction operation
with Op1=PC+8, Op2=4
</dd>

</dl>

</p>

