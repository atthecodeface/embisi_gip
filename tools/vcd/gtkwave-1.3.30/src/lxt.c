/* 
 * Copyright (c) Tony Bybell 2001-2002.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include <stdio.h>
#include <zlib.h>
#include <bzlib.h>

#include <unistd.h>
#include <sys/mman.h>

#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "aet.h"
#include "vcd.h"
#include "lxt.h"
#include "bsearch.h"
#include "debug.h"

/****************************************************************************/

/*
 * functions which emit various big endian
 * data to a file
 */
static int fpos;

static int lt_emit_u8(FILE *handle, int value)
{
unsigned char buf[1];
int nmemb;
             
buf[0] = value & 0xff;
nmemb=fwrite(buf, sizeof(char), 1, handle);
fpos+=nmemb;
return(nmemb);
}   


static int lt_emit_u16(FILE *handle, int value)
{
unsigned char buf[2];
int nmemb;
    
buf[0] = (value>>8) & 0xff;   
buf[1] = value & 0xff;   
nmemb = fwrite(buf, sizeof(char), 2, handle);
fpos+=nmemb;
return(nmemb);
}
    
        
static int lt_emit_u24(FILE *handle, int value)
{
unsigned char buf[3];
int nmemb;

buf[0] = (value>>16) & 0xff;
buf[1] = (value>>8) & 0xff;
buf[2] = value & 0xff;
nmemb=fwrite(buf, sizeof(char), 3, handle);
fpos+=nmemb;
return(nmemb);
}            


static int lt_emit_u32(FILE *handle, int value)
{
unsigned char buf[4];
int nmemb;

buf[0] = (value>>24) & 0xff;
buf[1] = (value>>16) & 0xff;
buf[2] = (value>>8) & 0xff;
buf[3] = value & 0xff;
nmemb=fwrite(buf, sizeof(char), 4, handle);
fpos+=nmemb; 
return(nmemb);
}

/****************************************************************************/

#define LXTHDR "LXTLOAD | "


/*
 * globals
 */
char is_lxt = 0;
char lxt_clock_compress_to_z = 0;

static void *mm, *mmcache;
static int version;

static struct fac *mvlfacs=NULL;
static TimeType first_cycle, last_cycle, total_cycles;
static TimeType time_scale=1;   /* multiplier is 1, 10, 100 */
static int maxchange=0, maxindex=0;
static int trace_start=0;
static off_t f_len;

static int *positional_information=NULL;
TimeType *time_information=NULL;

static unsigned int change_field_offset=0;
static unsigned int facname_offset=0;
static unsigned int facgeometry_offset=0;
static unsigned int time_table_offset=0;
static unsigned int time_table_offset64=0;
static unsigned int sync_table_offset=0;
static unsigned int initial_value_offset=0;
static unsigned int timescale_offset=0;
static unsigned int double_test_offset=0;
static unsigned int zdictionary_offset=0;

static unsigned int zfacname_predec_size=0;
static unsigned int zfacname_size=0;
static unsigned int zfacgeometry_size=0;
static unsigned int zsync_table_size=0;
static unsigned int ztime_table_size=0;
static unsigned int zchg_predec_size=0;
static unsigned int zchg_size=0;
static unsigned int zdictionary_predec_size=0;

static unsigned char initial_value = 1;

static unsigned int dict_num_entries;
static unsigned int dict_string_mem_required;
static unsigned int dict_16_offset;
static unsigned int dict_24_offset;
static unsigned int dict_32_offset;
static unsigned int dict_width;
static char **dict_string_mem_array=NULL;

static unsigned int exclude_offset=0;

static char *lt_buf=NULL;	/* max memory ever needed for parse_offset() */
static int lt_len = 0;

static int fd;			/* XXX : win32 requires decompression tempfiles since */
				/* its mmap() variant doesn't use file descriptors    */

/****************************************************************************/

#ifdef _WAVE_BE32

/*
 * reconstruct 8/16/24/32 bits out of the lxt's representation
 * of a big-endian integer.  this is for 32-bit PPC so no byte
 * swizzling needs to be done at all.  for 24-bit ints, we have no danger of
 * running off the end of memory provided we do the "-1" trick
 * since we'll never read a 24-bit int at the very start of a file which
 * means that we'll have a 32-bit word that we can read.
 */
   
#define get_byte(offset)        ((unsigned int)(*((unsigned char *)mm+(offset))))
#define get_16(offset)          ((unsigned int)(*((unsigned short *)(((unsigned char *)mm)+(offset)))))
#define get_32(offset)          (*(unsigned int *)(((unsigned char *)mm)+(offset)))
#define get_24(offset)		((get_32((offset)-1)<<8)>>8)
#define get_64(offset)          ((((UTimeType)get_32(offset))<<32)|((UTimeType)get_32((offset)+4)))
 
#else

/*
 * reconstruct 8/16/24/32 bits out of the lxt's representation
 * of a big-endian integer.  x86 specific version...
 */

#ifdef __i386__

#define get_byte(offset)        ((unsigned int)(*((unsigned char *)mm+offset)))

inline static unsigned int get_16(int offset)
{
unsigned short x = *((unsigned short *)((unsigned char *)mm+offset));

  __asm("xchgb %b0,%h0" :
        "=q" (x)        :
        "0" (x));

    return (unsigned int) x;
}

inline static unsigned int get_32(int offset)	/* note that bswap is really 486+ */
{
unsigned int x = *((unsigned int *)((unsigned char *)mm+offset));

 __asm("bswap   %0":
      "=r" (x) :
      "0" (x));

  return x;
}

#define get_24(offset)		((get_32((offset)-1)<<8)>>8)
#define get_64(offset)          ((((UTimeType)get_32(offset))<<32)|((UTimeType)get_32((offset)+4)))

#else

/*
 * reconstruct 8/16/24/32 bits out of the lxt's representation
 * of a big-endian integer.  this should work on all architectures.
 */
#define get_byte(offset) 	((unsigned int)(*((unsigned char *)mm+offset)))

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
unsigned int m1=*((unsigned char *)(nn++));
unsigned int m2=*((unsigned char *)(nn++));
unsigned int m3=*((unsigned char *)nn);
return((m1<<16)|(m2<<8)|m3);
}

static unsigned int get_32(int offset)
{
unsigned char *nn=(unsigned char *)mm+offset;
unsigned int m1=*((unsigned char *)(nn++));
unsigned int m2=*((unsigned char *)(nn++));
unsigned int m3=*((unsigned char *)(nn++));
unsigned int m4=*((unsigned char *)nn);
return((m1<<24)|(m2<<16)|(m3<<8)|m4);
}

static UTimeType get_64(int offset)
{
return(
(((UTimeType)get_32(offset))<<32)
|((UTimeType)get_32(offset+4))
);
}

#endif
#endif

/****************************************************************************/

static char double_mask[8]={0,0,0,0,0,0,0,0};
static char double_is_native=0;

static void create_double_endian_mask(int offset)
{
static double p = 3.14159;
double d;
int i, j;

d= *((double *)((unsigned char *)mm+offset));
if(p==d) 
	{
	double_is_native=1;
	}
	else
	{
	char *remote, *here;

	remote = (char *)&d;
	here = (char *)&p;
	for(i=0;i<8;i++)
		{
		for(j=0;j<8;j++)
			{
			if(here[i]==remote[j])
				{
				double_mask[i]=j;
				break;
				}
			}
		}	
	}
}

static char *swab_double_via_mask(int offset)
{
char swapbuf[8];
char *pnt = malloc_2(8*sizeof(char));
int i;

memcpy(swapbuf, ((unsigned char *)mm+offset), 8);
for(i=0;i<8;i++)
	{
	pnt[i]=swapbuf[double_mask[i]];
	}

return(pnt);
}

/****************************************************************************/

/*
 * convert 0123 to an mvl character representation
 */
static unsigned char convert_mvl(int value)
{
return("01zx1xx0xxxxxxxx"[value&15]);
}


/*
 * given an offset into the aet, calculate the "time" of that
 * offset in simulation.  this func similar to how gtkwave can
 * find the most recent valuechange that corresponds with
 * an arbitrary timevalue.
 */
