/* 
 * Copyright (c) Tony Bybell 1999.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include <gtk/gtk.h>
#include "debug.h"
#include <string.h>

static GtkWidget *fs;
char **fileselbox_text=NULL;
char filesel_ok=0;
static GtkSignalFunc cleanup;

static void enter_callback(GtkWidget *widget, GtkFileSelection *fw)
{
char *allocbuf;
int alloclen;

allocbuf=gtk_file_selection_get_filename(GTK_FILE_SELECTION(fs));
if((alloclen=strlen(allocbuf)))
	{
	filesel_ok=1;
	if(*fileselbox_text) free_2(*fileselbox_text);
	*fileselbox_text=(char *)malloc_2(alloclen+1);
	strcpy(*fileselbox_text, allocbuf);
	}

DEBUG(printf("Filesel OK %s\n",allocbuf));
gtk_grab_remove(fs);
gtk_widget_destroy(fs);
cleanup();
}

static void cancel_callback(GtkWidget *widget, GtkWidget *nothing)
{
  DEBUG(printf("Filesel Entry Cancel\n"));
  gtk_grab_remove(fs);
  gtk_widget_destroy(fs);
}

static void destroy_callback(GtkWidget *widget, GtkWidget *nothing)
{
  DEBUG(printf("Filesel Destroy\n"));
}

void fileselbox(char *title, char **filesel_path, GtkSignalFunc func)
{
fileselbox_text=filesel_path;
filesel_ok=0;
cleanup=func;

fs=gtk_file_selection_new(title);
gtk_signal_connect(GTK_OBJECT(fs), "destroy", (GtkSignalFunc) destroy_callback, NULL);
gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(fs)->ok_button), 
	"clicked", (GtkSignalFunc) enter_callback, GTK_OBJECT(fs));
gtk_signal_connect_object(GTK_OBJECT(GTK_FILE_SELECTION(fs)->cancel_button),
	"clicked", (GtkSignalFunc) cancel_callback, GTK_OBJECT(fs));
gtk_file_selection_hide_fileop_buttons(GTK_FILE_SELECTION(fs));
if(*fileselbox_text) gtk_file_selection_set_filename(GTK_FILE_SELECTION(fs), *fileselbox_text);

gtk_grab_add(fs);
gtk_widget_show(fs);
}
