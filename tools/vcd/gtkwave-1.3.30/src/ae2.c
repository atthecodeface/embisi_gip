#include <stdio.h>
#include "ae2.h"

#include <unistd.h>

#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include "aet.h"
#include "vcd.h"
#include "lxt.h"
#include "debug.h"

/****************************************************************************/

static AE2_HANDLE ae2=NULL;
static FILE *ae2_handle=NULL;

static void ae2_msg_fn(int sev, const char *format, ...)
{
va_list ap;
va_start(ap, format);
fprintf(stderr, "AE2 %02d | ", sev);
vfprintf(stderr, format, ap);
fprintf(stderr, "\n");
}

static void ae2_severror_fn(const char *format, ...)
{
va_list ap;
va_start(ap, format);
fprintf(stderr, "AE2 ** | ");
vfprintf(stderr, format, ap);
fprintf(stderr, "\n");
exit(255);
}

static void* ae2_alloc_fn(unsigned long size)
{
void *pnt = malloc_2(size);
return(pnt);
}

static void ae2_free_fn(void* ptr, unsigned long size)
{
free_2(ptr);
}


/****************************************************************************/

/*
 * globals
 */
char is_ae2 = 0;

static struct fac *mvlfacs=NULL;
static TimeType first_cycle, last_cycle, total_cycles;

static unsigned char initial_value = 0;

static char *lt_buf=NULL;	/* max memory ever needed for a fac's binary value */
static int lt_len = 0;


/*
 * mainline
 */
TimeType ae2_main(char *fname)
{
int i;
struct Node *n;
struct symbol *s, *prevsymroot=NULL, *prevsym=NULL;

ae2_handle = fopen(fname, "rb");
if(!ae2_handle)
        {
        fprintf(stderr, "Could not open '%s', exiting.\n", fname);
        exit(0);
        }

ae2 = ae2_read_initialize(ae2_severror_fn, ae2_msg_fn, ae2_alloc_fn, ae2_free_fn, ae2_handle);
if(!ae2)
        {
        fprintf(stderr, "Could not initialize '%s', exiting.\n", fname);
        exit(0);
        }

time_dimension = 'n';

numfacs=ae2_read_num_symbols(ae2);

ae2_msg_fn(00, "Number of facs: %d.", numfacs);
mvlfacs=(struct fac *)calloc(numfacs,sizeof(struct fac));

for(i=0;i<numfacs;i++)
	{
	char nam[65537];
	int idx = i+1;
	int nlen;
	
	nlen=ae2_read_symbol_name(ae2, idx, nam);
	mvlfacs[i].name=strcpy(malloc_2(nlen+1), nam);	
	mvlfacs[i].array_height=ae2_read_symbol_rows(ae2, idx);
	mvlfacs[i].msb=0;
	mvlfacs[i].len=ae2_read_symbol_length(ae2, idx);
	if(mvlfacs[i].len>lt_len) lt_len = mvlfacs[i].len;
	mvlfacs[i].lsb=mvlfacs[i].len-1;
	mvlfacs[i].flags=LT_SYM_F_BITS;
	}

lt_buf = malloc_2(lt_len ? lt_len+1 : 2);

ae2_msg_fn(00, "Finished building %d facs.", numfacs);

first_cycle = (TimeType) ae2_read_start_cycle(ae2);
last_cycle = (TimeType) ae2_read_end_cycle(ae2);
/* last_cycle = 2000+34*256; <-- max cacheable size for now is 42 ae2 blocks */
total_cycles = last_cycle - first_cycle + 1;

/* do your stuff here..all useful info has been initialized by now */

hier_delimeter='.';

for(i=0;i<numfacs;i++)
        {
	char buf[4096];
	char *str;	
	struct fac *f;

	f=mvlfacs+i;

	if(f->len>1)
		{
		sprintf(buf, "%s[%d:%d]", mvlfacs[i].name,mvlfacs[i].msb, mvlfacs[i].lsb);
		str=malloc_2(strlen(buf)+1);
		if(!alt_hier_delimeter)
			{
			strcpy(str, buf);
			}
			else
			{
			strcpy_vcdalt(str, buf, alt_hier_delimeter);
			}
	        s=symadd(str,hash(str));
		prevsymroot = prevsym = NULL;
		}
		else
		{
		str=malloc_2(strlen(mvlfacs[i].name)+1);
		if(!alt_hier_delimeter)
			{
			strcpy(str, mvlfacs[i].name);
			}
			else
			{
			strcpy_vcdalt(str, mvlfacs[i].name, alt_hier_delimeter);
			}
	        s=symadd(str,hash(str));
		prevsymroot = prevsym = NULL;
		}
		
        if(!firstnode)
                {
                firstnode=curnode=s;   
                }
                else
                {
                curnode->nextinaet=s;
                curnode=s;   
                }

        n=(struct Node *)calloc_2(1,sizeof(struct Node));
        n->nname=s->name;
        n->mvlfac = mvlfacs+i;

	if(f->len>1)
		{
		ExtNode *ext = (ExtNode *)calloc_2(1,sizeof(struct ExtNode));
		ext->msi = mvlfacs[i].msb;
		ext->lsi = mvlfacs[i].lsb;
		n->ext = ext;
		}
                 
        n->head.time=-1;        /* mark 1st node as negative time */
        n->head.v.val=1;
        s->n=n;
        }

facs=(struct symbol **)malloc_2(numfacs*sizeof(struct symbol *));
curnode=firstnode;
for(i=0;i<numfacs;i++)
	{
	char *subst, ch;
	int len;

	facs[i]=curnode;
        if((len=strlen(subst=curnode->name))>longestname) longestname=len;
	curnode=curnode->nextinaet;
	while((ch=(*subst)))
		{	
		if(ch==hier_delimeter) { *subst=0x01; }	/* forces sort at hier boundaries */
		subst++;
		}
	}

ae2_msg_fn(00,"Sorting facilities at hierarchy boundaries.");
heapsort(facs,numfacs);
	
for(i=0;i<numfacs;i++)
	{
	char *subst, ch;

	subst=facs[i]->name;
	while((ch=(*subst)))
		{	
		if(ch==0x01) { *subst=hier_delimeter; }	/* restore back to normal */
		subst++;
		}
	}

facs_are_sorted=1;

ae2_msg_fn(00,"Building facility hierarchy tree.");

init_tree();		
for(i=0;i<numfacs;i++)	
{
build_tree_from_name(facs[i]->name, i);
}
treeprune(treeroot);

min_time = first_cycle; max_time=last_cycle;
ae2_msg_fn(00, "["TTFormat"] start time.", min_time);
ae2_msg_fn(00,"["TTFormat"] end time.", max_time);
is_ae2 = ~0;

return(max_time);
}


