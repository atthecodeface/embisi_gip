/* 
 * Copyright (c) Tony Bybell 2001.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include <stdio.h>

#ifndef _MSC_VER
#include <unistd.h>
#include <sys/mman.h>
#else
#include <windows.h>
#endif


#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "aet.h"
#include "vcd.h"
#include "lxt.h"
#include "bsearch.h"

struct symbol **sym=NULL;
struct symbol **facs=NULL;
char facs_are_sorted=0;

int numfacs=0;
int regions=0;
int longestname=0;

struct symbol *firstnode=NULL;	/* 1st sym in aet */
struct symbol *curnode=NULL;	/* current loaded sym in aet loader */


/*
 * Generic hash function for symbol names...
 */
int hashcache;
int hash(char *s)
{
char *p;
char ch;
unsigned int h=0, h2=0, pos=0, g;
for(p=s;*p;p++)
        {
	ch=*p;
	h2<<=3;
	h2-=((unsigned int)ch+(pos++));		/* this handles stranded vectors quite well.. */

        h=(h<<4)+ch;
        if((g=h&0xf0000000))
                {
                h=h^(g>>24);
                h=h^g;
                }   
        }

h^=h2;						/* combine the two hashes */
hashcache=h%SYMPRIME;
return(hashcache);
}


/*
 * add symbol to table.  no duplicate checking
 * is necessary as aet's are "correct."
 */
struct symbol *symadd(char *name, int hv)
{
struct symbol *s;

s=(struct symbol *)calloc_2(1,sizeof(struct symbol));
strcpy(s->name=(char *)malloc_2(strlen(name)+1),name);
s->next=sym[hv];
sym[hv]=s;
return(s);
}


/*
 * find a slot already in the table...
 */
struct symbol *symfind(char *s)
{
int hv;
struct symbol *temp;

if(!facs_are_sorted)
	{
	hv=hash(s);
	if(!(temp=sym[hv])) return(NULL); /* no hash entry, add here wanted to add */
	
	while(temp)
	        {
	        if(!strcmp(temp->name,s))
	                {
	                return(temp); /* in table already */    
	                }
	        if(!temp->next) break;
	        temp=temp->next;
	        }
	
	return(NULL); /* not found, add here if you want to add*/
	}
	else	/* no sense hashing if the facs table is built */
	{	
	DEBUG(printf("BSEARCH: %s\n",s));
	return(bsearch_facs(s));
	}
}
