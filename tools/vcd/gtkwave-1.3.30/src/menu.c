/* 
 * Copyright (c) Tony Bybell 1999-2003.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

/*
 * note: any functions which add/remove traces must first look at
 * the global "straces".  if it's active, complain to the status
 * window and don't do the op. same for "dnd_state".
 */

#include "gtk12compat.h"
#include "menu.h"
#include "vcd.h"
#ifndef _MSC_VER
        #include <unistd.h>
#endif


/*
 * this enum MUST remain in sync with the menu_items struct
 * or fireworks will result!
 */
enum WV_MenuItems {
#ifndef _MSC_VER
WV_MENU_FONV,
WV_MENU_SEP0,
#endif
WV_MENU_FPTF,
WV_MENU_SEP1,
WV_MENU_FRSF,
WV_MENU_FWSF,
WV_MENU_SEP2,
WV_MENU_FQY,
WV_MENU_FQN,
WV_MENU_ESTMH,
WV_MENU_SEP3,
WV_MENU_EIB,
WV_MENU_EIC,
WV_MENU_EAHT,
WV_MENU_ERHA,
WV_MENU_EC,
WV_MENU_EP,
WV_MENU_SEP4,
WV_MENU_EE,
WV_MENU_ECD,
WV_MENU_ECU,
WV_MENU_ERSBV,
WV_MENU_SEP5,
WV_MENU_EDFH,
WV_MENU_EDFD,
WV_MENU_EDFSD,
WV_MENU_EDFB,
WV_MENU_EDFO,
WV_MENU_EDFA,
WV_MENU_EDFRJON,
WV_MENU_EDFRJOFF,
WV_MENU_EDFION,
WV_MENU_EDFIOFF,
WV_MENU_EDFRON,
WV_MENU_EDFROFF,
WV_MENU_ESCAH,
WV_MENU_ESCFH,
WV_MENU_SEP6,
WV_MENU_WARP,
WV_MENU_UNWARP,
WV_MENU_UNWARPA,
WV_MENU_SEP7A,
WV_MENU_EEX,
WV_MENU_ESH,
WV_MENU_SEP6A,
WV_MENU_EHR,
WV_MENU_EUHR,
WV_MENU_EHA,
WV_MENU_EUHA,
WV_MENU_SEP6B,
WV_MENU_ALPHA,
WV_MENU_ALPHA2,
WV_MENU_LEX,
WV_MENU_RVS,
WV_MENU_SPS,
WV_MENU_SEP7B,
WV_MENU_SSR,
WV_MENU_SSH,
WV_MENU_SST,
WV_MENU_SEP7,
WV_MENU_ACOL,
WV_MENU_ACOLR,
WV_MENU_ABON,
WV_MENU_HTGP,
WV_MENU_TMTT,
WV_MENU_TZZA,
WV_MENU_TZZB,
WV_MENU_TZZI,
WV_MENU_TZZO,
WV_MENU_TZZBF,
WV_MENU_TZZTS,
WV_MENU_TZZTE,
WV_MENU_TZUZ,
WV_MENU_TFFS,
WV_MENU_TFFR,
WV_MENU_TFFL,
WV_MENU_TDDR,
WV_MENU_TDDL,
WV_MENU_TSSR,
WV_MENU_TSSL,
WV_MENU_TPPR,
WV_MENU_TPPL,
WV_MENU_MSCMD,
WV_MENU_MDNM,
WV_MENU_MCNM,
WV_MENU_MCANM,
WV_MENU_MDPM,
WV_MENU_SEP8,
WV_MENU_MWSON,
WV_MENU_VSG,
WV_MENU_SEP9,
WV_MENU_VSBS,
WV_MENU_SEP10,
WV_MENU_VDR,
WV_MENU_SEP11,
WV_MENU_VCZ,
WV_MENU_SEP12,
WV_MENU_VTMM,
WV_MENU_SEP13,
WV_MENU_VCMU,
WV_MENU_SEP14,
WV_MENU_VDRV,
WV_MENU_SEP15,
WV_MENU_VLJS,
WV_MENU_VRJS,
WV_MENU_SEP16,
WV_MENU_VZPS,
WV_MENU_VFTP,
WV_MENU_SEP17,
WV_MENU_RMRKS,
WV_MENU_SEP18,
WV_MENU_LXTCC2Z,
WV_MENU_HWH,
WV_MENU_HWV,
};

extern GtkItemFactoryEntry menu_items[];
static GtkItemFactory *item_factory=NULL;

static char regexp_string[129]="";
static Trptr trace_to_alias=NULL;
static Trptr showchangeall=NULL;

static char *filesel_writesave=NULL,
	    *filesel_readsave=NULL,
	    *filesel_newviewer=NULL;




static void menu_unwarp_traces_all(GtkWidget *widget, gpointer data)
{
Trptr t;
int found=0;

if(helpbox_is_active)
        {
        help_text_bold("\n\nUnwarp All");
        help_text(
                " unconditionally removes all offsets on all traces."
        );
        return;
        }

t=traces.first;
while(t)
	{
	if(t->shift)
		{
		t->shift=LLDescriptor(0);
		found++;
		}
	t=t->next;
	}

if(found)
	{
	signalwindow_width_dirty=1;
	MaxSignalLength();
	signalarea_configure_event(signalarea, NULL);
	wavearea_configure_event(wavearea, NULL);
	}
}

static void menu_unwarp_traces(GtkWidget *widget, gpointer data)
{
Trptr t;
int found=0;

if(helpbox_is_active)
        {
        help_text_bold("\n\nUnwarp Marked");
        help_text(
                " removes all offsets on all highlighted traces."
        );
        return;
        }

t=traces.first;
while(t)
	{
	if(t->flags&TR_HIGHLIGHT)
		{
		t->shift=LLDescriptor(0);
		t->flags&=(~TR_HIGHLIGHT);
		found++;
		}
	t=t->next;
	}

if(found)
	{
	signalwindow_width_dirty=1;
	MaxSignalLength();
	signalarea_configure_event(signalarea, NULL);
	wavearea_configure_event(wavearea, NULL);
	}
}

static void warp_cleanup(GtkWidget *widget, gpointer data)
{
if(entrybox_text)
	{
	TimeType gt, delta;
	Trptr t;

	gt=unformat_time(entrybox_text, time_dimension);
	free_2(entrybox_text);
	entrybox_text=NULL;

	if(gt<0)
		{
		delta=tims.first-tims.last;
		if(gt<delta) gt=delta;
		}
	else
	if(gt>0)
		{
		delta=tims.last-tims.first;
		if(gt>delta) gt=delta;
		}

	t=traces.first;
	while(t)
		{
		if(t->flags&TR_HIGHLIGHT)
			{
			if((!(t->flags&TR_BLANK))&&(t->name))	/* though note if a user specifies comment warping in a .sav file we will honor it.. */
				{
				t->shift=gt;
				}
				else
				{
				t->shift=LLDescriptor(0);
				}
			t->flags&=(~TR_HIGHLIGHT);
			}
		t=t->next;
		}
	}

	signalwindow_width_dirty=1;
	MaxSignalLength();
	signalarea_configure_event(signalarea, NULL);
	wavearea_configure_event(wavearea, NULL);
}

static void menu_warp_traces(GtkWidget *widget, gpointer data)
{
char gt[32];
Trptr t;
int found=0;

if(helpbox_is_active)
        {
        help_text_bold("\n\nWarp Marked");
        help_text(
                " offsets all highlighted traces by the amount of"
                " time entered in the requester.  (Positive values"
		" will shift traces to the right.) Note that warp"
		" operations to not persist after a combine operation."
		" Attempting to shift greater than the absolute value of total simulation"
		" time will cap the shift magnitude at the length of simulation."
        );
        return;
        }


t=traces.first;
while(t)
	{
	if(t->flags&TR_HIGHLIGHT)
		{
		found++;
		break;
		}
	t=t->next;
	}

if(found)
	{
	reformat_time(gt, LLDescriptor(0), time_dimension);
	entrybox("Warp Traces",200,gt,20,warp_cleanup);
	}
}





static void wave_scrolling_on(GtkWidget *widget, gpointer data)
{
if(helpbox_is_active)
        {
        help_text_bold("\n\nWave Scrolling");
        help_text(
		" allows movement of the primary marker beyond screen boundaries"
		" which causes the wave window to scroll when enabled."
		" When disabled, it"
		" disallows movement of the primary marker beyond screen boundaries."
        );
        }
	else
	{
	if(!wave_scrolling)
		{
		status_text("Wave Scrolling On.\n");
		wave_scrolling=1;
		}
		else
		{
		status_text("Wave Scrolling Off.\n");
		wave_scrolling=0;
		}
	}

GTK_CHECK_MENU_ITEM(gtk_item_factory_get_widget(item_factory, 
	menu_items[WV_MENU_MWSON].path))->active=(wave_scrolling)?TRUE:FALSE;
}
/**/

static void menu_autocoalesce(GtkWidget *widget, gpointer data)
{
if(helpbox_is_active)
        {
        help_text_bold("\n\nAutocoalesce");
        help_text(
		" when enabled"
		" allows the wave viewer to reconstruct split vectors."
		" Split vectors will be indicated by a \"[]\""
		" prefix in the search requesters."
        );
        }
	else
	{
	if(!autocoalesce)
		{
		status_text("Autocoalesce On.\n");
		autocoalesce=1;
		}
		else
		{
		status_text("Autocoalesce Off.\n");
		autocoalesce=0;
		}
	}

GTK_CHECK_MENU_ITEM(gtk_item_factory_get_widget(item_factory, 
	menu_items[WV_MENU_ACOL].path))->active=(autocoalesce)?TRUE:FALSE;
}

static void menu_autocoalesce_reversal(GtkWidget *widget, gpointer data)
{
if(helpbox_is_active)
        {
        help_text_bold("\n\nAutocoalesce Reversal");
        help_text(
		" causes split vectors to be reconstructed in reverse order (only if autocoalesce is also active).  This is necessary with some simulators."
		" Split vectors will be indicated by a \"[]\""
		" prefix in the search requesters."
        );
        }
	else
	{
	if(!autocoalesce_reversal)
		{
		status_text("Autocoalesce Rvs On.\n");
		autocoalesce_reversal=1;
		}
		else
		{
		status_text("Autocoalesce Rvs Off.\n");
		autocoalesce_reversal=0;
		}
	}

GTK_CHECK_MENU_ITEM(gtk_item_factory_get_widget(item_factory, 
	menu_items[WV_MENU_ACOLR].path))->active=(autocoalesce_reversal)?TRUE:FALSE;
}

static void menu_autoname_bundles_on(GtkWidget *widget, gpointer data)
{
if(helpbox_is_active)
        {
        help_text_bold("\n\nAutoname Bundles");
        help_text(
		" when enabled"
		" modifies the bundle up/down operations in the hierarchy"
		" and tree searches such that a NULL bundle name is"
		" implicitly created which informs GTKWave to create bundle"
		" and signal names based on the position in the hierarchy."
		" When disabled, it"
		" modifies the bundle up/down operations in the hierarchy"
		" and tree searches such that a NULL bundle name is"
		" not implicitly created.  This informs GTKWave to create bundle"
		" and signal names based on the position in the hierarchy"
		" only if the user enters a zero-length bundle name.  This"
		" behavior is the default."
        );
        }
	else
	{
	if(!autoname_bundles)
		{
		status_text("Autoname On.\n");
		autoname_bundles=1;
		}
		else
		{
		status_text("Autoname Off.\n");
		autoname_bundles=0;
		}
	}

GTK_CHECK_MENU_ITEM(gtk_item_factory_get_widget(item_factory, 
	menu_items[WV_MENU_ABON].path))->active=(autoname_bundles)?TRUE:FALSE;
}


static void menu_hgrouping(GtkWidget *widget, gpointer data)
{
if(helpbox_is_active)
        {
        help_text_bold("\n\nSearch Hierarchy Grouping");
        help_text(
		" when enabled ensures that new members added to the ``Tree Search'' and"
		" ``Hierarchy Search'' widgets are added alphanumerically: first hierarchy names as a group followed by signal names as a group."
		" This is the default and is recommended.  When disabled, hierarchy names and signal names are interleaved together in"
		" strict alphanumerical ordering."
		" Note that due to the caching mechanism in ``Tree Search'', dynamically changing this flag when the widget is active "
		" may not produce immediately obvious results.  Closing the widget then opening it up again will ensure that it follows the"
		" behavior of this flag."
        );
        }
	else
	{
	if(!hier_grouping)
		{
		status_text("Hier Grouping On.\n");
		hier_grouping=1;
		}
		else
		{
		status_text("Hier Grouping Off.\n");
		hier_grouping=0;
		}
	}

GTK_CHECK_MENU_ITEM(gtk_item_factory_get_widget(item_factory, 
	menu_items[WV_MENU_HTGP].path))->active=(hier_grouping)?TRUE:FALSE;
}


