/*
 * Copyright (c) Tony Bybell 1999-2001
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */
#include "gtk12compat.h"
#include "currenttime.h"

char *fontname_signals=NULL;
char *fontname_waves=NULL;

void load_all_fonts(void)
{
if((fontname_signals)&&(strlen(fontname_signals)))
	{
        signalfont=gdk_font_load(fontname_signals);
	}
	else
	{
	if(use_big_fonts)
	        {
	        if(!use_nonprop_fonts) 
	                {
	                signalfont=gdk_font_load("-*-times-*-r-*-*-15-*-*-*-*-*-*-*");
	                }
	                else
	                {
	                signalfont=gdk_font_load("-*-courier-*-r-*-*-15-*-*-*-*-*-*-*");
	                }
	        }
	        else
	        {
	        if(use_nonprop_fonts)
	                {   
	                signalfont=gdk_font_load("-*-courier-*-r-*-*-14-*-*-*-*-*-*-*");
	                }
	        }
	}

if(!signalfont)
        {  
        signalfont=WAVE_GTK_SIGFONT;
        }

fontheight=(signalfont->ascent+signalfont->descent)+4;

if((fontname_waves)&&(strlen(fontname_waves)))
	{
        wavefont=wavefont_smaller=gdk_font_load(fontname_waves);
	}
	else
	{
	if(use_big_fonts)
	        {
	        wavefont=gdk_font_load("-*-courier-*-r-*-*-14-*-*-*-*-*-*-*");
	        wavefont_smaller=gdk_font_load("-*-courier-*-r-*-*-10-*-*-*-*-*-*-*");
	        }
	        else
	        {
	        wavefont=wavefont_smaller=gdk_font_load("-*-courier-*-r-*-*-10-*-*-*-*-*-*-*");
	        }
	}

if(!wavefont)
        {  
        wavefont=wavefont_smaller=WAVE_GTK_WAVEFONT;
        }

if(signalfont->ascent<wavefont->ascent)
	{
	fprintf(stderr, "Signalfont is smaller than wavefont.  Exiting!\n");
	exit(1);
	}

if(signalfont->ascent>100)
	{
	fprintf(stderr, "Fonts are too big!  Try fonts with a smaller size.  Exiting!\n");
	exit(1);
	}

wavecrosspiece=wavefont->ascent+1;
}
