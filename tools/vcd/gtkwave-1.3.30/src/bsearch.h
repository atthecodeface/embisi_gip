/* 
 * Copyright (c) Tony Bybell 1999.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#ifndef BSEARCH_NODES_VECTORS_H
#define BSEARCH_NODES_VECTORS_H

int bsearch_timechain(TimeType key);
hptr bsearch_node(nptr n, TimeType key);
vptr bsearch_vector(bvptr b, TimeType key);
char *bsearch_trunc(char *ascii, int maxlen);
struct symbol *bsearch_facs(char *ascii);

extern int maxlen_trunc;
extern TimeType shift_timebase, shift_timebase_default_for_add;

hptr *max_compare_index;
vptr *vmax_compare_index;

#endif


