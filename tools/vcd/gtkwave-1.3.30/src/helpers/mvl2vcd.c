/*
 * Copyright (c) 2001 Tony Bybell.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation 
 * the rights to use, copy, modify, merge, publish, distribute, sublicense, 
 * and/or sell copies of the Software, and to permit persons to whom the   
 * Software is furnished to do so, subject to the following conditions: 
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,  
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL   
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING   
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

void output_stripes(void);

#define MVLHDR "MVL2VCD | "


/*
 * some structs we use..
 */
struct fac
{
int array_height;
int msb, lsb, len;
char *name;
unsigned int lastchange;
char bus;
};

struct namehier
{
struct namehier *next;
char *name;
char not_final;
};


/*
 * globals
 */
static void *mm;
static int numfacs;
static struct fac *facs=NULL;
static int first_cycle, last_cycle, total_cycles;
static int maxchange=0, maxindex=0;
static int tableposition=0;
static int trace_start=0;
static FILE *thandle;
static int fsiz=0;
static struct namehier *nhold=NULL;
static off_t f_len;
int mesa=0;
int version;

/****************************************************************************/

/*
 * gtkwave chokes on hierarchies with $ prefixes
 */
static char undollar_buf[8192];
static char *undollar(char *s)
{
char *p=undollar_buf;
strcpy(p,s);

while(*p)
	{
	if(*p=='$') *p='_';
	p++;
	}

return(undollar_buf);
}

/****************************************************************************/

/*
 * reconstruct 8/16/24/32 bits out of the aet's representation
 * of a big-endian integer.  this can be optimized for 
 * big-endian platforms such as PPC but hasn't been.
 */
static unsigned int get_byte(int offset)
{
return(*((unsigned char *)mm+offset));
}

static unsigned int get_16(int offset)
{
unsigned char *nn=(unsigned char *)mm+offset;
unsigned int m1=*((unsigned char *)(nn++));
unsigned int m2=*((unsigned char *)nn);
return((m1<<8)|m2);
}

static unsigned int get_24(int offset)
{
unsigned char *nn=(unsigned char *)mm+offset;
unsigned long m1=*((unsigned char *)(nn++));
unsigned long m2=*((unsigned char *)(nn++));
unsigned long m3=*((unsigned char *)nn);
return((m1<<16)|(m2<<8)|m3);
}

static unsigned int get_32(int offset)
{
unsigned char *nn=(unsigned char *)mm+offset;
unsigned long m1=*((unsigned char *)(nn++));
unsigned long m2=*((unsigned char *)(nn++));
unsigned long m3=*((unsigned char *)(nn++));
unsigned long m4=*((unsigned char *)nn);
return((m1<<24)|(m2<<16)|(m3<<8)|m4);
}

/****************************************************************************/

/*
 * convert 0123 to an mvl character representation
 */
static unsigned char convert_mvl(int value)
{
return("01xz"[value&3]);
}


/* 
 * generate a vcd identifier for a give facindx
 */
static char *vcdid(int value)
{
static char buf[16];
int i;

for(i=0;i<15;i++)
	{
	buf[i]=(char)((value%94)+33); /* for range 33..126 */
	value=value/94;
	if(!value) {buf[i+1]=0; break;}
	}

return(buf);
}


/*
 * navigate up and down the scope hierarchy and
 * emit the appropriate vcd scope primitives
 */
static void diff_hier(struct namehier *nh1, struct namehier *nh2)
{
struct namehier *nhtemp;

if(!nh2)
	{
	while((nh1)&&(nh1->not_final))
		{
		printf("$scope module %s $end\n", undollar(nh1->name));
		nh1=nh1->next;
		}
	return;
	}

for(;;)
	{
	if((nh1->not_final==0)&&(nh2->not_final==0)) /* both are equal */
		{
		break;
		}

	if(nh2->not_final==0)	/* old hier is shorter */
		{
		nhtemp=nh1;
		while((nh1)&&(nh1->not_final))
			{
			printf("$scope module %s $end\n", undollar(nh1->name));
			nh1=nh1->next;
			}
		break;
		}

	if(nh1->not_final==0)	/* new hier is shorter */
		{
		nhtemp=nh2;
		while((nh2)&&(nh2->not_final))
			{
			printf("$upscope $end\n");
			nh2=nh2->next;
			}
		break;
		}

	if(strcmp(nh1->name, nh2->name))
		{
		nhtemp=nh2;				/* prune old hier */
		while((nh2)&&(nh2->not_final))
			{
			printf("$upscope $end\n");
			nh2=nh2->next;
			}

		nhtemp=nh1;				/* add new hier */
		while((nh1)&&(nh1->not_final))
			{
			printf("$scope module %s $end\n", undollar(nh1->name));
			nh1=nh1->next;
			}
		break;
		}

	nh1=nh1->next;
	nh2=nh2->next;
	}
}