static void max_hier_cleanup(GtkWidget *widget, gpointer data)
{
if(entrybox_text)
	{
	char update_string[128];
	Trptr t;
	int i;

	hier_max_level=atoi_64(entrybox_text);
	if(hier_max_level<0) hier_max_level=0;
	free_2(entrybox_text);
	entrybox_text=NULL;

	for(i=0;i<2;i++)
		{
		if(i==0) t=traces.first; else t=traces.buffer;

		while(t)
			{
			if(!(t->flags&TR_BLANK))
				{
				if(t->vector==TRUE)
					{
	    				if(!hier_max_level)
	        				{
	        				t->name = t->n.vec->name;
	        				}
	        				else
	        				{
	        				t->name = hier_extract(t->n.vec->name, hier_max_level);
	        				}
					}
					else 
					if(!t->is_alias)
					{
	        			if(!hier_max_level)
	                			{
	                			t->name = t->n.nd->nname;
	                			}
	                			else
	                			{
	                			t->name = hier_extract(t->n.nd->nname, hier_max_level);
	                			} 
					}
				}
			t=t->next;
			}
		}

	signalwindow_width_dirty=1;
	MaxSignalLength();
	signalarea_configure_event(signalarea, NULL);
	wavearea_configure_event(wavearea, NULL);
	sprintf(update_string, "Trace Hier Max Depth is now: %d\n", hier_max_level);
	status_text(update_string);
	}
}

static void menu_set_max_hier(GtkWidget *widget, gpointer data)
{
char za[32];

if(helpbox_is_active)
        {
        help_text_bold("\n\nSet Max Hier");
        help_text(
		" sets the maximum hierarchy depth (counting from the right"
		" with bit numbers or ranges ignored) that is displayable"
		" for trace names.  Zero indicates that no truncation will"
		" be performed (default).  Note that any aliased signals"
		" (prefix of a \"+\") will not have truncated names." 
        );
        return;
        }


sprintf(za,"%d",hier_max_level);

entrybox("Max Hier Depth",200,za,20,max_hier_cleanup);
}


/**/
static void menu_use_roundcaps(GtkWidget *widget, gpointer data)
{
if(helpbox_is_active)
        {
        help_text_bold("\n\nDraw Roundcapped Vectors");
        help_text(
		" draws vector transitions that have sloping edges when enabled."
		" Draws vector transitions that have sharp edges when disabled;"
		" this is the default."
        );
        }
	else
	{
	if(!use_roundcaps)
		{
		status_text("Using roundcaps.\n");
		use_roundcaps=1;
		}
		else
		{
		status_text("Using flatcaps.\n");
		use_roundcaps=0;
		}
	MaxSignalLength();
	signalarea_configure_event(signalarea, NULL);
	wavearea_configure_event(wavearea, NULL);
	}

GTK_CHECK_MENU_ITEM(gtk_item_factory_get_widget(item_factory, 
	menu_items[WV_MENU_VDRV].path))->active=(use_roundcaps)?TRUE:FALSE;
}

/**/
static void menu_lxt_clk_compress(GtkWidget *widget, gpointer data)
{
if(helpbox_is_active)
        {
        help_text_bold("\n\nLXT Clock Compress to Z");
        help_text(
		" reduces memory usage when active as clocks compressed in LXT format are"
		" kept at Z in order to save memory.  Traces imported with this are permanently"
		" kept at Z."
        );
        }
	else
	{
	if(lxt_clock_compress_to_z)
		{
		lxt_clock_compress_to_z=0;
		status_text("LXT CC2Z Off.\n");
		}
		else
		{
		lxt_clock_compress_to_z=1;
		status_text("LXT CC2Z On.\n");
		}
	}

GTK_CHECK_MENU_ITEM(gtk_item_factory_get_widget(item_factory, 
	menu_items[WV_MENU_LXTCC2Z].path))->active=(lxt_clock_compress_to_z)?TRUE:FALSE;
}
/**/
static void menu_use_full_precision(GtkWidget *widget, gpointer data)
{
if(helpbox_is_active)
        {
        help_text_bold("\n\nFull Precision");
        help_text(
		" does not round time values when the number of ticks per pixel onscreen is greater than"
		" 10 when active.  The default is that this feature is disabled."
        );
        }
	else
	{
	if(use_full_precision)
		{
		use_full_precision=0;
		status_text("Full Prec Off.\n");
		}
		else
		{
		use_full_precision=1;
		status_text("Full Prec On.\n");
		}

	calczoom(tims.zoom);
	fix_wavehadj();
                        
	gtk_signal_emit_by_name (GTK_OBJECT (GTK_ADJUSTMENT(wave_hslider)), "changed"); /* force zoom update */
	gtk_signal_emit_by_name (GTK_OBJECT (GTK_ADJUSTMENT(wave_hslider)), "value_changed"); /* force zoom update */
	update_maxmarker_labels();
	}

GTK_CHECK_MENU_ITEM(gtk_item_factory_get_widget(item_factory, 
	menu_items[WV_MENU_VFTP].path))->active=(use_full_precision)?TRUE:FALSE;
}
/**/
static void menu_remove_marked(GtkWidget *widget, gpointer data)
{
if(helpbox_is_active)
        {
        help_text_bold("\n\nRemove Pattern Marks");
        help_text(
		" removes any vertical traces on the display caused by the Mark"
		" feature in pattern search and reverts to the normal format."
        );
        }
	else
	{
	if(shadow_straces)
		{
		delete_strace_context();
		}

	strace_maketimetrace(0);
  
	MaxSignalLength();
	signalarea_configure_event(signalarea, NULL);
	wavearea_configure_event(wavearea, NULL);
	}
}
/**/
static void menu_zoom10_snap(GtkWidget *widget, gpointer data)
{
if(helpbox_is_active)
        {
        help_text_bold("\n\nZoom Pow10 Snap");
        help_text(
		" snaps time values to a power of ten boundary when active.  Fractional zooms are"
		" internally stored, but what is actually displayed will be rounded up/down to the"
		" nearest power of 10.  This only works when the ticks per frame is greater than 100"
		" units."
        );
        }
	else
	{
	if(zoom_pow10_snap)
		{
		zoom_pow10_snap=0;
		status_text("Pow10 Snap Off.\n");
		}
		else
		{
		zoom_pow10_snap=1;
		status_text("Pow10 Snap On.\n");
		}

	calczoom(tims.zoom);
	fix_wavehadj();
                        
	gtk_signal_emit_by_name (GTK_OBJECT (GTK_ADJUSTMENT(wave_hslider)), "changed"); /* force zoom update */
	gtk_signal_emit_by_name (GTK_OBJECT (GTK_ADJUSTMENT(wave_hslider)), "value_changed"); /* force zoom update */
	}

GTK_CHECK_MENU_ITEM(gtk_item_factory_get_widget(item_factory, 
	menu_items[WV_MENU_VZPS].path))->active=(zoom_pow10_snap)?TRUE:FALSE;
}

/**/
static void menu_left_justify(GtkWidget *widget, gpointer data)
{
if(helpbox_is_active)
        {
        help_text_bold("\n\nLeft Justify Signals");
        help_text(
		" draws signal names flushed to the left border of the signal window."
        );
        }
	else
	{
	status_text("Left Justification.\n");
	left_justify_sigs=~0;
	MaxSignalLength();
	signalarea_configure_event(signalarea, NULL);
	}
}

/**/
static void menu_right_justify(GtkWidget *widget, gpointer data)
{
if(helpbox_is_active)
        {
        help_text_bold("\n\nRight Justify Signals");
        help_text(
		" draws signal names flushed to the right (\"equals\") side of the signal window."
        );
        }
	else
	{
	status_text("Right Justification.\n");
	left_justify_sigs=0;
	MaxSignalLength();
	signalarea_configure_event(signalarea, NULL);
	}
}

/**/
static void menu_enable_constant_marker_update(GtkWidget *widget, gpointer data)
{
if(helpbox_is_active)
        {
        help_text_bold("\n\nConstant Marker Update");
        help_text(
		" when enabled,"
		" allows GTKWave to dynamically show the changing values of the"
		" traces under the primary marker while it is being dragged"
		" across the screen.  This works best with dynamic resizing disabled."
		" When disabled, it"
		" restricts GTKWave to only update the trace values when the"
		" left mouse button is initially pressed then again when it is released."
		" This is the default behavior."
        );
        }
	else
	{
	if(!constant_marker_update)
		{
		status_text("Constant marker update enabled.\n");
		constant_marker_update=~0;
		}
		else
		{
		status_text("Constant marker update disabled.\n");
		constant_marker_update=0;
		}
	}

GTK_CHECK_MENU_ITEM(gtk_item_factory_get_widget(item_factory, 
	menu_items[WV_MENU_VCMU].path))->active=(constant_marker_update)?TRUE:FALSE;
}
/**/
static void menu_enable_dynamic_resize(GtkWidget *widget, gpointer data)
{
if(helpbox_is_active)
        {
        help_text_bold("\n\nDynamic Resize");
        help_text(
		" allows GTKWave to dynamically resize the signal"
		" window for you when toggled active.  This can be helpful during numerous"
		" signal additions and/or deletions.  This is the default"
		" behavior."
        );
        }
	else
	{
	if(!do_resize_signals)
		{
		status_text("Resizing enabled.\n");
		do_resize_signals=~0;
		}
		else
		{
		status_text("Resizing disabled.\n");
		do_resize_signals=0;
		}
	signalwindow_width_dirty=1;
	MaxSignalLength();
	signalarea_configure_event(signalarea, NULL);
	wavearea_configure_event(wavearea, NULL);
	}

GTK_CHECK_MENU_ITEM(gtk_item_factory_get_widget(item_factory, 
	menu_items[WV_MENU_VDR].path))->active=(do_resize_signals)?TRUE:FALSE;
}
/**/
static void menu_toggle_max_or_marker(GtkWidget *widget, gpointer data)
{
if(helpbox_is_active)
        {
        help_text_bold("\n\nToggle Max-Marker");
        help_text(
		" allows you to switch between the maximum time and"
		" marker time for display in the upper right corner"
		" of the main window.  Default behavior is that the"
		" maximum time is displayed."
        );
        }
	else
	{
	use_maxtime_display=(use_maxtime_display)?0:1;
	update_maxmarker_labels();
	}
}
/**/
static void menu_help(GtkWidget *widget, gpointer data)
{
if(helpbox_is_active)
        {
        help_text_bold("\n\nWave Help");
        help_text(
		" is already active.  It's this window."
        );
        return;
        }

helpbox("Wave Help",300,"Select any main window menu item");
}
/**/
static void menu_version(GtkWidget *widget, gpointer data)
{
if(helpbox_is_active)
        {
        help_text_bold("\n\nWave Version");
        help_text(
                " merely brings up a requester which indicates the current"
		" version of this program."
        );
        return;
        }

simplereqbox("Wave Version",300,WAVE_VERSION_INFO,"OK", NULL, NULL);
}
/**/
static void menu_quit_callback(GtkWidget *widget, gpointer data)
{
if(data)
	{
	g_print("Exiting.\n");	
	gtk_exit(0);
	}
}
static void menu_quit(GtkWidget *widget, gpointer data)
{
if(helpbox_is_active)
	{
	help_text_bold("\n\nQuit");
	help_text(
		" exits GTKWave after an additional confirmation"
		" requester is given the OK to quit."
	);
	return;
	}

simplereqbox("Quit Program",300,"Do you really want to quit?","Yes", "No", menu_quit_callback);
}
/**/
static void must_sel(void)
{
status_text("Select one or more traces.\n");
}
static void must_sel_nb(void)
{
status_text("Select one or more nonblank traces.\n");
}
/**/

static void
menu_expand(GtkWidget *widget, gpointer data)
{
Trptr t, tmp;
int tmpi,dirty=0;

if(helpbox_is_active)
        {
        help_text_bold("\n\nExpand");
        help_text(
		" decomposes the highlighted signals into their individual bits."
		" The resulting bits are converted to traces and inserted after the"
		" last highlighted trace.  The original unexpanded traces will"
		" be placed in the cut buffer."
		" It will function seemingly randomly"
		" when used upon real valued single-bit traces."
		" When used upon multi-bit vectors that contain "
		" real valued traces, those traces will expand to their normal \"correct\" values,"
		" not individual bits."
        );
        return;
        }


if(dnd_state) { dnd_error(); return; } /* don't mess with sigs when dnd active */

DEBUG(printf("Expand Traces\n"));

t=traces.first;
while(t)
	{
	if((t->flags&TR_HIGHLIGHT)&&(!(t->flags&TR_BLANK)))
		{
		dirty=1;
		break;
		}
	t=t->next;
	}

if(dirty)
	{
	FreeCutBuffer();
	traces.buffer=traces.first;
	traces.bufferlast=traces.last;
	traces.buffercount=traces.total;

	traces.first=traces.last=NULL; traces.total=0;

	t=traces.buffer;

	while(t)
		{
		if(t->flags&TR_HIGHLIGHT)
			{
			if(t->flags&TR_BLANK)
				{
				AddBlankTrace(t->name);
				}
				else
				{
				if(t->vector)
					{
					bptr bits;	
					int i;

					bits=t->n.vec->bits;
					if(!(t->flags&TR_REVERSE))
						{
						for(i=0;i<bits->nbits;i++)
							{
							if(bits->nodes[i]->expansion) bits->nodes[i]->expansion->refcnt++;
							AddNode(bits->nodes[i],NULL);
							}
						}
						else
						{
						for(i=(bits->nbits-1);i>-1;i--)
							{
							if(bits->nodes[i]->expansion) bits->nodes[i]->expansion->refcnt++;
							AddNode(bits->nodes[i],NULL);
							}
						}
					}
					else
					{
					eptr e=ExpandNode(t->n.nd);
					int i;
					if(!e)
						{
						if(t->n.nd->expansion) t->n.nd->expansion->refcnt++;
						AddNode(t->n.nd,NULL);
						}
						else
						{
						for(i=0;i<e->width;i++)
							{
							AddNode(e->narray[i], NULL);						
							}
						free(e->narray);
						free(e);
						}
					}
				}
			}
		t=t->next;
		}

	tmp=traces.buffer; traces.buffer=traces.first; traces.first=tmp;
	tmp=traces.bufferlast; traces.bufferlast=traces.last; traces.last=tmp;
	tmpi=traces.buffercount; traces.buffercount=traces.total;
				traces.total=tmpi;
	PasteBuffer();
	CutBuffer();
	
	signalwindow_width_dirty=1;
	MaxSignalLength();
	signalarea_configure_event(signalarea, NULL);
	wavearea_configure_event(wavearea, NULL);
	}
	else
	{
	must_sel_nb();
	}
}

