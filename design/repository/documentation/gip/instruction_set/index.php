<?php

include "web_locals.php";
include "${toplevel}web_assist/web_globals.php";

site_set_location("gip.instruction_set");

$page_title = "GIP Documentation";

include "${toplevel}web_assist/web_header.php";

page_header( "GIP Internal Instruction Set" );

page_sp();
?>

<?php page_section( "overview", "Overview" ); ?>

<p>

This area describes the GIP internal instruction set, which is executed by the GIP execution pipeline on behalf of
the native 16-bit GIP instruction set decoder and the ARM 32-bit instruction emulation system.

</p>

<p>
There are very few basic classes of instruction:

<ul>

<li> Arithmetic instructions (including multiply/divide steps)

<li> Logical instructions

<li> Shift instructions

<li> Memory reads

<li> Memory writes

<li> Memory commands (prefetch, flush, and such like)

</ul>

Note that there are no flow control instructions; in fact, there is no specific program counter in the internal instruction set, and it does not issue any fetches, as that is the responsibility of the decoder or emulator feeding internal instructions to the pipeline.

</p>

<?php page_section( "instruction_encoding", "Instruction Encoding" ); ?>

<table border=1 class=data>
<tr>
<th>Field
<th>Size
<th>Description
</tr>

<tr>
<th>Class
<td>[3]
<td>Class of operation, see table below
</tr>

<tr>
<th>Subclass
<td>[4]
<td>Subclass of operation, see table below
</tr>

<tr>
<th>Options
<td>[2]
<td>Coded with class/subclass of operation; options for the operation
</tr>

<tr>
<th>CC
<td>[4]
<td>Condition under which to execute the instruction; see table below
</tr>

<tr>
<th>Rn
<td>[6]
<td>First register for use in the operation; if the top bit is a one, then the bottom 5 bits code a register source. If not then the bottom 2 bits code a source (PC input, ACC or SHF)
</tr>

<tr>
<th>Rm/Imm
<td>[33]
<td>Immediate or second register field; if the top bit is a 1, then the remaining 32 bits are an immediate value. If not, then the bottom 6 bits code a source in the same way as Rn.
</tr>

<tr>
<th>Rd
<td>[6]
<td>Six bit encoding of register destination for the operation; if the top bit is set then the result is to a register encoded in the bottom 5 bits; if the top two bits are clear then it encodes none; if the two bits are 01 then the bottom four bits encode a condition for conditional setting instructions or the PC.
</tr>

<tr>
<th>k
<td>[4]
<td>Number of loads/stores still to come in a burst; 0 for a single load/store. If a burst then the addresses must be consecutive words, and the direction will be given in the main instruction
</tr>

<tr>
<th>A
<td>[1]
<td>Set if the accumulator should be written with the ALU result
</tr>

<tr>
<th>F
<td>[1]
<td>Flush: set if the instruction should flush the pipeline behind it if it is executed, and this should be indicated to the decoder.
</tr>

</table>

<?php page_section( "class", "Instruction classes" ); ?>

<?php
encoding_start( 3, "Class" );
encoding( "Arith", "000", "Arithmetic operation (with S flag sets ZCVN)" );
encoding( "Logic", "001", "Logical operation (with S flag sets ZN, with P flag sets C)" );
encoding( "SHF", "010", "Shift, always writes P flag and SHF register" );
encoding( "Load", "100", "Load from memory" );
encoding( "Store", "101", "Store to memory" );
encoding_end();
?>

<?php page_section( "arith_subclass", "Arithmetic instruction subclasses" ); ?>

<?php

encoding_start( 4, "Subclass" );
encoding( "Add", "0000", "Add" );
encoding( "Adc", "0001", "Add with carry in from current C flag" );
encoding( "Sub", "0010", "Subtract" );
encoding( "Sbc", "0011", "Subtract with carry in from current C flag" );
encoding( "Rsb", "0100", "Reverse subtract" );
encoding( "Rsc", "0101", "Reverse subtract with carry in from current C flag" );
encoding( "Init", "1000", "Clear carry, Acc=Rm/Imm, SHF=Rn" );
encoding( "Mla", "1010", "Acc+=2/1/0/-1*Rm/Imm; B=Rm/Imm LSL 2; SHF=SHF LSR 2; P=0/1" );
encoding( "Mlb", "1011", "Acc+=2/1/0/-1*B; B=B LSL 2; SHF=SHF LSR 2; P=0/1" );
encoding( "Dva", "1100", "Acc=Acc-?Rm/Imm; SHF=SHF|?Rn; B=Rm/Imm LSR 1; A=Rn LSR 1" );
encoding( "Dvb", "1101", "Acc=Acc-?B; SHF=SHF|?A; B=Rm/Imm LSR 1; A=Rn LSR 1" );
encoding_end();

?>

<?php page_section( "logic_subclass", "Logical instruction subclasses" ); ?>

<?php