/* 
 * actually import an ae2 trace but don't do it if it's already been imported 
 */
void import_ae2_trace(nptr np)
{
TimeType tmval;
struct HistEnt *htemp;
struct HistEnt *histent_head, *histent_tail;
int len, i, j;
struct fac *f;

if(!(f=np->mvlfac)) return;	/* already imported */

tmval=LLDescriptor(-1);
len = np->mvlfac->len;
         
histent_tail = htemp = histent_calloc();
if(len>1)
	{
	htemp->v.vector = (char *)malloc_2(len);
	for(i=0;i<len;i++) htemp->v.vector[i] = 2;
	}
	else
	{
	htemp->v.val = 2;		/* z */
	}
htemp->time = MAX_HISTENT_TIME;

histent_head = histent_calloc();
if(len>1)
	{
	histent_head->v.vector = (char *)malloc_2(len);
	for(i=0;i<len;i++) histent_head->v.vector[i] = 1;
	}
	else
	{
	histent_head->v.val = 1;	/* x */
	}
histent_head->time = MAX_HISTENT_TIME-1;
histent_head->next = htemp;	/* x */

np->numhist=2;

if(f->array_height <= 1) /* sorry, arrays not supported */
	{
	int txidx = f - mvlfacs + 1;
	long len = f->len;
	uint64_t ncyc=last_cycle+1;
	unsigned char val=0;

	int lp=0;
	
	do
		{
		ncyc--;
		ae2_read_value(ae2, txidx, 0 /* rows */, 0, len, ncyc, lt_buf);
		/* fprintf(stderr, "%d %s\n", (int)ncyc, lt_buf); */

		for(j=0;j<len;j++)
			{
			switch(lt_buf[j])
				{
				case '0': case '1':	
				case 'Z': case 'z':	break;
				case 'H': case 'h':	lt_buf[j]='z'; break;
				default:		lt_buf[j]='x'; break;
				}
			}

		if(len==1)
			{
			switch(lt_buf[0])
				{
			        case '0':       val = 0; break;
			        case 'x':	val = 1; break;
			        case 'z':	val = 2; break;
			        case '1':       val = 3; break;
			        }

			if(val!=histent_head->v.val)
				{
				htemp = histent_calloc();
				htemp->v.val = val;
				htemp->time = ncyc;
				htemp->next = histent_head;
				histent_head = htemp;
				np->numhist++;
				}
				else
				{
				histent_head->time = ncyc;
				}
			}
			else
			{
			if(memcmp(lt_buf, histent_head->v.vector, len))
				{
				htemp = histent_calloc();
				htemp->v.vector = (char *)malloc_2(len);
				memcpy(htemp->v.vector, lt_buf, len);
				htemp->time = ncyc;
				htemp->next = histent_head;
				histent_head = htemp;
				np->numhist++;
				}
				else
				{
				histent_head->time = ncyc;
				}
			}
		} while((ncyc) && (ncyc != first_cycle));
	tmval = 0;
	}

np->mvlfac = NULL;	/* it's imported and cached so we can forget it's an mvlfac now */

if(tmval>first_cycle)
	{
	char init;

	htemp = histent_calloc();

	init = initial_value;
		
	if(len>1)
		{
		char *pnt = htemp->v.vector = (char *)malloc_2(len);	/* zeros */
		int i;
		for(i=0;i<len;i++)
			{
			*(pnt++) = init;
			}
		}
		else
		{
		htemp->v.val = init;
		}
	
	htemp->time = first_cycle;
	htemp->next = histent_head;
	histent_head = htemp;
	np->numhist++;
	}

if(len>1)
	{
	np->head.v.vector = (char *)malloc_2(len);
	for(i=0;i<len;i++) np->head.v.vector[i] = 1;
	}
	else
	{
	np->head.v.val = 1;                     /* 'x' */
	}

np->head.time  = -2;
np->head.next = histent_head;
np->curr = histent_tail;
np->numhist++;
}
