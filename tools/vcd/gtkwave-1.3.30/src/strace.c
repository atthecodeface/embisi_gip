/* 
 * Copyright (c) Tony Bybell 1999-2003.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include "strace.h"

TimeType *timearray=NULL;
int timearray_size=0;

static GtkWidget *ptr_mark_count_label=NULL;   /* pointer to mark count label on pattern search dialog */

struct strace *straces=NULL;   /* when active, disables certain ops! */
struct strace *shadow_straces=NULL;

struct strace_defer_free *strace_defer_free_head=NULL;

static GtkWidget *window;
static GtkSignalFunc cleanup;

static char *logical[]=
	{"AND", "OR", "XOR", "NAND", "NOR", "XNOR"};

static char *stype[]=
	{"Don't Care", "High", "Z (Mid)", "X", "Low", "String",
	 "Rising Edge", "Falling Edge", "Any Edge"};

char logical_mutex[6]={0,0,0,0,0,0};
char shadow_logical_mutex[6]={0,0,0,0,0,0};
char shadow_active = 0;
char shadow_type = 0;
char *shadow_string = NULL;


/*
 * trap timescale overflows
 */
static TimeType adjust(TimeType a, TimeType b)
{
TimeType res=a+b;

if((b>LLDescriptor(0))&&(res<a))
	{
	return(MAX_HISTENT_TIME);
	}

return(res);
}


/*
 * free the straces...
 */
static void free_straces(void)
{
struct strace *s, *skill;
int i;
struct strace_defer_free *sd, *sd2;

s=straces;

while(s)
	{
	for(i=0;i<9;i++)
		{
		if(s->back[i]) free_2(s->back[i]);
		}
	if(s->string) free_2(s->string);
	skill=s;
	s=s->next;
	free_2(skill);
	}

straces=NULL;

sd = strace_defer_free_head;

while(sd)
	{
	FreeTrace(sd->defer);
	sd2 = sd->next;
	free_2(sd);
	sd = sd2;
	}
strace_defer_free_head = NULL;
}


/*
 * button/menu/entry activations..
 */
static void logical_clicked(GtkWidget *widget, gpointer which)
{
int i;
char *which_char;

for(i=0;i<6;i++) logical_mutex[i]=0;
which_char=(char *)which;
*which_char=1;			/* mark our choice */

DEBUG(printf("picked: %s\n", logical[which_char-logical_mutex]));
}

static void stype_clicked(GtkWidget *widget, gpointer back)
{
struct strace_back *b;
struct strace *s;

b=(struct strace_back *)back;
s=b->parent;

s->value=b->which;

DEBUG(printf("Trace %s Search Type: %s\n", s->trace->name, stype[(int)s->value]));
}


static void enter_callback(GtkWidget *widget, gpointer strace_tmp)
{
gchar *entry_text;
struct strace *s;
int i, len;

s=(struct strace *)strace_tmp;
if(s->string) { free_2(s->string); s->string=NULL; }

entry_text = gtk_entry_get_text(GTK_ENTRY(widget));
DEBUG(printf("Trace %s Entry contents: %s\n", s->trace->name, entry_text));

if(!(len=strlen(entry_text))) return;

gtk_entry_select_region (GTK_ENTRY (widget),
                             0, GTK_ENTRY(widget)->text_length);

strcpy((s->string=(char *)malloc_2(len+1)),entry_text);
for(i=0;i<len;i++)
	{
	char ch;
	ch=s->string[i];
	if((ch>='a')&&(ch<='z')) s->string[i]=ch-('a'-'A');
	}
}

static void forwards_callback(GtkWidget *widget, GtkWidget *nothing)
{
/* no cleanup necessary, but do real search */
DEBUG(printf("Searching Forward..\n"));
strace_search(STRACE_FORWARD);
}

static void backwards_callback(GtkWidget *widget, GtkWidget *nothing)
{
/* no cleanup necessary, but do real search */
DEBUG(printf("Searching Backward..\n"));
strace_search(STRACE_BACKWARD);
}

static void mark_callback(GtkWidget *widget, GtkWidget *nothing)
{
DEBUG(printf("Marking..\n"));
if(shadow_straces)
	{
	delete_strace_context();
	}

strace_maketimetrace(1);
cache_actual_pattern_mark_traces();

MaxSignalLength();
signalarea_configure_event(signalarea, NULL);
wavearea_configure_event(wavearea, NULL);
}

static void clear_callback(GtkWidget *widget, GtkWidget *nothing)
{
DEBUG(printf("Clearing..\n"));
if(shadow_straces)
	{
	delete_strace_context();
	}
strace_maketimetrace(0);

MaxSignalLength();
signalarea_configure_event(signalarea, NULL);
wavearea_configure_event(wavearea, NULL);
}

