/* 
 * Copyright (c) Tony Bybell 1999.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#ifndef ANALYZER_H
#define ANALYZER_H

#include <gtk/gtk.h>
#include <stdlib.h>
#include "alloca.h"
#include "debug.h"

typedef struct _SearchProgressData {
    GtkWidget *window;
    GtkWidget *pbar;
    GtkAdjustment *adj;
    int timer;	/* might be used later.. */
    gfloat value, oldvalue;
} SearchProgressData;


typedef struct ExpandInfo *eptr;
typedef struct ExpandReferences *exptr;
typedef struct Node	  *nptr;
typedef struct HistEnt	  *hptr;
typedef struct Bits       *bptr;
typedef struct VectorEnt  *vptr;
typedef struct BitVector  *bvptr;

typedef unsigned long  Ulong;
typedef unsigned int   Uint;


typedef struct HistEnt
{
hptr next;	      /* next transition in history */
TimeType time;        /* time of transition */
unsigned char flags;  /* so far only set on glitch/real condition */

union
  {
  unsigned char val;    /* value: "0XZ1"[val] */
  char *vector;		/* pointer to a whole vector */
  } v;

} HistEnt;

enum HistEntFlagBits
{ HIST_GLITCH_B, HIST_REAL_B, HIST_STRING_B 
};

#define HIST_GLITCH (1<<HIST_GLITCH_B)
#define HIST_REAL   (1<<HIST_REAL_B)

#ifndef STRICT_VCD_ONLY
	#define HIST_STRING (1<<HIST_STRING_B)
#else
	#define HIST_STRING 0	/* for gcc -O2 optimization */
#endif

typedef struct VectorEnt
{
vptr next;
TimeType time;
unsigned char v[1];
} VectorEnt;


#define MAX_HISTENT_TIME (~( (ULLDescriptor(-1)) << (sizeof(TimeType) * 8 - 1)))


typedef struct ExpandInfo	/* only used when expanding atomic vex.. */
{
nptr	*narray;
int	msb, lsb;
int	width;
} ExpandInfo;

typedef struct ExpandReferences
{
nptr   parent;			/* which atomic vec we expanded from */
int    parentbit;		/* which bit from that atomic vec */
int    actual;			/* bit number to be used in [] */
int    refcnt;
} ExpandReferences;

typedef struct ExtNode
  {
  int msi, lsi;
  } ExtNode;


struct Node
  {
    exptr    expansion; /* indicates which nptr this node was expanded from (if it was expanded at all) and (when implemented) refcnts */
    char     *nname;	/* ascii name of node */
    ExtNode  *ext;	/* extension to node for vectors */    
    HistEnt  head;	/* first entry in transition history */
    hptr     curr;      /* ptr. to current history entry */

    hptr     *harray;   /* fill this in when we make a trace.. contains  */
			/*  a ptr to an array of histents for bsearching */
    int      numhist;	/* number of elements in the harray */
    struct fac *mvlfac; /* for use with mvlsim aets */
  };

typedef struct Bits
  {
    char    *name;		/* name of this vector of bits   */
    int     nbits;		/* number of bits in this vector */
    nptr    nodes[1];		/* pointers to the bits (nodes)  */
  } Bits;

typedef struct BitVector
  {
    char    *name;		/* name of this vector of bits           */
    int     nbits;		/* number of bits in this vector         */
    int     numregions;		/* number of regions that follow         */
    bptr    bits;		/* pointer to Bits structs for save file */
    vptr    vectors[1];		/* pointers to the vectors               */
  } BitVector;


typedef struct
  {
    TimeType    first;		/* beginning time of trace */
    TimeType    last;		/* end time of trace */
    TimeType    start;		/* beginning time of trace on screen */
    TimeType    end;		/* ending time of trace on screen */
    TimeType    marker;
    TimeType    prevmarker;	/* from last drawmarker()	        */
    TimeType    lmbcache;	/* initial marker pos                   */
    TimeType    timecache;	/* to get around floating pt limitation */
    TimeType    laststart;      /* caches last set value                */

    gdouble    	zoom;		/* current zoom  */
    gdouble    	prevzoom;	/* for zoom undo */
  } Times;