encoding_start( 3, "Subclass" );
encoding( "AND", "0000", "AND" );
encoding( "OR",  "0001", "OR" );
encoding( "XOR", "0010", "XOR" );
encoding( "ORN", "0011", "ORN" );
encoding( "BIC", "0100", "BIC" );
encoding( "MOV", "0101", "MOV" );
encoding( "MVN", "0110", "MVN" );
encoding( "ANDC", "0111", "ANDC" );
encoding( "ANDX", "1000", "ANDX" );
encoding( "XORF", "1001", "XORF" );
encoding( "XORL", "1011", "XORL" );
encoding( "BITR", "1100", "BITR" );
encoding( "BYTR", "1101", "BYTR" );
encoding_end();

?>

<?php page_section( "shift_subclass", "Shift instruction subclasses" ); ?>

<?php

encoding_start( 4, "Subclass" );
encoding( "LSL", "0000", "LSL" );
encoding( "LSR", "0001", "LSR" );
encoding( "ASR", "0010", "ASR" );
encoding( "ROR", "0011", "ROR" );
encoding( "ROR33", "0100", "Rotate right 33-bit value (33rd bit comes from carry flag)" );
encoding_end();

?>

<?php page_section( "memory_subclass", "Memory (load/store) instruction subclasses" ); ?>

encoding_start( 4, "Subclass" );
encoding( "Preindex", "1xxx", "The transaction adds/subtracts the offset to the index and uses the result as the address" );
encoding( "Postindex", "0xxx", "The transaction adds/subtracts the offset to the index in the ALU but uses the 'Rn' value as the address" );
encoding( "Sub", "x0xx", "The transaction subtracts the offset from the index" );
encoding( "Add", "x1xx", "The transaction adds the offset from the index" );
encoding( "Word", "xx00", "Perform a word access" );
encoding( "Half", "xx01", "Perform a 16-bit access" );
encoding( "Byte", "xx10", "Perform a byte access" );
encoding_end();

?>

<?php page_section( "cc", "Condition code encoding" ); ?>

<table border=1 class=data>
<tr>
<th>CC
<th>Value
<th>Meaning
<th>Flags
<th>Description
</tr>

<tr>
<th>EQ</th>
<td>0000</td>
<td>Equal</td>
<td>Z</td>
<td>Zero flag set, indicating an equality of comparison or an ALU result being zero</td>
</tr>

<tr>
<th>NE</th>
<td>0001</td>
<td>Not equal</td>
<td>!Z</td>
<td>Zero flag clear, indicating an equality of comparison or an ALU result being zero</td>
</tr>

<tr>
<th>CS</th>
<td>0010</td>
<td>Carry set</td>
<td>C</td>
<td>Carry flag set, indicating an overflow in an unsigned addition, or no borrow in an unsigned subtraction; thus it also is equivalent to an unsigned 'higher or same', or 'greater than or equal to'</td>
</tr>

<tr>
<th>CC</th>
<td>0001</td>
<td>Carry clear</td>
<td>!C</td>
<td>Carry flag clear, indicating no overflow in an unsigned addition, or a borrow in an unsigned subtraction; thus it also is equivalent to an unsigned 'less than', or 'lower than'</td>
</tr>

<tr>
<th>MI</th>
<td>0100</td>
<td>Negative</td>
<td>N</td>
<td>Negative flag set, indicating bit 31 of an ALU result is asserted; normally indicates a twos complement result is negative</td>
</tr>

<tr>
<th>PL</th>
<td>0101</td>
<td>Positive</td>
<td>!N</td>
<td>Negative flag clear, indicating bit 31 of an ALU result is deasserted; normally indicates a twos complement result is positive or zero</td>
</tr>

<tr>
<th>VS</th>
<td>0110</td>
<td>Overflow set</td>
<td>V</td>
<td>Overflow flag set, indicating a signed twos complement addition operation overflowed</td>
</tr>

<tr>
<th>VC</th>
<td>0111</td>
<td>Overflow clear</td>
<td>!V</td>
<td>Overflow flag clear, indicating a signed twos complement addition operation did not overflow</td>
</tr>

<tr>
<th>HI</th>
<td>1000</td>
<td>Higher</td>
<td>C & !Z</td>
<td>Combining CS and NE, thus indicating an unsigned 'higher than' in a comparision</td>
</tr>

<tr>
<th>LS</th>
<td>1001</td>
<td>Lower or same</td>
<td>!C | Z</td>
<td>Combining CC and EQ, thus indicating an unsigned 'lower or same' in a comparision</td>
</tr>

<tr>
<th>GE</th>
<td>1010</td>
<td>Greater than or equal</td>
<td>(!N&!V) | (N&V)</td>
<td>Indicates a signed twos complement arithmetic result is greater than or equal to zero</td>
</tr>

<tr>
<th>LT</th>
<td>1011</td>
<td>Less than</td>
<td>(!N&V) | (N&!V)</td>
<td>Indicates a signed twos complement arithmetic result is less than zero</td>
</tr>

<tr>
<th>GT</th>
<td>1100</td>
<td>Greater than</td>
<td>!Z & ((!N&!V) | (N&V)))</td>
<td>Indicates a signed twos complement arithmetic result is greater than zero</td>
</tr>

