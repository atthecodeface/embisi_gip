/* 
 * Copyright (c) Tony Bybell 1999.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include <stdio.h>
#include <stdlib.h>
#include "aet.h"
#include "lxt.h"
#include "debug.h"
#include "bsearch.h"
#include "strace.h"

#ifdef _MSC_VER
#define strcasecmp _stricmp
#endif

unsigned int default_flags=TR_RJUSTIFY;
Times        tims = {0, 0, 0, 0, 0, 0, 0};
Traces       traces = {0, NULL, NULL, NULL, NULL, 0};

/*
 * extract last n levels of hierarchy
 */
int hier_max_level=0;

char *hier_extract(char *pnt, int levels)
{
int i, len;
char ch, *pnt2;
char only_nums_so_far=1;

if(!pnt) return(NULL);

len=strlen(pnt);
if(!len) return(pnt);

if(levels<1) levels=1;

pnt2=pnt+len-1;
ch=*pnt2;

for(i=0;i<len;i++)
	{
	ch=*(pnt2--);
	if((only_nums_so_far)&&(ch>='0')&&(ch<='9'))		/* skip 1st set of signal.number hier from right if it exists */
		{
		continue;
		/* nothing */
		}
	else
		{
		if(ch==hier_delimeter) 
			{
			if(!only_nums_so_far) levels--;
			if(!levels)
				{
				pnt2+=2;
				return(pnt2);
				}
			}
		only_nums_so_far=0;
		}
	}

return(pnt); /* not as many levels as max, so give the full name.. */
}


/* 
 * Add a trace to the display...
 */
static void AddTrace( Trptr t )
{
if(default_flags&TR_NUMMASK) t->flags=default_flags;
	else t->flags=(t->flags&TR_NUMMASK)|default_flags;

if(shift_timebase_default_for_add)
	t->shift=shift_timebase_default_for_add;

if(!shadow_active)
	{
	if( traces.first == NULL )
		{
		t->next = t->prev = NULL;
		traces.first = traces.last = t;
	      	}
	    	else
	      	{
		t->next = NULL;
		t->prev = traces.last;
		traces.last->next = t;
		traces.last = t;
	      	}
	traces.total++;
	}
	else	/* hide offscreen */
	{
	struct strace *st = calloc_2(1, sizeof(struct strace));
	st->next = shadow_straces;
	st->value = shadow_type;
	st->trace = t;

	st->string = shadow_string;	/* copy string over */
	shadow_string = NULL;

	shadow_straces = st;

	}
}


/*
 * Add a blank trace to the display...
 */
static char *precondition_string(char *s)
{
int len=0;
char *s2;

if(!s) return(NULL);
s2=s;
while((*s2)&&((*s2)!='\n')&&((*s2)!='\r'))	/* strip off ending CR/LF */
	{
	len++;
	s2++;
	}
if(!len) return(NULL);
s2=(char *)calloc_2(1,len+1);
memcpy(s2,s,len);
return(s2);
}

int AddBlankTrace(char *commentname)
{
Trptr  t;
char *comment;

if( (t = (Trptr) calloc_2( 1, sizeof( TraceEnt ))) == NULL )
	{
	fprintf( stderr, "Out of memory, can't add blank trace to analyzer\n");
	return( 0 );
      	}
AddTrace(t);
t->flags=TR_BLANK;

if((comment=precondition_string(commentname)))
	{
	t->name=comment;
	t->is_alias=1;
	}

return(1);
}


/*
 * Insert a blank [or comment] trace into the display...
 */
int InsertBlankTrace(char *comment)
{
TempBuffer tb;
char *comm;
Trptr  t;

if( (t = (Trptr) calloc_2( 1, sizeof( TraceEnt ))) == NULL )
	{
	fprintf( stderr, "Out of memory, can't insert blank trace to analyzer\n");
	return( 0 );
      	}
t->flags=TR_BLANK;

if((comm=precondition_string(comment)))
	{
	t->name=comm;
	t->is_alias=1;
	}

if(!traces.first)
	{
	traces.first=traces.last=t;
	traces.total=1;
	return(1);
	}
	else
	{
	tb.buffer=traces.buffer;
	tb.bufferlast=traces.bufferlast;
	tb.buffercount=traces.buffercount;
	
	traces.buffer=traces.bufferlast=t;
	traces.buffercount=1;
	PasteBuffer();

	traces.buffer=tb.buffer;
	traces.bufferlast=tb.bufferlast;
	traces.buffercount=tb.buffercount;

	return(1);
	}
}


