#ifndef WAVE_AE2RDR_H
#define WAVE_AE2RDR_H

#include <inttypes.h>
#include "vcd.h"

extern char is_ae2;
TimeType ae2_main(char *fname);
void import_ae2_trace(nptr np);

/* start of ae2 subset */

typedef void* AE2_HANDLE;
typedef void (*AE2_SEVERROR) (const char*, ...);
typedef void (*AE2_MSG) (int, const char*, ...);
typedef void* (*AE2_ALLOC) (unsigned long size);
typedef void (*AE2_FREE) (void* ptr, unsigned long size);

AE2_HANDLE ae2_read_initialize(AE2_SEVERROR error_fn, AE2_MSG msg_fn, AE2_ALLOC alloc_fn, AE2_FREE free_fn, FILE* file);
void ae2_read_close(AE2_HANDLE handle);

uint64_t ae2_read_start_cycle(AE2_HANDLE handle);
uint64_t ae2_read_end_cycle(AE2_HANDLE handle);

unsigned long ae2_read_num_symbols(AE2_HANDLE handle);
unsigned long ae2_read_symbol_rows(AE2_HANDLE handle, unsigned long symbol_idx);
unsigned long ae2_read_symbol_length(AE2_HANDLE handle, unsigned long symbol_idx);

/* Returns the length of the string which is copied to name. */
unsigned long ae2_read_symbol_name(AE2_HANDLE handle, unsigned long symbol_idx, char* name);

/* Returns the number of bytes copied to value. */
unsigned long ae2_read_value(AE2_HANDLE handle, unsigned long symbol_idx, unsigned long row, unsigned long offset, long field_length, uint64_t cycle, char* value);

/* end of ae2 subset */

#endif