/*
 * output vcd var entry for a given name
 */
static void output_varname(int indx)
{
char *name=facs[indx].name;
char *pnt, *pnt2;
char *s;
int len;
struct namehier *nh_head=NULL, *nh_curr=NULL, *nhtemp;

pnt=pnt2=name;

for(;;)
{
while((*pnt2!='.')&&(*pnt2)) pnt2++;
s=(char *)calloc(1,(len=pnt2-pnt)+1);
memcpy(s, pnt, len);
nhtemp=(struct namehier *)calloc(1,sizeof(struct namehier));
nhtemp->name=s;

if(!nh_curr)
	{
	nh_head=nh_curr=nhtemp;
	}
	else
	{
	nh_curr->next=nhtemp;
	nh_curr->not_final=1;
	nh_curr=nhtemp;
	}

if(!*pnt2) break;
pnt=(++pnt2);
}

diff_hier(nh_head, nhold);

/* output vars here */
while(nhold)
	{
	nhtemp=nhold->next;	
	free(nhold->name);
	free(nhold);
	nhold=nhtemp;
	}
nhold=nh_head;

if(facs[indx].len==1)
	{
	printf("$var wire 1 %s %s $end\n", vcdid(indx), undollar(nh_curr->name));
	}
	else
	{
	printf("$var wire %d %s %s[%d:%d] $end\n", facs[indx].len, vcdid(indx), undollar(nh_curr->name), facs[indx].msb, facs[indx].lsb);
	}
}


/*
 * given an offset into the aet, calculate the "time" of that
 * offset in simulation.  this func similar to how gtkwave can
 * find the most recent valuechange that corresponds with
 * an arbitrary timevalue.
 */
