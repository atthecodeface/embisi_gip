<?php

include "web_locals.php";
include "${toplevel}web_assist/web_globals.php";

site_set_location("gip.instruction_set");

$page_title = "GIP Documentation";

include "${toplevel}web_assist/web_header.php";

page_header( "GIP Native Instruction Set" );

page_sp();
?>

<?php page_section( "overview", "Overview" ); ?>

<p>

This area describes the 16-bit GIP native instruction set, which is
mapped by the 16-bit instruction decoder in to GIP internal
instructions. Some instruction encodings are modal; that is they map
to different internal instructions depending on the mode of the
thread, which may be adjusted by another 16-bit instruction. Another
item to note is the instruction set is a 2-register instruction set,
with separated memory and operation instructions, much like simple
RISC processors.

<p>

The native instruction set is designed for a wide range of
functionality with high code density, with the expectation that most
code blocks written will be relatively small. Memory accesses are
reduced through a fairly large register set; lowering power
consumption and improving performance a little, for address and data
calculations. But at the end of the day the bulk of the operation in a
microcontroller is data-touching, and so some powerful memory access
instructions are included. The native instruction set is not expected
to support high level languages, so complex stack manipulation
instructions must be built from many individual instructions.

</p>

<?php page_section( "encodings", "Instruction encodings" ); ?>

The instruction encodings are listed below; the placing is for ease of reference by the sections below.

<p>

<?php

bit_breakout_start( 16, "Mnemonic" );

bit_breakout_hdr( "ALU Rn, Rm" );
bit_breakout_bits_split( "0000" );
bit_breakout_bits( 4, "ALU" );
bit_breakout_bits( 4, "Rd/n" );
bit_breakout_bits( 4, "Rm" );
bit_breakout_bits( -1, "Extcmd, ExtRnRm, ExtRdRm" );

bit_breakout_hdr( "ALU Rn, #I" );
bit_breakout_bits_split( "0001" );
bit_breakout_bits( 4, "ALU" );
bit_breakout_bits( 4, "Rd/n" );
bit_breakout_bits_split( "IIII" );
bit_breakout_bits( -1, "Extimm, Extcmd, ExtRnRm, ExtRdRm" );

bit_breakout_hdr( "Cnd Rn, Rm" );
bit_breakout_bits_split( "0010" );
bit_breakout_bits( 4, "Cnd" );
bit_breakout_bits( 4, "Rn" );
bit_breakout_bits( 4, "Rm" );
bit_breakout_bits( -1, "Extcmd, ExtRnRm, ExtRdRm" );

bit_breakout_hdr( "Cnd Rn, #I" );
bit_breakout_bits_split( "0011" );
bit_breakout_bits( 4, "Cnd" );
bit_breakout_bits( 4, "Rn" );
bit_breakout_bits_split( "IIII" );
bit_breakout_bits( -1, "Extimm, Extcmd, ExtRnRm, ExtRdRm" );

bit_breakout_hdr( "Shf Rd/n, Rm" );
bit_breakout_bits_split( "0100" );
bit_breakout_bits( 2, "Shf" );
bit_breakout_bits_split( "0x" );
bit_breakout_bits( 4, "Rd/n" );
bit_breakout_bits( 4, "Rm" );
bit_breakout_bits( -1, "Extcmd, ExtRnRm, ExtRdRm" );

bit_breakout_hdr( "Shf Rd/n, #I" );
bit_breakout_bits_split( "0100" );
bit_breakout_bits( 2, "Shf" );
bit_breakout_bits_split( "1" );
bit_breakout_bits_split( "I" );
bit_breakout_bits( 4, "Rd/n" );
bit_breakout_bits_split( "IIII" );
bit_breakout_bits( -1, "Extimm, Extcmd, ExtRnRm, ExtRdRm" );



bit_breakout_hdr( "Ldr Rd, [Rn]" );
bit_breakout_bits_split( "0101" );
bit_breakout_bits_split( "000" );
bit_breakout_bits_split( "0" );
bit_breakout_bits( 4, "Rn" );
bit_breakout_bits( 4, "Rd" );
bit_breakout_bits( -1, "Extimm, Extcmd, ExtRnRm, ExtRdRm" );

bit_breakout_hdr( "LdrH Rd, [Rn]" );
bit_breakout_bits_split( "0101" );
bit_breakout_bits_split( "001" );
bit_breakout_bits_split( "0" );
bit_breakout_bits( 4, "Rn" );
bit_breakout_bits( 4, "Rd" );
bit_breakout_bits( -1, "Extimm, Extcmd, ExtRnRm, ExtRdRm" );

bit_breakout_hdr( "LdrB Rd, [Rn]" );
bit_breakout_bits_split( "0101" );
bit_breakout_bits_split( "010" );
bit_breakout_bits_split( "0" );
bit_breakout_bits( 4, "Rn" );
bit_breakout_bits( 4, "Rd" );
bit_breakout_bits( -1, "Extimm, Extcmd, ExtRnRm, ExtRdRm" );

bit_breakout_hdr( "Ldr Rd, [Rn, #4]" );
bit_breakout_bits_split( "0101" );
bit_breakout_bits_split( "011" );
bit_breakout_bits_split( "0" );
bit_breakout_bits( 4, "Rn" );
bit_breakout_bits( 4, "Rd" );
bit_breakout_bits( -1, "Extcmd, ExtRnRm, ExtRdRm" );