static void destroy_callback(GtkWidget *widget, GtkWidget *nothing)
{
  free_straces();
  ptr_mark_count_label=NULL;
  gtk_widget_destroy(window);
}

/* update mark count label on pattern search dialog */

static void update_mark_count_label()
{
if(ptr_mark_count_label)
    {
    char mark_count_buf[64];
    if (timearray_size < 1)
	mark_count_buf[0] = 0;
    else
	sprintf (mark_count_buf, "Mark Count: %d", timearray_size>=1 ? timearray_size-1 : 0);
    gtk_label_set_text (GTK_LABEL(ptr_mark_count_label), mark_count_buf);
    }
}

void tracesearchbox(char *title, GtkSignalFunc func)
{
    GtkWidget *menu, *menuitem, *optionmenu;
    GSList *group; 
    GtkWidget *entry;
    GtkWidget *vbox, *hbox, *small_hbox, *count_hbox, *vbox_g, *label;
    GtkWidget *button1, *button1a, *button1b, *button1c, *button2, *scrolled_win, *frame, *separator;
    Trptr t;
    int i;
    int numtraces;

    if(straces) 
	{
	gdk_window_raise(window->window);
	return; /* is already active */
	}

    cleanup=func;
    numtraces=0;

    /* create a new window */
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW (window), title);
    gtk_widget_set_usize( GTK_WIDGET (window), 420, -1); 
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

    small_hbox = gtk_hbox_new (TRUE, 0);
    gtk_widget_show (small_hbox);

    label=gtk_label_new("Logical Operation");
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (small_hbox), label, TRUE, FALSE, 0);


    menu = gtk_menu_new ();
    group=NULL;

    for(i=0;i<6;i++)
	{
    	menuitem = gtk_radio_menu_item_new_with_label (group, logical[i]);
    	group = gtk_radio_menu_item_group (GTK_RADIO_MENU_ITEM (menuitem));
    	gtk_menu_append (GTK_MENU (menu), menuitem);
    	gtk_widget_show (menuitem);
        gtk_signal_connect(GTK_OBJECT (menuitem), "activate",
                                 GTK_SIGNAL_FUNC(logical_clicked),
                                 &logical_mutex[i]);
	logical_mutex[i]=0;
	}

	logical_mutex[0]=1;	/* "and" */

	optionmenu = gtk_option_menu_new ();
	gtk_option_menu_set_menu (GTK_OPTION_MENU (optionmenu), menu);
	gtk_box_pack_start (GTK_BOX (small_hbox), optionmenu, TRUE, FALSE, 0);
	gtk_widget_show (optionmenu);

	gtk_box_pack_start (GTK_BOX (vbox), small_hbox, FALSE, FALSE, 0);

    scrolled_win = gtk_scrolled_window_new (NULL, NULL);
    gtk_widget_set_usize( GTK_WIDGET (scrolled_win), -1, 300); 
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
                                      GTK_POLICY_AUTOMATIC,
                                      GTK_POLICY_AUTOMATIC);
    gtk_widget_show(scrolled_win);
    gtk_container_add (GTK_CONTAINER (frame), scrolled_win);
    gtk_container_add (GTK_CONTAINER (vbox), frame);

    for(t=traces.first;t;t=t->next)
    {
    struct strace *s;

    if ((t->flags&TR_BLANK)||(!(t->flags&TR_HIGHLIGHT))||(!(t->name))) continue;

    numtraces++;
    if(numtraces==500) 
	{
	status_text("Limiting waveform display search to 500 traces.\n");
	break;
	}

    s=(struct strace *)calloc_2(1,sizeof(struct strace));
    s->next=straces;
    straces=s;
    s->trace=t;

    if(t!=traces.first)
	{
    	separator = gtk_hseparator_new ();
    	gtk_widget_show (separator);
    	gtk_box_pack_start (GTK_BOX (vbox_g), separator, FALSE, FALSE, 0);
	}

    small_hbox = gtk_hbox_new (TRUE, 0);
    gtk_widget_show (small_hbox);

    label=gtk_label_new(t->name);
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (vbox_g), label, FALSE, FALSE, 0);

    menu = gtk_menu_new ();
    group=NULL;

    for(i=0;i<9;i++)
	{
	struct strace_back *b;

	b=(struct strace_back *)calloc_2(1,sizeof(struct strace_back));
	b->parent=s;
	b->which=i;
	s->back[i]=b;
    	menuitem = gtk_radio_menu_item_new_with_label (group, stype[i]);
    	group = gtk_radio_menu_item_group (GTK_RADIO_MENU_ITEM (menuitem));
    	gtk_menu_append (GTK_MENU (menu), menuitem);
    	gtk_widget_show (menuitem);
        gtk_signal_connect(GTK_OBJECT (menuitem), "activate",
                                 GTK_SIGNAL_FUNC(stype_clicked),
                                 b);
	}

	optionmenu = gtk_option_menu_new ();
	gtk_option_menu_set_menu (GTK_OPTION_MENU (optionmenu), menu);
	gtk_box_pack_start (GTK_BOX (small_hbox), optionmenu, TRUE, FALSE, 0);
	gtk_widget_show (optionmenu);

    entry = gtk_entry_new_with_max_length (257); /* %+256ch */
    gtk_signal_connect(GTK_OBJECT(entry), "activate",
		       GTK_SIGNAL_FUNC(enter_callback),
		       s);

    gtk_box_pack_start (GTK_BOX (small_hbox), entry, TRUE, FALSE, 0);
    gtk_widget_show (entry);
    gtk_box_pack_start (GTK_BOX (vbox_g), small_hbox, FALSE, FALSE, 0);
    }

    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled_win), vbox_g);

    do		/* add a horizontal box to display the mark count */
	{
	count_hbox = gtk_hbox_new (TRUE, 0);
	gtk_widget_show (count_hbox);
	gtk_box_pack_start (GTK_BOX (vbox), count_hbox, FALSE, FALSE, 0);

	ptr_mark_count_label = gtk_label_new ("");
	gtk_widget_show (ptr_mark_count_label);
	gtk_box_pack_start (GTK_BOX (count_hbox), ptr_mark_count_label, TRUE, FALSE, 0);
	update_mark_count_label ();
	} while (0);

    hbox = gtk_hbox_new (FALSE, 1);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
    gtk_widget_show (hbox);

    button1 = gtk_button_new_with_label ("Fwd");
    gtk_widget_set_usize(button1, 75, -1);
    gtk_signal_connect(GTK_OBJECT (button1), "clicked",
			       GTK_SIGNAL_FUNC(forwards_callback),
			       NULL);
    gtk_widget_show (button1);
    gtk_container_add (GTK_CONTAINER (hbox), button1);
    GTK_WIDGET_SET_FLAGS (button1, GTK_CAN_DEFAULT);
    gtk_signal_connect_object (GTK_OBJECT (button1),
                                "realize",
                             (GtkSignalFunc) gtk_widget_grab_default,
                             GTK_OBJECT (button1));


    button1a = gtk_button_new_with_label ("Bkwd");
    gtk_widget_set_usize(button1a, 75, -1);
    gtk_signal_connect(GTK_OBJECT (button1a), "clicked",
			       GTK_SIGNAL_FUNC(backwards_callback),
			       NULL);
    gtk_widget_show (button1a);
    gtk_container_add (GTK_CONTAINER (hbox), button1a);
    GTK_WIDGET_SET_FLAGS (button1a, GTK_CAN_DEFAULT);

    button1b = gtk_button_new_with_label ("Mark");
    gtk_widget_set_usize(button1b, 75, -1);
    gtk_signal_connect(GTK_OBJECT (button1b), "clicked",
			       GTK_SIGNAL_FUNC(mark_callback),
			       NULL);
    gtk_widget_show (button1b);
    gtk_container_add (GTK_CONTAINER (hbox), button1b);
    GTK_WIDGET_SET_FLAGS (button1b, GTK_CAN_DEFAULT);

    button1c = gtk_button_new_with_label ("Clear");
    gtk_widget_set_usize(button1c, 75, -1);
    gtk_signal_connect(GTK_OBJECT (button1c), "clicked",
			       GTK_SIGNAL_FUNC(clear_callback),
			       NULL);
    gtk_widget_show (button1c);
    gtk_container_add (GTK_CONTAINER (hbox), button1c);
    GTK_WIDGET_SET_FLAGS (button1c, GTK_CAN_DEFAULT);

    button2 = gtk_button_new_with_label ("Exit");
    gtk_widget_set_usize(button2, 75, -1);
    gtk_signal_connect(GTK_OBJECT (button2), "clicked",
			       GTK_SIGNAL_FUNC(destroy_callback),
			       NULL);
    GTK_WIDGET_SET_FLAGS (button2, GTK_CAN_DEFAULT);
    gtk_widget_show (button2);
    gtk_container_add (GTK_CONTAINER (hbox), button2);

    gtk_widget_show(window);
}


