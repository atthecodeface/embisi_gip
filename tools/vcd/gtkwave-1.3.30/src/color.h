/* 
 * Copyright (c) Tony Bybell 1999.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#ifndef WAVE_COLOR_H
#define WAVE_COLOR_H

#include <stdlib.h>
#include <gtk/gtk.h>

extern int color_back;
extern int color_grid;
extern int color_high;
extern int color_low;
extern int color_mark;
extern int color_mid;
extern int color_time;
extern int color_timeb;
extern int color_trans;
extern int color_umark;
extern int color_value;
extern int color_vbox;
extern int color_vtrans;
extern int color_x;
extern int color_xfill;

GdkGC *alloc_color(GtkWidget *widget, int tuple, GdkGC *fallback);	/* tuple is encoded as 32bit: --RRGGBB (>=0 is valid) */

#endif

