<?php

include "web_locals.php";
include "${toplevel}web_assist/web_globals.php";

site_set_location($site_location);

include "${toplevel}web_assist/web_header.php";

page_header( $page_title );

page_sp();
?>

<?php page_section( "overview", "Overview" ); ?>

For the FFT we need to be able to take in real data and perform an FFT

The real data can be munged to produce the outcome of the first two
rounds of FFTs with just adds. If each rounds is thought of as
utilizing a 4-cycle multiply, then we can do the first round in just
one round time, and use 4-cycles for the work

The first round of an FFT does:

out0 = in0 + in1
out1 = in0 - in1

The first two rounds of an FFT do:

out0 = in0 + in2 + in1 + in3
out2 = in0 + in2 - in1 - in3
out1 = in0 - in2 + i.in1 - i.in3
out3 = in0 - in2 - i.in1 + i.in3

If the inputs are real then the outputs are:
out0 = (in0+in2) + (in1+in3) + i . 0
out1 = (in0-in2)             + i . (in1-in3)
out2 = (in0+in2) - (in1+in3) + i . 0
out3 = (in0-in2)             - i . (in1-in3)

The first two rounds of the FFT can then generate the required
results:
Cycle 1 - (in0+in2), (in0-in2)
Cycle 2 - (in0+in2)+(in1+in3), (in0+in2)-(in1+in3)
Cycle 3 - (in1-in3)


For a 32-point FFT we can store the data linearly in m0 through m31.

The first round pair then use (in this order):
 m0, m8,m16,m24 -> r2.0, r2.8, r2.16, r2.24 etc.
 m1, m9,m17,m25
 m2,m10,m18,m26
 m3,m11,m19,m27
 m4,m12,m20,m28
 m5,m13,m21,m29
 m6,m14,m22,m30
 m7,m15,m23,m31

r2.0  = m0+m8+m16+m24
r2.8  = m0-m8+m16-m24
r2.16 = m0+i.m8-m16-i.m24
r2.24 = m0-i.m8-m16+i.m24
and similarly for r2.1 thru r2.7, etc.
i.e.
r2.0  = Sum(i=0,8..) e(0.i/4)mi
r2.8  = Sum(i=0,8..) e(2.i/4)mi
r2.16 = Sum(i=0,8..) e(1.i/4)mi
r2.24 = Sum(i=0,8..) e(3.i/4)mi

The next round then uses
r2.0, r2.4  -> r3.0,  r3.4  (butterfly with 0/8, 4/8) etc thru r2.3/7
r2.8, r2.12 -> r3.8,  r3.12 (butterfly with 2/8, 6/8) etc thru r2.11/15
r2.16, r2.20-> r3.16, r3.20 (butterfly with 1/8, 5/8) etc thru r2.19/23
r2.24, r2.28-> r3.24, r3.28 (butterfly with 3/8, 7/8) etc thru r2.27/31
where
r3.0 = r2.0 + r2.4        = m0+m4+m8+m12+m16+m20+m24+m28
r3.4 = r2.0 - r2.4        = m0-m4+m8-m12+m16-m20+m24-m28
r3.8 = r2.8 + (1/8)r2.12  = m0+(2/8)m4+(4/8)m8+(6/8)m12+m16+(2/8)m20+(4/8)m24+(6/8)m28
r3.12= r2.8 - (1/8)r2.12  = m0+(6/8)m4+(4/8)m8+(2/8)m12+m16+(6/8)m20+(4/8)m24+(2/8)m28
r3.16= r2.16 + (2/8)r2.20 = m0+(1/8)m4+(2/8)m8+(3/8)m12+(4/8)m16+(5/8)m20+(6/8)i.m24+(7/8)m28
r3.20= r2.16 - (2/8)r2.20 = m0+(5/8)m4+(2/8)m8+(7/8)m12+(4/8)m16+(1/8)m20+(6/8)i.m24+(3/8)m28
r3.24= r2.24 + (3/8)r2.28 = m0+(3/8)m4+(6/8)m8+(1/8)m12+(4/8)m16+(7/8)m20+(2/8)i.m24+(5/8)m28
r3.28= r2.24 - (3/8)r2.28 = m0+(7/8)m4+(6/8)m8+(5/8)m12+(4/8)m16+(3/8)m20+(2/8)i.m24+(1/8)m28

