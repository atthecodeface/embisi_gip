/* 
 * Copyright (c) Tony Bybell 1999.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include "gtk12compat.h"
#include "currenttime.h"
#include "pixmaps.h"
#include "aet.h"
#include "debug.h"

GtkWidget *signalarea=NULL;
GdkFont   *signalfont=NULL;
GdkPixmap *signalpixmap=NULL;
int max_signal_name_pixel_width = 0;
int signal_pixmap_width = 0;
int signal_fill_width = 0;
int old_signal_fill_width=0, old_signal_fill_height=0;
int fontheight = 1;		/* initial dummy value */

char dnd_state=0;		/* for button 3 */

static GtkWidget *hscroll=NULL;
GtkObject *signal_hslider=NULL;

static Ulong cachedhiflag=0;
static int cachedwhich=-1;	/* if -1, it's invalid, else */
				/* use as a fill base */

Trptr cachedtrace=NULL;
Trptr shift_click_trace=NULL;

/*
 * complain about certain ops conflict with dnd...
 */
void dnd_error(void)
{ 
status_text("Can't perform that operation when waveform drag and drop is in progress!\n");
}  


static void     
service_hslider(GtkWidget *text, gpointer data)
{
GtkAdjustment *hadj;
gint xsrc;

if(signalpixmap)
	{
	hadj=GTK_ADJUSTMENT(signal_hslider);
	xsrc=(gint)hadj->value;
	DEBUG(printf("Signal HSlider Moved to %d\n",xsrc));

	gdk_draw_rectangle(signalpixmap, signalarea->style->bg_gc[GTK_STATE_ACTIVE], TRUE,
	        0, -1, signal_fill_width, fontheight);
	gdk_draw_line(signalpixmap, signalarea->style->white_gc,  
	        0, fontheight-1, signal_fill_width-1, fontheight-1);
	gdk_draw_string(signalpixmap, signalfont,
	        signalarea->style->fg_gc[0], 3+xsrc, fontheight-4, "Time");

	gdk_draw_pixmap(signalarea->window, signalarea->style->fg_gc[GTK_WIDGET_STATE(signalarea)],
                signalpixmap,
		xsrc, 0,
		0, 0,
                signalarea->allocation.width, signalarea->allocation.height);
	}
}


