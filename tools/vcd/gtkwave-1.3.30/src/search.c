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
#include "aet.h"
#include "vcd.h"
#include "debug.h"

static GtkWidget *window1;
static GtkWidget *entry_a;
static char *entrybox_text_local=NULL;
static GtkSignalFunc cleanup_e;
SearchProgressData *pdata;

static int is_active=0;

static char is_insert_running = 0;
static char is_replace_running = 0;
static char is_append_running = 0;
static char is_searching_running = 0;


int searchbox_is_active(void)
{
return(is_active);
}

static void enter_callback_e(GtkWidget *widget, GtkWidget *nothing)
{
  gchar *entry_text;
  int len;
  char *vname="<Vector>";
  entry_text = gtk_entry_get_text(GTK_ENTRY(entry_a));
  DEBUG(printf("Entry contents: %s\n", entry_text));
  if(!(len=strlen(entry_text)))
	strcpy((entrybox_text_local=(char *)malloc_2(strlen(vname)+1)),vname);	/* make consistent with other widgets rather than producing NULL */
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

static char *regex_type[]={"\\(\\[.*\\]\\)*$", "\\>.\\([0-9]\\)*$", 
			   "\\(\\[.*\\]\\)*$", "\\>.\\([0-9]\\)*$", ""};
static char *regex_name[]={"WRange", "WStrand", "Range", "Strand", "None"};
static char regex_mutex[5]={0,0,0,0,0};
static int  regex_which=0;

static void regex_clicked(GtkWidget *widget, gpointer which)
{
int i;
char *which_char;
 
for(i=0;i<5;i++) regex_mutex[i]=0;
which_char=(char *)which;
*which_char=1;                  /* mark our choice */

regex_which=which_char-regex_mutex;
  
DEBUG(printf("picked: %s\n", regex_name[regex_which]));
}  

/***************************************************************************/


static GtkWidget *window;
static GtkWidget *entry;
static GtkWidget *clist;
static char default_null_searchbox_text = 0;
static char *searchbox_text=&default_null_searchbox_text;
/*static char *searchbox_text="";*/
static char bundle_direction=0;
static GtkSignalFunc cleanup;
static int num_rows=0;
static int selected_rows=0;

/* call cleanup() on ok/insert functions */

static void
bundle_cleanup(GtkWidget *widget, gpointer data)
{   
if(entrybox_text_local)
	{
	char *efix;

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
	add_vector_selected(entrybox_text_local, selected_rows, bundle_direction);
	free_2(entrybox_text_local);
	}

MaxSignalLength();
signalarea_configure_event(signalarea, NULL);
wavearea_configure_event(wavearea, NULL);
}
 
static void
bundle_callback_generic(void)
{
DEBUG(printf("Selected_rows: %d\n",selected_rows));
if(selected_rows>0)
	{
	entrybox_local("Enter Bundle Name",300,"",128,bundle_cleanup);
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
struct symchain *symc, *symc_current;
int i;

gfloat interval;

if(is_insert_running) return;
is_insert_running = ~0;
gtk_grab_add(widget);

symc=NULL;

memcpy(&tcache,&traces,sizeof(Traces));
traces.total=0;
traces.first=traces.last=NULL;

GTK_ADJUSTMENT(pdata->adj)->upper = (gfloat)((num_rows>1)?num_rows-1:1);
interval = (gfloat)(num_rows/100.0);

for(i=0;i<num_rows;i++)
	{
	int len;
	struct symbol *s, *t;
	s=(struct symbol *)gtk_clist_get_row_data(GTK_CLIST(clist), i);
	if(s->selected)
		{
		pdata->value = i;
		if(((int)(pdata->value/interval))!=((int)(pdata->oldvalue/interval)))		
			{
			gtk_progress_set_value (GTK_PROGRESS (pdata->pbar), i);
			while (gtk_events_pending()) gtk_main_iteration();
			}
		pdata->oldvalue = i;

		if((!s->vec_root)||(!autocoalesce))
			{
			AddNode(s->n, NULL);
			}
			else
			{
			len=0;
			t=s->vec_root;
			while(t)
				{
				if(t->selected)
					{
					if(len) t->selected=0;
					symc_current=(struct symchain *)calloc_2(1,sizeof(struct symchain));	
					symc_current->next=symc;
					symc_current->symbol=t;
					symc=symc_current;
					}
				len++;
				t=t->vec_chain;
				}
			if(len)add_vector_chain(s->vec_root, len);			
			}
		}
	}

while(symc)
	{
	symc->symbol->selected=1;
	symc_current=symc;
	symc=symc->next;
	free_2(symc_current);
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

gtk_progress_set_value (GTK_PROGRESS (pdata->pbar), 0.0);
pdata->oldvalue = -1.0;
gtk_grab_remove(widget);
is_insert_running=0;
}

static void replace_callback(GtkWidget *widget, GtkWidget *nothing)
{
Traces tcache;
int i;
Trptr tfirst, tlast;
struct symchain *symc, *symc_current;

gfloat interval;

if(is_replace_running) return;
is_replace_running = ~0;
gtk_grab_add(widget);


tfirst=NULL; tlast=NULL;
symc=NULL;
memcpy(&tcache,&traces,sizeof(Traces));
traces.total=0;
traces.first=traces.last=NULL;

GTK_ADJUSTMENT(pdata->adj)->upper = (gfloat)((num_rows>1)?num_rows-1:1);
interval = (gfloat)(num_rows/100.0);
pdata->oldvalue = -1.0;

for(i=0;i<num_rows;i++)
	{
	int len;
	struct symbol *s, *t;
	s=(struct symbol *)gtk_clist_get_row_data(GTK_CLIST(clist), i);
	if(s->selected)
		{
                pdata->value = i;
                if(((int)(pdata->value/interval))!=((int)(pdata->oldvalue/interval)))
                        {
                        gtk_progress_set_value (GTK_PROGRESS (pdata->pbar), i);
                        while (gtk_events_pending()) gtk_main_iteration();
                        }
                pdata->oldvalue = i;

		if((!s->vec_root)||(!autocoalesce))
			{
			AddNode(s->n, NULL);
			}
			else
			{
			len=0;
			t=s->vec_root;
			while(t)
				{
				if(t->selected)
					{
					if(len) t->selected=0;
					symc_current=(struct symchain *)calloc_2(1,sizeof(struct symchain));	
					symc_current->next=symc;
					symc_current->symbol=t;
					symc=symc_current;
					}
				len++;
				t=t->vec_chain;
				}
			if(len)add_vector_chain(s->vec_root, len);			
			}
		}
	}

while(symc)
	{
	symc->symbol->selected=1;
	symc_current=symc;
	symc=symc->next;
	free_2(symc_current);
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

gtk_progress_set_value (GTK_PROGRESS (pdata->pbar), 0.0);
pdata->oldvalue = -1.0;  
gtk_grab_remove(widget);
is_replace_running=0;
}

static void ok_callback(GtkWidget *widget, GtkWidget *nothing)
{
int i;
struct symchain *symc, *symc_current;

gfloat interval;

if(is_append_running) return;
is_append_running = ~0;
gtk_grab_add(widget);

symc=NULL;

GTK_ADJUSTMENT(pdata->adj)->upper = (gfloat)((num_rows>1)?num_rows-1:1);
interval = (gfloat)(num_rows/100.0);
pdata->oldvalue = -1.0;

for(i=0;i<num_rows;i++)
	{
	int len;
	struct symbol *s, *t;
	s=(struct symbol *)gtk_clist_get_row_data(GTK_CLIST(clist), i);
	if(s->selected)
		{
                pdata->value = i;
                if(((int)(pdata->value/interval))!=((int)(pdata->oldvalue/interval)))
                        {
                        gtk_progress_set_value (GTK_PROGRESS (pdata->pbar), i);
                        while (gtk_events_pending()) gtk_main_iteration();
                        }
                pdata->oldvalue = i;

		if((!s->vec_root)||(!autocoalesce))
			{
			AddNode(s->n, NULL);
			}
			else
			{
			len=0;
			t=s->vec_root;
			while(t)
				{
				if(t->selected)
					{
					if(len) t->selected=0;
					symc_current=(struct symchain *)calloc_2(1,sizeof(struct symchain));	
					symc_current->next=symc;
					symc_current->symbol=t;
					symc=symc_current;
					}
				len++;
				t=t->vec_chain;
				}
			if(len)add_vector_chain(s->vec_root, len);			
			}
		}
	}


while(symc)
	{
	symc->symbol->selected=1;
	symc_current=symc;
	symc=symc->next;
	free_2(symc_current);
	}

MaxSignalLength();
signalarea_configure_event(signalarea, NULL);
wavearea_configure_event(wavearea, NULL);

gtk_progress_set_value (GTK_PROGRESS (pdata->pbar), 0.0);
pdata->oldvalue = -1.0; 
gtk_grab_remove(widget);   
is_append_running=0;
}

static void select_row_callback(GtkWidget *widget, gint row, gint column,
	GdkEventButton *event, gpointer data)
{
struct symbol *s;

s=(struct symbol *)gtk_clist_get_row_data(GTK_CLIST(clist), row);
DEBUG(printf("Select: %p %s\n",s, s->name));
s->selected=1;
selected_rows++;
}

static void unselect_row_callback(GtkWidget *widget, gint row, gint column,
	GdkEventButton *event, gpointer data)
{
struct symbol *s;

s=(struct symbol *)gtk_clist_get_row_data(GTK_CLIST(clist), row);
DEBUG(printf("Unselect: %p %s\n",s, s->name));
s->selected=0;
selected_rows--;
}


static void enter_callback(GtkWidget *widget, GtkWidget *do_warning)
{
GtkCList *cl;
gchar *entry_text;
char *entry_suffixed;
int len;
int i, j, k, row;
int pixlen, maxpixlen;
char *s, *tmp2;

gfloat interval;

if(is_searching_running) return;
is_searching_running = ~0;
gtk_grab_add(widget);

pixlen=0; maxpixlen=0;

entry_text = gtk_entry_get_text(GTK_ENTRY(entry));
DEBUG(printf("Entry contents: %s\n", entry_text));
/* if(!(len=strlen(entry_text))) searchbox_text=NULL; */
if(!(len=strlen(entry_text))) searchbox_text=&default_null_searchbox_text;
      else strcpy((searchbox_text=(char *)malloc_2(len+1)),entry_text);

num_rows=0;
gtk_clist_freeze(cl=GTK_CLIST(clist));
gtk_clist_clear(cl);

entry_suffixed=wave_alloca(strlen(entry_text)+strlen(regex_type[regex_which])+1+((regex_which<2)?2:0));
*entry_suffixed=0x00;
if(regex_which<2) strcpy(entry_suffixed, "\\<");	/* match on word boundary */
strcat(entry_suffixed,entry_text);
strcat(entry_suffixed,regex_type[regex_which]);
wave_regex_compile(entry_suffixed);
for(i=0;i<numfacs;i++)
	{
	facs[i]->selected=0;
	}

GTK_ADJUSTMENT(pdata->adj)->upper = (gfloat)((numfacs>1)?numfacs-1:1);
pdata->oldvalue = -1.0;
interval = (gfloat)(numfacs/100.0);

for(i=0;i<numfacs;i++)
	{
	pdata->value = i;
	if(((int)(pdata->value/interval))!=((int)(pdata->oldvalue/interval)))		
		{
		gtk_progress_set_value (GTK_PROGRESS (pdata->pbar), i);
		while (gtk_events_pending()) gtk_main_iteration();
		}
	pdata->oldvalue = i;

	if(wave_regex_match(facs[i]->name))
		{
		if(!facs[i]->vec_root)
			{
			row=gtk_clist_append(cl,(gchar **)&(facs[i]->name));
			pixlen=gdk_string_measure(signalfont,(gchar *)(facs[i]->name));
			}
			else
			{
			if(autocoalesce)
				{
				if(facs[i]->vec_root!=facs[i]) continue;
				
				tmp2=makename_chain(facs[i]);
				s=(char *)malloc_2(strlen(tmp2)+4);
				strcpy(s,"[] ");
				strcpy(s+3, tmp2);
				free_2(tmp2);
				}
				else
				{
				s=(char *)malloc_2(strlen(facs[i]->name)+4);
				strcpy(s,"[] ");
				strcpy(s+3, facs[i]->name);
				}

			row=gtk_clist_append(cl,(gchar **)&s);
			pixlen=gdk_string_measure(signalfont,(gchar *)s);
			free_2(s);
			}

		maxpixlen=(pixlen>maxpixlen)?pixlen:maxpixlen;		
		gtk_clist_set_row_data(cl, row,facs[i]); 
		num_rows++;
		if(num_rows==WAVE_MAX_CLIST_LENGTH) break;
		}
	}

gtk_clist_set_column_width(GTK_CLIST(clist),0,maxpixlen?maxpixlen:1);
gtk_clist_thaw(cl);

gtk_progress_set_value (GTK_PROGRESS (pdata->pbar), 0.0);
pdata->oldvalue = -1.0;
gtk_grab_remove(widget);
is_searching_running=0;

if(do_warning)
if(num_rows>=WAVE_MAX_CLIST_LENGTH) 
	{
	char buf[256];
	sprintf(buf, "Limiting results to first %d entries.", num_rows);
	simplereqbox("Regex Search Warning",300,buf,"OK", NULL, NULL);
	}
}

static void destroy_callback(GtkWidget *widget, GtkWidget *nothing)
{
if((!is_insert_running)&&(!is_replace_running)&&(!is_append_running)&&(!is_searching_running))
	{
  	is_active=0;
  	gtk_widget_destroy(window);
	}
}


static void select_all_callback(GtkWidget *widget, GtkWidget *nothing)
{
gtk_clist_select_all(GTK_CLIST(clist));
}

static void unselect_all_callback(GtkWidget *widget, GtkWidget *nothing)
{
gtk_clist_unselect_all(GTK_CLIST(clist));
}


/*
 * mainline..
 */
void searchbox(char *title, GtkSignalFunc func)
{
    int i;
    GtkWidget *menu, *menuitem, *optionmenu;
    GSList *group;
    GtkWidget *small_hbox;

    GtkWidget *scrolled_win;
    GtkWidget *vbox1, *hbox, *hbox0;
    GtkWidget *button1, *button2, *button3, *button3a, *button4, *button5, *button6, *button7;
    GtkWidget *label;
    gchar *titles[]={"Matches"};
    GtkWidget *frame1, *frame2, *frameh, *frameh0;
    GtkWidget *table;
    GtkTooltips *tooltips;
    GtkAdjustment *adj;
    GtkWidget *align;

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

    label=gtk_label_new("Signal Search Expression");
    gtk_widget_show(label);

    gtk_box_pack_start (GTK_BOX (vbox1), label, TRUE, TRUE, 0);

    entry = gtk_entry_new_with_max_length (256);
    gtk_signal_connect(GTK_OBJECT(entry), "activate",
		       GTK_SIGNAL_FUNC(enter_callback),
		       entry);
    gtk_entry_set_text (GTK_ENTRY (entry), searchbox_text);
    gtk_entry_select_region (GTK_ENTRY (entry),
			     0, GTK_ENTRY(entry)->text_length);
    gtk_widget_show (entry);
    gtk_tooltips_set_tip_2(tooltips, entry, 
		"Enter search expression here.  POSIX Wildcards are allowed.  Note that you may also "
		"modify the search criteria by selecting ``[W]Range'', ``[W]Strand'', or ``None'' for suffix "
		"matching.",NULL);

    gtk_box_pack_start (GTK_BOX (vbox1), entry, TRUE, TRUE, 0);

    /* Allocate memory for the data that is used later */
    pdata = calloc_2(1, sizeof(SearchProgressData) );
    pdata->value = pdata->oldvalue = 0.0;
    /* Create a centering alignment object */  
    align = gtk_alignment_new (0.5, 0.5, 0, 0);
    gtk_widget_show(align);
    /* Create a Adjustment object to hold the range of the
     * progress bar */
    adj = (GtkAdjustment *) gtk_adjustment_new (0.0, 0.0, (gfloat)((numfacs>1)?numfacs-1:1), 0, 0, 0);
    pdata->adj = adj;
    /* Create the GtkProgressBar using the adjustment */
    pdata->pbar = gtk_progress_bar_new_with_adjustment (adj);
    /* Set the format of the string that can be displayed in the
     * trough of the progress bar:
     * %p - percentage
     * %v - value
     * %l - lower range value
     * %u - upper range value */
    gtk_progress_set_format_string (GTK_PROGRESS (pdata->pbar), "(%p%%)");
    gtk_progress_set_show_text (GTK_PROGRESS (pdata->pbar), TRUE);
    gtk_widget_show(pdata->pbar);
    gtk_box_pack_start (GTK_BOX (vbox1), pdata->pbar, TRUE, TRUE, 0);

    gtk_container_add (GTK_CONTAINER (frame1), vbox1);


    frame2 = gtk_frame_new (NULL);
    gtk_container_border_width (GTK_CONTAINER (frame2), 3);
    gtk_widget_show(frame2);

    gtk_table_attach (GTK_TABLE (table), frame2, 0, 1, 1, 254,
                        GTK_FILL | GTK_EXPAND,
                        GTK_FILL | GTK_EXPAND | GTK_SHRINK, 1, 1);

    clist=gtk_clist_new_with_titles(1,titles);
    gtk_clist_column_titles_passive(GTK_CLIST(clist)); 

    gtk_clist_set_selection_mode(GTK_CLIST(clist), GTK_SELECTION_EXTENDED);
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


    frameh0 = gtk_frame_new (NULL);
    gtk_container_border_width (GTK_CONTAINER (frameh0), 3);
    gtk_widget_show(frameh0);
    gtk_table_attach (GTK_TABLE (table), frameh0, 0, 1, 254, 255,
                        GTK_FILL | GTK_EXPAND,
                        GTK_FILL | GTK_EXPAND | GTK_SHRINK, 1, 1);


    hbox0 = gtk_hbox_new (FALSE, 1);
    gtk_widget_show (hbox0);

    button6 = gtk_button_new_with_label (" Select All ");
    gtk_container_border_width (GTK_CONTAINER (button6), 3);
    gtk_signal_connect_object (GTK_OBJECT (button6), "clicked",
			       GTK_SIGNAL_FUNC(select_all_callback),
			       GTK_OBJECT (window));
    gtk_widget_show (button6);
    gtk_tooltips_set_tip_2(tooltips, button6, 
		"Highlight all signals listed in the match window.",NULL);

    gtk_box_pack_start (GTK_BOX (hbox0), button6, TRUE, FALSE, 0);



    menu = gtk_menu_new ();
    group=NULL;

    small_hbox = gtk_hbox_new (TRUE, 0);
    gtk_widget_show (small_hbox);
    
    for(i=0;i<5;i++)
        {
        menuitem = gtk_radio_menu_item_new_with_label (group, regex_name[i]);
        group = gtk_radio_menu_item_group (GTK_RADIO_MENU_ITEM (menuitem));
        gtk_menu_append (GTK_MENU (menu), menuitem);
        gtk_widget_show (menuitem);
        gtk_signal_connect(GTK_OBJECT (menuitem), "activate",
                                 GTK_SIGNAL_FUNC(regex_clicked),
                                 &regex_mutex[i]);
        regex_mutex[i]=0;
        }
    
        regex_mutex[0]=1;     /* "range" */
    
        optionmenu = gtk_option_menu_new ();
        gtk_option_menu_set_menu (GTK_OPTION_MENU (optionmenu), menu);
        gtk_box_pack_start (GTK_BOX (small_hbox), optionmenu, TRUE, FALSE, 0);
        gtk_widget_show (optionmenu);   
	gtk_tooltips_set_tip_2(tooltips, optionmenu,
		"You may "
		"modify the search criteria by selecting ``Range'', ``Strand'', or ``None'' for suffix "
		"matching.  This optionally matches the string you enter in the search string above with a Verilog "
		"format range (signal[7:0]), a strand (signal.1, signal.0), or with no suffix.  "
		"The ``W'' modifier for ``Range'' and ``Strand'' explicitly matches on word boundaries.  "
		"(addr matches unit.freezeaddr[63:0] for ``Range'' but only unit.addr[63:0] for ``WRange'' since addr has to be on a word boundary.  "
		"Note that when ``None'' "
		"is selected, the search string may be located anywhere in the signal name.",NULL);
    
        gtk_box_pack_start (GTK_BOX (hbox0), small_hbox, FALSE, FALSE, 0);


    button7 = gtk_button_new_with_label (" Unselect All ");
    gtk_container_border_width (GTK_CONTAINER (button7), 3);
    gtk_signal_connect_object (GTK_OBJECT (button7), "clicked",
			       GTK_SIGNAL_FUNC(unselect_all_callback),
			       GTK_OBJECT (window));
    gtk_widget_show (button7);
    gtk_tooltips_set_tip_2(tooltips, button7, 
		"Unhighlight all signals listed in the match window.",NULL);
    gtk_box_pack_start (GTK_BOX (hbox0), button7, TRUE, FALSE, 0);

    gtk_container_add (GTK_CONTAINER (frameh0), hbox0);


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
		"Add selected signals after last highlighted signal on the main window.",NULL);
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
		"Bundle selected signals into a single bit vector with the topmost selected signal as the LSB and the lowest as the MSB.",NULL);
    	gtk_box_pack_start (GTK_BOX (hbox), button3, TRUE, FALSE, 0);

    	button3a = gtk_button_new_with_label (" Bundle Down ");
    	gtk_container_border_width (GTK_CONTAINER (button3a), 3);
    	gtk_signal_connect_object (GTK_OBJECT (button3a), "clicked",
			       GTK_SIGNAL_FUNC(bundle_callback_down),
			       GTK_OBJECT (window));
    	gtk_widget_show (button3a);
    	gtk_tooltips_set_tip_2(tooltips, button3a, 
		"Bundle selected signals into a single bit vector with the topmost selected signal as the MSB and the lowest as the LSB.",NULL);
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

    if(strlen(searchbox_text)) enter_callback(entry,NULL);
}
