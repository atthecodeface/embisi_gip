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
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h> 
#include <fcntl.h>   
#include <errno.h>
#include <sys/types.h>
#include "analyzer.h"
#include "currenttime.h"
#include "aet.h"  
#include "vcd.h"
#include "alloca.h"
#include "fgetdynamic.h"
#include "debug.h"
#include "main.h"
#include "color.h"
#include "rc.h"

#ifndef _MSC_VER
	#include <unistd.h>
	#include <pwd.h>
	static char *rcname=".gtkwaverc";	/* name of environment file--POSIX */
#else
	static char *rcname="gtkwave.ini";      /* name of environment file--WIN32 */
	#define strcasecmp _stricmp
#endif

int rc_line_no;

/*
 * functions that set the individual rc variables..
 */
static int f_alt_hier_delimeter(char *str)
{
DEBUG(printf("f_alt_hier_delimeter(\"%s\")\n",str));

if(strlen(str)) { alt_hier_delimeter=str[0]; }
return(0);
}

static int f_append_vcd_hier(char *str)
{
DEBUG(printf("f_append_vcd_hier(\"%s\")\n",str));
append_vcd_slisthier(str);
return(0);
}

static int f_atomic_vectors(char *str)
{
DEBUG(printf("f_atomic_vectors(\"%s\")\n",str));
atomic_vectors=atoi_64(str)?1:0;
return(0);
}

static int f_autoname_bundles(char *str)
{
DEBUG(printf("f_autoname_bundles(\"%s\")\n",str));
autoname_bundles=atoi_64(str)?1:0;
return(0);
}

static int f_autocoalesce(char *str)
{
DEBUG(printf("f_autocoalesce(\"%s\")\n",str));
autocoalesce=atoi_64(str)?1:0;
return(0);
}

static int f_autocoalesce_reversal(char *str)
{
DEBUG(printf("f_autocoalesce_reversal(\"%s\")\n",str));
autocoalesce_reversal=atoi_64(str)?1:0;
return(0);
}

static int f_constant_marker_update(char *str)
{
DEBUG(printf("f_constant_marker_update(\"%s\")\n",str));
constant_marker_update=atoi_64(str)?1:0;
return(0);
}

static int f_convert_to_reals(char *str)
{
DEBUG(printf("f_convert_to_reals(\"%s\")\n",str));
convert_to_reals=atoi_64(str)?1:0;
return(0);
}

static int f_disable_tooltips(char *str)
{
DEBUG(printf("f_disable_tooltips(\"%s\")\n",str));
disable_tooltips=atoi_64(str)?1:0;
return(0);
}

static int f_do_initial_zoom_fit(char *str)
{
DEBUG(printf("f_do_initial_zoom_fit(\"%s\")\n",str));
do_initial_zoom_fit=atoi_64(str)?1:0;
return(0);
}

static int f_dynamic_resizing(char *str)
{
DEBUG(printf("f_dynamic_resizing(\"%s\")\n",str));
do_resize_signals=atoi_64(str)?1:0;
return(0);
}

static int f_enable_ghost_marker(char *str)
{
DEBUG(printf("f_enable_ghost_marker(\"%s\")\n",str));
enable_ghost_marker=atoi_64(str)?1:0;
return(0);
}

static int f_enable_horiz_grid(char *str)
{
DEBUG(printf("f_enable_horiz_grid(\"%s\")\n",str));
enable_horiz_grid=atoi_64(str)?1:0;
return(0);
}

static int f_enable_vcd_autosave(char *str)
{
DEBUG(printf("f_enable_vcd_autosave(\"%s\")\n",str));
make_vcd_save_file=atoi_64(str)?1:0;
return(0);
}

static int f_enable_vert_grid(char *str)
{
DEBUG(printf("f_enable_vert_grid(\"%s\")\n",str));
enable_vert_grid=atoi_64(str)?1:0;
return(0);
}

static int f_fontname_signals(char *str)
{
DEBUG(printf("f_fontname_signals(\"%s\")\n",str));
if(fontname_signals) free_2(fontname_signals);
fontname_signals=(char *)malloc_2(strlen(str)+1);
strcpy(fontname_signals,str);
return(0);
}

