/* 
 * Copyright (c) Tony Bybell 1999-2001.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */


/*
 * vcd.c 			23jan99ajb
 * evcd parts 			29jun99ajb
 * profiler optimizations 	15jul99ajb
 * more profiler optimizations	25jan00ajb
 * finsim parameter fix		26jan00ajb
 * vector rechaining code	03apr00ajb
 * multiple var section code	06apr00ajb
 * fix for duplicate nets	19dec00ajb
 * support for alt hier seps	23dec00ajb
 * fix for rcs identifiers	16jan01ajb
 * coredump fix for bad VCD	04apr02ajb
 * min/maxid speedup            27feb03ajb
 * bugfix on min/maxid speedup  06jul03ajb
 */
#include "vcd.h"

#undef VCD_BSEARCH_IS_PERFECT		/* bsearch is imperfect under linux, but OK under AIX */

char autocoalesce=1, autocoalesce_reversal=0;

int vcd_explicit_zero_subscripts=-1;  /* 0=yes, -1=no */
char convert_to_reals=0;
char atomic_vectors=1;
char make_vcd_save_file=0;
FILE *vcd_save_handle=NULL;

static FILE *vcd_handle=NULL;
static char vcd_is_compressed=0;

static void add_histent(TimeType time, struct Node *n, char ch, int regadd, char *vector);
static void add_tail_histents(void);
static void vcd_build_symbols(void);
static void vcd_cleanup(void);
static void evcd_strcpy(char *dst, char *src);

static int vcdbyteno=0;		/* really should be size_t, but this is only used for debugging mangled traces */
static int error_count=0;	/* should always be zero */

static int header_over=0;
static int dumping_off=0;
static TimeType start_time=-1;
static TimeType end_time=-1;
static TimeType current_time=-1;
static TimeType time_scale=1;	/* multiplier is 1, 10, 100 */

static int num_glitches=0;
static int num_glitch_regions=0;

static char vcd_hier_delimeter[2]={0, 0};   /* fill in after rc reading code */

static struct vcdsymbol *pv=NULL, *rootv=NULL;
static char *vcdbuf=NULL, *vst=NULL, *vend=NULL;

/******************************************************************/

void strcpy_vcdalt(char *too, char *from, char delim)
{
char ch;

do
	{
	ch=*(from++);
	if(ch==delim)
		{
		ch=hier_delimeter;
		}
	} while((*(too++)=ch));
}

/******************************************************************/

static struct slist *slistroot=NULL, *slistcurr=NULL;
static char *slisthier=NULL;
static int slisthier_len=0;

/******************************************************************/

enum Tokens   { T_VAR, T_END, T_SCOPE, T_UPSCOPE,
		T_COMMENT, T_DATE, T_DUMPALL, T_DUMPOFF, T_DUMPON,
		T_DUMPVARS, T_ENDDEFINITIONS, 
		T_DUMPPORTS, T_DUMPPORTSOFF, T_DUMPPORTSON, T_DUMPPORTSALL,
		T_TIMESCALE, T_VERSION, T_VCDCLOSE,
		T_EOF, T_STRING, T_UNKNOWN_KEY };

char *tokens[]={ "var", "end", "scope", "upscope",
		 "comment", "date", "dumpall", "dumpoff", "dumpon",
		 "dumpvars", "enddefinitions",
		 "dumpports", "dumpportsoff", "dumpportson", "dumpportsall",
		 "timescale", "version", "vcdclose",
		 "", "", "" };

#define NUM_TOKENS 18

static int T_MAX_STR=1024;	/* was originally a const..now it reallocs */
static char *yytext=NULL;
static int yylen=0, yylen_cache=0;

#define T_GET tok=get_token();if((tok==T_END)||(tok==T_EOF))break;

/******************************************************************/

static struct vcdsymbol *vcdsymroot=NULL, *vcdsymcurr=NULL;
static struct vcdsymbol **sorted=NULL;
static struct vcdsymbol **indexed=NULL;

enum VarTypes { V_EVENT, V_PARAMETER,
		V_INTEGER, V_REAL, V_REG, V_SUPPLY0,
		V_SUPPLY1, V_TIME, V_TRI, V_TRIAND, V_TRIOR,
		V_TRIREG, V_TRI0, V_TRI1, V_WAND, V_WIRE, V_WOR, V_PORT,
		V_END, V_LB, V_COLON, V_RB, V_STRING };

char *vartypes[]={ "event", "parameter",
		"integer", "real", "reg", "supply0",
		"supply1", "time", "tri", "triand", "trior",
		"trireg", "tri0", "tri1", "wand", "wire", "wor", "port",
		"$end", "", "", "", ""};

#define NUM_VTOKENS 19

static int numsyms=0;

/******************************************************************/

/*
 * histent structs are NEVER freed so this is OK..
 */
#define VCD_HISTENT_GRANULARITY 100
static struct HistEnt *he_curr=NULL, *he_fini=NULL;

struct HistEnt *histent_calloc(void)
{
if(he_curr==he_fini)
	{
	he_curr=(struct HistEnt *)calloc_2(VCD_HISTENT_GRANULARITY, sizeof(struct HistEnt));
	he_fini=he_curr+VCD_HISTENT_GRANULARITY;
	}

return(he_curr++);	
}

/******************************************************************/

static struct queuedevent *queuedevents=NULL;

/******************************************************************/
 
static unsigned int vcd_minid = ~0;
static unsigned int vcd_maxid = 0;

static unsigned int vcdid_hash(char *s, int len)
{  
unsigned int val=0;
int i;

s+=(len-1);
                 
for(i=0;i<len;i++)
        {
        val *= 95;				/* was 94 but XL uses '!' as right hand side chars which act as leading zeros */
        val += (((unsigned char)*s) - 32);	/* was 33 but XL ... */
        s--;
        }

return(val);
}

/******************************************************************/

/*
 * bsearch compare
 */
static int vcdsymbsearchcompare(const void *s1, const void *s2)
{
char *v1;
struct vcdsymbol *v2;

v1=(char *)s1;
v2=*((struct vcdsymbol **)s2);

return(strcmp(v1, v2->id));
}


/*
 * actual bsearch
 */
static struct vcdsymbol *bsearch_vcd(char *key, int len)
{
struct vcdsymbol **v;
struct vcdsymbol *t;

if(indexed)
        {
        unsigned int hsh = vcdid_hash(key, len);
        if((hsh>=vcd_minid)&&(hsh<=vcd_maxid))
                {
                return(indexed[hsh-vcd_minid]);
                }

	return(NULL);
        }

if(sorted)
	{
	v=(struct vcdsymbol **)bsearch(key, sorted, numsyms, 
		sizeof(struct vcdsymbol *), vcdsymbsearchcompare);

	if(v)
		{
		#ifndef VCD_BSEARCH_IS_PERFECT
			for(;;)
				{
				t=*v;
		
				if((v==sorted)||(strcmp((*(--v))->id, key)))
					{
					return(t);
					}
				}
		#else
			return(*v);
		#endif
		}
		else
		{
		return(NULL);
		}
	}
	else
	{
	static int err = 0;
	if(!err)
		{
		fprintf(stderr, "Near byte %d, VCD search table NULL..is this a VCD file?\n", vcdbyteno+(vst-vcdbuf));
		err=1;
		}
	return(NULL);
	}
}


/*
 * sort on vcdsymbol pointers
 */