/*
 * strace backward or forward..
 */
void strace_search(int direction)
{
struct strace *s;
TimeType basetime, maxbase, sttim, fintim;
Trptr t;
int totaltraces, passcount;
int whichpass;
TimeType middle=0, width;


if(direction==STRACE_BACKWARD) /* backwards */
{
if(tims.marker<0)
	{
	basetime=MAX_HISTENT_TIME;
	}
	else
	{	
	basetime=tims.marker;
	}
}
else /* go forwards */
{
if(tims.marker<0)
	{
	basetime=tims.first;
	}
	else
	{	
	basetime=tims.marker;
	}
} 

sttim=tims.first;
fintim=tims.last;

for(whichpass=0;;whichpass++)
{

if(direction==STRACE_BACKWARD) /* backwards */
{
maxbase=-1;
s=straces;
while(s)
	{
	t=s->trace;
	shift_timebase=t->shift;
	if(!(t->vector))
		{
		hptr h;
		hptr *hp;
		UTimeType utt;
		TimeType  tt;

		h=bsearch_node(t->n.nd, basetime);
		hp=max_compare_index;
		if((hp==&(t->n.nd->harray[1]))||(hp==&(t->n.nd->harray[0]))) return;
		hp--;
		h=*hp;
		s->his.h=h;
		utt=adjust(h->time,shift_timebase); tt=utt;
		if(tt > maxbase) maxbase=tt;
		}
		else
		{
		vptr v;
		vptr *vp;
		UTimeType utt;
		TimeType  tt;

		v=bsearch_vector(t->n.vec, basetime);
		vp=vmax_compare_index;
		if((vp==&(t->n.vec->vectors[1]))||(vp==&(t->n.vec->vectors[0]))) return;
		vp--;
		v=*vp;
		s->his.v=v;
		utt=adjust(v->time,shift_timebase); tt=utt;
		if(tt > maxbase) maxbase=tt;
		}

	s=s->next;
	}
}
else /* go forward */
{
maxbase=MAX_HISTENT_TIME;
s=straces;
while(s)
	{
	t=s->trace;
	shift_timebase=t->shift;
	if(!(t->vector))
		{
		hptr h;
		UTimeType utt;
		TimeType  tt;

		h=bsearch_node(t->n.nd, basetime);
		while(h->next && h->time==h->next->time) h=h->next;
		if((whichpass)||(tims.marker>=0)) h=h->next;
		if(!h) return;
		s->his.h=h;
		utt=adjust(h->time,shift_timebase); tt=utt;		
		if(tt < maxbase) maxbase=tt;
		}
		else
		{
		vptr v;
		UTimeType utt;
		TimeType  tt;

		v=bsearch_vector(t->n.vec, basetime);
		while(v->next && v->time==v->next->time) v=v->next;
		if((whichpass)||(tims.marker>=0)) v=v->next;
		if(!v) return;
		s->his.v=v;
		utt=adjust(v->time,shift_timebase); tt=utt;
		if(tt < maxbase) maxbase=tt;
		}

	s=s->next;
	}
}

s=straces;
totaltraces=0;	/* increment when not don't care */
while(s)
	{
	char str[2];
	t=s->trace;
	s->search_result=0;	/* explicitly must set this */
	shift_timebase=t->shift;
	
	if((!t->vector)&&(!(t->n.nd->ext)))
		{
		if(adjust(s->his.h->time,shift_timebase)!=maxbase) 
			{
			s->his.h=bsearch_node(t->n.nd, maxbase);
			while(s->his.h->next && s->his.h->time==s->his.h->next->time) s->his.h=s->his.h->next;
			}
		if(t->flags&TR_INVERT)  
                	{
                        str[0]="1XZ0"[s->his.h->v.val];
                        }
                        else   
                        {
                        str[0]="0XZ1"[s->his.h->v.val];
                        }
		str[1]=0x00;

		switch(s->value)
			{
			case ST_DC:
				break;
				
			case ST_HIGH:
				totaltraces++;
				if(str[0]=='1') s->search_result=1;	
				break;

			case ST_RISE:
				totaltraces++;
				if((str[0]=='1')&&(adjust(s->his.h->time,shift_timebase)==maxbase)) 
					s->search_result=1;	
				break;

			case ST_LOW:
				totaltraces++;
				if(str[0]=='0') s->search_result=1;
				break;

			case ST_FALL:
				totaltraces++;
				if((str[0]=='0')&&(adjust(s->his.h->time,shift_timebase)==maxbase))
 					s->search_result=1;
				break;

			case ST_MID:
				totaltraces++;
				if(str[0]=='Z')
 					s->search_result=1;
				break;				

			case ST_X:
				totaltraces++;
				if(str[0]=='X') s->search_result=1;
				break;

			case ST_ANY:
				totaltraces++;
				if(adjust(s->his.h->time,shift_timebase)==maxbase)s->search_result=1;
				break;
		
			case ST_STRING:
				totaltraces++;
				if(s->string)
				if(strstr(s->string,str)) s->search_result=1;
				break;

			default:
				printf("Internal error: st_type of %d\n",s->value);
				exit(0);
			}


		}
		else
		{
		char *chval, *chval2;
		char ch;

		if(t->vector)
			{
			if(adjust(s->his.v->time,shift_timebase)!=maxbase) 
				{
				s->his.v=bsearch_vector(t->n.vec, maxbase);
				while(s->his.v->next && s->his.v->time==s->his.v->next->time) s->his.v=s->his.v->next;
				}
			chval=convert_ascii(t,s->his.v);
			}
			else
			{
			if(adjust(s->his.h->time,shift_timebase)!=maxbase) 
				{
				s->his.h=bsearch_node(t->n.nd, maxbase);
				while(s->his.h->next && s->his.h->time==s->his.h->next->time) s->his.h=s->his.h->next;
				}
			if(s->his.h->flags&HIST_REAL)
				{
				if(!(s->his.h->flags&HIST_STRING))
					{
					chval=convert_ascii_real((double *)s->his.h->v.vector);
					}
					else
					{
					chval=convert_ascii_string((char *)s->his.h->v.vector);
					chval2=chval;
					while((ch=*chval2))	/* toupper() the string */
						{
						if((ch>='a')&&(ch<='z')) { *chval2= ch-('a'-'A'); }
						chval2++;
						}
					}
				}
				else
				{
				chval=convert_ascii_vec(t,s->his.h->v.vector);
				}
			}

		switch(s->value)
			{
			case ST_DC:
				break;

			case ST_RISE:
			case ST_FALL:
				totaltraces++;
				break;
				
			case ST_HIGH:
				totaltraces++;
				if((chval2=chval))
				while((ch=*(chval2++)))
					{
					if((((ch>='1')&&(ch<='9'))||((ch>='A')&&(ch<='F')))
						&&(ch!='X')&&(ch!='Z'))
						{
						s->search_result=1;
						break;
						}
					}
				break;

			case ST_LOW:
				totaltraces++;
				if((chval2=chval))
				{
				s->search_result=1;
				while((ch=*(chval2++)))
					{
					if(((ch>='1')&&(ch<='9'))||((ch>='A')&&(ch<='F'))
						||(ch=='X')||(ch=='Z'))
						{
						s->search_result=0;
						break;
						}
					}
				}
				break;

			case ST_MID:
				totaltraces++;
				if((chval2=chval))
				{
				s->search_result=1;
				while((ch=*(chval2++)))
					{
					if(((ch>='0')&&(ch<='9'))||((ch>='A')&&(ch<='F'))
						||(ch=='X'))
						{
						s->search_result=0;
						break;
						}
					}
				}
				break;

			case ST_X:
				totaltraces++;
				if((chval2=chval))
				{
				s->search_result=1;
				while((ch=*(chval2++)))
					{
					if(((ch>='0')&&(ch<='9'))||((ch>='A')&&(ch<='F'))
						||(ch=='Z'))
						{
						s->search_result=0;
						break;
						}
					}
				}
				break;

			case ST_ANY:
				totaltraces++;
				if(adjust(s->his.v->time,shift_timebase)==maxbase)
					s->search_result=1;
				break;
		
			case ST_STRING:
				totaltraces++;
				if(s->string)
				if(strstr(chval, s->string)) s->search_result=1;
				break;

			default:
				printf("Internal error: st_type of %d\n",s->value);
				exit(0);
			}

		free_2(chval);
		}
	s=s->next;
	}

if((maxbase<sttim)||(maxbase>fintim)) return;

DEBUG(printf("Maxbase: "TTFormat", total traces: %d\n",maxbase, totaltraces));
s=straces;
passcount=0;
while(s)
	{
	DEBUG(printf("\tPass: %d, Name: %s\n",s->search_result, s->trace->name));
	if(s->search_result) passcount++;
	s=s->next;
	}

if(totaltraces)
	{
	if(logical_mutex[0])	/* and */
		{
		if(totaltraces==passcount) break;		
		}
	else
	if(logical_mutex[1])	/* or */
		{
		if(passcount) break;
		}
	else
	if(logical_mutex[2])	/* xor */
		{
		if(passcount&1) break;
		}
	else
	if(logical_mutex[3])	/* nand */
		{
		if(totaltraces!=passcount) break;
		}
	else
	if(logical_mutex[4])	/* nor */
		{
		if(!passcount) break;
		}
	else
	if(logical_mutex[5])	/* xnor */
		{
		if(!(passcount&1)) break;
		}
	}

basetime=maxbase;
}

update_markertime(tims.marker=maxbase);

width=(TimeType)(((gdouble)wavewidth)*nspx);
if((tims.marker<tims.start)||(tims.marker>=tims.start+width))
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
	
	tims.start=time_trunc(middle-(width/2));
	if(tims.start+width>tims.last) tims.start=tims.last-width;
	if(tims.start<tims.first) tims.start=tims.first;  
	GTK_ADJUSTMENT(wave_hslider)->value=tims.timecache=tims.start;
	}

