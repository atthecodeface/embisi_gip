/* 
 * Copyright (c) Tony Bybell 1999-2001.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include <gtk/gtk.h>
#include "analyzer.h"
#include "aet.h"
#include "vcd.h"
#include "debug.h"

char hier_grouping=1;

static GtkWidget *window;
static GtkWidget *entry_main;
static GtkWidget *clist;
static char bundle_direction=0;
static GtkSignalFunc cleanup;
static int num_rows=0;
static int selected_rows=0;

static GtkWidget *window1;
static GtkWidget *entry;
static char *entrybox_text_local=NULL;
static GtkSignalFunc cleanup_e;

static struct tree *h_selectedtree=NULL;
static struct tree *current_tree=NULL;
static struct treechain *treechain=NULL;

static int is_active=0;

int hier_searchbox_is_active(void)
{
return(is_active);
}


static void refresh_tree(struct tree *t) 
{
struct tree *t2;
GtkCList *cl;
int len;
int row;
int pixlen=0, maxpixlen=0;
static char *dotdot="..";
struct treechain *tc;

gtk_clist_freeze(cl=GTK_CLIST(clist));
gtk_clist_clear(cl);

num_rows=0;

if(t!=treeroot)
	{
	maxpixlen=gdk_string_measure(signalfont,(gchar *)(dotdot));
	}

if(!hier_grouping)
	{
        char *tmp, *tmp2, *tmp3;
	t2=t;
	while(t2)
		{
	                {
	                if(t2->child)
	                        {
	                        tmp=wave_alloca(strlen(t2->name)+5);
	                        strcpy(tmp,   "(+) ");
	                        strcpy(tmp+4, t2->name);
	                        }
                        	else
				if(t2->which!=-1)
				{
				if(facs[t2->which]->vec_root)
					{
					if(autocoalesce)
						{
						if(facs[t2->which]->vec_root!=facs[t2->which])
							{
							t2=t2->next;
							continue;
							}

						tmp2=makename_chain(facs[t2->which]);
						tmp3=leastsig_hiername(tmp2);
						tmp=wave_alloca(strlen(tmp3)+4);
						strcpy(tmp,   "[] ");
						strcpy(tmp+3, tmp3);
						free_2(tmp2);
						}	
						else
						{					
						tmp=wave_alloca(strlen(t2->name)+4);
						strcpy(tmp,   "[] ");
						strcpy(tmp+3, t2->name);
						}
					}
					else
					{
					tmp=t2->name;
					}				
				}
				else
	                        {
	                        tmp=t2->name;
	                        }
			row=gtk_clist_prepend(cl,(gchar **)&tmp);
			pixlen=gdk_string_measure(signalfont,(gchar *)(tmp));
			}
		maxpixlen=(pixlen>maxpixlen)?pixlen:maxpixlen;		
		gtk_clist_set_row_data(cl, row,t2); 
		num_rows++;
		t2=t2->next;
		}
	}
	else
	{
        char *tmp, *tmp2, *tmp3;

	t2=t;
	while(t2)
		{
                if(!t2->child)
                        {
			if(t2->which!=-1)
				{
				if(facs[t2->which]->vec_root)
					{
					if(autocoalesce)
						{
						if(facs[t2->which]->vec_root!=facs[t2->which])
							{
							t2=t2->next;
							continue;
							}

						tmp2=makename_chain(facs[t2->which]);
						tmp3=leastsig_hiername(tmp2);
						tmp=wave_alloca(strlen(tmp3)+4);
						strcpy(tmp,   "[] ");
						strcpy(tmp+3, tmp3);
						free_2(tmp2);
						}	
						else
						{					
						tmp=wave_alloca(strlen(t2->name)+4);
						strcpy(tmp,   "[] ");
						strcpy(tmp+3, t2->name);
						}
					}
					else
					{
					tmp=t2->name;
					}				
				}
				else
	                        {
	                        tmp=t2->name;
	                        }

			row=gtk_clist_prepend(cl,(gchar **)&tmp);
			pixlen=gdk_string_measure(signalfont,(gchar *)(tmp));
			maxpixlen=(pixlen>maxpixlen)?pixlen:maxpixlen;		
			gtk_clist_set_row_data(cl, row,t2); 
			num_rows++;
                        }
		t2=t2->next;
		}

	t2=t;
	while(t2)
		{
                if(t2->child)
                        {
                        tmp=wave_alloca(strlen(t2->name)+5);
                        strcpy(tmp,   "(+) ");
                        strcpy(tmp+4, t2->name);
			row=gtk_clist_prepend(cl,(gchar **)&tmp);
			pixlen=gdk_string_measure(signalfont,(gchar *)(tmp));
			maxpixlen=(pixlen>maxpixlen)?pixlen:maxpixlen;		
			gtk_clist_set_row_data(cl, row,t2); 
			num_rows++;
                        }
		t2=t2->next;
		}
	}


if(t!=treeroot)
	{
	row=gtk_clist_prepend(cl,(gchar **)&dotdot);
	gtk_clist_set_row_data(cl, row,NULL); 
	num_rows++;
	}

if(maxpixlen)gtk_clist_set_column_width(GTK_CLIST(clist),0,maxpixlen);
gtk_clist_thaw(cl);

if((tc=treechain))
	{
	char *buf;
	char hier_str[2];

	len=1;
	while(tc)
		{
		len+=strlen(tc->label->name);
		if(tc->next) len++;
		tc=tc->next;
		}

	buf=calloc_2(1,len);
	hier_str[0]=hier_delimeter;
	hier_str[1]=0;

	tc=treechain;
	while(tc)
		{
		strcat(buf,tc->label->name);
		if(tc->next) strcat(buf,hier_str);
		tc=tc->next;
		}
	gtk_entry_set_text(GTK_ENTRY(entry_main), buf);
	free_2(buf);
	}
	else
	{
	gtk_entry_set_text(GTK_ENTRY(entry_main),"");
	}
}


static void enter_callback_e(GtkWidget *widget, GtkWidget *nothing)
{
  gchar *entry_text;
  int len;
  entry_text = gtk_entry_get_text(GTK_ENTRY(entry));
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

    entry = gtk_entry_new_with_max_length (maxch);
    gtk_signal_connect(GTK_OBJECT(entry), "activate",
		       GTK_SIGNAL_FUNC(enter_callback_e),
		       entry);
    gtk_entry_set_text (GTK_ENTRY (entry), default_text);
    gtk_entry_select_region (GTK_ENTRY (entry),
			     0, GTK_ENTRY(entry)->text_length);
    gtk_box_pack_start (GTK_BOX (vbox), entry, TRUE, TRUE, 0);
    gtk_widget_show (entry);

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

static void ok_callback(GtkWidget *widget, GtkWidget *nothing)
{
int i;
 
if(!h_selectedtree) return;

for(i=fetchlow(h_selectedtree)->which;i<=fetchhigh(h_selectedtree)->which;i++)
        {
        struct symbol *s;  
        s=facs[i];
	if(s->vec_root)
		{
		s->vec_root->selected=autocoalesce;
		}
        }

for(i=fetchlow(h_selectedtree)->which;i<=fetchhigh(h_selectedtree)->which;i++)
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


static void insert_callback(GtkWidget *widget, GtkWidget *nothing)
{
Traces tcache;
int i;
    
if(!h_selectedtree) return;

memcpy(&tcache,&traces,sizeof(Traces));
traces.total=0; 
traces.first=traces.last=NULL;

for(i=fetchlow(h_selectedtree)->which;i<=fetchhigh(h_selectedtree)->which;i++)
        {
        struct symbol *s;  
        s=facs[i];
	if(s->vec_root)
		{
		s->vec_root->selected=autocoalesce;
		}
        }

for(i=fetchlow(h_selectedtree)->which;i<=fetchhigh(h_selectedtree)->which;i++)
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

if(!h_selectedtree) return;
        
memcpy(&tcache,&traces,sizeof(Traces));
traces.total=0;
traces.first=traces.last=NULL;  

for(i=fetchlow(h_selectedtree)->which;i<=fetchhigh(h_selectedtree)->which;i++)
        {
        struct symbol *s;  
        s=facs[i];
	if(s->vec_root)
		{
		s->vec_root->selected=autocoalesce;
		}
        }

for(i=fetchlow(h_selectedtree)->which;i<=fetchhigh(h_selectedtree)->which;i++)
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
 
tfirst=traces.first; tlast=traces.last; /* cache for highlighting */

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


