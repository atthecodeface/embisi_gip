/* 
 * Copyright (c) Tony Bybell 1999.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include <gtk/gtk.h>
#include "currenttime.h"
#include "aet.h"

char is_vcd=0;

char use_maxtime_display=1;
static GtkWidget *max_or_marker_label=NULL;
   
TimeType currenttime=0;
TimeType max_time=0;
TimeType min_time=-1;
char display_grid=~0;       /* default to displaying grid */
char time_dimension='n';
static char *time_prefix=" munpf";
static GtkWidget *maxtimewid;
static GtkWidget *curtimewid;
static char *maxtext;
static char *curtext;
static char *maxtime_label_text="Maximum Time";
static char *marker_label_text ="Marker Time";


void update_maxmarker_labels(void)
{
if(use_maxtime_display) 
	{
	gtk_label_set(GTK_LABEL(max_or_marker_label),maxtime_label_text);
	update_maxtime(max_time);
	}
	else
	{
	gtk_label_set(GTK_LABEL(max_or_marker_label),marker_label_text);
	update_markertime(tims.marker);
	}
}

TimeType unformat_time(char *buf, char dim)
{
TimeType rval;
char *pnt, *offs=NULL, *doffs;
char ch;
int i, ich, delta;

rval=atoi_64(buf);
if((pnt=atoi_cont_ptr))
	{
	while((ch=*(pnt++)))
		{
		if((ch==' ')||(ch=='\t')) continue;

		ich=tolower((int)ch);		
		if(ich=='s') ich=' ';	/* as in plain vanilla seconds */

		offs=strchr(time_prefix, ich);
		break;
		}
	}

if(!offs) return(rval);
doffs=strchr(time_prefix, (int)dim);
if(!doffs) return(rval); /* should *never* happen */

delta= (doffs-time_prefix) - (offs-time_prefix);
if(delta<0)
	{
	for(i=delta;i<0;i++)
		{
		rval=rval/1000;
		}
	}
	else
	{
	for(i=0;i<delta;i++)
		{
		rval=rval*1000;
		}
	}

return(rval);
}

void reformat_time(char *buf, TimeType val, char dim)
{
char *pnt;
int i, offset;

pnt=strchr(time_prefix, (int)dim);
if(pnt) { offset=pnt-time_prefix; } else offset=0;

for(i=offset; i>0; i--)
	{
	if(val%1000) break;
	val=val/1000;
	}

if(i)
	{
	sprintf(buf, TTFormat" %cs", val, time_prefix[i]);
	}
	else
	{
	sprintf(buf, TTFormat" sec", val);
	}
}


void reformat_time_blackout(char *buf, TimeType val, char dim)
{
char *pnt;
int i, offset;
struct blackout_region_t *bt = blackout_regions;
char blackout = ' ';

while(bt)
	{
	if((val>=bt->bstart)&&(val<bt->bend))
		{
		blackout = '*';
		break;
		}

	bt=bt->next;
	}

pnt=strchr(time_prefix, (int)dim);
if(pnt) { offset=pnt-time_prefix; } else offset=0;

for(i=offset; i>0; i--)
	{
	if(val%1000) break;
	val=val/1000;
	}

if(i)
	{
	sprintf(buf, TTFormat"%c%cs", val, blackout, time_prefix[i]);
	}
	else
	{
	sprintf(buf, TTFormat"%csec", val, blackout);
	}
}


void update_markertime(TimeType val)
{
if(!use_maxtime_display)
	{
	if(val>=0)
		{
		if(tims.lmbcache>=0) val-=tims.lmbcache; /* do delta instead */

		if((tims.lmbcache>=0)&&(val>=0))
			{
			*maxtext='+';
			reformat_time(maxtext+1, val, time_dimension);
			}
			else
			{
			reformat_time(maxtext, val, time_dimension);
			}
		}
		else
		{
		sprintf(maxtext, "--");
		}

	gtk_label_set(GTK_LABEL(maxtimewid), maxtext);
	}
}


void update_maxtime(TimeType val)
{
max_time=val;

if(use_maxtime_display)
	{
	reformat_time(maxtext, val, time_dimension);
	gtk_label_set(GTK_LABEL(maxtimewid), maxtext);
	}
}


void update_currenttime(TimeType val)
{
currenttime=val;
reformat_time_blackout(curtext, val, time_dimension);
gtk_label_set(GTK_LABEL(curtimewid), curtext);
}

   
/* Create an entry box */
GtkWidget *
create_time_box(void)
{
GtkWidget *label2;
GtkWidget *mainbox;

max_or_marker_label=(use_maxtime_display)
	? gtk_label_new(maxtime_label_text)
	: gtk_label_new(marker_label_text);

maxtext=(char *)malloc_2(40);
if(use_maxtime_display)
	{
	reformat_time(maxtext, max_time, time_dimension);
	}
	else
	{
	sprintf(maxtext,"--");
	}

maxtimewid=gtk_label_new(maxtext);

label2=gtk_label_new("Current Time");
curtext=(char *)malloc_2(40);
reformat_time(curtext, (currenttime=min_time), time_dimension);

curtimewid=gtk_label_new(curtext);

mainbox=gtk_vbox_new(FALSE, 0);
gtk_box_pack_start(GTK_BOX(mainbox), max_or_marker_label, TRUE, FALSE, 0);
gtk_widget_show(max_or_marker_label);
gtk_box_pack_start(GTK_BOX(mainbox), maxtimewid, TRUE, FALSE, 0);
gtk_widget_show(maxtimewid);
gtk_box_pack_start(GTK_BOX(mainbox), label2, TRUE, FALSE, 0);
gtk_widget_show(label2);
gtk_box_pack_start(GTK_BOX(mainbox), curtimewid, TRUE, FALSE, 0);
gtk_widget_show(curtimewid);
   
return(mainbox);
}
   

static TimeType time_trunc_val=1;
char use_full_precision=0;


TimeType time_trunc(TimeType t)
{
if(!use_full_precision)
if(time_trunc_val!=1)
	{
	t=t/time_trunc_val;
	t=t*time_trunc_val;
	if(t<tims.first) t=tims.first;
	}
 
return(t);
}

void time_trunc_set(void)
{
gdouble gcompar=1e15;
TimeType compar=LLDescriptor(1000000000000000);

for(;compar!=1;compar=compar/10,gcompar=gcompar/((gdouble)10.0))
	{
	if(nspx>=gcompar)
		{
		time_trunc_val=compar;
		return;
		}
        }
 
time_trunc_val=1;
}