<tr>
<th>LE</th>
<td>1101</td>
<td>Less than or equal</td>
<td>Z | (!N&V) | (N&!V)</td>
<td>Indicates a signed twos complement arithmetic result is less than or equal to zero</td>
</tr>

<tr>
<th>AL</th>
<td>1110</td>
<td>Always</td>
<td>1</td>
<td>Also a blank CC, indicates an instruction is always executed, the condition is always true</td>
</tr>

<tr>
<th>CP</th>
<td>1111</td>
<td>Condition passed</td>
<td></td>
<td>If the previous instruction condition passed, then this does, else this does not</td>
</tr>

</table>

Note that 'HS' is not used as a synonym for 'CS'; this would lead to ambiguity in some halfword load instructions! The only recognized conditions for GIP code are those given above.

<?php page_section( "options", "Options" ); ?>

<table>

<tr><th>Class</th><th>Encoding</th></tr>
<tr><th>Arith</th><td>S0</td></tr>
<tr><th>Logic</th><td>SP</td></tr>
<tr><th>Shift</th><td>SP</td></tr>
<tr><th>Coproc</th><td>xx</td></tr>
<tr><th>Load</th><td>0s</td></tr>
<tr><th>Store</th><td>Os</td></tr>
</table>

<table>
<tr><th>ID</th><th>Name</th><th>Description</th></tr>

<tr>
<th>S
<td>Set flags
<td>Assert if the flags should be set by the arithmetic/logical/shift operation; for arithmetic this is ZCVN, for shift ZCN, for logical ZN.
</tr>

<tr>
<th>P
<td>Copy P flag
<td>Assert if the P flag in the shifter should be copied to the carry flag with a logical operation
</tr>

<tr>
<th>O
<td>Offset
<td>0=> offset of 1/2/4 (depending on access size), 1=> use SHF as the offset
</tr>

<tr>
<th>s
<td>Stack access
<td>1=> use stack locality for caching access, 0=> use default locality for caching access
</tr>

</table>

<?php page_section( "encoding", "Encoding" ); ?>

<table border=1 class=data>
<tr>
<th>Mnemonic</th>
<th>Class</th>
<th>Subclass</th>
<th>Opts</th>
<th>CC</th>
<th>Rd</th>
<th>A</th>
<th>F</th>
</tr>

<tr>
<th colspan=8>Arithmetic</th>
</tr>

<tr>
<th align=left>
IADD[CC][S][A][F] Rn, Rm/Imm -> Rd
</th>
<td>Arith</td>
<td>ADD</td>
<td>S0</td>
<td>CC</td>
<td>Rd</td>
<td>A</td>
<td>F</td>
</tr>

<tr>
<th align=left>
IADC[CC][S][A][F] Rn, Rm/Imm -> Rd
</th>
<td>Arith</td>
<td>ADC</td>
<td>S0</td>
<td>CC</td>
<td>Rd</td>
<td>A</td>
<td>F</td>
</tr>

<tr>
<th align=left>
ISUB[CC][S][A][F] Rn, Rm/Imm -> Rd
</th>
<td>Arith</td>
<td>SUB</td>
<td>S0</td>
<td>CC</td>
<td>Rd</td>
<td>A</td>
<td>F</td>
</tr>

<tr>
<th align=left>
ISBC[CC][S][A][F] Rn, Rm/Imm -> Rd
</th>
<td>Arith</td>
<td>SBC</td>
<td>S0</td>
<td>CC</td>
<td>Rd</td>
<td>A</td>
<td>F</td>
</tr>

<tr>
<th align=left>
IRSB[CC][S][A][F] Rn, Rm/Imm -> Rd
</th>
<td>Arith</td>
<td>RSB</td>
<td>S0</td>
<td>CC</td>
<td>Rd</td>
<td>A</td>
<td>F</td>
</tr>

<tr>
<th align=left>
IRSC[CC][S][A][F] Rn, Rm/Imm -> Rd
</th>
<td>Arith</td>
<td>RSC</td>
<td>S0</td>
<td>CC</td>
<td>Rd</td>
<td>A</td>
<td>F</td>
</tr>

<tr>
<th align=left>
INIT[CC][S][A][F] Rn, Rm/Imm -> Rd
</th>
<td>Arith</td>
<td>INIT</td>
<td>S0</td>
<td>CC</td>
<td>Rd</td>
<td>A</td>
<td>F</td>
</tr>

<tr>
<th align=left>
IMLA[CC][S][A][F] Rn, Rm/Imm -> Rd
</th>
<td>Arith</td>
<td>INIT</td>
<td>S0</td>
<td>CC</td>
<td>Rd</td>
<td>A</td>
<td>F</td>
</tr>

<tr>
<th align=left>
IMLB[CC][S][A][F] -> Rd
</th>
<td>Arith</td>
<td>INIT</td>
<td>S0</td>
<td>CC</td>
<td>Rd</td>
<td>A</td>
<td>F</td>
</tr>

<tr>
<th align=left>
IDIV[CC][S][A][F] -> Rd
</th>
<td>Arith</td>
<td>INIT</td>
<td>S0</td>
<td>CC</td>
<td>Rd</td>
<td>A</td>
<td>F</td>
</tr>