bit_breakout_hdr( "Ldr Rd, [Rn, SHF]" );
bit_breakout_bits_split( "0101" );
bit_breakout_bits_split( "100" );
bit_breakout_bits_split( "0" );
bit_breakout_bits( 4, "Rn" );
bit_breakout_bits( 4, "Rd" );
bit_breakout_bits( -1, "Extcmd, ExtRnRm, ExtRdRm" );

bit_breakout_hdr( "Ldr Rd, [Rn, -SHF]" );
bit_breakout_bits_split( "0101" );
bit_breakout_bits_split( "101" );
bit_breakout_bits_split( "0" );
bit_breakout_bits( 4, "Rn" );
bit_breakout_bits( 4, "Rd" );
bit_breakout_bits( -1, "Extcmd, ExtRnRm, ExtRdRm" );

bit_breakout_hdr( "Ldr Rd, [Rn], #4" );
bit_breakout_bits_split( "0101" );
bit_breakout_bits_split( "110" );
bit_breakout_bits_split( "0" );
bit_breakout_bits( 4, "Rn" );
bit_breakout_bits( 4, "Rd" );
bit_breakout_bits( -1, "Extcmd, ExtRnRm, ExtRdRm<br>Does not write back Rn" );

bit_breakout_hdr( "Ldr Rd, [Rn], #-4" );
bit_breakout_bits_split( "0101" );
bit_breakout_bits_split( "111" );
bit_breakout_bits_split( "0" );
bit_breakout_bits( 4, "Rn" );
bit_breakout_bits( 4, "Rd" );
bit_breakout_bits( -1, "Extimm, Extcmd, ExtRnRm, ExtRdRm<br>Does not write back Rn" );

bit_breakout_hdr( "Str Rm, [Rn]" );
bit_breakout_bits_split( "0101" );
bit_breakout_bits_split( "000" );
bit_breakout_bits_split( "1" );
bit_breakout_bits( 4, "Rn" );
bit_breakout_bits( 4, "Rm" );
bit_breakout_bits( -1, "Extcmd, ExtRnRm, ExtRdRm<br>Writes address to Rd if extended" );

bit_breakout_hdr( "StrH Rm, [Rn]" );
bit_breakout_bits_split( "0101" );
bit_breakout_bits_split( "001" );
bit_breakout_bits_split( "1" );
bit_breakout_bits( 4, "Rn" );
bit_breakout_bits( 4, "Rm" );
bit_breakout_bits( -1, "Extcmd, ExtRnRm, ExtRdRm<br>Writes address to Rd if extended" );

bit_breakout_hdr( "StrB Rm, [Rn]" );
bit_breakout_bits_split( "0101" );
bit_breakout_bits_split( "010" );
bit_breakout_bits_split( "1" );
bit_breakout_bits( 4, "Rn" );
bit_breakout_bits( 4, "Rm" );
bit_breakout_bits( -1, "Extcmd, ExtRnRm, ExtRdRm<br>Writes address to Rd if extended" );

bit_breakout_hdr( "Str Rm, [Rn, #4]!" );
bit_breakout_bits_split( "0101" );
bit_breakout_bits_split( "011" );
bit_breakout_bits_split( "1" );
bit_breakout_bits( 4, "Rn" );
bit_breakout_bits( 4, "Rm" );
bit_breakout_bits( -1, "Extcmd, ExtRnRm, ExtRdRm<br>Writes back Rd (=Rn if not extended)" );

bit_breakout_hdr( "Str Rm, [Rn, +SHF]" );
bit_breakout_bits_split( "0101" );
bit_breakout_bits_split( "100" );
bit_breakout_bits_split( "1" );
bit_breakout_bits( 4, "Rn" );
bit_breakout_bits( 4, "Rm" );
bit_breakout_bits( -1, "Extcmd, ExtRnRm, ExtRdRm<br>Writes address to Rd if extended" );

bit_breakout_hdr( "Str Rm, [Rn, -SHF]" );
bit_breakout_bits_split( "0101" );
bit_breakout_bits_split( "101" );
bit_breakout_bits_split( "1" );
bit_breakout_bits( 4, "Rn" );
bit_breakout_bits( 4, "Rm" );
bit_breakout_bits( -1, "Extcmd, ExtRnRm, ExtRdRm<br>Writes address to Rd if extended" );

bit_breakout_hdr( "Str Rm, [Rn], #4" );
bit_breakout_bits_split( "0101" );
bit_breakout_bits_split( "110" );
bit_breakout_bits_split( "1" );
bit_breakout_bits( 4, "Rn" );
bit_breakout_bits( 4, "Rm" );
bit_breakout_bits( -1, "Extcmd, ExtRnRm, ExtRdRm<br>Writes back Rd (=Rn if not extended)" );

bit_breakout_hdr( "Str Rm, [Rn], #-4" );
bit_breakout_bits_split( "0101" );
bit_breakout_bits_split( "111" );
bit_breakout_bits_split( "1" );
bit_breakout_bits( 4, "Rn" );
bit_breakout_bits( 4, "Rm" );
bit_breakout_bits( -1, "Extcmd, ExtRnRm, ExtRdRm<br>Writes back Rd (=Rn if not extended)" );



