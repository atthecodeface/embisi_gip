/*a Includes
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "c_execution_model_class.h"
#include "c_memory_model.h"

/*a Defines
 */

/*a Types
 */

/*a Constructors and destructors
 */
/*f c_execution_model_class::c_execution_model_class
 */
c_execution_model_class::c_execution_model_class( c_memory_model *memory )
{
    trace_file = NULL;
}

/*f c_execution_model_class::~c_execution_model_class
 */
c_execution_model_class::~c_execution_model_class()
{
}

/*a Log methods
 */
/*f c_execution_model_class::log_reset
 */
void c_execution_model_class::log_reset( void )
{
    log_data.sequential = !log_data.branch;
    log_data.conditional = 0;
    log_data.condition_passed = 1;
    log_data.branch = 0;
    log_data.rfr = 0;
    log_data.rfw = 0;
    log_data.sign = 0;
}

/*f c_execution_model_class::log
 */
void c_execution_model_class::log( char *reason, unsigned int arg )
{
    if (!strcmp(reason, "address"))
    {
        log_data.address = arg;
    }
    else if (!strcmp(reason, "opcode"))
    {
        log_data.opcode = arg;
    }
    else if (!strcmp(reason, "conditional"))
    {
        log_data.conditional = arg;
    }
    else if (!strcmp(reason, "condition_passed"))
    {
        log_data.condition_passed = arg;
    }
    else if (!strcmp(reason, "branch"))
    {
        log_data.branch = arg;
    }
    else if (!strcmp(reason, "rfr"))
    {
        log_data.rfr |= (1<<arg);
    }
    else if (!strcmp(reason, "rfw"))
    {
        log_data.rfw |= (1<<arg);
    }
    else if (!strcmp(reason, "sign"))
    {
        log_data.sign = arg;
    }
}

/*f c_execution_model_class::log_display
 */
void c_execution_model_class::log_display( FILE *f )
{
    fprintf( f, "%08x %08x %1d %1d %1d %04x %04x %1d",
             log_data.address,
             log_data.opcode,
             log_data.sequential,
             log_data.conditional,
             log_data.condition_passed,
             log_data.rfr,
             log_data.rfw,
             log_data.sign
        );
    fprintf( f, "\n" );
}

/*a Trace interface
 */
/*f c_execution_model_class::trace_output
 */
void c_execution_model_class::trace_output( char *format, ... )
{
    va_list args;
    va_start( args, format );

    if (trace_file)
    {
        vfprintf( trace_file, format, args );
        fflush (trace_file);
    }
    else
    {
        vprintf( format, args );
    }

    va_end(args);
}

/*f c_execution_model_class::trace_set_file
 */
int c_execution_model_class::trace_set_file( char *filename )
{
    if (trace_file)
    {
        fclose( trace_file );
        trace_file = NULL;
    }
    if (filename)
    {
        trace_file = fopen( filename, "w+" );
    }
    return (trace_file!=NULL);
}

/*f c_execution_model_class::trace_region
 */
int c_execution_model_class::trace_region( int region, unsigned int start_address, unsigned int end_address )
{
    if ((region<0) || (region>=MAX_TRACING))
        return 0;
    trace_region_starts[region] = start_address;
    trace_region_ends[region] = end_address;
    tracing_enabled = 1;
    printf ("Tracing enabled from %x to %x\n", start_address, end_address);
    return 1;
}

/*f c_execution_model_class::trace_region_stop
 */
int c_execution_model_class::trace_region_stop( int region )
{
    if ((region<0) || (region>=MAX_TRACING))
        return 0;
    return trace_restart();
}

/*f c_execution_model_class::trace_all_stop
 */
int c_execution_model_class::trace_all_stop( void )
{
    tracing_enabled = 0;
    return 1;
}

/*f c_execution_model_class::trace_restart
 */
int c_execution_model_class::trace_restart( void )
{
    int i;

    tracing_enabled = 0;
    for (i=0; i<MAX_TRACING; i++)
    {
        if (trace_region_starts[i] != trace_region_ends[i])
        {
            tracing_enabled = 1;
        }
    }
    return 1;
}