<tr>
<th colspan=8>Logical</th>
</tr>

<tr>
<th align=left>
IAND[CC][S][P][A][F] Rn, Rm/Imm -> Rd
</th>
<td>Logic</td>
<td>AND</td>
<td>SP</td>
<td>CC</td>
<td>Rd</td>
<td>A</td>
<td>F</td>
</tr>

<tr>
<th align=left>
IORR[CC][S][P][A][F] Rn, Rm/Imm -> Rd
</th>
<td>Logic</td>
<td>OR</td>
<td>SP</td>
<td>CC</td>
<td>Rd</td>
<td>A</td>
<td>F</td>
</tr>

<tr>
<th align=left>
IEOR[CC][S][P][A][F] Rn, Rm/Imm -> Rd
</th>
<td>Logic</td>
<td>XOR</td>
<td>SP</td>
<td>CC</td>
<td>Rd</td>
<td>A</td>
<td>F</td>
</tr>

<tr>
<th align=left>
IBIC[CC][S][P][A][F] Rn, Rm/Imm -> Rd
</th>
<td>Logic</td>
<td>BIC</td>
<td>SP</td>
<td>CC</td>
<td>Rd</td>
<td>A</td>
<td>F</td>
</tr>

<tr>
<th align=left>
IORN[CC][S][P][A][F] Rn, Rm/Imm -> Rd
</th>
<td>Logic</td>
<td>ORN</td>
<td>SP</td>
<td>CC</td>
<td>Rd</td>
<td>A</td>
<td>F</td>
</tr>

<tr>
<th align=left>
IMOV[CC][S][P][A][F] Rm/Imm -> Rd
</th>
<td>Logic</td>
<td>MOV</td>
<td>SP</td>
<td>CC</td>
<td>Rd</td>
<td>A</td>
<td>F</td>
</tr>

<tr>
<th align=left>
IMVN[CC][S][P][A][F] Rm/Imm -> Rd
</th>
<td>Logic</td>
<td>MVN</td>
<td>SP</td>
<td>CC</td>
<td>Rd</td>
<td>A</td>
<td>F</td>
</tr>

<tr>
<th align=left>
IXORFIRST[CC][S][P][A][F] Rn, Rm/Imm -> Rd
</th>
<td>Logic</td>
<td>XORF</td>
<td>SP</td>
<td>CC</td>
<td>Rd</td>
<td>A</td>
<td>F</td>
</tr>

<tr>
<th align=left>
IXORLAST[CC][S][P][A][F] Rn, Rm/Imm -> Rd
</th>
<td>Logic</td>
<td>XORL</td>
<td>SP</td>
<td>CC</td>
<td>Rd</td>
<td>A</td>
<td>F</td>
</tr>

<tr>
<th align=left>
IANDCOUNT[CC][S][P][A][F] Rn, Rm/Imm -> Rd
</th>
<td>Logic</td>
<td>ANDC</td>
<td>SP</td>
<td>CC</td>
<td>Rd</td>
<td>A</td>
<td>F</td>
</tr>

<tr>
<th align=left>
IANDXOR[CC][S][P][A][F] Rn, Rm/Imm -> Rd
</th>
<td>Logic</td>
<td>ANDX</td>
<td>SP</td>
<td>CC</td>
<td>Rd</td>
<td>A</td>
<td>F</td>
</tr>

<tr>
<th align=left>
IBITREV[CC][S][P][A][F] Rm/Imm -> Rd
</th>
<td>Logic</td>
<td>BITR</td>
<td>SP</td>
<td>CC</td>
<td>Rd</td>
<td>A</td>
<td>F</td>
</tr>

<tr>
<th align=left>
IBYTEREV[CC][S][P][A][F] Rm/Imm -> Rd
</th>
<td>Logic</td>
<td>BYTR</td>
<td>SP</td>
<td>CC</td>
<td>Rd</td>
<td>A</td>
<td>F</td>
</tr>

<tr>
<th colspan=8>Immediate comparison</th>
</tr>

<tr>
<th align=left>
IADD[CC][S][A] Rn, Rm/Imm -> {cond}
</th>
<td>Arith</td>
<td>ADD</td>
<td>S0</td>
<td>CC</td>
<td>cond</td>
<td>A</td>
<td>0</td>
</tr>

<tr>
<th align=left>
ISUB[CC][S][A] Rn, Rm/Imm -> {cond}
</th>
<td>Arith</td>
<td>SUB</td>
<td>S0</td>
<td>CC</td>
<td>cond</td>
<td>A</td>
<td>0</td>
</tr>

<tr>
<th align=left>
IRSB[CC][S][A] Rn, Rm/Imm -> {cond}
</th>
<td>Arith</td>
<td>RSB</td>
<td>S0</td>
<td>CC</td>
<td>cond</td>
<td>A</td>
<td>0</td>
</tr>

<tr>
<th align=left>
IAND[CC][S][P][A][F] Rn, Rm/Imm -> {cond}
</th>
<td>Logic</td>
<td>AND</td>
<td>SP</td>
<td>CC</td>
<td>cond</td>
<td>A</td>
<td>F</td>
</tr>