static int f_fontname_waves(char *str)
{
DEBUG(printf("f_fontname_signals(\"%s\")\n",str));
if(fontname_waves) free_2(fontname_waves);
fontname_waves=(char *)malloc_2(strlen(str)+1);
strcpy(fontname_waves,str);
return(0);
}

static int f_force_toolbars(char *str)
{
DEBUG(printf("f_force_toolbars(\"%s\")\n",str));
force_toolbars=atoi_64(str)?1:0;
return(0);
}

static int f_hier_delimeter(char *str)
{
DEBUG(printf("f_hier_delimeter(\"%s\")\n",str));

if(strlen(str)) { hier_delimeter=str[0]; hier_was_explicitly_set=1; }
return(0);
}

static int f_hier_grouping(char *str)
{
DEBUG(printf("f_hier_grouping(\"%s\")\n",str));
hier_grouping=atoi_64(str)?1:0;
return(0);
}

static int f_hier_max_level(char *str)
{
DEBUG(printf("f_hier_max_level(\"%s\")\n",str));
hier_max_level=atoi_64(str);
return(0);
}

static int f_hpane_pack(char *str)
{
DEBUG(printf("f_hpane_pack(\"%s\")\n",str));
paned_pack_semantics=atoi_64(str)?1:0;
return(0);
}

static int f_initial_window_x(char *str)
{
int val;
DEBUG(printf("f_initial_window_x(\"%s\")\n",str));
val=atoi_64(str);
initial_window_x=(val<=0)?-1:val;
return(0);
}

static int f_initial_window_y(char *str)
{
int val;
DEBUG(printf("f_initial_window_y(\"%s\")\n",str));
val=atoi_64(str);
initial_window_y=(val<=0)?-1:val;
return(0);
}

static int f_left_justify_sigs(char *str)
{
DEBUG(printf("f_left_justify_sigs(\"%s\")\n",str));
left_justify_sigs=atoi_64(str)?1:0;
return(0);
}

static int f_lxt_clock_compress_to_z(char *str)
{
DEBUG(printf("f_lxt_clock_compress_to_z(\"%s\")\n",str));
lxt_clock_compress_to_z=atoi_64(str)?1:0;
return(0);
}

static int f_page_divisor(char *str)
{
DEBUG(printf("f_page_divisor(\"%s\")\n",str));
sscanf(str,"%lg",&page_divisor);

if(page_divisor<0.01)
	{
	page_divisor=0.01;
	}
else
if(page_divisor>100.0)
	{
	page_divisor=100.0;
	}

if(page_divisor>1.0) page_divisor=1.0/page_divisor;

return(0);
}

static int f_ps_maxveclen(char *str)
{
DEBUG(printf("f_ps_maxveclen(\"%s\")\n",str));
ps_maxveclen=atoi_64(str);
if(ps_maxveclen<4)
	{
	ps_maxveclen=4;
	}
else
if(ps_maxveclen>66)
	{
	ps_maxveclen=66;
	}

return(0);
}

static int f_show_base_symbols(char *str)
{
DEBUG(printf("f_show_base_symbols(\"%s\")\n",str));
show_base=atoi_64(str)?1:0;
return(0);
}

static int f_show_grid(char *str)
{
DEBUG(printf("f_show_grid(\"%s\")\n",str));
display_grid=atoi_64(str)?1:0;
return(0);
}

static int f_use_big_fonts(char *str)
{
DEBUG(printf("f_use_big_fonts(\"%s\")\n",str));
use_big_fonts=atoi_64(str)?1:0;
return(0);
}

static int f_use_full_precision(char *str)
{
DEBUG(printf("f_use_full_precision(\"%s\")\n",str));
use_full_precision=atoi_64(str)?1:0;
return(0);
}

static int f_use_maxtime_display(char *str)
{
DEBUG(printf("f_use_maxtime_display(\"%s\")\n",str));
use_maxtime_display=atoi_64(str)?1:0;
return(0);
}