static int max_compare_time_tc, max_compare_pos_tc;
static int compar_mvl_timechain(const void *s1, const void *s2)
{
int key, obj, delta;
int rv;

key=*((int *)s1);
obj=*((int *)s2);

if((obj<=key)&&(obj>max_compare_time_tc))
        {
        max_compare_time_tc=obj;
        max_compare_pos_tc=(int *)s2 - positional_information;
        }

delta=key-obj;
if(delta<0) rv=-1;
else if(delta>0) rv=1;
else rv=0;

return(rv);
}

static TimeType bsearch_mvl_timechain(int key)
{
max_compare_time_tc=-1; max_compare_pos_tc=-1; 

bsearch((void *)&key, (void *)positional_information, total_cycles, sizeof(int), compar_mvl_timechain);
if((max_compare_pos_tc<=0)||(max_compare_time_tc<0)) 
        {
        max_compare_pos_tc=0; /* aix bsearch fix */
        }

return(time_information[max_compare_pos_tc]);
}


/*
 * build up decompression dictionary for MVL2 facs >16 bits ...
 */
static void build_dict(void)
{
FILE *tmp;
gzFile zhandle;
int offs = zdictionary_offset+24;
int total_mem, rc, i;
char *decmem=NULL;
char *pnt;
unsigned char *t;

dict_num_entries = get_32(zdictionary_offset+0);
dict_string_mem_required = get_32(zdictionary_offset+4);
dict_16_offset = get_32(zdictionary_offset+8);
dict_24_offset = get_32(zdictionary_offset+12);
dict_32_offset = get_32(zdictionary_offset+16);
dict_width = get_32(zdictionary_offset+20);

DEBUG(fprintf(stderr, LXTHDR"zdictionary_offset = %08x\n", zdictionary_offset));
DEBUG(fprintf(stderr, LXTHDR"zdictionary_predec_size = %08x\n\n", zdictionary_predec_size));
DEBUG(fprintf(stderr, LXTHDR"dict_num_entries = %d\n", dict_num_entries));
DEBUG(fprintf(stderr, LXTHDR"dict_string_mem_required = %d\n", dict_string_mem_required));
DEBUG(fprintf(stderr, LXTHDR"dict_16_offset = %d\n", dict_16_offset));
DEBUG(fprintf(stderr, LXTHDR"dict_24_offset = %d\n", dict_24_offset));
DEBUG(fprintf(stderr, LXTHDR"dict_32_offset = %d\n", dict_32_offset));

printf(LXTHDR"Dictionary compressed MVL2 change records detected...\n");

if(offs!=lseek(fd, offs, SEEK_SET)) { fprintf(stderr, LXTHDR"lseek error at offset %08x\n", offs); exit(255); }
zhandle = gzdopen(dup(fd), "rb");
decmem = malloc_2(total_mem = dict_string_mem_required);

rc=gzread(zhandle, decmem, total_mem);
DEBUG(printf(LXTHDR"section offs for name decompression = %08x of len %d\n", offs, dict_string_mem_required));
DEBUG(printf(LXTHDR"Decompressed size is %d bytes (vs %d)\n", rc, total_mem));	
if(rc!=total_mem) { fprintf(stderr, LXTHDR"decompression size disparity  %d bytes (vs %d)\n", rc, total_mem); exit(255); }

dict_string_mem_array = (char **)calloc(dict_num_entries, sizeof(char *));
pnt = decmem;
for(i=0;i<dict_num_entries;i++)
	{
	dict_string_mem_array[i]=pnt;
	pnt+=(strlen(pnt)+1);
	DEBUG(printf(LXTHDR"Dict %d: '1%s'\n", i, dict_string_mem_array[i]));
	}

gzclose(zhandle);

printf(LXTHDR"...expanded %d entries from %08x into %08x bytes.\n", dict_num_entries, zdictionary_predec_size, dict_string_mem_required);
}


/*
 * decompress facility names and build up fac geometry
 */
static void build_facs(char *fname)
{
char *buf, *bufprev, *bufcurr;
int offs=facname_offset+8;
int i, j, clone;
char *pnt;
int total_mem = get_32(facname_offset+4);
gzFile zhandle;
char *decmem=NULL;

buf=malloc_2(total_mem);
pnt=bufprev=buf;

if(zfacname_size)
	{
	int rc;

	if(offs!=lseek(fd, offs, SEEK_SET)) { fprintf(stderr, LXTHDR"lseek error at offset %08x\n", offs); exit(255); }
	zhandle = gzdopen(dup(fd), "rb");

	mmcache = mm;
	decmem = malloc_2(total_mem = zfacname_predec_size); mm = decmem;

	rc=gzread(zhandle, decmem, total_mem);
	DEBUG(printf(LXTHDR"section offs for name decompression = %08x of len %d\n", offs, zfacname_size));
	DEBUG(printf(LXTHDR"Decompressed size is %d bytes (vs %d)\n", rc, total_mem));	
	if(rc!=total_mem) { fprintf(stderr, LXTHDR"decompression size disparity  %d bytes (vs %d)\n", rc, total_mem); exit(255); }

	offs=0;	/* we're in our new memory region now.. */
	}

printf(LXTHDR"Building %d facilities.\n", numfacs);
for(i=0;i<numfacs;i++)
	{
        clone=get_16(offs);  offs+=2;
	bufcurr=pnt;
	for(j=0;j<clone;j++)
		{
		*(pnt++) = *(bufprev++);
		}
        while((*(pnt++)=get_byte(offs++)));
        mvlfacs[i].name=bufcurr;
	DEBUG(printf(LXTHDR"Encountered facility %d: '%s'\n", i, bufcurr));
	bufprev=bufcurr;
        }

if(zfacname_size)
	{
	mm = mmcache;
	free_2(decmem); decmem = NULL;
	gzclose(zhandle);
	}

if(!facgeometry_offset)
	{
	fprintf(stderr, "LXT '%s' is missing a facility geometry section, exiting.\n", fname);
	exit(255);
	}

offs=facgeometry_offset;

if(zfacgeometry_size)
	{
	int rc;

	if(offs!=lseek(fd, offs, SEEK_SET)) { fprintf(stderr, LXTHDR"lseek error at offset %08x\n", offs); exit(255); }
	zhandle = gzdopen(dup(fd), "rb");

	mmcache = mm;
	total_mem = numfacs * 16;
	decmem = malloc_2(total_mem); mm = decmem;

	rc=gzread(zhandle, decmem, total_mem);
	DEBUG(printf(LXTHDR"section offs for facgeometry decompression = %08x of len %d\n", offs, zfacgeometry_size));
	DEBUG(printf(LXTHDR"Decompressed size is %d bytes (vs %d)\n", rc, total_mem));	
	if(rc!=total_mem) { fprintf(stderr, LXTHDR"decompression size disparity  %d bytes (vs %d)\n", rc, total_mem); exit(255); }

	offs=0;	/* we're in our new memory region now.. */
	}

for(i=0;i<numfacs;i++)
	{
	mvlfacs[i].array_height=get_32(offs);
	mvlfacs[i].msb=get_32(offs+4);
	mvlfacs[i].lsb=get_32(offs+8);
	mvlfacs[i].flags=get_32(offs+12);
	mvlfacs[i].len=(mvlfacs[i].lsb>mvlfacs[i].msb)?(mvlfacs[i].lsb-mvlfacs[i].msb+1):(mvlfacs[i].msb-mvlfacs[i].lsb+1);

	if(mvlfacs[i].len>lt_len) lt_len = mvlfacs[i].len;
	DEBUG(printf(LXTHDR"%s[%d:%d]\n", mvlfacs[i].name, mvlfacs[i].msb, mvlfacs[i].lsb));

	offs+=0x10;
	}

lt_buf = malloc_2(lt_len ? lt_len : 1);

if(zfacgeometry_size)
	{
	mm = mmcache;
	free_2(decmem); decmem = NULL;
	gzclose(zhandle);
	}
}


/*
 * build up the lastchange entries so we can start to walk
 * through the aet..
 */
