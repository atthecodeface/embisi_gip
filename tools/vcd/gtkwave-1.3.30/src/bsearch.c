/* 
 * Copyright (c) Tony Bybell 1999.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include "analyzer.h"
#include "currenttime.h"
#include "aet.h"
#include "bsearch.h"
#include "strace.h"

TimeType shift_timebase=LLDescriptor(0);
TimeType shift_timebase_default_for_add=LLDescriptor(0);
static TimeType max_compare_time_tc;
static TimeType *max_compare_pos_tc;

static int compar_timechain(const void *s1, const void *s2)
{
TimeType key, obj, delta;
TimeType *cpos;
int rv;

key=*((TimeType *)s1);
obj=*(cpos=(TimeType *)s2);

if((obj<=key)&&(obj>max_compare_time_tc))
	{
	max_compare_time_tc=obj;
	max_compare_pos_tc=cpos;
	}

delta=key-obj;
if(delta<0) rv=-1;
else if(delta>0) rv=1;
else rv=0;

return(rv);
}

int bsearch_timechain(TimeType key)
{
max_compare_time_tc=-2+shift_timebase; max_compare_pos_tc=NULL; 

if(!timearray) return(-1);

bsearch(&key, timearray, timearray_size, sizeof(TimeType), compar_timechain);
if((!max_compare_pos_tc)||(max_compare_time_tc<shift_timebase)) 
	{
	max_compare_pos_tc=timearray; /* aix bsearch fix */
	}

return(max_compare_pos_tc-timearray);
}

/*****************************************************************************************/
static TimeType max_compare_time;
static hptr max_compare_pos;
hptr *max_compare_index;

static int compar_histent(const void *s1, const void *s2)
{
TimeType key, obj, delta;
hptr cpos;
int rv;

key=*((TimeType *)s1);
obj=(cpos=(*((hptr *)s2)))->time+shift_timebase;

if((obj<=key)&&(obj>max_compare_time))
	{
	max_compare_time=obj;
	max_compare_pos=cpos;
	max_compare_index=(hptr *)s2;
	}

delta=key-obj;
if(delta<0) rv=-1;
else if(delta>0) rv=1;
else rv=0;

return(rv);
}

hptr bsearch_node(nptr n, TimeType key)
{
max_compare_time=-2+shift_timebase; max_compare_pos=NULL; max_compare_index=NULL;

bsearch(&key, n->harray, n->numhist, sizeof(hptr), compar_histent);
if((!max_compare_pos)||(max_compare_time<shift_timebase)) 
	{
	max_compare_pos=n->harray[1]; /* aix bsearch fix */
	max_compare_index=&(n->harray[1]); 
	}

return(max_compare_pos);
}

/*****************************************************************************************/

static TimeType vmax_compare_time;
static vptr vmax_compare_pos;
vptr *vmax_compare_index;

static int compar_vectorent(const void *s1, const void *s2)
{
TimeType key, obj, delta;
vptr cpos;
int rv;

key=*((TimeType *)s1);
obj=(cpos=(*((vptr *)s2)))->time+shift_timebase;

if((obj<=key)&&(obj>vmax_compare_time))
	{
	vmax_compare_time=obj;
	vmax_compare_pos=cpos;
        vmax_compare_index=(vptr *)s2;
	}

delta=key-obj;
if(delta<0) rv=-1;
else if(delta>0) rv=1;
else rv=0;

return(rv);
}

vptr bsearch_vector(bvptr b, TimeType key)
{
vmax_compare_time=-2+shift_timebase; vmax_compare_pos=NULL; vmax_compare_index=NULL;

bsearch(&key, b->vectors, b->numregions, sizeof(vptr), compar_vectorent);
if((!vmax_compare_pos)||(vmax_compare_time<shift_timebase)) 
	{
	vmax_compare_pos=b->vectors[1]; /* aix bsearch fix */
	vmax_compare_index=&(b->vectors[1]);
	}

return(vmax_compare_pos);
}

/*****************************************************************************************/

int maxlen_trunc;
static char *maxlen_trunc_pos;
static char *trunc_asciibase;

static int compar_trunc(const void *s1, const void *s2)
{
char *str;
char vcache[2];
int key, obj;

str=(char *)s2;
key=*((int*)s1);

vcache[0]=*str;
vcache[1]=*(str+1);
*str='+';
*(str+1)=0;
obj=gdk_string_measure(wavefont,trunc_asciibase);
*str=vcache[0];
*(str+1)=vcache[1];

if((obj<=key)&&(obj>maxlen_trunc))
        {
        maxlen_trunc=obj;
        maxlen_trunc_pos=str;
        }

return(key-obj);
}


char *bsearch_trunc(char *ascii, int maxlen)
{
int len;

if((maxlen<=0)||(!ascii)||(!(len=strlen(ascii)))) return(NULL);

maxlen_trunc=0; maxlen_trunc_pos=NULL;

bsearch(&maxlen, trunc_asciibase=ascii, len, sizeof(char), compar_trunc);
return(maxlen_trunc_pos);
}

/*****************************************************************************************/

static int compar_facs(const void *key, const void *v2)
{
struct symbol *s2;
int rc;

s2=*((struct symbol **)v2);
rc=sigcmp((char *)key,s2->name);
return(rc);
}

struct symbol *bsearch_facs(char *ascii)
{
struct symbol **rc;

if ((!ascii)||(!strlen(ascii))) return(NULL);
rc=(struct symbol **)bsearch(ascii, facs, numfacs, sizeof(struct symbol *), compar_facs);
if(rc) return(*rc); else return(NULL);
}