static int vcdsymcompare(const void *s1, const void *s2)
{
struct vcdsymbol *v1, *v2;

v1=*((struct vcdsymbol **)s1);
v2=*((struct vcdsymbol **)s2);

return(strcmp(v1->id, v2->id));
}


/*
 * create sorted (by id) table
 */
static void create_sorted_table(void)
{
struct vcdsymbol *v;
struct vcdsymbol **pnt;
unsigned int vcd_distance;
int i;

if(sorted) 
	{
	free_2(sorted);	/* this means we saw a 2nd enddefinition chunk! */
	sorted=NULL;
	}

if(indexed)
	{
	free_2(indexed);
	indexed=NULL;
	}

if(numsyms)
	{
        vcd_distance = vcd_maxid - vcd_minid + 1;

        if(vcd_distance <= VCD_INDEXSIZ)
                {
                indexed = (struct vcdsymbol **)calloc_2(vcd_distance, sizeof(struct vcdsymbol *));
         
		/* printf("%d symbols span ID range of %d, using indexing...\n", numsyms, vcd_distance); */

                v=vcdsymroot;
                while(v)
                        {
                        if(!indexed[v->nid - vcd_minid]) indexed[v->nid - vcd_minid] = v;
                        v=v->next;
                        }
                }
                else
		{	
		pnt=sorted=(struct vcdsymbol **)calloc_2(numsyms, sizeof(struct vcdsymbol *));
		v=vcdsymroot;
		while(v)
			{
			*(pnt++)=v;
			v=v->next;
			}
	
		qsort(sorted, numsyms, sizeof(struct vcdsymbol *), vcdsymcompare);
		}
	}
}

/******************************************************************/

/*
 * single char get inlined/optimized
 */
static void getch_alloc(void)
{
vend=vst=vcdbuf=(char *)calloc_2(1,VCD_BSIZ);
}

static void getch_free(void)
{
free_2(vcdbuf);
vcdbuf=vst=vend=NULL;
}


static int getch_fetch(void)
{
size_t rd;

if(feof(vcd_handle)||errno) return(-1);

vcdbyteno+=(vend-vcdbuf);
rd=fread(vcdbuf, sizeof(char), VCD_BSIZ, vcd_handle);
vend=(vst=vcdbuf)+rd;

if(!rd) return(-1);

return((int)(*(vst++)));
}

#define getch() ((vst!=vend)?((int)(*(vst++))):(getch_fetch()))


static char *varsplit=NULL, *vsplitcurr=NULL;
static int getch_patched(void)
{
char ch;

ch=*vsplitcurr;
if(!ch)
	{
	return(-1);
	}
	else
	{
	vsplitcurr++;
	return((int)ch);
	}
}

/*
 * simple tokenizer
 */
static int get_token(void)
{
int ch;
int i, len=0;
int is_string=0;
char *yyshadow;

for(;;)
	{
	ch=getch();
	if(ch<0) return(T_EOF);
	if(ch<=' ') continue;	/* val<=' ' is a quick whitespace check      */
	break;			/* (take advantage of fact that vcd is text) */
	}
if(ch=='$') 
	{
	yytext[len++]=ch;
	for(;;)
		{
		ch=getch();
		if(ch<0) return(T_EOF);
		if(ch<=' ') continue;
		break;
		}
	}
	else
	{
	is_string=1;
	}

for(yytext[len++]=ch;;yytext[len++]=ch)
	{
	if(len==T_MAX_STR)
		{
		yytext=(char *)realloc_2(yytext, (T_MAX_STR=T_MAX_STR*2)+1);
		}
	ch=getch();
	if(ch<=' ') break;
	}
yytext[len]=0;	/* terminator */

if(is_string) 
	{
	yylen=len;
	return(T_STRING);
	}

yyshadow=yytext;
do
{
yyshadow++;
for(i=0;i<NUM_TOKENS;i++)
	{
	if(!strcmp(yyshadow,tokens[i]))
		{
		return(i);
		}
	}

} while(*yyshadow=='$'); /* fix for RCS ids in version strings */

return(T_UNKNOWN_KEY);
}


static int var_prevch=0;
static int get_vartoken_patched(void)
{
int ch;
int i, len=0;

if(!var_prevch)
	{
	for(;;)
		{
		ch=getch_patched();
		if(ch<0) { free_2(varsplit); varsplit=NULL; return(V_END); }
		if((ch==' ')||(ch=='\t')||(ch=='\n')||(ch=='\r')) continue;
		break;
		}
	}
	else
	{
	ch=var_prevch;
	var_prevch=0;
	}
	
if(ch=='[') return(V_LB);
if(ch==':') return(V_COLON);
if(ch==']') return(V_RB);

for(yytext[len++]=ch;;yytext[len++]=ch)
	{
	if(len==T_MAX_STR)
		{
		yytext=(char *)realloc_2(yytext, (T_MAX_STR=T_MAX_STR*2)+1);
		}
	ch=getch_patched();
	if(ch<0) break;
	if((ch==':')||(ch==']'))
		{
		var_prevch=ch;
		break;
		}
	}
yytext[len]=0;	/* terminator */

for(i=0;i<NUM_VTOKENS;i++)
	{
	if(!strcmp(yytext,vartypes[i]))
		{
		if(ch<0) { free_2(varsplit); varsplit=NULL; }
		return(i);
		}
	}

yylen=len;
if(ch<0) { free_2(varsplit); varsplit=NULL; }
return(V_STRING);
}

static int get_vartoken(void)
{
int ch;
int i, len=0;

if(varsplit)
	{
	int rc=get_vartoken_patched();
	if(rc!=V_END) return(rc);
	var_prevch=0;
	}

if(!var_prevch)
	{
	for(;;)
		{
		ch=getch();
		if(ch<0) return(V_END);
		if((ch==' ')||(ch=='\t')||(ch=='\n')||(ch=='\r')) continue;
		break;
		}
	}
	else
	{
	ch=var_prevch;
	var_prevch=0;
	}
	
if(ch=='[') return(V_LB);
if(ch==':') return(V_COLON);
if(ch==']') return(V_RB);

for(yytext[len++]=ch;;yytext[len++]=ch)
	{
	if(len==T_MAX_STR)
		{
		yytext=(char *)realloc_2(yytext, (T_MAX_STR=T_MAX_STR*2)+1);
		}
	ch=getch();
	if((ch==' ')||(ch=='\t')||(ch=='\n')||(ch=='\r')||(ch<0)) break;
	if((ch=='[')&&(yytext[0]!='\\'))
		{
		varsplit=yytext+len;		/* keep looping so we get the *last* one */
		}
	else
	if(((ch==':')||(ch==']'))&&(!varsplit)&&(yytext[0]!='\\'))
		{
		var_prevch=ch;
		break;
		}
	}
yytext[len]=0;	/* absolute terminator */
if((varsplit)&&(yytext[len-1]==']'))
	{
	char *vst;
	vst=malloc_2(strlen(varsplit)+1);
	strcpy(vst, varsplit);

	*varsplit=0x00;		/* zero out var name at the left bracket */
	len=varsplit-yytext;

	varsplit=vsplitcurr=vst;
	var_prevch=0;
	}
	else
	{
	varsplit=NULL;
	}

for(i=0;i<NUM_VTOKENS;i++)
	{
	if(!strcmp(yytext,vartypes[i]))
		{
		return(i);
		}
	}

yylen=len;
return(V_STRING);
}