<tr>
<th align=left>
IORR[CC][S][P][A][F] Rn, Rm/Imm -> {cond}
</th>
<td>Logic</td>
<td>OR</td>
<td>SP</td>
<td>CC</td>
<td>cond</td>
<td>A</td>
<td>F</td>
</tr>

<tr>
<th align=left>
IEOR[CC][S][P][A][F] Rn, Rm/Imm -> {cond}
</th>
<td>Logic</td>
<td>XOR</td>
<td>SP</td>
<td>CC</td>
<td>cond</td>
<td>A</td>
<td>F</td>
</tr>

<tr>
<th align=left>
IBIC[CC][S][P][A][F] Rn, Rm/Imm -> {cond}
</th>
<td>Logic</td>
<td>BIC</td>
<td>SP</td>
<td>CC</td>
<td>cond</td>
<td>A</td>
<td>F</td>
</tr>

<tr>
<th align=left>
IORN[CC][S][P][A][F] Rn, Rm/Imm -> {cond}
</th>
<td>Logic</td>
<td>ORN</td>
<td>SP</td>
<td>CC</td>
<td>cond</td>
<td>A</td>
<td>F</td>
</tr>

<tr>
<th align=left>
IANDXOR[CC][S][P][A][F] Rn, Rm/Imm -> {cond}
</th>
<td>Logic</td>
<td>ANDX</td>
<td>SP</td>
<td>CC</td>
<td>cond</td>
<td>A</td>
<td>F</td>
</tr>

<tr>
<th colspan=8>Shift</th>
</tr>

<tr>
<th align=left>
ILSL[CC][S][F] Rn, Rm/Imm -> Rd
</th>
<td>SHF</td>
<td>LSL</td>
<td>S0</td>
<td>CC</td>
<td>Rd</td>
<td>0</td>
<td>F</td>
</tr>

<tr>
<th align=left>
ILSR[CC][S][F] Rn, Rm/Imm -> Rd
</th>
<td>SHF</td>
<td>LSR</td>
<td>S0</td>
<td>CC</td>
<td>Rd</td>
<td>0</td>
<td>F</td>
</tr>

<tr>
<th align=left>
IASR[CC][S][F] Rn, Rm/Imm -> Rd
</th>
<td>SHF</td>
<td>ASR</td>
<td>S0</td>
<td>CC</td>
<td>Rd</td>
<td>0</td>
<td>F</td>
</tr>

<tr>
<th align=left>
IROR[CC][S][F] Rn, Rm/Imm -> Rd
</th>
<td>SHF</td>
<td>ROR</td>
<td>S0</td>
<td>CC</td>
<td>Rd</td>
<td>0</td>
<td>F</td>
</tr>

<tr>
<th align=left>
IROR33[CC][S][F] Rn -> Rd
</th>
<td>SHF</td>
<td>ROR33</td>
<td>S0</td>
<td>CC</td>
<td>Rd</td>
<td>0</td>
<td>F</td>
</tr>

<tr>
<th colspan=8>Coprocessor</th>
</tr>

<tr>
<th align=left>
ICPRD[CC] Rm/Imm -> Rd
</th>
<td>Coproc</td>
<td>Read</td>
<td>00</td>
<td>CC</td>
<td>Rd</td>
<td>0</td>
<td>0</td>
</tr>

<tr>
<th align=left>
ICPWR[CC] Rn, Rm/Imm
</th>
<td>Coproc</td>
<td>Write</td>
<td>00</td>
<td>CC</td>
<td>000000</td>
<td>0</td>
<td>0</td>
</tr>

<tr>
<th align=left>
ICPCMD[CC] Rm/Imm
</th>
<td>Coproc</td>
<td>Cmd</td>
<td>00</td>
<td>CC</td>
<td>000000</td>
<td>0</td>
<td>0</td>
</tr>

<tr>
<th colspan=8>Word loads</th>
</tr>

<tr>
<th align=left>
ILDR[CC][S][F] #k (Rn) -> Rd
</th>
<td>Load</td>
<td>Post<br>Up<br>Word</td>
<td>0s</td>
<td>CC</td>
<td>Rd</td>
<td>0</td>
<td>F</td>
</tr>

<tr>
<th align=left>
ILDR[CC]A[S][F] #k (Rn), Rm/Imm -> Rd
</th>
<td>Load</td>
<td>Post<br>Up<br>Word</td>
<td>1s</td>
<td>CC</td>
<td>Rd</td>
<td>1</td>
<td>F</td>
</tr>

<tr>
<th align=left>
ILDR[CC][A][S][F] #k (Rn, Rm/Imm) -> Rd
</th>
<td>Load</td>
<td>Pre<br>Up<br>Word</td>
<td>0s</td>
<td>CC</td>
<td>Rd</td>
<td>A</td>
<td>F</td>
</tr>

<tr>
<th align=left>
ILDR[CC]A[S][F] #k (Rn), -Rm/Imm -> Rd
</th>
<td>Load</td>
<td>Post<br>Down<br>Word</td>
<td>1s</td>
<td>CC</td>
<td>Rd</td>
<td>1</td>
<td>F</td>
</tr>