static void
menu_combine(int direction)
{
Trptr t, tmp;
int tmpi,dirty=0;
nptr bitblast_parent;
int bitblast_delta=0;

if(dnd_state) { dnd_error(); return; } /* don't mess with sigs when dnd active */

DEBUG(printf("Combine Traces\n"));

t=traces.first;
while(t)
	{
	if((t->flags&TR_HIGHLIGHT)&&(!(t->flags&TR_BLANK)))
		{
		if(t->vector)
			{
			dirty+=t->n.vec->nbits;
			}
			else
			{
			if(t->n.nd->ext)
				{
				int msb, lsb, width;
				msb = t->n.nd->ext->msi;
				lsb = t->n.nd->ext->lsi;
				if(msb>lsb) width = msb-lsb+1; else width = lsb-msb+1;
				dirty += width;
				}
				else
				{
				dirty++;
				}
			}
		}
	t=t->next;
	}

if(!dirty)
	{
	must_sel_nb();
	return;
	}

if(dirty>512)
	{
	char buf[512];

	sprintf(buf,"%d bits selected, please use <= 512.\n",dirty);
	status_text(buf);
	}
	else
	{
	int i,nodepnt=0;
	struct Node *n[512];
	struct Bits *b=NULL;
	bvptr v=NULL;

	FreeCutBuffer();
	traces.buffer=traces.first;
	traces.bufferlast=traces.last;
	traces.buffercount=traces.total;

	traces.first=traces.last=NULL; traces.total=0;

	t=traces.buffer;

	while(t)
		{
		if(t->flags&TR_HIGHLIGHT)
			{
			if(t->flags&TR_BLANK)
				{
				/* nothing */
				}
				else
				{
				if(t->vector)
					{
					bptr bits;	
					int i;

					bits=t->n.vec->bits;

					if(!(t->flags&TR_REVERSE))
						{
						for(i=0;i<bits->nbits;i++)
							{
							if(bits->nodes[i]->expansion) bits->nodes[i]->expansion->refcnt++;
							n[nodepnt++]=bits->nodes[i];
							}
						}
						else
						{
						for(i=(bits->nbits-1);i>-1;i--)
							{
							if(bits->nodes[i]->expansion) bits->nodes[i]->expansion->refcnt++;
							n[nodepnt++]=bits->nodes[i];
							}
						}
					}
					else
					{
					eptr e=ExpandNode(t->n.nd);
					int i;
					if(!e)
						{
						if(t->n.nd->expansion) t->n.nd->expansion->refcnt++;
						n[nodepnt++]=t->n.nd;
						}
						else
						{
						for(i=0;i<e->width;i++)
							{
							n[nodepnt++]=e->narray[i];	
							e->narray[i]->expansion->refcnt++;
							}
						free(e->narray);
						free(e);
						}
					}
				}
			}
		if(nodepnt==dirty) break;
		t=t->next;
		}

        b=(struct Bits *)calloc_2(1,sizeof(struct Bits)+(nodepnt-1)*
                                  sizeof(struct Node *));
                
	if(n[0]->expansion)
		{
		bitblast_parent = n[0]->expansion->parent;
		}
		else
		{
		bitblast_parent = NULL;
		}

	if(direction)
		{
	        for(i=0;i<nodepnt;i++)
	                {
	                b->nodes[i]=n[i];
			if(n[i]->expansion)
				{
				if(bitblast_parent != n[i]->expansion->parent) 
					{
					bitblast_parent=NULL;
					}
					else
					{
					if(i==1)
						{
						bitblast_delta = n[1]->expansion->actual - n[0]->expansion->actual;
						if(bitblast_delta<-1) bitblast_delta=0;
						else if(bitblast_delta>1) bitblast_delta=0;
						}
					else if((bitblast_delta)&&(i>1))
						{
						if((n[i]->expansion->actual - n[i-1]->expansion->actual) != bitblast_delta) bitblast_delta=0;
						}
					}
				}
				else
				{
				bitblast_parent = NULL;
				}
	                }
		}
		else
		{
		int rev;
		rev=nodepnt-1;
	        for(i=0;i<nodepnt;i++)
	                {
	                b->nodes[i]=n[rev--];
			if(n[i]->expansion)
				{
				if(bitblast_parent != n[i]->expansion->parent) 
					{
					bitblast_parent=NULL;
					}
					else
					{
					if(i==1)
						{
						bitblast_delta = n[1]->expansion->actual - n[0]->expansion->actual;
						if(bitblast_delta<-1) bitblast_delta=0;
						else if(bitblast_delta>1) bitblast_delta=0;
						}
					else if((bitblast_delta)&&(i>1))
						{
						if((n[i]->expansion->actual - n[i-1]->expansion->actual) != bitblast_delta) bitblast_delta=0;
						}
					}
				}
				else
				{
				bitblast_parent = NULL;
				}
	                }
		}

        b->nbits=nodepnt;

	if(!bitblast_parent)
		{
		strcpy(b->name=(char *)malloc_2(strlen("<Vector>")+1),"<Vector>");
		}
		else
		{
		int i, offset;
		char *nam;

	        offset = strlen(n[0]->nname);
	        for(i=offset-1;i>=0;i--)
	                {
	                if(n[0]->nname[i]=='[') break;
	                }
	        if(i>-1) offset=i;
	
	        nam=(char *)wave_alloca(offset+40);
	        memcpy(nam, n[0]->nname, offset);
		if(direction)
			{
                	sprintf(nam+offset, "[%d%s%d]", n[0]->expansion->actual, (bitblast_delta!=0) ? ":" : "|", n[nodepnt-1]->expansion->actual);
			}
			else
			{
                	sprintf(nam+offset, "[%d%s%d]", n[nodepnt-1]->expansion->actual,  (bitblast_delta!=0) ? ":" : "|", n[0]->expansion->actual);
			}
	
		strcpy(b->name=(char *)malloc_2(offset + strlen(nam+offset)+1), nam);
		DEBUG(printf("Name is: '%s'\n", nam));
		}

	if((v=bits2vector(b)))
        	{
                v->bits=b;      /* only needed for savefile function */
                AddVector(v);
                free_2(b->name);
                b->name=NULL;
                }
                else
                {
                free_2(b->name);
                free_2(b);
                }

	tmp=traces.buffer; traces.buffer=traces.first; traces.first=tmp;
	tmp=traces.bufferlast; traces.bufferlast=traces.last; traces.last=tmp;
	tmpi=traces.buffercount; traces.buffercount=traces.total;
				traces.total=tmpi;
	PasteBuffer();
	CutBuffer();
	
	signalwindow_width_dirty=1;
	MaxSignalLength();
	signalarea_configure_event(signalarea, NULL);
	wavearea_configure_event(wavearea, NULL);
	}
}

static void
menu_combine_down(GtkWidget *widget, gpointer data)
{
if(helpbox_is_active)
        {
        help_text_bold("\n\nCombine Down");
        help_text(
                " coalesces the highlighted signals into a single vector named"
		" \"<Vector>\" in a top to bottom fashion"
                " placed after the last highlighted trace.  The original traces will"
                " be placed in the cut buffer."
		" It will function seemingly randomly"
		" when used upon real valued single-bit traces."
        );
        return;
        }

if(dnd_state) { dnd_error(); return; } /* don't mess with sigs when dnd active */
menu_combine(1); /* down */
}

static void
menu_combine_up(GtkWidget *widget, gpointer data)
{
if(helpbox_is_active)
        {
        help_text_bold("\n\nCombine Up");
        help_text(
                " coalesces the highlighted signals into a single vector named"
                " \"<Vector>\" in a bottom to top fashion"
                " placed after the last highlighted trace.  The original traces will"
                " be placed in the cut buffer."
		" It will function seemingly randomly"
		" when used upon real valued single-bit traces."
        );
        return;
        }

if(dnd_state) { dnd_error(); return; } /* don't mess with sigs when dnd active */
menu_combine(0); /* up */
}

/**/

static void
menu_reduce_singlebit_vex(GtkWidget *widget, gpointer data)
{
Trptr t, tmp;
int tmpi,dirty=0;

if(helpbox_is_active)
        {
        help_text_bold("\n\nReduce Single Bit Vectors");
        help_text(
		" decomposes the highlighted traces into their individual"
		" bits only if the highlighted traces are one bit wide vectors."
		" In effect, this function allows single-bit vectors"
		" to be viewed as signals."
		" The resulting bits are converted to traces and inserted after the"
		" last converted trace with the pre-conversion traces"
		" being placed in the cut buffer."
        );
        return;
        }


if(dnd_state) { dnd_error(); return; } /* don't mess with sigs when dnd active */

DEBUG(printf("Reduce Singlebit Vex\n"));

t=traces.first;
while(t)
	{
	if((t->flags&TR_HIGHLIGHT)&&(!(t->flags&TR_BLANK)))
		{
		dirty=1;
		break;
		}
	t=t->next;
	}

if(dirty)
	{
	FreeCutBuffer();
	traces.buffer=traces.first;
	traces.bufferlast=traces.last;
	traces.buffercount=traces.total;

	traces.first=traces.last=NULL; traces.total=0;

	t=traces.buffer;

	while(t)
		{
		if(t->flags&TR_HIGHLIGHT)
			{
                        if(t->flags&TR_BLANK)
                                {
                                AddBlankTrace(t->name);
                                }
				else
				{
				if(t->vector)
					{
					bptr bits;
					bits=t->n.vec->bits;
					if(bits->nbits==1)
						{
						AddNode(bits->nodes[0],NULL);
						}
						else
						{
						/* reset the cut criteria */
						t->flags&=(~TR_HIGHLIGHT);
						}
					}
					else
					{
					AddNode(t->n.nd,NULL);
					}
				}
			}
		t=t->next;
		}

	tmp=traces.buffer; traces.buffer=traces.first; traces.first=tmp;
	tmp=traces.bufferlast; traces.bufferlast=traces.last; traces.last=tmp;
	tmpi=traces.buffercount; traces.buffercount=traces.total;
				traces.total=tmpi;
	PasteBuffer();
	CutBuffer();
	
	signalwindow_width_dirty=1;
	MaxSignalLength();
	signalarea_configure_event(signalarea, NULL);
	wavearea_configure_event(wavearea, NULL);
	}
	else
	{
	must_sel_nb();
	}
}

/**/
static void menu_tracesearchbox_callback(GtkWidget *widget, gpointer data)
{
}

static void menu_tracesearchbox(GtkWidget *widget, gpointer data)
{
Trptr t;

if(helpbox_is_active)
        {  
        help_text_bold("\n\nPattern Search");
        help_text(
		" only works when at least one trace is highlighted. "
		" A requester will appear that lists all the selected"
		" traces (maximum of 500) and allows various criteria"
		" to be specified for each trace.  Searches can go forward"
		" or backward from the primary (unnamed) marker.  If the"
		" primary marker has not been set, the search starts at the"
		" beginning of the displayed data (\"From\") for a forwards"
		" search and starts at the end of the displayed data (\"To\")"
		" for a backwards search."
		" \"Mark\" and \"Clear\" are used to modify the normal time"
		" vertical markings such that they can be used to indicate"
		" all the times that a specific pattern search condition is"
		" true (e.g., every upclock of a specific signal)."
        );
        return;
        }

for(t=traces.first;t;t=t->next)
	{
	if ((t->flags&TR_BLANK)||(!(t->flags&TR_HIGHLIGHT))||(!(t->name))) 
		{
		continue;
		}
		else	/* at least one good trace, so do it */
		{	
		tracesearchbox("Waveform Display Search", menu_tracesearchbox_callback);
		return;
		}
	}

must_sel();
}

/**/
#ifndef _MSC_VER

