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
#include "ae2.h"
   
/* Add some text to our text widget - this is a callback that is invoked
when our window is realized. We could also force our window to be
realized with gtk_widget_realize, but it would have to be part of
a hierarchy first */

static GtkWidget *text;
static GtkWidget *vscrollbar;

void status_text(char *str)
{
gtk_text_insert (GTK_TEXT (text), NULL, &text->style->black, NULL, str, -1);
}

void
realize_text (GtkWidget *text, gpointer data)
{
char buf[128];

if(is_vcd)
	{
	status_text("VCD loaded successfully.\n");
	}
else
if(is_lxt)
	{
	status_text("LXT loaded successfully.\n");
	}
else
if(is_ae2)
	{
	status_text("AET loaded successfully.\n");
	}

sprintf(buf,"[%d] facilities found.\n",numfacs);
status_text(buf);

if(is_vcd)
	{
	sprintf(buf,"[%d] regions found.\n",regions);
	status_text(buf);
	}
	else
	{
	sprintf(buf,"Regions loaded on demand.\n");
	status_text(buf);
	}
}
   
/* Create a scrolled text area that displays a "message" */
GtkWidget *
create_text (void)
{
GtkWidget *table;

GtkTooltips *tooltips;

tooltips=gtk_tooltips_new_2();
gtk_tooltips_set_delay_2(tooltips,1500);

/* Create a table to hold the text widget and scrollbars */
table = gtk_table_new (1, 16, FALSE);
   
/* Put a text widget in the upper left hand corner. Note the use of
* GTK_SHRINK in the y direction */
text = gtk_text_new (NULL, NULL);
gtk_table_attach (GTK_TABLE (table), text, 0, 14, 0, 1,
		      	GTK_FILL | GTK_EXPAND,
		      	GTK_FILL | GTK_SHRINK | GTK_EXPAND, 0, 0);
gtk_widget_set_usize(GTK_WIDGET(text), 100, 50); 
gtk_widget_show (text);

/* And a VScrollbar in the upper right */
vscrollbar = gtk_vscrollbar_new ((GTK_TEXT (text))->vadj);
gtk_table_attach (GTK_TABLE (table), vscrollbar, 15, 16, 0, 1,
			GTK_FILL, GTK_FILL | GTK_SHRINK | GTK_EXPAND, 0, 0);
gtk_widget_show (vscrollbar);
   
/* Add a handler to put a message in the text widget when it is realized */
gtk_signal_connect (GTK_OBJECT (text), "realize",
			GTK_SIGNAL_FUNC (realize_text), NULL);
   
gtk_tooltips_set_tip_2(tooltips, text, "Status Window", NULL);
return(table);
}
   
