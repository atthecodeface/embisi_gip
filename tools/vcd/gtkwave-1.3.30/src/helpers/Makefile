# Generated automatically from Makefile.in by configure.
CC = gcc
CFLAGS = -O2 -lz -lbz2
AIXFLAGS = -bmaxdata:0x80000000
DEBUGS =

all:	mvlbin

clean:
	rm -rf mvl2vcd mvl2lxt tex2vcd vcd2lxt *.core core ../../bin/mvl2vcd ../../bin/mvl2lxt ../../bin/tex2vcd ../../bin/vcd2lxt

mvlbin: mvl2vcd mvl2lxt tex2vcd vcd2lxt
	cp mvl2vcd  ../../bin; cp tex2vcd ../../bin; cp mvl2lxt  ../../bin; cp vcd2lxt  ../../bin

mvl2vcd: mvl2vcd.c
	$(CC) $(CFLAGS) -o mvl2vcd mvl2vcd.c

mvl2lxt: mvl2lxt.c lxt_write.c lxt_write.h
	$(CC) $(CFLAGS) -o mvl2lxt mvl2lxt.c lxt_write.c

tex2vcd: tex2vcd.c
	$(CC) $(CFLAGS) -o tex2vcd tex2vcd.c

vcd2lxt: vcd2lxt.c lxt_write.c lxt_write.h v2l_debug.c v2l_debug.h v2l_analyzer.h
	$(CC) $(CFLAGS) -o vcd2lxt vcd2lxt.c lxt_write.c v2l_debug.c
