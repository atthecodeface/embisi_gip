/* 
 * Copyright (c) Tony Bybell 1999.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */


/*
 * aet.h 08/09/97ajb
 */
#ifndef AET_H
#define AET_H

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include "alloca.h"
#include "analyzer.h"
#include "currenttime.h"
#include "debug.h"
#include "tree.h"

#define SYMPRIME 65519
#define WAVE_DECOMPRESSOR "gzip -cd "	/* zcat alone doesn't cut it for AIX */


struct fac
{
int array_height;
int msb, lsb, len;
char *name;
unsigned int lastchange;
unsigned int flags;
struct Node *resolve_lxt_alias_to;
char bus;
};


struct symbol
{
struct symbol *nextinaet;/* for aet node chaining */
struct HistEnt *h;	 /* points to previous one */

struct symbol *vec_root, *vec_chain;
	
struct symbol *next;	/* for hash chain */
char *name;
char selected;		/* for the clist object */

struct Node *n;
};


struct symchain		/* for restoring state of ->selected in signal regex search */
{
struct symchain *next;
struct symbol *symbol;
};


struct symbol *symfind(char *);
struct symbol *symadd(char *, int);
int hash(char *s);

void facsplit(char *, int *, int *);
int sigcmp(char *, char *);
void quicksort(struct symbol **, int, int);

#ifdef	__NetBSD__ 
#define	heapsort my_heapsort
#endif
void heapsort(struct symbol **a, int num);

struct Bits *makevec(char *, char *);
int maketraces(char *);

int parsewavline(char *);

extern struct symbol **sym, **facs;
extern char facs_are_sorted;
extern int numfacs;
extern int regions;
extern struct symbol *firstnode;
extern struct symbol *curnode;
extern int longestname;
extern char anna_compatibility;
extern int hashcache;

/* additions to bitvec.c because of search.c/menu.c ==> formerly in analyzer.h */
bvptr bits2vector(struct Bits *b);
struct Bits *makevec_selected(char *vec, int numrows, char direction);
int add_vector_selected(char *alias, int numrows, char direction);
struct Bits *makevec_range(char *vec, int lo, int hi, char direction);
int add_vector_range(char *alias, int lo, int hi, char direction);
struct Bits *makevec_chain(char *vec, struct symbol *sym, int len);
int add_vector_chain(struct symbol *s, int len);
char *makename_chain(struct symbol *sym);

#endif