static int f_use_nonprop_fonts(char *str)
{
DEBUG(printf("f_use_nonprop_fonts(\"%s\")\n",str));
use_nonprop_fonts=atoi_64(str)?1:0;
return(0);
}

static int f_use_roundcaps(char *str)
{
DEBUG(printf("f_use_roundcaps(\"%s\")\n",str));
use_roundcaps=atoi_64(str)?1:0;
return(0);
}

static int f_use_scrollbar_only(char *str)
{
DEBUG(printf("f_use_scrollbar_only(\"%s\")\n",str));
use_scrollbar_only=atoi_64(str)?1:0;
return(0);
}

static int f_vcd_explicit_zero_subscripts(char *str)
{
DEBUG(printf("f_vcd_explicit_zero_subscripts(\"%s\")\n",str));
vcd_explicit_zero_subscripts=atoi_64(str)?0:-1;	/* 0==yes, -1==no */
return(0);
}

static int f_vector_padding(char *str)
{
DEBUG(printf("f_vector_padding(\"%s\")\n",str));
vector_padding=atoi_64(str);
if(vector_padding<4) vector_padding=4;
else if(vector_padding>16) vector_padding=16;
return(0);
}

static int f_wave_scrolling(char *str)
{
DEBUG(printf("f_wave_scrolling(\"%s\")\n",str));
wave_scrolling=atoi_64(str)?1:0;
return(0);
}

static int f_zoom_base(char *str)
{
float f;
DEBUG(printf("f_zoom_base(\"%s\")\n",str));
sscanf(str,"%f",&f);
if(f<1.5) f=1.5; else if(f>10.0) f=10.0;
zoombase=(gdouble)f;
return(0);
}

static int f_zoom_center(char *str)
{
DEBUG(printf("f_zoom_center(\"%s\")\n",str));
do_zoom_center=atoi_64(str)?1:0;
return(0);
}

static int f_zoom_pow10_snap(char *str)
{
DEBUG(printf("f_zoom_pow10_snap(\"%s\")\n",str));
zoom_pow10_snap=atoi_64(str)?1:0;
return(0);
}


static int rc_compare(const void *v1, const void *v2)
{
return(strcasecmp((char *)v1, ((struct rc_entry *)v2)->name));
}


/* make the color functions */
#define color_make(Z) static int f_color_##Z (char *str) \
{ \
int rgb; \
if((rgb=get_rgb_from_name(str))!=~0) \
	{ \
	color_##Z=rgb; \
	} \
return(0); \
}

color_make(back)
color_make(grid)
color_make(high)
color_make(low)
color_make(mark)
color_make(mid)
color_make(time)
color_make(timeb)
color_make(trans)
color_make(umark)
color_make(value)
color_make(vbox)
color_make(vtrans)
color_make(x)
color_make(xfill)


/*
 * rc variables...these MUST be in alphabetical order for the bsearch!
 */ 
