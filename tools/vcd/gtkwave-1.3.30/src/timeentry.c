/* 
 * Copyright (c) Tony Bybell 1999.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include <gtk/gtk.h>
#include "aet.h"
#include "debug.h"

GtkWidget *from_entry=NULL, *to_entry=NULL;


void time_update(void)
{
DEBUG(printf("Timeentry Configure Event\n"));

calczoom(tims.zoom);
fix_wavehadj();
gtk_signal_emit_by_name (GTK_OBJECT (GTK_ADJUSTMENT(wave_hslider)), "changed");
gtk_signal_emit_by_name (GTK_OBJECT (GTK_ADJUSTMENT(wave_hslider)), "value_changed");
}

   
static void
callback(GtkWidget *widget, GtkWidget *entry)
{
gchar *entry_text;
TimeType newlo;
char fromstr[40];

entry_text=gtk_entry_get_text(GTK_ENTRY(entry));
DEBUG(printf("From Entry contents: %s\n",entry_text));

newlo=unformat_time(entry_text, time_dimension);

if(newlo<min_time) 
	{
	newlo=min_time; 
	}

if(newlo<(tims.last)) 
	{ 
	tims.first=newlo;
	if(tims.start<tims.first) tims.timecache=tims.start=tims.first;

	reformat_time(fromstr, tims.first, time_dimension);
	gtk_entry_set_text(GTK_ENTRY(entry),fromstr);

	time_update(); 
	return;
	}
	else
	{
	reformat_time(fromstr, tims.first, time_dimension);
	gtk_entry_set_text(GTK_ENTRY(entry),fromstr);
	return;
	}
}

static void
callback2(GtkWidget *widget, GtkWidget *entry)
{
gchar *entry_text;
TimeType newhi;
char tostr[40];

entry_text=gtk_entry_get_text(GTK_ENTRY(entry));
DEBUG(printf("To Entry contents: %s\n",entry_text));

newhi=unformat_time(entry_text, time_dimension);

if(newhi>max_time) 
	{
	newhi=max_time; 
	}

if(newhi>(tims.first)) 
	{ 
	tims.last=newhi;
	reformat_time(tostr, tims.last, time_dimension);
	gtk_entry_set_text(GTK_ENTRY(entry),tostr);
	time_update(); 
	return;
	}
	else
	{
	reformat_time(tostr, tims.last, time_dimension);
	gtk_entry_set_text(GTK_ENTRY(entry),tostr);
	return;
	}
}
   
/* Create an entry box */
GtkWidget *
create_entry_box(void)
{
GtkWidget *label, *label2;
GtkWidget *box, *box2;
GtkWidget *mainbox;

char fromstr[32], tostr[32];

GtkTooltips *tooltips;

tooltips=gtk_tooltips_new_2();
gtk_tooltips_set_delay_2(tooltips,1500);

label=gtk_label_new("From:");
from_entry=gtk_entry_new_with_max_length(40);

reformat_time(fromstr, min_time, time_dimension);

gtk_entry_set_text(GTK_ENTRY(from_entry),fromstr);
gtk_signal_connect (GTK_OBJECT (from_entry), "activate",
			GTK_SIGNAL_FUNC (callback), from_entry);
box=gtk_hbox_new(FALSE, 0);
gtk_box_pack_start(GTK_BOX(box), label, TRUE, TRUE, 0); 
gtk_widget_show(label);
gtk_box_pack_start(GTK_BOX(box), from_entry, TRUE, TRUE, 0); 
gtk_widget_set_usize(GTK_WIDGET(from_entry), 90, 22); 
gtk_tooltips_set_tip_2(tooltips, from_entry, "Scroll Lower Bound", NULL);
gtk_widget_show(from_entry);


label2=gtk_label_new("To:");
to_entry=gtk_entry_new_with_max_length(40);

reformat_time(tostr, max_time, time_dimension);

gtk_entry_set_text(GTK_ENTRY(to_entry),tostr);
gtk_signal_connect (GTK_OBJECT (to_entry), "activate",
			GTK_SIGNAL_FUNC (callback2), to_entry);
box2=gtk_hbox_new(FALSE, 0);
gtk_box_pack_start(GTK_BOX(box2), label2, TRUE, TRUE, 0); 
gtk_widget_show(label2);
gtk_box_pack_start(GTK_BOX(box2), to_entry, TRUE, TRUE, 0); 
gtk_widget_set_usize(GTK_WIDGET(to_entry), 90, 22); 
gtk_tooltips_set_tip_2(tooltips, to_entry, "Scroll Upper Bound", NULL);
gtk_widget_show(to_entry);

mainbox=gtk_vbox_new(FALSE, 0);
gtk_box_pack_start(GTK_BOX(mainbox), box, TRUE, FALSE, 1);
gtk_widget_show(box);
gtk_box_pack_start(GTK_BOX(mainbox), box2, TRUE, FALSE, 1);
gtk_widget_show(box2);
   
return(mainbox);
}
   