bit_breakout_hdr( "B" );
bit_breakout_bits_split( "0110" );
bit_breakout_bits( 1, "Dly" );
bit_breakout_bits( 11, "Offset" );
bit_breakout_bits( -1, "Extimm" );

bit_breakout_hdr( "Special" );
bit_breakout_bits_split( "0111" );
bit_breakout_bits( 12, "Undefined" );
bit_breakout_bits( -1, "Extimm, Extcmd, ExtRnRm, ExtRdRm" );



bit_breakout_hdr( "ExtImm" );
bit_breakout_bits_split( "10" );
bit_breakout_bits( 14, "imm value" );
bit_breakout_bits( -1, "Extimm (to fully extend)" );

bit_breakout_hdr( "ExtRdRm" );
bit_breakout_bits_split( "1100" );
bit_breakout_bits( 8, "Rd extend" );
bit_breakout_bits( 4, "Rm top extend" );
bit_breakout_bits( -1, "" );

bit_breakout_hdr( "ExtRnRm" );
bit_breakout_bits_split( "1101" );
bit_breakout_bits( 4, "Rn top extend" );
bit_breakout_bits_split( "xxxx" );
bit_breakout_bits( 4, "Rm top extend" );
bit_breakout_bits( -1, "" );

bit_breakout_hdr( "ExtCmd" );
bit_breakout_bits_split( "1110" );
bit_breakout_bits( 4, "Cond" );
bit_breakout_bits( 1, "Sign" );
bit_breakout_bits( 1, "Acc" );
bit_breakout_bits( 2, "Op" );
bit_breakout_bits( 4, "Burst" );
bit_breakout_bits( -1, "" );

bit_breakout_hdr( "Undefined" );
bit_breakout_bits_split( "1111" );
bit_breakout_bits_split( "xxxxxxxxxxxx" );
bit_breakout_bits( -1, "" );

bit_breakout_end();

?>

<?php page_section( "extending", "Extending instructions" ); ?>

<p>

A 16-bit native instruction may be extended in a number of ways, all
of which utilize 'extend' instructions immediately prior to the actual
extended instruction itself (note also that the order of extensions is
important - see below). The following extension types are provided:

<?php page_subsection( "extimm", "Extimm - Extend immediate" ); ?>

This supplies an extension to an immediate value in ALU immediate,
conditional immediate, branch and load instructions. A single extimm
instruction provides a sign-extended 14-bit number, which is used as
bits 31 to 4 of the immediate value for ALU/cond immediate
instructions. A pair of extimm instructions provides a full 28-bit
number which is used as bits 31 to 4 of the immediate value for
ALU/cond immediate instructions.

<br> Branch instructions use the 28-bit number as the top bit of a 31
bit value which is then ORred with the 10 bit offset in the
instruction; the offset is a 16-bit word offset, so only 31-bits are
required.

<br> Load instructions use the 28-bit number as the offset for
preindexed or postindexed accesses; the number is sign extended to 32
bits. Do note that the instructions themselves determine whether the
offset is added or subtracted from the index register, so only
positive immediate values are of real use.

<br> Note: if an extimm instruction is used to extend an immediate
instruction then it must be the first extension instruction (if more
than one) for that immediate instruction.


<?php page_subsection( "extrdrdm", "ExtRdRm - Extend destination register and second source register" ); ?>

This supplies a third register for an instruction, and allows for
a global specification of the second source register for an
instruction. An 8-bit field specifies Rd; if the top 4 bits are all 1
then Rd is not extended (this provides for extending purely Rm). A
4-bit field specifies the extension of Rm (the remaining 4 bits are
given in the instruction itself); if these bits are all 1 then Rm is
not extended.

<?php page_subsection( "extrnrm", "ExtRnRm - Extend first source register and second source register" ); ?>

This allows for a global specification of the source registers for
an instruction. A 4-bit field specifies the extension to Rn; if the 4
bits are all 1 then Rn is not extended (this provides for extending
purely Rm). A 4-bit field specifies the extension of Rm (the remaining
4 bits are given in the instruction itself); if these bits are all 1
then Rm is not extended.

<?php page_subsection( "extcmd", "ExtCmd - Extend command" ); ?>

The instruction contains a number of fields. The 4-bit condition field is used as indicated in the following table:

<?php
encoding_start( 4, "Condition" );
encoding( "EQ", "0000", "internal 'eq'" );
encoding( "NE", "0001", "internal 'ne'" );
encoding( "CS", "0010", "internal 'cs'" );
encoding( "CC", "0011", "internal 'cc'" );
encoding( "MI", "0100", "internal 'mi'" );
encoding( "PL", "0101", "internal 'pl'" );
encoding( "VS", "0110", "internal 'vs'" );
encoding( "VC", "0111", "internal 'vc'" );
encoding( "HI", "1000", "internal 'hi'" );
encoding( "LS", "1001", "internal 'ls'" );
encoding( "GE", "1010", "internal 'ge'" );
encoding( "LT", "1011", "internal 'lt'" );
encoding( "LE", "1100", "internal 'le'" );
encoding( "GT", "1101", "internal 'gt'" );
encoding( "", "1110", "do not override condition" );
encoding( "CP", "1111", "internal 'cp'" );
encoding_end();
?>