static void build_facs2(char *fname)
{
int i;
int offs;
int chg;
int maxchg=0, maxindx=0;
int last_position;
TimeType last_time;
char *decmem;
int total_mem;
gzFile zhandle;

if((!time_table_offset)&&(!time_table_offset64))
	{
	fprintf(stderr, "LXT '%s' is missing a time table section, exiting.\n", fname);
	exit(255);
	}

if((time_table_offset)&&(time_table_offset64))
	{
	fprintf(stderr, "LXT '%s' has both 32 and 64-bit time table sections, exiting.\n", fname);
	exit(255);
	}

if(time_table_offset)
	{
	offs = time_table_offset;

	DEBUG(printf(LXTHDR"Time table position: %08x\n", time_table_offset + 12));
	total_cycles=get_32(offs+0);
	DEBUG(printf(LXTHDR"Total cycles: %d\n", total_cycles));

	if(ztime_table_size)
		{
		int rc;

		if((offs+4)!=lseek(fd, offs+4, SEEK_SET)) { fprintf(stderr, LXTHDR"lseek error at offset %08x\n", offs); exit(255); }
		zhandle = gzdopen(dup(fd), "rb");

		mmcache = mm;
		total_mem = 4 + 4 + (total_cycles * 4) + (total_cycles * 4);
		decmem = malloc_2(total_mem); mm = decmem;

		rc=gzread(zhandle, decmem, total_mem);
		DEBUG(printf(LXTHDR"section offs for timetable decompression = %08x of len %d\n", offs, ztime_table_size));
		DEBUG(printf(LXTHDR"Decompressed size is %d bytes (vs %d)\n", rc, total_mem));	
		if(rc!=total_mem) { fprintf(stderr, LXTHDR"decompression size disparity  %d bytes (vs %d)\n", rc, total_mem); exit(255); }

		offs=0;	/* we're in our new memory region now.. */
		}
		else
		{
		offs+=4; /* skip past count to make consistent view between compressed/uncompressed */
		}

	first_cycle=get_32(offs);
	DEBUG(printf(LXTHDR"First cycle: %d\n", first_cycle));
	last_cycle=get_32(offs+4);
	DEBUG(printf(LXTHDR"Last cycle: %d\n", last_cycle));
	DEBUG(printf(LXTHDR"Total cycles (actual): %d\n", last_cycle-first_cycle+1));
	
	/* rebuild time table from its deltas... */

	positional_information = (int *)malloc_2(total_cycles * sizeof(int));
	last_position=0;
	offs+=8;
	for(i=0;i<total_cycles;i++)
		{
		last_position = positional_information[i] = get_32(offs) + last_position;
		offs+=4;
		}
	time_information =       (TimeType *)malloc_2(total_cycles * sizeof(TimeType));
	last_time=LLDescriptor(0);
	for(i=0;i<total_cycles;i++)
		{
		last_time = time_information[i] = ((TimeType)get_32(offs)) + last_time;
		time_information[i] *= (time_scale);
		offs+=4;
		}

	if(ztime_table_size)
		{
		mm = mmcache;
		free_2(decmem); decmem = NULL;
		gzclose(zhandle);
		}
	}
	else	/* 64-bit read */
	{
	offs = time_table_offset64;

	DEBUG(printf(LXTHDR"Time table position: %08x\n", time_table_offset64 + 20));

	total_cycles=(TimeType)((unsigned int)get_32(offs+0));
	DEBUG(printf(LXTHDR"Total cycles: %d\n", total_cycles));

	if(ztime_table_size)
		{
		int rc;

		if((offs+4)!=lseek(fd, offs+4, SEEK_SET)) { fprintf(stderr, LXTHDR"lseek error at offset %08x\n", offs); exit(255); }
		zhandle = gzdopen(dup(fd), "rb");

		mmcache = mm;
		total_mem = 8 + 8 + (total_cycles * 4) + (total_cycles * 8);
		decmem = malloc_2(total_mem); mm = decmem;

		rc=gzread(zhandle, decmem, total_mem);
		DEBUG(printf(LXTHDR"section offs for timetable decompression = %08x of len %d\n", offs, ztime_table_size));
		DEBUG(printf(LXTHDR"Decompressed size is %d bytes (vs %d)\n", rc, total_mem));	
		if(rc!=total_mem) { fprintf(stderr, LXTHDR"decompression size disparity  %d bytes (vs %d)\n", rc, total_mem); exit(255); }
	
		offs=0;	/* we're in our new memory region now.. */
		}
		else
		{
		offs+=4; /* skip past count to make consistent view between compressed/uncompressed */
		}

	first_cycle=get_64(offs);
	DEBUG(printf(LXTHDR"First cycle: %d\n", first_cycle));
	last_cycle=get_64(offs+8);
	DEBUG(printf(LXTHDR"Last cycle: %d\n", last_cycle));
	DEBUG(printf(LXTHDR"Total cycles (actual): %Ld\n", last_cycle-first_cycle+1));
	
	/* rebuild time table from its deltas... */

	positional_information = (int *)malloc_2(total_cycles * sizeof(int));
	last_position=0;
	offs+=16;
	for(i=0;i<total_cycles;i++)
		{
		last_position = positional_information[i] = get_32(offs) + last_position;
		offs+=4;
		}
	time_information =       (TimeType *)malloc_2(total_cycles * sizeof(TimeType));
	last_time=LLDescriptor(0);
	for(i=0;i<total_cycles;i++)
		{
		last_time = time_information[i] = ((TimeType)get_64(offs)) + last_time;
		time_information[i] *= (time_scale);
		offs+=8;
		}

	if(ztime_table_size)
		{
		mm = mmcache;
		free_2(decmem); decmem = NULL;
		gzclose(zhandle);
		}
	}

if(sync_table_offset)
	{
	offs = sync_table_offset;

	if(zsync_table_size)
		{
		int rc;

		if(offs!=lseek(fd, offs, SEEK_SET)) { fprintf(stderr, LXTHDR"lseek error at offset %08x\n", offs); exit(255); }
		zhandle = gzdopen(dup(fd), "rb");

		mmcache = mm;
		decmem = malloc_2(total_mem = numfacs * 4); mm = decmem;

		rc=gzread(zhandle, decmem, total_mem);
		DEBUG(printf(LXTHDR"section offs for synctable decompression = %08x of len %d\n", offs, zsync_table_size));
		DEBUG(printf(LXTHDR"Decompressed size is %d bytes (vs %d)\n", rc, total_mem));	
		if(rc!=total_mem) { fprintf(stderr, LXTHDR"decompression size disparity  %d bytes (vs %d)\n", rc, total_mem); exit(255); }
	
		offs=0;	/* we're in our new memory region now.. */
		}

	for(i=0;i<numfacs;i++)
		{
		chg=get_32(offs);  offs+=4;
		if(chg>maxchg) {maxchg=chg; maxindx=i; }
		mvlfacs[i].lastchange=chg;
		DEBUG(printf(LXTHDR"Changes: %d '%s' %08x\n", i, mvlfacs[i].name, chg));
		}

	if(zsync_table_size)
		{
		mm = mmcache;
		free_2(decmem); decmem = NULL;
		gzclose(zhandle);
		}

	DEBUG(printf(LXTHDR"Maxchange at: %08x for symbol '%s' of len %d\n", maxchg, mvlfacs[maxindx].name,  mvlfacs[maxindx].len));

	maxchange=maxchg;
	maxindex=maxindx;
	}

if(zchg_size)
	{
	if(zchg_predec_size > 128*1024*1024)
		{
		char *nam = tmpnam(NULL);
		FILE *tmp = fopen(nam, "wb");
		unsigned int len=zchg_predec_size;
		int rc;
		char buf[32768];
		int fd2 = open(nam, O_RDONLY);
		char testbyte[2]={0,0};
		char is_bz2;
	
		unlink(nam);
	
		printf(LXTHDR"Compressed change records detected, making tempfile...\n");
		if(change_field_offset != lseek(fd, change_field_offset, SEEK_SET)) { fprintf(stderr, LXTHDR"lseek error at offset %08x\n", change_field_offset); exit(255); }

		read(fd, &testbyte, 2);
		is_bz2 = (testbyte[0]=='B')&&(testbyte[1]=='Z');

		if(change_field_offset != lseek(fd, change_field_offset, SEEK_SET)) { fprintf(stderr, LXTHDR"lseek error at offset %08x\n", change_field_offset); exit(255); }

		if(is_bz2)
			{
			zhandle = BZ2_bzdopen(dup(fd), "rb");
	
			while(len)
				{
				int siz = (len>32768) ? 32768 : len; 
				rc = BZ2_bzread(zhandle, buf, siz);	
				if(rc!=siz) { fprintf(stderr, LXTHDR"gzread error to tempfile %d (act) vs %d (exp), exiting.\n", rc, siz); exit(255); }
				if(1 != fwrite(buf, siz, 1, tmp)) { fprintf(stderr, LXTHDR"fwrite error to tempfile, exiting.\n", rc, siz); exit(255); };
				len -= siz;		
				}
	
			printf(LXTHDR"...expanded %08x into %08x bytes.\n", zchg_size, zchg_predec_size);
			BZ2_bzclose(zhandle);
			}
			else
			{
			zhandle = gzdopen(dup(fd), "rb");
	
			while(len)
				{
				int siz = (len>32768) ? 32768 : len; 
				rc = gzread(zhandle, buf, siz);	
				if(rc!=siz) { fprintf(stderr, LXTHDR"gzread error to tempfile %d (act) vs %d (exp), exiting.\n", rc, siz); exit(255); }
				if(1 != fwrite(buf, siz, 1, tmp)) { fprintf(stderr, LXTHDR"fwrite error to tempfile, exiting.\n", rc, siz); exit(255); };
				len -= siz;		
				}
	
			printf(LXTHDR"...expanded %08x into %08x bytes.\n", zchg_size, zchg_predec_size);
			gzclose(zhandle);
			}
		munmap(mm, f_len); close(fd);
		fflush(tmp);
		fseek(tmp, 0, SEEK_SET);
		fclose(tmp);
	
		fd = fd2;
		mm=mmap(NULL, zchg_predec_size, PROT_READ, MAP_SHARED, fd, 0);	
		mm-=4; /* because header and version don't exist in packed change records */
		}
		else
		{
		unsigned int len=zchg_predec_size;
		int rc;
		char *buf = malloc_2(zchg_predec_size);
		char *pnt = buf;
		char testbyte[2]={0,0};
		char is_bz2;
		
		printf(LXTHDR"Compressed change records detected...\n");
		if(change_field_offset != lseek(fd, change_field_offset, SEEK_SET)) { fprintf(stderr, LXTHDR"lseek error at offset %08x\n", change_field_offset); exit(255); }

		read(fd, &testbyte, 2);
		is_bz2 = (testbyte[0]=='B')&&(testbyte[1]=='Z');

		if(change_field_offset != lseek(fd, change_field_offset, SEEK_SET)) { fprintf(stderr, LXTHDR"lseek error at offset %08x\n", change_field_offset); exit(255); }

		if(is_bz2)
			{
			zhandle = BZ2_bzdopen(dup(fd), "rb");
	
			while(len)
				{
				int siz = (len>32768) ? 32768 : len; 
				rc = BZ2_bzread(zhandle, pnt, siz);	
				if(rc!=siz) { fprintf(stderr, LXTHDR"BZ2_bzread error to buffer %d (act) vs %d (exp), exiting.\n", rc, siz); exit(255); }
				pnt += siz;
				len -= siz;		
				}
	
			printf(LXTHDR"...expanded %08x into %08x bytes.\n", zchg_size, zchg_predec_size);
			BZ2_bzclose(zhandle);
			}
			else
			{
			zhandle = gzdopen(dup(fd), "rb");
	
			while(len)
				{
				int siz = (len>32768) ? 32768 : len; 
				rc = gzread(zhandle, pnt, siz);	
				if(rc!=siz) { fprintf(stderr, LXTHDR"gzread error to buffer %d (act) vs %d (exp), exiting.\n", rc, siz); exit(255); }
				pnt += siz;
				len -= siz;		
				}
	
			printf(LXTHDR"...expanded %08x into %08x bytes.\n", zchg_size, zchg_predec_size);
			gzclose(zhandle);
			}

		munmap(mm, f_len); close(fd);
	
		fd=-1;
		mm=buf-4; /* because header and version don't exist in packed change records */
		}
	}


if(!sync_table_offset)
	{
	unsigned int vlen = zchg_predec_size ? zchg_predec_size+4 : 0;
	unsigned int offs = zchg_predec_size ? 4 : 0;
	unsigned int numfacs_bytes;
	unsigned int num_records = 0;
	unsigned int last_change_delta, numbytes;
	int *positional_compar = positional_information;
        int *positional_kill_pnt = positional_information + total_cycles;
        char positional_kill = 0;

	unsigned int dict_16_offset_new = 0;
	unsigned int dict_24_offset_new = 0;
	unsigned int dict_32_offset_new = 0;

	char *nam;
	FILE *tmp;
	int recfd;
	
	printf(LXTHDR"Linear LXT encountered...\n");

	if(!zchg_predec_size)
		{
		fprintf(stderr, LXTHDR"Uncompressed linear LXT not supported, exiting.\n");
		exit(255);
		}

	if(numfacs >= 256*65536)
        	{
                numfacs_bytes = 3;
                }
        else
        if(numfacs >= 65536)
                {
                numfacs_bytes = 2;
               	}
        else
        if(numfacs >= 256)
               	{
               	numfacs_bytes = 1;
		}
        else
               	{
               	numfacs_bytes = 0;
               	}

	nam = tmpnam(NULL);
	tmp = fopen(nam, "wb");
	fpos = 4;	/* fake 4 bytes padding */
	recfd = open(nam, O_RDONLY);

	unlink(nam);

	while(offs < vlen)
		{
		unsigned int facidx;
		unsigned char cmd;
		unsigned int offscache = offs;
		unsigned int offscache2, offscache3;
		unsigned int height;
		unsigned char cmdkill;

		num_records++;

                /* remake time vs position table on the fly */
                if(!positional_kill) 
                        {
                        if(offs == *positional_compar)  
                                {
                                *positional_compar = fpos;
                                positional_compar++;
                                if(positional_compar == positional_kill_pnt) positional_kill = 1;
                                }
                        }

		switch(numfacs_bytes&3)
			{
			case 0: facidx = get_byte(offs); break;
			case 1: facidx = get_16(offs); break;
			case 2: facidx = get_24(offs); break;
			case 3: facidx = get_32(offs); break;
			}

		if(facidx>numfacs)
			{
			fprintf(stderr, LXTHDR"Facidx %d out of range (vs %d) at offset %08x, exiting.\n", facidx, numfacs, offs);
			exit(255);
			}

		offs += (numfacs_bytes+1);

		cmdkill = mvlfacs[facidx].flags & (LT_SYM_F_DOUBLE|LT_SYM_F_STRING);
		if(!cmdkill)
			{
			cmd = get_byte(offs);

			if(cmd>0xf)
				{
				fprintf(stderr, LXTHDR"Command byte %02x invalid at offset %08x, exiting.\n", cmd, offs);
				exit(0);
				}

			offs++;
			}
			else
			{
			cmd=0;
			}

		offscache2 = offs;

		height = mvlfacs[facidx].array_height;
		if(height)
			{
			if(height >= 256*65536)       
			        {
			        offs += 4;
			        }
			else                    
			if(height >= 65536)
			        {
			        offs += 3;
			        }
			else
			if(height >= 256)     
			        {
			        offs += 2;
			        }
			else
			        {
			        offs += 1;
			        }
			}


		offscache3 = offs;
		if(!dict_16_offset_new)
			{
			if (offs == dict_16_offset) { dict_16_offset_new = fpos; }
			}
		else
		if(!dict_24_offset_new)
			{
			if (offs == dict_24_offset) { dict_24_offset_new = fpos; }
			}
		else
		if(!dict_32_offset_new)
			{
			if (offs == dict_32_offset) { dict_32_offset_new = fpos; }
			}


		/* printf("%08x : %04x %02x (%d) %s[%d:%d]\n", offscache, facidx, cmd, mvlfacs[facidx].len, mvlfacs[facidx].name, mvlfacs[facidx].msb, mvlfacs[facidx].lsb); */

		if(!cmdkill)
		switch(cmd)
			{
			int modlen;
			case 0x0:	
			modlen = (!(mvlfacs[facidx].flags&LT_SYM_F_INTEGER)) ? mvlfacs[facidx].len : 32;
			if((dict_string_mem_array) && (modlen>dict_width))
				{
				if((!dict_16_offset)||(offscache3<dict_16_offset))
					{
					offs += 1;
					}
				else
				if((!dict_24_offset)||(offscache3<dict_24_offset))
					{
					offs += 2;
					}
				else
				if((!dict_32_offset)||(offscache3<dict_32_offset))
					{
					offs += 3;
					}
				else
					{				
					offs += 4;
					}
				}
				else
				{
				offs += (mvlfacs[facidx].len + 7)/8;
				}

			break;


			case 0x1:
					offs += (mvlfacs[facidx].len + 3)/4;
					break;

			case 0x2:
					offs += (mvlfacs[facidx].len + 1)/2;
					break;

			case 0x3:
			case 0x4:
			case 0x5:
			case 0x6:
			case 0x7:
			case 0x8:
			case 0x9:
			case 0xa:
			case 0xb:	break;	/* single byte, no extra "skip" */


			case 0xc:
			case 0xd:
			case 0xe:
			case 0xf:	offs += ((cmd&3)+1);	/* skip past numbytes_trans */
					break;
			}
		else
			{
			/* cmdkill = 1 for strings + reals skip bytes */
			if(mvlfacs[facidx].flags & LT_SYM_F_DOUBLE)
				{
				offs += 8;
				}
				else	/* strings */
				{
				while(get_byte(offs)) offs++;
				offs++;
				}
			}

		last_change_delta = fpos - mvlfacs[facidx].lastchange - 2;
		mvlfacs[facidx].lastchange = fpos;

		maxchange=fpos;
		maxindex=facidx;

        	if(last_change_delta >= 256*65536)
                	{
                	numbytes = 3;
                	}
        	else
        	if(last_change_delta >= 65536)
                	{
                	numbytes = 2;
                	}
        	else
        	if(last_change_delta >= 256)
                	{
                	numbytes = 1;
                	}
        	else
                	{
                	numbytes = 0;
                	}

		lt_emit_u8(tmp, (numbytes<<4) | cmd);
	        switch(numbytes&3)
	                {
	                case 0: lt_emit_u8(tmp, last_change_delta); break;
	                case 1: lt_emit_u16(tmp, last_change_delta); break;
	                case 2: lt_emit_u24(tmp, last_change_delta); break;
	                case 3: lt_emit_u32(tmp, last_change_delta); break;
                	}

		if(offs-offscache2)
			{
			fpos += fwrite(mm+offscache2, 1, offs-offscache2, tmp);	/* copy rest of relevant info */
			}
		}

	dict_16_offset = dict_16_offset_new;
	dict_24_offset = dict_24_offset_new;
	dict_32_offset = dict_32_offset_new;

        fflush(tmp);
        fseek(tmp, 0, SEEK_SET);
        fclose(tmp);
	printf(LXTHDR"%d linear records converted into %08x bytes.\n", num_records, fpos-4);

	if(zchg_predec_size > 128*1024*1024)
		{
	        munmap(mm+4, zchg_predec_size); close(fd);
		}
		else
		{
		free(mm+4);
		}

	fd = recfd;              
        mm=mmap(NULL, fpos-4, PROT_READ, MAP_SHARED, recfd, 0);
        mm-=4; /* because header and version don't exist in packed change records */
	}
}


