/* 
 * Copyright (c) Tony Bybell 1999.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include "fgetdynamic.h"
#include "debug.h"

int fgetmalloc_len;

char *fgetmalloc(FILE *handle)
{
char *pnt, *pnt2;
struct alloc_bytechain *bytechain_root=NULL, *bytechain_current=NULL;
int ch;

fgetmalloc_len=0;

for(;;)
	{
	ch=fgetc(handle);
	if((ch==EOF)||(ch==0x00)||(ch=='\n')||(ch=='\r')) break;
	fgetmalloc_len++;
	if(bytechain_current)
		{
		bytechain_current->next=wave_alloca(sizeof(struct alloc_bytechain));
		bytechain_current=bytechain_current->next;
		bytechain_current->val=(char)ch;
		bytechain_current->next=NULL;
		}
		else
		{
		bytechain_root=bytechain_current=wave_alloca(sizeof(struct alloc_bytechain));
		bytechain_current->val=(char)ch;
		bytechain_current->next=NULL;
		}
	}

if(!fgetmalloc_len) 
	{
	return(NULL);
	}
	else
	{
	pnt=pnt2=(char *)malloc_2(fgetmalloc_len+1);
	while(bytechain_root)
		{
		*(pnt2++)=bytechain_root->val;
		bytechain_root=bytechain_root->next;
		}
	*(pnt2)=0;

	return(pnt);
	}


}
