/* 
 * Copyright (c) Tony Bybell 1999-2001.
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
#include "bsearch.h"
#include "color.h"
#include "strace.h"
#include "debug.h"

static void rendertimebar(void);
static void draw_hptr_trace(Trptr t, hptr h, int which, int dodraw);
static void draw_hptr_trace_vector(Trptr t, hptr h, int which);
static void draw_vptr_trace(Trptr t, vptr v, int which);
static void rendertraces(void);
static void rendertimes(void);

static gint m1x, m2x;

char signalwindow_width_dirty=1;	/* prime it up for the 1st one at least.. */
char enable_ghost_marker=1;
char enable_horiz_grid=1;
char enable_vert_grid=1;
char use_big_fonts=0, use_nonprop_fonts=0;
char do_resize_signals=~0;
char constant_marker_update=0;
char use_roundcaps=0;
char show_base=~0;
char wave_scrolling=~0;
int vector_padding=4;
static int in_button_press=0;
char left_justify_sigs=0;
char zoom_pow10_snap=0;

gfloat old_wvalue=-1.0;

struct blackout_region_t *blackout_regions = NULL;
TimeType zoom=0, scale=1, nsperframe=1, pixelsperframe=1, hashstep=1;
static TimeType prevtim=-1;

gdouble pxns=1.0, nspx=1.0;
gdouble zoombase=2.0;

Trptr topmost_trace=NULL;
int waveheight=1, wavecrosspiece;
int wavewidth=1;
GdkFont *wavefont=NULL;
GdkFont *wavefont_smaller=NULL;

GtkWidget *wavearea=NULL;
static GtkWidget *vscroll=NULL;
static GtkWidget *hscroll=NULL;
static GdkPixmap *wavepixmap = NULL;

GtkObject *wave_vslider=NULL, *wave_hslider=NULL;
TimeType named_markers[26];

static char made_gc_contexts=0;

static GdkGC    *gc_back  =NULL;
static GdkGC    *gc_grid  =NULL;
static GdkGC    *gc_time  =NULL;
static GdkGC    *gc_timeb =NULL;
static GdkGC	*gc_value =NULL;                 
static GdkGC	*gc_low   =NULL;
static GdkGC    *gc_high  =NULL;
static GdkGC    *gc_trans =NULL;
static GdkGC    *gc_mid   =NULL;
static GdkGC    *gc_xfill =NULL;
static GdkGC    *gc_x     =NULL;
static GdkGC    *gc_vbox  =NULL;
static GdkGC    *gc_vtrans=NULL;
static GdkGC    *gc_mark  =NULL;
static GdkGC    *gc_umark =NULL;

static const GdkModifierType   bmask[4]= {0, GDK_BUTTON1_MASK, 0, GDK_BUTTON3_MASK };		        /* button 1, 3 press/rel encodings */
static const GdkModifierType m_bmask[4]= {0, GDK_BUTTON1_MOTION_MASK, 0, GDK_BUTTON3_MOTION_MASK };	/* button 1, 3 motion encodings */


static void draw_named_markers(void)
{
gdouble pixstep;
gint xl, y;
int i;
TimeType t;

pixstep=((gdouble)nsperframe)/((gdouble)pixelsperframe);

for(i=0;i<26;i++)
{
if((t=named_markers[i])!=-1)
	{
	if((t>=tims.start)&&(t<=tims.last)
		&&(t<=tims.end))
		{
		xl=((gdouble)(t-tims.start))/pixstep;     /* snap to integer */
		if((xl>=0)&&(xl<wavewidth))
			{
			char nbuff[2];
			nbuff[0]='A'+i; nbuff[1]=0x00;

			for(y=fontheight-1;y<=waveheight-1;y+=8)
				{
				gdk_draw_line(wavepixmap,
					gc_mark,
        	        		xl, y, xl, y+5);
				}

			gdk_draw_string(wavepixmap, wavefont_smaller,
				gc_mark,
				xl-(gdk_string_measure(wavefont_smaller, nbuff)>>1), 
				fontheight-2, nbuff);
			}
		}
	}
}
}


static void sync_marker(void)
{
if((tims.prevmarker==-1)&&(tims.marker!=-1))
	{
	signalwindow_width_dirty=1;
	}
	else
if((tims.marker==-1)&&(tims.prevmarker!=-1))
	{
	signalwindow_width_dirty=1;
	}
tims.prevmarker=tims.marker;
}


static void draw_marker(void)
{
gdouble pixstep;
gint xl;

m1x=m2x=-1;

if(tims.marker>=0)
	{
	if((tims.marker>=tims.start)&&(tims.marker<=tims.last)
		&&(tims.marker<=tims.end))
		{
		pixstep=((gdouble)nsperframe)/((gdouble)pixelsperframe);
		xl=((gdouble)(tims.marker-tims.start))/pixstep;     /* snap to integer */
		if((xl>=0)&&(xl<wavewidth))
			{
			gdk_draw_line(wavearea->window,
				gc_umark,
                		xl, fontheight-1, xl, waveheight-1);
			m1x=xl;
			}
		}
	}

if((enable_ghost_marker)&&(in_button_press)&&(tims.lmbcache>=0))
	{
	if((tims.lmbcache>=tims.start)&&(tims.lmbcache<=tims.last)
		&&(tims.lmbcache<=tims.end))
		{
		pixstep=((gdouble)nsperframe)/((gdouble)pixelsperframe);
		xl=((gdouble)(tims.lmbcache-tims.start))/pixstep;     /* snap to integer */
		if((xl>=0)&&(xl<wavewidth))
			{
			gdk_draw_line(wavearea->window,
				gc_umark,
                		xl, fontheight-1, xl, waveheight-1);
			m2x=xl;
			}
		}
	}

if(m1x>m2x)		/* ensure m1x <= m2x for partitioned refresh */
	{
	gint t;

	t=m1x;
	m1x=m2x;
	m2x=t;
	}

if(m1x==-1) m1x=m2x;	/* make both markers same if no ghost marker or v.v. */
}


static void draw_marker_partitions(void)
{
draw_marker();

if(m1x==m2x)
	{
	gdk_draw_pixmap(wavearea->window, wavearea->style->fg_gc[GTK_WIDGET_STATE(wavearea)],
	wavepixmap, m1x, 0, m1x, 0, 1, fontheight-2);

	if(m1x<0)
		{
		gdk_draw_pixmap(wavearea->window, wavearea->style->fg_gc[GTK_WIDGET_STATE(wavearea)],
		wavepixmap, 0, 0, 0, 0, wavewidth, waveheight);
		}
		else
		{
		if(m1x==0)
			{
			gdk_draw_pixmap(wavearea->window, wavearea->style->fg_gc[GTK_WIDGET_STATE(wavearea)],
			wavepixmap, 1, 0, 1, 0, wavewidth-1, waveheight);
			}
		else
		if(m1x==wavewidth-1)
			{

			gdk_draw_pixmap(wavearea->window, wavearea->style->fg_gc[GTK_WIDGET_STATE(wavearea)],
			wavepixmap, 0, 0, 0, 0, wavewidth-1, waveheight);
			}
		else
			{
			gdk_draw_pixmap(wavearea->window, wavearea->style->fg_gc[GTK_WIDGET_STATE(wavearea)],
			wavepixmap, 0, 0, 0, 0, m1x, waveheight);
			gdk_draw_pixmap(wavearea->window, wavearea->style->fg_gc[GTK_WIDGET_STATE(wavearea)],
			wavepixmap, m1x+1, 0, m1x+1, 0, wavewidth-m1x-1, waveheight);
			}
		}
	}
	else
	{
	gdk_draw_pixmap(wavearea->window, wavearea->style->fg_gc[GTK_WIDGET_STATE(wavearea)],
	wavepixmap, m1x, 0, m1x, 0, 1, fontheight-2);
	gdk_draw_pixmap(wavearea->window, wavearea->style->fg_gc[GTK_WIDGET_STATE(wavearea)],
	wavepixmap, m2x, 0, m2x, 0, 1, fontheight-2);

	if(m1x>0)
		{
		gdk_draw_pixmap(wavearea->window, wavearea->style->fg_gc[GTK_WIDGET_STATE(wavearea)],
		wavepixmap, 0, 0, 0, 0, m1x, waveheight);
		}

	if(m2x-m1x>1)
		{
		gdk_draw_pixmap(wavearea->window, wavearea->style->fg_gc[GTK_WIDGET_STATE(wavearea)],
		wavepixmap, m1x+1, 0, m1x+1, 0, m2x-m1x-1, waveheight);
		}

	if(m2x!=wavewidth-1)
		{
		gdk_draw_pixmap(wavearea->window, wavearea->style->fg_gc[GTK_WIDGET_STATE(wavearea)],
		wavepixmap, m2x+1, 0, m2x+1, 0, wavewidth-m2x-1, waveheight);
		}
	}
}

static void renderblackout(void)
{
gfloat pageinc;
TimeType lhs, rhs, lclip, rclip;
struct blackout_region_t *bt = blackout_regions;

if(bt)
	{
	pageinc=(gfloat)(((gdouble)wavewidth)*nspx);
	lhs = tims.start;
	rhs = pageinc + lhs;

	while(bt)
		{
		if( ((bt->bstart <= lhs) && (bt->bend >= lhs)) || ((bt->bstart >= lhs) && (bt->bstart <= rhs)) )
			{
			lclip = bt->bstart; rclip = bt->bend;

			if(lclip < lhs) lclip = lhs;
				else if (lclip > rhs) lclip = rhs;

			if(rclip < lhs) rclip = lhs;

			lclip -= lhs;
			rclip -= lhs;
			if(rclip>(wavewidth+1)) rclip = wavewidth+1;
			
			gdk_draw_rectangle(wavepixmap, gc_xfill, TRUE, 
				(((gdouble)lclip)*pxns), fontheight,
				(((gdouble)(rclip-lclip))*pxns), waveheight-fontheight);
			}

		bt=bt->next;
		}
	}
}

