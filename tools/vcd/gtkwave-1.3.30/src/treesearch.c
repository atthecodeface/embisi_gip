/* 
 * Copyright (c) Tony Bybell 1999.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include <gtk/gtk.h>
#include "analyzer.h"
#include "tree.h"
#include "aet.h"
#include "vcd.h"
#include "debug.h"

char autoname_bundles=0;

static GtkWidget *window1;
static GtkWidget *entry_a;
static char *entrybox_text_local=NULL;
static GtkSignalFunc cleanup_e;
static struct tree *selectedtree=NULL;


static int is_active=0;
GtkCTree *ctree_main=NULL;

static void select_row_callback(GtkWidget *widget, gint row, gint column,
        GdkEventButton *event, gpointer data)
{
struct tree *t;

t=(struct tree *)gtk_clist_get_row_data(GTK_CLIST(ctree_main), row);
DEBUG(printf("TS: %08x %s\n",t,t->name));
selectedtree=t;
}

static void unselect_row_callback(GtkWidget *widget, gint row, gint column,
        GdkEventButton *event, gpointer data)
{
struct tree *t;

t=(struct tree *)gtk_clist_get_row_data(GTK_CLIST(ctree_main), row);
DEBUG(printf("TU: %08x %s\n",t,t->name));
selectedtree=NULL;
}




int treebox_is_active(void)
{
return(is_active);
}

static void enter_callback_e(GtkWidget *widget, GtkWidget *nothing)
{
  gchar *entry_text;
  int len;
  entry_text = gtk_entry_get_text(GTK_ENTRY(entry_a));
  DEBUG(printf("Entry contents: %s\n", entry_text));
  if(!(len=strlen(entry_text))) entrybox_text_local=NULL;
	else strcpy((entrybox_text_local=(char *)malloc_2(len+1)),entry_text);

  gtk_grab_remove(window1);
  gtk_widget_destroy(window1);

  cleanup_e();
}

static void destroy_callback_e(GtkWidget *widget, GtkWidget *nothing)
{
  DEBUG(printf("Entry Cancel\n"));
  entrybox_text_local=NULL;
  gtk_grab_remove(window1);
  gtk_widget_destroy(window1);
}

static void entrybox_local(char *title, int width, char *default_text, int maxch, GtkSignalFunc func)
{
    GtkWidget *vbox, *hbox;
    GtkWidget *button1, *button2;

    cleanup_e=func;

    /* create a new modal window */
    window1 = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_grab_add(window1);
    gtk_widget_set_usize( GTK_WIDGET (window1), width, 60);
    gtk_window_set_title(GTK_WINDOW (window1), title);
    gtk_signal_connect(GTK_OBJECT (window1), "delete_event",
                       (GtkSignalFunc) destroy_callback_e, NULL);

    vbox = gtk_vbox_new (FALSE, 0);
    gtk_container_add (GTK_CONTAINER (window1), vbox);
    gtk_widget_show (vbox);

    entry_a = gtk_entry_new_with_max_length (maxch);
    gtk_signal_connect(GTK_OBJECT(entry_a), "activate",
		       GTK_SIGNAL_FUNC(enter_callback_e),
		       entry_a);
    gtk_entry_set_text (GTK_ENTRY (entry_a), default_text);
    gtk_entry_select_region (GTK_ENTRY (entry_a),
			     0, GTK_ENTRY(entry_a)->text_length);
    gtk_box_pack_start (GTK_BOX (vbox), entry_a, TRUE, TRUE, 0);
    gtk_widget_show (entry_a);

    hbox = gtk_hbox_new (FALSE, 1);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);
    gtk_widget_show (hbox);

    button1 = gtk_button_new_with_label ("OK");
    gtk_widget_set_usize(button1, 100, -1);
    gtk_signal_connect(GTK_OBJECT (button1), "clicked",
			       GTK_SIGNAL_FUNC(enter_callback_e),
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
			       GTK_SIGNAL_FUNC(destroy_callback_e),
			       NULL);
    GTK_WIDGET_SET_FLAGS (button2, GTK_CAN_DEFAULT);
    gtk_widget_show (button2);
    gtk_container_add (GTK_CONTAINER (hbox), button2);

    gtk_widget_show(window1);
}