The use of this condition mechanism is outside the scope of the normal conditional mechanism, and should be used with caution.


<dl>

<dt>ALU instructions

<dd>When extended in this manner the ALU operation can be told: to not set sign flags and/or accumulator (basic instructions do); to specify full 5-bit ALU operation (using 2 bits in 'extcmd' with 4 bits in instruction); to provide for extended conditional instruction as discussed above.

<dt>Conditional instructions

<dd>When extended in this manner the conditional instruction can be told: to not set sign flags and/or accumulator (basic instructions do set accumulator, do not set sign flags); to use a full 5-bit condition operation; to provide for extended conditional instructions as discussed above.

<br>
This extension has potential uses in the full condition use, but the other features are probably not that useful.

<dt>Shift instructions

<dd>When extension in this manner the shift instruction can be told: to set sign flags (Z and N); to provide for extended conditional instruction as discussed above.

<dt>Branch instructions

<dd>Do not use with branch instructions

<dt>Load/store instructions

<dd>When extension in this manner the load/store instruction can be told: to set sign flags and/or accumulator from address calculation (basic instructions only set accumulator); set the burst size (which is decremented for following accesses, so if the burst size is used then successive instructions should be identical accesses to successive word addresses with no burst size specified - default burst size is the last burst size-1, or zero); to provide for extended conditional instruction as discussed above.

</dl>

<?php page_section( "alu_class", "ALU instructions" ); ?>

There are 8 basic ALU instructions accessible in all modes; a further 8 ALU instructions can be accessed which depend on the mode. However, in all modes every ALU operation may be accessed through extended commands

<?php page_subsection( "basic", "Basic ALU instructions" ); ?>

<?php
bit_breakout_start( 4, "Mnemonic" );

bit_breakout_hdr( "and" );
bit_breakout_bits_split( "0000" );
bit_breakout_bits( -1, "Rd <= Op1 & Op2" );

bit_breakout_hdr( "or" );
bit_breakout_bits_split( "0001" );
bit_breakout_bits( -1, "Rd <= Op1 | Op2" );

bit_breakout_hdr( "xor" );
bit_breakout_bits_split( "0010" );
bit_breakout_bits( -1, "Rd <= Op1 ^ Op2" );

bit_breakout_hdr( "mov" );
bit_breakout_bits_split( "0011" );
bit_breakout_bits( -1, "Rd <= ~Op2" );

bit_breakout_hdr( "mvn" );
bit_breakout_bits_split( "0100" );
bit_breakout_bits( -1, "Rd <= ~Op2" );

bit_breakout_hdr( "add" );
bit_breakout_bits_split( "0101" );
bit_breakout_bits( -1, "Rd <= Op1 + Op2" );

bit_breakout_hdr( "sub" );
bit_breakout_bits_split( "0110" );
bit_breakout_bits( -1, "Rd <= Op1 - Op2" );

bit_breakout_hdr( "adc" );
bit_breakout_bits_split( "0111" );
bit_breakout_bits( -1, "Rd <= Op1 + Op2 + C" );

bit_breakout_end();

?>

<?php page_subsection( "math", "Math mode ALU instructions" ); ?>

Math mode instructions are optimized for fast math, and can be used
for basic calculation. The 'xorfirst' instruction is useful for
floating point normalization. This mode also supports multiplication
(at two bits per instruction) and division (at one bit per
instruction).

<?php
bit_breakout_start( 4, "Mnemonic" );

bit_breakout_hdr( "xorfirst" );
bit_breakout_bits_split( "1000" );
bit_breakout_bits( -1, "Number of first bit set in Op1 ^ Op2" );

bit_breakout_hdr( "rsb" );
bit_breakout_bits_split( "1001" );
bit_breakout_bits( -1, "Rd <=-Op1 + Op2" );

bit_breakout_hdr( "init" );
bit_breakout_bits_split( "1010" );
bit_breakout_bits( -1, "MulInit" );

bit_breakout_hdr( "mla" );
bit_breakout_bits_split( "1011" );
bit_breakout_bits( -1, "MulA" );

bit_breakout_hdr( "mlb" );
bit_breakout_bits_split( "1100" );
bit_breakout_bits( -1, "Rd <= Mulb" );

bit_breakout_hdr( "sbc" );
bit_breakout_bits_split( "1101" );
bit_breakout_bits( -1, "Rd <= Op1 - Op2 - !C" );

bit_breakout_hdr( "dva" );
bit_breakout_bits_split( "1110" );
bit_breakout_bits( -1, "Rd <= Op1 + Op2" );

bit_breakout_hdr( "dvb" );
bit_breakout_bits_split( "1111" );
bit_breakout_bits( -1, "Rd <= Op1 + Op2" );

bit_breakout_end();

?>

<?php page_subsection( "bit", "Bit mode ALU instructions" ); ?>

Bit mode instructions are aimed at hardware interfacing, network
processing and media access control implementations. 'xorfirst' and
'xorlast' are useful for determining the first and last bits set in a
word, for example for address parsing. 'andcnt' is used for hardware
interfacing including parity and error correction. 'bic' and 'orn' are
useful bit manipulation instructions. 'bitreverse' and 'bytereverse'
are very useful in interfacing and network processing.