typedef struct TraceEnt *Trptr;

typedef struct
  {
    int      total;		/* total number of traces */
    Trptr    first;		/* ptr. to first trace in list */
    Trptr    last;		/* end of list of traces */
    Trptr    buffer;		/* cut/copy buffer of traces */
    Trptr    bufferlast;	/* last element of bufferchain */
    int      buffercount;	/* number of traces in buffer */
  } Traces;


typedef struct
  {
    Trptr    buffer;            /* cut/copy buffer of traces */
    Trptr    bufferlast;        /* last element of bufferchain */
    int      buffercount;       /* number of traces in buffer */
  } TempBuffer;

typedef struct TraceEnt
  {
    Trptr    next;		/* doubly linked list of traces */
    Trptr    prev;
    char     *name;		/* name stripped of path */
    char     *asciivalue;	/* value that marker points to */
    TimeType asciitime;		/* time this value corresponds with */
    TimeType shift;		/* offset added to all entries in the trace */
    char     is_alias;		/* set when it's an alias (safe to free t->name then) */
    short    vector;		/* 1 if bit vector, 0 if node */
    unsigned int flags;		/* see def below in TraceEntFlagBits */
    union
      {
	nptr    nd;		/* what makes up this trace */
	bvptr   vec;
      } n;
  } TraceEnt;


enum TraceEntFlagBits
{ TR_HIGHLIGHT_B, TR_HEX_B, TR_DEC_B, TR_BIN_B, TR_OCT_B, 
  TR_RJUSTIFY_B, TR_INVERT_B, TR_REVERSE_B, TR_EXCLUDE_B,
  TR_BLANK_B, TR_SIGNED_B, TR_ASCII_B
};
 
#define TR_HIGHLIGHT 	(1<<TR_HIGHLIGHT_B)
#define TR_HEX		(1<<TR_HEX_B)
#define TR_ASCII	(1<<TR_ASCII_B)
#define TR_DEC		(1<<TR_DEC_B)
#define TR_BIN		(1<<TR_BIN_B)
#define TR_OCT		(1<<TR_OCT_B)
#define TR_RJUSTIFY	(1<<TR_RJUSTIFY_B)
#define TR_INVERT	(1<<TR_INVERT_B)
#define TR_REVERSE	(1<<TR_REVERSE_B)
#define TR_EXCLUDE	(1<<TR_EXCLUDE_B)
#define TR_BLANK	(1<<TR_BLANK_B)
#define TR_SIGNED	(1<<TR_SIGNED_B)

#define TR_NUMMASK	(TR_ASCII|TR_HEX|TR_DEC|TR_BIN|TR_OCT|TR_SIGNED)

void DisplayTraces(int val);
int AddNode(nptr nd, char *aliasname);
int AddVector(bvptr vec);
int AddBlankTrace(char *commentname);
int InsertBlankTrace(char *comment);
void RemoveNode(nptr n);
void RemoveTrace(Trptr t, int dofree);
void FreeTrace(Trptr t);
Trptr CutBuffer(void);
void FreeCutBuffer(void);
Trptr PasteBuffer(void);
Trptr PrependBuffer(void);
int TracesAlphabetize(int mode);
int TracesReverse(void);

void import_trace(nptr np);

eptr ExpandNode(nptr n);
void DeleteNode(nptr n);
nptr ExtractNodeSingleBit(nptr n, int bit);

extern Times       tims;
extern Traces      traces;
extern unsigned int default_flags;
extern Trptr	   shift_click_trace;


/* hierarchy depths */
char *hier_extract(char *pnt, int levels);
extern int hier_max_level;

#endif
