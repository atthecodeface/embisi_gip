/*
 * Copyright (c) Tony Bybell 1999.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#ifdef __linux__
#include <sys/types.h>
#include <stdlib.h>
#include <regex.h>
#else			/* or for any other compiler that doesn't support POSIX.2 regexs properly like xlc or vc++ */
#ifdef _MSC_VER
#include <malloc.h> 
#define STDC_HEADERS  
#define alloca _alloca  /* AIX doesn't like this */
#endif
#define REGEX_MAY_COMPILE
#include "gnu_regex.c"
#endif


static regex_t preg;
static int regex_ok=0;

/*
 * compile a regular expression into a regex_t and
 * dealloc any previously valid ones
 */
int wave_regex_compile(char *regex)
{
int comp_rc;

if(regex_ok) { regfree(&preg); } /* free previous regex_t ancillary data if valid */
comp_rc=regcomp(&preg, regex, REG_ICASE|REG_NOSUB);
return(regex_ok=(comp_rc)?0:1);
}


/*
 * do match
 */
int wave_regex_match(char *str)
{
int rc=regexec(&preg, str, 0, NULL, 0);

return((rc)?0:1);
}