<?php
bit_breakout_start( 4, "Mnemonic" );

bit_breakout_hdr( "xorfirst" );
bit_breakout_bits_split( "1000" );
bit_breakout_bits( -1, "Number of first bit set in Op1 ^ Op2" );

bit_breakout_hdr( "rsb" );
bit_breakout_bits_split( "1001" );
bit_breakout_bits( -1, "Rd <=-Op1 + Op2" );

bit_breakout_hdr( "bic" );
bit_breakout_bits_split( "1010" );
bit_breakout_bits( -1, "Rd <= Op1 &~Op2" );

bit_breakout_hdr( "orn" );
bit_breakout_bits_split( "1011" );
bit_breakout_bits( -1, "Rd <= Op1 |~Op2" );

bit_breakout_hdr( "andcnt" );
bit_breakout_bits_split( "1100" );
bit_breakout_bits( -1, "Rd <= number of ones in (Op1 & Op2)" );

bit_breakout_hdr( "xorlast" );
bit_breakout_bits_split( "1101" );
bit_breakout_bits( -1, "Rd <= Op1 + Op2" );

bit_breakout_hdr( "bitreverse" );
bit_breakout_bits_split( "1110" );
bit_breakout_bits( -1, "Rd <= Op2 (bottom byte bit reversed)" );

bit_breakout_hdr( "bytereverse" );
bit_breakout_bits_split( "1111" );
bit_breakout_bits( -1, "Rd <= Op2 byte reversed" );

bit_breakout_end();

?>

<?php page_subsection( "gp", "General purpose mode ALU instructions" ); ?>

This area is not yet cleaned up.

<?php
bit_breakout_start( 4, "Mnemonic" );

bit_breakout_hdr( "xorfirst" );
bit_breakout_bits_split( "1000" );
bit_breakout_bits( -1, "Number of first bit set in Op1 ^ Op2" );

bit_breakout_hdr( "rsb" );
bit_breakout_bits_split( "1001" );
bit_breakout_bits( -1, "Rd <=-Op1 + Op2" );

bit_breakout_hdr( "bic" );
bit_breakout_bits_split( "1010" );
bit_breakout_bits( -1, "Rd <= Op1 &~Op2" );

bit_breakout_hdr( "andxor" );
bit_breakout_bits_split( "1110" );
bit_breakout_bits( -1, "Rd <= Op1 + Op2; used in conditional 'all set'" );

bit_breakout_hdr( "rsc" );
bit_breakout_bits_split( "1110" );
bit_breakout_bits( -1, "Math mode: Rd <=-Op1 + Op2 - !C" );

bit_breakout_end();

?>

The native ALU instruction is mapped to an internal ALU instruction first by mapping the ALU operation, the registers, and the conditional. If not extended then the conditional is derived in the standard way, and the registers are mapped in the standard way with the Rn specified in the instruction mapping both for Rn and Rd. If extended then the extended mappings are used.

<p>

The ALU op is mapped according to table:

<table>
<tr><th>ALU op<th>Internal instruction op</tr>
<tr><td>and<td>IAND</tr>
<tr><td>bic<td>IBIC</tr>
<tr><td>or <td>IORR</tr>
<tr><td>orn<td>IORN</tr>
<tr><td>xor<td>IXOR</tr>
<tr><td>mov<td>IMOV</tr>
<tr><td>mvn<td>IMVN</tr>

<tr><td>add<td>IADD</tr>
<tr><td>adc<td>IADC</tr>
<tr><td>sub<td>ISUB</tr>
<tr><td>sbc<td>ISBC</tr>
<tr><td>rsb<td>IRSB</tr>
<tr><td>rsc<td>IRSC</tr>

<tr><td>bitreverse<td>IBITREV</tr>
<tr><td>bytereverse<td>IBYTEREV</tr>

<tr><td>andcnt<td>IANDCOUNT</tr>
<tr><td>xorfirst<td>IXORFIRST</tr>
<tr><td>xorlast<td>IXORLAST</tr>

<tr><td>init<td>INIT</tr>
<tr><td>mla<td>IMLA</tr>
<tr><td>mlb<td>IMLB</tr>

<tr><td>dva<td>IDIVA</tr>
<tr><td>dvb<td>IDIVB</tr>

</table>

The conditional is determined by whether the instruction is in the shadow of a condition instruction or not. If not, then always 'AL' is used; if it is, then 'CP' is used. However, none if this applies if the instruction is extended with an 'extcmd' instruction and the conditional specified there is not 'always'.

<p>

ALU instructions usually set the sign flags and accumulator; only if extended may they not. The extension also allows for an actual operation to be specified directly; the internal instruction encoding should be used in the 'extcmd' instruction to specify the internal ALU instruction operation.

<p>

So, unextended the mapping is (for an add) to:

<br>

IADDSA Rn, Rm -> Rn

<br>

If extended with Rd set to PC, then instead the instruction is:

<br>

IADDSAF Rn, Rm -> PC

<br>

If the instruction is extended with an 'extcmd' and the specified condition in the 'extcmd' is not 1110 (''), say 'EQ' instead, and the 'extcmd' op bits in conjunction with the 4 ALU op bits in the ALU instruction yield '010011' then the instruction is mapped to:

<br>

IORNSA Rn, Rm -> Rn

<br>