/*
 * Adds a single bit signal to the display...
 */
int AddNode(nptr nd, char *aliasname)
  {
    Trptr  t;
    hptr histpnt;
    hptr *harray;
    int histcount;
    int i;

    if(!nd) return(0); /* passed it a null node ptr by mistake */
    if(nd->mvlfac) import_trace(nd);

    signalwindow_width_dirty=1;
    
    if( (t = (Trptr) calloc_2( 1, sizeof( TraceEnt ))) == NULL )
      {
	fprintf( stderr, "Out of memory, can't add %s to analyzer\n",
	  nd->nname );
	return( 0 );
      }

if(!nd->harray)		/* make quick array lookup for aet display */
	{
	histpnt=&(nd->head);
	histcount=0;

	while(histpnt)
		{
		histcount++;
		histpnt=histpnt->next;
		}

	nd->numhist=histcount;
	
	if(!(nd->harray=harray=(hptr *)malloc_2(histcount*sizeof(hptr))))
		{
		fprintf( stderr, "Out of memory, can't add %s to analyzer\n",
		  	nd->nname );
		free_2(t);
		return(0);
		}

	histpnt=&(nd->head);
	for(i=0;i<histcount;i++)
		{
		*harray=histpnt;

		/* printf("%s, time: %d, val: %d\n", nd->nname, 
			(*harray)->time, (*harray)->val); */

		harray++;
		histpnt=histpnt->next;
		}
	}

if(aliasname)
	{	
	char *alias;

	t->name=alias=(char *)malloc_2((strlen(aliasname)+2)+1);
	strcpy(alias,"+ "); /* use plus sign to mark aliases */
	strcpy(alias+2,aliasname);
        t->is_alias=1;	/* means can be freed later */
	}
	else
	{
    	if(!hier_max_level) 
		{
		t->name = nd->nname;
		}
		else
		{
		t->name = hier_extract(nd->nname, hier_max_level);
		}
	}

    if(nd->ext)	/* expansion vectors */
	{	
	int n;

	n = nd->ext->msi - nd->ext->lsi;
	if(n<0)n=-n;
	n++;

	t->flags = (( n > 3 )||( n < -3 )) ? TR_HEX|TR_RJUSTIFY : TR_BIN|TR_RJUSTIFY;
	}
	else
	{
	t->flags |= TR_BIN;	/* binary */
	}
    t->vector = FALSE;
    t->n.nd = nd;
    AddTrace( t );
    return( 1 );
  }


/*
 * Adds a vector to the display...
 */
int AddVector( bvptr vec )
  {
    Trptr  t;
    int    n;

    if(!vec) return(0); /* must've passed it a null pointer by mistake */

    signalwindow_width_dirty=1;

    n = vec->nbits;
    t = (Trptr) calloc_2(1, sizeof( TraceEnt ) );
    if( t == NULL )
      {
	fprintf( stderr, "Out of memory, can't add %s to analyzer\n",
	  vec->name );
	return( 0 );
      }

    if(!hier_max_level)
	{	
    	t->name = vec->name;
	}
	else
	{
	t->name = hier_extract(vec->name, hier_max_level);
	}
    t->flags = ( n > 3 ) ? TR_HEX|TR_RJUSTIFY : TR_BIN|TR_RJUSTIFY;
    t->vector = TRUE;
    t->n.vec = vec;
    AddTrace( t );
    return( 1 );
  }


/*
 * Free up a trace's mallocs...
 */
void FreeTrace(Trptr t)
{
if(straces)
	{
	struct strace_defer_free *sd = calloc_2(1, sizeof(struct strace_defer_free));
	sd->next = strace_defer_free_head;
	sd->defer = t;

	strace_defer_free_head = sd;
	return;
	}

if(t->vector)
      	{
      	bvptr bv;
	int i;

	bv=t->n.vec;
	for(i=0;i<bv->numregions;i++)
		{
		if(bv->vectors[i]) free_2(bv->vectors[i]);
		}
	
	if(bv->bits)
		{
		if(bv->bits->name) free_2(bv->bits->name);
		for(i=0;i<bv->nbits;i++)
			{
			DeleteNode(bv->bits->nodes[i]);
			}
		free_2(bv->bits);
		}

	if(bv->name) free_2(bv->name);
      	if(t->n.vec)free_2(t->n.vec);
      	}
	else
	{
	if(t->n.nd && t->n.nd->expansion)
		{
		DeleteNode(t->n.nd);
		}
	}

if(t->asciivalue) free_2(t->asciivalue);
if((t->is_alias)&&(t->name)) free_2(t->name);

free_2( t );
}