MaxSignalLength();
signalarea_configure_event(signalarea, NULL);
wavearea_configure_event(wavearea, NULL);
}


/*********************************************/

/*
 * strace forward to make the timetrace
 */
TimeType strace_timetrace(TimeType basetime, int notfirst)
{
struct strace *s;
TimeType maxbase, fintim;
Trptr t;
int totaltraces, passcount;
int whichpass;

fintim=tims.last;

for(whichpass=0;;whichpass++)
{
maxbase=MAX_HISTENT_TIME;
s=straces;
while(s)
	{
	t=s->trace;
	shift_timebase=t->shift;
	if(!(t->vector))
		{
		hptr h;
		UTimeType utt;
		TimeType  tt;

		h=bsearch_node(t->n.nd, basetime);
		s->his.h=h;
		while(h->time==h->next->time) h=h->next;
		if((whichpass)||(notfirst)) h=h->next;
		if(!h) return(MAX_HISTENT_TIME);
		utt=adjust(h->time,shift_timebase); tt=utt;		
		if(tt < maxbase) maxbase=tt;
		}
		else
		{
		vptr v;
		UTimeType utt;
		TimeType  tt;

		v=bsearch_vector(t->n.vec, basetime);
		if((whichpass)||(notfirst)) v=v->next;
		if(!v) return(MAX_HISTENT_TIME);
		s->his.v=v;
		utt=adjust(v->time,shift_timebase); tt=utt;
		if(tt < maxbase) maxbase=tt;
		}

	s=s->next;
	}

s=straces;
totaltraces=0;	/* increment when not don't care */
while(s)
	{
	char str[2];
	t=s->trace;
	s->search_result=0;	/* explicitly must set this */
	shift_timebase=t->shift;
	
	if((!t->vector)&&(!(t->n.nd->ext)))
		{
		if(adjust(s->his.h->time,shift_timebase)!=maxbase) 
			{
			s->his.h=bsearch_node(t->n.nd, maxbase);
			while(s->his.h->next && s->his.h->time==s->his.h->next->time) s->his.h=s->his.h->next;
			}
		if(t->flags&TR_INVERT)  
                	{
                        str[0]="1XZ0"[s->his.h->v.val];
                        }
                        else   
                        {
                        str[0]="0XZ1"[s->his.h->v.val];
                        }
		str[1]=0x00;

		switch(s->value)
			{
			case ST_DC:
				break;
				
			case ST_HIGH:
				totaltraces++;
				if(str[0]=='1') s->search_result=1;	
				break;

			case ST_RISE:
				totaltraces++;
				if((str[0]=='1')&&(adjust(s->his.h->time,shift_timebase)==maxbase)) 
					s->search_result=1;	
				break;

			case ST_LOW:
				totaltraces++;
				if(str[0]=='0') s->search_result=1;
				break;

			case ST_FALL:
				totaltraces++;
				if((str[0]=='0')&&(adjust(s->his.h->time,shift_timebase)==maxbase))
 					s->search_result=1;
				break;

			case ST_MID:
				totaltraces++;
				if(str[0]=='Z')
 					s->search_result=1;
				break;				

			case ST_X:
				totaltraces++;
				if(str[0]=='X') s->search_result=1;
				break;

			case ST_ANY:
				totaltraces++;
				if(adjust(s->his.h->time,shift_timebase)==maxbase)s->search_result=1;
				break;
		
			case ST_STRING:
				totaltraces++;
				if(s->string)
				if(strstr(s->string,str)) s->search_result=1;
				break;

			default:
				printf("Internal error: st_type of %d\n",s->value);
				exit(0);
			}


		}
		else
		{
		char *chval, *chval2;
		char ch;

		if(t->vector)
			{
			if(adjust(s->his.v->time,shift_timebase)!=maxbase) 
				{
				s->his.v=bsearch_vector(t->n.vec, maxbase);
				while(s->his.v->next && s->his.v->time==s->his.v->next->time) s->his.v=s->his.v->next;
				}
			chval=convert_ascii(t,s->his.v);
			}
			else
			{
			if(adjust(s->his.h->time,shift_timebase)!=maxbase) 
				{
				s->his.h=bsearch_node(t->n.nd, maxbase);
				while(s->his.h->next && s->his.h->time==s->his.h->next->time) s->his.h=s->his.h->next;
				}
			if(s->his.h->flags&HIST_REAL)
				{
				if(!(s->his.h->flags&HIST_STRING))
					{
					chval=convert_ascii_real((double *)s->his.h->v.vector);
					}
					else
					{
					chval=convert_ascii_string((char *)s->his.h->v.vector);
					chval2=chval;
					while((ch=*chval2))	/* toupper() the string */
						{
						if((ch>='a')&&(ch<='z')) { *chval2= ch-('a'-'A'); }
						chval2++;
						}
					}
				}
				else
				{
				chval=convert_ascii_vec(t,s->his.h->v.vector);
				}
			}

		switch(s->value)
			{
			case ST_DC:
				break;

			case ST_RISE:
			case ST_FALL:
				totaltraces++;
				break;
				
			case ST_HIGH:
				totaltraces++;
				if((chval2=chval))
				while((ch=*(chval2++)))
					{
					if((((ch>='1')&&(ch<='9'))||((ch>='A')&&(ch<='F')))
						&&(ch!='X')&&(ch!='Z'))
						{
						s->search_result=1;
						break;
						}
					}
				break;

			case ST_LOW:
				totaltraces++;
				if((chval2=chval))
				{
				s->search_result=1;
				while((ch=*(chval2++)))
					{
					if(((ch>='1')&&(ch<='9'))||((ch>='A')&&(ch<='F'))
						||(ch=='X')||(ch=='Z'))
						{
						s->search_result=0;
						break;
						}
					}
				}
				break;

			case ST_MID:
				totaltraces++;
				if((chval2=chval))
				{
				s->search_result=1;
				while((ch=*(chval2++)))
					{
					if(((ch>='0')&&(ch<='9'))||((ch>='A')&&(ch<='F'))
						||(ch=='X'))
						{
						s->search_result=0;
						break;
						}
					}
				}
				break;

			case ST_X:
				totaltraces++;
				if((chval2=chval))
				{
				s->search_result=1;
				while((ch=*(chval2++)))
					{
					if(((ch>='0')&&(ch<='9'))||((ch>='A')&&(ch<='F'))
						||(ch=='Z'))
						{
						s->search_result=0;
						break;
						}
					}
				}
				break;

			case ST_ANY:
				totaltraces++;
				if(adjust(s->his.v->time,shift_timebase)==maxbase)
					s->search_result=1;
				break;
		
			case ST_STRING:
				totaltraces++;
				if(s->string)
				if(strstr(chval, s->string)) s->search_result=1;
				break;

			default:
				printf("Internal error: st_type of %d\n",s->value);
				exit(0);
			}

		free_2(chval);
		}
	s=s->next;
	}

if(maxbase>fintim) return(MAX_HISTENT_TIME);

DEBUG(printf("Maxbase: "TTFormat", total traces: %d\n",maxbase, totaltraces));
s=straces;
passcount=0;
while(s)
	{
	DEBUG(printf("\tPass: %d, Name: %s\n",s->search_result, s->trace->name));
	if(s->search_result) passcount++;
	s=s->next;
	}

if(totaltraces)
	{
	if(logical_mutex[0])	/* and */
		{
		if(totaltraces==passcount) break;		
		}
	else
	if(logical_mutex[1])	/* or */
		{
		if(passcount) break;
		}
	else
	if(logical_mutex[2])	/* xor */
		{
		if(passcount&1) break;
		}
	else
	if(logical_mutex[3])	/* nand */
		{
		if(totaltraces!=passcount) break;
		}
	else
	if(logical_mutex[4])	/* nor */
		{
		if(!passcount) break;
		}
	else
	if(logical_mutex[5])	/* xnor */
		{
		if(!(passcount&1)) break;
		}
	}

basetime=maxbase;
}


return(maxbase);
}