<tr>
<th align=left>
ILDR[CC][A][S][F] #k (Rn, -Rm/Imm) -> Rd
</th>
<td>Load</td>
<td>Pre<br>Down<br>Word</td>
<td>0s</td>
<td>CC</td>
<td>Rd</td>
<td>A</td>
<td>F</td>
</tr>


<tr>
<th colspan=8>Byte loads</th>
</tr>

<tr>
<th align=left>
ILDR[CC]B[S][F] #k (Rn) -> Rd
</th>
<td>Load</td>
<td>Post<br>Up<br>Byte</td>
<td>0s</td>
<td>CC</td>
<td>Rd</td>
<td>0</td>
<td>F</td>
</tr>

<tr>
<th align=left>
ILDR[CC]BA[S][F] #k (Rn), Rm/Imm -> Rd
</th>
<td>Load</td>
<td>Post<br>Up<br>Byte</td>
<td>1s</td>
<td>CC</td>
<td>Rd</td>
<td>1</td>
<td>F</td>
</tr>

<tr>
<th align=left>
ILDR[CC]B[A][S][F] #k (Rn, Rm/Imm) -> Rd
</th>
<td>Load</td>
<td>Pre<br>Up<br>Byte</td>
<td>0s</td>
<td>CC</td>
<td>Rd</td>
<td>A</td>
<td>F</td>
</tr>

<tr>
<th align=left>
ILDR[CC]BA[S][F] #k (Rn), -Rm/Imm -> Rd
</th>
<td>Load</td>
<td>Post<br>Down<br>Byte</td>
<td>1s</td>
<td>CC</td>
<td>Rd</td>
<td>1</td>
<td>F</td>
</tr>

<tr>
<th align=left>
ILDR[CC]B[A][S][F] #k (Rn, -Rm/Imm) -> Rd
</th>
<td>Load</td>
<td>Pre<br>Down<br>Byte</td>
<td>0s</td>
<td>CC</td>
<td>Rd</td>
<td>A</td>
<td>F</td>
</tr>


<tr>
<th colspan=8>Halfword loads</th>
</tr>

<tr>
<th align=left>
ILDR[CC]H[S][F] #k (Rn) -> Rd
</th>
<td>Load</td>
<td>Post<br>Up<br>Half</td>
<td>0s</td>
<td>CC</td>
<td>Rd</td>
<td>0</td>
<td>F</td>
</tr>

<tr>
<th align=left>
ILDR[CC]HA[S][F] #k (Rn), Rm/Imm -> Rd
</th>
<td>Load</td>
<td>Post<br>Up<br>Half</td>
<td>1s</td>
<td>CC</td>
<td>Rd</td>
<td>1</td>
<td>F</td>
</tr>

<tr>
<th align=left>
ILDR[CC]H[A][S][F] #k (Rn, Rm/Imm) -> Rd
</th>
<td>Load</td>
<td>Pre<br>Up<br>Half</td>
<td>0s</td>
<td>CC</td>
<td>Rd</td>
<td>A</td>
<td>F</td>
</tr>

<tr>
<th align=left>
ILDR[CC]HA[S][F] #k (Rn), -Rm/Imm -> Rd
</th>
<td>Load</td>
<td>Post<br>Down<br>Half</td>
<td>1s</td>
<td>CC</td>
<td>Rd</td>
<td>1</td>
<td>F</td>
</tr>

<tr>
<th align=left>
ILDR[CC]H[A][S][F] #k (Rn, -Rm/Imm) -> Rd
</th>
<td>Load</td>
<td>Pre<br>Down<br>Half</td>
<td>0s</td>
<td>CC</td>
<td>Rd</td>
<td>A</td>
<td>F</td>
</tr>


<tr>
<th colspan=8>Word stores</th>
</tr>

<tr>
<th align=left>
ISTR[CC][S] #k (Rn) <- Rm/Imm
</th>
<td>Store</td>
<td>Post<br>Up<br>Word</td>
<td>0s</td>
<td>CC</td>
<td>000000</td>
<td>0</td>
<td>0</td>
</tr>

<tr>
<th align=left>
ISTR[CC][A][S] #k (Rn), #+4 <- Rm/Imm [-> Rd]
</th>
<td>Store</td>
<td>Post<br>Up<br>Word</td>
<td>0s</td>
<td>CC</td>
<td>Rd</td>
<td>A</td>
<td>0</td>
</tr>

<tr>
<th align=left>
ISTR[CC][A][S] #k (Rn, #+4) <- Rm/Imm [-> Rd]
</th>
<td>Store</td>
<td>Pre<br>Up<br>Word</td>
<td>0s</td>
<td>CC</td>
<td>Rd</td>
<td>A</td>
<td>0</td>
</tr>

<tr>
<th align=left>
ISTR[CC]A[S] #k (Rn), #+SHF <- Rm/Imm [-> Rd]
</th>
<td>Store</td>
<td>Post<br>Up<br>Word</td>
<td>1s</td>
<td>CC</td>
<td>Rd</td>
<td>A</td>
<td>0</td>
</tr>