static void
menu_new_viewer_cleanup(GtkWidget *widget, gpointer data)
{
pid_t pid;

if(filesel_ok)
	{
	/*
	 * for some reason, X won't let us double-fork in order to cleanup zombies.. *shrug*
         */
	pid=fork();
	if(((int)pid) < 0) { return; /* not much we can do about this.. */ }
	
	if(pid)         /* parent==original server_pid */
	        {
		return;
       		}

	execlp(whoami, whoami, *fileselbox_text, NULL);
	exit(0);	/* control never gets here if successful */
	}
}

static void
menu_new_viewer(GtkWidget *widget, gpointer data)
{
if(helpbox_is_active)
	{
	help_text_bold("\n\nOpen New Viewer");
	help_text(
		" will open a file requester that will ask for the name"
		" of a VCD or AET file to view.  This will fork off a"
		" new viewer process."
	);
	return;
	}

fileselbox("Select a trace to view...",&filesel_newviewer,menu_new_viewer_cleanup);
}
#endif

/**/

static void
menu_print(GtkWidget *widget, gpointer data)
{
if(helpbox_is_active)
	{
	help_text_bold("\n\nPrint To File");
	help_text(
		" will open up a requester that will allow you to select"
		" print options (PS or MIF; Letter, A4, or Legal; Full or Minimal)."
		" After selecting the options you want,"
		" a file requester will ask for the name of the"
		" output file to generate"
		" that reflects the current main window display's contents. "
	);
	return;
	}

renderbox("Print Formatting Options");
}

/**/
static void menu_markerbox_callback(GtkWidget *widget, gpointer data)
{
}

static void menu_markerbox(GtkWidget *widget, gpointer data)
{
if(helpbox_is_active)
        {
        help_text_bold("\n\nShow-Change Marker Data");
        help_text(
                " displays and allows the modification of the times for"
		" all 26 named markers.  The time for each marker must"
		" be unique."
        );
        return;
        }

markerbox("Markers", menu_markerbox_callback);
}

/**/
static void delete_unnamed_marker(GtkWidget *widget, gpointer data)
{
if(helpbox_is_active)
        {
        help_text_bold("\n\nDelete Primary Marker");
        help_text(
                " removes the primary marker from the display if present."
        );
        return;
        }        

DEBUG(printf("delete_unnamed marker()\n"));

if(tims.marker!=-1)
	{
	Trptr t;

	for(t=traces.first;t;t=t->next)
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
	signalarea_configure_event(signalarea, NULL);
	wavearea_configure_event(wavearea, NULL);
	}
}

/**/
static void collect_all_named_markers(GtkWidget *widget, gpointer data)
{
int i;
int dirty=0;

if(helpbox_is_active)
        {
        help_text_bold("\n\nCollect All Named Markers");
        help_text(
		" simply collects any and all named markers which have"
		" been dropped."
        );
        return;
        }

DEBUG(printf("collect_all_unnamed_markers()\n"));

for(i=0;i<26;i++)
	{
	if(named_markers[i]!=-1)
		{
		named_markers[i]=-1;
		dirty=1;
		}
	}

if(dirty)
	{
	signalarea_configure_event(signalarea, NULL);
	wavearea_configure_event(wavearea, NULL);
	}
}
/**/
static void collect_named_marker(GtkWidget *widget, gpointer data)
{
int i;

if(helpbox_is_active)
        {
        help_text_bold("\n\nCollect Named Marker");
        help_text(
                " collects a named marker where the current primary (unnamed)"
                " marker is placed if there is a named marker at its position."
        );
        return;
        }

DEBUG(printf("collect_named_marker()\n"));

if(tims.marker!=-1)
	{
	for(i=0;i<26;i++)
		{
		if(named_markers[i]==tims.marker)
			{
			named_markers[i]=-1;
			signalarea_configure_event(signalarea, NULL);
			wavearea_configure_event(wavearea, NULL);
			return;
			}
		}
	}
}
/**/
static void drop_named_marker(GtkWidget *widget, gpointer data)
{
int i;

if(helpbox_is_active)
        {
        help_text_bold("\n\nDrop Named Marker");
        help_text(
		" drops a named marker where the current primary (unnamed)"
		" marker is placed.  A maximum of 26 named markers are allowed"
		" and the times for all must be different."
        );
        return;
        }


DEBUG(printf("drop_named_marker()\n"));

if(tims.marker!=-1)
	{
	for(i=0;i<26;i++)
		{
		if(named_markers[i]==tims.marker) return; /* only one per slot */
		}

	for(i=0;i<26;i++)
		{
		if(named_markers[i]==-1)
			{
			named_markers[i]=tims.marker;
			signalarea_configure_event(signalarea, NULL);
			wavearea_configure_event(wavearea, NULL);
			return;
			}
		}
	}
}
/**/
static void menu_treesearch_cleanup(GtkWidget *widget, gpointer data)
{
MaxSignalLength();
signalarea_configure_event(signalarea, NULL);
wavearea_configure_event(wavearea, NULL);
DEBUG(printf("menu_treesearch_cleanup()\n"));
}

static void menu_treesearch(GtkWidget *widget, gpointer data)
{
if(helpbox_is_active)
        {
        help_text_bold("\n\nSignal Search Tree");
        help_text(
                " provides an easy means of adding traces to the display."
                " Various functions are provided in the Signal Search Tree requester"
                " which allow searching a treelike hierarchy and bundling"
                " (coalescing individual bits into a single vector)."
        );
        return;
        }

if(dnd_state) { dnd_error(); return; } /* don't mess with sigs when dnd active */

treebox("Signal Search Tree",menu_treesearch_cleanup);
}
/**/
static void 
menu_showchangeall_cleanup(GtkWidget *widget, gpointer data)
{
Trptr t;
Ulong flags;

t=showchangeall;
if(t)
	{
	flags=t->flags;
	while(t)
		{
		if((t->flags&TR_HIGHLIGHT)&&(!(t->flags&TR_BLANK))&&(t->name))
			{
			t->flags=flags;
			}	
		t=t->next;
		}
	}

signalwindow_width_dirty=1;
MaxSignalLength();
signalarea_configure_event(signalarea, NULL);
wavearea_configure_event(wavearea, NULL);
DEBUG(printf("menu_showchangeall_cleanup()\n"));
}

static void 
menu_showchangeall(GtkWidget *widget, gpointer data)
{
Trptr t;

if(helpbox_is_active)
        {
        help_text_bold("\n\nShow-Change All Highlighted");   
        help_text(
                " provides an easy means of changing trace attributes en masse."
                " Various functions are provided in a Show-Change requester."
        );
        return;
        }

DEBUG(printf("menu_showchangeall()\n"));

showchangeall=NULL;
t=traces.first;
while(t)
	{
	if((t->flags&TR_HIGHLIGHT)&&(!(t->flags&TR_BLANK))&&(t->name))
		{
		showchange("Show-Change All", showchangeall=t, menu_showchangeall_cleanup);
		return;
		}
	t=t->next;
	}

must_sel();
}

/**/
static void 
menu_showchange_cleanup(GtkWidget *widget, gpointer data)
{
signalwindow_width_dirty=1;
MaxSignalLength();
signalarea_configure_event(signalarea, NULL);
wavearea_configure_event(wavearea, NULL);
DEBUG(printf("menu_showchange_cleanup()\n"));
}

static void 
menu_showchange(GtkWidget *widget, gpointer data)
{
Trptr t;

if(helpbox_is_active)
        {
        help_text_bold("\n\nShow-Change First Highlighted");
        help_text(
                " provides a means of changing trace attributes for the"
		" first highlighted trace. "
                " Various functions are provided in a Show-Change requester. "
  		" When a function is applied, the trace will be unhighlighted."
        );
        return;
        }

DEBUG(printf("menu_showchange()\n"));

t=traces.first;
while(t)
	{
	if((t->flags&TR_HIGHLIGHT)&&(!(t->flags&TR_BLANK))&&(t->name))
		{
		showchange("Show-Change", t, menu_showchange_cleanup);
		return;
		}
	t=t->next;
	}

must_sel();
}
/**/
static void menu_remove_aliases(GtkWidget *widget, gpointer data)
{
Trptr t;
int dirty=0;

if(helpbox_is_active)
        {
        help_text_bold("\n\nRemove Highlighted Aliases");
        help_text(
                " only works when at least one trace has been highlighted. "
                " Any aliased traces will have their names restored to their"
		" original names.  As vectors get their names from aliases,"
		" vector aliases will not be removed."
        );
        return;
        }

if(dnd_state) { dnd_error(); return; } /* don't mess with sigs when dnd active */

t=traces.first;
while(t)
	{
	if((!t->vector)&&(t->is_alias))
		{
		if(t->name) free_2(t->name);
		t->is_alias=0;
		if(!(t->flags&TR_BLANK)) t->name=t->n.nd->nname; else t->name=NULL;
		dirty=1;
		}
	t=t->next;
	}

if(dirty)
	{
	signalwindow_width_dirty=1;
	MaxSignalLength();
	signalarea_configure_event(signalarea, NULL);
	wavearea_configure_event(wavearea, NULL);
	DEBUG(printf("menu_remove_aliases()\n"));
	}
	else
	{
	must_sel();
	}
}
/**/
static void alias_cleanup(GtkWidget *widget, gpointer data)
{
Trptr t;

t=trace_to_alias;

if(entrybox_text)
	{
	char *efix;

	if(t->is_alias) free_2(t->name);
	t->is_alias=1;

	if(!(t->flags&TR_BLANK))
		{
		efix=entrybox_text;
		while(*efix)
			{
			if(*efix==' ')
				{
				*efix='_';
				}
			efix++;
			}
		}

	if((!t->vector)&&(!(t->flags&TR_BLANK)))
		{
		t->name=(char *)malloc_2(3+strlen(entrybox_text));
		strcpy(t->name, "+ ");
		strcpy(t->name+2, entrybox_text);
		}
		else
		{
		t->name=(char *)malloc_2(1+strlen(entrybox_text));
		strcpy(t->name, entrybox_text);
		}

	t->flags&=(~TR_HIGHLIGHT);

	signalwindow_width_dirty=1;
	MaxSignalLength();
	signalarea_configure_event(signalarea, NULL);
	wavearea_configure_event(wavearea, NULL);
	DEBUG(printf("alias_cleanup()\n"));
	}
}

static void menu_alias(GtkWidget *widget, gpointer data)
{
Trptr t;
t=traces.first;
trace_to_alias=NULL;

if(helpbox_is_active)
        {
        help_text_bold("\n\nAlias Highlighted Trace");
        help_text(
                " only works when at least one trace has been highlighted. "
                " With this function, you will be prompted for an alias"
                " name for the first highlighted trace.  After successfully"
		" aliasing a trace, the aliased trace will be unhighlighted."
		" Single bits will be marked with a leading \"+\" and vectors"
		" will have no such designation.  The purpose of this is to"
		" provide a fast method of determining which trace names are"
		" real and which ones are aliases."
        );
        return;
        }

if(dnd_state) { dnd_error(); return; } /* don't mess with sigs when dnd active */

while(t)
	{
	if(t->flags&TR_HIGHLIGHT)
		{
		trace_to_alias=t;
		break;
		}
	t=t->next;
	}

if(trace_to_alias)
	{
	entrybox("Alias Highlighted Trace",300,"",128,alias_cleanup);
	}
	else
	{
	must_sel();
	}
}
/**/
static void menu_hiersearch_cleanup(GtkWidget *widget, gpointer data)
{
MaxSignalLength();
signalarea_configure_event(signalarea, NULL);
wavearea_configure_event(wavearea, NULL);
DEBUG(printf("menu_hiersearch_cleanup()\n"));
}

static void menu_hiersearch(GtkWidget *widget, gpointer data)
{
if(helpbox_is_active)
        {
        help_text_bold("\n\nHierarchy Search");
        help_text(
		" provides an easy means of adding traces to the display in a text based"
		" treelike fashion."
        );
        return;
        }

if(dnd_state) { dnd_error(); return; } /* don't mess with sigs when dnd active */

hier_searchbox("Hierarchy Search",menu_hiersearch_cleanup);
}
/**/
static void menu_signalsearch_cleanup(GtkWidget *widget, gpointer data)
{
MaxSignalLength();
signalarea_configure_event(signalarea, NULL);
wavearea_configure_event(wavearea, NULL);
DEBUG(printf("menu_signalsearch_cleanup()\n"));
}

static void menu_signalsearch(GtkWidget *widget, gpointer data)
{
if(helpbox_is_active)
        {
        help_text_bold("\n\nSignal Search Regexp");
        help_text(
		" provides an easy means of adding traces to the display. "
		" Various functions are provided in the Signal Search requester"
		" which allow searching using POSIX regular expressions and bundling"
		" (coalescing individual bits into a single vector). "
        );
        return;
        }

if(dnd_state) { dnd_error(); return; } /* don't mess with sigs when dnd active */

searchbox("Signal Search",menu_signalsearch_cleanup);
}
/**/
static void 
regexp_highlight_generic(int mode)
{
if(entrybox_text)
	{
	Trptr t;
	Ulong modebits;
	char dirty=0;

	modebits=(mode)?TR_HIGHLIGHT:0;

	strcpy(regexp_string, entrybox_text);
	wave_regex_compile(regexp_string);
	free_2(entrybox_text);
	t=traces.first;
	while(t)
		{
		char *pnt;

		pnt=(t->name)?t->name:""; /* handle (really) blank lines */

		if(*pnt=='+')		  /* skip alias prefix if present */
			{
			pnt++;
			if(*pnt==' ')
				{
				pnt++;
				}
			}

		if(wave_regex_match(pnt))
			{
			t->flags=((t->flags&(~TR_HIGHLIGHT))|modebits);
			dirty=1;
			}

		t=t->next;
		}

	if(dirty)
		{
		signalarea_configure_event(signalarea, NULL);
		wavearea_configure_event(wavearea, NULL);
		}
	}
}