void strace_maketimetrace(int mode)
{
TimeType basetime=tims.first;
int i, notfirst=0;
struct timechain *tchead=NULL, *tc=NULL;
TimeType *t;

if(timearray)
	{
	free_2(timearray);
	timearray=NULL;
	}

timearray_size=0;

if((!mode)&&(!shadow_active))
	{
	update_mark_count_label();
	delete_mprintf();
	return;	/* merely free stuff up */
	}

do
	{
	basetime=strace_timetrace(basetime, notfirst);
	notfirst=1;

	if(!tc)
		{
		tchead=tc=wave_alloca(sizeof(struct timechain));
		}
		else
		{
		tc->next=wave_alloca(sizeof(struct timechain));
		tc=tc->next;
		}

	tc->t=basetime;
	tc->next=NULL;
	timearray_size++;
	} while(basetime!=MAX_HISTENT_TIME);

timearray=t=malloc_2(sizeof(TimeType)*timearray_size);
for(i=0;i<timearray_size;i++)
	{
	*(t++)=tchead->t;
	tchead=tchead->next;
	}

if(!shadow_active) update_mark_count_label();
}


/*
 * swap context for mark during trace load...
 */
void swap_strace_contexts(void)
{
struct strace *stemp;
char logical_mutex_temp[6];

stemp = straces;
straces = shadow_straces;
shadow_straces = stemp;

memcpy(logical_mutex_temp, logical_mutex, 6);
memcpy(logical_mutex, shadow_logical_mutex, 6);
memcpy(shadow_logical_mutex, logical_mutex_temp, 6);
}


