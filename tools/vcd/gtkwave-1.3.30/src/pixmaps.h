/* 
 * Copyright (c) Tony Bybell 1999.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#ifndef WAVE_PIXMAPS_H
#define WAVE_PIXMAPS_H

#include <gtk/gtk.h>

void make_pixmaps(GtkWidget *window);

extern GdkPixmap *larrow_pixmap;
extern GdkBitmap *larrow_mask;
extern GdkPixmap *rarrow_pixmap;
extern GdkBitmap *rarrow_mask;

extern GdkPixmap *zoomout_pixmap;
extern GdkBitmap *zoomout_mask;
extern GdkPixmap *zoomin_pixmap;
extern GdkBitmap *zoomin_mask;
extern GdkPixmap *zoomfit_pixmap;
extern GdkBitmap *zoomfit_mask;
extern GdkPixmap *zoomundo_pixmap;
extern GdkBitmap *zoomundo_mask;

extern GdkPixmap *zoom_larrow_pixmap;
extern GdkBitmap *zoom_larrow_mask;
extern GdkPixmap *zoom_rarrow_pixmap;
extern GdkBitmap *zoom_rarrow_mask;

#endif