static gint motion_notify_event(GtkWidget *widget, GdkEventMotion *event)
{
int num_traces_displayable;
int which;
int trwhich, trtarget;
int new_cachedwhich;
GtkAdjustment *wadj;
Trptr t;
gdouble x,y;
GdkModifierType state;
int doloop;
gint xi, yi;

if(event->is_hint)
	{
	WAVE_GDK_GET_POINTER(event->window, &x, &y, &xi, &yi, &state);
	WAVE_GDK_GET_POINTER_COPY;
	}
	else
	{
	x = event->x;
	y = event->y;
	state = event->state;
	}

top: doloop=0;

/********************* button 1 *********************/

if((traces.total)&&(state&(GDK_BUTTON1_MASK|GDK_BUTTON3_MASK))&&(signalpixmap)&&(cachedwhich!=-1))
	{
	num_traces_displayable=widget->allocation.height/(fontheight);
	num_traces_displayable--;   /* for the time trace that is always there */

	which=(int)(y);
	which=(which/fontheight)-1;

	if(which>=traces.total)
		{
		which=traces.total-1;
		}


	wadj=GTK_ADJUSTMENT(wave_vslider);

	if(num_traces_displayable<traces.total)
		{
		if(which>=num_traces_displayable)
			{
			int target;

			target=((int)wadj->value)+1;
			which=num_traces_displayable-1;

			if(target+which>=(traces.total-1)) target=traces.total-which-1;
			wadj->value=target;

			if(cachedwhich==which) cachedwhich=which-1; /* force update */

			gtk_signal_emit_by_name (GTK_OBJECT (wadj), "changed");	/* force bar update */
			gtk_signal_emit_by_name (GTK_OBJECT (wadj), "value_changed"); /* force text update */
	
			doloop=1;
			}
			else
			if (which<0)
			{
			int target;
	
			target=((int)wadj->value)-1;
			if(target<0) target=0;
			wadj->value=target;
	
			which=0;
			if(cachedwhich==which) cachedwhich=-1; /* force update */
	
			gtk_signal_emit_by_name (GTK_OBJECT (wadj), "changed");	/* force bar update */
			gtk_signal_emit_by_name (GTK_OBJECT (wadj), "value_changed"); /* force text update */
	
			doloop=1;
			}
		}
		else
		{
		if(which<0) which=0;
		}

	trtarget=((int)wadj->value)+which;

	cachedtrace=t=traces.first;
	trwhich=0;
	while(t)
	        {
	        if((trwhich<trtarget)&&(t->next))
	                {
	                trwhich++;
	                t=t->next;
	                }
	                else
	                {
	                break;
	                }
	        }

	cachedtrace=t;

	if((dnd_state==1)&&(state&GDK_BUTTON3_MASK))
		{
		GtkAdjustment *hadj;
		gint xsrc;
		int yval;

		hadj=GTK_ADJUSTMENT(signal_hslider);
		wadj=GTK_ADJUSTMENT(wave_vslider);
		gtk_signal_emit_by_name (GTK_OBJECT (wadj), "changed");	/* force bar update */
		gtk_signal_emit_by_name (GTK_OBJECT (wadj), "value_changed"); /* force text update */

		xsrc=(gint)hadj->value;

		yval=RenderSig(t, which, 2);
        	gdk_draw_pixmap(signalarea->window, signalarea->style->fg_gc[GTK_WIDGET_STATE(signalarea)],
                	signalpixmap,
                	xsrc, yval,
                	0, yval,
                	signalarea->allocation.width, fontheight-1);

		}
	else
	if((state&GDK_BUTTON1_MASK)&&(dnd_state==0))
	if((t)&&(which!=cachedwhich))
		{
		GtkAdjustment *hadj;
		gint xsrc;

		hadj=GTK_ADJUSTMENT(signal_hslider);
		xsrc=(gint)hadj->value;

		new_cachedwhich=which;	/* save so fill ins go from deltas in the future */
		
		do {
		int oldflags;

		oldflags = t->flags;

		t->flags = (t->flags & (~TR_HIGHLIGHT)) | cachedhiflag;

		if(oldflags!=t->flags)
			{
			int yval;
			DEBUG(printf("Motion highlight swap in signalarea at x: %d, y: %d row: %d\n",
				(int)x, (int)y, which));

			yval=RenderSig(t, which, 1);
        		gdk_draw_pixmap(signalarea->window, signalarea->style->fg_gc[GTK_WIDGET_STATE(signalarea)],
                		signalpixmap,
                		xsrc, yval,
                		0, yval,
                		signalarea->allocation.width, fontheight-1);
			}

		if(which>cachedwhich)
			{
			which--;
			t=t->prev;
			}
		else if(which<cachedwhich)
			{
			which++;
			t=t->next;
			}

		} while((which!=cachedwhich)&&(t));

		cachedwhich=new_cachedwhich;	/* for next time around */
		}
	}

if(doloop)
	{
	WAVE_GDK_GET_POINTER(event->window, &x, &y, &xi, &yi, &state);
	WAVE_GDK_GET_POINTER_COPY;
	goto top;
	}

return(TRUE);
}


static gint button_release_event(GtkWidget *widget, GdkEventButton *event)
{
int which;

if(event->button==1)
	{
	if(dnd_state==0) cachedwhich=-1;
	gdk_pointer_ungrab(event->time);
	DEBUG(printf("Button 1 released\n"));
	}

/********************* button 3 *********************/

if((event->button==3)&&(signalpixmap))
	{
	cachedwhich=-1;
	gdk_pointer_ungrab(event->time);
	DEBUG(printf("Button 3 released\n"));

	if(dnd_state==1)
		{
		if(cachedtrace)
			{
			cachedtrace->flags|=TR_HIGHLIGHT;
			}

		which=(int)(event->y);
		which=(which/fontheight)-1;
	
		if( ((which<0) && (topmost_trace==traces.first) && PrependBuffer()) || (PasteBuffer()) ) /* short circuit on special which<0 case */
	       		{
			status_text("Drop completed.\n");

        		MaxSignalLength();
        		signalarea_configure_event(signalarea, NULL);
        		wavearea_configure_event(wavearea, NULL);
        		}
		dnd_state=0;
		}
	}

/********************* button 3 *********************/

return(TRUE);
}