static int get_strtoken(void)
{
int ch;
int len=0;

if(!var_prevch)
      {
      for(;;)
              {
              ch=getch();
              if(ch<0) return(V_END);
              if((ch==' ')||(ch=='\t')||(ch=='\n')||(ch=='\r')) continue;
              break;
              }
      }
      else
      {
      ch=var_prevch;
      var_prevch=0;
      }
      
for(yytext[len++]=ch;;yytext[len++]=ch)
      {
	if(len==T_MAX_STR)
		{
		yytext=(char *)realloc_2(yytext, (T_MAX_STR=T_MAX_STR*2)+1);
		}
      ch=getch();
      if((ch==' ')||(ch=='\t')||(ch=='\n')||(ch=='\r')||(ch<0)) break;
      }
yytext[len]=0;        /* terminator */

yylen=len;
return(V_STRING);
}

static void sync_end(char *hdr)
{
int tok;

if(hdr) DEBUG(fprintf(stderr,"%s",hdr));
for(;;)
	{
	tok=get_token();
	if((tok==T_END)||(tok==T_EOF)) break;
	if(hdr)DEBUG(fprintf(stderr," %s",yytext));
	}
if(hdr) DEBUG(fprintf(stderr,"\n"));
}

static char *build_slisthier(void)
{
struct slist *s;
int len=0;

if(!slistroot)
	{
	if(slisthier)
		{
		free_2(slisthier);
		}

	slisthier_len=0;
	slisthier=(char *)malloc_2(1);
	*slisthier=0;
	return(slisthier);
	}

s=slistroot; len=0;
while(s)
	{
	len+=s->len+(s->next?1:0);
	s=s->next;
	}

slisthier=(char *)malloc_2((slisthier_len=len)+1);
s=slistroot; len=0;
while(s)
	{
	strcpy(slisthier+len,s->str);
	len+=s->len;
	if(s->next)
		{
		strcpy(slisthier+len,vcd_hier_delimeter);
		len++;
		}
	s=s->next;
	}
return(slisthier);
}


void append_vcd_slisthier(char *str)
{
struct slist *s;
s=(struct slist *)calloc_2(1,sizeof(struct slist));
s->len=strlen(str);
s->str=(char *)malloc_2(s->len+1);
strcpy(s->str,str);

if(slistcurr)
	{
	slistcurr->next=s;
	slistcurr=s;
	}
	else
	{
	slistcurr=slistroot=s;
	}

build_slisthier();
DEBUG(fprintf(stderr, "SCOPE: %s\n",slisthier));
}


static void parse_valuechange(void)
{
struct vcdsymbol *v;
char *vector;
int vlen;

switch(yytext[0])
	{
	case '0':
	case '1':
	case 'x':
	case 'X':
	case 'z':
	case 'Z':
		if(yylen>1)
			{
			v=bsearch_vcd(yytext+1, yylen-1);	
			if(!v)
				{
				fprintf(stderr,"Near byte %d, Unknown VCD identifier: '%s'\n",vcdbyteno+(vst-vcdbuf),yytext+1);
				}
				else
				{
				if(v->vartype!=V_EVENT)
					{
					v->value[0]=yytext[0];
					DEBUG(fprintf(stderr,"%s = '%c'\n",v->name,v->value[0]));
					add_histent(current_time,v->narray[0],v->value[0],1, NULL);
					}
					else
					{
					v->value[0]=(dumping_off)?'x':'1'; /* only '1' is relevant */
					if(current_time!=(v->ev->last_event_time+1))
						{
						/* dump degating event */
						DEBUG(fprintf(stderr,"#"TTFormat" %s = '%c' (event)\n",v->ev->last_event_time+1,v->name,'0'));
						add_histent(v->ev->last_event_time+1,v->narray[0],'0',1, NULL);
						}
					DEBUG(fprintf(stderr,"%s = '%c' (event)\n",v->name,v->value[0]));
					add_histent(current_time,v->narray[0],v->value[0],1, NULL);
					v->ev->last_event_time=current_time;
					}
				}
			}
			else
			{
			fprintf(stderr,"Near byte %d, Malformed VCD identifier\n", vcdbyteno+(vst-vcdbuf));
			}
		break;

	case 'b':
	case 'B':
		{
		/* extract binary number then.. */
		vector=malloc_2(yylen_cache=yylen); 
		strcpy(vector,yytext+1);
		vlen=yylen-1;

		get_strtoken();
		v=bsearch_vcd(yytext, yylen);	
		if(!v)
			{
			fprintf(stderr,"Near byte %d, Unknown identifier: '%s'\n",vcdbyteno+(vst-vcdbuf), yytext);
			free_2(vector);
			}
			else
			{
			if ((v->vartype==V_REAL)||((convert_to_reals)&&((v->vartype==V_INTEGER)||(v->vartype==V_PARAMETER))))
				{
				double *d;
				char *pnt;
				char ch;
				TimeType k=0;
		
				pnt=vector;
				while((ch=*(pnt++))) { k=(k<<1)|((ch=='1')?1:0); }
				free_2(vector);
			
				d=malloc_2(sizeof(double));
				*d=(double)k;
			
				if(!v)
					{
					fprintf(stderr,"Near byte %d, Unknown identifier: '%s'\n",vcdbyteno+(vst-vcdbuf), yytext);
					free_2(d);
					}
					else
					{
					add_histent(current_time, v->narray[0],'g',1,(char *)d);
					}
				break;
				}

			if(vlen<v->size) 	/* fill in left part */
				{
				char extend;
				int i, fill;

				extend=(vector[0]=='1')?'0':vector[0];

				fill=v->size-vlen;				
				for(i=0;i<fill;i++)
					{
					v->value[i]=extend;
					}
				strcpy(v->value+fill,vector);
				}
			else if(vlen==v->size) 	/* straight copy */
				{
				strcpy(v->value,vector);
				}
			else			/* too big, so copy only right half */
				{
				int skip;

				skip=vlen-v->size;
				strcpy(v->value,vector+skip);
				}
			DEBUG(fprintf(stderr,"%s = '%s'\n",v->name, v->value));

			if((v->size==1)||(!atomic_vectors))
				{
				int i;
				for(i=0;i<v->size;i++)
					{
					add_histent(current_time, v->narray[i],v->value[i],1, NULL);
					}
				free_2(vector);
				}
				else
				{
				if(yylen_cache!=(v->size+1))
					{
					free_2(vector);
					vector=malloc_2(v->size+1);
					}
				strcpy(vector,v->value);
				add_histent(current_time, v->narray[0],0,1,vector);
				}

			}
		break;
		}

	case 'p':
		/* extract port dump value.. */
		vector=malloc_2(yylen_cache=yylen); 
		strcpy(vector,yytext+1);
		vlen=yylen-1;

		get_strtoken();	/* throw away 0_strength_component */
		get_strtoken(); /* throw away 0_strength_component */
		get_strtoken(); /* this is the id                  */
		v=bsearch_vcd(yytext, yylen);	
		if(!v)
			{
			fprintf(stderr,"Near byte %d, Unknown identifier: '%s'\n",vcdbyteno+(vst-vcdbuf), yytext);
			free_2(vector);
			}
			else
			{
			if ((v->vartype==V_REAL)||((convert_to_reals)&&((v->vartype==V_INTEGER)||(v->vartype==V_PARAMETER))))
				{
				double *d;
				char *pnt;
				char ch;
				TimeType k=0;
		
				pnt=vector;
				while((ch=*(pnt++))) { k=(k<<1)|((ch=='1')?1:0); }
				free_2(vector);
			
				d=malloc_2(sizeof(double));
				*d=(double)k;
			
				if(!v)
					{
					fprintf(stderr,"Near byte %d, Unknown identifier: '%s'\n",vcdbyteno+(vst-vcdbuf), yytext);
					free_2(d);
					}
					else
					{
					add_histent(current_time, v->narray[0],'g',1,(char *)d);
					}
				break;
				}

			if(vlen<v->size) 	/* fill in left part */
				{
				char extend;
				int i, fill;

				extend='0';

				fill=v->size-vlen;				
				for(i=0;i<fill;i++)
					{
					v->value[i]=extend;
					}
				evcd_strcpy(v->value+fill,vector);
				}
			else if(vlen==v->size) 	/* straight copy */
				{
				evcd_strcpy(v->value,vector);
				}
			else			/* too big, so copy only right half */
				{
				int skip;

				skip=vlen-v->size;
				evcd_strcpy(v->value,vector+skip);
				}
			DEBUG(fprintf(stderr,"%s = '%s'\n",v->name, v->value));

			if((v->size==1)||(!atomic_vectors))
				{
				int i;
				for(i=0;i<v->size;i++)
					{
					add_histent(current_time, v->narray[i],v->value[i],1, NULL);
					}
				free_2(vector);
				}
				else
				{
				if(yylen_cache<v->size)
					{
					free_2(vector);
					vector=malloc_2(v->size+1);
					}
				strcpy(vector,v->value);
				add_histent(current_time, v->narray[0],0,1,vector);
				}
			}
		break;


	case 'r':
	case 'R':
		{
		double *d;

		d=malloc_2(sizeof(double));
		sscanf(yytext+1,"%lg",d);
		
		get_strtoken();
		v=bsearch_vcd(yytext, yylen);	
		if(!v)
			{
			fprintf(stderr,"Near byte %d, Unknown identifier: '%s'\n",vcdbyteno+(vst-vcdbuf), yytext);
			free_2(d);
			}
			else
			{
			add_histent(current_time, v->narray[0],'g',1,(char *)d);
			}

		break;
		}

#ifndef STRICT_VCD_ONLY
	case 's':
	case 'S':
		{
		char *d;

		d=(char *)malloc_2(yylen);
		strcpy(d, yytext+1);
		
		get_strtoken();
		v=bsearch_vcd(yytext, yylen);	
		if(!v)
			{
			fprintf(stderr,"Near byte %d, Unknown identifier: '%s'\n",vcdbyteno+(vst-vcdbuf), yytext);
			free_2(d);
			}
			else
			{
			add_histent(current_time, v->narray[0],'s',1,(char *)d);
			}

		break;
		}
#endif
	}

}


