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
#include "lxt_write.h"

void output_stripes(void);

#define MVLHDR "MVL2LXT | "


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

struct lt_symbol *ltsym;
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
static off_t f_len;
int mesa=0;
int version;

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
 * output lxt var entry for a given name
 */
static void output_varname(struct lt_trace *lt, int indx)
{
char *name=facs[indx].name;
facs[indx].ltsym=lt_symbol_add(lt, name, facs[indx].array_height-1, facs[indx].msb, facs[indx].lsb, LT_SYM_F_BITS);
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
 * emit the LXT data for that 2-tuple.  array
 * emission is not implemented, but it's trivial
 */
static void parse_offset(struct lt_trace *lt, unsigned int which, unsigned int offs)
{
int v, j, k, nbits, l;
int skip=0;
char *pnt;
char buf[65537];

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
				lt_emit_value_bit_string(lt, facs[which].ltsym, 0, "0");
				/* printf("0%s\n",vcdid(which)); */
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
				
				lt_emit_value_bit_string(lt, facs[which].ltsym, 0, pnt);
				/* printf("b%s %s\n", pnt, vcdid(which)); */
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
			lt_emit_value_bit_string(lt, facs[which].ltsym, 0, "x");
			/* printf("x%s\n",vcdid(which)); */
			break;

		case 0x2c:
		case 0x4c:
		case 0x6c:
			lt_emit_value_bit_string(lt, facs[which].ltsym, 0, "z");
			/* printf("z%s\n",vcdid(which)); */
			break;

		case 0x64:
			skip++;
		case 0x44:
			skip++;
		case 0x24:
			skip+=3;

			if(nbits==1)
				{
				lt_emit_value_bit_string(lt, facs[which].ltsym, 0, "1");
				/* printf("1%s\n",vcdid(which)); */
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
			lt_emit_value_bit_string(lt, facs[which].ltsym, 0, pnt);
			/* printf("b%s %s\n", pnt, vcdid(which)); */
			break;
	
		default:
			fprintf(stderr, MVLHDR"Unknown %02x at offset: %08x\n", v, offs);
			exit(0);
		}
}


/*
 * generate the LXT file 
 */
static void output_lxt(char *fnam, int clock_compress, int change_compress)
{
int i;
unsigned int which, pos;
unsigned int tmval=~0, tmvalchk;
struct lt_trace *lt;

fprintf(stderr, MVLHDR"Generating LXT file '%s'...\n", fnam);
lt = lt_init(fnam);
if(!lt)
	{
	fprintf(stderr, "Could not create LXT file '%s'.\n", fnam);
	perror("Why");
	exit(255);
	}

for(i=0;i<numfacs;i++)
	{
	output_varname(lt, i);
	}

if(clock_compress) lt_set_clock_compress(lt);
if(change_compress) lt_set_chg_compress(lt);
lt_set_timescale(lt, -9);
lt_set_time(lt, first_cycle);
lt_set_initial_value(lt, '0');

fseek(thandle, -2*(sizeof(unsigned int)), SEEK_CUR);
for(;;)
	{
	fread((void *)&which, sizeof(unsigned int), 1, thandle);
	fread((void *)&pos, sizeof(unsigned int), 1, thandle);
	tmvalchk=bsearch_timechain(pos);
	if(tmval!=tmvalchk)
		{
		lt_set_time(lt, tmval=tmvalchk);
		}

	parse_offset(lt, which, pos);

	fsiz-=2*(sizeof(unsigned int));
	if(fsiz<=0) break;
	fseek(thandle, -4*(sizeof(unsigned int)), SEEK_CUR);
	}

lt_set_time(lt, last_cycle);
lt_close(lt);
}


/*
 * mainline
 */
int main(int argc, char **argv)
{
int fd;
int i, doclock=0, dochg=0;

if(argc<3)
        {
        fprintf(stderr, "Usage:\n------\n%s filename.aet filename.lxt [-clockpack][-chgpack]\n", argv[0]);
        exit(0);
        }

for(i=3;i<argc;i++)
        {
        if(!strcmp(argv[i], "-clockpack")) doclock = 1;
        else if(!strcmp(argv[i], "-chgpack")) dochg = 1;
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

fprintf(stderr, "\n"MVLHDR"MVLSIM AET to LXT Converter (w)2001 BSI\n");
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

iter_backwards();
output_lxt(argv[2], doclock, dochg);

munmap(mm, f_len); mm=NULL;
fprintf(stderr, MVLHDR"Finished, exiting.\n");
exit(0);
}

