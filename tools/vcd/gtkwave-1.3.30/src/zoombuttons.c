/* 
 * Copyright (c) Tony Bybell 1999.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include "pixmaps.h"
#include "currenttime.h"
#include "debug.h"

char do_zoom_center=1;
char do_initial_zoom_fit=0;

void fix_wavehadj(void)
{
GtkAdjustment *hadj;
gfloat pageinc;

hadj=GTK_ADJUSTMENT(wave_hslider);
hadj->lower=tims.first;
hadj->upper=tims.last+2.0;

pageinc=(gfloat)(((gdouble)wavewidth)*nspx);
hadj->page_size=hadj->page_increment=(pageinc>=1.0)?pageinc:1.0;
hadj->step_increment=(nspx>=1.0)?nspx:1.0;

if(hadj->page_size >= (hadj->upper-hadj->lower)) hadj->value=hadj->lower;
if(hadj->value+hadj->page_size>hadj->upper)
	{
	hadj->value=hadj->upper-hadj->page_size;
	if(hadj->value<hadj->lower)
		hadj->value=hadj->lower;
	}

}

void service_zoom_left(GtkWidget *text, gpointer data)
{
GtkAdjustment *hadj;

if(helpbox_is_active)
        {
        help_text_bold("\n\nZoom To Start");
        help_text(
		" is used to jump scroll to the trace's beginning."
        );
        return;
        }

hadj=GTK_ADJUSTMENT(wave_hslider);
hadj->value=tims.timecache=tims.first;
time_update();
}

void service_zoom_right(GtkWidget *text, gpointer data)
{
GtkAdjustment *hadj;
TimeType ntinc;

if(helpbox_is_active)
        {
        help_text_bold("\n\nZoom To End");
        help_text(
		" is used to jump scroll to the trace's end."
        );
        return;
        }

ntinc=(TimeType)(((gdouble)wavewidth)*nspx);

tims.timecache=tims.last-ntinc+1;
        if(tims.timecache<tims.first) tims.timecache=tims.first;

hadj=GTK_ADJUSTMENT(wave_hslider);
hadj->value=tims.timecache;
time_update();
}

void service_zoom_out(GtkWidget *text, gpointer data)
{
TimeType middle=0, width;

if(helpbox_is_active)
        {
        help_text_bold("\n\nZoom Out");
        help_text(
		" is used to decrease the zoom factor."
        );
        return;
        }

if(do_zoom_center)
	{
	if((tims.marker<0)||(tims.marker<tims.first)||(tims.marker>tims.last))
		{
		if(tims.end>tims.last) tims.end=tims.last;
		middle=(tims.start/2)+(tims.end/2);
		if((tims.start&1)&&(tims.end&1)) middle++;
		}
		else
		{
		middle=tims.marker;
		}
	}

tims.prevzoom=tims.zoom;

tims.zoom--;
calczoom(tims.zoom);

if(do_zoom_center)
	{
	width=(TimeType)(((gdouble)wavewidth)*nspx);
	tims.start=time_trunc(middle-(width/2));
        if(tims.start+width>tims.last) tims.start=time_trunc(tims.last-width);
	if(tims.start<tims.first) tims.start=tims.first;
	GTK_ADJUSTMENT(wave_hslider)->value=tims.timecache=tims.start;
	}
	else
	{
	tims.timecache=0;
	}	

fix_wavehadj();

gtk_signal_emit_by_name (GTK_OBJECT (GTK_ADJUSTMENT(wave_hslider)), "changed"); /* force zoom update */
gtk_signal_emit_by_name (GTK_OBJECT (GTK_ADJUSTMENT(wave_hslider)), "value_changed"); /* force zoom update */

DEBUG(printf("Zoombuttons out\n"));
}