/***************************************************************************/

static GtkWidget *window;
static GtkWidget *entry;
static GtkWidget *tree;
static char bundle_direction=0;
static GtkSignalFunc cleanup;


struct tree *fetchhigh(struct tree *t)
{
while(t->child) t=t->child;
return(t);
}

struct tree *fetchlow(struct tree *t)
{
if(t->child) 
	{
	t=t->child;

	for(;;)
		{
		while(t->next) t=t->next;
		if(t->child) t=t->child; else break;
		}
	}
return(t);
}

static void fetchvex2(struct tree *t, char direction, char level)
{
while(t)
	{
	if(t->child)
		{
		if(t->child->child)
			{
			fetchvex2(t->child, direction, 1);
			}
			else
			{
			add_vector_range(NULL, fetchlow(t)->which,
				fetchhigh(t)->which, direction);
			}
		}
	if(level) { t=t->next; } else { break; }
	}
}

void fetchvex(struct tree *t, char direction)
{
if(t)
if(t->child)
	{
	fetchvex2(t, direction, 0);
	}
	else
	{
	add_vector_range(NULL, fetchlow(t)->which, 
		fetchhigh(t)->which, direction);
	}
}




/* call cleanup() on ok/insert functions */

static void
bundle_cleanup(GtkWidget *widget, gpointer data)
{ 
if(entrybox_text_local) 
        {
        char *efix;
 
	if(!strlen(entrybox_text_local))
		{
	        DEBUG(printf("Bundle name is not specified--recursing into hierarchy.\n"));
		fetchvex(selectedtree, bundle_direction);
		}
		else
		{
	        efix=entrybox_text_local;
	        while(*efix)
	                {
	                if(*efix==' ')
	                        {
	                        *efix='_';
	                        }
	                efix++;
	                }
	 
	        DEBUG(printf("Bundle name is: %s\n",entrybox_text_local));
	        add_vector_range(entrybox_text_local, 
				fetchlow(selectedtree)->which,
				fetchhigh(selectedtree)->which, 
				bundle_direction);
		}
        free_2(entrybox_text_local);
        }
	else
	{
        DEBUG(printf("Bundle name is not specified--recursing into hierarchy.\n"));
	fetchvex(selectedtree, bundle_direction);
	}

MaxSignalLength();
signalarea_configure_event(signalarea, NULL);
wavearea_configure_event(wavearea, NULL);
}
 
static void
bundle_callback_generic(void)
{
if(selectedtree)
	{
	if(!autoname_bundles)
	        {
	        entrybox_local("Enter Bundle Name",300,"",128,bundle_cleanup);
	        }
	        else
	        {
	        entrybox_text_local=NULL;
	        bundle_cleanup(NULL, NULL);
	        }
	}
}

static void
bundle_callback_up(GtkWidget *widget, gpointer data)
{
bundle_direction=0;
bundle_callback_generic();
}

static void
bundle_callback_down(GtkWidget *widget, gpointer data)
{
bundle_direction=1;
bundle_callback_generic();
}