The mapping of the ALU code is done with the 'op' field. If the top bit is clear, then no mapping of the ALU code is done. If it is set, then the bottom bit is used as the top bit of a five bit operation, the bottom 4 coming from the ALU instruction encoding; this five bit field is prefixed by two zeros to get the internal instruction class of the internal instruction.

<br>

<?php page_section( "cnd_class", "Condition instructions" ); ?>

Condition instructions are the mechanism used by the native GIP
instruction set to perform conditional execution. The condition
instructions create a shadow; that is, instructions that are put in to
the GIP pipeline following a condition instruction fall within its
shadow and are therefore executed based on the conditions success or
failure. The length of the shadow can be configured to be one or two
instructions, depending on the thread configuration; note that
extensions are not counted, only the instructions they extend. 16
basic conditional instructions are available at all times without
extension; these instructions implement the comparison themselves.

<p>

A condition instruction immediately following (ignoring extensions)
another condition instruction generates either an 'and' condition or
an 'or' condition, depending on the thread configuration. Note,
however, that if the shadow length is configured as 2 and a condition
instruction follows another condition instruction with an intervening
instruction in the shadow, then the second condition instruction is
treated as though it were not within the shadow of the first condition
instruction.

<?php

encoding_start( 5, "Mnemonic" );

encoding( "cmpeq", "00000", "Op1 == Op2" );
encoding( "cmpne", "00001", "Op1 != Op2" );
encoding( "cmpgt", "00010", "Op1 > Op2 (signed)" );
encoding( "cmpge", "00011", "Op1 >= Op2 (signed)" );
encoding( "cmplt", "00100", "Op1 < Op2 (signed)" );
encoding( "cmple", "00101", "Op1 <= Op2 (signed)" );
encoding( "cmphi", "00110", "Op1 > Op2 (unsigned)" );
encoding( "cmphs", "00111", "Op1 >= Op2 (unsigned)" );
encoding( "cmplo", "01000", "Op1 < Op2 (unsigned)" );
encoding( "cmpls", "01001", "Op1 <= Op2 (unsigned)" );
encoding( "tstsps", "01010", "shifter P set" );
encoding( "tstspc", "01011", "shifter P clear" );
encoding( "tstallset", "01100", "(Op1 & Op2) == Op2" );
encoding( "tstallclr", "01101", "(Op1 & Op2)==0" );
encoding( "tstanyset", "01110", "(Op1 & Op2)!=0" );
encoding( "tstanyclr", "01111", "(Op1 & Op2)!=Op2" );


encoding( "tstseq", "10000", "Z set" );
encoding( "tstsne", "10001", "Z clear" );
encoding( "tstsgt", "10010", "!Z & ((!N&!V) | (N&V)))" );
encoding( "tstsge", "10011", "(!N&!V) | (N&V)" );
encoding( "tstslt", "10100", "(!N&V) | (N&!V)" );
encoding( "tstsle", "10101", "Z | (!N&V) | (N&!V)" );
encoding( "tstshi", "10110", "C & !Z" );
encoding( "tstshs", "10111", "C" );
encoding( "tstslo", "11000", "!C" );
encoding( "tstsls", "11001", "C | !Z" );
encoding( "tstsmi", "11010", "N" );
encoding( "tstspl", "11011", "!N" );
encoding( "tstsvs", "11100", "V" );
encoding( "tstsvc", "11101", "!V" );

encoding_end();

?>

The native condition instruction is mapped to an internal ALU instruction with a destination given by the condition. The Rn and Rm registers are mapped in the normal way. The mapped instruction will be conditional 'CP' only if it immediately follows a condition instruction (perhaps with intervening extensions only) and if the thread operating mode is 'AND' for stacked conditions; else the condition will be always. However, if extended then the conditional is generated directly from the extension.

<p>

<?php
mapping_start("Native", "Internal" );
mapping( "cmpeq", "ISUBccsa Rn, Rm -> EQ" );
mapping( "cmpne", "ISUBccsa Rn, Rm -> NE" );
mapping( "cmpgt", "ISUBccsa Rn, Rm -> GT" );
mapping( "cmpge", "ISUBccsa Rn, Rm -> GE" );
mapping( "cmplt", "ISUBccsa Rn, Rm -> LT" );
mapping( "cmple", "ISUBccsa Rn, Rm -> LE" );
mapping( "cmphi", "ISUBccsa Rn, Rm -> HI" );
mapping( "cmphs", "ISUBccsa Rn, Rm -> CS" );
mapping( "cmplo", "ISUBccsa Rn, Rm -> CC" );
mapping( "cmpls", "ISUBccsa Rn, Rm -> LS" );
mapping( "tstsps", "IMOVccSa Rn, Rm -> CS" );
mapping( "tstspc", "IMOVccSa Rn, Rm -> CC" );
mapping( "tstallset", "IANDXORccsa Rn, Rm -> EQ" );
mapping( "tstallclr", "IANDccSa Rn, Rm -> EQ" );
mapping( "tstanyset", "IANDccsa Rn, Rm -> NE" );
mapping( "tstanyclr", "IANDXORccSa Rn, Rm -> NE" );
mapping( "tstseq", "ISUBEQsa Rn, Rn -> EQ" );
mapping( "tstsne", "ISUBNEsa Rn, Rn -> EQ" );
mapping( "tstsgt", "ISUBGTsa Rn, Rn -> EQ" );
mapping( "tstsge", "ISUBGEsa Rn, Rn -> EQ" );
mapping( "tstslt", "ISUBLTsa Rn, Rn -> EQ" );
mapping( "tstsle", "ISUBLEsa Rn, Rn -> EQ" );
mapping( "tstshi", "ISUBHIsa Rn, Rn -> EQ" );
mapping( "tstshs", "ISUBCSsa Rn, Rn -> EQ" );
mapping( "tstslo", "ISUBCCsa Rn, Rn -> EQ" );
mapping( "tstsls", "ISUBLSsa Rn, Rn -> EQ" );
mapping( "tstsmi", "ISUBMIsa Rn, Rn -> EQ" );
mapping( "tstspl", "ISUBPLsa Rn, Rn -> EQ" );
mapping( "tstsvs", "ISUBVSsa Rn, Rn -> EQ" );
mapping( "tstsvc", "ISUBVCsa Rn, Rn -> EQ" );
mapping_end();