static gint button_press_event(GtkWidget *widget, GdkEventButton *event)
{
int num_traces_displayable;
int which;
int trwhich, trtarget;
GtkAdjustment *wadj;
Trptr t;

if((traces.total)&&(signalpixmap))
	{
	gdk_pointer_grab(widget->window, FALSE,
		GDK_POINTER_MOTION_HINT_MASK | GDK_BUTTON1_MOTION_MASK | GDK_BUTTON3_MOTION_MASK | 
		GDK_BUTTON_RELEASE_MASK, NULL, NULL, event->time);

	num_traces_displayable=widget->allocation.height/(fontheight);
	num_traces_displayable--;   /* for the time trace that is always there */

	which=(int)(event->y);
	which=(which/fontheight)-1;

	if((which>=traces.total)||(which>=num_traces_displayable)||(which<0))
		{
		if(dnd_state==0)cachedwhich=-1;
		goto check_button_3;	/* off in no man's land, but check 3rd anyways.. */
		}

	cachedwhich=which;	/* cache for later fill in */
	wadj=GTK_ADJUSTMENT(wave_vslider);
	trtarget=((int)wadj->value)+which;

	t=traces.first;
	trwhich=0;
	while(t)
	        {
	        if((trwhich<trtarget)&&(t->next))
	                {
	                trwhich++;
	                t=t->next;
	                }
	                else
	                {
	                break;
	                }
	        }

	cachedtrace=t;
	if((dnd_state==0)&&(event->button==1))
	if(t)
		{
		int yval;
		GtkAdjustment *hadj;
		gint xsrc;

		if((shift_click_trace)&&(event->state&GDK_SHIFT_MASK))
			{
			Trptr t2;
			unsigned int f;

			t2=shift_click_trace;
			while(t2)
				{
				if(t2==t)
					{
					t2=shift_click_trace;
					f=t2->flags&TR_HIGHLIGHT;
					while(t2)
						{
						t2->flags = (t2->flags & (~TR_HIGHLIGHT)) | f;
						if(t2==t) break;
						t2=t2->prev;
						}					
					goto resync_signalarea;
					}
				t2=t2->prev;
				}

			t2=shift_click_trace;
			while(t2)
				{
				if(t2==t)
					{
					t2=shift_click_trace;
					f=t2->flags&TR_HIGHLIGHT;
					while(t2)
						{
						t2->flags = (t2->flags & (~TR_HIGHLIGHT)) | f;
						if(t2==t) break;
						t2=t2->next;
						}					
					goto resync_signalarea;
					}
				t2=t2->next;
				}

			goto normal_button1_press;	/* couldn't find original so make this original... */
			
			resync_signalarea:
        		MaxSignalLength();
        		signalarea_configure_event(signalarea, NULL);
			DEBUG(printf("Shift-Click in signalarea!\n"));
			return(TRUE);
			}
			else
			{
			normal_button1_press:
			hadj=GTK_ADJUSTMENT(signal_hslider);
			xsrc=(gint)hadj->value;

			shift_click_trace=t;
			t->flags ^= TR_HIGHLIGHT;
			cachedhiflag = t->flags & TR_HIGHLIGHT;

			DEBUG(printf("Button pressed in signalarea at x: %d, y: %d row: %d\n",
				(int)event->x, (int)event->y, which));

			yval=RenderSig(t, which, 1);
	        	gdk_draw_pixmap(signalarea->window, signalarea->style->fg_gc[GTK_WIDGET_STATE(signalarea)],
	                	signalpixmap,
	                	xsrc, yval,
	                	0, yval,
	                	signalarea->allocation.width, fontheight-1);
			}
		}

check_button_3:
	if(event->button==3)
		{
		if(dnd_state==0)
			{
			if(CutBuffer())
	        		{
				char buf[32];

				sprintf(buf,"Dragging %d trace%s.\n",traces.buffercount,traces.buffercount!=1?"s":"");
				status_text(buf);
	        		MaxSignalLength();
	        		signalarea_configure_event(signalarea, NULL);
	        		wavearea_configure_event(wavearea, NULL);
				dnd_state=1;
				}
			}
		}
	}

return(TRUE);
}

