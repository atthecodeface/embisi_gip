/* 
 * Copyright (c) Tony Bybell 1999-2003.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include "analyzer.h"
#include "aet.h"
#include "ae2.h"
#include "lxt.h"
#include "vcd.h"
#include "debug.h"
#include "bsearch.h"
#include "strace.h"
#include <stdlib.h>

/*
 * mvlfac resolution
 */
void import_trace(nptr np)
{
if(is_lxt)
	{
	import_lxt_trace(np);
	}
else
if(is_ae2)
	{
	import_ae2_trace(np);
	}
else
	{
	fprintf(stderr, "Internal error with mvlfac trace handling, exiting.\n");
	exit(255);
	}
}


/*
 * turn a Bits structure into a vector with deltas for faster displaying
 */
bvptr bits2vector(struct Bits *b)
{
int i;
int regions=0;	
struct Node *n;
hptr *h;
vptr vhead=NULL, vcurr=NULL, vadd;
int numextrabytes;
TimeType mintime, lasttime=-1;
bvptr bitvec=NULL;

if(!b) return(NULL);

h=(hptr *)calloc_2(b->nbits, sizeof(hptr));

numextrabytes=(b->nbits)-1;

for(i=0;i<b->nbits;i++)
	{
	n=b->nodes[i];
	h[i]=&(n->head);
	}

while(h[0])	/* should never exit through this point the way we set up histents with trailers now */
	{
	mintime=MAX_HISTENT_TIME;

	vadd=(vptr)calloc_2(1,sizeof(struct VectorEnt)+numextrabytes);
	
	for(i=0;i<b->nbits;i++)	/* was 1...big mistake */
		{
		if(h[i]->next)
		if(h[i]->next->time < mintime)
			{	
			mintime = h[i]->next->time;
			}
		}

	vadd->time=lasttime;
	lasttime=mintime;	

	regions++;

	for(i=0;i<b->nbits;i++)
		{
		unsigned char enc;

		enc  = ((unsigned char)(h[i]->v.val))&3;
		vadd->v[i] = enc;

		if(h[i]->next)
		if(h[i]->next->time == mintime)
			{
			h[i]=h[i]->next;
			}
		}

	if(vhead)
		{
		vcurr->next=vadd;
		vcurr=vadd;
		}
		else
		{
		vhead=vcurr=vadd;
		}

	if((mintime==MAX_HISTENT_TIME)) break;	/* normal bail part */
	}

vadd=(vptr)calloc_2(1,sizeof(struct VectorEnt)+numextrabytes);
vadd->time=MAX_HISTENT_TIME;
for(i=0;i<=numextrabytes;i++) vadd->v[i]=0x55;
vcurr->next=vadd;
regions++;

bitvec=(bvptr)calloc_2(1,sizeof(struct BitVector)+
		(regions*sizeof(vptr)));

strcpy(bitvec->name=(char *)malloc_2(strlen(b->name)+1),b->name);
bitvec->nbits=b->nbits;
bitvec->numregions=regions;

vcurr=vhead;
for(i=0;i<regions;i++)
	{
	bitvec->vectors[i]=vcurr;
	vcurr=vcurr->next;
	}

return(bitvec);
}


/*
 * Parse a line of the wave file and act accordingly.. 
 * Returns nonzero if trace(s) added.
 */
