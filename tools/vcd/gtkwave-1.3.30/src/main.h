/* 
 * Copyright (c) Tony Bybell 1999.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#ifndef __MFMAIN_H__
#define __MFMAIN_H__

#define HAVE_PANED_PACK	/* undefine this if you have an older GTK */

extern char *whoami;

GtkWidget *create_text(void);
GtkWidget *create_zoom_buttons(void);
GtkWidget *create_page_buttons(void);
GtkWidget *create_fetch_buttons(void);
GtkWidget *create_discard_buttons(void);
GtkWidget *create_shift_buttons(void);
GtkWidget *create_entry_box(void);
GtkWidget *create_time_box(void);
GtkWidget *create_wavewindow(void);
GtkWidget *create_signalwindow(void);

extern GtkWidget *signalwindow;
extern GtkWidget *wavewindow;
extern char paned_pack_semantics;    /* 1 for paned_pack, 0 for paned_add */
extern int initial_window_x, initial_window_y; /* inital window sizes */

#endif