static void evcd_strcpy(char *dst, char *src)
{
static char *evcd="DUNZduLHXTlh01?FAaBbCcf";
static char  *vcd="01xz0101xz0101xzxxxxxxx";

char ch;
int i;

while((ch=*src))
	{
	for(i=0;i<23;i++)
		{
		if(evcd[i]==ch)
			{
			*dst=vcd[i];
			break;
			}
		}	
	if(i==23) *dst='x';

	src++;
	dst++;
	}

*dst=0;	/* null terminate destination */
}


static void vcd_parse(void)
{
int tok;

for(;;)
	{
	switch(tok=get_token())
		{
		case T_COMMENT:
			sync_end("COMMENT:");
			break;
		case T_DATE:
			sync_end("DATE:");
			break;
		case T_VERSION:
			sync_end("VERSION:");
			break;
		case T_TIMESCALE:
			{
			int vtok;
			int i;
			char prefix=' ';

			vtok=get_token();
			if((vtok==T_END)||(vtok==T_EOF)) break;
			time_scale=atoi_64(yytext);
			if(!time_scale) time_scale=1;
			for(i=0;i<yylen;i++)
				{
				if((yytext[i]<'0')||(yytext[i]>'9'))
					{
					prefix=yytext[i];
					break;
					}
				}
			if(prefix==' ')
				{
				vtok=get_token();
				if((vtok==T_END)||(vtok==T_EOF)) break;
				prefix=yytext[0];		
				}
			switch(prefix)
				{
				case ' ':
				case 'm':
				case 'u':
				case 'n':
				case 'p':
				case 'f':
					time_dimension=prefix;
					break;
				case 's':
					time_dimension=' ';
					break;
				default:	/* unknown */
					time_dimension='n';
					break;
				}

			DEBUG(fprintf(stderr,"TIMESCALE: "TTFormat" %cs\n",time_scale, time_dimension));
			sync_end(NULL);
			}
			break;
		case T_SCOPE:
			T_GET;
			T_GET;
			if(tok==T_STRING)
				{
				struct slist *s;
				s=(struct slist *)calloc_2(1,sizeof(struct slist));
				s->len=yylen;
				s->str=(char *)malloc_2(yylen+1);
				strcpy(s->str,yytext);

				if(slistcurr)
					{
					slistcurr->next=s;
					slistcurr=s;
					}
					else
					{
					slistcurr=slistroot=s;
					}

				build_slisthier();
				DEBUG(fprintf(stderr, "SCOPE: %s\n",slisthier));
				}
			sync_end(NULL);
			break;
		case T_UPSCOPE:
			if(slistroot)
				{
				struct slist *s;

				s=slistroot;
				if(!s->next)
					{
					free_2(s->str);
					free_2(s);
					slistroot=slistcurr=NULL;
					}
				else
				for(;;)
					{
					if(!s->next->next)
						{
						free_2(s->next->str);
						free_2(s->next);
						s->next=NULL;
						slistcurr=s;
						break;
						}
					s=s->next;
					}
				build_slisthier();
				DEBUG(fprintf(stderr, "SCOPE: %s\n",slisthier));
				}
			sync_end(NULL);
			break;
		case T_VAR:
			if((header_over)&&(0))
			{
			fprintf(stderr,"$VAR encountered after $ENDDEFINITIONS near byte %d.  VCD is malformed, exiting.\n",vcdbyteno+(vst-vcdbuf));
			exit(0);
			}
			else
			{
			int vtok;
			struct vcdsymbol *v=NULL;

			var_prevch=0;
			varsplit=NULL;
			vtok=get_vartoken();
			if(vtok>V_PORT) goto bail;

			v=(struct vcdsymbol *)calloc_2(1,sizeof(struct vcdsymbol));
			v->vartype=vtok;
			v->msi=v->lsi=vcd_explicit_zero_subscripts; /* indicate [un]subscripted status */

			if(vtok==V_PORT)
				{
				vtok=get_vartoken();
				if(vtok==V_STRING)
					{
					v->size=atoi_64(yytext);
					if(!v->size) v->size=1;
					}
					else 
					if(vtok==V_LB)
					{
					vtok=get_vartoken();
					if(vtok==V_END) goto err;
					if(vtok!=V_STRING) goto err;
					v->msi=atoi_64(yytext);
					vtok=get_vartoken();
					if(vtok==V_RB)
						{
						v->lsi=v->msi;
						v->size=1;
						}
						else
						{
						if(vtok!=V_COLON) goto err;
						vtok=get_vartoken();
						if(vtok!=V_STRING) goto err;
						v->lsi=atoi_64(yytext);
						vtok=get_vartoken();
						if(vtok!=V_RB) goto err;

						if(v->msi>v->lsi)
							{
							v->size=v->msi-v->lsi+1;
							}
							else
							{
							v->size=v->lsi-v->msi+1;
							}
						}
					}
					else goto err;

				vtok=get_strtoken();
				if(vtok==V_END) goto err;
				v->id=(char *)malloc_2(yylen+1);
				strcpy(v->id, yytext);
                                v->nid=vcdid_hash(yytext,yylen);

                                if(v->nid < vcd_minid) vcd_minid = v->nid;
                                if(v->nid > vcd_maxid) vcd_maxid = v->nid;

				vtok=get_vartoken();
				if(vtok!=V_STRING) goto err;
				if(slisthier_len)
					{
					v->name=(char *)malloc_2(slisthier_len+1+yylen+1);
					strcpy(v->name,slisthier);
					strcpy(v->name+slisthier_len,vcd_hier_delimeter);
					if(alt_hier_delimeter)
						{
						strcpy_vcdalt(v->name+slisthier_len+1,yytext,alt_hier_delimeter);
						}
						else
						{
						strcpy(v->name+slisthier_len+1,yytext);
						}
					}
					else
					{
					v->name=(char *)malloc_2(yylen+1);
					if(alt_hier_delimeter)
						{
						strcpy_vcdalt(v->name,yytext,alt_hier_delimeter);
						}
						else
						{
						strcpy(v->name,yytext);
						}
					}

                                if(pv)
                                        { 
                                        if(!strcmp(pv->name,v->name))
                                                {
                                                pv->chain=v;
                                                v->root=rootv;
                                                if(pv==rootv) pv->root=rootv;
                                                }
                                                else
                                                {
                                                rootv=v;
                                                }
                                        }
					else
					{
					rootv=v;
					}
                                pv=v;
				}
				else	/* regular vcd var, not an evcd port var */
				{
				vtok=get_vartoken();
				if(vtok==V_END) goto err;
				v->size=atoi_64(yytext);
				if(!v->size) v->size=1;
				vtok=get_strtoken();
				if(vtok==V_END) goto err;
				v->id=(char *)malloc_2(yylen+1);
				strcpy(v->id, yytext);
                                v->nid=vcdid_hash(yytext,yylen);
                                
                                if(v->nid < vcd_minid) vcd_minid = v->nid;
                                if(v->nid > vcd_maxid) vcd_maxid = v->nid;

				vtok=get_vartoken();
				if(vtok!=V_STRING) goto err;
				if(slisthier_len)
					{
					v->name=(char *)malloc_2(slisthier_len+1+yylen+1);
					strcpy(v->name,slisthier);
					strcpy(v->name+slisthier_len,vcd_hier_delimeter);
					if(alt_hier_delimeter)
						{
						strcpy_vcdalt(v->name+slisthier_len+1,yytext,alt_hier_delimeter);
						}
						else
						{
						strcpy(v->name+slisthier_len+1,yytext);
						}
					}
					else
					{
					v->name=(char *)malloc_2(yylen+1);
					if(alt_hier_delimeter)
						{
						strcpy_vcdalt(v->name,yytext,alt_hier_delimeter);
						}
						else
						{
						strcpy(v->name,yytext);
						}
					}

                                if(pv)
                                        { 
                                        if(!strcmp(pv->name,v->name))
                                                {
                                                pv->chain=v;
                                                v->root=rootv;
                                                if(pv==rootv) pv->root=rootv;
                                                }
                                                else
                                                {
                                                rootv=v;
                                                }
                                        }
					else
					{
					rootv=v;
					}
                                pv=v;
				
				vtok=get_vartoken();
				if(vtok==V_END) goto dumpv;
				if(vtok!=V_LB) goto err;
				vtok=get_vartoken();
				if(vtok!=V_STRING) goto err;
				v->msi=atoi_64(yytext);
				vtok=get_vartoken();
				if(vtok==V_RB)
					{
					v->lsi=v->msi;
					goto dumpv;
					}
				if(vtok!=V_COLON) goto err;
				vtok=get_vartoken();
				if(vtok!=V_STRING) goto err;
				v->lsi=atoi_64(yytext);
				vtok=get_vartoken();
				if(vtok!=V_RB) goto err;
				}

			dumpv:
			if((v->vartype==V_REAL)||((convert_to_reals)&&((v->vartype==V_INTEGER)||(v->vartype==V_PARAMETER))))
				{
				v->vartype=V_REAL;
				v->size=1;		/* override any data we parsed in */
				v->msi=v->lsi=0;
				}
			else
			if((v->size>1)&&(v->msi<=0)&&(v->lsi<=0))
				{
				if(v->vartype==V_EVENT) 
					{
					v->size=1;
					}
					else
					{
					/* any criteria for the direction here? */
					v->msi=v->size-1;	
					v->lsi=0;
					}
				}
			else
			if((v->msi>v->lsi)&&((v->msi-v->lsi+1)!=v->size))
				{
				if((v->vartype!=V_EVENT)&&(v->vartype!=V_PARAMETER)) goto err;
				v->size=v->msi-v->lsi+1;
				}
			else
			if((v->lsi>=v->msi)&&((v->lsi-v->msi+1)!=v->size)) 
				{
				if((v->vartype!=V_EVENT)&&(v->vartype!=V_PARAMETER)) goto err;
				v->size=v->msi-v->lsi+1;
				}

			/* initial conditions */
			v->value=(char *)malloc_2(v->size+1);
			v->value[v->size]=0;
			v->narray=(struct Node **)calloc_2(v->size,sizeof(struct Node *));
				{
				int i;
				if(atomic_vectors)
					{
					for(i=0;i<v->size;i++)
						{
						v->value[i]='x';
						}
					v->narray[0]=(struct Node *)calloc_2(1,sizeof(struct Node));
					v->narray[0]->head.time=-1;
					v->narray[0]->head.v.val=1;
					}
					else
					{
					for(i=0;i<v->size;i++)
						{
						v->value[i]='x';
	
						v->narray[i]=(struct Node *)calloc_2(1,sizeof(struct Node));
						v->narray[i]->head.time=-1;
						v->narray[i]->head.v.val=1;
						}
					}
				}

			if(v->vartype==V_EVENT)
				{
				struct queuedevent *q;
				v->ev=q=(struct queuedevent *)calloc_2(1,sizeof(struct queuedevent));
				q->sym=v;
				q->last_event_time=-1;		
				q->next=queuedevents;
				queuedevents=q;		
				}

			if(!vcdsymroot)
				{
				vcdsymroot=vcdsymcurr=v;
				}
				else
				{
				vcdsymcurr->next=v;
				vcdsymcurr=v;
				}
			numsyms++;

			if(vcd_save_handle)
				{
				if(v->msi==v->lsi)
					{
					if(v->vartype==V_REAL)
						{
						fprintf(vcd_save_handle,"%s\n",v->name);
						}
						else
						{
						if(v->msi>=0)
							{
							if(!vcd_explicit_zero_subscripts)
								fprintf(vcd_save_handle,"%s%c%d\n",v->name,hier_delimeter,v->msi);
								else
								fprintf(vcd_save_handle,"%s[%d]\n",v->name,v->msi);
							}
							else
							{
							fprintf(vcd_save_handle,"%s\n",v->name);
							}
						}
					}
					else
					{
					int i;

					if(!atomic_vectors)
						{
						fprintf(vcd_save_handle,"#%s[%d:%d]",v->name,v->msi,v->lsi);
						if(v->msi>v->lsi)
							{
							for(i=v->msi;i>=v->lsi;i--)
								{
								if(!vcd_explicit_zero_subscripts)
									fprintf(vcd_save_handle," %s%c%d",v->name,hier_delimeter,i);
									else
									fprintf(vcd_save_handle," %s[%d]",v->name,i);
								}
							}
							else
							{
							for(i=v->msi;i<=v->lsi;i++)
								{
								if(!vcd_explicit_zero_subscripts)
									fprintf(vcd_save_handle," %s%c%d",v->name,hier_delimeter,i);
									else
									fprintf(vcd_save_handle," %s[%d]",v->name,i);
								}
							}
						fprintf(vcd_save_handle,"\n");
						}
						else
						{
						fprintf(vcd_save_handle,"%s[%d:%d]\n",v->name,v->msi,v->lsi);
						}
					}
				}

			DEBUG(fprintf(stderr,"VAR %s %d %s %s[%d:%d]\n",
				vartypes[v->vartype], v->size, v->id, v->name, 
					v->msi, v->lsi));
			goto bail;
			err:
			if(v)
				{
				error_count++;
				if(v->name) 
					{
					fprintf(stderr, "Near byte %d, $VAR parse error encountered with '%s'\n", vcdbyteno+(vst-vcdbuf), v->name);
					free_2(v->name);
					}
					else
					{
					fprintf(stderr, "Near byte %d, $VAR parse error encountered\n", vcdbyteno+(vst-vcdbuf));
					}
				if(v->id) free_2(v->id);
				if(v->value) free_2(v->value);
				free_2(v);
				}

			bail:
			if(vtok!=V_END) sync_end(NULL);
			break;
			}
		case T_ENDDEFINITIONS:
			header_over=1;	/* do symbol table management here */
			create_sorted_table();
			if((!sorted)&&(!indexed))
				{
				fprintf(stderr, "No symbols in VCD file..nothing to do!\n");
				exit(1);
				}
			if(error_count)
				{
				fprintf(stderr, "\n%d VCD parse errors encountered, exiting.\n", error_count);
				exit(1);
				}
			break;
		case T_STRING:
			if(!header_over)
				{
				header_over=1;	/* do symbol table management here */
				create_sorted_table();
				if((!sorted)&&(!indexed)) break;
				}
				{
				/* catchall for events when header over */
				if(yytext[0]=='#')
					{
					TimeType time;
					time=atoi_64(yytext+1);
					
					if(start_time<0)
						{
						start_time=time;
						}

					current_time=time;
					if(end_time<time) end_time=time;	/* in case of malformed vcd files */
					DEBUG(fprintf(stderr,"#"TTFormat"\n",time));
					}
					else
					{
					parse_valuechange();
					}
				}
			break;
		case T_DUMPALL:	/* dump commands modify vals anyway so */
		case T_DUMPPORTSALL:
			break;	/* just loop through..                 */
		case T_DUMPOFF:
		case T_DUMPPORTSOFF:
			dumping_off=1;
			if((!blackout_regions)||((blackout_regions)&&(blackout_regions->bstart<=blackout_regions->bend)))
				{
				struct blackout_region_t *bt = calloc(1, sizeof(struct blackout_region_t));

				bt->bstart = current_time;
				bt->next = blackout_regions;
				blackout_regions = bt;
				}
			break;
		case T_DUMPON:
		case T_DUMPPORTSON:
			dumping_off=0;
			if((blackout_regions)&&(blackout_regions->bstart>blackout_regions->bend))
				{
				blackout_regions->bend = current_time;
				}
			break;
		case T_DUMPVARS:
		case T_DUMPPORTS:
			if(current_time<0)
				{ start_time=current_time=end_time=0; }
			break;
		case T_VCDCLOSE:
			break;	/* next token will be '#' time related followed by $end */
		case T_END:	/* either closure for dump commands or */
			break;	/* it's spurious                       */
		case T_UNKNOWN_KEY:
			sync_end(NULL);	/* skip over unknown keywords */
			break;
		case T_EOF:
			if((blackout_regions)&&(blackout_regions->bstart>blackout_regions->bend))
				{
				blackout_regions->bend = current_time;
				}
			return;
		default:
			DEBUG(fprintf(stderr,"UNKNOWN TOKEN\n"));
		}
	}
}