static void
bundle_cleanup(GtkWidget *widget, gpointer data)
{
if(entrybox_text_local)  
        {
        char *efix;

	if(!strlen(entrybox_text_local))
		{
        	DEBUG(printf("Bundle name is not specified--recursing into hierarchy.\n"));
        	fetchvex(h_selectedtree, bundle_direction);
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
	                        fetchlow(h_selectedtree)->which,
	                        fetchhigh(h_selectedtree)->which,
	                        bundle_direction);
		}
        free_2(entrybox_text_local);
        }
        else
        {
        DEBUG(printf("Bundle name is not specified--recursing into hierarchy.\n"));
        fetchvex(h_selectedtree, bundle_direction);
        }  

MaxSignalLength();
signalarea_configure_event(signalarea, NULL);
wavearea_configure_event(wavearea, NULL);
}


static void
bundle_callback_generic(void)
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

/****************************************************************************/

static void select_row_callback(GtkWidget *widget, gint row, gint column,
	GdkEventButton *event, gpointer data)
{
struct tree *t;

t=(struct tree *)gtk_clist_get_row_data(GTK_CLIST(clist), row);
if(t)
	{
        h_selectedtree=t;
	DEBUG(printf("Selected: %s\n",t->name));
	if(t->child)
		{
		struct treechain *tc, *tc2;

		tc=treechain;
		if(tc)
			{
			while(tc->next) tc=tc->next;			

    			tc2=calloc_2(1,sizeof(struct treechain));
			tc2->label=t;
			tc2->tree=current_tree;
			tc->next=tc2;
			}
			else
			{
    			treechain=calloc_2(1,sizeof(struct treechain));
    			treechain->tree=current_tree;
			treechain->label=t;
			}

		current_tree=t->child;
		refresh_tree(current_tree);
		}
	}
	else
	{
	struct treechain *tc;

	h_selectedtree=NULL;
	tc=treechain;
	if(tc)
		{
		for(;;)
			{
			if(tc->next)
				{
				if(tc->next->next)
					{
					tc=tc->next;
					continue;
					}
					else
					{
					current_tree=tc->next->tree;
					free_2(tc->next);
					tc->next=NULL;
					break;
					}					
				}
				else
				{
				free_2(tc);
				treechain=NULL;
				current_tree=treeroot;
				break;
				}
					
			}
		refresh_tree(current_tree);
		}
	}

}

