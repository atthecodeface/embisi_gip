<?php

include "web_locals.php";
include "${toplevel}web_assist/web_globals.php";

site_set_location($site_location);

include "${toplevel}web_assist/web_header.php";

page_header( $page_title );

page_sp();
?>

<?php page_section( "overview", "Overview" ); ?>

The postbus is the processor and I/O interconnect system used between
GIPs, data processors, and I/O. It is basically a high speed,
blocking, switchable packet bus system. In simple terms, lumps of data
can be transferred from one point to another through a simple fabric
of registers, and optionally switches. Each endpoint has an outgoing
interface which indicates data is available to be sent and provides
formatted lumps of data, and each endpoint has an incoming interface
which indicates it can take data.

<p>

In its simplest form two endpoints can be tied together back-to-back;
that is, their repective outgoing interfaces can be wired without
logic to each other's incoming interfaces.

<p>

<?php page_section( "high_level_protocol", "High level protocol" ); ?>

Each transaction on the postbus consists of a number of words of data
(one or more), each 32-bits, the first of which is a control word
(which includes destination and user information). The transaction
word is framed with an accompanying word type indicator: idle/hold (no
data), data (more to come, data in this timeslot), start data (first
word of transaction ready, possibly the only word), last data (data in
this timeslot). The high level protocol is then sequences of
transaction words, of the form:

<br>

( ((Start multi (Hold* Data*)* Last) | (Start single) | Idle )*

<br>

The format of the transaction words within, the transaction header word, is:

<?php

bit_breakout_start( 32, "Transaction" );

bit_breakout_hdr( "Start" );
bit_breakout_bits( 19, "User data" );
bit_breakout_bits( 4, "(Target port)" );
bit_breakout_bits( 4, "(Length)" );
bit_breakout_bits( 4, "Target address" );
bit_breakout_bits( 1, "Single word" );
bit_breakout_bits( -1, "Routing header word, bracketed items recommended but not required" );

bit_breakout_hdr( "Data*" );
bit_breakout_bits( 32, "User data" );
bit_breakout_bits( -1, "Optional data words" );

bit_breakout_hdr( "Hold*" );
bit_breakout_bits( 32, "<i>Ignored</i>" );
bit_breakout_bits( -1, "Pauses the transaction" );

bit_breakout_hdr( "Last" );
bit_breakout_bits( 32, "User data" );
bit_breakout_bits( -1, "Optional last data word" );

bit_breakout_end();

?>

A single word transaction is indicated by a Start transaction word
with bit 0 of its data set; a multi-word transaction is indicated by a
Start transaction word with bit 0 of its data clear, and it requires
zero or more data/hold words and a Last transaction word to follow it.

<?php page_section( "routing", "Routing" ); ?>

Routing is accomplished by looking at the target address bits in the
Start transaction word. The simple fabric will try to form a route
through the fabric to the target, and if it can do so it will. That
route is then held exclusively for that packet, until it has been
fully transferred. The targets indicate that they can accept a word of
data. The bus therefore supports hop-to-hop flow control, but it does
not support higher level end-to-end flow control, and it is a blocking
fabric. This means that data should only be sent to a target address
if it is known that the target will take the data, else the fabric may
block and prevent other clients from using the fabric.

<p>

Target addresses are defined for the system in silicon, and are thus
hardware defined, not soft. The routing fabric knows its topology and
its capabilities, and can manage transfers as it wishes with those
bounds. The fabric can be a simple many-to-many bus, or it may be a
multistage register system with simple muxes, or a complete star
fabric, or even a ring. The endpoint interface is not effected by this choice.

<?php page_section( "hardware_interface", "Hardware interface" ); ?>

The hardware interfaces into and out of the fabric are well defined,
with signals specified as early or late in the clock cycle. Early
signals are required to be generated from a register with up to 2
layers of simple (nand) logic on the output of the register. Late
signals may go through more logic, and are expected to go through at
most 2 layers of simple (nand) logic prior to the clock edge. Medium
arrival signals are expected to be driven by early arrival signals,
and may generate a late signal, using a small amount of propagation
time. For complex systems, though, propagation delays are expected to
be too large for medium signals to be used; only simple systems can
use medium signals.

<?php page_subsection( "into", "Signal definition for data in to fabric" ); ?>

<?php

signal_list("Ports on postbus source");
signal_output( "Type", "2", "early", "Type of transaction word" );
signal_output( "Data", "32", "early", "Data for transaction word" );
signal_input( "Ack", "1", "late", "Acknowledge data presented in this cycle" );
signal_end();

signal_list("Ports on postbus target");
signal_output( "Ack", "1", "early", "Will take presented data" );
signal_input( "Type", "2", "late", "Type of transaction word" );
signal_input( "Data", "32", "late", "Data for transaction word" );
signal_end();

signal_list("Ports on simple postbus router");
signal_input( "n SrcType", "2*n", "medium", "Type of transaction word (one per source)" );
signal_input( "n SrcData", "32*n", "medium", "Data for transaction word (one per source)" );
signal_input( "m TgtAck", "m", "medium", "Can take presented data" );
signal_output( "SrcAck", "n", "medium", "Is taking presented data; combinatorial (one per source)" );
signal_output( "TgtType", "2*n", "medium", "Type of transaction word" );
signal_output( "TgtData", "32", "medium", "Data for transaction word (to all n targets)" );
signal_end();

signal_list("Ports on complex registered postbus router");
signal_input( "n SrcType", "2*n", "late", "Type of transaction word (one per source)" );
signal_input( "n SrcData", "32*n", "late", "Data for transaction word (one per source)" );
signal_input( "m TgtAck", "m", "late", "Can take presented data" );
signal_output( "SrcAck", "n", "early", "Is taking presented data; combinatorial (one per source)" );
signal_output( "TgtType", "2*m", "early", "Type of transaction word" );
signal_output( "TgtData", "32*m", "early", "Data for transaction word" );
signal_end();

?>

<?php page_section( "simple_implementation", "Simple implementation" ); ?>

A simple implementation of a postbus router takes a small number of
sources, say 4, and distributes data to a small
number of targets, say 8. The simple
implementation has a state machine, and it multiplexes its incoming
4 sets of source data together to present to all 8 targets.

<p>

<?php code_format( "cdl", "cdl/simple.cdl"); ?>

<?php
page_ep();

include "${toplevel}web_assist/web_footer.php"; ?>