void service_zoom_in(GtkWidget *text, gpointer data)
{
if(helpbox_is_active)
        {
        help_text_bold("\n\nZoom In");
        help_text(
                " is used to increase the zoom factor."  
        );
        return;
        }

if(tims.zoom<0)		/* otherwise it's ridiculous and can cause */
	{		/* overflow problems in the scope          */
	TimeType middle=0, width;

	if(do_zoom_center)
		{
		if((tims.marker<0)||(tims.marker<tims.first)||(tims.marker>tims.last))
			{
			if(tims.end>tims.last) tims.end=tims.last;
			middle=(tims.start/2)+(tims.end/2);
			if((tims.start&1)&&(tims.end&1)) middle++;
			}
			else
			{
			middle=tims.marker;
			}
		}

	tims.prevzoom=tims.zoom;

	tims.zoom++;
	calczoom(tims.zoom);

	if(do_zoom_center)
		{
		width=(TimeType)(((gdouble)wavewidth)*nspx);
		tims.start=time_trunc(middle-(width/2));
	        if(tims.start+width>tims.last) tims.start=time_trunc(tims.last-width);
		if(tims.start<tims.first) tims.start=tims.first;
		GTK_ADJUSTMENT(wave_hslider)->value=tims.timecache=tims.start;
		}
		else
		{
		tims.timecache=0;
		}	

	fix_wavehadj();
	
	gtk_signal_emit_by_name (GTK_OBJECT (GTK_ADJUSTMENT(wave_hslider)), "changed"); /* force zoom update */
	gtk_signal_emit_by_name (GTK_OBJECT (GTK_ADJUSTMENT(wave_hslider)), "value_changed"); /* force zoom update */
	
	DEBUG(printf("Zoombuttons in\n"));
	}
}

void service_zoom_undo(GtkWidget *text, gpointer data)
{
gdouble temp;

if(helpbox_is_active)
        {
        help_text_bold("\n\nZoom Undo");
        help_text(
                " is used to revert to the previous zoom value used.  Undo"  
		" only works one level deep."
        );
        return;
        }


temp=tims.zoom;
tims.zoom=tims.prevzoom;
tims.prevzoom=temp;
tims.timecache=0;
calczoom(tims.zoom);
fix_wavehadj();

gtk_signal_emit_by_name (GTK_OBJECT (GTK_ADJUSTMENT(wave_hslider)), "changed"); /* force zoom update */
gtk_signal_emit_by_name (GTK_OBJECT (GTK_ADJUSTMENT(wave_hslider)), "value_changed"); /* force zoom update */

DEBUG(printf("Zoombuttons Undo\n"));
}

void service_zoom_fit(GtkWidget *text, gpointer data)
{
gdouble estimated;
int fixedwidth;

if(helpbox_is_active)
        {
        help_text_bold("\n\nZoom Best Fit");
        help_text(
		" attempts a \"best fit\" to get the whole trace onscreen."
		"  Note that the trace may be more or less than a whole screen since"
		" this isn't a \"perfect fit.\""
        );
        return;
        }

if(wavewidth>4) { fixedwidth=wavewidth-4; } else { fixedwidth=wavewidth; }
estimated=-log(((gdouble)(tims.last-tims.first+1))/((gdouble)fixedwidth)*((gdouble)200.0))/log(zoombase);
if(estimated>((gdouble)(0.0))) estimated=((gdouble)(0.0));

tims.prevzoom=tims.zoom;
tims.timecache=0;

calczoom(estimated);
tims.zoom=estimated;

fix_wavehadj();

gtk_signal_emit_by_name (GTK_OBJECT (GTK_ADJUSTMENT(wave_hslider)), "changed"); /* force zoom update */
gtk_signal_emit_by_name (GTK_OBJECT (GTK_ADJUSTMENT(wave_hslider)), "value_changed"); /* force zoom update */

DEBUG(printf("Zoombuttons Fit\n"));
}