/*******************************************************************************/

void add_histent(TimeType time, struct Node *n, char ch, int regadd, char *vector)
{
struct HistEnt *he;
char heval;

if(!vector)
{
if(!n->curr)
	{
	he=histent_calloc();
        he->time=-1;
        he->v.val=1;

	n->curr=he;
	n->head.next=he;

	add_histent(time,n,ch,regadd, vector);
	}
	else
	{
	if(regadd) { time*=(time_scale); }

        if(ch=='0') heval=0; else
        if(ch=='1') heval=3; else
        if((ch=='x')||(ch=='X')) heval=1; else
        heval=2;
	
	if((n->curr->v.val!=heval)||(time==start_time)) /* same region == go skip */ 
        	{
		if(n->curr->time==time)
			{
			DEBUG(printf("Warning: Glitch at time ["TTFormat"] Signal [%p], Value [%c->%c].\n",
				time, n, "0XZ1"[n->curr->v.val], ch));
			n->curr->v.val=heval;		/* we have a glitch! */

			num_glitches++;
			if(!(n->curr->flags&HIST_GLITCH))
				{
				n->curr->flags|=HIST_GLITCH;	/* set the glitch flag */
				num_glitch_regions++;
				}
			}
			else
			{
                	he=histent_calloc();
                	he->time=time;
                	he->v.val=heval;

                	n->curr->next=he;
			n->curr=he;
                	regions+=regadd;
			}
                }
       }
}
else
{
switch(ch)
	{
	case 's': /* string */
	{
	if(!n->curr)
		{
		he=histent_calloc();
		he->flags=(HIST_STRING|HIST_REAL);
	        he->time=-1;
	        he->v.vector=NULL;
	
		n->curr=he;
		n->head.next=he;
	
		add_histent(time,n,ch,regadd, vector);
		}
		else
		{
		if(regadd) { time*=(time_scale); }
	
			if(n->curr->time==time)
				{
				DEBUG(printf("Warning: String Glitch at time ["TTFormat"] Signal [%p].\n",
					time, n));
				if(n->curr->v.vector) free_2(n->curr->v.vector);
				n->curr->v.vector=vector;		/* we have a glitch! */
	
				num_glitches++;
				if(!(n->curr->flags&HIST_GLITCH))
					{
					n->curr->flags|=HIST_GLITCH;	/* set the glitch flag */
					num_glitch_regions++;
					}
				}
				else
				{
	                	he=histent_calloc();
				he->flags=(HIST_STRING|HIST_REAL);
	                	he->time=time;
	                	he->v.vector=vector;
	
	                	n->curr->next=he;
				n->curr=he;
	                	regions+=regadd;
				}
	       }
	break;
	}
	case 'g': /* real number */
	{
	if(!n->curr)
		{
		he=histent_calloc();
		he->flags=HIST_REAL;
	        he->time=-1;
	        he->v.vector=NULL;
	
		n->curr=he;
		n->head.next=he;
	
		add_histent(time,n,ch,regadd, vector);
		}
		else
		{
		if(regadd) { time*=(time_scale); }
	
		if(
		  (n->curr->v.vector&&vector&&(*(double *)n->curr->v.vector!=*(double *)vector))
			||(time==start_time)
			||(!n->curr->v.vector)
			) /* same region == go skip */ 
	        	{
			if(n->curr->time==time)
				{
				DEBUG(printf("Warning: Real number Glitch at time ["TTFormat"] Signal [%p].\n",
					time, n));
				if(n->curr->v.vector) free_2(n->curr->v.vector);
				n->curr->v.vector=vector;		/* we have a glitch! */
	
				num_glitches++;
				if(!(n->curr->flags&HIST_GLITCH))
					{
					n->curr->flags|=HIST_GLITCH;	/* set the glitch flag */
					num_glitch_regions++;
					}
				}
				else
				{
	                	he=histent_calloc();
				he->flags=HIST_REAL;
	                	he->time=time;
	                	he->v.vector=vector;
	
	                	n->curr->next=he;
				n->curr=he;
	                	regions+=regadd;
				}
	                }
			else
			{
			free_2(vector);
			}
	       }
	break;
	}
	default:
	{
	if(!n->curr)
		{
		he=histent_calloc();
	        he->time=-1;
	        he->v.vector=NULL;
	
		n->curr=he;
		n->head.next=he;
	
		add_histent(time,n,ch,regadd, vector);
		}
		else
		{
		if(regadd) { time*=(time_scale); }
	
		if(
		  (n->curr->v.vector&&vector&&(strcmp(n->curr->v.vector,vector)))
			||(time==start_time)
			||(!n->curr->v.vector)
			) /* same region == go skip */ 
	        	{
			if(n->curr->time==time)
				{
				DEBUG(printf("Warning: Glitch at time ["TTFormat"] Signal [%p], Value [%c->%c].\n",
					time, n, "0XZ1"[n->curr->v.val], ch));
				if(n->curr->v.vector) free_2(n->curr->v.vector);
				n->curr->v.vector=vector;		/* we have a glitch! */
	
				num_glitches++;
				if(!(n->curr->flags&HIST_GLITCH))
					{
					n->curr->flags|=HIST_GLITCH;	/* set the glitch flag */
					num_glitch_regions++;
					}
				}
				else
				{
	                	he=histent_calloc();
	                	he->time=time;
	                	he->v.vector=vector;
	
	                	n->curr->next=he;
				n->curr=he;
	                	regions+=regadd;
				}
	                }
			else
			{
			free_2(vector);
			}
	       }
	break;
	}
	}
}

}