int parsewavline(char *w)
{
int i, j;
int len;
char *prefix, *suffix;
char *w2;
nptr nexp;

if(!(len=strlen(w))) return(0);
if(*(w+len-1)=='\n')
	{
	*(w+len-1)=0x00; /* strip newline if present */
	len--;
	if(!len) return(0);
	}

prefix=(char *)wave_alloca(len+1);
suffix=(char *)wave_alloca(len+1);

w2=w;
while(1)
{
if(isspace(*w2)) { w2++; continue; }
if(!(*w2)) return(0);	/* no args */
break;			/* start grabbing chars from here */
}

sscanf(w2,"%s",prefix);
if(*w2=='*')
	{
	float f;
	TimeType ttlocal;
	int which=0;

	zoom_was_explicitly_set=~0;
	w2++;

	for(;;)
		{
		while(*w2==' ') w2++;
		if(*w2==0) return(~0);

		if(!which) { sscanf(w2,"%f",&f); tims.zoom=(gdouble)f; }
		else
		{
		sscanf(w2,TTFormat,&ttlocal);
		switch(which)
			{
			case 1:  tims.marker=ttlocal; break;
			default: 
				if((which-2)<26) named_markers[which-2]=ttlocal; 
				break;
			}
		}
		which++;
		w2++;
		for(;;)
			{
			if(*w2==0) return(~0);
			if(*w2=='\n') return(~0);
			if(*w2!=' ') w2++; else break;
			}
		}
	}
else
if(*w2=='-')
	{
	AddBlankTrace((*(w2+1)!=0)?(w2+1):NULL);
	}
else
if(*w2=='>')
	{
	char *nptr=(*(w2+1)!=0)?(w2+1):NULL;
	shift_timebase_default_for_add=nptr?atoi_64(nptr):LLDescriptor(0);
	}
else
if(*w2=='@')
	{
	/* handle trace flags */
	sscanf(w2+1, "%x", &default_flags);
	return(~0);
	}
else
if(*w2=='+')
	{
	/* handle aliasing */
	struct symbol *s;
	sscanf(w2+strlen(prefix),"%s",suffix);

	if(suffix[0]=='(')
		{
		for(i=1;;i++)
			{
			if(suffix[i]==0) return(0);
			if((suffix[i]==')')&&(suffix[i+1])) {i++; goto aliaspt2; }
			}
		return(0);

aliaspt2:
		s=symfind(suffix+i);
		nexp = ExtractNodeSingleBit(s->n, atoi(suffix+1));
		if(nexp)
			{
			AddNode(nexp, prefix+1);
			return(~0);
			}
			else
			{
			return(0);
			}		
		}
		else
		{
		if((s=symfind(suffix)))
			{
			AddNode(s->n,prefix+1);
			return(~0);
			}
			else
			{
			return(0);
			}
		}
	}
else
if(*w2=='#')
	{
	/* handle bitvec */
	bvptr v=NULL;
	bptr b=NULL;

	w2=w2+strlen(prefix);
	while(1)
		{
		if(isspace(*w2)) { w2++; continue; }
		if(!(*w2)) return(0);	/* no more args */	
		break;			/* start grabbing chars from here */
		}


	if((b=makevec(prefix+1,w2)))
		{
		if((v=bits2vector(b)))
			{
			v->bits=b;	/* only needed for savefile function */
			AddVector(v);
			free_2(b->name);
			b->name=NULL;
			return(v!=NULL);
			}
			else
			{
			free_2(b->name);
			free_2(b);
			}
		}
	
	return(v!=NULL);
	}
else
if(*w2=='!')
	{
	/* fill logical_mutex */
	int i;
	char ch;

	for(i=0;i<6;i++)
		{
		ch = *(w2+i+1);
		if(ch != 0)
			{
			if(ch=='!')
				{
				shadow_active = 0;
				return(~0);
				}

			if((!i)&&(shadow_straces))
				{
				delete_strace_context();
				}

			shadow_logical_mutex[i] = (ch & 1);
			}
			else	/* in case of short read */
			{
			shadow_logical_mutex[i] = 0;
			}
		}

	shadow_active = 1;
	return(~0);
	}
	else
if(*w2=='?')
	{
	/* fill st->type */
	if(*(w2+1)=='\"')
		{
		int len = strlen(w2+2);
		if(shadow_string) free_2(shadow_string);
		shadow_string=NULL;

		if(len)
			{
			shadow_string = malloc_2(len+1);		
			strcpy(shadow_string, w2+2);
			}

		shadow_type = ST_STRING;
		}
		else
		{
		int hex;
		sscanf(w2+1, "%x", &hex);	
		shadow_type = hex;
		}

	return(~0);
	}
	else
	{
	return(maketraces(w));
	}

return(0);
}


/*
 * Make solitary traces from wildcarded signals...
 */
int maketraces(char *str)
{
char *pnt, *wild;
char ch, wild_active=0;
int len;
int i;
int made=0;

pnt=str;
while((ch=*pnt))
	{
	if(ch=='*') 
		{
		wild_active=1;
		break;
		}
	pnt++;
	}

if(!wild_active)	/* short circuit wildcard evaluation with bsearch */
	{
	struct symbol *s;
	nptr nexp;

	if(str[0]=='(')
		{
		for(i=1;;i++)
			{
			if(str[i]==0) return(0);
			if((str[i]==')')&&(str[i+1])) {i++; goto maketracespt2; }
			}
		return(0);

maketracespt2:
		s=symfind(str+i);
		nexp = ExtractNodeSingleBit(s->n, atoi(str+1));
		if(nexp)
			{
			AddNode(nexp, NULL);
			return(~0);
			}
			else
			{
			return(0);
			}		
		}
		else
		{
		if((s=symfind(str)))
			{
			AddNode(s->n,NULL);
			return(~0);
			}
			else
			{
			return(0);
			}
		}
	}

while(1)
{
pnt=str;
len=0;

while(1)
	{
	ch=*pnt++;
	if(isspace(ch)||(!ch)) break;
	len++;
	}

if(len)
	{
	wild=(char *)calloc_2(1,len+1);
	memcpy(wild,str,len);
	wave_regex_compile(wild);

	for(i=0;i<numfacs;i++)
		{
		if(wave_regex_match(facs[i]->name))
			{
			AddNode(facs[i]->n,NULL);
			made=~0;
			}
		}
	}

if(!ch) break;
str=pnt;
}
return(made);
}


/*
 * Create a vector from wildcarded signals...
 */
