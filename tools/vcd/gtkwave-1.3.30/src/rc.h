/* 
 * Copyright (c) Tony Bybell 1999.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#ifndef WAVE_RC_H
#define WAVE_RC_H

struct rc_entry
{
char *name;
int (*func)(char *);
};

void read_rc_file(void);
int get_rgb_from_name(char *str);

extern int rc_line_no;

#endif