static void 
regexp_unhighlight_cleanup(GtkWidget *widget, gpointer data)
{
regexp_highlight_generic(0);
}

static void 
menu_regexp_unhighlight(GtkWidget *widget, gpointer data)
{
if(helpbox_is_active)
        {
        help_text_bold("\n\nUnHighlight Regexp");
        help_text(
                " brings up a text requester that will ask for a"
                " regular expression that may contain text with POSIX regular expressions."
                " All traces meeting this criteria will be"
                " unhighlighted if they are currently highlighted."
        );
        return;
        }

entrybox("Regexp UnHighlight",300,regexp_string,128,regexp_unhighlight_cleanup);
}
/**/
static void 
regexp_highlight_cleanup(GtkWidget *widget, gpointer data)
{
regexp_highlight_generic(1);
}

static void 
menu_regexp_highlight(GtkWidget *widget, gpointer data)
{
if(helpbox_is_active)
        {
        help_text_bold("\n\nHighlight Regexp");
        help_text(
                " brings up a text requester that will ask for a"
                " regular expression that may contain text with POSIX regular expressions."
		" All traces meeting this criteria will be"
		" highlighted."
        );
        return;
        }

entrybox("Regexp Highlight",300,regexp_string,128,regexp_highlight_cleanup);
}
/**/
static void
menu_write_save_cleanup(GtkWidget *widget, gpointer data)
{
FILE *wave;
struct strace *st;

if(filesel_ok)
if(!(wave=fopen(*fileselbox_text,"wb")))
        {
        fprintf(stderr, "Error opening save file '%s' for writing.\n",*fileselbox_text);
	perror("Why");
	errno=0;
        }
	else
	{
	Trptr t;
	int i;
	unsigned int def=0;
	TimeType prevshift=LLDescriptor(0);

	DEBUG(printf("Write Save Fini: %s\n", *fileselbox_text));

	fprintf(wave,"*%f "TTFormat, (float)(tims.zoom),tims.marker);

	for(i=0;i<26;i++)
		{
		fprintf(wave," "TTFormat,named_markers[i]);
		}
	fprintf(wave,"\n");

	t=traces.first;
	while(t)
		{
		if(t->flags!=def)
			{
			fprintf(wave,"@%x\n",def=t->flags);
			}

		if((t->shift)||((prevshift)&&(!t->shift)))
			{
			fprintf(wave,">"TTFormat"\n", t->shift);
			}
		prevshift=t->shift;

		if(!(t->flags&TR_BLANK))	
			{
			if(t->vector)
				{
				int i;
				nptr *nodes;

				fprintf(wave,"#%s",t->name);

				nodes=t->n.vec->bits->nodes;
				for(i=0;i<t->n.vec->nbits;i++)
					{
					if(nodes[i]->expansion)
						{
						fprintf(wave," (%d)%s",nodes[i]->expansion->parentbit, nodes[i]->expansion->parent->nname);
						}
						else
						{
						fprintf(wave," %s",nodes[i]->nname);
						}
					}
				fprintf(wave,"\n");
				}
				else
				{
				if(t->is_alias)
					{
					if(t->n.nd->expansion)
						{
						fprintf(wave,"+%s (%d)%s\n",t->name+2,t->n.nd->expansion->parentbit, t->n.nd->expansion->parent->nname);
						}
						else
						{
						fprintf(wave,"+%s %s\n",t->name+2,t->n.nd->nname);
						}
					}
					else
					{
					if(t->n.nd->expansion)
						{
						fprintf(wave,"(%d)%s\n",t->n.nd->expansion->parentbit, t->n.nd->expansion->parent->nname);
						}
						else
						{
						fprintf(wave,"%s\n",t->n.nd->nname);
						}
					}
				}
			}
			else
			{
			if(!t->name) fprintf(wave,"-\n");
			else fprintf(wave,"-%s\n",t->name);
			}
		t=t->next;
		}

	if(timearray)
		{
		if(shadow_straces)
			{
			swap_strace_contexts();

			st=straces;
			if(straces)
				{
				fprintf(wave, "!%d%d%d%d%d%d\n", logical_mutex[0], logical_mutex[1], logical_mutex[2], logical_mutex[3], logical_mutex[4], logical_mutex[5]);
				}

			while(st)
				{
				if(st->value==ST_STRING)
					{
					fprintf(wave, "?\"%s\n", st->string ? st->string : ""); /* search type for this trace is string.. */
					}
					else
					{
					fprintf(wave, "?%02x\n", (unsigned char)st->value);	/* else search type for this trace.. */
					}
			
				t=st->trace;

				if(t->flags!=def)
					{
					fprintf(wave,"@%x\n",def=t->flags);
					}

				if((t->shift)||((prevshift)&&(!t->shift)))
					{
					fprintf(wave,">"TTFormat"\n", t->shift);
					}
				prevshift=t->shift;

				if(!(t->flags&TR_BLANK))	
					{
					if(t->vector)
						{
						int i;
						nptr *nodes;

						fprintf(wave,"#%s",t->name);

						nodes=t->n.vec->bits->nodes;
						for(i=0;i<t->n.vec->nbits;i++)
							{
							if(nodes[i]->expansion)
								{
								fprintf(wave," (%d)%s",nodes[i]->expansion->parentbit, nodes[i]->expansion->parent->nname);
								}
								else
								{
								fprintf(wave," %s",nodes[i]->nname);
								}
							}
						fprintf(wave,"\n");
						}
						else
						{
						if(t->is_alias)
							{
							if(t->n.nd->expansion)
								{
								fprintf(wave,"+%s (%d)%s\n",t->name+2,t->n.nd->expansion->parentbit, t->n.nd->expansion->parent->nname);
								}
								else
								{
								fprintf(wave,"+%s %s\n",t->name+2,t->n.nd->nname);
								}
							}
							else
							{
							if(t->n.nd->expansion)
								{
								fprintf(wave,"(%d)%s\n",t->n.nd->expansion->parentbit, t->n.nd->expansion->parent->nname);
								}
								else
								{
								fprintf(wave,"%s\n",t->n.nd->nname);
								}
							}
						}
					}

				st=st->next;
				} /* while(st)... */

			if(straces)
				{
				fprintf(wave, "!!\n");	/* mark end of strace region */
				}
		
				swap_strace_contexts();
			}
			else
			{
			struct mprintf_buff_t *mt = mprintf_buff_head;

			while(mt)	
				{
				fprintf(wave, "%s", mt->str);
				mt=mt->next;
				}
			}

		} /* if(timearray)... */

	fclose(wave);
	}

}

static void
menu_write_save_file(GtkWidget *widget, gpointer data)
{
if(helpbox_is_active)
	{
	help_text_bold("\n\nWrite Save File");
	help_text(
		" will open a file requester that will ask for the name"
		" of a GTKWave save file.  The contents of the save file"
		" generated will be the traces as well as their"
 		" format (binary, decimal, hex, reverse, etc.) which"
		" are currently a part of the display.  Marker positional"
		" data and the zoom factor are also a part of the save file."
	);
	return;
	}

fileselbox("Write Save File",&filesel_writesave,menu_write_save_cleanup);
}
/**/
static void
menu_read_save_cleanup(GtkWidget *widget, gpointer data)
{
FILE *wave;

if(filesel_ok)
	{
	char *wname;
        int wave_is_compressed;
	DEBUG(printf("Read Save Fini: %s\n", *fileselbox_text));
        
        wname=*fileselbox_text;
        
        if(((strlen(wname)>2)&&(!strcmp(wname+strlen(wname)-3,".gz")))||
          ((strlen(wname)>3)&&(!strcmp(wname+strlen(wname)-4,".zip"))))
                {
                char *str;
                str=wave_alloca(strlen(wname)+5+1);
                strcpy(str,"zcat ");
                strcpy(str+5,wname);
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
                fprintf(stderr, "Error opening save file '%s' for reading.\n",*fileselbox_text);
		perror("Why");
		errno=0;
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

                default_flags=TR_RJUSTIFY;
		shift_timebase_default_for_add=LLDescriptor(0);
		update_markertime(time_trunc(tims.marker));
                if(wave_is_compressed) pclose(wave); else fclose(wave);

		MaxSignalLength();
		signalarea_configure_event(signalarea, NULL);
		wavearea_configure_event(wavearea, NULL);
                }
	}
}

static void
menu_read_save_file(GtkWidget *widget, gpointer data)
{
if(helpbox_is_active)
	{
	help_text_bold("\n\nRead Save File");
	help_text(
		" will open a file requester that will ask for the name"
		" of a GTKWave save file.  The contents of the save file"
		" will determine which traces and vectors as well as their"
 		" format (binary, decimal, hex, reverse, etc.) are to be"
		" appended to the display.  Note that the marker positional"
		" data and zoom factor present in the save file will"
		" replace any current settings."
	);
	return;
	}

if(dnd_state) { dnd_error(); return; } /* don't mess with sigs when dnd active */

fileselbox("Read Save File",&filesel_readsave,menu_read_save_cleanup);
}
/**/
static void
menu_insert_blank_traces(GtkWidget *widget, gpointer data)
{
if(helpbox_is_active)
        {  
        help_text_bold("\n\nInsert Blank");
        help_text(
                " inserts a blank trace after the last highlighted trace."
                " If no traces are highlighted, the blank is inserted after"
		" the last trace."
        );
        return;
        }

if(dnd_state) { dnd_error(); return; } /* don't mess with sigs when dnd active */

DEBUG(printf("Insert Blank Trace\n"));

InsertBlankTrace(NULL);
signalarea_configure_event(signalarea, NULL);
wavearea_configure_event(wavearea, NULL);
}
/**/
static void
comment_trace_cleanup(GtkWidget *widget, gpointer data)
{
InsertBlankTrace(entrybox_text);
if(entrybox_text) { free_2(entrybox_text); entrybox_text=NULL; }
signalwindow_width_dirty=1;
MaxSignalLength();
signalarea_configure_event(signalarea, NULL);
wavearea_configure_event(wavearea, NULL);
}

static void
menu_insert_comment_traces(GtkWidget *widget, gpointer data)
{
if(helpbox_is_active)
        {  
        help_text_bold("\n\nInsert Comment");
        help_text(
                " inserts a comment trace after the last highlighted trace."
                " If no traces are highlighted, the comment is inserted after"
		" the last trace."
        );
        return;
        }

if(dnd_state) { dnd_error(); return; } /* don't mess with sigs when dnd active */

DEBUG(printf("Insert Comment Trace\n"));

entrybox("Insert Comment Trace",300,"",128,comment_trace_cleanup);
}
/**/
static void movetotime_cleanup(GtkWidget *widget, gpointer data)
{
if(entrybox_text)
	{
	TimeType gt;
	char update_string[128];
	char timval[40];
	GtkAdjustment *hadj;
	TimeType pageinc;

	gt=unformat_time(entrybox_text, time_dimension);
	free_2(entrybox_text);
	entrybox_text=NULL;

	if(gt<tims.first) gt=tims.first;
	else if(gt>tims.last) gt=tims.last;

	hadj=GTK_ADJUSTMENT(wave_hslider);
	hadj->value=gt;

	pageinc=(TimeType)(((gdouble)wavewidth)*nspx);
	if(gt<(tims.last-pageinc+1))
		tims.timecache=gt;
	        else
	        {
	        tims.timecache=tims.last-pageinc+1;
        	if(tims.timecache<tims.first) tims.timecache=tims.first;
        	}

	reformat_time(timval,tims.timecache,time_dimension);
	sprintf(update_string, "Moved to time: %s\n", timval);
	status_text(update_string);

	time_update();
	}
}

static void menu_movetotime(GtkWidget *widget, gpointer data)
{
char gt[32];

if(helpbox_is_active)
        {
        help_text_bold("\n\nMove To Time");
        help_text(
                " scrolls the waveform display such that the left border"
                " is the time entered in the requester."
        );
        return;
        }

reformat_time(gt, tims.start, time_dimension);

entrybox("Move To Time",200,gt,20,movetotime_cleanup);
}
/**/
static void fetchsize_cleanup(GtkWidget *widget, gpointer data)
{
if(entrybox_text)
	{
	TimeType fw;
	char update_string[128];
	fw=unformat_time(entrybox_text, time_dimension);
	if(fw<1)
		{
		fw=fetchwindow; /* in case they try to pull 0 or <0 */
		}
		else
		{
		fetchwindow=fw;
		}
	free_2(entrybox_text);
	entrybox_text=NULL;
	sprintf(update_string, "Fetch Size is now: "TTFormat"\n", fw);
	status_text(update_string);
	}
}