static void insert_callback(GtkWidget *widget, GtkWidget *nothing)
{
Traces tcache;
int i;

if(!selectedtree) return;

memcpy(&tcache,&traces,sizeof(Traces));
traces.total=0;
traces.first=traces.last=NULL;

for(i=fetchlow(selectedtree)->which;i<=fetchhigh(selectedtree)->which;i++)
        {
        struct symbol *s;  
        s=facs[i];
	if(s->vec_root)
		{
		s->vec_root->selected=autocoalesce;
		}
        }

for(i=fetchlow(selectedtree)->which;i<=fetchhigh(selectedtree)->which;i++)
        {
	int len;
        struct symbol *s, *t;  
        s=facs[i];
	t=s->vec_root;
	if((t)&&(autocoalesce))
		{
		if(t->selected)
			{
			t->selected=0;
			len=0;
			while(t)
				{
				len++;
				t=t->vec_chain;
				}
			if(len) add_vector_chain(s->vec_root, len);
			}
		}
		else
		{
	        AddNode(s->n, NULL);  
		}
        }

traces.buffercount=traces.total;
traces.buffer=traces.first;
traces.bufferlast=traces.last;
traces.first=tcache.first;
traces.last=tcache.last;
traces.total=tcache.total;

PasteBuffer();

traces.buffercount=tcache.buffercount;
traces.buffer=tcache.buffer;
traces.bufferlast=tcache.bufferlast;

MaxSignalLength();
signalarea_configure_event(signalarea, NULL);
wavearea_configure_event(wavearea, NULL);
}

static void replace_callback(GtkWidget *widget, GtkWidget *nothing)
{
Traces tcache;
int i;
Trptr tfirst=NULL, tlast=NULL;

if(!selectedtree) return;

memcpy(&tcache,&traces,sizeof(Traces));
traces.total=0;
traces.first=traces.last=NULL;

for(i=fetchlow(selectedtree)->which;i<=fetchhigh(selectedtree)->which;i++)
        {
        struct symbol *s;  
        s=facs[i];
	if(s->vec_root)
		{
		s->vec_root->selected=autocoalesce;
		}
        }

for(i=fetchlow(selectedtree)->which;i<=fetchhigh(selectedtree)->which;i++)
        {
	int len;
        struct symbol *s, *t;  
        s=facs[i];
	t=s->vec_root;
	if((t)&&(autocoalesce))
		{
		if(t->selected)
			{
			t->selected=0;
			len=0;
			while(t)
				{
				len++;
				t=t->vec_chain;
				}
			if(len) add_vector_chain(s->vec_root, len);
			}
		}
		else
		{
	        AddNode(s->n, NULL);  
		}
        }

tfirst=traces.first; tlast=traces.last;	/* cache for highlighting */

traces.buffercount=traces.total;
traces.buffer=traces.first;
traces.bufferlast=traces.last;
traces.first=tcache.first;
traces.last=tcache.last;
traces.total=tcache.total;

PasteBuffer();

traces.buffercount=tcache.buffercount;
traces.buffer=tcache.buffer;
traces.bufferlast=tcache.bufferlast;

CutBuffer();

while(tfirst)
	{
	tfirst->flags |= TR_HIGHLIGHT;
	if(tfirst==tlast) break;
	tfirst=tfirst->next;
	}

MaxSignalLength();
signalarea_configure_event(signalarea, NULL);
wavearea_configure_event(wavearea, NULL);
}

static void ok_callback(GtkWidget *widget, GtkWidget *nothing)
{
int i;

if(!selectedtree) return;

for(i=fetchlow(selectedtree)->which;i<=fetchhigh(selectedtree)->which;i++)
        {
        struct symbol *s;  
        s=facs[i];
	if(s->vec_root)
		{
		s->vec_root->selected=autocoalesce;
		}
        }

for(i=fetchlow(selectedtree)->which;i<=fetchhigh(selectedtree)->which;i++)
        {
	int len;
        struct symbol *s, *t;  
        s=facs[i];
	t=s->vec_root;
	if((t)&&(autocoalesce))
		{
		if(t->selected)
			{
			t->selected=0;
			len=0;
			while(t)
				{
				len++;
				t=t->vec_chain;
				}
			if(len) add_vector_chain(s->vec_root, len);
			}
		}
		else
		{
	        AddNode(s->n, NULL);  
		}
        }

MaxSignalLength();
signalarea_configure_event(signalarea, NULL);
wavearea_configure_event(wavearea, NULL);
}