/*
 * Remove a trace from the display and optionally 
 * deallocate its memory usage...
 */ 
void RemoveTrace( Trptr t, int dofree )
  {
    traces.total--;
    if( t == traces.first )
      {
	traces.first = t->next;
	if( t->next )
            t->next->prev = NULL;
        else
            traces.last = NULL;
      }
    else
      {
        t->prev->next = t->next;
        if( t->next )
            t->next->prev = t->prev;
        else
            traces.last = t->prev;
      }
    
    if(dofree)
	{
        FreeTrace(t);
	}
  }


/*
 * Deallocate the cut/paste buffer...
 */
void FreeCutBuffer(void)
{
Trptr t, t2;

t=traces.buffer;

while(t)
	{
	t2=t->next;
	FreeTrace(t);
	t=t2;	
	}

traces.buffer=traces.bufferlast=NULL;
traces.buffercount=0;
}


/*
 * Cut highlighted traces from the main screen
 * and throw them in the cut buffer.  If anything's
 * in the cut buffer, deallocate it first...
 */
Trptr CutBuffer(void)
{
Trptr t, tnext;
Trptr first=NULL, current=NULL;

shift_click_trace=NULL;		/* so shift-clicking doesn't explode */

t=traces.first;
while(t)
	{
	if((t->flags)&(TR_HIGHLIGHT)) break;
	t=t->next;
	}
if(!t) return(NULL);	/* keeps a double cut from blowing out the buffer */

signalwindow_width_dirty=1;

FreeCutBuffer();
t=traces.first;

while(t)
	{
	tnext=t->next;
	if(t->flags&TR_HIGHLIGHT)
		{
		traces.bufferlast=t;
		traces.buffercount++;

		t->flags&=(~TR_HIGHLIGHT);
		RemoveTrace(t, 0);
		if(!current)
			{
			first=current=t;
			t->prev=NULL;
			t->next=NULL;
			}
			else
			{
			current->next=t;
			t->prev=current;
			current=t;
			t->next=NULL;
			}
		}
	t=tnext;
	}

return(traces.buffer=first);
}


/*
 * Paste the cut buffer into the main display one and
 * mark the cut buffer empty...
 */
Trptr PasteBuffer(void)
{
Trptr t, tinsert=NULL, tinsertnext;

if(!traces.buffer) return(NULL);

signalwindow_width_dirty=1;

if(!(t=traces.first))
	{
	t=traces.last=traces.first=traces.buffer;
	while(t)
		{
		traces.last=t;
		traces.total++;
		t=t->next;
		}	

	traces.buffer=traces.bufferlast=NULL;
	traces.buffercount=0;

	return(traces.first);
	}

while(t)
	{
	if(t->flags&TR_HIGHLIGHT) tinsert=t;
	t=t->next;
	}

if(!tinsert) tinsert=traces.last;
tinsertnext=tinsert->next;
tinsert->next=traces.buffer;
traces.buffer->prev=tinsert;
traces.bufferlast->next=tinsertnext;
traces.total+=traces.buffercount;

if(!tinsertnext)
	{
	traces.last=traces.bufferlast;
	}
	else
	{
	tinsertnext->prev=traces.bufferlast;
	}

traces.buffer=traces.bufferlast=NULL;
traces.buffercount=0;

return(traces.first);
}


/*
 * Prepend the cut buffer into the main display one and
 * mark the cut buffer empty...
 */
Trptr PrependBuffer(void)
{
Trptr t, prev;

if(!traces.buffer) return(NULL);

signalwindow_width_dirty=1;

t=traces.buffer;

while(t)
	{
	prev=t;
	t->flags&=(~TR_HIGHLIGHT);
	traces.total++;
	t=t->next;
	}

if((prev->next=traces.first))
	{
	/* traces.last current value is ok as it stays the same */
	traces.first->prev=prev; /* but we need the reverse link back up */
	}
	else
	{
	traces.last=prev;
	}

traces.first=traces.buffer;

traces.buffer=traces.bufferlast=NULL;
traces.buffercount=0;

return(traces.first);
}


/*
 * reversal of traces
 */