/*
 * given a fac+offset, return the binary data for it
 */
static char *parse_offset(struct fac *which, unsigned int offs)
{
int v, v2, j;
int k, l;
char *pnt;
char repeat;

l=which->len;
pnt = lt_buf;
v=get_byte(offs);
v2=v&0x0f;

switch(v2)
	{
	case 0x00:	/* MVL2 */
			{
			unsigned int msk;
			int bitcnt=0;
			int ch;
			
			if((dict_string_mem_array) && (l>dict_width))
				{
				unsigned int dictpos;
				unsigned int ld;

				offs += ((v>>4)&3)+2;	/* skip value */

				if((!dict_16_offset)||(offs<dict_16_offset))
					{
					dictpos = get_byte(offs);
					}
				else
				if((!dict_24_offset)||(offs<dict_24_offset))
					{
					dictpos = get_16(offs);
					}
				else
				if((!dict_32_offset)||(offs<dict_32_offset))
					{
					dictpos = get_24(offs);
					}
				else
					{				
					dictpos = get_32(offs);
					}

				if(dictpos <= dict_num_entries)
					{
					ld = strlen(dict_string_mem_array[dictpos]);
					for(j=0;j<(l-(ld+1));j++)
						{
						*(pnt++) = '0';
						}
					*(pnt++) = '1';
					memcpy(pnt, dict_string_mem_array[dictpos], ld);
					}
					else
					{
					fprintf(stderr, LXTHDR"dict entry at offset %08x [%d] out of range, ignoring!\n", dictpos, offs);
					for(j=0;j<l;j++)
						{
						*(pnt++) = '0';
						}
					}
				}
				else
				{
				offs += ((v>>4)&3)+2;	/* skip value */
				for(j=0;;j++)
					{
					ch=get_byte(offs+j);
					msk=0x80;
					for(k=0;k<8;k++)
						{
						*(pnt++)= (ch&msk) ? '1' : '0';
						msk>>=1;
						bitcnt++;
						if(bitcnt==l) goto bail;
						}
					}
				}
			}
			break;

	case 0x01:	/* MVL4 */
			{
			int bitcnt=0;
			int ch;
			int rsh;

			offs += ((v>>4)&3)+2;	/* skip value */
			for(j=0;;j++)
				{
				ch=get_byte(offs+j);
				rsh=6;
				for(k=0;k<4;k++)
					{
					*(pnt++)=convert_mvl((ch>>rsh)&0x3);
					rsh-=2;
					bitcnt++;
					if(bitcnt==l) goto bail;
					}
				}
			}
			break;

	case 0x02:	/* MVL9 */
			{
			int bitcnt=0;
			int ch;
			int rsh;

			offs += ((v>>4)&3)+2;	/* skip value */
			for(j=0;;j++)
				{
				ch=get_byte(offs+j);
				rsh=4;
				for(k=0;k<2;k++)
					{
					*(pnt++)=convert_mvl(ch>>rsh);
					rsh-=4;
					bitcnt++;
					if(bitcnt==l) goto bail;
					}
				}
			}
			break;

	case 0x03:	/* mvl repeat expansions */
	case 0x04:
	case 0x05:
	case 0x06:
	case 0x07:
	case 0x08:
	case 0x09:
	case 0x0a:
	case 0x0b:
			repeat = convert_mvl(v2-3);
			for(j=0;j<l;j++)
				{
				*(pnt++)=repeat;
				}
			break;

	default:
			fprintf(stderr, LXTHDR"Unknown %02x at offset: %08x\n", v, offs);
			exit(255);
	}

bail:
return(lt_buf);
}