struct Bits *makevec(char *vec, char *str)
{
char *pnt, *pnt2, *wild=NULL;
char ch, ch2, wild_active;
int len, nodepnt=0;
int i;
struct Node *n[512];
struct Bits *b=NULL;

while(1)
{
pnt=str;
len=0;

while(1)
	{
	ch=*pnt++;
	if(isspace(ch)||(!ch)) break;
	len++;
	}

if(len)
	{
	wild=(char *)calloc_2(1,len+1);
	memcpy(wild,str,len);

	DEBUG(printf("WILD: %s\n",wild));

	wild_active=0;
	pnt2=wild;
	while((ch2=*pnt2))
		{
		if(ch2=='*') 
			{
			wild_active=1;
			break;
			}
		pnt2++;
		}

	if(!wild_active)	/* short circuit wildcard evaluation with bsearch */
		{
		struct symbol *s;
		if(wild[0]=='(')
			{
			nptr nexp;
			
			for(i=1;;i++)
				{
				if(wild[i]==0) break;
				if((wild[i]==')')&&(wild[i+1])) 
					{
					i++; 
					s=symfind(wild+i);
					nexp = ExtractNodeSingleBit(s->n, atoi(wild+1));
					if(nexp)
						{
						n[nodepnt++]=nexp;
						if(nodepnt==512) { free_2(wild); goto ifnode; }
						}		
					break;
					}
				}
			}
			else
			{
			if((s=symfind(wild)))	
				{
				n[nodepnt++]=s->n;
				if(nodepnt==512) { free_2(wild); goto ifnode; }
				}
			}
		}
		else
		{
		wave_regex_compile(wild);
		for(i=numfacs-1;i>=0;i--)	/* to keep vectors in little endian hi..lo order */
			{
			if(wave_regex_match(facs[i]->name))
				{
				n[nodepnt++]=facs[i]->n;
				if(nodepnt==512) { free_2(wild); goto ifnode; }
				}
			}
		}
	free_2(wild);
	}

if(!ch) break;
str=pnt;
}

ifnode:
if(nodepnt)
	{
	b=(struct Bits *)calloc_2(1,sizeof(struct Bits)+(nodepnt-1)*
				  sizeof(struct Node *));

	for(i=0;i<nodepnt;i++)
		{
		b->nodes[i]=n[i];
		if(n[i]->mvlfac) import_trace(n[i]);
		}

	b->nbits=nodepnt;
	strcpy(b->name=(char *)malloc_2(strlen(vec)+1),vec);
	}

return(b);
}


/*
 * Create a vector from selected_status signals...
 */
struct Bits *makevec_selected(char *vec, int numrows, char direction)
{
int nodepnt=0;
int i;
struct Node *n[512];
struct Bits *b=NULL;

if(!direction)
for(i=numfacs-1;i>=0;i--)	/* to keep vectors in hi..lo order */
	{
	if(facs[i]->selected)
		{
		n[nodepnt++]=facs[i]->n;
		if((nodepnt==512)||(numrows==nodepnt)) break;
		}
	}
else
for(i=0;i<numfacs;i++)		/* to keep vectors in lo..hi order */
	{
	if(facs[i]->selected)
		{
		n[nodepnt++]=facs[i]->n;
		if((nodepnt==512)||(numrows==nodepnt)) break;
		}
	}

if(nodepnt)
	{
	b=(struct Bits *)calloc_2(1,sizeof(struct Bits)+(nodepnt-1)*
				  sizeof(struct Node *));

	for(i=0;i<nodepnt;i++)
		{
		b->nodes[i]=n[i];
		if(n[i]->mvlfac) import_trace(n[i]);
		}

	b->nbits=nodepnt;
	strcpy(b->name=(char *)malloc_2(strlen(vec)+1),vec);
	}

return(b);
}

/*
 * add vector made in previous function
 */
int add_vector_selected(char *alias, int numrows, char direction)
{
bvptr v=NULL;
bptr b=NULL;

if((b=makevec_selected(alias, numrows, direction)))
	{
        if((v=bits2vector(b)))
                {
                v->bits=b;      /* only needed for savefile function */
                AddVector(v);
                free_2(b->name);
                b->name=NULL;
                return(v!=NULL);
                }
                else
                {
                free_2(b->name);
                free_2(b);
                }
        }
return(v!=NULL);
}

/***********************************************************************************/

/*
 * Create a vector from a range of signals...currently the single
 * bit facility_name[x] case never gets hit, but may be used in the
 * future...
 */