<tr>
<th align=left>
ISTR[CC][A][S] #k (Rn, #+SHF) <- Rm/Imm [-> Rd]
</th>
<td>Store</td>
<td>Pre<br>Up<br>Word</td>
<td>1s</td>
<td>CC</td>
<td>Rd</td>
<td>A</td>
<td>0</td>
</tr>

<tr>
<th align=left>
ISTR[CC][A][S] #k (Rn), #-4 <- Rm/Imm [-> Rd]
</th>
<td>Store</td>
<td>Post<br>Down<br>Word</td>
<td>0s</td>
<td>CC</td>
<td>Rd</td>
<td>A</td>
<td>0</td>
</tr>

<tr>
<th align=left>
ISTR[CC][A][S] #k (Rn, #-4) <- Rm/Imm [-> Rd]
</th>
<td>Store</td>
<td>Pre<br>Down<br>Word</td>
<td>0s</td>
<td>CC</td>
<td>Rd</td>
<td>A</td>
<td>0</td>
</tr>

<tr>
<th align=left>
ISTR[CC][A][S] #k (Rn), #-SHF <- Rm/Imm [-> Rd]
</th>
<td>Store</td>
<td>Post<br>Down<br>Word</td>
<td>1s</td>
<td>CC</td>
<td>Rd</td>
<td>A</td>
<td>0</td>
</tr>

<tr>
<th align=left>
ISTR[CC][A][S] #k (Rn, #-SHF) <- Rm/Imm [-> Rd]
</th>
<td>Store</td>
<td>Pre<br>Down<br>Word</td>
<td>1s</td>
<td>CC</td>
<td>Rd</td>
<td>A</td>
<td>0</td>
</tr>


<tr>
<th colspan=8>Byte stores</th>
</tr>

<tr>
<th align=left>
ISTR[CC]B[S] #k (Rn) <- Rm/Imm
</th>
<td>Store</td>
<td>Post<br>Up<br>Byte</td>
<td>0s</td>
<td>CC</td>
<td>000000</td>
<td>0</td>
<td>0</td>
</tr>

<tr>
<th align=left>
ISTR[CC]B[A][S] #k (Rn), #+1 <- Rm/Imm [-> Rd]
</th>
<td>Store</td>
<td>Post<br>Up<br>Byte</td>
<td>0s</td>
<td>CC</td>
<td>Rd</td>
<td>A</td>
<td>0</td>
</tr>

<tr>
<th align=left>
ISTR[CC]B[A][S] #k (Rn, #+1) <- Rm/Imm [-> Rd]
</th>
<td>Store</td>
<td>Pre<br>Up<br>Byte</td>
<td>0s</td>
<td>CC</td>
<td>Rd</td>
<td>A</td>
<td>0</td>
</tr>

<tr>
<th align=left>
ISTR[CC]BA[S] #k (Rn), #+SHF <- Rm/Imm [-> Rd]
</th>
<td>Store</td>
<td>Post<br>Up<br>Byte</td>
<td>1s</td>
<td>CC</td>
<td>Rd</td>
<td>A</td>
<td>0</td>
</tr>

<tr>
<th align=left>
ISTR[CC]B[A][S] #k (Rn, #+SHF) <- Rm/Imm [-> Rd]
</th>
<td>Store</td>
<td>Pre<br>Up<br>Byte</td>
<td>1s</td>
<td>CC</td>
<td>Rd</td>
<td>A</td>
<td>0</td>
</tr>

<tr>
<th align=left>
ISTR[CC]B[A][S] #k (Rn), #-1 <- Rm/Imm [-> Rd]
</th>
<td>Store</td>
<td>Post<br>Down<br>Byte</td>
<td>0s</td>
<td>CC</td>
<td>Rd</td>
<td>A</td>
<td>0</td>
</tr>

<tr>
<th align=left>
ISTR[CC]B[A][S] #k (Rn, #-1) <- Rm/Imm [-> Rd]
</th>
<td>Store</td>
<td>Pre<br>Down<br>Byte</td>
<td>0s</td>
<td>CC</td>
<td>Rd</td>
<td>A</td>
<td>0</td>
</tr>

<tr>
<th align=left>
ISTR[CC]B[A][S] #k (Rn), #-SHF <- Rm/Imm [-> Rd]
</th>
<td>Store</td>
<td>Post<br>Down<br>Byte</td>
<td>1s</td>
<td>CC</td>
<td>Rd</td>
<td>A</td>
<td>0</td>
</tr>

<tr>
<th align=left>
ISTR[CC]B[A][S] #k (Rn, #-SHF) <- Rm/Imm [-> Rd]
</th>
<td>Store</td>
<td>Pre<br>Down<br>Byte</td>
<td>1s</td>
<td>CC</td>
<td>Rd</td>
<td>A</td>
<td>0</td>
</tr>


<tr>
<th colspan=8>Halfword stores</th>
</tr>