static void add_tail_histents(void)
{
int i,j;
struct vcdsymbol *v;

/* dump out any pending events 1st */
struct queuedevent *q;
q=queuedevents;
while(q)
	{	
	v=q->sym;
	if(current_time!=(v->ev->last_event_time+1))
		{
		/* dump degating event */
		DEBUG(fprintf(stderr,"#"TTFormat" %s = '%c' (event)\n",v->ev->last_event_time+1,v->name,'0'));
		add_histent(v->ev->last_event_time+1,v->narray[0],'0',1, NULL);	
		}
	q=q->next;
	}

/* then do 'x' trailers */

v=vcdsymroot;
while(v)
	{
	if(v->vartype==V_REAL)
		{
		double *d;

		d=malloc_2(sizeof(double));
		*d=1.0;
		add_histent(MAX_HISTENT_TIME-1, v->narray[0], 'g', 0, (char *)d);
		}
	else
	if((v->size==1)||(!atomic_vectors))
	for(j=0;j<v->size;j++)
		{
		add_histent(MAX_HISTENT_TIME-1, v->narray[j], 'x', 0, NULL);
		}
	else
		{
		add_histent(MAX_HISTENT_TIME-1, v->narray[0], 'x', 0, (char *)calloc_2(1,sizeof(char)));
		}

	v=v->next;
	}

v=vcdsymroot;
while(v)
	{
	if(v->vartype==V_REAL)
		{
		double *d;

		d=malloc_2(sizeof(double));
		*d=0.0;
		add_histent(MAX_HISTENT_TIME, v->narray[0], 'g', 0, (char *)d);
		}
	else
	if((v->size==1)||(!atomic_vectors))
	for(j=0;j<v->size;j++)
		{
		add_histent(MAX_HISTENT_TIME, v->narray[j], 'z', 0, NULL);
		}
	else
		{
		add_histent(MAX_HISTENT_TIME, v->narray[0], 'z', 0, (char *)calloc_2(1,sizeof(char)));
		}

	v=v->next;
	}
}

