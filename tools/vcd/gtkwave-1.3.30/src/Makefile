# Generated automatically from Makefile.in by configure.
CC = gcc
CFLAGS = -O2 -DGTK_ENABLE_BROKEN -DWAVE_USE_GTK2
AIXFLAGS = -bmaxdata:0x80000000 -D_WAVE_BE32
DEBUGS = -DDEBUG_FACILITIES -DDEBUG_PRINTF -DDEBUG_MALLOC -DSTRICT_VCD_ONLY -DDEBUG_MALLOC_LINES

SRCS = menu.c main.c status.c zoombuttons.c pagebuttons.c \
		fetchbuttons.c shiftbuttons.c pixmaps.c \
		timeentry.c currenttime.c wavewindow.c signalwindow.c \
		aet.c bitvec.c analyzer.c bsearch.c entry.c \
		tree.c file.c search.c discardbuttons.c \
		fgetdynamic.c showchange.c treesearch.c \
		markerbox.c print.c strace.c \
		simplereq.c help.c vcd.c debug.c rc.c \
		color.c hiersearch.c renderopt.c \
		rgb.c regex.c fonts.c lxt.c ae2.c ae2reader.c

OBJS = menu.o main.o status.o zoombuttons.o pagebuttons.o \
		fetchbuttons.o shiftbuttons.o pixmaps.o \
		timeentry.o currenttime.o wavewindow.o signalwindow.o \
		aet.o bitvec.o analyzer.o bsearch.o entry.o \
		tree.o file.o search.o discardbuttons.o \
		fgetdynamic.o showchange.o treesearch.o \
		markerbox.o print.o strace.o \
		simplereq.o help.o vcd.o debug.o rc.o \
		color.o hiersearch.o renderopt.o \
		rgb.o regex.o fonts.o lxt.o ae2.o ae2reader.o

all: 	gtkwave
	cp gtkwave ../bin

gtkwave:$(OBJS)
	$(CC) $(CFLAGS) `pkg-config gtk+-2.0 --libs` $(OBJS) -lz -lbz2 -o gtkwave

.c.o:	$(H_FILES)
	$(CC) $(CFLAGS) `pkg-config gtk+-2.0 --cflags` -c -o $@ $<

clean: 
	rm -f core *.core *.o gtkwave ../bin/gtkwave