gint signalarea_configure_event(GtkWidget *widget, GdkEventConfigure *event)
{
static int trtarget=0;
GtkAdjustment *wadj, *hadj;
int num_traces_displayable;
int width;

num_traces_displayable=widget->allocation.height/(fontheight);
num_traces_displayable--;   /* for the time trace that is always there */

DEBUG(printf("SigWin Configure Event h: %d, w: %d\n",
		widget->allocation.height,
		widget->allocation.width));

old_signal_fill_width=signal_fill_width;
signal_fill_width = ((width=widget->allocation.width) > signal_pixmap_width)
        ? widget->allocation.width : signal_pixmap_width;

if(signalpixmap)
	{
	if((old_signal_fill_width!=signal_fill_width)||(old_signal_fill_height!=widget->allocation.height))
		{
		gdk_pixmap_unref(signalpixmap);
		signalpixmap=gdk_pixmap_new(widget->window, 
			signal_fill_width, widget->allocation.height, -1);
		}
	}
	else
	{
	signalpixmap=gdk_pixmap_new(widget->window, 
		signal_fill_width, widget->allocation.height, -1);
	}

old_signal_fill_height= widget->allocation.height;
gdk_draw_rectangle(signalpixmap, widget->style->bg_gc[GTK_STATE_PRELIGHT], TRUE, 0, 0,
			signal_fill_width, widget->allocation.height);

hadj=GTK_ADJUSTMENT(signal_hslider);
hadj->page_size=hadj->page_increment=(gfloat)width;
hadj->step_increment=(gfloat)10.0;  /* approx 1ch at a time */
hadj->lower=(gfloat)0.0;
hadj->upper=(gfloat)signal_pixmap_width;

if( ((int)hadj->value)+width > signal_fill_width)
	{
	hadj->value = (gfloat)(signal_fill_width-width);
	}


wadj=GTK_ADJUSTMENT(wave_vslider);
wadj->page_size=wadj->page_increment=(gfloat) num_traces_displayable;
wadj->step_increment=(gfloat)1.0;
wadj->lower=(gfloat)0.0;
wadj->upper=(gfloat)traces.total;

if(num_traces_displayable>traces.total)
	{
	wadj->value=(gfloat)(trtarget=0);
	}
	else
	if (wadj->value + num_traces_displayable > traces.total)
	{
	wadj->value=(gfloat)(trtarget=traces.total-num_traces_displayable);
	}

gtk_signal_emit_by_name (GTK_OBJECT (wadj), "changed");	/* force bar update */
gtk_signal_emit_by_name (GTK_OBJECT (wadj), "value_changed"); /* force text update */

gtk_signal_emit_by_name (GTK_OBJECT (hadj), "changed");	/* force bar update */

return(TRUE);
}

static gint expose_event(GtkWidget *widget, GdkEventExpose *event)
{
GtkAdjustment *hadj;
int xsrc;

hadj=GTK_ADJUSTMENT(signal_hslider);
xsrc=(gint)hadj->value;

gdk_draw_pixmap(widget->window, widget->style->fg_gc[GTK_WIDGET_STATE(widget)],
		signalpixmap, 
		xsrc+event->area.x, event->area.y,
		event->area.x, event->area.y,
		event->area.width, event->area.height);

return(FALSE);
}

GtkWidget *
create_signalwindow(void)
{
GtkWidget *table;
GtkWidget *frame;

table = gtk_table_new(10, 10, FALSE);

signalarea=gtk_drawing_area_new();

gtk_widget_show(signalarea);
MaxSignalLength();

gtk_widget_set_events(signalarea, 
		GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK | 
		GDK_BUTTON_RELEASE_MASK | 
		GDK_POINTER_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK
		);

gtk_signal_connect(GTK_OBJECT(signalarea), "configure_event",
                        GTK_SIGNAL_FUNC(signalarea_configure_event), NULL);
gtk_signal_connect(GTK_OBJECT(signalarea), "expose_event",
                        GTK_SIGNAL_FUNC(expose_event), NULL);
gtk_signal_connect(GTK_OBJECT(signalarea), "button_press_event",
                        GTK_SIGNAL_FUNC(button_press_event), NULL);
gtk_signal_connect(GTK_OBJECT(signalarea), "button_release_event",
                        GTK_SIGNAL_FUNC(button_release_event), NULL);
gtk_signal_connect(GTK_OBJECT(signalarea), "motion_notify_event",
                        GTK_SIGNAL_FUNC(motion_notify_event), NULL);

gtk_table_attach (GTK_TABLE (table), signalarea, 0, 10, 0, 9,
                        GTK_FILL | GTK_EXPAND,
                        GTK_FILL | GTK_EXPAND | GTK_SHRINK, 3, 3);

signal_hslider=gtk_adjustment_new(0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
gtk_signal_connect(GTK_OBJECT(signal_hslider), "value_changed",
                        GTK_SIGNAL_FUNC(service_hslider), NULL);
hscroll=gtk_hscrollbar_new(GTK_ADJUSTMENT(signal_hslider));
gtk_widget_show(hscroll);
gtk_table_attach (GTK_TABLE (table), hscroll, 0, 10, 9, 10,
                        GTK_FILL,
                        GTK_FILL | GTK_SHRINK, 3, 3);
gtk_widget_show(table);

frame=gtk_frame_new("Signals");
gtk_container_border_width(GTK_CONTAINER(frame),2);

gtk_container_add(GTK_CONTAINER(frame),table);

return(frame);
}
   