static struct rc_entry rcitems[]=
{
{ "alt_hier_delimeter", f_alt_hier_delimeter },
{ "append_vcd_hier", f_append_vcd_hier },
{ "atomic_vectors", f_atomic_vectors },
{ "autocoalesce", f_autocoalesce },
{ "autocoalesce_reversal", f_autocoalesce_reversal },
{ "autoname_bundles", f_autoname_bundles },
{ "color_back", f_color_back },
{ "color_grid", f_color_grid },
{ "color_high", f_color_high },
{ "color_low", f_color_low },
{ "color_mark", f_color_mark },
{ "color_mid", f_color_mid },
{ "color_time", f_color_time },
{ "color_timeb", f_color_timeb },
{ "color_trans", f_color_trans },
{ "color_umark", f_color_umark },
{ "color_value", f_color_value },
{ "color_vbox", f_color_vbox },
{ "color_vtrans", f_color_vtrans },
{ "color_x", f_color_x },
{ "color_xfill", f_color_xfill },
{ "constant_marker_update", f_constant_marker_update },
{ "convert_to_reals", f_convert_to_reals },
{ "disable_tooltips", f_disable_tooltips },
{ "do_initial_zoom_fit", f_do_initial_zoom_fit },
{ "dynamic_resizing", f_dynamic_resizing },
{ "enable_ghost_marker", f_enable_ghost_marker },
{ "enable_horiz_grid", f_enable_horiz_grid}, 
{ "enable_vcd_autosave", f_enable_vcd_autosave },
{ "enable_vert_grid", f_enable_vert_grid}, 
{ "fontname_signals", f_fontname_signals}, 
{ "fontname_waves", f_fontname_waves}, 
{ "force_toolbars", f_force_toolbars}, 
{ "hier_delimeter", f_hier_delimeter },
{ "hier_grouping", f_hier_grouping },
{ "hier_max_level", f_hier_max_level },
{ "hpane_pack", f_hpane_pack },
{ "initial_window_x", f_initial_window_x },
{ "initial_window_y", f_initial_window_y },
{ "left_justify_sigs", f_left_justify_sigs },
{ "lxt_clock_compress_to_z", f_lxt_clock_compress_to_z },
{ "page_divisor", f_page_divisor },
{ "ps_maxveclen", f_ps_maxveclen },
{ "show_base_symbols", f_show_base_symbols },
{ "show_grid", f_show_grid },
{ "use_big_fonts", f_use_big_fonts },
{ "use_full_precision", f_use_full_precision },
{ "use_maxtime_display", f_use_maxtime_display },
{ "use_nonprop_fonts", f_use_nonprop_fonts },
{ "use_roundcaps", f_use_roundcaps },
{ "use_scrollbar_only", f_use_scrollbar_only },
{ "vcd_explicit_zero_subscripts", f_vcd_explicit_zero_subscripts },
{ "vector_padding", f_vector_padding },
{ "wave_scrolling", f_wave_scrolling },
{ "zoom_base", f_zoom_base },
{ "zoom_center", f_zoom_center },
{ "zoom_pow10_snap", f_zoom_pow10_snap }
};



void read_rc_file(void)
{
FILE *handle;

#ifndef _MSC_VER
if(!(handle=fopen(rcname,"rb")))
	{
	char *home;
	char *rcpath;
	
	home=getpwuid(geteuid())->pw_dir;
	rcpath=alloca(strlen(home)+1+strlen(rcname)+1);
	strcpy(rcpath,home);
	strcat(rcpath,"/");
	strcat(rcpath,rcname);

	if(!(handle=fopen(rcpath,"rb")))
		{
		errno=0;
		return; /* no .rc file */
		} 
	}
#else
if(!(handle=fopen(rcname,"rb")))		/* no concept of ~ in win32 */
	{
	errno=0;
	return; /* no .rc file */
	} 
#endif

rc_line_no=0;
while(!feof(handle))
	{
	char *str;

	rc_line_no++;
	if((str=fgetmalloc(handle)))
		{
		int i, len;
		len=strlen(str);
		if(len)
			{
			for(i=0;i<len;i++)
				{
				int pos;
				if((str[i]==' ')||(str[i]=='\t')) continue;	/* skip leading ws */
				if(str[i]=='#') break; 				/* is a comment */
				for(pos=i;i<len;i++)
					{
					if((str[i]==' ')||(str[i]=='\t'))
						{
						str[i]=0; /* null term envname */
	
						for(i=i+1;i<len;i++)
							{
							struct rc_entry *r;
	
							if((str[i]==' ')||(str[i]=='\t')) continue;
							if((r=bsearch((void *)(str+pos), (void *)rcitems, 
								sizeof(rcitems)/sizeof(struct rc_entry), 
								sizeof(struct rc_entry), rc_compare)))
								{
								int j;

								for(j=len-1;j>=i;j--)
									{
									if((str[j]==' ')||(str[j]=='\t')) /* nuke trailing spaces */
										{
										str[j]=0;
										continue;
										}
										else
										{
										break;
										}
									}
								r->func(str+i); /* call resolution function */
								}
							break;
							}
						break;	/* added so multiple word values work properly*/
						}
					}
				break;
				}

			}
		free_2(str);
		}
	}

fclose(handle);
errno=0;
return;
}
