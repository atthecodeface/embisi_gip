/* 
 * Copyright (c) Tony Bybell 1999.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include <gtk/gtk.h>
#include <stdio.h>
#include "aet.h"
#include "main.h"
#include "menu.h"
#include "vcd.h"
#include "lxt.h"
#include "ae2.h"
#include "pixmaps.h"
#include "currenttime.h"
#include "fgetdynamic.h"
#include "rc.h"

char *whoami=NULL;

GtkWidget *signalwindow;
GtkWidget *wavewindow;
char paned_pack_semantics=1;	/* 1 for paned_pack, 0 for paned_add */
char zoom_was_explicitly_set=0; /* set on '*' encountered in .sav file read  */
int initial_window_x=700, initial_window_y=400; /* inital window sizes */
char use_scrollbar_only=0;
char force_toolbars=0;		/* 1 sez use toolbar rendering */

int main(int argc, char *argv[])
{
char *fname=NULL;
char *winname=NULL;
static char *winprefix="GTKWave - ";
static char *winstd="GTKWave (stdio) ";
char *wname=NULL;
static char *vcd_autosave_name="vcd_autosave.sav";
int i;
FILE *wave;

GtkWidget *window;
GtkWidget *main_vbox, *top_table, *whole_table;
GtkWidget *menubar;
GtkWidget *text1;
GtkWidget *zoombuttons;
GtkWidget *pagebuttons;
GtkWidget *fetchbuttons;
GtkWidget *discardbuttons;
GtkWidget *shiftbuttons;
GtkWidget *entry;
GtkWidget *timebox;
GtkWidget *panedwindow;
GtkWidget *dummy1, *dummy2;

GtkWidget *toolbar=NULL, *toolhandle=NULL;

char is_vcd=0;

if(argv[0])
	{
	whoami=malloc_2(strlen(argv[0])+1);	/* cache name in case we fork later */
	strcpy(whoami, argv[0]);
	}

gtk_init(&argc, &argv);

if((argc<2)||(argc>3))
        {
        printf("Usage:\n------\n%s  -vcd|tracefilename[.vcd|.lxt] [signalfilename[.gz|.zip]]\n\n"
		"-vcd indicates that stdin is to be used for a VCD trace file.\n\n"
		"To read an LXT file, the suffix must be '.lxt' in order to invoke\n"
		"the correct loader.  If that suffix is not detected, the VCD loader\n"
		"is used instead.  Note that it is permissible if VCD files are\n"
		"compressed with zip or gzip, but LXT files may not be compressed as\n"
		"they are memory mapped!\n\n"
                "See the accompanying documentation for more information.\n\n", argv[0]);
        exit(0);
        }

read_rc_file();
printf("\n%s\n\n",WAVE_VERSION_INFO);

if((argc==2)&&(make_vcd_save_file))
	{
	vcd_save_handle=fopen(vcd_autosave_name,"wb");
	errno=0;	/* just in case */
	}

sym=(struct symbol **)calloc_2(SYMPRIME,sizeof(struct symbol *));

/* load either the vcd or aet file depending on suffix then mode setting */
fname=argv[1];

is_vcd=(!strcmp(fname,"-vcd"));

if(is_vcd)
	{
	winname=malloc_2(strlen(winstd)+4+1);
	strcpy(winname,winstd);
	}
	else
	{
	winname=malloc_2(strlen(fname)+strlen(winprefix)+1);
	strcpy(winname,winprefix);
	}

strcat(winname,fname);


if((strlen(fname)>3)&&(!strcmp(fname+strlen(fname)-4,".lxt")))
	{
	lxt_main(fname);
	}
else
if((strlen(fname)>3)&&(!strcmp(fname+strlen(fname)-4,".aet")))
	{
	ae2_main(fname);
	}
else	/* nothing else left so default to "something" */
	{
	vcd_main(fname);
	}

for(i=0;i<26;i++) named_markers[i]=-1;	/* reset all named markers */

tims.last=max_time;
tims.end=tims.last;		/* until the configure_event of wavearea */
tims.first=tims.start=tims.laststart=min_time;
tims.zoom=tims.prevzoom=0;	/* 1 pixel/ns default */
tims.marker=tims.lmbcache=-1;	/* uninitialized at first */

if((argc==3)||(vcd_save_handle))
	{
	int wave_is_compressed;

	if(!vcd_save_handle)
		{
		wname=argv[2];
		}
		else
		{
		wname=vcd_autosave_name;
		do_initial_zoom_fit=1;
		}

	if(((strlen(wname)>2)&&(!strcmp(wname+strlen(wname)-3,".gz")))||
	   ((strlen(wname)>3)&&(!strcmp(wname+strlen(wname)-4,".zip"))))
	        {
	        char *str;
        	int dlen;
        	dlen=strlen(WAVE_DECOMPRESSOR);
	        str=wave_alloca(strlen(wname)+dlen+1);
	        strcpy(str,WAVE_DECOMPRESSOR);
	        strcpy(str+dlen,wname);
	        wave=popen(str,"r");
	        wave_is_compressed=~0;
	        }
	        else
	        {
	        wave=fopen(wname,"rb");
	        wave_is_compressed=0;
	        }

	if(!wave)
	        {
	        fprintf(stderr, "** WARNING: Error opening .sav file '%s', skipping.\n",wname);
	        }	
	        else
	        {
	        char *iline;
		char any_shadow = 0;

		default_flags=TR_RJUSTIFY;
		shift_timebase_default_for_add=LLDescriptor(0);
	        while((iline=fgetmalloc(wave)))
	                {
	                parsewavline(iline);
			any_shadow |= shadow_active;
			free_2(iline);
	                }
		default_flags=TR_RJUSTIFY;
		shift_timebase_default_for_add=LLDescriptor(0);
		if(wave_is_compressed) pclose(wave); else fclose(wave);

                if(any_shadow)
                        {
                        if(shadow_straces)
                                {
                                shadow_active = 1;

                                swap_strace_contexts();
                                strace_maketimetrace(1);
                                swap_strace_contexts();

				shadow_active = 0;
                                }
                        }
	        }
	}

if ((!zoom_was_explicitly_set)&&
	((tims.last-tims.first)<=400)) do_initial_zoom_fit=1;  /* force zoom on small traces */

calczoom(tims.zoom);

window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
gtk_window_set_title(GTK_WINDOW(window), winname);
gtk_widget_set_usize(GTK_WIDGET(window), initial_window_x, initial_window_y);
gtk_signal_connect(GTK_OBJECT(window), "destroy", 
			GTK_SIGNAL_FUNC(file_quit_cmd_callback), 
		       	"WM destroy");
gtk_widget_show(window);
make_pixmaps(window);

main_vbox = gtk_vbox_new(FALSE, 5);
gtk_container_border_width(GTK_CONTAINER(main_vbox), 1);
gtk_container_add(GTK_CONTAINER(window), main_vbox);
gtk_widget_show(main_vbox);
    
get_main_menu(window, &menubar);
gtk_widget_show(menubar);

if(force_toolbars)
	{
	toolhandle=gtk_handle_box_new();
	gtk_widget_show(toolhandle);
	gtk_container_add(GTK_CONTAINER(toolhandle), menubar);
	gtk_box_pack_start(GTK_BOX(main_vbox), toolhandle, FALSE, TRUE, 0);
	}
	else
	{
	gtk_box_pack_start(GTK_BOX(main_vbox), menubar, FALSE, TRUE, 0);
	}

top_table = gtk_table_new (1, 256, FALSE);

if(force_toolbars)
	{
	toolhandle=gtk_handle_box_new();
	gtk_widget_show(toolhandle);
	gtk_container_add(GTK_CONTAINER(toolhandle), top_table);
	}

whole_table = gtk_table_new (256, 16, FALSE);

text1 = create_text ();
gtk_table_attach (GTK_TABLE (top_table), text1, 0, 141, 0, 1,
                      	GTK_FILL | GTK_EXPAND,
                      	GTK_FILL | GTK_SHRINK, 0, 0);
gtk_widget_set_usize(GTK_WIDGET(text1), 200, -1);
gtk_widget_show (text1);

dummy1=gtk_label_new("");
gtk_table_attach (GTK_TABLE (top_table), dummy1, 141, 171, 0, 1,
                      	GTK_FILL | GTK_EXPAND,
                      	GTK_SHRINK, 0, 0);
gtk_widget_show (dummy1);

zoombuttons = create_zoom_buttons ();
gtk_table_attach (GTK_TABLE (top_table), zoombuttons, 171, 173, 0, 1,
                      	GTK_FILL | GTK_EXPAND,
                      	GTK_SHRINK, 0, 0);
gtk_widget_show (zoombuttons);

if(!use_scrollbar_only)
	{
	pagebuttons = create_page_buttons ();
	gtk_table_attach (GTK_TABLE (top_table), pagebuttons, 173, 174, 0, 1,
	                      	GTK_FILL | GTK_EXPAND,
	                      	GTK_SHRINK, 0, 0);
	gtk_widget_show (pagebuttons);
	fetchbuttons = create_fetch_buttons ();
	gtk_table_attach (GTK_TABLE (top_table), fetchbuttons, 174, 175, 0, 1,
	                      	GTK_FILL | GTK_EXPAND,
	                      	GTK_SHRINK, 0, 0);
	gtk_widget_show (fetchbuttons);
	discardbuttons = create_discard_buttons ();
	gtk_table_attach (GTK_TABLE (top_table), discardbuttons, 175, 176, 0, 1,
	                      	GTK_FILL | GTK_EXPAND,
	                      	GTK_SHRINK, 0, 0);
	gtk_widget_show (discardbuttons);
	
	shiftbuttons = create_shift_buttons ();
	gtk_table_attach (GTK_TABLE (top_table), shiftbuttons, 176, 177, 0, 1,
	                      	GTK_FILL | GTK_EXPAND,
	                      	GTK_SHRINK, 0, 0);
	gtk_widget_show (shiftbuttons);
	}

dummy2=gtk_label_new("");
gtk_table_attach (GTK_TABLE (top_table), dummy2, 177, 215, 0, 1,
                      	GTK_FILL | GTK_EXPAND,
                      	GTK_SHRINK, 0, 0);
gtk_widget_show (dummy2);

entry = create_entry_box();
gtk_table_attach (GTK_TABLE (top_table), entry, 215, 216, 0, 1,
                      	GTK_SHRINK,
                      	GTK_SHRINK, 0, 0);
gtk_widget_show(entry);

timebox = create_time_box();
gtk_table_attach (GTK_TABLE (top_table), timebox, 216, 256, 0, 1,
                      	GTK_FILL | GTK_EXPAND,
                      	GTK_FILL | GTK_EXPAND | GTK_SHRINK, 0, 0);
gtk_widget_show (timebox);

wavewindow = create_wavewindow();
load_all_fonts(); /* must be done before create_signalwindow() */
gtk_widget_show(wavewindow);
signalwindow = create_signalwindow();

if(do_resize_signals) 
                {
                int os;
                os=max_signal_name_pixel_width;
                os=(os<30)?30:os;
                gtk_widget_set_usize(GTK_WIDGET(signalwindow),
                                os+30, -1);
                }

gtk_widget_show(signalwindow);

panedwindow=gtk_hpaned_new();

#ifdef HAVE_PANED_PACK
if(paned_pack_semantics)
	{
	gtk_paned_pack1(GTK_PANED(panedwindow), signalwindow, 0, 0); 
	gtk_paned_pack2(GTK_PANED(panedwindow), wavewindow, ~0, 0);
	}
	else
#endif
	{
	gtk_paned_add1(GTK_PANED(panedwindow), signalwindow);
	gtk_paned_add2(GTK_PANED(panedwindow), wavewindow);
	}

gtk_widget_show(panedwindow);

gtk_widget_show(top_table);

gtk_table_attach (GTK_TABLE (whole_table), force_toolbars?toolhandle:top_table, 0, 16, 0, 1,
                      	GTK_FILL | GTK_EXPAND,
                      	GTK_FILL | GTK_EXPAND | GTK_SHRINK, 0, 0);
gtk_table_attach (GTK_TABLE (whole_table), panedwindow, 0, 16, 1, 256,
                      	GTK_FILL | GTK_EXPAND,
                      	GTK_FILL | GTK_EXPAND | GTK_SHRINK, 0, 0);
gtk_widget_show(whole_table);
gtk_container_add (GTK_CONTAINER (main_vbox), whole_table);

update_markertime(time_trunc(tims.marker));

gtk_main();
    
return(0);
}