/*******************************************************************************/

static void vcd_build_symbols(void)
{
int i,j;
int max_slen=-1;
struct sym_chain *sym_chain=NULL, *sym_curr=NULL;
int duphier=0;
char hashdirty;
struct vcdsymbol *v, *vprime;

v=vcdsymroot;
while(v)
	{
	int msi;
	int delta;

		{
		char *str;
		int slen;
		int substnode;

		msi=v->msi;
		delta=((v->lsi-v->msi)<0)?-1:1;
		substnode=0;

		slen=strlen(v->name);
		str=(slen>max_slen)?(wave_alloca((max_slen=slen)+32)):(str); /* more than enough */
		strcpy(str,v->name);

		if(v->msi>=0)
			{
			strcpy(str+slen,vcd_hier_delimeter);
			slen++;
			}

		if((vprime=bsearch_vcd(v->id, strlen(v->id)))!=v) /* hash mish means dup net */
			{
			if(v->size!=vprime->size)
				{
				fprintf(stderr,"ERROR: Duplicate IDs with differing width: %s %s\n", v->name, vprime->name);
				}
				else
				{
				substnode=1;
				}
			}

		if(((v->size==1)||(!atomic_vectors))&&(v->vartype!=V_REAL))
			{
			struct symbol *s;
	
			for(j=0;j<v->size;j++)
				{
				if(v->msi>=0) 
					{
					if(!vcd_explicit_zero_subscripts)
						sprintf(str+slen,"%d",msi);
						else
						sprintf(str+slen-1,"[%d]",msi);
					}

				hashdirty=0;
				if(symfind(str))
					{
					char *dupfix=(char *)malloc_2(max_slen+32);
					hashdirty=1;
					DEBUG(fprintf(stderr,"Warning: %s is a duplicate net name.\n",str));

					do sprintf(dupfix, "$DUP%d%s%s", duphier++, vcd_hier_delimeter, str);
						while(symfind(dupfix));

					strcpy(str, dupfix);
					free_2(dupfix);
					duphier=0; /* reset for next duplicate resolution */
					}
					/* fallthrough */
					{
					s=symadd(str,hashdirty?hash(str):hashcache);
	
					s->n=v->narray[j];
					if(substnode)
						{
						struct Node *n, *n2;
	
						n=s->n;
						n2=vprime->narray[j];
						/* nname stays same */
						n->head=n2->head;
						n->curr=n2->curr;
						/* harray calculated later */
						n->numhist=n2->numhist;
						}
	
					s->n->nname=s->name;
					s->h=s->n->curr;
					if(!firstnode)
						{
						firstnode=curnode=s;
						}
						else
						{
						curnode->nextinaet=s;
						curnode=s;
						}
	
					numfacs++;
					DEBUG(fprintf(stderr,"Added: %s\n",str));
					}
				msi+=delta;
				}

			if((j==1)&&(v->root))
				{
				s->vec_root=(struct symbol *)v->root;		/* these will get patched over */
				s->vec_chain=(struct symbol *)v->chain;		/* these will get patched over */
				v->sym_chain=s;

				if(!sym_chain)
					{
					sym_curr=(struct sym_chain *)calloc_2(1,sizeof(struct sym_chain));
					sym_chain=sym_curr;
					}
					else
					{
					sym_curr->next=(struct sym_chain *)calloc_2(1,sizeof(struct sym_chain));
					sym_curr=sym_curr->next;
					}
				sym_curr->val=s;
				}
			}
			else	/* atomic vector */
			{
			if(v->vartype!=V_REAL)
				{
				sprintf(str+slen-1,"[%d:%d]",v->msi,v->lsi);
				}
				else
				{
				*(str+slen-1)=0;
				}


			hashdirty=0;
			if(symfind(str))
				{
				char *dupfix=(char *)malloc_2(max_slen+32);
				hashdirty=1;
				DEBUG(fprintf(stderr,"Warning: %s is a duplicate net name.\n",str));

				do sprintf(dupfix, "$DUP%d%s%s", duphier++, vcd_hier_delimeter, str);
					while(symfind(dupfix));

				strcpy(str, dupfix);
				free_2(dupfix);
				duphier=0; /* reset for next duplicate resolution */
				}
				/* fallthrough */
				{
				struct symbol *s;

				s=symadd(str,hashdirty?hash(str):hashcache);	/* cut down on double lookups.. */

				s->n=v->narray[0];
				if(substnode)
					{
					struct Node *n, *n2;

					n=s->n;
					n2=vprime->narray[0];
					/* nname stays same */
					n->head=n2->head;
					n->curr=n2->curr;
					/* harray calculated later */
					n->numhist=n2->numhist;
					n->ext=n2->ext;
					}
					else
					{
					struct ExtNode *en;
					en=(struct ExtNode *)malloc_2(sizeof(struct ExtNode));
					en->msi=v->msi;
					en->lsi=v->lsi;

					s->n->ext=en;
					}

				s->n->nname=s->name;
				s->h=s->n->curr;
				if(!firstnode)
					{
					firstnode=curnode=s;
					}
					else
					{
					curnode->nextinaet=s;
					curnode=s;
					}

				numfacs++;
				DEBUG(fprintf(stderr,"Added: %s\n",str));
				}
			}
		}

	v=v->next;
	}

if(sym_chain)
	{
	sym_curr=sym_chain;	
	while(sym_curr)
		{
		sym_curr->val->vec_root= ((struct vcdsymbol *)sym_curr->val->vec_root)->sym_chain;

		if ((struct vcdsymbol *)sym_curr->val->vec_chain)
			sym_curr->val->vec_chain=((struct vcdsymbol *)sym_curr->val->vec_chain)->sym_chain;

		DEBUG(printf("Link: ('%s') '%s' -> '%s'\n",sym_curr->val->vec_root->name, sym_curr->val->name, sym_curr->val->vec_chain?sym_curr->val->vec_chain->name:"(END)"));

		sym_chain=sym_curr;
		sym_curr=sym_curr->next;
		free_2(sym_chain);
		}
	}
}