static void unselect_row_callback(GtkWidget *widget, gint row, gint column,
	GdkEventButton *event, gpointer data)
{
struct tree *t;

t=(struct tree *)gtk_clist_get_row_data(GTK_CLIST(clist), row);
h_selectedtree=NULL;

if(t)
	{
	DEBUG(printf("Unselected: %s\n",t->name));
	}
	else
	{
	/* just ignore */
	}
}


static void destroy_callback(GtkWidget *widget, GtkWidget *nothing)
{
  is_active=0;
  gtk_widget_destroy(window);
}


/*
 * mainline..
 */
void hier_searchbox(char *title, GtkSignalFunc func)
{
    GtkWidget *scrolled_win;
    GtkWidget *vbox1, *hbox;
    GtkWidget *button1, *button2, *button3, *button3a, *button4, *button5;
    GtkWidget *label;
    gchar *titles[]={"Children"};
    GtkWidget *frame1, *frame2, *frameh;
    GtkWidget *table;
    GtkTooltips *tooltips;

    if(is_active) 
	{
	gdk_window_raise(window->window);
	return;
	}

    is_active=1;
    cleanup=func;
    num_rows=selected_rows=0;

    /* create a new modal window */
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW (window), title);
    gtk_signal_connect(GTK_OBJECT (window), "delete_event",
                       (GtkSignalFunc) destroy_callback, NULL);

    tooltips=gtk_tooltips_new_2();

    table = gtk_table_new (256, 1, FALSE);
    gtk_widget_show (table);

    vbox1 = gtk_vbox_new (FALSE, 0);
    gtk_container_border_width (GTK_CONTAINER (vbox1), 3);
    gtk_widget_show (vbox1);
    frame1 = gtk_frame_new (NULL);
    gtk_container_border_width (GTK_CONTAINER (frame1), 3);
    gtk_widget_show(frame1);
    gtk_table_attach (GTK_TABLE (table), frame1, 0, 1, 0, 1,
                        GTK_FILL | GTK_EXPAND,
                        GTK_FILL | GTK_EXPAND | GTK_SHRINK, 1, 1);

    label=gtk_label_new("Signal Hierarchy");
    gtk_widget_show(label);

    gtk_box_pack_start (GTK_BOX (vbox1), label, TRUE, TRUE, 0);

    entry_main = gtk_entry_new();
    gtk_entry_set_editable(GTK_ENTRY(entry_main), FALSE);
    gtk_widget_show (entry_main);
    gtk_tooltips_set_tip_2(tooltips, entry_main, 
		"The hierarchy is built here by clicking on the appropriate "
		"items below in the scrollable window.  Click on \"..\" to "
		"go up a level."
		,NULL);

    gtk_box_pack_start (GTK_BOX (vbox1), entry_main, TRUE, TRUE, 0);
    gtk_container_add (GTK_CONTAINER (frame1), vbox1);

    frame2 = gtk_frame_new (NULL);
    gtk_container_border_width (GTK_CONTAINER (frame2), 3);
    gtk_widget_show(frame2);

    gtk_table_attach (GTK_TABLE (table), frame2, 0, 1, 1, 254,
                        GTK_FILL | GTK_EXPAND,
                        GTK_FILL | GTK_EXPAND | GTK_SHRINK, 1, 1);

    clist=gtk_clist_new_with_titles(1,titles);
    gtk_clist_column_titles_passive(GTK_CLIST(clist)); 

    gtk_clist_set_selection_mode(GTK_CLIST(clist), GTK_SELECTION_SINGLE);
    gtk_signal_connect_object (GTK_OBJECT (clist), "select_row",
			       GTK_SIGNAL_FUNC(select_row_callback),
			       NULL);
    gtk_signal_connect_object (GTK_OBJECT (clist), "unselect_row",
			       GTK_SIGNAL_FUNC(unselect_row_callback),
			       NULL);
    gtk_widget_show (clist);

    scrolled_win = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
                                      GTK_POLICY_AUTOMATIC,
                                      GTK_POLICY_AUTOMATIC);
    gtk_widget_set_usize( GTK_WIDGET (scrolled_win), -1, 300);
    gtk_widget_show(scrolled_win);

    /* gtk_scrolled_window_add_with_viewport doesn't seen to work right here.. */
    gtk_container_add (GTK_CONTAINER (scrolled_win), clist);

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
		"Add selected signals to end of the display on the main window.",NULL);

    gtk_box_pack_start (GTK_BOX (hbox), button1, TRUE, FALSE, 0);

    button2 = gtk_button_new_with_label (" Insert ");
    gtk_container_border_width (GTK_CONTAINER (button2), 3);
    gtk_signal_connect_object (GTK_OBJECT (button2), "clicked",
			       GTK_SIGNAL_FUNC(insert_callback),
			       GTK_OBJECT (window));
    gtk_widget_show (button2);
    gtk_tooltips_set_tip_2(tooltips, button2, 
		"Add children after last highlighted signal on the main window.",NULL);
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
		"Bundle children into a single bit vector with the topmost signal as the LSB and the lowest as the MSB.",NULL);
    	gtk_box_pack_start (GTK_BOX (hbox), button3, TRUE, FALSE, 0);

    	button3a = gtk_button_new_with_label (" Bundle Down ");
    	gtk_container_border_width (GTK_CONTAINER (button3a), 3);
    	gtk_signal_connect_object (GTK_OBJECT (button3a), "clicked",
			       GTK_SIGNAL_FUNC(bundle_callback_down),
			       GTK_OBJECT (window));
    	gtk_widget_show (button3a);
    	gtk_tooltips_set_tip_2(tooltips, button3a, 
		"Bundle children into a single bit vector with the topmost signal as the MSB and the lowest as the LSB.",NULL);
    	gtk_box_pack_start (GTK_BOX (hbox), button3a, TRUE, FALSE, 0);
	}

    button4 = gtk_button_new_with_label (" Replace ");
    gtk_container_border_width (GTK_CONTAINER (button4), 3);
    gtk_signal_connect_object (GTK_OBJECT (button4), "clicked",
			       GTK_SIGNAL_FUNC(replace_callback),
			       GTK_OBJECT (window));
    gtk_widget_show (button4);
    gtk_tooltips_set_tip_2(tooltips, button4, 
		"Replace highlighted signals on the main window with children shown above.",NULL);
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

    if(!current_tree) 
	{
	current_tree=treeroot;
    	h_selectedtree=NULL;
	}

    refresh_tree(current_tree);
}