struct Bits *makevec_chain(char *vec, struct symbol *sym, int len)
{
int nodepnt=0, nodepnt_rev;
int i;
struct Node **n;
struct Bits *b=NULL;
struct symbol *symhi, *symlo;
char hier_delimeter2;

if(!vcd_explicit_zero_subscripts)	/* 0==yes, -1==no */
	{
	hier_delimeter2=hier_delimeter;
	}
	else
	{
	hier_delimeter2='[';
	}

n=(struct Node **)wave_alloca(len*sizeof(struct Node *));

if(!autocoalesce_reversal)		/* normal case for MTI */
	{
	symhi=sym;
	while(sym)
		{
		symlo=sym;
		n[nodepnt++]=sym->n;
		sym=sym->vec_chain;
		}
	}
	else				/* for verilog XL */
	{
	nodepnt_rev=len;
	symlo=sym;
	while(sym)
		{
		nodepnt++;
		symhi=sym;
		n[--nodepnt_rev]=sym->n;
		sym=sym->vec_chain;
		}
	}

if(nodepnt)
	{
	b=(struct Bits *)calloc_2(1,sizeof(struct Bits)+(nodepnt-1)*
				  sizeof(struct Node *));

	for(i=0;i<nodepnt;i++)
		{
		b->nodes[i]=n[i];
		if(n[i]->mvlfac) import_trace(n[i]);
		}

	b->nbits=nodepnt;

	if(vec)
		{
		strcpy(b->name=(char *)malloc_2(strlen(vec)+1),vec);
		}
		else
		{
		char *s1, *s2;
		int i, root1len=0, root2len=0;
		int l1, l2;

		s1=symhi->n->nname;
		s2=symlo->n->nname;
		
		l1=strlen(s1); 

		for(i=l1-1;i>=0;i--)
			{
			if(s1[i]==hier_delimeter2) { root1len=i+1; break; }
			}

		l2=strlen(s2);	
		for(i=l2-1;i>=0;i--)
			{
			if(s2[i]==hier_delimeter2) { root2len=i+1; break; }
			}

		if((root1len!=root2len)||(!root1len)||(!root2len)||
			(strncasecmp(s1,s2,root1len)))
			{
			if(symlo!=symhi)
				{
				strcpy(b->name=(char *)malloc_2(8+1),"<Vector>");
				}
				else
				{
				strcpy(b->name=(char *)malloc_2(l1+1),s1);
				}
			}
			else
			{
			int add1, add2, totallen;

			add1=l1-root1len; add2=l2-root2len;
			if(vcd_explicit_zero_subscripts==-1)
				{
				add1--;
				add2--;
				}
			
			if(symlo!=symhi)
				{
				unsigned char fixup1, fixup2;

				totallen=
					root1len
					-1		/* zap HIER_DELIMETER */
					+1		/* add [              */
					+add1		/* left value	      */
					+1		/* add :	      */
					+add2		/* right value	      */
					+1		/* add ]	      */
					+1		/* add 0x00	      */
					;

				if(vcd_explicit_zero_subscripts==-1)
					{
					fixup1=*(s1+l1-1); *(s1+l1-1)=0;
					fixup2=*(s2+l2-1); *(s2+l2-1)=0;
					}

				b->name=(char *)malloc_2(totallen);
				strncpy(b->name,s1,root1len-1);
				sprintf(b->name+root1len-1,"[%s:%s]",s1+root1len, s2+root2len);

				if(vcd_explicit_zero_subscripts==-1)
					{
					*(s1+l1-1)=fixup1;
					*(s2+l2-1)=fixup2;
					}
				}
				else
				{
				unsigned char fixup1;

				totallen=
					root1len
					-1		/* zap HIER_DELIMETER */
					+1		/* add [              */
					+add1		/* left value	      */
					+1		/* add ]	      */
					+1		/* add 0x00	      */
					;

				if(vcd_explicit_zero_subscripts==-1)
					{
					fixup1=*(s1+l1-1); *(s1+l1-1)=0;
					}

				b->name=(char *)malloc_2(totallen);
				strncpy(b->name,s1,root1len-1);
				sprintf(b->name+root1len-1,"[%s]",s1+root1len);

				if(vcd_explicit_zero_subscripts==-1)
					{
					*(s1+l1-1)=fixup1;
					}
				}
			}
		}
	}

return(b);
}

/*
 * add vector made in previous function
 */
int add_vector_chain(struct symbol *s, int len)
{
bvptr v=NULL;
bptr b=NULL;

if(len>1)
	{
	if((b=makevec_chain(NULL, s, len)))
		{
	        if((v=bits2vector(b)))
	                {
	                v->bits=b;      /* only needed for savefile function */
	                AddVector(v);
	                free_2(b->name);
	                b->name=NULL;
	                return(v!=NULL);
	                }
	                else
	                {
	                free_2(b->name);
	                free_2(b);
	                }
	        }
	return(v!=NULL);
	}
	else
	{
	return(AddNode(s->n,NULL));
	}
}

/***********************************************************************************/

/*
 * Create a vector from a range of signals...currently the single
 * bit facility_name[x] case never gets hit, but may be used in the
 * future...
 */
struct Bits *makevec_range(char *vec, int lo, int hi, char direction)
{
int nodepnt=0;
int i;
struct Node *n[512];
struct Bits *b=NULL;

if(!direction)
for(i=hi;i>=lo;i--)	/* to keep vectors in hi..lo order */
	{
	n[nodepnt++]=facs[i]->n;
	if(nodepnt==512) break;
	}
else
for(i=lo;i<=hi;i++)	/* to keep vectors in lo..hi order */
	{
	n[nodepnt++]=facs[i]->n;
	if(nodepnt==512) break;
	}

if(nodepnt)
	{
	b=(struct Bits *)calloc_2(1,sizeof(struct Bits)+(nodepnt-1)*
				  sizeof(struct Node *));

	for(i=0;i<nodepnt;i++)
		{
		b->nodes[i]=n[i];
		if(n[i]->mvlfac) import_trace(n[i]);
		}

	b->nbits=nodepnt;

	if(vec)
		{
		strcpy(b->name=(char *)malloc_2(strlen(vec)+1),vec);
		}
		else
		{
		char *s1, *s2;
		int i, root1len=0, root2len=0;
		int l1, l2;

		if(!direction)
			{
			s1=facs[hi]->n->nname;
			s2=facs[lo]->n->nname;
			}
			else
			{
			s1=facs[lo]->n->nname;
			s2=facs[hi]->n->nname;
			}
		
		l1=strlen(s1); 

		for(i=l1-1;i>=0;i--)
			{
			if(s1[i]==hier_delimeter) { root1len=i+1; break; }
			}

		l2=strlen(s2);	
		for(i=l2-1;i>=0;i--)
			{
			if(s2[i]==hier_delimeter) { root2len=i+1; break; }
			}

		if((root1len!=root2len)||(!root1len)||(!root2len)||
			(strncasecmp(s1,s2,root1len)))
			{
			if(lo!=hi)
				{
				strcpy(b->name=(char *)malloc_2(8+1),"<Vector>");
				}
				else
				{
				strcpy(b->name=(char *)malloc_2(l1+1),s1);
				}
			}
			else
			{
			int add1, add2, totallen;

			add1=l1-root1len; add2=l2-root2len;
			
			if(lo!=hi)
				{
				totallen=
					root1len
					-1		/* zap HIER_DELIMETER */
					+1		/* add [              */
					+add1		/* left value	      */
					+1		/* add :	      */
					+add2		/* right value	      */
					+1		/* add ]	      */
					+1		/* add 0x00	      */
					;

				b->name=(char *)malloc_2(totallen);
				strncpy(b->name,s1,root1len-1);
				sprintf(b->name+root1len-1,"[%s:%s]",s1+root1len, s2+root2len);
				}
				else
				{
				totallen=
					root1len
					-1		/* zap HIER_DELIMETER */
					+1		/* add [              */
					+add1		/* left value	      */
					+1		/* add ]	      */
					+1		/* add 0x00	      */
					;

				b->name=(char *)malloc_2(totallen);
				strncpy(b->name,s1,root1len-1);
				sprintf(b->name+root1len-1,"[%s]",s1+root1len);
				}
			}
		}
	}

return(b);
}