int TracesReverse(void)
{
Trptr t;
Trptr *tsort, *tsort_pnt;
int i;
   
if(!traces.total) return(0);

t=traces.first;
tsort=tsort_pnt=wave_alloca(sizeof(Trptr)*traces.total);   
for(i=0;i<traces.total;i++)
        {
        if(!t)
                {
                fprintf(stderr, "INTERNAL ERROR: traces.total vs traversal mismatch!  Exiting.\n");
                exit(255);
                }
        *(tsort_pnt++)=t;

        t=t->next;
        }

traces.first=*(--tsort_pnt);

for(i=traces.total-1;i>=0;i--)
        {
        t=*tsort_pnt;

	if(i==traces.total-1)
		{
		t->prev=NULL;
		}
		else
		{
		t->prev=*(tsort_pnt+1);
		}

	if(i)
		{
		t->next=*(--tsort_pnt);
		}
        }

traces.last=*tsort;
traces.last->next=NULL;

return(1);
}  


/*************************************************************/


/*
 * sort on tracename pointers (alpha/caseins alpha/sig sort)
 */
static int tracenamecompare(const void *s1, const void *s2)
{
char *str1, *str2;

str1=(*((Trptr *)s1))->name;
str2=(*((Trptr *)s2))->name;

if((!str1) || (!*str1))	/* force blank lines to go to bottom */
	{
	if((!str2) || (!*str2))
		{
		return(0);
		}
		else
		{
		return(1);
		}
	}
else
if((!str2) || (!*str2))
	{
	return(-1);		/* str1==str2==zero case is covered above */
	}
  
return(strcmp(str1, str2));
}


static int traceinamecompare(const void *s1, const void *s2)
{
char *str1, *str2;

str1=(*((Trptr *)s1))->name;
str2=(*((Trptr *)s2))->name;

if((!str1) || (!*str1))	/* force blank lines to go to bottom */
	{
	if((!str2) || (!*str2))
		{
		return(0);
		}
		else
		{
		return(1);
		}
	}
else
if((!str2) || (!*str2))
	{
	return(-1);		/* str1==str2==zero case is covered above */
	}
  
return(strcasecmp(str1, str2));
}

static int tracesignamecompare(const void *s1, const void *s2)
{
char *str1, *str2;

str1=(*((Trptr *)s1))->name;
str2=(*((Trptr *)s2))->name;

if((!str1) || (!*str1))	/* force blank lines to go to bottom */
	{
	if((!str2) || (!*str2))
		{
		return(0);
		}
		else
		{
		return(1);
		}
	}
else
if((!str2) || (!*str2))
	{
	return(-1);		/* str1==str2==zero case is covered above */
	}
  
return(sigcmp(str1, str2));
}


/*
 * alphabetization of traces
 */
int TracesAlphabetize(int mode)
{
Trptr t, prev;
Trptr *tsort, *tsort_pnt;
char *subst, ch;
int i;
int (*cptr)(const void*, const void*);
   
if(!traces.total) return(0);

t=traces.first;
tsort=tsort_pnt=wave_alloca(sizeof(Trptr)*traces.total);   
for(i=0;i<traces.total;i++)
        {
        if(!t)
                {
                fprintf(stderr, "INTERNAL ERROR: traces.total vs traversal mismatch!  Exiting.\n");
                exit(255);
                }
        *(tsort_pnt++)=t;

	if((subst=t->name))
	        while((ch=(*subst)))
        	        {
        	        if(ch==hier_delimeter) { *subst=0x01; } /* forces sort at hier boundaries */
        	        subst++;
        	        }

        t=t->next;
        }

switch(mode)
	{
	case 0:		cptr=traceinamecompare;   break;
	case 1:		cptr=tracenamecompare;	  break;
	default:	cptr=tracesignamecompare; break;
	}

qsort(tsort, traces.total, sizeof(Trptr), cptr);

tsort_pnt=tsort;
for(i=0;i<traces.total;i++)
        {
        t=*(tsort_pnt++);

	if(!i)
		{
		traces.first=t;
		t->prev=NULL;
		}
		else
		{
		prev->next=t;
		t->prev=prev;
		}

	prev=t;

	if((subst=t->name))
	        while((ch=(*subst)))
        	        {
        	        if(ch==0x01) { *subst=hier_delimeter; } /* restore hier */
        	        subst++;
        	        }
        }

traces.last=prev;
prev->next=NULL;

return(1);
}  