static int max_compare_time_tc, max_compare_pos_tc;
static int compar_timechain(const void *s1, const void *s2)
{
int key, obj, delta;
int cpos;
int rv;

key=(int)s1;
obj=get_32(cpos=(int)s2);

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

static int bsearch_timechain(int key)
{
max_compare_time_tc=-1; max_compare_pos_tc=-1; 

bsearch((void *)key, (void *)tableposition, total_cycles, 8, compar_timechain);
if((!max_compare_pos_tc)||(max_compare_time_tc<0)) 
        {
        max_compare_pos_tc=tableposition; /* aix bsearch fix */
        }

return(get_32(max_compare_pos_tc+4));
}


/*
 * in effect, we want to merge n items contained in m 
 * sorted lists.  a heap is a convenient way to get the
 * most significant element at the top of m lists.
 *
 * store all the lastchange entries in a heap such that we
 * can retrieve the *last* aet valuechange that we have not
 * seen yet.  a heap is used because this allows us to
 * trace through the entire aet in O(n*lg m) time where
 * n is the total number of valuechanges and
 * m is the total number of facilities.
 *
 */
static unsigned int *lastchange=NULL;
static unsigned int *lcindex=NULL;

static void heapify(int i)
{
int l, r;
unsigned int largest;
int heap_size=numfacs;
unsigned int t;

for(;;)
	{
	l=2*i+1;
	r=l+1;

	if((l<heap_size)&&(lastchange[l]>lastchange[i]))
		{
		largest=l;
		}
		else
		{
		largest=i;
		}
	if((r<heap_size)&&(lastchange[r]>lastchange[largest])) 
		{
		largest=r;
		}

	if(i!=largest)
		{
		t=lcindex[i]; 
		lcindex[i]=lcindex[largest];
		lcindex[largest]=t;

		t=lastchange[largest]; 
		lastchange[largest]=lastchange[i];
		lastchange[i]=t;

		if(largest<=(numfacs/2-1))
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

#if 0
for(i=start;i<=(numfacs/2-1);i++)	/* this is debugging code only! */
	{
	l=2*i+1;
	r=l+1;

	if((l<heap_size)&&(lastchange[l]>lastchange[i]))
		{
		fprintf(stderr, MVLHDR"Invalid heap at %d, start=%d (L %08x=>%08x)\n", i,start,lastchange[i], lastchange[l]);
		exit(0);
		}
	if((r<heap_size)&&(lastchange[r]>lastchange[i]))
		{
		fprintf(stderr, MVLHDR"Invalid heap at %d, start=%d (R %08x=>%08x)\n", i,start,lastchange[i], lastchange[r]);
		exit(0);
		}
	}
#endif
}


/*
 * build a heap out of the lastchange index entries.  this runs
 * in O(n) time
 */
static void build_heap(void)
{
int i;

fprintf(stderr, MVLHDR"Building symbol heap with %d elements\n", numfacs);

lastchange=(unsigned int *)calloc(numfacs, sizeof(unsigned int));
lcindex=(unsigned int *)calloc(numfacs, sizeof(unsigned int));

for(i=0;i<numfacs;i++)
	{
	lcindex[i]=i;
	lastchange[i]=facs[i].lastchange;
	}

for(i=(numfacs/2-1);i>=0;i--)
	{
	heapify(i);
	}
}


/*
 * walk backward through the aet and generate the tempfile
 * that is used to index into the aet from front to back
 * so we can write out the vcd
 */
static void iter_backwards(void)
{
unsigned int pos;
unsigned int opcode;

thandle=tmpfile();
build_heap();

for(;;)
	{
	if((pos=lastchange[0])!=0)
		{
		opcode=get_byte(pos);

		fwrite((void *)lcindex, sizeof(unsigned int), 1, thandle);
		fwrite((void *)&pos, sizeof(unsigned int), 1, thandle);
		fsiz+=(2*sizeof(unsigned int));

		/* fprintf(stderr, MVLHDR"Pos: %08x, time=%d, sym=%s, opcode=%02x\n",  pos, tmval, facs[lcindex[0]].name,opcode); */
		switch(opcode&0x60)
			{
			case 0x20:
				lastchange[0]=get_16(pos+1);
				break;
			case 0x40:
				lastchange[0]=get_24(pos+1);
				break;
			case 0x60:
				lastchange[0]=get_32(pos+1);
				break;
			default:
				fprintf(stderr, MVLHDR"Unknown opcode %02x found in reverse traversal at %08x (sym=%s), exiting.", 
					opcode, pos, facs[lcindex[0]].name);			
				exit(0);
			}
		heapify(0);				
		}
		else
		{
		fprintf(stderr, MVLHDR"Finished reverse traversal of AET\n");
		fprintf(stderr, MVLHDR"Transient tempfile size is %d bytes\n", fsiz);
		break;
		}
	}
}


/*
 * sync past the very last signal change and find the starting
 * address of the time table.  there is probably a much cleaner 
 * way to do this but i don't know how to "properly" do it.
 */
static void sync_timetable(void)
{
int offs=maxchange;
int l, ary_skip;
int doskip=1;
unsigned int ch;
int offscache;

l=facs[maxindex].len;
if(l==1) 
	{
	l=0; 
	}
	else 
	{
	if(facs[maxindex].bus) l=l*2;	/* mvl representation: 00=0 01=1 10=X 11=H */
	l=(l+7)/8;			/* number of skip bytes */
	}

if(facs[maxindex].bus)
	{
	if(facs[maxindex].array_height>256)
		{
		ary_skip=2;
		}
		else
		{
		ary_skip=1;
		}
	}
	else
	{
	ary_skip=0;
	}

while(doskip)
	{
	ch=get_byte(offs);
	switch(ch)
		{
		case 0x22:
		case 0x32:
			offs++;
			offs+=2;
			offs+=ary_skip;
			offs+=l;
			break;
		case 0x24:		/* set to 1 or explicit */
			offs++;
			offs+=2;
			offs+=l;
			break;
		case 0x20:		/* set to 0 [or super multi-value change] */
		case 0x28:		/* set to x */
		case 0x2c:		/* set to z */
			offs++;
			offs+=2;
			if((ch==0x20)&&(facs[maxindex].len!=1)) offs+=facs[maxindex].len;
			break;

		case 0x42:
		case 0x52:
			offs++;
			offs+=3;
			offs+=ary_skip;
			offs+=l;
			break;
		case 0x44:
			offs++;
			offs+=3;
			offs+=l;
			break;
		case 0x40:
		case 0x48:
		case 0x4c:
			offs++;
			offs+=3;
			if((ch==0x40)&&(facs[maxindex].len!=1)) offs+=facs[maxindex].len;
			break;

		case 0x62:
		case 0x72:
			offs++;
			offs+=4;
			offs+=ary_skip;
			offs+=l;
			break;
		case 0x64:
			offs++;
			offs+=4;
			offs+=l;
			break;
		case 0x60:
		case 0x68:
		case 0x6c:
			offs++;
			offs+=4;
			if((ch==0x60)&&(facs[maxindex].len!=1)) offs+=facs[maxindex].len;
			break;

		case 0xa6:	/* incr time */
		case 0xac:	/* flash 0 [only supported flash value at this time] */
		case 0xad:	/* flash 1 */	
		case 0xae:	/* flash x */
		case 0xaf:	/* flash z */
			offs++;
			break;

		case 0xa5:	/* delta time */
			offs++;
			offs++;
			break;

		case 0xa4:	/* explicit timechange */
			offs++;
			offs+=4;
			break;			

		default:
			doskip=0;
			break;
		}
	}

tableposition=offscache=offs;
for(;;)
	{
	int offsetpos=get_32(offs);
	int timev=get_32(offs+4);

	if(timev!=first_cycle)
		{
		offs++;
		if(offs>(f_len-numfacs*4))
			{
			fprintf(stderr, MVLHDR"Could not sync up to time table, trying time zero.\n");
			break;
			}
		}
		else
		{
		if((offsetpos>=trace_start)&&(offsetpos<tableposition))
			{
			tableposition=offs;
			goto fini;
			}			
		offs++;
		}
	}


tableposition=offs=offscache;
        {
        int offsetpos=get_32(offs);
        int timev=get_32(offs+4);

        if((timev!=0)&&(offsetpos!=0))
                {
                fprintf(stderr, MVLHDR"Could not sync up to time table, exiting.\n");
                exit(0);    
                }
                else
                {
                tableposition=offs;
		if(first_cycle>0) total_cycles++;
                }
        }

fini:
fprintf(stderr,MVLHDR"Time table position: %08x\n", tableposition);
}


/*
 * decompress facility names and build up fac geometry
 */
static int build_facs(void)
{
char buf[2048];
int offs=0x0100+0x10*numfacs;
int i, clone;
char *pnt;
int rv;

if(version==2)	/* uncompressed identifiers */
	{
	for(i=0;i<numfacs;i++)
		{
		pnt=buf;
		while((*(pnt++)=get_byte(offs++)));
		strcpy(facs[i].name=(char *)malloc(pnt-buf), buf);
		}
	}
	else	/* compressed identifiers */
	{
	for(i=0;i<numfacs;i++)
		{
		clone=get_16(offs);  offs+=2;
		pnt=buf+clone;
		while((*(pnt++)=get_byte(offs++)));
		strcpy(facs[i].name=(char *)malloc(pnt-buf), buf);
		}
	}

rv=offs+8; /* start of trace data coming up at this byte..there are 8 zeros..don't know if they have real data in some cases */

offs=0x0104;
for(i=0;i<numfacs;i++)
	{
	facs[i].array_height=get_16(offs);
	facs[i].len=get_16(offs+2);
	facs[i].lsb=facs[i].len-1;
	facs[i].msb=0;
	facs[i].bus=(get_byte(offs+4)&0x80);	/* top bit is set for buses.. */

	offs+=0x10;
	}

return(rv);
}


/*
 * build up the lastchange entries so we can start to walk
 * through the aet..
 */
static int build_facs2(int len)
{
int i;
int offs;
int chg;
int maxchg=0, maxindx=0;

if(get_byte(len-1)!=0xb4)
	{
	fprintf(stderr, MVLHDR"AET truncated, exiting.\n");
	exit(0);
	}

first_cycle=get_32(len-9);
fprintf(stderr, MVLHDR"First cycle: %d\n", first_cycle);
last_cycle=get_32(len-5);
fprintf(stderr, MVLHDR"Last cycle: %d\n", last_cycle);
fprintf(stderr, MVLHDR"Total cycles: %d (vs %d)\n", get_32(len-19), total_cycles=last_cycle-first_cycle+1);

offs=len-27;
for(i=numfacs-1;i>=0;i--)
	{
	chg=get_32(offs);  offs-=4;
	if(chg>maxchg) {maxchg=chg; maxindx=i; }
	facs[i].lastchange=chg;
	/* fprintf(stderr, MVLHDR"Changes: %d %08x\n", i, chg); */
	}

fprintf(stderr, MVLHDR"Maxchange at: %08x for symbol '%s' of len %d\n", maxchg, facs[maxindx].name,  facs[maxindx].len);

maxchange=maxchg;
maxindex=maxindx;
return(0);
}


/*
 * given a facindex and an offset into the aet,
 * emit the VCD data for that 2-tuple.  array
 * emission is not supported but you may look at
 * "output_stripes()" to see how to read them.
 */
static void parse_offset(unsigned int which, unsigned int offs)
{
int v, j, k, nbits, l;
int skip=0;
char *pnt;
char buf[4096];
char first, second;

l=nbits=facs[which].len;

if(l==1) 
	{
	l=0; 
	}
	else 
	{
	if(facs[which].bus) l=l*2;	/* mvl representation: 00=0 01=1 10=X 11=H */
	l=(l+7)/8;
	}


	v=get_byte(offs);
	switch(v&0x7f)			/* it seems A0 aliases to 20, etc */
		{
		case 0x60:
			skip++;
		case 0x40:
			skip++;
		case 0x20:
			skip+=3;

			if((nbits==1)&&(!mesa))
				{
				printf("0%s\n",vcdid(which));
				}
				else
				{
				pnt=buf;
				for(j=0;j<nbits;j++)
					{
					*(pnt++)=convert_mvl(get_byte(offs+skip+j));	/* super literal for many-way mvl, evil! */
					}
				*(pnt++)=0;
				pnt=buf;
				for(;;)
					{
					first=*pnt;
					second=*(pnt+1);
	
					if(first!=second)
						{
						if((first=='0')&&(second=='1')) pnt++;
						break;
						}
					if(first=='1') break;
					pnt++;
					}

				printf("b%s %s\n", pnt, vcdid(which));
				}
			break;

			/* array dumping is not architected in VCD */
		case 0x22:
		case 0x42:
		case 0x62:
			/* 32, 52, 72 seem to be mesa arrays */
		case 0x32:
		case 0x52:
		case 0x72:
			break;

		case 0x28:
		case 0x48:
		case 0x68:
			printf("x%s\n",vcdid(which));
			break;

		case 0x2c:
		case 0x4c:
		case 0x6c:
			printf("z%s\n",vcdid(which));
			break;

		case 0x64:
			skip++;
		case 0x44:
			skip++;
		case 0x24:
			skip+=3;

			if(nbits==1)
				{
				printf("1%s\n",vcdid(which));
				break;
				}

			pnt=buf;
			if(facs[which].bus)
				{
				int bitcnt=0;
				int ch;
				int rsh;

				for(j=0;j<l;j++)
					{
					ch=get_byte(offs+skip+j);
					rsh=6;
					for(k=0;k<4;k++)
						{
						if(bitcnt<nbits)
							{
							*(pnt++)=convert_mvl(ch>>rsh);
							rsh-=2;
							bitcnt++;
							}
							else
							{
							break;
							}
						}
					}
				}
				else
				{
				unsigned int msk;
				int bitcnt=0;
				int ch;
				
				for(j=0;j<l;j++)
					{
					ch=get_byte(offs+skip+j);
					msk=0x80;
					for(k=0;k<8;k++)
						{
						if(bitcnt<nbits)
							{
							*(pnt++)= (ch&msk) ? '1' : '0';
							msk>>=1;
							bitcnt++;
							}
							else
							{
							break;
							}
						}
					}
				}
			*(pnt++)=0;
			pnt=buf;

			for(;;)
				{
				first=*pnt;
				second=*(pnt+1);

				if(first!=second)
					{
					if((first=='0')&&(second=='1')) pnt++;
					break;
					}
				if(first=='1') break;
				pnt++;
				}

			printf("b%s %s\n", pnt, vcdid(which));
			break;
	
		default:
			fprintf(stderr, MVLHDR"Unknown %02x at offset: %08x\n", v, offs);
			exit(0);
		}
}


/*
 * generate the VCD file to stdout
 */
static void output_vcd(void)
{
int i;
char buf[4096];
unsigned int which, pos;
unsigned int tmval, tmvalchk;

fprintf(stderr, MVLHDR"Generating VCD file...\n");


printf("$version\n");
strncpy(buf, (char *)mm+0x14, 8);
buf[8]=0;
strncpy(buf+9, (char *)mm+0x14+8, 8);
buf[17]=0;
printf("\tAET created at %8s on %8s.\n", buf+9, buf);

strncpy(buf, (char *)mm+0x58, 8);
buf[8]=0;
strncpy(buf+9, (char *)mm+0x58+8, 8);
buf[17]=0;
printf("\tModel %s created at %s on %s.\n", (char *)mm+0x58+16, buf+9, buf);

printf("\t%d facilities found.\n", numfacs);
printf("$end\n\n");
printf("$timescale\n\t1ns\n$end\n\n");

for(i=0;i<numfacs;i++)
	{
	output_varname(i);
	}
printf("$enddefinitions $end\n\n");

printf("#%d\n", tmval=first_cycle);

for(i=0;i<numfacs;i++)		/* init all facs to zero.. */
	{
	if(facs[i].len==1)
		{
		printf("0%s\n", vcdid(i));
		}
		else
		{
		printf("b0 %s\n", vcdid(i));
		}
	}

fseek(thandle, -2*(sizeof(unsigned int)), SEEK_CUR);
for(;;)
	{
	fread((void *)&which, sizeof(unsigned int), 1, thandle);
	fread((void *)&pos, sizeof(unsigned int), 1, thandle);
	tmvalchk=bsearch_timechain(pos);
	if(tmval!=tmvalchk)
		{
		printf("#%d\n", tmval=tmvalchk);
		}

	/* printf("%08x %08x %08x\n", fsiz, which, pos); */
	parse_offset(which, pos);

	fsiz-=2*(sizeof(unsigned int));
	if(fsiz<=0) break;
	fseek(thandle, -4*(sizeof(unsigned int)), SEEK_CUR);
	}

printf("#%d\n", last_cycle);

}


/*
 * mainline
 */
int main(int argc, char **argv)
{
int fd;

if(argc!=2)
        {
        fprintf(stderr, "Usage:\n------\n%s filename\n\nVCD is then output to stdout.\n", argv[0]);
        exit(0);
        }

fd=open(argv[1], O_RDONLY);
if(fd<0)
        {
        fprintf(stderr, "Could not open '%s', exiting.\n", argv[1]);
        exit(0);
        }

f_len=lseek(fd, 0, SEEK_END);
mm=mmap(NULL, f_len, PROT_READ, MAP_SHARED, fd, 0);

if(get_byte(0)!=0xd0)
	{
	fprintf(stderr, "Not an MVLSIM format AET, exiting.\n");
	exit(0);
	}

version=get_16(4);
if(version>6) mesa=1;
if((version<6)&&(version!=2))
	{
	fprintf(stderr, "Version %d not supported.\n", version);
	exit(0);
	}

fprintf(stderr, "\n"MVLHDR"MVLSIM AET to VCD Converter (w)2001 BSI\n");
fprintf(stderr, MVLHDR"\n");
fprintf(stderr, MVLHDR"Converting %s AET '%s'...\n", mesa?"MESA":"MVLSIM", argv[1]);
fprintf(stderr, MVLHDR"Len: %d\n", (unsigned int)f_len);

numfacs=get_32(0x00f0);
fprintf(stderr, MVLHDR"Number of facs: %d\n", numfacs);
facs=(struct fac *)calloc(numfacs,sizeof(struct fac));

trace_start=build_facs();
fprintf(stderr, MVLHDR"Trace start: %08x\n",trace_start);

build_facs2(f_len);
sync_timetable();
/* do your stuff here..all useful info has been initialized by now */

/* output_stripes(); */

iter_backwards();
output_vcd();

munmap(mm, f_len); mm=NULL;
fprintf(stderr, MVLHDR"Finished, exiting.\n");
exit(0);
}

/*
 * this is normally used for debugging...or you'd use a similar method to 
 * import traces into an actual viewer.  since it's unused here, it's
 * ifdefed out but it's been kept in the source to illustrate how to
 * read arrays, etc.
 */

#if 0
static void output_stripes(void)
{
int zzz;
int offs;
int v, j, l;
int ary_indx, ary_skip;
int tmval;
int quiet=0;


for(zzz=0;zzz<numfacs;zzz++)
{
offs=facs[zzz].lastchange;
l=facs[zzz].len;
if(!quiet)
	{
	fprintf(stderr, "faclen is: %d for '%s' (%d)\n", l,facs[zzz].name,zzz);
	}

if(l==1) 
	{
	l=0; 
	}
	else 
	{
	if(facs[zzz].bus) l=l*2;	/* mvl representation: 00=0 01=1 10=X 11=H */
	l=(l+7)/8;
	}

while(offs)
	{
	tmval=bsearch_timechain(offs);
	if(!quiet)
		{
		fprintf(stderr, "#%d ",tmval);
		}

	v=get_byte(offs);
	switch(v&0x6f)			/* it seems A0 aliases to 20, etc */
		{
		case 0x40:
			if(!quiet)
				{
				fprintf(stderr, "LC40: %08x ", offs);
				fprintf(stderr, "\n");
				}
			offs=get_24(offs+1);
			break;
	
		case 0x42:
			if(!quiet)
				{
				fprintf(stderr, "LC42: %08x ", offs);
	
				if(facs[zzz].array_height>256)
					{
					ary_indx=get_16(offs+4);
					ary_skip=2;
					}
					else
					{
					ary_indx=get_byte(offs+4);
					ary_skip=1;
					}
				fprintf(stderr, "[%d] ",ary_indx);
			
				if(facs[zzz].bus)
					{
					for(j=0;j<l;j++)
						{
						int ch=get_byte(offs+ary_skip+4+j);
						fprintf(stderr, "%c%c%c%c ", convert_mvl(ch>>6), convert_mvl(ch>>4), convert_mvl(ch>>2), convert_mvl(ch>>0));
						}
					}
					else
					{
					for(j=0;j<l;j++)
						{
						fprintf(stderr, "%02x ", get_byte(offs+ary_skip+4+j));
						}
					}
				fprintf(stderr, "\n");
				}
			offs=get_24(offs+1);
			break;
	
		case 0x44:
			if(!quiet)
				{
				fprintf(stderr, "LC44: %08x ", offs);
	
				if(facs[zzz].bus)
					{
					for(j=0;j<l;j++)
						{
						int ch=get_byte(offs+4+j);
						fprintf(stderr, "%c%c%c%c ", convert_mvl(ch>>6), convert_mvl(ch>>4), convert_mvl(ch>>2), convert_mvl(ch>>0));
						}
					}
					else
					{
					for(j=0;j<l;j++)
						{
						fprintf(stderr, "%02x ", get_byte(offs+4+j));
						}
					}
				fprintf(stderr, "\n");
				}
			offs=get_24(offs+1);
			break;
	
		case 0x48:
			if(!quiet)
				{
				fprintf(stderr, "LC40: %08x ", offs);
				fprintf(stderr, "\n");
				}
			offs=get_24(offs+1);
			break;
	
		case 0x4C:
			if(!quiet)
				{
				fprintf(stderr, "LC40: %08x ", offs);
				fprintf(stderr, "\n");
				}
			offs=get_24(offs+1);
			break;
	
		case 0x20:
			if(!quiet)
				{
				fprintf(stderr, "LC20: %08x ", offs);
				fprintf(stderr, "\n");
				}
			offs=get_16(offs+1);
			break;
	
		case 0x22:
			if(!quiet)
				{
				fprintf(stderr, "LC22: %08x ", offs);
	
				if(facs[zzz].array_height>256)
					{
					ary_indx=get_16(offs+3);
					ary_skip=2;
					}
					else
					{
					ary_indx=get_byte(offs+3);
					ary_skip=1;
					}
				fprintf(stderr, "[%d] ",ary_indx);
	
				if(facs[zzz].bus)
					{
					for(j=0;j<l;j++)
						{
						int ch=get_byte(offs+ary_skip+3+j);
						fprintf(stderr, "%c%c%c%c ", convert_mvl(ch>>6), convert_mvl(ch>>4), convert_mvl(ch>>2), convert_mvl(ch>>0));
						}
					}
					else
					{
					for(j=0;j<l;j++)
						{
						fprintf(stderr, "%02x ", get_byte(offs+ary_skip+3+j));
						}
					}
				fprintf(stderr, "\n");
				}
			offs=get_16(offs+1);
			break;

		case 0x24:
			if(!quiet)
				{
				fprintf(stderr, "LC24: %08x ", offs);
				if(facs[zzz].bus)
					{
					for(j=0;j<l;j++)
						{
						int ch=get_byte(offs+3+j);
						fprintf(stderr, "%c%c%c%c ", convert_mvl(ch>>6), convert_mvl(ch>>4), convert_mvl(ch>>2), convert_mvl(ch>>0));
						}
					}
					else
					{
					for(j=0;j<l;j++)
						{
						fprintf(stderr, "%02x ", get_byte(offs+3+j));
						}
					}
				fprintf(stderr, "\n");
				}
			offs=get_16(offs+1);
			break;

		case 0x28:
			if(!quiet)
				{
				fprintf(stderr, "LC20: %08x ", offs);
				fprintf(stderr, "\n");
				}
			offs=get_16(offs+1);
			break;
	
		case 0x2C:
			if(!quiet)
				{
				fprintf(stderr, "LC20: %08x ", offs);
				fprintf(stderr, "\n");
				}
			offs=get_16(offs+1);
			break;
	

		case 0x60:
			if(!quiet)
				{
				fprintf(stderr, "LC60: %08x ", offs);
				fprintf(stderr, "\n");
				}
			offs=get_32(offs+1);
			break;

		case 0x62:
			if(!quiet)
				{
				fprintf(stderr, "LC62: %08x ", offs);
	
				if(facs[zzz].array_height>256)
					{
					ary_indx=get_16(offs+5);
					ary_skip=2;
					}
					else
					{
					ary_indx=get_byte(offs+5);
					ary_skip=1;
					}
				fprintf(stderr, "[%d] ",ary_indx);
		
				if(facs[zzz].bus)
					{
					for(j=0;j<l;j++)
						{
						int ch=get_byte(offs+ary_skip+5+j);
						fprintf(stderr, "%c%c%c%c ", convert_mvl(ch>>6), convert_mvl(ch>>4), convert_mvl(ch>>2), convert_mvl(ch>>0));
						}
					}
					else
					{
					for(j=0;j<l;j++)
						{
						fprintf(stderr, "%02x ", get_byte(offs+ary_skip+5+j));
						}
					}
				fprintf(stderr, "\n");
				}
			offs=get_32(offs+1);
			break;
	
		case 0x64:
			if(!quiet)
				{
				fprintf(stderr, "LC64: %08x ", offs);
				if(facs[zzz].bus)
					{
					for(j=0;j<l;j++)
						{
						int ch=get_byte(offs+5+j);
						fprintf(stderr, "%c%c%c%c ", convert_mvl(ch>>6), convert_mvl(ch>>4), convert_mvl(ch>>2), convert_mvl(ch>>0));
						}
					}
					else
					{
					for(j=0;j<l;j++)
						{
						fprintf(stderr, "%02x ", get_byte(offs+5+j));
						}
					}
				fprintf(stderr, "\n");
			}
			offs=get_32(offs+1);
			break;
	
		case 0x68:
			if(!quiet)
				{
				fprintf(stderr, "LC68: %08x ", offs);
				fprintf(stderr, "\n");
				}
			offs=get_32(offs+1);
			break;

		case 0x6C:
			if(!quiet)
				{
				fprintf(stderr, "LC68: %08x ", offs);
				fprintf(stderr, "\n");
				}
			offs=get_32(offs+1);
			break;
		
		default:
			fprintf(stderr, "Unknown %02x at offset: %08x\n", v, offs);
			exit(0);
		}
	}
}
}
#endif