/*
 * add vector made in previous function
 */
int add_vector_range(char *alias, int lo, int hi, char direction)
{
bvptr v=NULL;
bptr b=NULL;

if(lo!=hi)
	{
	if((b=makevec_range(alias, lo, hi, direction)))
		{
	        if((v=bits2vector(b)))
	                {
	                v->bits=b;      /* only needed for savefile function */
	                AddVector(v);
	                free_2(b->name);
	                b->name=NULL;
	                return(v!=NULL);
	                }
	                else
	                {
	                free_2(b->name);
	                free_2(b);
	                }
	        }
	return(v!=NULL);
	}
	else
	{
	return(AddNode(facs[lo]->n,NULL));
	}
}


/*
 * splits facility name into signal and bitnumber
 */
void facsplit(char *str, int *len, int *number)
{
char *ptr;
char *numptr=NULL;
char ch;

ptr=str;

while((ch=*ptr))
        {
        if((ch>='0')&&(ch<='9')) 
                {
                if(!numptr) numptr=ptr;
                }
                else numptr=NULL;
        ptr++;
        }

if(numptr)
        {
        *number=atoi(numptr);
        *len=numptr-str;
        }
        else
        {
        *number=0;
        *len=ptr-str;
        }
}


/*
 * compares two facilities a la strcmp but preserves
 * numbers for comparisons
 *
 * there are two flavors..the slow and accurate to any 
 * arbitrary number of digits version (first) and the
 * fast one good to 2**31-1.  we default to the faster
 * version since there's probably no real need to
 * process ints larger than two billion anyway...
 */

#ifdef WAVE_USE_SIGCMP_INFINITE_PRECISION
int sigcmp(char *s1, char *s2)
{
char *n1, *n2;
unsigned char c1, c2;
int len1, len2;

for(;;)
	{
	c1=(unsigned char)*s1;
	c2=(unsigned char)*s2;

	if((c1==0)&&(c2==0)) return(0);
	if((c1>='0')&&(c1<='9')&&(c2>='0')&&(c2<='9'))
		{
		n1=s1; n2=s2;
		len1=len2=0;

		do	{
			len1++;
			c1=(unsigned char)*(n1++);
			} while((c1>='0')&&(c1<='9'));
		if(!c1) n1--;

		do	{
			len2++;
			c2=(unsigned char)*(n2++);
			} while((c2>='0')&&(c2<='9'));
		if(!c2) n2--;

		do	{
			if(len1==len2)
				{
				c1=(unsigned char)*(s1++);
				len1--;
				c2=(unsigned char)*(s2++);
				len2--;
				}
			else
			if(len1<len2)
				{
				c1='0';
				c2=(unsigned char)*(s2++);
				len2--;
				}
			else
				{
				c1=(unsigned char)*(s1++);
				len1--;
				c2='0';
				}

			if(c1!=c2) return((int)c1-(int)c2);
			} while(len1);

		s1=n1; s2=n2;
		continue;
		}
		else
		{
		if(c1!=c2) return((int)c1-(int)c2);
		}

	s1++; s2++;
	}
}
#else
int sigcmp(char *s1, char *s2)
{
unsigned char c1, c2;
int u1, u2;

for(;;)
	{
	c1=(unsigned char)*(s1++);
	c2=(unsigned char)*(s2++);

	if((!c1)&&(!c2)) return(0);
	if((c1<='9')&&(c2<='9')&&(c2>='0')&&(c1>='0'))
		{
		u1=(int)(c1&15);
		u2=(int)(c2&15);

		while(((c2=(unsigned char)*s2)>='0')&&(c2<='9'))
			{
			u2*=10;
			u2+=(unsigned int)(c2&15);
			s2++;
			}

		while(((c2=(unsigned char)*s1)>='0')&&(c2<='9'))
			{
			u1*=10;
			u1+=(unsigned int)(c2&15);
			s1++;
			}

		if(u1==u2) continue;
			else return((int)u1-(int)u2);
		}
		else
		{
		if(c1!=c2) return((int)c1-(int)c2);
		}
	}
}
#endif