?>

<?php page_section( "shf_class", "Shift instructions" ); ?>

The shift instructions provide for logical shift left, logical shift
right, arithmetic shift right and rotate right. Additionally a 33-bit
rotation is supported, and this is performed if a ROR Rn, #0
instruction is issued.

<?php

encoding_start( 4, "Mnemonic" );
encoding( "lsl", "00", "Rd,SHF,P = Op1 << Op2" );
encoding( "lsr", "01", "Rd,SHF,P = Op1 >> Op2" );
encoding( "asr", "10", "Rd,SHF,P = Op1 >>> Op2" );
encoding( "ror", "11", "Rd,SHF,P = Op1 >><< Op2 (immediate not zero)" );
encoding( "rrx", "11", "Rd,SHF,P = P,Op1 >><< (33)1 (immediate of zero)" );
encoding_end();

?>

The native shift instruction is mapped to a shift instruction. If not extended then the conditional is derived in the standard way, and the registers are mapped in the standard way with the Rn specified in the instruction mapping both for Rn and Rd. If extended then the extended mappings are used.

<p>

<?php
mapping_start("Native", "Internal" );
mapping( "lsl", "ILSLccs Rn, Rm -> Rd" );
mapping( "lsr", "ILSRccs Rn, Rm -> Rd" );
mapping( "asr", "IASRccs Rn, Rm -> Rd" );
mapping( "ror", "IRORccs Rn, Rm -> Rd" );
mapping( "rrx", "IROR33ccs Rn -> Rd" );
mapping_end();
?>

<?php page_section( "branch_class", "Branch instructions" ); ?>

Branch instructions have an optional delay slot. The instruction in the delay slot is effected by the shadow of a conditional instruction if the shadow length configured for the thread is 2 instructions, and the branch is placed as the instruction following the condition instruction. Delay slots should not be used if the thread may be preempted (i.e. if the thread is not high priority and preemption is enabled).

<p>

The branch instruction contains an 11 bit offset to which the execution flow should branch. This value is sign extended and added to the PC as a 16-bit offset. If the instruction is not in the shadow of a conditional then the branch is taken in the decode stage; if it is conditional then it will force a conditional imovf instruction in to the pipeline.

<p>

If the instruction indicates there is a delay slot then the following instruction will be inserted in to the pipeline also, marked as not flushable. As a delay slot the following instruction must not be extended; it must be just a single 16-bit instruction. It may also not be a branch. It may, however, be an extension for a following instruction that will be fetched as the target of the branch.

<p>

If the branch instruction is extended with an 'extimm' instruction then the extended immediate value is prepended to the 11-bit offset in the instruction itself.

<p>

To implement a branch-with-link the code should include an 'add rn, pc, #n' instruction somehow. Ideally we will get that down to a single 16-bit instruction.

<?php page_section( "load_class", "Load instructions" ); ?>

The native load instructions are mapped to internal load
instructions. If not extended then the conditional is derived in the
standard way, and the registers are mapped in the standard way. If
extended then the extended mappings are used. If an extended immedaite
is provided then that immediate value is used as the offset for the
instruction, be it preindexed or postindexed.

<p>

In general the burst ('k' in the internal instruction) for a load instruction is zero. However, if a load is extended with an extcmd specifying a non-zero burst then the following loads (up to that burst length) will have a decreasing burst size down to 0.

<?php
mapping_start("Native unextended", "Internal" );
mapping( "Ldr Rd, [Rn]",      "ILDRccA #0 (Rn) -> Rd" );
mapping( "LdrH Rd, [Rn]",     "ILDRccHA #0 (Rn) -> Rd" );
mapping( "LdrB Rd, [Rn]",     "ILDRccBA #0 (Rn) -> Rd" );
mapping( "Ldr Rd, [Rn, #4]",  "ILDRccA #0 (Rn, 4) -> Rd" );
mapping( "Ldr Rd, [Rn, SHF]",  "ILDRccA #0 (Rn, SHF) -> Rd" );
mapping( "Ldr Rd, [Rn, -SHF]", "ILDRccA #0 (Rn, -SHF) -> Rd" );
mapping( "Ldr Rd, [Rn], #4",  "ILDRccA #0 (Rn), 4 -> Rd" );
mapping( "Ldr Rd, [Rn], #-4", "ILDRccA #0 (Rn), -4 -> Rd" );
mapping_end();