/*******************************************************************************/

void vcd_sortfacs(void)
{
int i;

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
                if(ch==hier_delimeter) { *subst=0x01; } /* forces sort at hier boundaries */
                subst++;
                }
        }

/* quicksort(facs,0,numfacs-1); */	/* quicksort deprecated because it degenerates on sorted traces..badly.  very badly. */
heapsort(facs,numfacs);

for(i=0;i<numfacs;i++)
        {
        char *subst, ch;
         
        subst=facs[i]->name;
        while((ch=(*subst)))
                {
                if(ch==0x01) { *subst=hier_delimeter; } /* restore back to normal */
                subst++;
                }
        
#ifdef DEBUG_FACILITIES
        printf("%-4d %s\n",i,facs[i]->name);
#endif
        }

facs_are_sorted=1;

init_tree();
for(i=0;i<numfacs;i++)
{                       
build_tree_from_name(facs[i]->name, i);
}                       
treeprune(treeroot);
}

/*******************************************************************************/

static void vcd_cleanup(void)
{
int i;
struct slist *s, *s2;
struct vcdsymbol *v, *vt;

if(indexed)
	{
	free_2(indexed); indexed=NULL; 
	}

if(sorted)
	{
	free_2(sorted); sorted=NULL; 
	}

v=vcdsymroot;
while(v)
	{
	if(v->name) free_2(v->name);
	if(v->id) free_2(v->id);
	if(v->value) free_2(v->value);
	if(v->ev) free_2(v->ev);
	if(v->narray) free_2(v->narray);
	vt=v;
	v=v->next;
	free_2(vt);
	}
vcdsymroot=vcdsymcurr=NULL;

if(slisthier) { free_2(slisthier); slisthier=NULL; }
s=slistroot;
while(s)
	{
	s2=s->next;
	if(s->str)free_2(s->str);
	free_2(s);
	s=s2;
	}

slistroot=slistcurr=NULL; slisthier_len=0;
queuedevents=NULL; /* deallocated in the symbol stuff */

if(vcd_is_compressed)
	{
	pclose(vcd_handle);
	}
	else
	{
	fclose(vcd_handle);
	}

if(yytext)
	{
	free_2(yytext);
	yytext=NULL;
	}
}

/*******************************************************************************/

TimeType vcd_main(char *fname)
{
int flen;

pv=rootv=NULL;
vcd_hier_delimeter[0]=hier_delimeter;

errno=0;	/* reset in case it's set for some reason */

yytext=(char *)malloc_2(T_MAX_STR+1);

if(!hier_was_explicitly_set) /* set default hierarchy split char */
	{
	hier_delimeter='.';
	}

flen=strlen(fname);
if (((flen>2)&&(!strcmp(fname+flen-3,".gz")))||
   ((flen>3)&&(!strcmp(fname+flen-4,".zip"))))
	{
	char *str;
	int dlen;
	dlen=strlen(WAVE_DECOMPRESSOR);
	str=wave_alloca(strlen(fname)+dlen+1);
	strcpy(str,WAVE_DECOMPRESSOR);
	strcpy(str+dlen,fname);
	vcd_handle=popen(str,"r");
	vcd_is_compressed=~0;
	}
	else
	{
	if(strcmp("-vcd",fname))
		{
		vcd_handle=fopen(fname,"rb");
		}
		else
		{
		vcd_handle=stdin;
		}
	vcd_is_compressed=0;
	}

if(!vcd_handle)
	{
	fprintf(stderr, "Error opening %s .vcd file '%s'.\n",
		vcd_is_compressed?"compressed":"", fname);
	exit(1);
	}

getch_alloc();		/* alloc membuff for vcd getch buffer */

build_slisthier();
vcd_parse();
if((!sorted)&&(!indexed))
	{
	fprintf(stderr, "No symbols in VCD file..is it malformed?  Exiting!\n");
	exit(1);
	}
add_tail_histents();

if(vcd_save_handle) fclose(vcd_save_handle);

printf("["TTFormat"] start time.\n["TTFormat"] end time.\n", start_time*time_scale, end_time*time_scale);
if(num_glitches) printf("Warning: encountered %d glitch%s across %d glitch region%s.\n", 
		num_glitches, (num_glitches!=1)?"es":"",
		num_glitch_regions, (num_glitch_regions!=1)?"s":"");

vcd_build_symbols();
vcd_sortfacs();
vcd_cleanup();

getch_free();		/* free membuff for vcd getch buffer */

min_time=start_time*time_scale;
max_time=end_time*time_scale;

if((min_time==max_time)||(max_time==0))
        {
        fprintf(stderr, "VCD times range is equal to zero.  Exiting.\n");
        exit(1);
        }

is_vcd=~0;

return(max_time);
}

/*******************************************************************************/