i.e.
r3.0  = Sum(i=0,4,8..) e(0.i/8)mi
r3.4  = Sum(i=0,4,8..) e(4.i/8)mi
r3.8  = Sum(i=0,4,8..) e(2.i/8)mi
r3.12 = Sum(i=0,4,8..) e(6.i/8)mi
r3.16 = Sum(i=0,4,8..) e(1.i/8)mi
r3.20 = Sum(i=0,4,8..) e(5.i/8)mi
r3.24 = Sum(i=0,4,8..) e(3.i/8)mi
r3.28 = Sum(i=0,4,8..) e(7.i/8)mi

The next round then uses
r3.0,  r3.2 -> r4.0,  r4.2  (butterfly with 0/16, 8/16)
r3.4,  r3.6 -> r4.4,  r4.6  (butterfly with 4/16, 12/16)
r3.8,  r3.10-> r4.8,  r4.10 (butterfly with 2/16, 10/16)
r3.12, r3.14-> r4.12, r4.14 (butterfly with 6/16, 14/16)
r3.16, r3.18-> r4.16, r4.18 (butterfly with 1/16, 9/16)
r3.20, r3.22-> r4.20, r4.22 (butterfly with 5/16, 13/16)
r3.24, r3.26-> r4.24, r4.26 (butterfly with 3/16, 11/16)
r3.28, r3.30-> r4.28, r4.30 (butterfly with 7/16, 15/16)

So...
r4.0 = r3.0 + (0/16)r3.2   = m0+m2+m4+m6+m8+m10+m12+m14+m16+m18+m20+m22+m24+m26+m28+m30
r4.2 = r3.0 - (0/16)r3.2   = m0-m2+m4-m6+m8-m10+m12-m14+m16-m18+m20-m22+m24-m26+m28-m30
r4.4 = r3.4 + (4/16)r3.6   = m0+(4/16)m2+(8/16)m4+(12/16)m6+m8+(4/16)m10+(8/16)m12+...
r4.6 = r3.4 - (4/16)r3.6   = m0+(12/16)m2+(8/16)m4+(4/16)m6+m8+(12/16)m10+(8/16)m12+...
r4.8 = r3.8 + (2/16)r3.10  = m0+(2/16)m2+(4/16)m4+(6/16)m6+(8/16)m8+(10/16)m10+(12/16)m12+...
r4.10= r3.8 - (2/16)r3.10  = m0+(10/16)m2+(4/16)m4+(14/16)m6+(8/16)m8+(2/16)m10+(12/16)m12+...
i.e.
r4.0  = Sum(i=0,2,4..) e(0.i/16)mi
r4.2  = Sum(i=0,2,4..) e(8.i/16)mi
r4.4  = Sum(i=0,2,4..) e(4.i/16)mi
r4.6  = Sum(i=0,2,4..) e(12.i/16)mi
r4.8  = Sum(i=0,2,4..) e(2.i/16)mi
r4.10 = Sum(i=0,2,4..) e(10.i/16)mi
r4.12 = Sum(i=0,2,4..) e(6.i/16)mi
r4.14 = Sum(i=0,2,4..) e(14.i/16)mi
r4.16 = Sum(i=0,2,4..) e(1.i/16)mi
r4.18 = Sum(i=0,2,4..) e(9.i/16)mi
r4.20 = Sum(i=0,2,4..) e(5.i/16)mi
r4.22 = Sum(i=0,2,4..) e(13.i/16)mi
r4.24 = Sum(i=0,2,4..) e(3.i/16)mi
r4.26 = Sum(i=0,2,4..) e(11.i/16)mi
r4.28 = Sum(i=0,2,4..) e(7.i/16)mi
r4.30 = Sum(i=0,2,4..) e(15.i/16)mi

