#include <stdio.h>
#include "ae2.h"

AE2_HANDLE ae2_read_initialize(AE2_SEVERROR eror_fn, AE2_MSG msg_fn, AE2_ALLOC alloc_fn, AE2_FREE free_fn, FILE* file)
{
fprintf(stderr, "AE2 support not enabled! Please recompile with the ae2reader.o binary.\n");
return((AE2_HANDLE)0);
}

void ae2_read_close(AE2_HANDLE handle)
{
}

unsigned long ae2_read_num_symbols(AE2_HANDLE handle)
{
return((unsigned long)0);
}

unsigned long ae2_read_symbol_name(AE2_HANDLE handle, unsigned long symbol_idx, char* name)
{
return((unsigned long)0);
}

unsigned long ae2_read_symbol_rows(AE2_HANDLE handle, unsigned long symbol_idx)
{
return((unsigned long)0);
}

unsigned long ae2_read_symbol_length(AE2_HANDLE handle, unsigned long symbol_idx)
{
return((unsigned long)0);
}

unsigned long ae2_read_value(AE2_HANDLE handle, unsigned long symbol_idx, unsigned long row, unsigned long offset, long field_length, uint64_t cycle, char* value)
{
return((unsigned long)0);
}

uint64_t ae2_read_prev_value(AE2_HANDLE handle, unsigned long symbol_idx, unsigned long row, unsigned long offset, long field_length, uint64_t cycle, char* value)
{
return((uint64_t)0);
}

uint64_t ae2_read_start_cycle(AE2_HANDLE handle)
{
return((uint64_t)0);
}

uint64_t ae2_read_end_cycle(AE2_HANDLE handle)
{
return((uint64_t)0);
}