void service_dragzoom(TimeType time1, TimeType time2)	/* the function you've been waiting for... */
{
gdouble estimated;
int fixedwidth;
TimeType temp;
GtkAdjustment *hadj;
Trptr t;

if(time2<time1)
	{
	temp=time1;
	time1=time2;
	time2=temp;
	}

if(time2>time1)	/* ensure at least 1 tick */
	{
	if(wavewidth>4) { fixedwidth=wavewidth-4; } else { fixedwidth=wavewidth; }
	estimated=-log(((gdouble)(time2-time1+1))/((gdouble)fixedwidth)*((gdouble)200.0))/log(zoombase);
	if(estimated>((gdouble)(0.0))) estimated=((gdouble)(0.0));

	tims.prevzoom=tims.zoom;
	tims.timecache=tims.laststart=tims.start=time_trunc(time1);

        for(t=traces.first;t;t=t->next)	/* have to nuke string refs so printout is ok! */
                {
                if(t->asciivalue) { free_2(t->asciivalue); t->asciivalue=NULL; }
                }

        for(t=traces.buffer;t;t=t->next)
                {
                if(t->asciivalue) { free_2(t->asciivalue); t->asciivalue=NULL; }
                }

        update_markertime(tims.marker=-1);
        signalwindow_width_dirty=1;
        MaxSignalLength();


        hadj=GTK_ADJUSTMENT(wave_hslider);
        hadj->value=time1;

	calczoom(estimated);
	tims.zoom=estimated;

	fix_wavehadj();

	gtk_signal_emit_by_name (GTK_OBJECT (GTK_ADJUSTMENT(wave_hslider)), "changed"); /* force zoom update */
	gtk_signal_emit_by_name (GTK_OBJECT (GTK_ADJUSTMENT(wave_hslider)), "value_changed"); /* force zoom update */

	DEBUG(printf("Drag Zoom\n"));
	}
}

