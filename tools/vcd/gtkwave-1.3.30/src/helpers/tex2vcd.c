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

/*
 * tex2vcd.c
 * 050501ajb
 *
 * texsim aets are simply collections of single bit
 * changes that are indexed 16-bits at a time.  to
 * make the range larger than this, "offsets" are used
 * that provide either direct access or 64k trampolines.
 * they are *much* simpler than mvlsim format aets.
 *
 * 161001ajb: added support for awan format 0xa0 aets
 *
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

struct txfac
{
int array_height;
int msb, lsb;
int facindx;
int namindx;
};

struct namehier
{
struct namehier *next;
char *name;
char not_final;
};

static int numfacs=0;
static int facbytes=0;
static int multiplier=1;

static char *facnames=NULL;
static struct txfac *txfacs=NULL;
static FILE *handle=NULL;

/****************************************************************************/

static int get_byte(void)
{
return(fgetc(handle));
}

static unsigned int get_long(void)
{
unsigned int ch, v;
ch=get_byte(); v=ch; v<<=8;
ch=get_byte(); v|=ch; v<<=8;
ch=get_byte(); v|=ch; v<<=8;
ch=get_byte(); v|=ch;
return(v);
}

static unsigned int get_24(void)
{
unsigned int ch, v;
ch=get_byte(); v=ch; v<<=8;
ch=get_byte(); v|=ch; v<<=8;
ch=get_byte(); v|=ch;
return(v);
}

static unsigned int get_short(void)
{
unsigned int ch, v;
ch=get_byte(); v=ch; v<<=8;
ch=get_byte(); v|=ch; 
return(v);
}

/****************************************************************************/

struct namehier *nhold=NULL;

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

static void diff_hier(struct namehier *nh1, struct namehier *nh2)
{
struct namehier *nhtemp;

if(!nh2)
	{
	while((nh1)&&(nh1->not_final))
		{
		printf("$scope module %s $end\n", nh1->name);
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
			printf("$scope module %s $end\n", nh1->name);
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
			printf("$scope module %s $end\n", nh1->name);
			nh1=nh1->next;
			}
		break;
		}

	nh1=nh1->next;
	nh2=nh2->next;
	}
}


static void output_varname(struct txfac *t)
{
char *name=facnames+t->namindx;
char *pnt, *pnt2;
char *s;
int len;
struct namehier *nh_head=NULL, *nh_curr=NULL, *nhtemp;
int newlsb;

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

newlsb=((t->lsb+1)*(t->array_height))-1;
if(t->msb==newlsb)
        {
        printf("$var wire 1 %s %s $end\n", vcdid(t->facindx), nh_curr->name);
        }
        else
        {
        int i, delta=1, cbit=t->msb;

        len=newlsb-t->msb;
        if(len<0) {len=-len; delta=-delta;}
        len++;

        for(i=0;i<len;i++)
                {
                printf("$var wire 1 %s %s[%d] $end\n", vcdid(t->facindx+i), nh_curr->name, cbit);
                cbit+=delta;
                }
        }
}

/****************************************************************************/

static int hdr_process(void)
{
int ch;
char buf[1024];
int i, pos;
int aettype;

aettype=get_byte();
if((aettype!=0xc0)&&(aettype!=0xa0))
	{
	fprintf(stderr, "Not a Texsim format AET, exiting.\n");
	exit(0);
	}

fseek(handle, 0x14, SEEK_SET);		/* get sim date/time */
fread(buf, sizeof(char), 8, handle); buf[8]=0;
fread(buf+9, sizeof(char), 8, handle); buf[17]=0;
printf("$version\n");
printf("\tAET created at %s on %s.\n", buf+9, buf);

fseek(handle, 0x44, SEEK_SET);		/* get number of facs */
numfacs=get_long();
facbytes=get_long();

fseek(handle, 0x3c, SEEK_SET);          /* XXX : get delta multiplier of 65536 (is this offset right?) */
multiplier=get_long();

if(aettype==0xc0)
	{
	fseek(handle, 0x58, SEEK_SET);		/* get model date/time */
	fread(buf, sizeof(char), 8, handle); buf[8]=0;
	fread(buf+9, sizeof(char), 8, handle); buf[17]=0;
	pos=18;
	while((ch=get_byte())) buf[pos++]=ch;	
	buf[pos]=0;

	printf("\tModel %s created at %s on %s.\n", buf+18, buf+9, buf);
	}

printf("\t%d facilities found occupying %d bytes.\n", numfacs, facbytes);
printf("$end\n\n");
printf("$timescale\n\t1ns\n$end\n\n");

facnames=(char *)malloc(facbytes);
txfacs=(struct txfac *)calloc(numfacs, sizeof(struct txfac));

if(aettype==0xc0)
	{
	/* XXX : probably name starts at 0xf0 and you pad to next /4 boundary after the name.. */
	fseek(handle, 0xf0, SEEK_SET);
	while(get_byte());				/* skip over duplicate of model name.. */
	pos=ftell(handle);
	pos=(pos+3)&~3;
	fseek(handle, pos, SEEK_SET);
	}
	else
	{
	fseek(handle, 0xa4, SEEK_SET);
	}

for(i=0;i<numfacs;i++)                  /* pull in facinfo entries */
      	{
       	fseek(handle, 4, SEEK_CUR);             /* skip past low/high row (?) fields */
       	txfacs[i].array_height=get_short();;
       	txfacs[i].msb=0;                        /* MSB always zero */
       	txfacs[i].lsb=get_short()-1; 		/* +6 = LEN */
       	txfacs[i].facindx=get_long();           /* +8 = FAC START (FAC extends an additional abs(lsb-msb) entries) */
       	txfacs[i].namindx=get_long();           /* +12 = NAME POS IN NAME TABLE */
       	}   

fread(facnames, sizeof(char), facbytes, handle);
for(i=0;i<numfacs;i++) output_varname(txfacs+i);/* dump the names with hierarchy */
printf("$enddefinitions $end\n\n");
return(0);
}