#ifndef __linux__
/* 
 * heapsort algorithm.  this typically outperforms quicksort.  note
 * that glibc will use a modified mergesort if memory is available, so
 * under linux use the stock qsort instead.
 */
static struct symbol **hp;
static void heapify(int i, int heap_size)
{
int l, r;
unsigned int largest;
struct symbol *t;
int start=i;
int maxele=heap_size/2-1;	/* points to where heapswaps don't matter anymore */
                
for(;;)
        {
        l=2*i+1;
        r=l+1;
                         
        if((l<heap_size)&&(sigcmp(hp[l]->name,hp[i]->name)>0))
                {
                largest=l;
                }   
                else
                {
                largest=i;
                }
        if((r<heap_size)&&(sigcmp(hp[r]->name,hp[largest]->name)>0))
                {
                largest=r;
                }
        
        if(i!=largest)
                {
                t=hp[i];
                hp[i]=hp[largest];
                hp[largest]=t;
                
                if(largest<=maxele)
                        {
                        i=largest;
                        }
                        else
                        {
                        break;
                        } 
                }   
                else
                {
                break;
                }
        }
}

void heapsort(struct symbol **a, int num)
{
int i;
int indx=num-1;
struct symbol *t;

hp=a;

for(i=(num/2-1);i>0;i--)	/* build facs into having heap property */
        {
        heapify(i,num);
        }

for(;;)
	{
        if(indx) heapify(0,indx+1);
	DEBUG(printf("%s\n", a[0]->name));

	if(indx!=0)
		{
		t=a[0];			/* sort in place by doing a REVERSE sort and */
		a[0]=a[indx];		/* swapping the front and back of the tree.. */
		a[indx--]=t;
		}
		else
		{
		break;
		}
	}
}

#else

static int qssigcomp(const void *v1, const void *v2)
{
struct symbol *a1 = *((struct symbol **)v1);
struct symbol *a2 = *((struct symbol **)v2);
return(sigcmp(a1->name, a2->name));
}

void heapsort(struct symbol **a, int num)
{
qsort(a, num, sizeof(struct symbol *), qssigcomp);
}

#endif

/*
 * Malloc/Create a name from a range of signals starting from vec_root...currently the single
 * bit facility_name[x] case never gets hit, but may be used in the
 * future...
 */
char *makename_chain(struct symbol *sym)
{
int i;
struct symbol *symhi, *symlo;
char hier_delimeter2;
char *name=NULL;
char *s1, *s2;
int root1len=0, root2len=0;
int l1, l2;

if(!vcd_explicit_zero_subscripts)	/* 0==yes, -1==no */
	{
	hier_delimeter2=hier_delimeter;
	}
	else
	{
	hier_delimeter2='[';
	}

if(!autocoalesce_reversal)		/* normal case for MTI */
	{
	symhi=sym;
	while(sym)
		{
		symlo=sym;
		sym=sym->vec_chain;
		}
	}
	else				/* for verilog XL */
	{
	symlo=sym;
	while(sym)
		{
		symhi=sym;
		sym=sym->vec_chain;
		}
	}


s1=symhi->n->nname;
s2=symlo->n->nname;
		
l1=strlen(s1); 

for(i=l1-1;i>=0;i--)
	{
	if(s1[i]==hier_delimeter2) { root1len=i+1; break; }
	}

l2=strlen(s2);	
for(i=l2-1;i>=0;i--)
	{
	if(s2[i]==hier_delimeter2) { root2len=i+1; break; }
	}

if((root1len!=root2len)||(!root1len)||(!root2len)||
	(strncasecmp(s1,s2,root1len)))
	{
	if(symlo!=symhi)
		{
		strcpy(name=(char *)malloc_2(8+1),"<Vector>");
		}
		else
		{
		strcpy(name=(char *)malloc_2(l1+1),s1);
		}
	}
	else
	{
	int add1, add2, totallen;

	add1=l1-root1len; add2=l2-root2len;
	if(vcd_explicit_zero_subscripts==-1)
		{
		add1--;
		add2--;
		}
			
	if(symlo!=symhi)
		{
		unsigned char fixup1, fixup2;

		totallen=
			root1len
			-1		/* zap HIER_DELIMETER */
			+1		/* add [              */
			+add1		/* left value	      */
			+1		/* add :	      */
			+add2		/* right value	      */
			+1		/* add ]	      */
			+1		/* add 0x00	      */
			;

		if(vcd_explicit_zero_subscripts==-1)
			{
			fixup1=*(s1+l1-1); *(s1+l1-1)=0;
			fixup2=*(s2+l2-1); *(s2+l2-1)=0;
			}

		name=(char *)malloc_2(totallen);
		strncpy(name,s1,root1len-1);
		sprintf(name+root1len-1,"[%s:%s]",s1+root1len, s2+root2len);

		if(vcd_explicit_zero_subscripts==-1)
			{
			*(s1+l1-1)=fixup1;
			*(s2+l2-1)=fixup2;
			}
		}
		else
		{
		unsigned char fixup1;

		totallen=
			root1len
			-1		/* zap HIER_DELIMETER */
			+1		/* add [              */
			+add1		/* left value	      */
			+1		/* add ]	      */
			+1		/* add 0x00	      */
			;

		if(vcd_explicit_zero_subscripts==-1)
			{
			fixup1=*(s1+l1-1); *(s1+l1-1)=0;
			}

		name=(char *)malloc_2(totallen);
		strncpy(name,s1,root1len-1);
		sprintf(name+root1len-1,"[%s]",s1+root1len);

		if(vcd_explicit_zero_subscripts==-1)
			{
			*(s1+l1-1)=fixup1;
			}
		}
	}

return(name);
}

