/* 
 * Copyright (c) Tony Bybell 1999.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include "pixmaps.h"

/* XPM */
static char *icon_larrow[] = {
/* width height num_colors chars_per_pixel */
"    21    11        2            1",
/* colors */
". c #000000",
"# c None",
/* pixels */
"#########.###########",
"#######...###########",
"#####.....###########",
"###.......###########",
"#.........###########",
".....................",
"#.........###########",
"###.......###########",
"#####.....###########",
"#######...###########",
"#########.###########"
};

GdkPixmap *larrow_pixmap;
GdkBitmap *larrow_mask;


/* XPM */
static char *icon_rarrow[] = {
/* width height num_colors chars_per_pixel */
"    21    11        2            1",
/* colors */
". c #000000",
"# c None",
/* pixels */
"###########.#########",
"###########...#######",
"###########.....#####",
"###########.......###",
"###########.........#",
".....................",
"###########.........#",
"###########.......###",
"###########.....#####",
"###########...#######",
"###########.#########"
};

GdkPixmap *rarrow_pixmap;
GdkBitmap *rarrow_mask;


/* XPM */
static char *icon_zoomout[] = {
/* width height num_colors chars_per_pixel */
"    27    11        2            1",
/* colors */
". c #000000",
"# c None",
/* pixels */
"#########.#######.#########",
"#######...#######...#######",
"#####.....#######.....#####",
"###.......#######.......###",
"#.........#######.........#",
"...........................",
"#.........#######.........#",
"###.......#######.......###",
"#####.....#######.....#####",
"#######...#######...#######",
"#########.#######.#########"
};

GdkPixmap *zoomout_pixmap;
GdkBitmap *zoomout_mask;


/* XPM */
static char *icon_zoomin[] = {
/* width height num_colors chars_per_pixel */
"    27    11        2            1",
/* colors */
". c #000000",
"# c None",
/* pixels */
"###.###################.###",
"###...###############...###",
"###.....###########.....###",
"###.......#######.......###",
"###.........###.........###",
"...........................",
"###.........###.........###",
"###.......#######.......###",
"###.....###########.....###",
"###...###############...###",
"###.###################.###"
};

GdkPixmap *zoomin_pixmap;
GdkBitmap *zoomin_mask;


/* XPM */
static char *icon_zoomfit[] = {
/* width height num_colors chars_per_pixel */
"    27    11        2            1",
/* colors */
". c #000000",
"# c None",
/* pixels */
".########.#######.########.",
".######...#######...######.",
".####.....#######.....####.",
".##.......#######.......##.",
"..........#######..........",
"...........................",
"..........#######..........",
".##.......#######.......##.",
".####.....#######.....####.",
".######...#######...######.",
".########.#######.########."
};

GdkPixmap *zoomfit_pixmap;
GdkBitmap *zoomfit_mask;


/* XPM */
static char *icon_zoomundo[] = {
/* width height num_colors chars_per_pixel */
"    27    11        2            1",
/* colors */
". c #000000",
"# c None",
/* pixels */
"..##..#..##..#.....###....#",
"..##..#..##..#..##..#..##..",
"..##..#..##..#..##..#..##..",
"..##..#...#..#..##..#..##..",
"..##..#......#..##..#..##..",
"..##..#......#..##..#..##..",
"..##..#..#...#..##..#..##..",
"..##..#..##..#..##..#..##..",
"..##..#..##..#..##..#..##..",
"..##..#..##..#..##..#..##..",
"#....##..##..#.....###....#"
};

GdkPixmap *zoomundo_pixmap;
GdkBitmap *zoomundo_mask;


/* XPM */
static char *zoom_larrow[] = {
/* width height num_colors chars_per_pixel */
"    27    11        2            1",
/* colors */
". c #000000",
"# c None",
/* pixels */
"#.#########.###############",
"#.#######...###############",
"#.#####.....###############",
"#.###.......###############",
"#.#.........###############",
"#.......................###",
"#.#.........###############",
"#.###.......###############",
"#.#####.....###############",
"#.#######...###############",
"#.#########.###############"
};

GdkPixmap *zoom_larrow_pixmap;
GdkBitmap *zoom_larrow_mask;


/* XPM */
static char *zoom_rarrow[] = {
/* width height num_colors chars_per_pixel */
"    27    11        2            1",
/* colors */
". c #000000",
"# c None",
/* pixels */
"###############.#########.#",
"###############...#######.#",
"###############.....#####.#",
"###############.......###.#",
"###############.........#.#",
"###.......................#",
"###############.........#.#",
"###############.......###.#",
"###############.....#####.#",
"###############...#######.#",
"###############.#########.#"
};

GdkPixmap *zoom_rarrow_pixmap;
GdkBitmap *zoom_rarrow_mask;




void make_pixmaps(GtkWidget *window)
{
GtkStyle *style;

style=gtk_widget_get_style(window);

larrow_pixmap=gdk_pixmap_create_from_xpm_d(window->window, &larrow_mask,
	&style->bg[GTK_STATE_NORMAL], (gchar **)icon_larrow);
rarrow_pixmap=gdk_pixmap_create_from_xpm_d(window->window, &rarrow_mask,
	&style->bg[GTK_STATE_NORMAL], (gchar **)icon_rarrow);

zoomout_pixmap=gdk_pixmap_create_from_xpm_d(window->window, &zoomout_mask,
	&style->bg[GTK_STATE_NORMAL], (gchar **)icon_zoomout);
zoomin_pixmap=gdk_pixmap_create_from_xpm_d(window->window, &zoomin_mask,
	&style->bg[GTK_STATE_NORMAL], (gchar **)icon_zoomin);
zoomfit_pixmap=gdk_pixmap_create_from_xpm_d(window->window, &zoomfit_mask,
	&style->bg[GTK_STATE_NORMAL], (gchar **)icon_zoomfit);
zoomundo_pixmap=gdk_pixmap_create_from_xpm_d(window->window, &zoomundo_mask,
	&style->bg[GTK_STATE_NORMAL], (gchar **)icon_zoomundo);

zoom_larrow_pixmap=gdk_pixmap_create_from_xpm_d(window->window, &zoom_larrow_mask,
	&style->bg[GTK_STATE_NORMAL], (gchar **)zoom_larrow);
zoom_rarrow_pixmap=gdk_pixmap_create_from_xpm_d(window->window, &zoom_rarrow_mask,
	&style->bg[GTK_STATE_NORMAL], (gchar **)zoom_rarrow);
}