/****************************************************************************/

int main(int argc, char **argv)
{
int ch, tag;
unsigned int v;
int tm=0;
int offset=0;
int i, n;

if(argc!=2)
	{
	fprintf(stderr, "Usage:\n------\n%s filename\n\nVCD is then output to stdout.\n", argv[0]);
	exit(0);
	}

handle=fopen(argv[1], "rb");
if(!handle)
	{
	fprintf(stderr, "Could not open '%s', exiting.\n", argv[1]);
	exit(0);
	}

hdr_process(); 			/* build symbol table */

for(;;)
	{
	tag=ch=get_byte();	/* these are the actual trace dump infos */
	switch(ch)
		{
		case 0xa4:
			tm=get_long();
			/* printf("[%02x] Absolute time: %d\n", tag, tm); */
			printf("#%d\n", tm);
			break;			

		case 0xa5:
			v=get_byte();
			tm+=v;
			/* printf("[%02x] Delta (+%d) time: %d\n", tag, v, tm); */
			printf("#%d\n", tm);
			break;

		case 0xa6:
			tm++;
			/* printf("[%02x] Increment time to %d\n", tag, tm); */
			printf("#%d\n", tm);
			break;

		case 0xa7:
			tm=0;
			/* printf("[%02x] Reset time to 0\n", tag); */
			printf("#%d\n", tm);
			break;
		
		case 0xa8:
			offset=get_long();
			/* printf("[%02x] Absolute offset: %d\n", tag, offset); */
			break;			

		case 0xa9:
			v=get_byte();
			offset+=(v*multiplier);
			/* printf("[%02x] Delta (+%d) offset: %d\n", tag, v, offset); */
			break;

		case 0xaa:			/* 0xa9 case when next char would've been 0x01 */
			offset+=multiplier;
			/* printf("[%02x] Increment (+%d) offset to %d\n", tag, multiplier, offset); */
			break;

		case 0xab:
			/* printf("[%02x] Reset offset to 0\n", offset); */
			break;

		case 0xac:
		case 0xad:
		case 0xae:
		case 0xaf:
			/* printf("[%02x] Reset all FACS to %c\n", "01xz"[tag&3]); */
			for(i=0;i<numfacs;i++)
				{
				int j, len, facindx;
				char rv="01xz"[tag&3];
				len=(txfacs[i].lsb+1)*(txfacs[i].array_height);
				facindx=txfacs[i].facindx;
				for(j=0;j<len;j++)
					{
					printf("%c%s\n", rv, vcdid(facindx+j));
					}
				}
			break;

		case 0xb0:		
		case 0xb1:		
		case 0xb2:		
		case 0xb3:		
			n=get_short();
			/* printf("[%02x] Change %d FACS to %c\n", tag, "01xz"[tag&3]); */
			for(i=0;i<n;i++)
				{
				v=get_short();
				printf("%c%s\n", "01xz"[tag&3], vcdid(v+offset));
				}
			break;

		case 0xb4:
			/* printf("[%02x] Stop tag reached\n", tag); */
			goto fini;
		
		case 0xb5:					/* 24-bit explicit variants, no offset */
		case 0xb6:
		case 0xb7:
		case 0xb8:
			v=get_24();
			/* printf("[%02x] Change FAC %d to %c\n", tag, "01xz"[(tag-1)&3]); */
			printf("%c%s\n", "01xz"[(tag-1)&3], vcdid(v));

		case EOF:
			/* printf("File end reached\n"); */
			goto fini;			

		default:
			fprintf(stderr, "[%02x] Unknown tag, exiting.\n", tag);
			exit(0);
		}
	}

fini:
fclose(handle);
exit(0);
}