static void     
service_hslider(GtkWidget *text, gpointer data)
{
DEBUG(printf("Wave HSlider Moved\n"));

if((wavepixmap)&&(wavewidth>1))
	{
	GtkAdjustment *hadj;

	hadj=GTK_ADJUSTMENT(wave_hslider);

	if(!tims.timecache)
		{
		tims.start=time_trunc(hadj->value);
		}
		else
		{
		tims.start=time_trunc(tims.timecache);
		tims.timecache=0;	/* reset */
		}

	if(tims.start<tims.first) tims.start=tims.first;
		else if(tims.start>tims.last) tims.start=tims.last;

	tims.laststart=tims.start;

	gdk_draw_rectangle(wavepixmap, gc_back, TRUE, 0, 0,
		wavewidth, waveheight);
	rendertimebar();
	}
}

static void     
service_vslider(GtkWidget *text, gpointer data)
{
GtkAdjustment *sadj, *hadj;
int trtarget;
int xsrc;

if(signalpixmap)
	{
	hadj=GTK_ADJUSTMENT(signal_hslider);
	sadj=GTK_ADJUSTMENT(wave_vslider);
	xsrc=(gint)hadj->value;

	trtarget=(int)(sadj->value);
	DEBUG(printf("Wave VSlider Moved to %d\n",trtarget));

		gdk_draw_rectangle(signalpixmap, 
			signalarea->style->bg_gc[GTK_STATE_PRELIGHT], TRUE, 0, 0,
	            	signal_fill_width, signalarea->allocation.height);
	
		sync_marker();
		RenderSigs(trtarget,(old_wvalue==sadj->value)?0:1);

	old_wvalue=sadj->value;

		draw_named_markers();
		gdk_draw_pixmap(signalarea->window, signalarea->style->fg_gc[GTK_WIDGET_STATE(signalarea)],
	                signalpixmap,
			xsrc, 0,
			0, 0,
	                signalarea->allocation.width, signalarea->allocation.height);
		draw_marker();
	}
}

static void button_press_release_common(void)
{
MaxSignalLength();
gdk_draw_rectangle(signalpixmap, 
	signalarea->style->bg_gc[GTK_STATE_PRELIGHT], TRUE, 0, 0,
        signal_fill_width, signalarea->allocation.height);
sync_marker();
RenderSigs((int)(GTK_ADJUSTMENT(wave_vslider)->value),0);
gdk_draw_pixmap(signalarea->window, signalarea->style->fg_gc[GTK_WIDGET_STATE(signalarea)],
      	signalpixmap,
	(gint)(GTK_ADJUSTMENT(signal_hslider)->value), 0,
	0, 0,
        signalarea->allocation.width, signalarea->allocation.height);
}

static void button_motion_common(gint xin, int pressrel) 
{
gdouble x,offset,pixstep;
TimeType newcurr;
if(xin<0) xin=0;
if(xin>(wavewidth-1)) xin=(wavewidth-1);

x=xin;	/* for pix time calc */

pixstep=((gdouble)nsperframe)/((gdouble)pixelsperframe);
newcurr=(TimeType)(offset=((gdouble)tims.start)+(x*pixstep));

if(offset-newcurr>0.5)	/* round to nearest integer ns */
	{
	newcurr++;
	}

if(newcurr>tims.last) 		/* sanity checking */
	{
	newcurr=tims.last;
	}
if(newcurr>tims.end)
	{
	newcurr=tims.end;
	}
if(newcurr<tims.start)
	{
	newcurr=tims.start;
	}

update_markertime(tims.marker=time_trunc(newcurr));
if(tims.lmbcache<0) tims.lmbcache=time_trunc(newcurr);

draw_marker_partitions();

if((pressrel)||(constant_marker_update)) 
	{
	button_press_release_common();
	}
}

static gint motion_notify_event(GtkWidget *widget, GdkEventMotion *event)
{
gdouble x, y, pixstep, offset;
GdkModifierType state;
TimeType newcurr;
int scrolled;
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


do
	{
	scrolled=0;
	if(state&bmask[in_button_press]) /* needed for retargeting in AIX/X11 */
	if(x<0)
		{ 
		if(wave_scrolling)
		if(tims.start>tims.first)
			{
			if(nsperframe<10) 
				{
				tims.start-=nsperframe;
				}
				else
				{
				tims.start-=(nsperframe/10);
				}
			if(tims.start<tims.first) tims.start=tims.first;
			GTK_ADJUSTMENT(wave_hslider)->value=
				tims.marker=time_trunc(tims.timecache=tims.start);

			gtk_signal_emit_by_name (GTK_OBJECT (GTK_ADJUSTMENT(wave_hslider)), "changed");
			gtk_signal_emit_by_name (GTK_OBJECT (GTK_ADJUSTMENT(wave_hslider)), "value_changed");
			scrolled=1;
			}	
		x=0; 
		}
	else
	if(x>wavewidth) 
		{ 
		if(wave_scrolling)
		if(tims.start!=tims.last)
			{
			gfloat pageinc;
	
			pageinc=(gfloat)(((gdouble)wavewidth)*nspx);

			if(nsperframe<10) 
				{
				tims.start+=nsperframe;
				}
				else
				{
				tims.start+=(nsperframe/10);
				}

			if(tims.start>tims.last-pageinc+1) tims.start=time_trunc(tims.last-pageinc+1);
			if(tims.start<tims.first) tims.start=tims.first;

			tims.marker=time_trunc(tims.start+pageinc);
			if(tims.marker>tims.last) tims.marker=tims.last;
	
			GTK_ADJUSTMENT(wave_hslider)->value=tims.timecache=tims.start;

			gtk_signal_emit_by_name (GTK_OBJECT (GTK_ADJUSTMENT(wave_hslider)), "changed");
			gtk_signal_emit_by_name (GTK_OBJECT (GTK_ADJUSTMENT(wave_hslider)), "value_changed");
			scrolled=1;
			}
		x=wavewidth-1; 
		}
	
	pixstep=((gdouble)nsperframe)/((gdouble)pixelsperframe);
	newcurr=tims.start+(offset=x*pixstep);
	if((offset-((int)offset))>0.5)  /* round to nearest integer ns */
	        {
	        newcurr++;
	        }
	
	if(newcurr>tims.last) newcurr=tims.last;
	
	if(newcurr!=prevtim)
		{
		update_currenttime(time_trunc(newcurr));
		prevtim=newcurr;
		}
	
	if(state&bmask[in_button_press])
		{
		button_motion_common(x,0);
		}
	
	if(scrolled)	/* make sure counters up top update.. */
		{
		while(gtk_events_pending())
			{
			gtk_main_iteration();
			}
		}

	WAVE_GDK_GET_POINTER(event->window, &x, &y, &xi, &yi, &state);
	WAVE_GDK_GET_POINTER_COPY;

	} while((scrolled)&&(state&bmask[in_button_press]));

return(TRUE);
}

static gint button_press_event(GtkWidget *widget, GdkEventButton *event)
{
if((event->button==1)||((event->button==3)&&(!in_button_press)))
	{
	in_button_press=event->button;
	
	DEBUG(printf("Button Press Event\n"));
	button_motion_common(event->x,1);
	tims.timecache=tims.start;

	gdk_pointer_grab(widget->window, FALSE,
		m_bmask[in_button_press] | 				/* key up on motion for button pressed ONLY */
		GDK_POINTER_MOTION_HINT_MASK |
	      	GDK_BUTTON_RELEASE_MASK, NULL, NULL, event->time);
	
	}

return(TRUE);
}

static gint button_release_event(GtkWidget *widget, GdkEventButton *event)
{
if((event->button)&&(event->button==in_button_press))
	{
	in_button_press=0;

	DEBUG(printf("Button Release Event\n"));
	button_motion_common(event->x,1);
	tims.timecache=tims.start;

	gdk_pointer_ungrab(event->time);

	if(event->button==3)	/* oh yeah, dragzoooooooom! */
		{
		service_dragzoom(tims.lmbcache, tims.marker);
		}

	tims.lmbcache=-1;
	update_markertime(time_trunc(tims.marker));
	}

tims.timecache=0;
return(TRUE);
}