/******************************************************************/

eptr ExpandNode(nptr n)
{
int width;
int msb, lsb, delta;
int actual;
hptr h, htemp;
hptr *harray;
int i, j, k;
nptr *narray;
char *nam;
int offset, len;
eptr rc=NULL;
exptr exp;

if(n->mvlfac) import_trace(n);

DEBUG(fprintf(stderr, "expanding '%s'\n", n->nname));
if(!n->ext)
	{
	DEBUG(fprintf(stderr, "Nothing to expand\n"));
	}
	else
	{
	msb = n->ext->msi;
	lsb = n->ext->lsi;
	if(msb>lsb)
		{
		width = msb - lsb + 1;
		delta = -1;
		}
		else
		{
		width = lsb - msb + 1;
		delta = 1;
		}
	actual = msb;

	narray=(nptr *)malloc_2(width*sizeof(nptr));
	rc = malloc_2(sizeof(ExpandInfo));
	rc->narray = narray;
	rc->msb = msb;
	rc->lsb = lsb;
	rc->width = width;

	offset = strlen(n->nname);
	for(i=offset-1;i>=0;i--)
		{
		if(n->nname[i]=='[') break;
		}
	if(i>-1) offset=i;

	nam=(char *)wave_alloca(offset+20);
	memcpy(nam, n->nname, offset);

	if(!n->harray)         /* make quick array lookup for aet display--normally this is done in addnode */
	        {
		hptr histpnt;
		int histcount;
		hptr *harray;

	        histpnt=&(n->head);
	        histcount=0;
	
	        while(histpnt)
	                {
	                histcount++;
	                histpnt=histpnt->next;
	                }
	
	        n->numhist=histcount;
	 
	        if(!(n->harray=harray=(hptr *)malloc_2(histcount*sizeof(hptr))))
	                {
	                fprintf( stderr, "Out of memory, can't add %s to analyzer\n",
	                        n->nname );
	                return(NULL);
	                }
	
	        histpnt=&(n->head);
	        for(i=0;i<histcount;i++)
	                {
	                *harray=histpnt;
	                harray++;
	                histpnt=histpnt->next;
	                }
	        }

	h=&(n->head);
	while(h)
		{
		if(h->flags & (HIST_REAL|HIST_STRING)) return(NULL);
		h=h->next;
		}

	DEBUG(fprintf(stderr, "Expanding: (%d to %d) for %d bits over %d entries.\n", msb, lsb, width, n->numhist));

	for(i=0;i<width;i++)
		{
		narray[i] = (nptr)calloc_2(1, sizeof(struct Node));
		sprintf(nam+offset, "[%d]", actual);	
		len = offset + strlen(nam+offset);
		narray[i]->nname = (char *)malloc_2(len+1);
		strcpy(narray[i]->nname, nam);

		exp = (exptr) calloc_2(1, sizeof(struct ExpandReferences));
		exp->parent=n;							/* point to parent */
		exp->parentbit=i;
		exp->actual = actual;
		actual += delta;
		narray[i]->expansion = exp;					/* can be safely deleted if expansion set like here */
		}

	for(i=0;i<n->numhist;i++)
		{
		h=n->harray[i];
		if((h->time<min_time)||(h->time>max_time))
			{
			for(j=0;j<width;j++)
				{
				if(narray[j]->curr)
					{
					htemp = (hptr) calloc_2(1, sizeof(struct HistEnt));				
					htemp->v.val = 1;			/* 'x' */
					htemp->time = h->time;
					narray[j]->curr->next = htemp;
					narray[j]->curr = htemp;
					}
					else
					{
					narray[j]->head.v.val = 1; 		/* 'x' */
					narray[j]->head.time  = h->time;
					narray[j]->curr = &(narray[j]->head);
					}

				narray[j]->numhist++;
				}
			}
			else
			{
			for(j=0;j<width;j++)
				{
				unsigned char val = h->v.vector[j];
				switch(val)
					{
					case '0':	val = 0; break;
					case 'x':
					case 'X':	val = 1; break;
					case 'z':
					case 'Z':	val = 2; break;
					case '1':	val = 3; break;
					default:	break;			/* leave val alone as it's been converted already.. */
					}

				if(narray[j]->curr->v.val != val) 		/* curr will have been established already by 'x' at time: -1 */
					{
					htemp = (hptr) calloc_2(1, sizeof(struct HistEnt));				
					htemp->v.val = val;
					htemp->time = h->time;
					narray[j]->curr->next = htemp;
					narray[j]->curr = htemp;
					narray[j]->numhist++;
					}
				}
			}
		}

	for(i=0;i<width;i++)
		{
		narray[i]->harray = (hptr *)calloc_2(narray[i]->numhist, sizeof(hptr));
		htemp = &(narray[i]->head);
		for(j=0;j<narray[i]->numhist;j++)
			{
			narray[i]->harray[j] = htemp;
			htemp = htemp->next;
			}
		}

	}

return(rc);
}