/* Create actual buttons */
GtkWidget *
create_zoom_buttons (void)
{
GtkWidget *table;
GtkWidget *table2;
GtkWidget *frame;
GtkWidget *main_vbox;
GtkWidget *b1;
GtkWidget *b2;
GtkWidget *b3;
GtkWidget *b4;
GtkWidget *b5;
GtkWidget *b6;
GtkWidget *pixmapzout, *pixmapzin, *pixmapzfit, *pixmapzundo;
GtkWidget *pixmapzleft, *pixmapzright;

GtkTooltips *tooltips;

tooltips=gtk_tooltips_new_2();
gtk_tooltips_set_delay_2(tooltips,1500);

pixmapzin=gtk_pixmap_new(zoomin_pixmap, zoomin_mask);
gtk_widget_show(pixmapzin);
pixmapzout=gtk_pixmap_new(zoomout_pixmap, zoomout_mask);
gtk_widget_show(pixmapzout);
pixmapzfit=gtk_pixmap_new(zoomfit_pixmap, zoomfit_mask);
gtk_widget_show(pixmapzfit);
pixmapzundo=gtk_pixmap_new(zoomundo_pixmap, zoomundo_mask);
gtk_widget_show(pixmapzundo);

pixmapzleft=gtk_pixmap_new(zoom_larrow_pixmap, zoom_larrow_mask);
gtk_widget_show(pixmapzleft);
pixmapzright=gtk_pixmap_new(zoom_rarrow_pixmap, zoom_rarrow_mask);
gtk_widget_show(pixmapzright);


/* Create a table to hold the text widget and scrollbars */
table = gtk_table_new (1, 1, FALSE);

main_vbox = gtk_vbox_new (FALSE, 1);
gtk_container_border_width (GTK_CONTAINER (main_vbox), 1);
gtk_container_add (GTK_CONTAINER (table), main_vbox);

frame = gtk_frame_new ("Zoom ");
gtk_box_pack_start (GTK_BOX (main_vbox), frame, TRUE, TRUE, 0);
gtk_widget_show (frame);
gtk_widget_show (main_vbox);

table2 = gtk_table_new (2, 3, FALSE);

b1 = gtk_button_new();
gtk_container_add(GTK_CONTAINER(b1), pixmapzin);
gtk_table_attach (GTK_TABLE (table2), b1, 0, 1, 0, 1,
		      	GTK_FILL | GTK_EXPAND,
		      	GTK_FILL | GTK_EXPAND | GTK_SHRINK, 1, 1);
gtk_signal_connect_object (GTK_OBJECT (b1), "clicked",
			GTK_SIGNAL_FUNC(service_zoom_out), GTK_OBJECT (table2));
gtk_tooltips_set_tip_2(tooltips, b1, "Zoom Out", NULL);
gtk_widget_show(b1);

b2 = gtk_button_new();
gtk_container_add(GTK_CONTAINER(b2), pixmapzout);
gtk_table_attach (GTK_TABLE (table2), b2, 0, 1, 1, 2,
		      	GTK_FILL | GTK_EXPAND,
		      	GTK_FILL | GTK_EXPAND | GTK_SHRINK, 1, 1);
gtk_signal_connect_object (GTK_OBJECT (b2), "clicked",
			GTK_SIGNAL_FUNC(service_zoom_in), GTK_OBJECT (table2));
gtk_tooltips_set_tip_2(tooltips, b2, "Zoom In", NULL);
gtk_widget_show(b2);

b3 = gtk_button_new();
gtk_container_add(GTK_CONTAINER(b3), pixmapzfit);
gtk_table_attach (GTK_TABLE (table2), b3, 1, 2, 0, 1,
		      	GTK_FILL | GTK_EXPAND,
		      	GTK_FILL | GTK_EXPAND | GTK_SHRINK, 1, 1);
gtk_signal_connect_object (GTK_OBJECT (b3), "clicked",
			GTK_SIGNAL_FUNC(service_zoom_fit), GTK_OBJECT (table2));
gtk_tooltips_set_tip_2(tooltips, b3, "Zoom Best Fit", NULL);
gtk_widget_show(b3);

b4 = gtk_button_new();
gtk_container_add(GTK_CONTAINER(b4), pixmapzundo);
gtk_table_attach (GTK_TABLE (table2), b4, 1, 2, 1, 2,
		      	GTK_FILL | GTK_EXPAND,
		      	GTK_FILL | GTK_EXPAND | GTK_SHRINK, 1, 1);
gtk_signal_connect_object (GTK_OBJECT (b4), "clicked",
			GTK_SIGNAL_FUNC(service_zoom_undo), GTK_OBJECT (table2));
gtk_tooltips_set_tip_2(tooltips, b4, "Undo Last Zoom", NULL);
gtk_widget_show(b4);

b5 = gtk_button_new();
gtk_container_add(GTK_CONTAINER(b5), pixmapzleft);
gtk_table_attach (GTK_TABLE (table2), b5, 2, 3, 0, 1,
		      	GTK_FILL | GTK_EXPAND,
		      	GTK_FILL | GTK_EXPAND | GTK_SHRINK, 1, 1);
gtk_signal_connect_object (GTK_OBJECT (b5), "clicked",
			GTK_SIGNAL_FUNC(service_zoom_left), GTK_OBJECT (table2));
gtk_tooltips_set_tip_2(tooltips, b5, "Zoom To Start", NULL);
gtk_widget_show(b5);

b6 = gtk_button_new();
gtk_container_add(GTK_CONTAINER(b6), pixmapzright);
gtk_table_attach (GTK_TABLE (table2), b6, 2, 3, 1, 2,
		      	GTK_FILL | GTK_EXPAND,
		      	GTK_FILL | GTK_EXPAND | GTK_SHRINK, 1, 1);
gtk_signal_connect_object (GTK_OBJECT (b6), "clicked",
			GTK_SIGNAL_FUNC(service_zoom_right), GTK_OBJECT (table2));
gtk_tooltips_set_tip_2(tooltips, b6, "Zoom To End", NULL);
gtk_widget_show(b6);


gtk_container_add (GTK_CONTAINER (frame), table2);

gtk_widget_show(table2);

return table;
}