static void menu_fetchsize(GtkWidget *widget, gpointer data)
{
char fw[32];

if(helpbox_is_active)   
        {
        help_text_bold("\n\nFetch Size");
        help_text(
                " brings up a requester which allows input of the"
                " number of ticks used for fetch/discard operations."
		"  Default is 100."
        );
        return;
        }

reformat_time(fw, fetchwindow, time_dimension);

entrybox("New Fetch Size",200,fw,20,fetchsize_cleanup);
}
/**/
static void zoomsize_cleanup(GtkWidget *widget, gpointer data)
{
if(entrybox_text)
	{
	float f;
	char update_string[128];

	sscanf(entrybox_text, "%f", &f);
	if(f>0.0)
		{
		f=0.0; /* in case they try to go out of range */
		}
	else
	if(f<-62.0)
		{
		f=-62.0; /* in case they try to go out of range */
		}

	tims.prevzoom=tims.zoom;
	tims.zoom=(gdouble)f;
	calczoom(tims.zoom);
	fix_wavehadj();

	gtk_signal_emit_by_name (GTK_OBJECT (GTK_ADJUSTMENT(wave_hslider)), "changed");
	gtk_signal_emit_by_name (GTK_OBJECT (GTK_ADJUSTMENT(wave_hslider)), "value_changed");

	free_2(entrybox_text);
	entrybox_text=NULL;
	sprintf(update_string, "Zoom Amount is now: %g\n", f);
	status_text(update_string);
	}
}

static void menu_zoomsize(GtkWidget *widget, gpointer data)
{
char za[32];

if(helpbox_is_active)
        {
        help_text_bold("\n\nZoom Amount");
        help_text(
                " allows entry of zero or a negative value for the display"
		" zoom.  Zero is no magnification."
        );
        return;
        }


sprintf(za,"%g",(float)(tims.zoom));

entrybox("New Zoom Amount",200,za,20,zoomsize_cleanup);
}
/**/
static void zoombase_cleanup(GtkWidget *widget, gpointer data)
{
if(entrybox_text)
	{
	float za;
	char update_string[128];
	sscanf(entrybox_text, "%f", &za);
	if(za>10.0)
		{
		za=10.0;
		}
	else
	if(za<1.5)
		{
		za=1.5;
		}

	zoombase=(gdouble)za;
	calczoom(tims.zoom);
	fix_wavehadj();

	gtk_signal_emit_by_name (GTK_OBJECT (GTK_ADJUSTMENT(wave_hslider)), "changed");
	gtk_signal_emit_by_name (GTK_OBJECT (GTK_ADJUSTMENT(wave_hslider)), "value_changed");

	free_2(entrybox_text);
	entrybox_text=NULL;
	sprintf(update_string, "Zoom Base is now: %g\n", za);
	status_text(update_string);
	}
}

static void menu_zoombase(GtkWidget *widget, gpointer data)
{
char za[32];

if(helpbox_is_active)
        {
        help_text_bold("\n\nZoom Base");
        help_text(
                " allows entry of a zoom base for the zoom (magnification per integer step)"
		" Allowable values are 1.5 to 10.0.  Default is 2.0."
        );
        return;
        }


sprintf(za,"%g",zoombase);

entrybox("New Zoom Base Amount",200,za,20,zoombase_cleanup);
}
/**/
static void dataformat(int mask, int patch)
{
Trptr t;
int fix=0;

if((t=traces.first))
	{
	while(t)
		{
		if(t->flags&TR_HIGHLIGHT)
			{
			t->flags=((t->flags)&mask)|patch;
			fix=1;
			}
		t=t->next;
		}
	if(fix)
		{
		signalwindow_width_dirty=1;
		MaxSignalLength();
		signalarea_configure_event(signalarea, NULL);
		wavearea_configure_event(wavearea, NULL);
		}
	}
}

static void
menu_dataformat_ascii(GtkWidget *widget, gpointer data)
{
if(helpbox_is_active)
        {
        help_text_bold("\n\nData Format-ASCII");
        help_text(
		" will step through all highlighted traces and ensure that"
		" vectors with this qualifier will be displayed with ASCII"
		" values."
        );
        return;
        }

dataformat( ~(TR_HEX|TR_DEC|TR_BIN|TR_OCT|TR_SIGNED|TR_ASCII), TR_ASCII );
}

static void
menu_dataformat_hex(GtkWidget *widget, gpointer data)
{
if(helpbox_is_active)
        {
        help_text_bold("\n\nData Format-Hex");
        help_text(
		" will step through all highlighted traces and ensure that"
		" vectors with this qualifier will be displayed with hexadecimal"
		" values."
        );
        return;
        }

dataformat( ~(TR_HEX|TR_DEC|TR_BIN|TR_OCT|TR_SIGNED|TR_ASCII), TR_HEX );
}

static void
menu_dataformat_dec(GtkWidget *widget, gpointer data)
{
if(helpbox_is_active)
        {
        help_text_bold("\n\nData Format-Decimal");
        help_text(
		" will step through all highlighted traces and ensure that"
		" vectors with this qualifier will be displayed with decimal"
		" values."
        );
        return;
        }

dataformat( ~(TR_HEX|TR_DEC|TR_BIN|TR_OCT|TR_SIGNED|TR_ASCII), TR_DEC );
}

static void
menu_dataformat_signed(GtkWidget *widget, gpointer data)
{
if(helpbox_is_active)
        {
        help_text_bold("\n\nData Format-Signed");
        help_text(
		" will step through all highlighted traces and ensure that"
		" vectors with this qualifier will be displayed as sign extended decimal"
		" values."
        );
        return;
        }

dataformat( ~(TR_HEX|TR_DEC|TR_BIN|TR_OCT|TR_SIGNED|TR_ASCII), TR_SIGNED );
}

static void
menu_dataformat_bin(GtkWidget *widget, gpointer data)
{
if(helpbox_is_active)
        {
        help_text_bold("\n\nData Format-Binary");
        help_text(
		" will step through all highlighted traces and ensure that"
		" vectors with this qualifier will be displayed with binary"
		" values."
        );
        return;
        }

dataformat( ~(TR_HEX|TR_DEC|TR_BIN|TR_OCT|TR_SIGNED|TR_ASCII), TR_BIN );
}

static void
menu_dataformat_oct(GtkWidget *widget, gpointer data)
{
if(helpbox_is_active)
        {
        help_text_bold("\n\nData Format-Octal");
        help_text(
		" will step through all highlighted traces and ensure that"
		" vectors with this qualifier will be displayed with octal"
		" values."
        );
        return;
        }

dataformat( ~(TR_HEX|TR_DEC|TR_BIN|TR_OCT|TR_SIGNED|TR_ASCII), TR_OCT );
}

static void
menu_dataformat_rjustify_on(GtkWidget *widget, gpointer data)
{
if(helpbox_is_active)
        {
        help_text_bold("\n\nData Format-Right Justify-On");
        help_text(
		" will step through all highlighted traces and ensure that"
		" vectors with this qualifier will be displayed right"
		" justified."
        );
        return;
        }

dataformat( ~(TR_RJUSTIFY), TR_RJUSTIFY );
}

static void
menu_dataformat_rjustify_off(GtkWidget *widget, gpointer data)
{
if(helpbox_is_active)
        {
        help_text_bold("\n\nData Format-Right Justify-Off");
        help_text(
                " will step through all highlighted traces and ensure that"
                " vectors with this qualifier will not be displayed right"       
                " justified."
        );
        return;
        }  

dataformat( ~(TR_RJUSTIFY), 0 );
}

static void
menu_dataformat_invert_on(GtkWidget *widget, gpointer data)
{
if(helpbox_is_active)
        {
        help_text_bold("\n\nData Format-Invert-On");
        help_text(
                " will step through all highlighted traces and ensure that"
                " bits and vectors with this qualifier will be displayed with"       
                " 1's and 0's inverted."
        );
        return;
        }  

dataformat( ~(TR_INVERT), TR_INVERT );
}

static void
menu_dataformat_invert_off(GtkWidget *widget, gpointer data)
{
if(helpbox_is_active)
        {
        help_text_bold("\n\nData Format-Invert-Off");
        help_text(
                " will step through all highlighted traces and ensure that"
                " bits and vectors with this qualifier will not be displayed with" 
                " 1's and 0's inverted."                       
        );
        return;
        }  

dataformat( ~(TR_INVERT), 0 );
}

static void
menu_dataformat_reverse_on(GtkWidget *widget, gpointer data)
{
if(helpbox_is_active)
        {
        help_text_bold("\n\nData Format-Reverse Bits-On");
        help_text(
                " will step through all highlighted traces and ensure that"
                " vectors with this qualifier will be displayed in" 
                " reversed bit order."                       
        );
        return;
        }  

dataformat( ~(TR_REVERSE), TR_REVERSE );
}

static void
menu_dataformat_reverse_off(GtkWidget *widget, gpointer data)
{
if(helpbox_is_active)
        {
        help_text_bold("\n\nData Format-Reverse Bits-Off");
        help_text(
                " will step through all highlighted traces and ensure that"
                " vectors with this qualifier will not be displayed in"
                " reversed bit order."
        );
        return;   
        }

dataformat( ~(TR_REVERSE), 0 );
}

static void
menu_dataformat_exclude_on(GtkWidget *widget, gpointer data)
{
if(helpbox_is_active)
        {
        help_text_bold("\n\nExclude");
        help_text(
		" causes the waveform data for all currently highlighted traces"
		" to be blanked out."
        );
        return;
        }

dataformat( ~(TR_EXCLUDE), TR_EXCLUDE );
}

static void
menu_dataformat_exclude_off(GtkWidget *widget, gpointer data)
{
if(helpbox_is_active)
        {
        help_text_bold("\n\nShow");
        help_text(
                " causes the waveform data for all currently highlighted traces"
                " to be displayed as normal if the exclude attribute is currently"
		" set on the highlighted traces."            
        );
        return;
        }

dataformat( ~(TR_EXCLUDE), 0 );
}
/**/
static void menu_dataformat_highlight_all(GtkWidget *widget, gpointer data)
{
Trptr t;

if(helpbox_is_active)
        {
        help_text_bold("\n\nHighlight All");
        help_text(
		" simply highlights all displayed traces."
        );
        return;
        }

if(dnd_state) { dnd_error(); return; } /* don't mess with sigs when dnd active */

if((t=traces.first))
	{
	while(t)
		{
		t->flags|=TR_HIGHLIGHT;
		t=t->next;
		}
	signalarea_configure_event(signalarea, NULL);
	wavearea_configure_event(wavearea, NULL);
	}
}

static void menu_dataformat_unhighlight_all(GtkWidget *widget, gpointer data)
{
Trptr t;

if(helpbox_is_active)
        {
        help_text_bold("\n\nUnHighlight All");
        help_text(
                " simply unhighlights all displayed traces."
        );
        return;
        }

if(dnd_state) { dnd_error(); return; } /* don't mess with sigs when dnd active */

if((t=traces.first))
	{
	while(t)
		{
		t->flags&=(~TR_HIGHLIGHT);
		t=t->next;
		}
	signalarea_configure_event(signalarea, NULL);
	wavearea_configure_event(wavearea, NULL);
	}
}
/**/
static void menu_lexize(GtkWidget *widget, gpointer data)
{
Trptr t;

if(helpbox_is_active)
        {
        help_text_bold("\n\nSigsort All");
        help_text(
                " sorts all displayed traces with the numeric parts being taken into account.  Blank traces are sorted to the bottom."
        );
        return;
        }

if(dnd_state) { dnd_error(); return; } /* don't mess with sigs when dnd active */

if((t=traces.first))
	{
	if(TracesAlphabetize(2))
		{
		signalarea_configure_event(signalarea, NULL);
		wavearea_configure_event(wavearea, NULL);
		}
	}
}
/**/
static void menu_alphabetize(GtkWidget *widget, gpointer data)
{
Trptr t;

if(helpbox_is_active)
        {
        help_text_bold("\n\nAlphabetize All");
        help_text(
                " alphabetizes all displayed traces.  Blank traces are sorted to the bottom."
        );
        return;
        }

if(dnd_state) { dnd_error(); return; } /* don't mess with sigs when dnd active */

if((t=traces.first))
	{
	if(TracesAlphabetize(1))
		{
		signalarea_configure_event(signalarea, NULL);
		wavearea_configure_event(wavearea, NULL);
		}
	}
}
/**/
static void menu_alphabetize2(GtkWidget *widget, gpointer data)
{
Trptr t;

if(helpbox_is_active)
        {
        help_text_bold("\n\nAlphabetize All (CaseIns)");
        help_text(
                " alphabetizes all displayed traces without regard to case.  Blank traces are sorted to the bottom."
        );
        return;
        }

if(dnd_state) { dnd_error(); return; } /* don't mess with sigs when dnd active */

if((t=traces.first))
	{
	if(TracesAlphabetize(0))
		{
		signalarea_configure_event(signalarea, NULL);
		wavearea_configure_event(wavearea, NULL);
		}
	}
}
/**/
static void menu_reverse(GtkWidget *widget, gpointer data)
{
Trptr t;

if(helpbox_is_active)
        {
        help_text_bold("\n\nReverse All");
        help_text(
                " reverses all displayed traces unconditionally."
        );
        return;
        }

if(dnd_state) { dnd_error(); return; } /* don't mess with sigs when dnd active */

if((t=traces.first))
	{
	if(TracesReverse())
		{
		signalarea_configure_event(signalarea, NULL);
		wavearea_configure_event(wavearea, NULL);
		}
	}
}
/**/
static void
menu_cut_traces(GtkWidget *widget, gpointer data)
{
if(helpbox_is_active)
        {
        help_text_bold("\n\nCut");
        help_text(
                " removes highlighted signals from the display and places them" 
		" in an offscreen cut buffer for later Paste operations. "
		" Cut implicitly destroys the previous contents of the cut buffer."
        );
        return;
        }                

if(dnd_state) { dnd_error(); return; } /* don't mess with sigs when dnd active */

DEBUG(printf("Cut Traces\n"));

if(CutBuffer())
	{
	MaxSignalLength();
	signalarea_configure_event(signalarea, NULL);
	wavearea_configure_event(wavearea, NULL);
	}
	else
	{
	must_sel();
	}
}