gint wavearea_configure_event(GtkWidget *widget, GdkEventConfigure *event)
{
DEBUG(printf("WaveWin Configure Event h: %d, w: %d\n",widget->allocation.height,
		widget->allocation.width));

if(wavepixmap)
	{
	if((wavewidth!=widget->allocation.width)||(waveheight!=widget->allocation.height))
		{
		gdk_pixmap_unref(wavepixmap);
		wavepixmap=gdk_pixmap_new(widget->window, wavewidth=widget->allocation.width,
			waveheight=widget->allocation.height, -1);
		}
	old_wvalue=-1.0;
	}
	else
	{
	wavepixmap=gdk_pixmap_new(widget->window, wavewidth=widget->allocation.width,
		waveheight=widget->allocation.height, -1);
	}

if(!made_gc_contexts)
	{
	gc_back   = alloc_color(wavearea, color_back, wavearea->style->white_gc);    
	gc_grid   = alloc_color(wavearea, color_grid, wavearea->style->bg_gc[GTK_STATE_PRELIGHT]);
	gc_time   = alloc_color(wavearea, color_time, wavearea->style->black_gc);
	gc_timeb  = alloc_color(wavearea, color_timeb, wavearea->style->bg_gc[GTK_STATE_ACTIVE]);
	gc_value  = alloc_color(wavearea, color_value, wavearea->style->black_gc);
	gc_low    = alloc_color(wavearea, color_low, wavearea->style->black_gc);    
	gc_high   = alloc_color(wavearea, color_high, wavearea->style->black_gc);    
	gc_trans  = alloc_color(wavearea, color_trans, wavearea->style->black_gc);    
	gc_mid    = alloc_color(wavearea, color_mid, wavearea->style->black_gc);    
	gc_xfill  = alloc_color(wavearea, color_xfill, wavearea->style->bg_gc[GTK_STATE_PRELIGHT]);
	gc_x      = alloc_color(wavearea, color_x, wavearea->style->black_gc);
	gc_vbox   = alloc_color(wavearea, color_vbox, wavearea->style->black_gc);
	gc_vtrans = alloc_color(wavearea, color_vtrans, wavearea->style->black_gc);
	gc_mark   = alloc_color(wavearea, color_mark, wavearea->style->bg_gc[GTK_STATE_SELECTED]);
	gc_umark  = alloc_color(wavearea, color_umark, wavearea->style->bg_gc[GTK_STATE_SELECTED]);

	made_gc_contexts=~0;
	}

if(wavewidth>1)
if(!do_initial_zoom_fit)
	{
	calczoom(tims.zoom);
	fix_wavehadj();
	gtk_signal_emit_by_name (GTK_OBJECT (GTK_ADJUSTMENT(wave_hslider)), "value_changed"); /* force zoom update */ 
	gtk_signal_emit_by_name (GTK_OBJECT (GTK_ADJUSTMENT(wave_hslider)), "changed"); /* force zoom update */
	}
	else
	{
	do_initial_zoom_fit=0;
	service_zoom_fit(NULL,NULL);
	}	

/* tims.timecache=tims.laststart; */
return(TRUE);
}

static gint expose_event(GtkWidget *widget, GdkEventExpose *event)
{
gdk_draw_pixmap(widget->window, widget->style->fg_gc[GTK_WIDGET_STATE(widget)],
		wavepixmap, 
		event->area.x, event->area.y,
		event->area.x, event->area.y,
		event->area.width, event->area.height);
draw_marker();

return(FALSE);
}

