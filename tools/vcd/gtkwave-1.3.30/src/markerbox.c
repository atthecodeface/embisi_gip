/* 
 * Copyright (c) Tony Bybell 1999.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include <gtk/gtk.h>
#include <string.h>
#include "debug.h"
#include "analyzer.h"
#include "currenttime.h"

static GtkWidget *window;
static GtkWidget *entries[26];
static GtkSignalFunc cleanup;
static int dirty;

static TimeType shadow_markers[26];

static void enter_callback(GtkWidget *widget, gpointer which)
{
GtkWidget *entry;
TimeType *modify;
TimeType temp;
gchar *entry_text;
char buf[49];
int len, i;
int ent_idx;

ent_idx = ((int) (((long) which) & 31L)) % 26;
 
entry=entries[ent_idx];

entry_text = gtk_entry_get_text(GTK_ENTRY(entry));
if(!(len=strlen(entry_text))) goto failure;

temp=unformat_time(entry_text, time_dimension);
if((temp<tims.start)||(temp>tims.last)) goto failure;

for(i=0;i<26;i++)
	{
	if(temp==shadow_markers[i]) goto failure;
	}

reformat_time(buf, temp, time_dimension);
gtk_entry_set_text (GTK_ENTRY (entry), buf);

shadow_markers[ent_idx]=temp;
dirty=1;
gtk_entry_select_region (GTK_ENTRY (entry),
			     0, GTK_ENTRY(entry)->text_length);
return;

failure:
modify=(TimeType *)which;
if(shadow_markers[ent_idx]==-1)
	{
	sprintf(buf,"<None>");
	}
	else
	{
	reformat_time(buf, shadow_markers[ent_idx], time_dimension);
	}
gtk_entry_set_text (GTK_ENTRY (entry), buf);
}

static void ok_callback(GtkWidget *widget, GtkWidget *nothing)
{
if(dirty)
	{
	int i;
	for(i=0;i<26;i++) named_markers[i]=shadow_markers[i];
        MaxSignalLength();
        signalarea_configure_event(signalarea, NULL);
        wavearea_configure_event(wavearea, NULL);
	}

  gtk_grab_remove(window);
  gtk_widget_destroy(window);

  cleanup();
}

static void destroy_callback(GtkWidget *widget, GtkWidget *nothing)
{
  gtk_grab_remove(window);
  gtk_widget_destroy(window);
}

void markerbox(char *title, GtkSignalFunc func)
{
    GtkWidget *entry;
    GtkWidget *vbox, *hbox, *vbox_g, *label;
    GtkWidget *button1, *button2, *scrolled_win, *frame, *separator;
    char labtitle[2]={0,0};
    int i;

    cleanup=func;
    dirty=0;

    for(i=0;i<26;i++) shadow_markers[i]=named_markers[i];

    /* create a new modal window */
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_grab_add(window);
    gtk_window_set_title(GTK_WINDOW (window), title);
    gtk_signal_connect(GTK_OBJECT (window), "delete_event",
                       (GtkSignalFunc) destroy_callback, NULL);

    vbox = gtk_vbox_new (FALSE, 0);
    gtk_container_add (GTK_CONTAINER (window), vbox);
    gtk_widget_show (vbox);

    vbox_g = gtk_vbox_new (FALSE, 0);
    gtk_widget_show (vbox_g);

    frame = gtk_frame_new (NULL);
    gtk_container_border_width (GTK_CONTAINER (frame), 3);
    gtk_widget_show(frame);

    scrolled_win = gtk_scrolled_window_new (NULL, NULL);
    gtk_widget_set_usize( GTK_WIDGET (scrolled_win), -1, 300);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
                                      GTK_POLICY_AUTOMATIC,
                                      GTK_POLICY_AUTOMATIC);
    gtk_widget_show(scrolled_win);
    gtk_container_add (GTK_CONTAINER (frame), scrolled_win);
    gtk_container_add (GTK_CONTAINER (vbox), frame);

    for(i=0;i<26;i++)
    {
    char buf[49];

    if(i)
	{
    	separator = gtk_hseparator_new ();
        gtk_widget_show (separator);
        gtk_box_pack_start (GTK_BOX (vbox_g), separator, TRUE, TRUE, 0);
	}

    labtitle[0]='A'+i;
    label=gtk_label_new(labtitle);
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (vbox_g), label, TRUE, TRUE, 0);

    entries[i]=entry = gtk_entry_new_with_max_length (48);
    gtk_signal_connect(GTK_OBJECT(entry), "activate",
		       GTK_SIGNAL_FUNC(enter_callback),
		       (void *)((long) i));
    if(shadow_markers[i]==-1)
	{
	sprintf(buf,"<None>");
	}
	else
	{
	reformat_time(buf, shadow_markers[i], time_dimension);
	}

    gtk_entry_set_text (GTK_ENTRY (entry), buf);
    gtk_box_pack_start (GTK_BOX (vbox_g), entry, TRUE, TRUE, 0);
    gtk_widget_show (entry);
    }

    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled_win), vbox_g);

    hbox = gtk_hbox_new (FALSE, 1);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);
    gtk_widget_show (hbox);

    button1 = gtk_button_new_with_label ("OK");
    gtk_widget_set_usize(button1, 100, -1);
    gtk_signal_connect(GTK_OBJECT (button1), "clicked",
			       GTK_SIGNAL_FUNC(ok_callback),
			       NULL);
    gtk_widget_show (button1);
    gtk_container_add (GTK_CONTAINER (hbox), button1);
    GTK_WIDGET_SET_FLAGS (button1, GTK_CAN_DEFAULT);
    gtk_signal_connect_object (GTK_OBJECT (button1),
                                "realize",
                             (GtkSignalFunc) gtk_widget_grab_default,
                             GTK_OBJECT (button1));


    button2 = gtk_button_new_with_label ("Cancel");
    gtk_widget_set_usize(button2, 100, -1);
    gtk_signal_connect(GTK_OBJECT (button2), "clicked",
			       GTK_SIGNAL_FUNC(destroy_callback),
			       NULL);
    GTK_WIDGET_SET_FLAGS (button2, GTK_CAN_DEFAULT);
    gtk_widget_show (button2);
    gtk_container_add (GTK_CONTAINER (hbox), button2);

    gtk_widget_show(window);
}