static void
menu_paste_traces(GtkWidget *widget, gpointer data)
{
if(helpbox_is_active)
        {
        help_text_bold("\n\nPaste");
        help_text(
                " pastes signals from"       
                " an offscreen cut buffer and places them in a group after"
		" the last highlighted signal, or at the end of the display"
		" if no signal is highlighted."
                " Paste implicitly destroys the previous contents of the cut buffer."
        );
        return;
        }


if(dnd_state) { dnd_error(); return; } /* don't mess with sigs when dnd active */

DEBUG(printf("Paste Traces\n"));

if(PasteBuffer())
	{
	MaxSignalLength();
	signalarea_configure_event(signalarea, NULL);
	wavearea_configure_event(wavearea, NULL);
	}
}
/**/
static void menu_center_zooms(GtkWidget *widget, gpointer data)
{
if(helpbox_is_active)
        {
        help_text_bold("\n\nCenter Zooms");
        help_text(
		" when enabled"
		" configures zoom in/out operations such that all zooms use the center of the"
		" display as the fixed zoom origin if the primary (unnamed) marker is"
		" not present, otherwise, the primary marker is used as the center origin."
		" When disabled, it"
		" configures zoom in/out operations such that all zooms use the"
		" left margin of the display as the fixed zoom origin."
        );
        }
	else
	{
	do_zoom_center=(do_zoom_center)?0:1;
	DEBUG(printf("Center Zooms\n"));
	}

GTK_CHECK_MENU_ITEM(gtk_item_factory_get_widget(item_factory, 
	menu_items[WV_MENU_VCZ].path))->active=(do_zoom_center)?TRUE:FALSE;
}


static void menu_show_base(GtkWidget *widget, gpointer data)
{
if(helpbox_is_active)
        {
        help_text_bold("\n\nShow Base Symbols");
        help_text(
		" enables the display of leading base symbols ('$' for hex,"
		" '%' for binary, '#' for octal if they are turned off and"
		" disables the drawing of leading base symbols if"
		" they are turned on."
		" Base symbols are displayed by default."
        );
        }
	else
	{
	show_base=(show_base)?0:~0;
	signalwindow_width_dirty=1;
	MaxSignalLength();
	signalarea_configure_event(signalarea, NULL);
	wavearea_configure_event(wavearea, NULL);
	DEBUG(printf("Show Base Symbols\n"));
	}

GTK_CHECK_MENU_ITEM(gtk_item_factory_get_widget(item_factory, 
	menu_items[WV_MENU_VSBS].path))->active=(show_base)?TRUE:FALSE;
}

/**/
static void menu_show_grid(GtkWidget *widget, gpointer data)
{
if(helpbox_is_active)
        {
        help_text_bold("\n\nShow Grid");
        help_text(
		" toggles the drawing of gridlines in the waveform display."
        );
        }
	else
	{
	display_grid=(display_grid)?0:~0;
	gtk_signal_emit_by_name (GTK_OBJECT (GTK_ADJUSTMENT(wave_hslider)),"changed");
	gtk_signal_emit_by_name (GTK_OBJECT (GTK_ADJUSTMENT(wave_hslider)),"value_changed");
	DEBUG(printf("Show Grid\n"));
	}

GTK_CHECK_MENU_ITEM(gtk_item_factory_get_widget(item_factory, 
	menu_items[WV_MENU_VSG].path))->active=(display_grid)?TRUE:FALSE;
}

/**/


/* this is the GtkMenuEntry structure used to create new menus.  The
 * first member is the menu definition string.  The second, the
 * default accelerator key used to access this menu function with
 * the keyboard.  The third is the callback function to call when
 * this menu item is selected (by the accelerator key, or with the
 * mouse.) The last member is the data to pass to your callback function.
 *
 * ...This has all been changed to use itemfactory stuff which is more
 * powerful.  The only real difference is the final item which tells 
 * the itemfactory just what the item "is".
 */