<tr>
<th align=left>
ISTR[CC]H[S] #k (Rn) <- Rm/Imm
</th>
<td>Store</td>
<td>Post<br>Up<br>Half</td>
<td>0s</td>
<td>CC</td>
<td>000000</td>
<td>0</td>
<td>0</td>
</tr>

<tr>
<th align=left>
ISTR[CC]H[A][S] #k (Rn), #+2 <- Rm/Imm [-> Rd]
</th>
<td>Store</td>
<td>Post<br>Up<br>Half</td>
<td>0s</td>
<td>CC</td>
<td>Rd</td>
<td>A</td>
<td>0</td>
</tr>

<tr>
<th align=left>
ISTR[CC]H[A][S] #k (Rn, #+2) <- Rm/Imm [-> Rd]
</th>
<td>Store</td>
<td>Pre<br>Up<br>Half</td>
<td>0s</td>
<td>CC</td>
<td>Rd</td>
<td>A</td>
<td>0</td>
</tr>

<tr>
<th align=left>
ISTR[CC]HA[S] #k (Rn), #+SHF <- Rm/Imm [-> Rd]
</th>
<td>Store</td>
<td>Post<br>Up<br>Half</td>
<td>1s</td>
<td>CC</td>
<td>Rd</td>
<td>A</td>
<td>0</td>
</tr>

<tr>
<th align=left>
ISTR[CC]H[A][S] #k (Rn, #+SHF) <- Rm/Imm [-> Rd]
</th>
<td>Store</td>
<td>Pre<br>Up<br>Half</td>
<td>1s</td>
<td>CC</td>
<td>Rd</td>
<td>A</td>
<td>0</td>
</tr>

<tr>
<th align=left>
ISTR[CC]H[A][S] #k (Rn), #-2 <- Rm/Imm [-> Rd]
</th>
<td>Store</td>
<td>Post<br>Down<br>Half</td>
<td>0s</td>
<td>CC</td>
<td>Rd</td>
<td>A</td>
<td>0</td>
</tr>

<tr>
<th align=left>
ISTR[CC]H[A][S] #k (Rn, #-2) <- Rm/Imm [-> Rd]
</th>
<td>Store</td>
<td>Pre<br>Down<br>Half</td>
<td>0s</td>
<td>CC</td>
<td>Rd</td>
<td>A</td>
<td>0</td>
</tr>

<tr>
<th align=left>
ISTR[CC]H[A][S] #k (Rn), #-SHF <- Rm/Imm [-> Rd]
</th>
<td>Store</td>
<td>Post<br>Down<br>Half</td>
<td>1s</td>
<td>CC</td>
<td>Rd</td>
<td>A</td>
<td>0</td>
</tr>

<tr>
<th align=left>
ISTR[CC]H[A][S] #k (Rn, #-SHF) <- Rm/Imm [-> Rd]
</th>
<td>Store</td>
<td>Pre<br>Down<br>Half</td>
<td>1s</td>
<td>CC</td>
<td>Rd</td>
<td>A</td>
<td>0</td>
</tr>



<tr>
<th colspan=8>Scheduler operations</th>
</tr>

<tr>
<th align=left>
ISCHDSEMOR[CC][A] Rm/Imm [-> Rd]
</th>
<td>Scheduler</td>
<td>OR</td>
<td>00</td>
<td>CC</td>
<td>000000</td>
<td>A</td>
<td>0</td>
</tr>

<tr>
<th align=left>
ISCHDSEMBIC[CC][A] Rm/Imm [-> Rd]
</th>
<td>Scheduler</td>
<td>BIC</td>
<td>00</td>
<td>CC</td>
<td>000000</td>
<td>A</td>
<td>0</td>
</tr>

<tr>
<th align=left>
ISCHDSEMXOR[CC][A] Rm/Imm [-> Rd]
</th>
<td>Scheduler</td>
<td>XOR</td>
<td>00</td>
<td>CC</td>
<td>000000</td>
<td>A</td>
<td>0</td>
</tr>

<tr>
<th align=left>
ISCHDINFO[CC][A] Rm/Imm [-> Rd]
</th>
<td>Scheduler</td>
<td>???</td>
<td>00</td>
<td>CC</td>
<td>000000</td>
<td>A</td>
<td>0</td>
</tr>

<tr>
<th align=left>
ISCHDPC[CC][A] Rm/Imm [-> Rd]
</th>
<td>Scheduler</td>
<td>???</td>
<td>00</td>
<td>CC</td>
<td>000000</td>
<td>A</td>
<td>0</td>
</tr>

<tr>
<th align=left>
ISCHDRESTART[CC][A] Rm/Imm [-> Rd]
</th>
<td>Scheduler</td>
<td>???</td>
<td>00</td>
<td>CC</td>
<td>000000</td>
<td>A</td>
<td>0</td>
</tr>


</table>

Still missing:
XMC (word, block), XCM (word, block), MEMCMD (prefetch, writeback, writeback_and_invalidate, flush, fill buffer from address, write buffer to address,
Sticky flags

condition passed shadow size, L=little-endian, u=unaligned will come from pipeline configuration, as do sticky flags.

<?php
page_ep();

include "${toplevel}web_assist/web_footer.php"; ?>

