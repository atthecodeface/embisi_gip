#ifndef WAVE_LXT_H
#define WAVE_LXT_H

#include "vcd.h"

TimeType lxt_main(char *fname);
void import_lxt_trace(nptr np);

#define LT_SECTION_END                          (0)
#define LT_SECTION_CHG                          (1)
#define LT_SECTION_SYNC_TABLE                   (2)
#define LT_SECTION_FACNAME                      (3)
#define LT_SECTION_FACNAME_GEOMETRY             (4)   
#define LT_SECTION_TIMESCALE                    (5)
#define LT_SECTION_TIME_TABLE                   (6)
#define LT_SECTION_INITIAL_VALUE                (7)
#define LT_SECTION_DOUBLE_TEST                  (8)
#define LT_SECTION_TIME_TABLE64                 (9)
#define LT_SECTION_ZFACNAME_PREDEC_SIZE         (10)
#define LT_SECTION_ZFACNAME_SIZE                (11)
#define LT_SECTION_ZFACNAME_GEOMETRY_SIZE       (12)
#define LT_SECTION_ZSYNC_SIZE                   (13)
#define LT_SECTION_ZTIME_TABLE_SIZE             (14)
#define LT_SECTION_ZCHG_PREDEC_SIZE             (15)
#define LT_SECTION_ZCHG_SIZE                    (16)
#define LT_SECTION_ZDICTIONARY                  (17)
#define LT_SECTION_ZDICTIONARY_SIZE             (18)
#define LT_SECTION_EXCLUDE_TABLE                (19)

#define LT_SYM_F_BITS           (0)
#define LT_SYM_F_INTEGER        (1<<0)
#define LT_SYM_F_DOUBLE         (1<<1)
#define LT_SYM_F_STRING         (1<<2)
#define LT_SYM_F_ALIAS          (1<<3)

#define LT_HDRID (0x0138)
#define LT_VERSION (0x0004)
#define LT_TRLID (0xB4)

#define LT_MINDICTWIDTH (16)

#endif