static GtkItemFactoryEntry menu_items[] =
{
#ifndef _MSC_VER
    WAVE_GTKIFE("/File/Open New Viewer", "Pause", menu_new_viewer, WV_MENU_FONV, "<Item>"),
    WAVE_GTKIFE("/File/<separator>", NULL, NULL, WV_MENU_SEP0, "<Separator>"),
#endif
    WAVE_GTKIFE("/File/Print To File", "Print", menu_print, WV_MENU_FPTF, "<Item>"),
    WAVE_GTKIFE("/File/<separator>", NULL, NULL, WV_MENU_SEP1, "<Separator>"),
    WAVE_GTKIFE("/File/Read Save File", "<Control>R", menu_read_save_file, WV_MENU_FRSF, "<Item>"),
    WAVE_GTKIFE("/File/Write Save File", "<Control>W", menu_write_save_file, WV_MENU_FWSF, "<Item>"),
    WAVE_GTKIFE("/File/<separator>", NULL, NULL, WV_MENU_SEP2, "<Separator>"),
    WAVE_GTKIFE("/File/Quit/Yes, Quit", "<Alt>Q", menu_quit, WV_MENU_FQY, "<Item>"),
    WAVE_GTKIFE("/File/Quit/Don't Quit", NULL, NULL, WV_MENU_FQN, "<Item>"),

    WAVE_GTKIFE("/Edit/Set Trace Max Hier", "<Control>T", menu_set_max_hier, WV_MENU_ESTMH, "<Item>"),
    WAVE_GTKIFE("/Edit/<separator>", NULL, NULL, WV_MENU_SEP3, "<Separator>"),
    WAVE_GTKIFE("/Edit/Insert Blank", "<Control>B", menu_insert_blank_traces, WV_MENU_EIB, "<Item>"),
    WAVE_GTKIFE("/Edit/Insert Comment", "<Control>C", menu_insert_comment_traces, WV_MENU_EIC, "<Item>"),
    WAVE_GTKIFE("/Edit/Alias Highlighted Trace", "<Alt>A", menu_alias, WV_MENU_EAHT, "<Item>"),
    WAVE_GTKIFE("/Edit/Remove Highlighted Aliases", "<Shift><Alt>A", menu_remove_aliases, WV_MENU_ERHA, "<Item>"),
    WAVE_GTKIFE("/Edit/Cut", "<Alt>C", menu_cut_traces, WV_MENU_EC, "<Item>"),
    WAVE_GTKIFE("/Edit/Paste", "<Alt>P", menu_paste_traces, WV_MENU_EP, "<Item>"),
    WAVE_GTKIFE("/Edit/<separator>", NULL, NULL, WV_MENU_SEP4, "<Separator>"),
    WAVE_GTKIFE("/Edit/Expand", "F3", menu_expand, WV_MENU_EE, "<Item>"),
    WAVE_GTKIFE("/Edit/Combine Down", "F4", menu_combine_down, WV_MENU_ECD, "<Item>"),
    WAVE_GTKIFE("/Edit/Combine Up", "F5", menu_combine_up, WV_MENU_ECU, "<Item>"),
    WAVE_GTKIFE("/Edit/Reduce Single Bit Vectors", "F6", menu_reduce_singlebit_vex, WV_MENU_ERSBV, "<Item>"),
    WAVE_GTKIFE("/Edit/<separator>", NULL, NULL, WV_MENU_SEP5, "<Separator>"),
    WAVE_GTKIFE("/Edit/Data Format/Hex", "<Alt>X", menu_dataformat_hex, WV_MENU_EDFH, "<Item>"),
    WAVE_GTKIFE("/Edit/Data Format/Decimal", "<Alt>D", menu_dataformat_dec, WV_MENU_EDFD, "<Item>"),
    WAVE_GTKIFE("/Edit/Data Format/Signed Decimal", NULL, menu_dataformat_signed, WV_MENU_EDFSD, "<Item>"),
    WAVE_GTKIFE("/Edit/Data Format/Binary", "<Alt>B", menu_dataformat_bin, WV_MENU_EDFB, "<Item>"),
    WAVE_GTKIFE("/Edit/Data Format/Octal", "<Alt>O", menu_dataformat_oct, WV_MENU_EDFO, "<Item>"),
    WAVE_GTKIFE("/Edit/Data Format/ASCII", NULL, menu_dataformat_ascii, WV_MENU_EDFA, "<Item>"),
    WAVE_GTKIFE("/Edit/Data Format/Right Justify/On", "<Alt>J", menu_dataformat_rjustify_on, WV_MENU_EDFRJON, "<Item>"),
    WAVE_GTKIFE("/Edit/Data Format/Right Justify/Off", "<Shift><Alt>J", menu_dataformat_rjustify_off, WV_MENU_EDFRJOFF, "<Item>"),
    WAVE_GTKIFE("/Edit/Data Format/Invert/On", "<Alt>I", menu_dataformat_invert_on, WV_MENU_EDFION, "<Item>"),
    WAVE_GTKIFE("/Edit/Data Format/Invert/Off", "<Shift><Alt>I", menu_dataformat_invert_off, WV_MENU_EDFIOFF, "<Item>"),
    WAVE_GTKIFE("/Edit/Data Format/Reverse Bits/On", "<Alt>V", menu_dataformat_reverse_on, WV_MENU_EDFRON, "<Item>"),
    WAVE_GTKIFE("/Edit/Data Format/Reverse Bits/Off", "<Shift><Alt>V", menu_dataformat_reverse_off, WV_MENU_EDFROFF, "<Item>"),
    WAVE_GTKIFE("/Edit/Show-Change All Highlighted", "<Control>S", menu_showchangeall, WV_MENU_ESCAH, "<Item>"),
    WAVE_GTKIFE("/Edit/Show-Change First Highlighted", "<Control>F", menu_showchange, WV_MENU_ESCFH, "<Item>"),
    WAVE_GTKIFE("/Edit/<separator>", NULL, NULL, WV_MENU_SEP6, "<Separator>"),
    WAVE_GTKIFE("/Edit/Time Warp/Warp Marked", NULL, menu_warp_traces, WV_MENU_WARP, "<Item>"),
    WAVE_GTKIFE("/Edit/Time Warp/Unwarp Marked", NULL, menu_unwarp_traces, WV_MENU_UNWARP, "<Item>"),
    WAVE_GTKIFE("/Edit/Time Warp/Unwarp All", NULL, menu_unwarp_traces_all, WV_MENU_UNWARPA, "<Item>"),
    WAVE_GTKIFE("/Edit/<separator>", NULL, NULL, WV_MENU_SEP7A, "<Separator>"),
    WAVE_GTKIFE("/Edit/Exclude", "<Shift><Alt>E", menu_dataformat_exclude_on, WV_MENU_EEX, "<Item>"),
    WAVE_GTKIFE("/Edit/Show", "<Shift><Alt>S", menu_dataformat_exclude_off, WV_MENU_ESH, "<Item>"),
    WAVE_GTKIFE("/Edit/<separator>", NULL, NULL, WV_MENU_SEP6A, "<Separator>"),
    WAVE_GTKIFE("/Edit/Highlight Regexp", "<Alt>R", menu_regexp_highlight, WV_MENU_EHR, "<Item>"),
    WAVE_GTKIFE("/Edit/UnHighlight Regexp", "<Shift><Alt>R", menu_regexp_unhighlight, WV_MENU_EUHR, "<Item>"),
    WAVE_GTKIFE("/Edit/Highlight All", "<Alt>H", menu_dataformat_highlight_all, WV_MENU_EHA, "<Item>"),
    WAVE_GTKIFE("/Edit/UnHighlight All", "<Shift><Alt>H", menu_dataformat_unhighlight_all, WV_MENU_EUHA, "<Item>"),
    WAVE_GTKIFE("/Edit/<separator>", NULL, NULL, WV_MENU_SEP6B, "<Separator>"),
    WAVE_GTKIFE("/Edit/Sort/Alphabetize All", NULL, menu_alphabetize, WV_MENU_ALPHA, "<Item>"),
    WAVE_GTKIFE("/Edit/Sort/Alphabetize All (CaseIns)", NULL, menu_alphabetize2, WV_MENU_ALPHA2, "<Item>"),
    WAVE_GTKIFE("/Edit/Sort/Sigsort All", NULL, menu_lexize, WV_MENU_LEX, "<Item>"),
    WAVE_GTKIFE("/Edit/Sort/Reverse All", NULL, menu_reverse, WV_MENU_RVS, "<Item>"),

    WAVE_GTKIFE("/Search/Pattern Search", "<Control>P", menu_tracesearchbox, WV_MENU_SPS, "<Item>"),
    WAVE_GTKIFE("/Search/<separator>", NULL, NULL, WV_MENU_SEP7B, "<Separator>"),
    WAVE_GTKIFE("/Search/Signal Search Regexp", "<Alt>S", menu_signalsearch, WV_MENU_SSR, "<Item>"),
    WAVE_GTKIFE("/Search/Signal Search Hierarchy", "<Alt>T", menu_hiersearch, WV_MENU_SSH, "<Item>"),
    WAVE_GTKIFE("/Search/Signal Search Tree", "<Shift><Alt>T", menu_treesearch, WV_MENU_SST, "<Item>"),
    WAVE_GTKIFE("/Search/<separator>", NULL, NULL, WV_MENU_SEP7, "<Separator>"),
    WAVE_GTKIFE("/Search/Autocoalesce", NULL, menu_autocoalesce, WV_MENU_ACOL, "<ToggleItem>"),
    WAVE_GTKIFE("/Search/Autocoalesce Reversal", NULL, menu_autocoalesce_reversal, WV_MENU_ACOLR, "<ToggleItem>"),
    WAVE_GTKIFE("/Search/Autoname Bundles", NULL, menu_autoname_bundles_on, WV_MENU_ABON, "<ToggleItem>"),
    WAVE_GTKIFE("/Search/Search Hierarchy Grouping", NULL, menu_hgrouping, WV_MENU_HTGP, "<ToggleItem>"),

    WAVE_GTKIFE("/Time/Move To Time", "F1", menu_movetotime, WV_MENU_TMTT, "<Item>"),
    WAVE_GTKIFE("/Time/Zoom/Zoom Amount", "F2", menu_zoomsize, WV_MENU_TZZA, "<Item>"),
    WAVE_GTKIFE("/Time/Zoom/Zoom Base", "<Shift>F2", menu_zoombase, WV_MENU_TZZB, "<Item>"),
    WAVE_GTKIFE("/Time/Zoom/Zoom In", "<Alt>Z", service_zoom_in, WV_MENU_TZZI, "<Item>"),
    WAVE_GTKIFE("/Time/Zoom/Zoom Out", "<Shift><Alt>Z", service_zoom_out, WV_MENU_TZZO, "<Item>"),
    WAVE_GTKIFE("/Time/Zoom/Zoom Best Fit", "<Alt>F", service_zoom_fit, WV_MENU_TZZBF, "<Item>"),
    WAVE_GTKIFE("/Time/Zoom/Zoom To Start", "Home", service_zoom_left, WV_MENU_TZZTS, "<Item>"),
    WAVE_GTKIFE("/Time/Zoom/Zoom To End", "End", service_zoom_right, WV_MENU_TZZTE, "<Item>"),
    WAVE_GTKIFE("/Time/Zoom/Undo Zoom", "<Alt>U", service_zoom_undo, WV_MENU_TZUZ, "<Item>"),
    WAVE_GTKIFE("/Time/Fetch/Fetch Size", "F7", menu_fetchsize, WV_MENU_TFFS, "<Item>"),
    WAVE_GTKIFE("/Time/Fetch/Fetch ->", "<Alt>2", fetch_right, WV_MENU_TFFR, "<Item>"),
    WAVE_GTKIFE("/Time/Fetch/Fetch <-", "<Alt>1", fetch_left, WV_MENU_TFFL, "<Item>"),
    WAVE_GTKIFE("/Time/Discard/Discard ->", "<Alt>4", discard_right, WV_MENU_TDDR, "<Item>"),
    WAVE_GTKIFE("/Time/Discard/Discard <-", "<Alt>3", discard_left, WV_MENU_TDDL, "<Item>"),
    WAVE_GTKIFE("/Time/Shift/Shift ->", "<Alt>6", service_right_shift, WV_MENU_TSSR, "<Item>"),
    WAVE_GTKIFE("/Time/Shift/Shift <-", "<Alt>5", service_left_shift, WV_MENU_TSSL, "<Item>"),
    WAVE_GTKIFE("/Time/Page/Page ->", "<Alt>8", service_right_page, WV_MENU_TPPR, "<Item>"),
    WAVE_GTKIFE("/Time/Page/Page <-", "<Alt>7", service_left_page, WV_MENU_TPPL, "<Item>"),

    WAVE_GTKIFE("/Markers/Show-Change Marker Data", "<Alt>M", menu_markerbox, WV_MENU_MSCMD, "<Item>"),
    WAVE_GTKIFE("/Markers/Drop Named Marker", "<Alt>N", drop_named_marker, WV_MENU_MDNM, "<Item>"),
    WAVE_GTKIFE("/Markers/Collect Named Marker", "<Shift><Alt>N", collect_named_marker, WV_MENU_MCNM, "<Item>"),
    WAVE_GTKIFE("/Markers/Collect All Named Markers", "<Shift><Control><Alt>N", collect_all_named_markers, WV_MENU_MCANM, "<Item>"),
    WAVE_GTKIFE("/Markers/Delete Primary Marker", "<Shift><Alt>M", delete_unnamed_marker, WV_MENU_MDPM, "<Item>"),
    WAVE_GTKIFE("/Markers/<separator>", NULL, NULL, WV_MENU_SEP8, "<Separator>"),
    WAVE_GTKIFE("/Markers/Wave Scrolling", "F9", wave_scrolling_on, WV_MENU_MWSON, "<ToggleItem>"),

    WAVE_GTKIFE("/View/Show Grid", "<Alt>G", menu_show_grid, WV_MENU_VSG, "<ToggleItem>"),
    WAVE_GTKIFE("/View/<separator>", NULL, NULL, WV_MENU_SEP9, "<Separator>"),
    WAVE_GTKIFE("/View/Show Base Symbols", "<Alt>F1", menu_show_base, WV_MENU_VSBS, "<ToggleItem>"),
    WAVE_GTKIFE("/View/<separator>", NULL, NULL, WV_MENU_SEP10, "<Separator>"),
    WAVE_GTKIFE("/View/Dynamic Resize", "<Alt>9", menu_enable_dynamic_resize, WV_MENU_VDR, "<ToggleItem>"),
    WAVE_GTKIFE("/View/<separator>", NULL, NULL, WV_MENU_SEP11, "<Separator>"),
    WAVE_GTKIFE("/View/Center Zooms", "F8", menu_center_zooms, WV_MENU_VCZ, "<ToggleItem>"),
    WAVE_GTKIFE("/View/<separator>", NULL, NULL, WV_MENU_SEP12, "<Separator>"),
    WAVE_GTKIFE("/View/Toggle Max-Marker", "F10", menu_toggle_max_or_marker, WV_MENU_VTMM, "<Item>"),
    WAVE_GTKIFE("/View/<separator>", NULL, NULL, WV_MENU_SEP13, "<Separator>"),
    WAVE_GTKIFE("/View/Constant Marker Update", "F11", menu_enable_constant_marker_update, WV_MENU_VCMU, "<ToggleItem>"),
    WAVE_GTKIFE("/View/<separator>", NULL, NULL, WV_MENU_SEP14, "<Separator>"),
    WAVE_GTKIFE("/View/Draw Roundcapped Vectors", "<Alt>F2", menu_use_roundcaps, WV_MENU_VDRV, "<ToggleItem>"),
    WAVE_GTKIFE("/View/<separator>", NULL, NULL, WV_MENU_SEP15, "<Separator>"),
    WAVE_GTKIFE("/View/Left Justified Signals", "<Shift>Home", menu_left_justify, WV_MENU_VLJS, "<Item>"),
    WAVE_GTKIFE("/View/Right Justified Signals", "<Shift>End", menu_right_justify, WV_MENU_VRJS, "<Item>"),
    WAVE_GTKIFE("/View/<separator>", NULL, NULL, WV_MENU_SEP16, "<Separator>"),
    WAVE_GTKIFE("/View/Zoom Pow10 Snap", "<Shift>Pause", menu_zoom10_snap, WV_MENU_VZPS, "<ToggleItem>"),
    WAVE_GTKIFE("/View/Full Precision", "<Alt>Pause", menu_use_full_precision, WV_MENU_VFTP, "<ToggleItem>"),
    WAVE_GTKIFE("/View/<separator>", NULL, NULL, WV_MENU_SEP17, "<Separator>"),
    WAVE_GTKIFE("/View/Remove Pattern Marks", NULL, menu_remove_marked, WV_MENU_RMRKS, "<Item>"),
    WAVE_GTKIFE("/View/<separator>", NULL, NULL, WV_MENU_SEP18, "<Separator>"),
    WAVE_GTKIFE("/View/LXT Clock Compress to Z", NULL, menu_lxt_clk_compress, WV_MENU_LXTCC2Z, "<ToggleItem>"),

    WAVE_GTKIFE("/Help/WAVE Help", "<Control>H", menu_help, WV_MENU_HWH, "<Item>"),
    WAVE_GTKIFE("/Help/Wave Version", "<Control>V", menu_version, WV_MENU_HWV, "<Item>"),
};


/*
 * set toggleitems to their initial states
 */
static void set_menu_toggles(void)
{
GTK_CHECK_MENU_ITEM(gtk_item_factory_get_widget(item_factory, 
	menu_items[WV_MENU_VZPS].path))->active=(zoom_pow10_snap)?TRUE:FALSE;

GTK_CHECK_MENU_ITEM(gtk_item_factory_get_widget(item_factory, 
	menu_items[WV_MENU_VSG].path))->active=(display_grid)?TRUE:FALSE;

GTK_CHECK_MENU_ITEM(gtk_item_factory_get_widget(item_factory, 
	menu_items[WV_MENU_VSBS].path))->active=(show_base)?TRUE:FALSE;

GTK_CHECK_MENU_ITEM(gtk_item_factory_get_widget(item_factory, 
	menu_items[WV_MENU_VDR].path))->active=(do_resize_signals)?TRUE:FALSE;

GTK_CHECK_MENU_ITEM(gtk_item_factory_get_widget(item_factory, 
	menu_items[WV_MENU_VCMU].path))->active=(constant_marker_update)?TRUE:FALSE;

GTK_CHECK_MENU_ITEM(gtk_item_factory_get_widget(item_factory, 
	menu_items[WV_MENU_VCZ].path))->active=(do_zoom_center)?TRUE:FALSE;

GTK_CHECK_MENU_ITEM(gtk_item_factory_get_widget(item_factory, 
	menu_items[WV_MENU_VDRV].path))->active=(use_roundcaps)?TRUE:FALSE;

GTK_CHECK_MENU_ITEM(gtk_item_factory_get_widget(item_factory, 
	menu_items[WV_MENU_MWSON].path))->active=(wave_scrolling)?TRUE:FALSE;

GTK_CHECK_MENU_ITEM(gtk_item_factory_get_widget(item_factory, 
	menu_items[WV_MENU_ABON].path))->active=(autoname_bundles)?TRUE:FALSE;

GTK_CHECK_MENU_ITEM(gtk_item_factory_get_widget(item_factory, 
	menu_items[WV_MENU_HTGP].path))->active=(hier_grouping)?TRUE:FALSE;

GTK_CHECK_MENU_ITEM(gtk_item_factory_get_widget(item_factory, 
	menu_items[WV_MENU_VFTP].path))->active=(use_full_precision)?TRUE:FALSE;

GTK_CHECK_MENU_ITEM(gtk_item_factory_get_widget(item_factory, 
	menu_items[WV_MENU_ACOL].path))->active=(autocoalesce)?TRUE:FALSE;

GTK_CHECK_MENU_ITEM(gtk_item_factory_get_widget(item_factory, 
	menu_items[WV_MENU_ACOLR].path))->active=(autocoalesce_reversal)?TRUE:FALSE;

GTK_CHECK_MENU_ITEM(gtk_item_factory_get_widget(item_factory, 
	menu_items[WV_MENU_LXTCC2Z].path))->active=(lxt_clock_compress_to_z)?TRUE:FALSE;
}


/*
 * create the menu through an itemfactory instance
 */
void get_main_menu(GtkWidget *window, GtkWidget ** menubar)
{
    int nmenu_items = sizeof(menu_items) / sizeof(menu_items[0]);
    GtkAccelGroup *global_accel;
    int i;

    global_accel = gtk_accel_group_new();
    item_factory = gtk_item_factory_new(GTK_TYPE_MENU_BAR, "<main>", global_accel);
    gtk_item_factory_create_items(item_factory, nmenu_items, menu_items, NULL);
    gtk_window_add_accel_group(GTK_WINDOW(window), global_accel);
    if(menubar)
	{
	*menubar = gtk_item_factory_get_widget (item_factory, "<main>");
        set_menu_toggles();
	}
}


/*
 * bail out
 */
void file_quit_cmd_callback (GtkWidget *widget, gpointer data)
{
    g_print ("%s\n", (char *) data);
    gtk_exit(0); 
}
