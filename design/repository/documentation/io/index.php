<?php

include "web_locals.php";
include "${toplevel}web_assist/web_globals.php";

site_set_location($site_location);

include "${toplevel}web_assist/web_header.php";

page_header( $page_title );

page_sp();
?>

<?php page_section( "overview", "Overview" ); ?>

The IO blocks in the product are designed to support 20 I/O pins, utilizing up to four of those pins as input clocks. The basic concept is that on average 5 pins or more are used for an I/O interface.

Each I/O interface requires up to one data FIFO, one command FIFO, and one status FIFO. With 20 pins per I/O block, and 5 pins or more average per I/O, this means that each I/O block must support four command, four status, and four data FIFOs.

The supported I/O types may be one of:

<ul>

<li>10/100 Ethernet TX (MII or RMII)

<li>
10/100 Ethernet RX (MII or RMII)

<li>

16-bit address/data ISA (Intel/Motorola bus) style target

<li>

16-bit address latch for ISA target

<li>

General purpose input, sampled at particular clock frequency

<li>

General purpose input, sampled at a particular clock frequency, and reported on value change

<li>

HSS bus (up to 8 bits), with fairly arbitrary framing, capturing on an input clock frequency or that divided down, with either clock edge for data sampling.

<li>

I2S bus for audio input

<li>

I2S bus for audio output

<li>

UART Rx with simple hardware handshaking, bytes presented as status, up to 13 bits per 'byte'

</ul>

The plan for the I/O block is to have a block that contains four clock baud enable generators, which may be set individually to arbitrary divides, which may be clocked on every input clock edge (where their input clock is selectable from internal or one of the four I/O clock pins) or every externally enabled clock edge.

<p>


Questions:

<ul>

<li>Can we be a PCMCIA target with the above pins?


</ul>


<p>

In its simplest form two endpoints can be tied together back-to-back;
that is, their repective outgoing interfaces can be wired without
logic to each other's incoming interfaces.

<p>

<?php page_section( "baud_rate_generator", "Baud rate generators" ); ?>

The baud rate generators are individually configurable, and run from one of six potential clock sources:

<ul>

<li>
No clock (low power)

<li>
Internal clock

<li>
I/O clock pin 0

<li>
I/O clock pin 1

<li>
I/O clock pin 2

<li>
I/O clock pin 3

</ul>

The baud rate generator may also clock only when its particular clock is enabled, such that an external clock divide circuit may be subdivided down with the baud rate generator.

<p>

The baud rate generator then uses an error function to provide a 'divide-by-(n/m)' clock enable. The values of 'n' and 'm' are inputs to each baud rate generator, and are part of its configuration. Additionally, the baud rate generator has a reset input that resets its divider asynchronously, so that the divide-by unit may be started when an external input fires, providing for 'every nth' input value style capture.

<p>

The divider and multiplier are 14-bit values (plus 1, see design details). This allows for a 200MHz clock to be divided down to get at least 1200baud*16 oversampling, for a 1200 baud UART. It thus supports divide by 2^14 (16384), or about 12kHz.

<p>

The baud rate generators may be stacked so that the output enable of one is passed to the next as its enable; note that in this case the output of the second BRG needs to be ANDed with the output of the first BRG to get the actual enable.

<?php
page_ep();

include "${toplevel}web_assist/web_footer.php"; ?>