/******************************************************************/

nptr ExtractNodeSingleBit(nptr n, int bit)
{
int lft, rgh;
hptr h, htemp;
hptr *harray;
int i, j, k;
int actual;
nptr np;
char *nam;
int offset, len;
exptr exp;

if(n->mvlfac) import_trace(n);

DEBUG(fprintf(stderr, "expanding '%s'\n", n->nname));
if(!n->ext)
	{
	DEBUG(fprintf(stderr, "Nothing to expand\n"));
	return(NULL);
	}
	else
	{
	if(n->ext->lsi > n->ext->msi)
		{
		rgh = n->ext->lsi; lft = n->ext->msi;
		actual = n->ext->msi + bit;
		}
		else
		{
		rgh = n->ext->msi; lft = n->ext->lsi;
		actual = n->ext->msi - bit;
		}

	if((bit>rgh)||(bit<lft))
		{
		DEBUG(fprintf(stderr, "Out of range\n"));
		return(NULL);
		}

	offset = strlen(n->nname);
	for(i=offset-1;i>=0;i--)
		{
		if(n->nname[i]=='[') break;
		}
	if(i>-1) offset=i;

	nam=(char *)wave_alloca(offset+20);
	memcpy(nam, n->nname, offset);

	if(!n->harray)         /* make quick array lookup for aet display--normally this is done in addnode */
	        {
		hptr histpnt;
		int histcount;
		hptr *harray;

	        histpnt=&(n->head);
	        histcount=0;
	
	        while(histpnt)
	                {
	                histcount++;
	                histpnt=histpnt->next;
	                }
	
	        n->numhist=histcount;
	 
	        if(!(n->harray=harray=(hptr *)malloc_2(histcount*sizeof(hptr))))
	                {
	                DEBUG(fprintf( stderr, "Out of memory, can't add %s to analyzer\n",
	                        n->nname ));
	                return(NULL);
	                }
	
	        histpnt=&(n->head);
	        for(i=0;i<histcount;i++)
	                {
	                *harray=histpnt;
	                harray++;
	                histpnt=histpnt->next;
	                }
	        }

	h=&(n->head);
	while(h)
		{
		if(h->flags & (HIST_REAL|HIST_STRING)) return(NULL);
		h=h->next;
		}

	DEBUG(fprintf(stderr, "Extracting: (%d to %d) for offset #%d over %d entries.\n", n->ext->msi, n->ext->lsi, bit, n->numhist));

	np = (nptr)calloc_2(1, sizeof(struct Node));
	sprintf(nam+offset, "[%d]", actual);
	len = offset + strlen(nam+offset);
	np->nname = (char *)malloc_2(len+1);
	strcpy(np->nname, nam);

	exp = (exptr) calloc_2(1, sizeof(struct ExpandReferences));
	exp->parent=n;							/* point to parent */
	exp->parentbit=bit;
	exp->actual=actual;						/* actual bitnum in [] */
	np->expansion = exp;						/* can be safely deleted if expansion set like here */

	for(i=0;i<n->numhist;i++)
		{
		h=n->harray[i];
		if((h->time<min_time)||(h->time>max_time))
			{
			if(np->curr)
				{
				htemp = (hptr) calloc_2(1, sizeof(struct HistEnt));				
				htemp->v.val = 1;			/* 'x' */
				htemp->time = h->time;
				np->curr->next = htemp;
				np->curr = htemp;
				}
				else
				{
				np->head.v.val = 1; 			/* 'x' */
				np->head.time  = h->time;
				np->curr = &(np->head);
				}

			np->numhist++;
			}
			else
			{
			unsigned char val = h->v.vector[bit];
			switch(val)
				{
				case '0':	val = 0; break;
				case 'x':
				case 'X':	val = 1; break;
				case 'z':
				case 'Z':	val = 2; break;
				case '1':	val = 3; break;
				default:	break;			/* leave val alone as it's been converted already.. */
				}

			if(np->curr->v.val != val) 		/* curr will have been established already by 'x' at time: -1 */
				{
				htemp = (hptr) calloc_2(1, sizeof(struct HistEnt));				
				htemp->v.val = val;
				htemp->time = h->time;
				np->curr->next = htemp;
				np->curr = htemp;
				np->numhist++;
				}
			}
		}

	np->harray = (hptr *)calloc_2(np->numhist, sizeof(hptr));
	htemp = &(np->head);
	for(j=0;j<np->numhist;j++)
		{
		np->harray[j] = htemp;
		htemp = htemp->next;
		}

	return(np);
	}
}

/******************************************************************/

/*
 * this only frees nodes created via expansion in ExpandNode() functions above!
 */
void DeleteNode(nptr n)
{
int i;

if(n->expansion)
	{
	DEBUG(fprintf(stderr, "DeleteNode: '%s', refcnt = %d\n", n->nname, n->expansion->refcnt));
	if(n->expansion->refcnt==0)
		{
		for(i=1;i<n->numhist;i++)	/* 1st is actually part of the Node! */
			{
			free_2(n->harray[i]);	
			}
		free_2(n->harray);
		free_2(n->expansion);
		free_2(n->nname);
		free_2(n);
		}
		else
		{
		n->expansion->refcnt--;
		}
	}
}

/******************************************************************/