mapping_start("Native extended immediate", "Internal" );
mapping( "Ldr Rd, [Rn]",      "ILDRccA #0 (Rn, +imm) -> Rd" );
mapping( "LdrH Rd, [Rn]",     "ILDRccHA #0 (Rn, +imm) -> Rd" );
mapping( "LdrB Rd, [Rn]",     "ILDRccBA #0 (Rn, +imm) -> Rd" );
mapping_end();

?>

<?php page_section( "store_class", "Store instructions" ); ?>

The native store instructions are mapped to internal store
instructions. If not extended then the conditional is derived in the
standard way, and the registers are mapped in the standard way. If
extended then the extended mappings are used. If an extended immedaite
is provided then that immediate value is used as the offset for the
instruction, be it preindexed or postindexed.

<p>

In general the burst ('k' in the internal instruction) for a store instruction is zero. However, if a store is extended with an extcmd specifying a non-zero burst then the following stores (up to that burst length) will have a decreasing burst size down to 0.

<?php
mapping_start("Native unextended", "Internal" );
mapping( "Str Rd, [Rn]",       "ISTRccA #0 (Rn) <- Rm" );
mapping( "StrH Rm, [Rn]",      "ISTRccHA #0 (Rn) <- Rm" );
mapping( "StrB Rm, [Rn]",      "ISTRccBA #0 (Rn) <- Rm" );
mapping( "Str Rm, [Rn, #4]!",  "ISTRccA #0 (Rn, 4) <- Rm -> Rn" );
mapping( "Str Rm, [Rn, SHF]",  "ISTRccA #0 (Rn, SHF) <- Rm" );
mapping( "Str Rm, [Rn, -SHF]", "ISTRccA #0 (Rn, -SHF) <- Rm" );
mapping( "Str Rm, [Rn], #4",   "ISTRccA #0 (Rn), 4 <- Rm -> Rn" );
mapping( "Str Rm, [Rn], #-4",  "ISTRccA #0 (Rn), -4 <- Rm -> Rn" );
mapping_end();

mapping_start("Native extended Rd", "Internal" );
mapping( "Str Rm, [Rn, #4]!",  "ISTRccA #0 (Rn, 4) <- Rm -> Rd" );
mapping( "Str Rm, [Rn, SHF]",  "ISTRccA #0 (Rn, SHF) <- Rm -> Rd" );
mapping( "Str Rm, [Rn, -SHF]", "ISTRccA #0 (Rn, -SHF) <- Rm -> Rd" );
mapping( "Str Rm, [Rn], #4",   "ISTRccA #0 (Rn), 4 <- Rm -> Rd" );
mapping( "Str Rm, [Rn], #-4",  "ISTRccA #0 (Rn), -4 <- Rm -> Rd" );
mapping_end();

mapping_start("Native extended op (?)", "Internal" );
mapping( "Str Rm, [Rn, #4]!",  "ISTRccHA #0 (Rn, 2) <- Rm -> Rn" );
mapping( "Str Rm, [Rn, #4]!",  "ISTRccBA #0 (Rn, 1) <- Rm -> Rn" );
mapping( "Str Rm, [Rn, SHF]",  "ISTRccHA #0 (Rn, SHF) <- Rm" );
mapping( "Str Rm, [Rn, SHF]",  "ISTRccBA #0 (Rn, SHF) <- Rm" );
mapping( "Str Rm, [Rn, -SHF]", "ISTRccHA #0 (Rn, -SHF) <- Rm -> Rn" );
mapping( "Str Rm, [Rn, -SHF]", "ISTRccBA #0 (Rn, -SHF) <- Rm -> Rn" );
mapping( "Str Rm, [Rn], #4",   "ISTRccHA #0 (Rn), 2 <- Rm -> Rn" );
mapping( "Str Rm, [Rn], #4",   "ISTRccBA #0 (Rn), 1 <- Rm -> Rn" );
mapping( "Str Rm, [Rn], #-4",  "ISTRccHA #0 (Rn), -2 <- Rm -> Rn" );
mapping( "Str Rm, [Rn], #-4",  "ISTRccBA #0 (Rn), -1 <- Rm -> Rn" );
mapping_end();
?>

Any extended immediate value is ignored by store instructions. If extended with extcmd, then 'op' of 0 indicates standard operation, op of 1 indicates half-word accesses and op of 2 indicates byte accesses, as shown above.

<?php page_section( "missing", "Missing instructions" ); ?>

Missing:

<ul>

<li> Memory commands (prefetch, flush, and such like)

<li> Coprocessor/memory DMA commands

<li>Change mode

<li>Deschedule

<li>Shift to ARM mode

<li>Shift to 16-bit mode (so that ARM may issue it...)

<li>Shift to sticky/unsticky flags

<li>Condition passed shadow size

<li>Endianness

<li>Unaligned accesses

<li>Atomicity (exclusive for 'n' instructions) - note atomic in shadow of conditionals and branches

<li>Scheduler control - start other threads, test and set/clear flags, read flags

</ul>

</p>

<?php
page_ep();

include "${toplevel}web_assist/web_footer.php"; ?>