static void destroy_callback(GtkWidget *widget, GtkWidget *nothing)
{
  is_active=0;
  gtk_widget_destroy(window);
}



/*
 * mainline..
 */
void treebox(char *title, GtkSignalFunc func)
{
    GtkWidget *scrolled_win;
    GtkWidget *hbox;
    GtkWidget *button1, *button2, *button3, *button3a, *button4, *button5;
    GtkWidget *frame2, *frameh;
    GtkWidget *table;
    GtkTooltips *tooltips;
    GtkCList  *clist;

    if(is_active) 
	{
	gdk_window_raise(window->window);
	return;
	}

    is_active=1;
    cleanup=func;

    /* create a new modal window */
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW (window), title);
    gtk_signal_connect(GTK_OBJECT (window), "delete_event",
                       (GtkSignalFunc) destroy_callback, NULL);

    tooltips=gtk_tooltips_new_2();

    table = gtk_table_new (256, 1, FALSE);
    gtk_widget_show (table);

    frame2 = gtk_frame_new (NULL);
    gtk_container_border_width (GTK_CONTAINER (frame2), 3);
    gtk_widget_show(frame2);

    gtk_table_attach (GTK_TABLE (table), frame2, 0, 1, 0, 255,
                        GTK_FILL | GTK_EXPAND,
                        GTK_FILL | GTK_EXPAND | GTK_SHRINK, 1, 1);

    tree=gtk_ctree_new(1,0);
    ctree_main=GTK_CTREE(tree);
    gtk_clist_set_column_auto_resize (GTK_CLIST (tree), 0, TRUE);
    gtk_widget_show(tree);

    clist=GTK_CLIST(tree);
    gtk_signal_connect_object (GTK_OBJECT (clist), "select_row",
                               GTK_SIGNAL_FUNC(select_row_callback),
                               NULL);
    gtk_signal_connect_object (GTK_OBJECT (clist), "unselect_row",
                               GTK_SIGNAL_FUNC(unselect_row_callback),
                               NULL);

    gtk_clist_freeze(clist);
    gtk_clist_clear(clist);

    maketree(NULL, treeroot);
    gtk_clist_thaw(clist);
    selectedtree=NULL;

    scrolled_win = gtk_scrolled_window_new (NULL, NULL);
    gtk_widget_set_usize( GTK_WIDGET (scrolled_win), -1, 300);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
                                      GTK_POLICY_AUTOMATIC,
                                      GTK_POLICY_AUTOMATIC);
    gtk_widget_show(scrolled_win);
    gtk_container_add (GTK_CONTAINER (scrolled_win), GTK_WIDGET (tree));
    gtk_container_add (GTK_CONTAINER (frame2), scrolled_win);


    frameh = gtk_frame_new (NULL);
    gtk_container_border_width (GTK_CONTAINER (frameh), 3);
    gtk_widget_show(frameh);
    gtk_table_attach (GTK_TABLE (table), frameh, 0, 1, 255, 256,
                        GTK_FILL | GTK_EXPAND,
                        GTK_FILL | GTK_EXPAND | GTK_SHRINK, 1, 1);


    hbox = gtk_hbox_new (FALSE, 1);
    gtk_widget_show (hbox);

    button1 = gtk_button_new_with_label ("Append");
    gtk_container_border_width (GTK_CONTAINER (button1), 3);
    gtk_signal_connect_object (GTK_OBJECT (button1), "clicked",
			       GTK_SIGNAL_FUNC(ok_callback),
			       GTK_OBJECT (window));
    gtk_widget_show (button1);
    gtk_tooltips_set_tip_2(tooltips, button1, 
		"Add selected signal hierarchy to end of the display on the main window.",NULL);

    gtk_box_pack_start (GTK_BOX (hbox), button1, TRUE, FALSE, 0);

    button2 = gtk_button_new_with_label (" Insert ");
    gtk_container_border_width (GTK_CONTAINER (button2), 3);
    gtk_signal_connect_object (GTK_OBJECT (button2), "clicked",
			       GTK_SIGNAL_FUNC(insert_callback),
			       GTK_OBJECT (window));
    gtk_widget_show (button2);
    gtk_tooltips_set_tip_2(tooltips, button2, 
		"Add selected signal hierarchy after last highlighted signal on the main window.",NULL);
    gtk_box_pack_start (GTK_BOX (hbox), button2, TRUE, FALSE, 0);

    if(vcd_explicit_zero_subscripts>=0)
	{
    	button3 = gtk_button_new_with_label (" Bundle Up ");
    	gtk_container_border_width (GTK_CONTAINER (button3), 3);
    	gtk_signal_connect_object (GTK_OBJECT (button3), "clicked",
			       GTK_SIGNAL_FUNC(bundle_callback_up),
			       GTK_OBJECT (window));
    	gtk_widget_show (button3);
    	gtk_tooltips_set_tip_2(tooltips, button3, 
		"Bundle selected signal hierarchy into a single bit "
		"vector with the topmost signal as the LSB and the "
		"lowest as the MSB.  Entering a zero length bundle "
		"name will reconstruct the individual vectors "
		"in the hierarchy.  Otherwise, all the bits in "
		"the hierarchy will be coalesced with the supplied "
		"name into a single vector.",NULL);
    	gtk_box_pack_start (GTK_BOX (hbox), button3, TRUE, FALSE, 0);

    	button3a = gtk_button_new_with_label (" Bundle Down ");
    	gtk_container_border_width (GTK_CONTAINER (button3a), 3);
    	gtk_signal_connect_object (GTK_OBJECT (button3a), "clicked",
			       GTK_SIGNAL_FUNC(bundle_callback_down),
			       GTK_OBJECT (window));
    	gtk_widget_show (button3a);
    	gtk_tooltips_set_tip_2(tooltips, button3a, 
		"Bundle selected signal hierarchy into a single bit "
		"vector with the topmost signal as the MSB and the "
		"lowest as the LSB.  Entering a zero length bundle "
		"name will reconstruct the individual vectors "
		"in the hierarchy.  Otherwise, all the bits in "
		"the hierarchy will be coalesced with the supplied "
		"name into a single vector.",NULL);
   	gtk_box_pack_start (GTK_BOX (hbox), button3a, TRUE, FALSE, 0);
	}

    button4 = gtk_button_new_with_label (" Replace ");
    gtk_container_border_width (GTK_CONTAINER (button4), 3);
    gtk_signal_connect_object (GTK_OBJECT (button4), "clicked",
			       GTK_SIGNAL_FUNC(replace_callback),
			       GTK_OBJECT (window));
    gtk_widget_show (button4);
    gtk_tooltips_set_tip_2(tooltips, button4, 
		"Replace highlighted signals on the main window with signals selected above.",NULL);
    gtk_box_pack_start (GTK_BOX (hbox), button4, TRUE, FALSE, 0);

    button5 = gtk_button_new_with_label (" Exit ");
    gtk_container_border_width (GTK_CONTAINER (button5), 3);
    gtk_signal_connect_object (GTK_OBJECT (button5), "clicked",
			       GTK_SIGNAL_FUNC(destroy_callback),
			       GTK_OBJECT (window));
    gtk_tooltips_set_tip_2(tooltips, button5, 
		"Do nothing and return to the main window.",NULL);
    gtk_widget_show (button5);
    gtk_box_pack_start (GTK_BOX (hbox), button5, TRUE, FALSE, 0);

    gtk_container_add (GTK_CONTAINER (frameh), hbox);
    gtk_container_add (GTK_CONTAINER (window), table);

    gtk_widget_show(window);
}