The final round runs with butterflies 0/16, 8/24, 4/20, 12/28, 2/18,
10/26, 6/20, 14/30, 1/17, 9/25, 5/21, 13/29, 3/19, 11/27, 7/21, 15/31.

The final results will then be r5.0 thru r5.31, with:

r5.n = Sum(i=0,1..31) e(br5(n).i/32)mi,
where br5(n) is 5-bit bit-reverse of n

Thus the result is an FFT that is bit-reverse addressed.

We need a fast mechanism for calculating the power of the FFT (squared
is fine, but assist for sqrt would actually be nice)

For this FFT to run fast we need a e^iwt ROM and a dual-read, single
write data RAM. You could time-multiplex the data RAM so that real and
imaginary components are accessed for reads on opposite cycles to
writes, so needing a read/write port and a read port; if built like
this you could envisage filling the RAM through the spare write port.
The FFT requires a butterfly MAC; this takes two data inputs and a
coefficient and generates a pair of outputs:

out0 = in0 + coeff * in1
out1 = in0 - coeff * in1

For a complex FFT, of course, this is four multiplies (coeff will be a
sin and cos pair).

To implement a sqrt we can do the software bit-divide approach
Take a 32-bit number to sqrt... it must be positive!
Take 2^15, square it, and see if the result is larger than the number
we want to square root

If it is, then put 0 in the result bit 15, else put 1.

Then move to bit 14.

Note that r^2 and (r+(1<<n))^2 differ by (1<<2n)+(r<<(n+1)), so
developing the square of the result one bit at a time is easy.

To do two bits at a time you could look at bits 15 and 14:

00...... ........ ^2 = 00000000 00000000 00000000 00000000
01...... ........ ^2 = 00010000 00000000 00000000 00000000
10...... ........ ^2 = 01000000 00000000 00000000 00000000
11...... ........ ^2 = 10010000 00000000 00000000 00000000

So the first stage is just some ORs, not even a subtract...
Actually, let's try the top 4 bits...
0000 ^2 = 0000 0000
0001 ^2 = 0000 0001
0010 ^2 = 0000 0100
0011 ^2 = 0000 1001
0100 ^2 = 0001 0000
0101 ^2 = 0001 1001
0110 ^2 = 0010 0100
0111 ^2 = 0011 0001
1000 ^2 = 0100 0000
1001 ^2 = 0101 0001
1010 ^2 = 0110 0100
1011 ^2 = 0111 1001
1100 ^2 = 1001 0000
1101 ^2 = 1010 1001
1110 ^2 = 1100 0100
1111 ^2 = 1110 0001

The bottom nybble is always 0, 1, 4, or 9 - so that is 4 simple
comparators. Then to determine the top 4 bits of result from the top 8
bits of operand requires a simple test of the top 4 bits (effectively
from a ROM) coupled with the result of the comparator of the next 4
bits.

With the top 4 bits of result, then tackle the remainder 2 bits at a
time:

rrrr0000 00000000 ^2 = RRRRRRRR 00000000 00000000 00000000
rrrr0100 00000000 ^2 = RRRRRRRR 00000000 00000000 00000000 +
                       00000000 00010000 00000000 00000000 +
                       00000rrr r0000000 00000000 00000000
rrrr1000 00000000 ^2 = RRRRRRRR 00000000 00000000 00000000 +
                       00000000 01000000 00000000 00000000 +
                       0000rrrr 00000000 00000000 00000000
rrrr1100 00000000 ^2 = RRRRRRRR 00000000 00000000 00000000 +
                       00000000 10010000 00000000 00000000 +
                       0000rrrr 00000000 00000000 00000000 +
                       00000rrr r0000000 00000000 00000000 +
This requires three 32-bit adders to generate the new squared result, and 3
32-bit comparators to determine the actual bits if the square root.
Six iterations are then required; this makes a total of seven cycles
for a square root, but it can run in parallel with a MAC.
Can we use a sort of data-flow DSP here?

<?php
page_ep();

include "${toplevel}web_assist/web_footer.php"; ?>