/*
 * delete context
 */
void delete_strace_context(void)
{
int i;
struct strace *stemp;
struct strace *strace_cache;

for(i=0;i<6;i++)
	{
	shadow_logical_mutex[i] = 0;
	}

strace_cache = straces;	/* so the trace actually deletes */
straces=NULL;

stemp = shadow_straces;
while(stemp)
	{
	shadow_straces = stemp->next;
	if(stemp->string) free_2(stemp->string);

	FreeTrace(stemp->trace);
	free_2(stemp);
	stemp = shadow_straces;
	}

if(shadow_string)
	{
	free_2(shadow_string);
	shadow_string = NULL;
	}

straces = strace_cache;
}

/*************************************************************************/

/*
 * printf to memory..
 */
struct mprintf_buff_t *mprintf_buff_head=NULL, *mprintf_buff_current=NULL;
  
int mprintf(const char *fmt, ... )
{
int len;
int rc;
va_list args;
struct mprintf_buff_t *bt = (struct mprintf_buff_t *)calloc(1, sizeof(struct mprintf_buff_t));
char buff[65537];
                                
va_start(args, fmt);
rc=vsprintf(buff, fmt, args);
len = strlen(buff);
bt->str = malloc_2(len+1);
strcpy(bt->str, buff);
                 
if(!mprintf_buff_current)
        {
        mprintf_buff_head = mprintf_buff_current = bt;
        }
        else
        {
        mprintf_buff_current->next = bt;
        mprintf_buff_current = bt;
        }
                        
return(rc);
}

