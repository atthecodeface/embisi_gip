/* 
 * Copyright (c) Tony Bybell 1999.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */


/*
 * tree.h 12/05/98ajb
 */
#ifndef WAVE_TREE_H
#define WAVE_TREE_H

#include <gtk/gtk.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "debug.h"
#include "aet.h"

extern char hier_delimeter, hier_was_explicitly_set, alt_hier_delimeter;
extern char hier_grouping;
extern struct tree *selectedtree;

struct tree
{
struct tree *parent; /* null if toplevel */
struct tree *next;
struct tree *child;
char *name;
int which;	/* 'i' for facs[i] table, -1 means not a full signame */
};

struct treechain
{
struct tree *tree;	/* top of list of selected item in hierarchy */
struct tree *label;	/* actual selected item in hierarchy */
struct treechain *next;
};


extern struct tree *treeroot;

void init_tree(void);
void build_tree_from_name(char *s, int which);
int treeprune(struct tree *t);
void treedebug(struct tree *t, char *s);
void maketree(GtkCTreeNode *subtree, struct tree *t);
char *leastsig_hiername(char *nam);

#endif