/*
 * mainline
 */
TimeType lxt_main(char *fname)
{
int i;
struct Node *n;
struct symbol *s, *prevsymroot=NULL, *prevsym=NULL;
unsigned int tagpnt;
int tag;

fd=open(fname, O_RDONLY);
if(fd<0)
        {
        fprintf(stderr, "Could not open '%s', exiting.\n", fname);
        exit(0);
        }

f_len=lseek(fd, 0, SEEK_END);
mm=mmap(NULL, f_len, PROT_READ, MAP_SHARED, fd, 0);

if((i=get_16(0))!=LT_HDRID)
	{
	fprintf(stderr, "Not an LXT format AET, exiting.\n");
	exit(255);
	}

if((version=get_16(2))>LT_VERSION)
	{
	fprintf(stderr, "Version %d of LXT format AETs not supported, exiting.\n", version);
	exit(255);
	}	

if(get_byte(f_len-1)!=LT_TRLID)
	{
	fprintf(stderr, "LXT '%s' is truncated, exiting.\n", fname);
	exit(255);
	}

DEBUG(printf(LXTHDR"Loading LXT '%s'...\n", fname));
DEBUG(printf(LXTHDR"Len: %d\n", (unsigned int)f_len));

tagpnt = f_len-2;
while((tag=get_byte(tagpnt))!=LT_SECTION_END)
	{
	int offset = get_32(tagpnt-4);
	tagpnt-=5;

	switch(tag)
		{
		case LT_SECTION_CHG:			change_field_offset=offset; DEBUG(printf(LXTHDR"LT_SECTION_CHG at %08x\n", offset)); break;
		case LT_SECTION_SYNC_TABLE:		sync_table_offset=offset; DEBUG(printf(LXTHDR"LT_SECTION_SYNC_TABLE at %08x\n", offset)); break;
		case LT_SECTION_FACNAME:		facname_offset=offset; DEBUG(printf(LXTHDR"LT_SECTION_FACNAME at %08x\n", offset)); break;
		case LT_SECTION_FACNAME_GEOMETRY:	facgeometry_offset=offset; DEBUG(printf(LXTHDR"LT_SECTION_FACNAME_GEOMETRY at %08x\n", offset)); break;
		case LT_SECTION_TIMESCALE:		timescale_offset=offset; DEBUG(printf(LXTHDR"LT_SECTION_TIMESCALE at %08x\n", offset)); break;
		case LT_SECTION_TIME_TABLE:		time_table_offset=offset; DEBUG(printf(LXTHDR"LT_SECTION_TIME_TABLE at %08x\n", offset)); break;
		case LT_SECTION_TIME_TABLE64:		time_table_offset64=offset; DEBUG(printf(LXTHDR"LT_SECTION_TIME_TABLE64 at %08x\n", offset)); break;
		case LT_SECTION_INITIAL_VALUE:		initial_value_offset=offset; DEBUG(printf(LXTHDR"LT_SECTION_INITIAL_VALUE at %08x\n", offset)); break;
		case LT_SECTION_DOUBLE_TEST:		double_test_offset=offset; DEBUG(printf(LXTHDR"LT_SECTION_DOUBLE_TEST at %08x\n", offset)); break;

		case LT_SECTION_ZFACNAME_PREDEC_SIZE:	zfacname_predec_size=offset; DEBUG(printf(LXTHDR"LT_SECTION_ZFACNAME_PREDEC_SIZE = %08x\n", offset)); break;
		case LT_SECTION_ZFACNAME_SIZE:		zfacname_size=offset; DEBUG(printf(LXTHDR"LT_SECTION_ZFACNAME_SIZE = %08x\n", offset)); break;
		case LT_SECTION_ZFACNAME_GEOMETRY_SIZE:	zfacgeometry_size=offset; DEBUG(printf(LXTHDR"LT_SECTION_ZFACNAME_GEOMETRY_SIZE = %08x\n", offset)); break;
		case LT_SECTION_ZSYNC_SIZE:		zsync_table_size=offset; DEBUG(printf(LXTHDR"LT_SECTION_ZSYNC_SIZE = %08x\n", offset)); break;
		case LT_SECTION_ZTIME_TABLE_SIZE:	ztime_table_size=offset; DEBUG(printf(LXTHDR"LT_SECTION_ZTIME_TABLE_SIZE = %08x\n", offset)); break;
		case LT_SECTION_ZCHG_PREDEC_SIZE:	zchg_predec_size=offset; DEBUG(printf(LXTHDR"LT_SECTION_ZCHG_PREDEC_SIZE = %08x\n", offset)); break;
		case LT_SECTION_ZCHG_SIZE:		zchg_size=offset; DEBUG(printf(LXTHDR"LT_SECTION_ZCHG_SIZE = %08x\n", offset)); break;

		case LT_SECTION_ZDICTIONARY:		zdictionary_offset=offset; DEBUG(printf(LXTHDR"LT_SECTION_ZDICTIONARY = %08x\n", offset)); break;
		case LT_SECTION_ZDICTIONARY_SIZE:	zdictionary_predec_size=offset; DEBUG(printf(LXTHDR"LT_SECTION_ZDICTIONARY_SIZE = %08x\n", offset)); break;

		case LT_SECTION_EXCLUDE_TABLE:		exclude_offset=offset; DEBUG(printf(LXTHDR"LT_SECTION_EXCLUDE_TABLE = %08x\n", offset)); break;

		default: fprintf(stderr, "Skipping unknown section tag %02x.\n", tag); break;
		}	
	}

if(exclude_offset)
	{
	unsigned int offset = exclude_offset;
	int i, num_blackouts = get_32(offset);
	TimeType bs, be;
	struct blackout_region_t *bt;

	offset+=4;

	for(i=0;i<num_blackouts;i++)
		{
		bs = get_64(offset); offset+=8;
		be = get_64(offset); offset+=8;

		bt = calloc(1, sizeof(struct blackout_region_t));
		bt->bstart = bs;
		bt->bend = be;
		bt->next = blackout_regions;

		blackout_regions = bt;
		}
	}

if(double_test_offset)
	{
	create_double_endian_mask(double_test_offset);
	}

if(timescale_offset)
	{
	signed char scale;

	scale=(signed char)get_byte(timescale_offset);
	switch(scale)
		{
		case 0:		time_dimension = 's'; break;

		case -1:	time_scale = LLDescriptor(100); time_dimension = 'm'; break;
		case -2:	time_scale = LLDescriptor(10);  
		case -3:					time_dimension = 'm'; break;

		case -4:	time_scale = LLDescriptor(100);	time_dimension = 'u'; break;
		case -5:	time_scale = LLDescriptor(10);	
		case -6:					time_dimension = 'u'; break;

		case -10:	time_scale = LLDescriptor(100); time_dimension = 'p'; break;
		case -11:	time_scale = LLDescriptor(10);  
		case -12:					time_dimension = 'p'; break;

		case -13:	time_scale = LLDescriptor(100); time_dimension = 'f'; break;
		case -14:	time_scale = LLDescriptor(10);  
		case -15:					time_dimension = 'f'; break;

		case -7:	time_scale = LLDescriptor(100); time_dimension = 'n'; break;
		case -8:	time_scale = LLDescriptor(10);  
		case -9:
		default:					time_dimension = 'n'; break;
		}
	}
	else
	{
	time_dimension = 'n';
	}

if(!facname_offset)
	{
	fprintf(stderr, "LXT '%s' is missing a facility name section, exiting.\n", fname);
	exit(255);
	}

numfacs=get_32(facname_offset);
DEBUG(printf(LXTHDR"Number of facs: %d\n", numfacs));
mvlfacs=(struct fac *)calloc(numfacs,sizeof(struct fac));

if(initial_value_offset)
	{
	switch(get_byte(initial_value_offset))
		{
		case 0:
		case 7:		initial_value = 0; break;
		case 1:
		case 4:		initial_value = 3; break;
		case 2:		initial_value = 2; break;
		default:	initial_value = 1; break;
		}
	}
	else
	{
	initial_value = 1;
	}

if(zdictionary_offset)
	{
	if(zdictionary_predec_size)
		{
		build_dict();
		}
		else
		{
		fprintf(stderr, "LXT '%s' is missing a zdictionary_predec_size chunk, exiting.\n", fname);
		exit(255);
		}
	}

build_facs(fname);
build_facs2(fname);


/* do your stuff here..all useful info has been initialized by now */

if(!hier_was_explicitly_set)    /* set default hierarchy split char */
        {
        hier_delimeter='.';
        }

for(i=0;i<numfacs;i++)
        {
	char buf[4096];
	char *str;	
	struct fac *f;

	if(mvlfacs[i].flags&LT_SYM_F_ALIAS)
		{
		int alias = mvlfacs[i].array_height;
		f=mvlfacs+alias;

		while(f->flags&LT_SYM_F_ALIAS)
			{
			f=mvlfacs+f->array_height;
			}
		}
		else
		{
		f=mvlfacs+i;
		}

	if((f->len>1)&& (!(f->flags&(LT_SYM_F_INTEGER|LT_SYM_F_DOUBLE|LT_SYM_F_STRING))) )
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
	else if ( (f->len==1)&&(!(f->flags&(LT_SYM_F_INTEGER|LT_SYM_F_DOUBLE|LT_SYM_F_STRING)))&&
			((i!=numfacs-1)&&(!strcmp(mvlfacs[i].name, mvlfacs[i+1].name))) ||
			((i!=0)&&(!strcmp(mvlfacs[i].name, mvlfacs[i-1].name))) &&
			(mvlfacs[i].msb!=-1)&&(mvlfacs[i].lsb!=-1)
		)
		{
		sprintf(buf, "%s[%d]", mvlfacs[i].name,mvlfacs[i].msb);
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
		if((prevsym)&&(i>0)&&(!strcmp(mvlfacs[i].name, mvlfacs[i-1].name)))	/* allow chaining for search functions.. */
			{
			prevsym->vec_root = prevsymroot;
			prevsym->vec_chain = s;
			s->vec_root = prevsymroot;
			prevsym = s;
			}
			else
			{
			prevsymroot = prevsym = s;
			}
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

		if(f->flags&LT_SYM_F_INTEGER)
			{
			mvlfacs[i].msb=31;
			mvlfacs[i].lsb=0;
			mvlfacs[i].len=32;
			}
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

	if((f->len>1)||(f->flags&&(LT_SYM_F_DOUBLE|LT_SYM_F_STRING)))
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

printf(LXTHDR"Sorting facilities at hierarchy boundaries...");
fflush(stdout);
heapsort(facs,numfacs);
printf("sorted.\n");
	
for(i=0;i<numfacs;i++)
	{
	char *subst, ch;

	subst=facs[i]->name;
	while((ch=(*subst)))
		{	
		if(ch==0x01) { *subst=hier_delimeter; }	/* restore back to normal */
		subst++;
		}

#ifdef DEBUG_FACILITIES
	printf("%-4d %s\n",i,facs[i]->name);
#endif
	}

facs_are_sorted=1;

printf(LXTHDR"Building facility hierarchy tree...");
fflush(stdout);
init_tree();		
for(i=0;i<numfacs;i++)	
{
build_tree_from_name(facs[i]->name, i);
}
treeprune(treeroot);
printf("built.\n\n");

#ifdef DEBUG_FACILITIES
treedebug(treeroot,"");
#endif

min_time = first_cycle*time_scale; max_time=last_cycle*time_scale;
printf("["TTFormat"] start time.\n["TTFormat"] end time.\n", min_time, max_time);
is_lxt = ~0;

return(max_time);
}


/*
 * this is the black magic that handles aliased signals...
 */
static void lxt_resolver(nptr np, nptr resolve)
{
np->ext = resolve->ext;
memcpy(&np->head, &resolve->head, sizeof(struct HistEnt));
np->curr = resolve->curr;
np->harray = resolve->harray;
np->numhist = resolve->numhist;
np->mvlfac=NULL;
}


/*
 * actually import an lxt trace but don't do it if
 * 1) it's already been imported
 * 2) an alias of this trace has been imported--instead
 *    copy over the relevant info and be done with it.
 */
void import_lxt_trace(nptr np)
{
unsigned int offs, offsdelta;
int v, w;
int ary_indx, ary_skip;
TimeType tmval;
TimeType prevtmval;
struct HistEnt *htemp;
struct HistEnt *histent_head, *histent_tail;
char *parsed;
int len, i, j;
struct fac *f;

if(!(f=np->mvlfac)) return;	/* already imported */

if(np->mvlfac->flags&LT_SYM_F_ALIAS)
	{
	int alias = np->mvlfac->array_height;
	f=mvlfacs+alias;

	if(f->resolve_lxt_alias_to)
		{
		if(!np->mvlfac->resolve_lxt_alias_to) np->mvlfac->resolve_lxt_alias_to = f->resolve_lxt_alias_to;
		}
		else
		{
		f->resolve_lxt_alias_to = np;
		}

	while(f->flags&LT_SYM_F_ALIAS)
		{
		f=mvlfacs+f->array_height;

		if(f->resolve_lxt_alias_to)
			{
			if(!np->mvlfac->resolve_lxt_alias_to) np->mvlfac->resolve_lxt_alias_to = f->resolve_lxt_alias_to;
			}
			else
			{
			f->resolve_lxt_alias_to = np; 
			}
		}
	}

/* f is the head minus any aliases, np->mvlfac is us... */
if(np->mvlfac->resolve_lxt_alias_to)	/* in case we're an alias head for later.. */
	{
	lxt_resolver(np, np->mvlfac->resolve_lxt_alias_to);
	return;
	}

np->mvlfac->resolve_lxt_alias_to = np;	/* in case we're an alias head for later.. */
offs=f->lastchange;

tmval=LLDescriptor(-1);
prevtmval = LLDescriptor(-1);
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

if(f->array_height < 1)	/* sorry, arrays not supported */
while(offs)
	{
	unsigned char val;

	if( (w=((v=get_byte(offs))&0xF)) >0xb)
		{
		unsigned int offsminus1, offsminus2, offsminus3;
		TimeType delta, time_offsminus1;
		int skip;

		switch(v&0xF0)
			{
			case 0x00:
				skip = 2;
				offsdelta=get_byte(offs+1);
				break;
			case 0x10:
				skip = 3;
				offsdelta=get_16(offs+1);
				break;
			case 0x20:
				skip = 4;
				offsdelta=get_24(offs+1);
				break;
			case 0x30:
				skip = 5;
				offsdelta=get_32(offs+1);
				break;
			default:
				fprintf(stderr, "Unknown %02x at offset: %08x\n", v, offs);
				exit(0);
			}
		offsminus1 = offs-offsdelta-2;

		switch(get_byte(offsminus1)&0xF0)
			{
			case 0x00:
				offsdelta=get_byte(offsminus1+1);
				break;
			case 0x10:
				offsdelta=get_16(offsminus1+1);
				break;
			case 0x20:
				offsdelta=get_24(offsminus1+1);
				break;
			case 0x30:
				offsdelta=get_32(offsminus1+1);
				break;
			default:
				fprintf(stderr, "Unknown %02x at offset: %08x\n", get_byte(offsminus1), offsminus1);
				exit(0);
			}
		offsminus2 = offsminus1-offsdelta-2;

		delta = (time_offsminus1=bsearch_mvl_timechain(offsminus1)) - bsearch_mvl_timechain(offsminus2);

		if(len>1)
			{
			DEBUG(fprintf(stderr, "!!! DELTA = %Ld\n", delta));
			DEBUG(fprintf(stderr, "!!! offsminus1 = %08x\n", offsminus1));

			if(!lxt_clock_compress_to_z)
				{
				int val = get_byte(offsminus1)&0xF;
				int reps;
				int rcnt;
				unsigned int reconstructm1 = 0;
				unsigned int reconstructm2 = 0;
				unsigned int reconstructm3 = 0;
				unsigned int rle_delta[2];
				int i;
	
				if((val!=0)&&(val!=3)&&(val!=4))
					{
					fprintf(stderr, "Unexpected clk compress byte %02x at offset: %08x\n", get_byte(offsminus1), offsminus1);
					exit(0);
					}
	
				switch(w&3)
					{
					case 0:	reps = get_byte(offs+skip); break;
					case 1: reps = get_16(offs+skip); break;
					case 2: reps = get_24(offs+skip); break;
					case 3: reps = get_32(offs+skip); break;
					}
	
				reps++;

				DEBUG(fprintf(stderr, "!!! reps = %d\n", reps));

				parsed=parse_offset(f, offsminus1);
				for(i=0;i<len;i++)
					{
					reconstructm1 <<= 1;
					reconstructm1 |= (parsed[i]&1);
					}

				DEBUG(fprintf(stderr, "!!! M1 = '%08x'\n", reconstructm1));

				parsed=parse_offset(f, offsminus2);
				for(i=0;i<len;i++)
					{
					reconstructm2 <<= 1;
					reconstructm2 |= (parsed[i]&1);
					}

				DEBUG(fprintf(stderr, "!!! M2 = '%08x'\n", reconstructm2));

				switch(get_byte(offsminus2)&0xF0)
					{
					case 0x00:
						offsdelta=get_byte(offsminus2+1);
						break;
					case 0x10:
						offsdelta=get_16(offsminus2+1);
						break;
					case 0x20:
						offsdelta=get_24(offsminus2+1);
						break;
					case 0x30:
						offsdelta=get_32(offsminus2+1);
						break;
					default:
						fprintf(stderr, "Unknown %02x at offset: %08x\n", get_byte(offsminus2), offsminus2);
						exit(0);
					}
				offsminus3 = offsminus2-offsdelta-2;

				parsed=parse_offset(f, offsminus3);
				for(i=0;i<len;i++)
					{
					reconstructm3 <<= 1;
					reconstructm3 |= (parsed[i]&1);
					}

				DEBUG(fprintf(stderr, "!!! M3 = '%08x'\n", reconstructm3));

				rle_delta[0] = reconstructm2 - reconstructm3;
				rle_delta[1] = reconstructm1 - reconstructm2;

				DEBUG(fprintf(stderr, "!!! RLE0 = '%08x'\n", rle_delta[0]));
				DEBUG(fprintf(stderr, "!!! RLE1 = '%08x'\n", rle_delta[1]));

				DEBUG(fprintf(stderr, "!!! time_offsminus1 = %Ld\n", time_offsminus1));

				tmval = time_offsminus1 + (delta * reps);
				for(rcnt=0;rcnt<reps;rcnt++)
					{
					int k;
					int j = (reps - rcnt);
					unsigned int res = reconstructm1 +
						((j/2)+(j&0))*rle_delta[1] + 
						((j/2)+(j&1))*rle_delta[0];

					DEBUG(fprintf(stderr, "!!! %Ld -> '%08x'\n", tmval, res));

					for(k=0;k<len;k++)
						{
						parsed[k] = ((1<<(len-k-1)) & res) ? '1' : '0';
						}

					htemp = histent_calloc();
					htemp->v.vector = (char *)malloc_2(len);
					memcpy(htemp->v.vector, parsed, len);
					htemp->time = tmval;
					htemp->next = histent_head;
					histent_head = htemp;
					np->numhist++;
	
					tmval-=delta;
					}
				}
				else	/* compress to z on multibit */
				{
				int i;	

				htemp = histent_calloc();
				htemp->v.vector = (char *)malloc_2(len);
				for(i=0;i<len;i++) { htemp->v.vector[i] = 'z'; }
				tmval = time_offsminus1 + delta;
				htemp->time = tmval;
				htemp->next = histent_head;
				histent_head = htemp;
				np->numhist++;
				}

			offs = offsminus1;	/* no need to recalc it again! */
			continue;
			}
			else
			{
			if(!lxt_clock_compress_to_z)
				{
				int val = get_byte(offsminus1)&0xF;
				int reps;
				int rcnt;
	
				if((val<3)||(val>4))
					{
					fprintf(stderr, "Unexpected clk compress byte %02x at offset: %08x\n", get_byte(offsminus1), offsminus1);
					exit(0);
					}
	
				switch(w&3)
					{
					case 0:	reps = get_byte(offs+skip); break;
					case 1: reps = get_16(offs+skip); break;
					case 2: reps = get_24(offs+skip); break;
					case 3: reps = get_32(offs+skip); break;
					}
	
				reps++;
				val = (reps & 1) ^ (val==4);	/* because x3='0', x4='1' */
				val |= val<<1;			/* because 00='0', 11='1' */
	
				tmval = time_offsminus1 + (delta * reps);
				for(rcnt=0;rcnt<reps;rcnt++)
					{
					if(val!=histent_head->v.val)
						{
						htemp = histent_calloc();
						htemp->v.val = val;
						htemp->time = tmval;
						htemp->next = histent_head;
						histent_head = htemp;
						np->numhist++;
						}
						else
						{
						histent_head->time = tmval;
						}
	
					tmval-=delta;
					val=val^3;
					}
				}
				else
				{
				int val=2;
	
				if(val!=histent_head->v.val)
					{
					htemp = histent_calloc();
					htemp->v.val = val;
					htemp->time = time_offsminus1 + delta;
					htemp->next = histent_head;
					histent_head = htemp;
					np->numhist++;
					}
					else
					{
					histent_head->time = time_offsminus1 + delta;
					}
				tmval = time_offsminus1 + delta;
				}
			}

		offs = offsminus1;	/* no need to recalc it again! */
		continue;
		}
	else
	if((tmval=bsearch_mvl_timechain(offs))!=prevtmval)	/* get rid of glitches (if even possible) */
		{
		DEBUG(printf(LXTHDR"offs: %08x is time %08x\n", offs, tmval));
		if(!(f->flags&(LT_SYM_F_DOUBLE|LT_SYM_F_STRING)))
			{
			parsed=parse_offset(f, offs);

			if(len==1)
				{
				switch(parsed[0])
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
					htemp->time = tmval;
					htemp->next = histent_head;
					histent_head = htemp;
					np->numhist++;
					}
					else
					{
					histent_head->time = tmval;
					}
				}
				else
				{
				if(memcmp(parsed, histent_head->v.vector, len))
					{
					htemp = histent_calloc();
					htemp->v.vector = (char *)malloc_2(len);
					memcpy(htemp->v.vector, parsed, len);
					htemp->time = tmval;
					htemp->next = histent_head;
					histent_head = htemp;
					np->numhist++;
					}
					else
					{
					histent_head->time = tmval;
					}
				}
			}
		else
		if(f->flags&LT_SYM_F_DOUBLE)
			{
			int offs_dbl = offs + ((get_byte(offs)>>4)&3)+2;   /* skip value */

			htemp = histent_calloc();
			htemp->flags = HIST_REAL;

			if(double_is_native)
				{
				htemp->v.vector = ((unsigned char *)mm+offs_dbl);
				DEBUG(printf(LXTHDR"Added double '%.16g'\n", *((double *)(mm+offs_dbl))));
				}
				else
				{
				htemp->v.vector = swab_double_via_mask(offs_dbl);
				DEBUG(printf(LXTHDR"Added bytefixed double '%.16g'\n", *((double *)(htemp->v.vector))));
				}
			htemp->time = tmval;
			htemp->next = histent_head;
			histent_head = htemp;
			np->numhist++;
			}
			else
			{						/* defaults to if(f->flags&LT_SYM_F_STRING) */
			int offs_str = offs + ((get_byte(offs)>>4)&3)+2;   /* skip value */

			htemp = histent_calloc();
			htemp->flags = HIST_REAL|HIST_STRING;

			htemp->v.vector = ((unsigned char *)mm+offs_str);
			DEBUG(printf(LXTHDR"Added string '%s'\n", (unsigned char *)mm+offs_str));
			htemp->time = tmval;
			htemp->next = histent_head;
			histent_head = htemp;
			np->numhist++;
			}
		}

	prevtmval = tmval;
	
/*	v=get_byte(offs); */
	switch(v&0xF0)
		{
		case 0x00:
			offsdelta=get_byte(offs+1);
			break;
	
		case 0x10:
			offsdelta=get_16(offs+1);
			break;
	
		case 0x20:
			offsdelta=get_24(offs+1);
			break;

		case 0x30:
			offsdelta=get_32(offs+1);
			break;

		default:
			fprintf(stderr, "Unknown %02x at offset: %08x\n", v, offs);
			exit(0);
		}

	offs = offs-offsdelta-2;
	}

np->mvlfac = NULL;	/* it's imported and cached so we can forget it's an mvlfac now */

for(j=0;j>-2;j--)
	{
	if(tmval!=first_cycle)
		{
		char init;

		htemp = histent_calloc();

		if(!(f->flags&(LT_SYM_F_DOUBLE|LT_SYM_F_STRING)))
			{
			if(initial_value_offset)
				{
				init = initial_value;
				}
				else
				{
				init = 1; /* x if unspecified */
				}
		
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
			}
			else
			{
			htemp->flags = HIST_REAL;
			if(f->flags&LT_SYM_F_STRING) htemp->flags |= HIST_STRING;
			}
	
		htemp->time = first_cycle+j;
		htemp->next = histent_head;
		histent_head = htemp;
		np->numhist++;
		}

	tmval=first_cycle+1;
	}

if(!(f->flags&(LT_SYM_F_DOUBLE|LT_SYM_F_STRING)))
	{
	if(len>1)
		{
		np->head.v.vector = (char *)malloc_2(len);
		for(i=0;i<len;i++) np->head.v.vector[i] = 1;
		}
		else
		{
		np->head.v.val = 1;                     /* 'x' */
		}
	}
	else
	{
	np->head.flags = HIST_REAL;
	if(f->flags&LT_SYM_F_STRING) np->head.flags |= HIST_STRING;
	}
np->head.time  = -2;
np->head.next = histent_head;
np->curr = histent_tail;
np->numhist++;
}