/*
 * kill mprint buffer
 */
void delete_mprintf(void)
{
if(mprintf_buff_head)
	{
	struct mprintf_buff_t *mb = mprintf_buff_head;
	struct mprintf_buff_t *mbt;		
		
	while(mb)
		{
		free_2(mb->str);
		mbt = mb->next;
		free(mb);
		mb = mbt;
		}

	mprintf_buff_head = mprintf_buff_current = NULL;
	}
}


/*
 * so we can (later) write out the traces which are actually marked...
 */
void cache_actual_pattern_mark_traces(void)
{
Trptr t;
int i;
unsigned int def=0;
TimeType prevshift=LLDescriptor(0);
struct strace *st;

delete_mprintf();

if(timearray)
	{
	mprintf("!%d%d%d%d%d%d\n", logical_mutex[0], logical_mutex[1], logical_mutex[2], logical_mutex[3], logical_mutex[4], logical_mutex[5]);
	st=straces;

	while(st)
		{
		if(st->value==ST_STRING)
			{
			mprintf("?\"%s\n", st->string ? st->string : ""); /* search type for this trace is string.. */
			}
			else
			{
			mprintf("?%02x\n", (unsigned char)st->value);	/* else search type for this trace.. */
			}

		t=st->trace;

		if((t->flags!=def)||(st==straces))
			{
			mprintf("@%x\n",def=t->flags);
			}

		if((t->shift)||((prevshift)&&(!t->shift)))
			{
			mprintf(">"TTFormat"\n", t->shift);
			}
		prevshift=t->shift;

		if(!(t->flags&TR_BLANK))	
			{
			if(t->vector)
				{
				int i;
				nptr *nodes;

				mprintf("#%s",t->name);

				nodes=t->n.vec->bits->nodes;
				for(i=0;i<t->n.vec->nbits;i++)
					{
					if(nodes[i]->expansion)
						{
						mprintf(" (%d)%s",nodes[i]->expansion->parentbit, nodes[i]->expansion->parent->nname);
						}
						else
						{
						mprintf(" %s",nodes[i]->nname);
						}
					}
				mprintf("\n");
				}
				else
				{
				if(t->is_alias)
					{
					if(t->n.nd->expansion)
						{
						mprintf("+%s (%d)%s\n",t->name+2,t->n.nd->expansion->parentbit, t->n.nd->expansion->parent->nname);
						}
						else
						{
						mprintf("+%s %s\n",t->name+2,t->n.nd->nname);
						}
					}
					else
					{
					if(t->n.nd->expansion)
						{
						mprintf("(%d)%s\n",t->n.nd->expansion->parentbit, t->n.nd->expansion->parent->nname);
						}
						else
						{
						mprintf("%s\n",t->n.nd->nname);
						}
					}
				}
			}

		st=st->next;
		} /* while(st)... */

	mprintf("!!\n");	/* mark end of strace region */
	}
}