GtkWidget *
create_wavewindow(void)
{
GtkWidget *table;
GtkWidget *frame;
GtkAdjustment *hadj, *vadj;

table = gtk_table_new(10, 10, FALSE);
wavearea=gtk_drawing_area_new();
gtk_widget_show(wavearea);

gtk_widget_set_events(wavearea,
                GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK | 
                GDK_BUTTON_RELEASE_MASK |
                GDK_POINTER_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK
                );

gtk_signal_connect(GTK_OBJECT(wavearea), "configure_event",
                        GTK_SIGNAL_FUNC(wavearea_configure_event), NULL);
gtk_signal_connect(GTK_OBJECT(wavearea), "expose_event",
                        GTK_SIGNAL_FUNC(expose_event), NULL);
gtk_signal_connect(GTK_OBJECT(wavearea), "motion_notify_event",
                        GTK_SIGNAL_FUNC(motion_notify_event), NULL);
gtk_signal_connect(GTK_OBJECT(wavearea), "button_press_event",
                        GTK_SIGNAL_FUNC(button_press_event), NULL);
gtk_signal_connect(GTK_OBJECT(wavearea), "button_release_event",
                        GTK_SIGNAL_FUNC(button_release_event), NULL);

gtk_table_attach (GTK_TABLE (table), wavearea, 0, 9, 0, 9,
                        GTK_FILL | GTK_EXPAND,
                        GTK_FILL | GTK_EXPAND | GTK_SHRINK, 3, 3);

wave_vslider=gtk_adjustment_new(0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
vadj=GTK_ADJUSTMENT(wave_vslider);
gtk_signal_connect(GTK_OBJECT(wave_vslider), "value_changed",
                        GTK_SIGNAL_FUNC(service_vslider), NULL);
vscroll=gtk_vscrollbar_new(vadj);
GTK_WIDGET_SET_FLAGS(vscroll, GTK_CAN_FOCUS);
gtk_widget_show(vscroll);
gtk_table_attach (GTK_TABLE (table), vscroll, 9, 10, 0, 9,
                        GTK_FILL,
                        GTK_FILL | GTK_SHRINK, 3, 3);

wave_hslider=gtk_adjustment_new(0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
hadj=GTK_ADJUSTMENT(wave_hslider);
gtk_signal_connect(GTK_OBJECT(wave_hslider), "value_changed",
                        GTK_SIGNAL_FUNC(service_hslider), NULL);
hscroll=gtk_hscrollbar_new(hadj);
GTK_WIDGET_SET_FLAGS(hscroll, GTK_CAN_FOCUS);

gtk_widget_show(hscroll);
gtk_table_attach (GTK_TABLE (table), hscroll, 0, 9, 9, 10,
                        GTK_FILL,
                        GTK_FILL | GTK_SHRINK, 3, 3);
gtk_widget_show(table);

frame=gtk_frame_new("Waves");
gtk_container_border_width(GTK_CONTAINER(frame),2);

gtk_container_add(GTK_CONTAINER(frame),table);
return(frame);
}
   

/**********************************************/

void RenderSigs(int trtarget, int update_waves)
{
Trptr t;
int i, trwhich;
int num_traces_displayable;
GtkAdjustment *hadj;
int xsrc;

hadj=GTK_ADJUSTMENT(signal_hslider);
xsrc=(gint)hadj->value;

num_traces_displayable=signalarea->allocation.height/(fontheight);
num_traces_displayable--;   /* for the time trace that is always there */

gdk_draw_rectangle(signalpixmap, signalarea->style->bg_gc[GTK_STATE_ACTIVE], TRUE, 
		0, -1, signal_fill_width, fontheight); 
gdk_draw_line(signalpixmap, signalarea->style->white_gc, 
		0, fontheight-1, signal_fill_width-1, fontheight-1);
gdk_draw_string(signalpixmap, signalfont, 
	signalarea->style->fg_gc[0], 3+xsrc, fontheight-4, "Time");

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

topmost_trace=t;
if(t)
        {
        for(i=0;(i<num_traces_displayable)&&(t);i++)
                {
		RenderSig(t, i, 0);
                t=t->next;
                }
        }

if((wavepixmap)&&(update_waves))
	{
	gdk_draw_rectangle(wavepixmap, gc_back, TRUE, 0, fontheight-1,
	                        wavewidth, waveheight-fontheight+1);

	if(display_grid) rendertimes();
	rendertraces();
	}
}


int RenderSig(Trptr t, int i, int dobackground)
{
int texty, liney;
int retval;
char buf[65];
int bufxlen;

UpdateSigValue(t); /* in case it's stale on nonprop */
if((t->name)&&(t->shift))
	{
	buf[0]='`';
	reformat_time(buf+1, t->shift, time_dimension);
	strcpy(buf+strlen(buf+1)+1,"\'");
	bufxlen=gdk_string_measure(signalfont, buf);
	}
	else
	{
	bufxlen=0;
	}

liney=((i+2)*fontheight)-2;
texty=liney-(signalfont->descent);

retval=liney-fontheight+1;

if(!(t->flags&TR_HIGHLIGHT)) 
	{
	if(dobackground)	/* for the highlight routines in signalwindow.c */
		{
		if(dobackground==2)
			{
			gdk_draw_rectangle(signalpixmap, signalarea->style->bg_gc[GTK_STATE_NORMAL], TRUE, 
				0, retval,
		            	signal_fill_width, fontheight-1);
			}
			else
			{
			gdk_draw_rectangle(signalpixmap, signalarea->style->bg_gc[GTK_STATE_PRELIGHT], TRUE, 
				0, retval,
		            	signal_fill_width, fontheight-1);
			}
		}

	gdk_draw_line(signalpixmap, 
		signalarea->style->white_gc,
		0, liney,
		signal_fill_width-1, liney);

	if(!(t->flags&TR_BLANK))
		{
		if(t->name)
			{
			if(bufxlen)
				{
				int baselen=gdk_string_measure(signalfont, t->name);
				int combined=baselen+bufxlen;

				gdk_draw_string(signalpixmap,
					signalfont,
				        signalarea->style->fg_gc[0],
				        left_justify_sigs?3:3+max_signal_name_pixel_width-
						combined, 
					texty,
				        t->name);
				gdk_draw_string(signalpixmap,
					signalfont,
				        signalarea->style->fg_gc[0],
				        left_justify_sigs?3+baselen:3+max_signal_name_pixel_width-
						bufxlen, 
					texty,
				        buf);
				}
				else
				{
				gdk_draw_string(signalpixmap,
					signalfont,
				        signalarea->style->fg_gc[0],
				        left_justify_sigs?3:3+max_signal_name_pixel_width-
						gdk_string_measure(signalfont, t->name), 
					texty,
				        t->name);
				}
			}

		if((t->asciivalue)&&(!(t->flags&TR_EXCLUDE)))
			gdk_draw_string(signalpixmap,
				signalfont,
	        		signalarea->style->fg_gc[0],
	        		max_signal_name_pixel_width+6,
				texty,
	        		t->asciivalue);
		}
		else
		{
		if(t->name)
			{
			if(bufxlen)
				{
				int baselen=gdk_string_measure(signalfont, t->name);
				int combined=baselen+bufxlen;

				gdk_draw_string(signalpixmap,
					signalfont,
				        signalarea->style->bg_gc[GTK_STATE_SELECTED],
				        left_justify_sigs?3:3+max_signal_name_pixel_width-
						combined, 
					texty,
				        t->name);
				gdk_draw_string(signalpixmap,
					signalfont,
				        signalarea->style->bg_gc[GTK_STATE_SELECTED],
				        left_justify_sigs?3+baselen:3+max_signal_name_pixel_width-
						bufxlen, 
					texty,
				        buf);
				}
				else
				{
				gdk_draw_string(signalpixmap,
					signalfont,
				        signalarea->style->bg_gc[GTK_STATE_SELECTED],
				        left_justify_sigs?3:3+max_signal_name_pixel_width-
						gdk_string_measure(signalfont, t->name), 
					texty,
				        t->name);
				}
			}
		}
	}
	else
	{
	gdk_draw_rectangle(signalpixmap, signalarea->style->bg_gc[GTK_STATE_SELECTED], TRUE, 
		0, retval,
            	signal_fill_width, fontheight-1);
	gdk_draw_line(signalpixmap, 
		signalarea->style->white_gc,
		0, liney,
		signal_fill_width-1, liney);

	if(!(t->flags&TR_BLANK))
		{
		if(t->name)
			{
			if(bufxlen)
				{
				int baselen=gdk_string_measure(signalfont, t->name);
				int combined=baselen+bufxlen;

				gdk_draw_string(signalpixmap,
					signalfont,
				        signalarea->style->white_gc,
				        left_justify_sigs?3:3+max_signal_name_pixel_width-
						combined, 
					texty,
				        t->name);
				gdk_draw_string(signalpixmap,
					signalfont,
				        signalarea->style->white_gc,
				        left_justify_sigs?3+baselen:3+max_signal_name_pixel_width-
						bufxlen, 
					texty,
				        buf);
				}
				else
				{
				gdk_draw_string(signalpixmap,
					signalfont,
				        signalarea->style->white_gc,
				        left_justify_sigs?3:3+max_signal_name_pixel_width-
						gdk_string_measure(signalfont, t->name), 
					texty,
				        t->name);
				}
			}

		if((t->asciivalue)&&(!(t->flags&TR_EXCLUDE)))
			gdk_draw_string(signalpixmap,
		      	signalfont,
		       	signalarea->style->white_gc,
		        max_signal_name_pixel_width+6,
			texty,
		        t->asciivalue);
		}
		else
		{
		if(t->name)
		gdk_draw_string(signalpixmap,
		      	signalfont,
		        (dobackground==2)?signalarea->style->bg_gc[GTK_STATE_PRELIGHT]:signalarea->style->bg_gc[GTK_STATE_ACTIVE],
		        left_justify_sigs?3:3+max_signal_name_pixel_width-
				gdk_string_measure(signalfont, t->name), 
			texty,
  		        t->name);
		}
	} 

return(retval);
}

/***************************************************************************/

void MaxSignalLength(void)
{
Trptr t;
int len=0,maxlen=0;
int vlen=0, vmaxlen=0;
char buf[65];
int bufxlen;

DEBUG(printf("signalwindow_width_dirty: %d\n",signalwindow_width_dirty));

if((!signalwindow_width_dirty)&&(use_nonprop_fonts)) return;

signalwindow_width_dirty=0;

t=traces.first;
while(t)
{
if(t->flags&TR_BLANK)	/* for "comment" style blank traces */
	{
	if(t->name)
		{
		len=gdk_string_measure(signalfont, t->name);
		if(len>maxlen) maxlen=len;
		}
	t=t->next;
	}
else
if(t->name)
	{
	if((shift_timebase=t->shift))
        	{
        	buf[0]='`';
        	reformat_time(buf+1, t->shift, time_dimension);
        	strcpy(buf+strlen(buf+1)+1,"\'");
        	bufxlen=gdk_string_measure(signalfont, buf);
        	}
        	else
        	{
        	bufxlen=0;
        	}

	len=gdk_string_measure(signalfont, t->name)+bufxlen;
	if(len>maxlen) maxlen=len;

	if((tims.marker!=-1)&&(!(t->flags&TR_EXCLUDE)))
		{
		t->asciitime=tims.marker;
		if(t->asciivalue) free_2(t->asciivalue);

		if(t->vector)
			{
			char *str, *str2;
			vptr v;

                        v=bsearch_vector(t->n.vec,tims.marker);
                        str=convert_ascii(t,v);
			if(str)
				{
				str2=(char *)malloc_2(strlen(str)+2);
				*str2='=';
				strcpy(str2+1,str);
				free_2(str);

				vlen=gdk_string_measure(signalfont,str2);
				t->asciivalue=str2;
				}
				else
				{
				vlen=0;
				t->asciivalue=NULL;
				}

			}
			else
			{
			char *str;
			hptr hptr;
			if((hptr=bsearch_node(t->n.nd,tims.marker)))
				{
				if(!t->n.nd->ext)
					{
					str=(char *)calloc_2(1,3*sizeof(char));
					str[0]='=';
					if(t->flags&TR_INVERT)
						{
						str[1]="1XZ0"[hptr->v.val];
						}
						else
						{
						str[1]="0XZ1"[hptr->v.val];
						}
					t->asciivalue=str;
					vlen=gdk_string_measure(signalfont,str);
					}
					else
					{
					char *str2;

					if(hptr->flags&HIST_REAL)
						{
						if(!(hptr->flags&HIST_STRING))
							{
							str=convert_ascii_real((double *)hptr->v.vector);
							}
							else
							{
							str=convert_ascii_string((char *)hptr->v.vector);
							}
						}
						else
						{
		                        	str=convert_ascii_vec(t,hptr->v.vector);
						}

					if(str)
						{
						str2=(char *)malloc_2(strlen(str)+2);
						*str2='=';
						strcpy(str2+1,str);

						free_2(str); 

						vlen=gdk_string_measure(signalfont,str2);
						t->asciivalue=str2;
						}
						else
						{
						vlen=0;
						t->asciivalue=NULL;
						}
					}
				}
				else
				{
				vlen=0;
				t->asciivalue=NULL;
				}
			}

		if(vlen>vmaxlen)
			{
			vmaxlen=vlen;
			}
		}

	t=t->next;
	}
	else
	{
	t=t->next;
	}
}

max_signal_name_pixel_width = maxlen;
signal_pixmap_width=maxlen+6; 		/* 2 * 3 pixel pad */
if(tims.marker!=-1)
	{
	signal_pixmap_width+=(vmaxlen+6);
	}

if(signal_pixmap_width<60) signal_pixmap_width=60;

if(!in_button_press)
if(!do_resize_signals)
	{
	int os;
	os=30;
	gtk_widget_set_usize(GTK_WIDGET(signalwindow), 
			os+30, -1);
	}
else
if((do_resize_signals)&&(signalwindow))
	{
	int oldusize;

	oldusize=signalwindow->allocation.width;
	if(oldusize!=max_signal_name_pixel_width)
		{
		int os;
		os=max_signal_name_pixel_width;
		os=(os<30)?30:os;
		gtk_widget_set_usize(GTK_WIDGET(signalwindow), 
				os+30, -1);
		}
	}
}
/***************************************************************************/

void UpdateSigValue(Trptr t)
{
if(!t) return;
if((t->asciivalue)&&(t->asciitime==tims.marker))return;

if((t->name)&&(!(t->flags&TR_BLANK)))
	{
	shift_timebase=t->shift;
	DEBUG(printf("UpdateSigValue: %s\n",t->name));

	if((tims.marker!=-1)&&(!(t->flags&TR_EXCLUDE)))
		{
		t->asciitime=tims.marker;
		if(t->asciivalue) free_2(t->asciivalue);

		if(t->vector)
			{
			char *str, *str2;
			vptr v;

                        v=bsearch_vector(t->n.vec,tims.marker);
                        str=convert_ascii(t,v);
			if(str)
				{
				str2=(char *)malloc_2(strlen(str)+2);
				*str2='=';
				strcpy(str2+1,str);
				free_2(str);

				t->asciivalue=str2;
				}
				else
				{
				t->asciivalue=NULL;
				}

			}
			else
			{
			char *str;
			hptr hptr;
			if((hptr=bsearch_node(t->n.nd,tims.marker)))
				{
				if(!t->n.nd->ext)
					{
					str=(char *)calloc_2(1,3*sizeof(char));
					str[0]='=';
					if(t->flags&TR_INVERT)
						{
						str[1]="1XZ0"[hptr->v.val];
						}
						else
						{
						str[1]="0XZ1"[hptr->v.val];
						}
					t->asciivalue=str;
					}
					else
					{
					char *str2;

					if(hptr->flags&HIST_REAL)
						{
						if(!(hptr->flags&HIST_STRING))
							{
							str=convert_ascii_real((double *)hptr->v.vector);
							}
							else
							{
							str=convert_ascii_string((char *)hptr->v.vector);
							}
						}
						else
						{
		                        	str=convert_ascii_vec(t,hptr->v.vector);
						}

					if(str)
						{
						str2=(char *)malloc_2(strlen(str)+2);
						*str2='=';
						strcpy(str2+1,str);
						free_2(str);

						t->asciivalue=str2;
						}
						else
						{
						t->asciivalue=NULL;
						}
					}
				}
				else
				{
				t->asciivalue=NULL;
				}
			}
		}
	}
}

/***************************************************************************/

void calczoom(gdouble z0)
{
gdouble ppf, frame;
ppf=((gdouble)(pixelsperframe=200));
frame=pow(zoombase,-z0);

if(frame>((gdouble)MAX_HISTENT_TIME/(gdouble)4.0))
	{
	nsperframe=((gdouble)MAX_HISTENT_TIME/(gdouble)4.0);
	}
	else
	if(frame<(gdouble)1.0)
	{
	nsperframe=1.0;
	}
	else
	{
	nsperframe=frame;
	}

if(zoom_pow10_snap)
if(nsperframe>100.0)
	{
	gdouble p=10.0;
	int l;
	l=(int)((log(nsperframe)/log(p))+0.5);
	nsperframe=pow(p, (gdouble)l);
	}

nspx=nsperframe/ppf;
pxns=ppf/nsperframe;
hashstep=10;

time_trunc_set();	/* map nspx to rounding value */

DEBUG(printf("Zoom: %e Pixelsperframe: %lld, nsperframe: %e\n",z0, pixelsperframe,(float)nsperframe));
}

static void renderhash(int x, TimeType tim)
{
TimeType rborder;
int hashoffset;
int fhminus2;
int rhs;

fhminus2=fontheight-2;

gdk_draw_line(wavepixmap, 
		gc_grid,
		x, 0,
		x, ((!timearray)&&(display_grid)&&(enable_vert_grid))?waveheight:fhminus2);

if(tim==tims.last) return;

rborder=(tims.last-tims.start)*pxns;
DEBUG(printf("Rborder: %lld, Wavewidth: %d\n", rborder, wavewidth));
if(rborder>wavewidth) rborder=wavewidth;
if((rhs=x+pixelsperframe)>rborder) rhs=rborder;

gdk_draw_line(wavepixmap, 
		gc_grid,
		x, wavecrosspiece,
		rhs, wavecrosspiece);

x+=(hashoffset=hashstep);

while((hashoffset<pixelsperframe)&&(x<=rhs))
	{
	gdk_draw_line(wavepixmap, 
		gc_grid,
		x, wavecrosspiece,
		x, fhminus2);

	hashoffset+=hashstep;
	x+=hashstep;
	} 

}

static void rendertimes(void)
{
int x, len;
TimeType tim, rem;
char timebuff[32];
char prevover=0;

renderblackout();

tim=tims.start;
tims.end=tims.start+(((gdouble)wavewidth)*nspx);

/***********/
if(timearray)
	{
	int pos, pos2;
	TimeType *t, tm;
	int y=fontheight+2;
	int oldx=-1;

	pos=bsearch_timechain(tims.start);
	top:
	if((pos>=0)&&(pos<timearray_size))
		{
		t=timearray+pos;
		for(;pos<timearray_size;t++, pos++)
			{
			tm=*t;
			if(tm>=tims.start)
				{
				if(tm<=tims.end)
					{
					x=(tm-tims.start)*pxns;
					if(oldx==x) 
						{
						pos2=bsearch_timechain(tims.start+(((gdouble)(x+1))*nspx));
						if(pos2>pos) { pos=pos2; goto top; } else continue;
						}
					oldx=x;
					gdk_draw_line(wavepixmap, gc_grid, x, y, x, waveheight);
					}
					else
					{
					break;
					}
				}
			}
		}
	}
/***********/


DEBUG(printf("Ruler Start time: "TTFormat", Finish time: "TTFormat"\n",tims.start, tims.end));

x=0;
if(tim)
	{
	rem=tim%((TimeType)nsperframe);
	if(rem)
		{
		tim=tim-nsperframe-rem;
		x=-pixelsperframe-((rem*pixelsperframe)/nsperframe);
		}
	}

for(;;)
	{
	renderhash(x, tim);

	if(tim)
		{
		reformat_time(timebuff, time_trunc(tim), time_dimension);
		}
		else
		{
		strcpy(timebuff, "0");
		}

	len=gdk_string_measure(wavefont, timebuff);
	gdk_draw_string(wavepixmap,
		wavefont,
	       	gc_time,
		x-(len>>1), wavefont->ascent-1,
	        timebuff);

	tim+=nsperframe;
	x+=pixelsperframe;
	if((prevover)||(tim>tims.last)) break;
	if(x>=wavewidth) prevover=1;
	}
}

/***************************************************************************/

static void rendertimebar(void)
{
gdk_draw_rectangle(wavepixmap, gc_timeb, TRUE,
		0, -1, wavewidth, fontheight); 
rendertimes();
rendertraces();
}

static void rendertraces(void)
{
if(!topmost_trace)
	{
	topmost_trace=traces.first;
	}

if(topmost_trace)
	{
	Trptr t;
	hptr h;
	vptr v;
	int i, num_traces_displayable;

	num_traces_displayable=wavearea->allocation.height/(fontheight);
	num_traces_displayable--;   /* for the time trace that is always there */

	t=topmost_trace;
	for(i=0;((i<num_traces_displayable)&&(t));i++)
		{
		if(!(t->flags&(TR_EXCLUDE|TR_BLANK)))
			{
			shift_timebase=t->shift;
			if(!t->vector)
				{
				h=bsearch_node(t->n.nd, tims.start);
				DEBUG(printf("Bit Trace: %s, %s\n", t->name, t->n.nd->nname));
				DEBUG(printf("Start time: "TTFormat", Histent time: "TTFormat"\n", tims.start,(h->time+shift_timebase)));

				if(!t->n.nd->ext)
					{
					draw_hptr_trace(t,h,i,1);
					}
					else
					{
					draw_hptr_trace_vector(t,h,i);
					}
				}
				else
				{
				v=bsearch_vector(t->n.vec, tims.start);
				DEBUG(printf("Vector Trace: %s, %s\n", t->name, t->n.vec->name));
				DEBUG(printf("Start time: "TTFormat", Vectorent time: "TTFormat"\n", tims.start,(v->time+shift_timebase)));
				draw_vptr_trace(t,v,i);
				}
			}
			else
			{
			draw_hptr_trace(NULL,NULL,i,0);
			}
		t=t->next;
		}
	}


draw_named_markers();
draw_marker_partitions();
}


/*
 * draw single traces and use this for rendering the grid lines
 * for "excluded" traces
 */
static void draw_hptr_trace(Trptr t, hptr h, int which, int dodraw)
{
TimeType x0, x1, newtime;
int y0, y1, yu, liney;
TimeType tim, h2tim;
hptr h2, h3;
char hval, h2val, invert;
int last=-1;
GdkGC    *c;

tims.start-=shift_timebase;
tims.end-=shift_timebase;

liney=((which+2)*fontheight)-2;
if((t)&&(t->flags&TR_INVERT))
	{
	y0=((which+1)*fontheight)+2;	
	y1=liney-2;
	invert=1;
	}
	else
	{
	y1=((which+1)*fontheight)+2;	
	y0=liney-2;
	invert=0;
	}
yu=(y0+y1)/2;

if((display_grid)&&(enable_horiz_grid))
	{
	gdk_draw_line(wavepixmap, 
		gc_grid,
		(tims.start<tims.first)?(tims.first-tims.start)*pxns:0, liney,
		(tims.last<=tims.end)?(tims.last-tims.start)*pxns:wavewidth-1, liney);
	}

if((h)&&(tims.start==h->time))
	if (h->v.val != 2) {
	c = (h->v.val == 1 ? gc_x: gc_trans);
	gdk_draw_line(wavepixmap, c, 0, y0, 0, y1);
	}

if(dodraw)
for(;;)
{
if(!h) break;
tim=(h->time);
if((tim>tims.end)||(tim>tims.last)) break;

x0=(tim - tims.start) * pxns;
if(x0<-1) 
	{ 
	x0=-1; 
	}
	else
if(x0>wavewidth)
	{
	break;
	}

h2=h->next;
if(!h2) break;
h2tim=tim=(h2->time);
if(tim>tims.last) tim=tims.last;
	else if(tim>tims.end+1) tim=tims.end+1;
x1=(tim - tims.start) * pxns;
if(x1<-1) 
	{ 
	x1=-1; 
	}
	else
if(x1>wavewidth)
	{
	x1=wavewidth;
	}

if(x0!=x1)
	{
	hval=h->v.val;
	h2val=h2->v.val;

	c = (hval == 1 || h2val == 1 ? gc_x: gc_trans);
	switch(hval)
		{
		case 0:	/* 0 */
		gdk_draw_line(wavepixmap, 
			gc_low,
			x0, y0,
			x1, y0);

		if(h2tim<=tims.end)
		switch(h2val)
			{
			case 2:		gdk_draw_line(wavepixmap, c, x1, y0, x1, yu);	break;
			default:	gdk_draw_line(wavepixmap, c, x1, y0, x1, y1);	break;
			}
		break;
	
		case 1: /* X */
		if(invert)
			{
			gdk_draw_rectangle(wavepixmap, 
				gc_xfill, TRUE,
				x0+1, y0, x1-x0, y1-y0+1); 
			}
			else
			{
			gdk_draw_rectangle(wavepixmap, 
				gc_xfill, TRUE,
				x0+1, y1, x1-x0, y0-y1+1); 
			}
		gdk_draw_line(wavepixmap, 
			gc_x,
			x0, y0,
			x1, y0);
		gdk_draw_line(wavepixmap, 
			gc_x,
			x0, y1,
			x1, y1);
		if(h2tim<=tims.end) gdk_draw_line(wavepixmap, c, x1, y0, x1, y1);
		break;
		
		case 2:	/* Z */
		gdk_draw_line(wavepixmap, 
			gc_mid,
			x0, yu,
			x1, yu);
		if(h2tim<=tims.end)
		switch(h2val)
			{
			case 0: 	gdk_draw_line(wavepixmap, c, x1, yu, x1, y0);	break;
			case 3:		gdk_draw_line(wavepixmap, c, x1, yu, x1, y1);	break;
			default:	gdk_draw_line(wavepixmap, c, x1, y0, x1, y1);	break;
			}
		break;
		
		case 3: /* 1 */
		gdk_draw_line(wavepixmap, 
			gc_high,
			x0, y1,
			x1, y1);
		if(h2tim<=tims.end)
		switch(h2val)
			{
			case 0: 	gdk_draw_line(wavepixmap, c, x1, y1, x1, y0);	break;
			case 2:		gdk_draw_line(wavepixmap, c, x1, y1, x1, yu);    break;
			default:	gdk_draw_line(wavepixmap, c, x1, y0, x1, y1);	break;
			}
			/* fallthrough on break */	
		default:
		break;
		}
	}
	else
	{
	gdk_draw_line(wavepixmap, gc_trans, x1, y0, x1, y1);		
	newtime=(((gdouble)(x1+WAVE_OPT_SKIP))*nspx)+tims.start+shift_timebase;	/* skip to next pixel */
	h3=bsearch_node(t->n.nd,newtime);
	if(h3->time>h->time)
		{
		h=h3;
		continue;
		}
	}

h=h->next;
}

tims.start+=shift_timebase;
tims.end+=shift_timebase;
}

/********************************************************************************************************/

/*
 * draw hptr vectors (integer+real)
 */
static void draw_hptr_trace_vector(Trptr t, hptr h, int which)
{
TimeType x0, x1, newtime, width;
int y0, y1, yu, liney, ytext;
TimeType tim, h2tim;
hptr h2, h3;
char *ascii=NULL;
int type;
int lasttype=-1;
GdkGC    *c;

tims.start-=shift_timebase;
tims.end-=shift_timebase;

liney=((which+2)*fontheight)-2;
y1=((which+1)*fontheight)+2;	
y0=liney-2;
yu=(y0+y1)/2;
ytext=yu-(wavefont->ascent/2)+wavefont->ascent;

if((display_grid)&&(enable_horiz_grid))
	{
	gdk_draw_line(wavepixmap, 
		gc_grid,
		(tims.start<tims.first)?(tims.first-tims.start)*pxns:0, liney,
		(tims.last<=tims.end)?(tims.last-tims.start)*pxns:wavewidth-1, liney);
	}

for(;;)
{
if(!h) break;
tim=(h->time);
if((tim>tims.end)||(tim>tims.last)) break;

x0=(tim - tims.start) * pxns;
if(x0<-1) 
	{ 
	x0=-1; 
	}
	else
if(x0>wavewidth)
	{
	break;
	}

h2=h->next;
if(!h2) break;
h2tim=tim=(h2->time);
if(tim>tims.last) tim=tims.last;
	else if(tim>tims.end+1) tim=tims.end+1;
x1=(tim - tims.start) * pxns;
if(x1<-1) 
	{ 
	x1=-1; 
	}
	else
if(x1>wavewidth)
	{
	x1=wavewidth;
	}

/* draw trans */
type = (!(h->flags&(HIST_REAL|HIST_STRING))) ? vtype(t,h->v.vector) : 0;
if(x0>-1)
if(use_roundcaps)
	{
	if (type == 2) {
		if (lasttype != -1) {
		gdk_draw_line(wavepixmap, 
			(lasttype==1? gc_x:gc_vtrans),
			x0-1, y0,
			x0,   yu);
		gdk_draw_line(wavepixmap, 
			(lasttype==1? gc_x:gc_vtrans),
			x0, yu,
			x0-1, y1);
		}
	} else
	if (lasttype==2) {
		gdk_draw_line(wavepixmap, 
			(type==1? gc_x:gc_vtrans),
			x0+1, y0,
			x0,   yu);
		gdk_draw_line(wavepixmap, 
			(type==1? gc_x:gc_vtrans),
			x0, yu,
			x0+1, y1);
	} else {
		if (lasttype != type) {
		gdk_draw_line(wavepixmap, 
			(lasttype==1? gc_x:gc_vtrans),
			x0-1, y0,
			x0,   yu);
		gdk_draw_line(wavepixmap, 
			(lasttype==1? gc_x:gc_vtrans),
			x0, yu,
			x0-1, y1);
		gdk_draw_line(wavepixmap, 
			(type==1? gc_x:gc_vtrans),
			x0+1, y0,
			x0,   yu);
		gdk_draw_line(wavepixmap, 
			(type==1? gc_x:gc_vtrans),
			x0, yu,
			x0+1, y1);
		} else {
	gdk_draw_line(wavepixmap, 
		(type==1? gc_x:gc_vtrans),
		x0-2, y0,
		x0+2, y1);
	gdk_draw_line(wavepixmap, 
		(type==1? gc_x:gc_vtrans),
		x0+2, y0,
		x0-2, y1);
		}
	}
	}
	else
	{
	gdk_draw_line(wavepixmap, 
		(type==1? gc_x:gc_vtrans),
		x0, y0,
		x0, y1);
	}

		
if(x0!=x1)
	{
	if (type == 2) 
		{
		if(use_roundcaps)
			{
			gdk_draw_line(wavepixmap, 
				gc_mid,
				x0+1, yu,
				x1-1, yu);
			} 
			else 
			{
			gdk_draw_line(wavepixmap, 
				gc_mid,
				x0, yu,
				x1, yu);
			}
		} 
		else 
		{
		if(type == 0) 
			{
			c = gc_vbox; 
			} 
			else 
			{
			c = gc_x;
			}
	
	if(use_roundcaps)
		{
		gdk_draw_line(wavepixmap, 
			c,
			x0+2, y0,
			x1-2, y0);
		gdk_draw_line(wavepixmap, 
			c,
			x0+2, y1,
			x1-2, y1);
		}
		else
		{
		gdk_draw_line(wavepixmap, 
			c,
			x0, y0,
			x1, y0);
		gdk_draw_line(wavepixmap, 
			c,
			x0, y1,
			x1, y1);
		}

if(x0<0) x0=0;	/* fixup left margin */

	if((width=x1-x0)>vector_padding)
		{
		if(h->flags&HIST_REAL)
			{
			if(!(h->flags&HIST_STRING))
				{
				ascii=convert_ascii_real((double *)h->v.vector);
				}
				else
				{
				ascii=convert_ascii_string((char *)h->v.vector);
				}
			}
			else
			{
			ascii=convert_ascii_vec(t,h->v.vector);
			}

		if((x1>=wavewidth)||(gdk_string_measure(wavefont, ascii)+vector_padding<=width))
			{
			gdk_draw_string(wavepixmap,
				wavefont,
			       	gc_value,
				x0+2,ytext,
			        ascii);
			}
		else
			{
			char *mod;

			mod=bsearch_trunc(ascii,width-vector_padding);
			if(mod)
				{
				*mod='+';
				*(mod+1)=0;

				gdk_draw_string(wavepixmap,
					wavefont,
				       	gc_value,
					x0+2,ytext,
				        ascii);
				}
			}

		}
	    }
	}
	else
	{
	newtime=(((gdouble)(x1+WAVE_OPT_SKIP))*nspx)+tims.start+shift_timebase;	/* skip to next pixel */
	h3=bsearch_node(t->n.nd,newtime);
	if(h3->time>h->time)
		{
		h=h3;
		lasttype=type;
		continue;
		}
	}

if(ascii) { free_2(ascii); ascii=NULL; }
h=h->next;
lasttype=type;
}

tims.start+=shift_timebase;
tims.end+=shift_timebase;
}

/********************************************************************************************************/

/*
 * draw vector traces
 */
static void draw_vptr_trace(Trptr t, vptr v, int which)
{
TimeType x0, x1, newtime, width;
int y0, y1, yu, liney, ytext;
TimeType tim, h2tim;
vptr h, h2, h3;
char *ascii=NULL;
int type;
int lasttype=-1;
GdkGC    *c;

tims.start-=shift_timebase;
tims.end-=shift_timebase;

h=v;
liney=((which+2)*fontheight)-2;
y1=((which+1)*fontheight)+2;	
y0=liney-2;
yu=(y0+y1)/2;
ytext=yu-(wavefont->ascent/2)+wavefont->ascent;

if((display_grid)&&(enable_horiz_grid))
	{
	gdk_draw_line(wavepixmap, 
		gc_grid,
		(tims.start<tims.first)?(tims.first-tims.start)*pxns:0, liney,
		(tims.last<=tims.end)?(tims.last-tims.start)*pxns:wavewidth-1, liney);
	}

for(;;)
{
if(!h) break;
tim=(h->time);
if((tim>tims.end)||(tim>tims.last)) break;

x0=(tim - tims.start) * pxns;
if(x0<-1) 
	{ 
	x0=-1; 
	}
	else
if(x0>wavewidth)
	{
	break;
	}

h2=h->next;
if(!h2) break;
h2tim=tim=(h2->time);
if(tim>tims.last) tim=tims.last;
	else if(tim>tims.end+1) tim=tims.end+1;
x1=(tim - tims.start) * pxns;
if(x1<-1) 
	{ 
	x1=-1; 
	}
	else
if(x1>wavewidth)
	{
	x1=wavewidth;
	}

/* draw trans */
type = vtype2(t,h);

if(x0>-1)
if(use_roundcaps)
	{
	if (type == 2) 
		{
		if (lasttype != -1) 
			{
			gdk_draw_line(wavepixmap, 
				(lasttype==1? gc_x:gc_vtrans),
				x0-1, y0,
				x0,   yu);
			gdk_draw_line(wavepixmap, 
				(lasttype==1? gc_x:gc_vtrans),
				x0, yu,
				x0-1, y1);
			}
		} 
		else
		if (lasttype==2) 
			{
			gdk_draw_line(wavepixmap, 
				(type==1? gc_x:gc_vtrans),
				x0+1, y0,
				x0,   yu);
			gdk_draw_line(wavepixmap, 
				(type==1? gc_x:gc_vtrans),
				x0, yu,
				x0+1, y1);
			} 
			else 
			{
			if (lasttype != type) 
				{
				gdk_draw_line(wavepixmap, 
					(lasttype==1? gc_x:gc_vtrans),
					x0-1, y0,
					x0,   yu);
				gdk_draw_line(wavepixmap, 
					(lasttype==1? gc_x:gc_vtrans),
					x0, yu,
					x0-1, y1);
				gdk_draw_line(wavepixmap, 
					(type==1? gc_x:gc_vtrans),
					x0+1, y0,
					x0,   yu);
				gdk_draw_line(wavepixmap, 
					(type==1? gc_x:gc_vtrans),
					x0, yu,
					x0+1, y1);
				} 
				else 
				{
				gdk_draw_line(wavepixmap, 
					(type==1? gc_x:gc_vtrans),
					x0-2, y0,
					x0+2, y1);
				gdk_draw_line(wavepixmap, 
					(type==1? gc_x:gc_vtrans),
					x0+2, y0,
					x0-2, y1);
				}
			}
		}
		else
		{
		gdk_draw_line(wavepixmap, 
			(type==1? gc_x:gc_vtrans),
			x0, y0,
			x0, y1);
		}

if(x0!=x1)
	{
	if (type == 2) 
		{
		if(use_roundcaps)
			{
			gdk_draw_line(wavepixmap, 
				gc_mid,
				x0+1, yu,
				x1-1, yu);
			} 
			else 
			{
			gdk_draw_line(wavepixmap, 
				gc_mid,
				x0, yu,
				x1, yu);
			}
		} 
		else 
		{
		if(type == 0) 
			{
			c = gc_vbox; 
			} 
			else 
			{
			c = gc_x;
			}
	
	if(use_roundcaps)
		{
		gdk_draw_line(wavepixmap, 
			c,
			x0+2, y0,
			x1-2, y0);
		gdk_draw_line(wavepixmap, 
			c,
			x0+2, y1,
			x1-2, y1);
		}
		else
		{
		gdk_draw_line(wavepixmap, 
			c,
			x0, y0,
			x1, y0);
		gdk_draw_line(wavepixmap, 
			c,
			x0, y1,
			x1, y1);
		}


if(x0<0) x0=0;	/* fixup left margin */

	if((width=x1-x0)>vector_padding)
		{
		ascii=convert_ascii(t,h);
		if((x1>=wavewidth)||(gdk_string_measure(wavefont, ascii)+vector_padding<=width))
			{
			gdk_draw_string(wavepixmap,
				wavefont,
			       	gc_value,
				x0+2,ytext,
			        ascii);
			}
		else
			{
			char *mod;

			mod=bsearch_trunc(ascii,width-vector_padding);
			if(mod)
				{
				*mod='+';
				*(mod+1)=0;

				gdk_draw_string(wavepixmap,
					wavefont,
				       	gc_value,
					x0+2,ytext,
				        ascii);
				}
			}

		}
	}
	}
	else
	{
	newtime=(((gdouble)(x1+WAVE_OPT_SKIP))*nspx)+tims.start+shift_timebase;	/* skip to next pixel */
	h3=bsearch_vector(t->n.vec,newtime);
	if(h3->time>h->time)
		{
		h=h3;
		lasttype=type;
		continue;
		}
	}

if(ascii) { free_2(ascii); ascii=NULL; }
lasttype=type;
h=h->next;
}

tims.start+=shift_timebase;
tims.end+=shift_timebase;
}

/********************************************************************************************************/

/*
 * convert trptr+vptr into an ascii string
 */
char *convert_ascii(Trptr t, vptr v)
{
Ulong flags;
int nbits;
unsigned char *bits;
char *os, *pnt, *newbuff;
int i, j, len;
char xtab[4];

static char xfwd[4]={0,1,2,3};
static char xrev[4]={3,1,2,0};

flags=t->flags;
nbits=t->n.vec->nbits;
bits=v->v;

if(flags&TR_INVERT)
	{
	memcpy(xtab,xrev,4);
	}
	else
	{
	memcpy(xtab,xfwd,4);
	}

newbuff=(char *)malloc_2(nbits+6); /* for justify */
if(flags&TR_REVERSE)
	{
	char *fwdpnt, *revpnt;

	fwdpnt=bits;
	revpnt=newbuff+nbits+6;
	for(i=0;i<3;i++) *(--revpnt)=xtab[0];
	for(i=0;i<nbits;i++)
		{
		*(--revpnt)=xtab[(int)(*(fwdpnt++))];
		}
	for(i=0;i<3;i++) *(--revpnt)=xtab[0];
	}
	else
	{
	char *fwdpnt, *fwdpnt2;

	fwdpnt=bits;
	fwdpnt2=newbuff;
	for(i=0;i<3;i++) *(fwdpnt2++)=xtab[0];
	for(i=0;i<nbits;i++)
		{
		*(fwdpnt2++)=xtab[(int)(*(fwdpnt++))];
		}
	for(i=0;i<3;i++) *(fwdpnt2++)=xtab[0];
	}


if(flags&TR_ASCII) 
	{
	char *parse;	
	int found=0;

	len=(nbits/8)+2+2;		/* $xxxxx */
	os=pnt=(char *)calloc_2(1,len);
	if(show_base) { *(pnt++)='"'; }

	parse=(flags&TR_RJUSTIFY)?(newbuff+((nbits+3)&3)):(newbuff+3);

	for(i=0;i<nbits;i+=8)
		{
		unsigned long val;

		val=0;
		for(j=0;j<8;j++)
			{
			val<<=1;

			if(parse[j]==1) { val=1000; }
			if(parse[j]==2) { val=1000; }

			if(parse[j]==3) { val|=1; }
			}
		

		if (val) {
			if (val > 0x7f || !isprint(val)) *pnt++ = '.'; else *pnt++ = val;
			found=1;
		}
		
		parse+=8;
		}
	if (!found && !show_base) {
		*(pnt++)='"';
		*(pnt++)='"';
	}
		
	if(show_base) { *(pnt++)='"'; }
	*(pnt++)=0x00;
	}
else if((flags&TR_HEX)||((flags&(TR_DEC|TR_SIGNED))&&(nbits>64)))
	{
	char *parse;

	len=(nbits/4)+2+1;		/* $xxxxx */
	os=pnt=(char *)calloc_2(1,len);
	if(show_base) { *(pnt++)='$'; }

	parse=(flags&TR_RJUSTIFY)?(newbuff+((nbits+3)&3)):(newbuff+3);

	for(i=0;i<nbits;i+=4)
		{
		unsigned char val;

		val=0;
		for(j=0;j<4;j++)
			{
			val<<=1;

			if(parse[j]==1) { val=16; break; }
			if(parse[j]==2) { val=17; break; }

			if(parse[j]==3) { val|=1; }
			}

		*(pnt++)="0123456789ABCDEFXZ"[val];
		
		parse+=4;
		}

	*(pnt++)=0x00;
	}
else if(flags&TR_OCT)
	{
	char *parse;

	len=(nbits/3)+2+1;		/* #xxxxx */
	os=pnt=(char *)calloc_2(1,len);
	if(show_base) { *(pnt++)='#'; }

	parse=(flags&TR_RJUSTIFY)
		?(newbuff+((nbits%3)?(nbits%3):3))
		:(newbuff+3);

	for(i=0;i<nbits;i+=3)
		{
		unsigned char val;

		val=0;
		for(j=0;j<3;j++)
			{
			val<<=1;

			if(parse[j]==1) { val=8; break; }
			if(parse[j]==2) { val=9; break; }

			if(parse[j]==3) { val|=1; }
			}

		*(pnt++)="01234567XZ"[val];
		
		parse+=3;
		}

	*(pnt++)=0x00;
	}
else if(flags&TR_BIN)
	{
	char *parse;

	len=(nbits/1)+2+1;		/* %xxxxx */
	os=pnt=(char *)calloc_2(1,len);
	if(show_base) { *(pnt++)='%'; }

	parse=newbuff+3;

	for(i=0;i<nbits;i++)
		{
		*(pnt++)="0XZ1"[(int)(*(parse++))];
		}

	*(pnt++)=0x00;
	}
else if(flags&TR_SIGNED)
	{
	char *parse;
	TimeType val;
	unsigned char fail=0;

	len=21;	/* len+1 of 0x8000000000000000 expressed in decimal */
	os=(char *)calloc_2(1,len);

	parse=newbuff+3;

	val=(parse[0]==3)?LLDescriptor(-1):LLDescriptor(0);
	if((parse[0]==1)||(parse[0]==2)) { fail=1; }
	else
	for(i=1;i<nbits;i++)
		{
		val<<=1;

		if((parse[i]==1)||(parse[i]==2)) { fail=1; break; }
		if(parse[i]==3) { val|=LLDescriptor(1); }
		}

	if(!fail)
		{
		sprintf(os, TTFormat, val);
		}
		else
		{
		strcpy(os, "XXX");
		}
	}
else	/* decimal when all else fails */
	{
	char *parse;
	UTimeType val=0;
	unsigned char fail=0;

	len=21;	/* len+1 of 0xffffffffffffffff expressed in decimal */
	os=(char *)calloc_2(1,len);

	parse=newbuff+3;

	for(i=0;i<nbits;i++)
		{
		val<<=1;

		if((parse[i]==1)||(parse[i]==2)) { fail=1; break; }
		if(parse[i]==3) { val|=ULLDescriptor(1); }
		}

	if(!fail)
		{
		sprintf(os, UTTFormat, val);
		}
		else
		{
		strcpy(os, "XXX");
		}
	}

free_2(newbuff);
return(os);
}


/*
 * convert trptr+hptr vectorstring into an ascii string
 */
char *convert_ascii_real(double *d)
{
char *rv;

rv=malloc_2(24);	/* enough for .16e format */

if(d)
	{
	sprintf(rv,"%.16g",*d);	
	}
else
	{
	strcpy(rv,"UNDEF");
	}

return(rv);
}

char *convert_ascii_string(char *s)
{
char *rv;

if(s)
	{
	rv=(char *)malloc_2(strlen(s)+1);
	strcpy(rv, s);
	}
	else
	{
	rv=(char *)malloc_2(6);
	strcpy(rv, "UNDEF");
	}
return(rv);
}

int vtype(Trptr t, char *vec)
{
	int i, nbits, res;

	if (vec == NULL)
		return(1);
	nbits=t->n.nd->ext->msi-t->n.nd->ext->lsi;
	if(nbits<0)nbits=-nbits;
	nbits++;
	res = 3;
	for (i = 0; i < nbits; i++)
	switch (*vec) {
	case 1:		
	case 'x':
	case 'X':
			return(1);
	case 2:		
	case 'z':
	case 'Z':
			if (res == 0) return(1); vec++; res = 2; break;
	default:	if (res == 2) return(1); vec++; res = 0; break;
	}

	return(res);
}

int vtype2(Trptr t, vptr v)
{
	int i, nbits, res;
	char *vec=v->v;

	if (vec == NULL)
		return(1);
	nbits=t->n.vec->nbits;
	res = 3;
	for (i = 0; i < nbits; i++)
		{
		switch (*vec) 
			{
			case 1:		
			case 'x':
			case 'X':
					return(1);
			case 2:		
			case 'z':
			case 'Z':
					if (res == 0) return(1); vec++; res = 2; break;
			default:	if (res == 2) return(1); vec++; res = 0; break;
			}
		}

	return(res);
}

/*
 * convert trptr+hptr vectorstring into an ascii string
 */
char *convert_ascii_vec(Trptr t, char *vec)
{
Ulong flags;
int nbits;
char *bits;
char *os, *pnt, *newbuff;
int i, j, len;
char xtab[4];

static char xfwd[4]={0,1,2,3};
static char xrev[4]={3,1,2,0};

flags=t->flags;

nbits=t->n.nd->ext->msi-t->n.nd->ext->lsi;
if(nbits<0)nbits=-nbits;
nbits++;

if(vec)
        {  
        bits=vec;
        if(*vec>3)              /* convert as needed */
        for(i=0;i<nbits;i++)
                {
                switch(*(vec))
                        {
                        case '0': *vec++=0; break;
                        case '1': *vec++=3; break;
                        case 'X':
                        case 'x': *vec++=1; break;
                        default:  *vec++=2; break;
                        }
                }
        }
        else
        {
        pnt=bits=wave_alloca(nbits);
        for(i=0;i<nbits;i++)
                {
                *pnt++=1;
                }
        }


if(flags&TR_INVERT)
	{
	memcpy(xtab,xrev,4);
	}
	else
	{
	memcpy(xtab,xfwd,4);
	}

newbuff=(char *)malloc_2(nbits+6); /* for justify */
if(flags&TR_REVERSE)
	{
	char *fwdpnt, *revpnt;

	fwdpnt=bits;
	revpnt=newbuff+nbits+6;
	for(i=0;i<3;i++) *(--revpnt)=xtab[0];
	for(i=0;i<nbits;i++)
		{
		*(--revpnt)=xtab[(int)(*(fwdpnt++))];
		}
	for(i=0;i<3;i++) *(--revpnt)=xtab[0];
	}
	else
	{
	char *fwdpnt, *fwdpnt2;

	fwdpnt=bits;
	fwdpnt2=newbuff;
	for(i=0;i<3;i++) *(fwdpnt2++)=xtab[0];
	for(i=0;i<nbits;i++)
		{
		*(fwdpnt2++)=xtab[(int)(*(fwdpnt++))];
		}
	for(i=0;i<3;i++) *(fwdpnt2++)=xtab[0];
	}

if(flags&TR_ASCII) 
	{
	char *parse;	
	int found=0;

	len=(nbits/8)+2+2;		/* $xxxxx */
	os=pnt=(char *)calloc_2(1,len);
	if(show_base) { *(pnt++)='"'; }

	parse=(flags&TR_RJUSTIFY)?(newbuff+((nbits+3)&3)):(newbuff+3);

	for(i=0;i<nbits;i+=8)
		{
		unsigned long val;

		val=0;
		for(j=0;j<8;j++)
			{
			val<<=1;

			if(parse[j]==1) { val=1000; }
			if(parse[j]==2) { val=1000; }

			if(parse[j]==3) { val|=1; }
			}
		

		if (val) {
			if (val > 0x7f || !isprint(val)) *pnt++ = '.'; else *pnt++ = val;
			found=1;
		}
		
		parse+=8;
		}
	if (!found && !show_base) {
		*(pnt++)='"';
		*(pnt++)='"';
	}
		
	if(show_base) { *(pnt++)='"'; }
	*(pnt++)=0x00;
	}
else if((flags&TR_HEX)||((flags&(TR_DEC|TR_SIGNED))&&(nbits>64)))
	{
	char *parse;

	len=(nbits/4)+2+1;		/* $xxxxx */
	os=pnt=(char *)calloc_2(1,len);
	if(show_base) { *(pnt++)='$'; }

	parse=(flags&TR_RJUSTIFY)?(newbuff+((nbits+3)&3)):(newbuff+3);

	for(i=0;i<nbits;i+=4)
		{
		unsigned char val;

		val=0;
		for(j=0;j<4;j++)
			{
			val<<=1;

			if(parse[j]==1) { val=16; break; }
			if(parse[j]==2) { val=17; break; }

			if(parse[j]==3) { val|=1; }
			}

		*(pnt++)="0123456789ABCDEFXZ"[val];
		
		parse+=4;
		}

	*(pnt++)=0x00;
	}
else if(flags&TR_OCT)
	{
	char *parse;

	len=(nbits/3)+2+1;		/* #xxxxx */
	os=pnt=(char *)calloc_2(1,len);
	if(show_base) { *(pnt++)='#'; }

	parse=(flags&TR_RJUSTIFY)
		?(newbuff+((nbits%3)?(nbits%3):3))
		:(newbuff+3);

	for(i=0;i<nbits;i+=3)
		{
		unsigned char val;

		val=0;
		for(j=0;j<3;j++)
			{
			val<<=1;

			if(parse[j]==1) { val=8; break; }
			if(parse[j]==2) { val=9; break; }

			if(parse[j]==3) { val|=1; }
			}

		*(pnt++)="01234567XZ"[val];
		
		parse+=3;
		}

	*(pnt++)=0x00;
	}
else if(flags&TR_BIN)
	{
	char *parse;

	len=(nbits/1)+2+1;		/* %xxxxx */
	os=pnt=(char *)calloc_2(1,len);
	if(show_base) { *(pnt++)='%'; }

	parse=newbuff+3;

	for(i=0;i<nbits;i++)
		{
		*(pnt++)="0XZ1"[(int)(*(parse++))];
		}

	*(pnt++)=0x00;
	}
else if(flags&TR_SIGNED)
	{
	char *parse;
	TimeType val;
	unsigned char fail=0;

	len=21;	/* len+1 of 0x8000000000000000 expressed in decimal */
	os=(char *)calloc_2(1,len);

	parse=newbuff+3;

	val=(parse[0]==3)?LLDescriptor(-1):LLDescriptor(0);
	if((parse[0]==1)||(parse[0]==2)) { fail=1; }
	else
	for(i=1;i<nbits;i++)
		{
		val<<=1;

		if((parse[i]==1)||(parse[i]==2)) { fail=1; break; }
		if(parse[i]==3) { val|=LLDescriptor(1); }
		}

	if(!fail)
		{
		sprintf(os, TTFormat, val);
		}
		else
		{
		strcpy(os, "XXX");
		}
	}
else	/* decimal when all else fails */
	{
	char *parse;
	UTimeType val=0;
	unsigned char fail=0;

	len=11;	/* len+1 of 0x7fffffff expressed in decimal */
	os=(char *)calloc_2(1,len);

	parse=newbuff+3;

	for(i=0;i<nbits;i++)
		{
		val<<=1;

		if((parse[i]==1)||(parse[i]==2)) { fail=1; break; }
		if(parse[i]==3) { val|=ULLDescriptor(1); }
		}

	if(!fail)
		{
		sprintf(os, UTTFormat, val);
		}
		else
		{
		strcpy(os, "XXX");
		}
	}

free_2(newbuff);
return(os);
}

