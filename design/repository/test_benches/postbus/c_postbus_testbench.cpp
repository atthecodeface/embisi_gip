/*a Copyright Gavin J Stark
 */

/*a Examples
 */

/*a Includes
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "be_model_includes.h"
#include "sl_general.h"
#include "sl_exec_file.h"
#include "sl_token.h"
#include "sl_data_stream.h"
#include "postbus.h"

/*a Defines
 */
#define MAX_CHANNELS (1)
#define MAX_SOURCES (1)
#define MAX_SINKS (1)

#define ASSIGN_TO_BIT(vector,size,bit,value) se_cmodel_assist_assign_to_bit(vector,size,bit,value)
#define ASSIGN_TO_BIT_RANGE(vector,size,bit,length,value) se_cmodel_assist_assign_to_bit_range(vector,size,bit,length,value)
#define COVER_STATEMENT(stmt_number) {if(stmt_number>=0)se_cmodel_assist_coverage_statement_reached(coverage,stmt_number);}
#define ASSERT(expr,string) {if (!(expr)) {\
    engine->message->add_error( NULL, error_level_info, error_number_se_dated_assertion, error_id_sl_exec_file_allocate_and_read_exec_file, \
        error_arg_type_integer, engine->cycle(),\
        error_arg_type_malloc_string, engine->get_instance_name(engine_handle),  \
        error_arg_type_const_string, string,\
        error_arg_type_none );}}

/*a Types for postbus_testbench
*/
/*t cmd_source_*
 */
enum
{
     cmd_source_send=sl_exec_file_cmd_first_external,
     cmd_wait,
     cmd_wait_for_complete,
};

/*t fn_*
 */
enum
{
    fn_input=sl_exec_file_fn_first_internal,
};

/*t t_src_fsm
*/
typedef enum t_src_fsm
{
    src_fsm_idle,
    src_fsm_first,
    src_fsm_data,
    src_fsm_last,
    src_fsm_ef_first,
    src_fsm_ef_data,
    src_fsm_ef_last,
};

/*t t_tgt_fsm
*/
typedef enum t_tgt_fsm
{
    tgt_fsm_idle,
    tgt_fsm_data,
};

/*t t_mon_fsm
*/
typedef enum t_mon_fsm
{
    mon_fsm_idle,
    mon_fsm_first_and_last,
    mon_fsm_first,
    mon_fsm_last,
};

/*t mon_level_*
 */
enum
{
    mon_level_none = 0,
    mon_level_verbose = 1, // Output a message on every clock with the signals on the bus
    mon_level_protocol = 2, // Check the postbus protocol is adhered to
    mon_level_data = 4, // Check data according to type given in the transaction word (if length is nonzero)
};

/*t t_fc_type
*/
typedef enum
{
    fc_type_none = 0,
    fc_type_transaction = 1,
    fc_type_data = 2,
} t_fc_type;

/*t t_source
*/
typedef struct t_source
{
    t_sl_data_stream *data_stream;
    int interval;
    int ready_to_clock;
    int channel;
} t_source;

/*t t_sink
*/
typedef struct t_sink
{
    t_sl_data_stream *data_stream;
    int ready_to_clock;
} t_sink;

/*t t_channel
 */
typedef struct t_channel
{
    t_fc_type fc_type;
    int fc_initial_credit;
} t_channel;

/*t t_sink_state
*/
typedef struct t_sink_state
{
    int counter;
} t_sink_state;

/*t t_source_state
*/
typedef struct t_source_state
{
    int counter;
    int pending;
} t_source_state;

/*t t_channel_state
*/
typedef struct t_channel_state
{
    int fc_credit;
    int source;
    int pending;
};

/*t t_postbus_testbench_event_data
 */
typedef struct t_postbus_testbench_event_data
{
    struct t_postbus_testbench_event_data *next_in_list;
    t_sl_exec_file_event_ptr event;
    int wait_for_complete;
    int timeout;
    int fired;
} t_postbus_testbench_event_data;

/*t t_postbus_testbench_posedge_int_clock_state
*/
typedef struct t_postbus_testbench_posedge_int_clock_state
{
    t_src_fsm src_fsm;
    unsigned int src_data;
    int src_left;
    int src_ptr;
    int src_channel;
    t_channel_state src_channel_state[MAX_CHANNELS];
    t_source_state src_source_state[MAX_SOURCES];

    t_tgt_fsm tgt_fsm;
    int tgt_sink;
    t_sink_state tgt_sink_state[MAX_SINKS];

    t_mon_fsm mon_fsm;
    unsigned int mon_failure;
    unsigned int last_mon_type;
    unsigned int last_mon_ack;
    unsigned int last_mon_data;

} t_postbus_testbench_posedge_int_clock_state;

/*t t_postbus_testbench_inputs
*/
typedef struct t_postbus_testbench_inputs
{
    unsigned int *int_reset;

    unsigned int *tgt_type;
    unsigned int *tgt_data;

    unsigned int *src_ack;

    unsigned int *mon_type;
    unsigned int *mon_data;
    unsigned int *mon_ack;

} t_postbus_testbench_inputs;

/*t t_postbus_testbench_combinatorials
*/
typedef struct t_postbus_testbench_combinatorials
{
    unsigned int tgt_ack;
    unsigned int src_type;
    unsigned int src_data;

} t_postbus_testbench_combinatorials;

/*t c_postbus_testbench
*/
class c_postbus_testbench
{
public:
    c_postbus_testbench::c_postbus_testbench( class c_engine *eng, void *eng_handle );
    c_postbus_testbench::~c_postbus_testbench();
    t_sl_error_level c_postbus_testbench::delete_instance( void );
    t_sl_error_level c_postbus_testbench::reset( int pass );
    t_sl_error_level c_postbus_testbench::evaluate_combinatorials( void );
    t_sl_error_level c_postbus_testbench::preclock_posedge_int_clock( void );
    t_sl_error_level c_postbus_testbench::clock_posedge_int_clock( void );
    t_sl_error_level c_postbus_testbench::reset_active_high_int_reset( void );
    void c_postbus_testbench::exec_file_instantiate_callback( struct t_sl_exec_file_data *file_data );
private:
    void c_postbus_testbench::run_exec_file( void );
    t_postbus_testbench_event_data *c_postbus_testbench::postbus_testbench_add_event( const char *reason, int wait_for_complete, int timeout );
    c_engine *engine;
    void *engine_handle;

    struct t_sl_exec_file_data *source_exec_file_data;
    char *source_exec_file_filename;

    int cycle_number;

    t_postbus_testbench_inputs inputs;
    t_postbus_testbench_combinatorials combinatorials;
    t_postbus_testbench_posedge_int_clock_state next_posedge_int_clock_state;
    t_postbus_testbench_posedge_int_clock_state posedge_int_clock_state;

    t_postbus_testbench_event_data *event_list;

    void c_postbus_testbench::add_channel( t_fc_type type, int credit );

    int num_channels;
    int num_sources;
    int num_sinks;
    int has_target;
    int monitor_level;
    t_source sources[MAX_SOURCES];
    t_sink sinks[MAX_SINKS];
    t_channel channels[MAX_CHANNELS];

    int source_exec_file_transaction_pending;
    int source_exec_file_transaction_length;
    int source_exec_file_transaction_data[32];
};

/*a Static function declarations
 */
static t_sl_exec_file_eval_fn ef_fn_eval_input;

/*a Static variables for postbus_testbench
*/
/*v postbus_file_source_cmds
 */
static t_sl_exec_file_cmd postbus_file_source_cmds[] =
{
     {cmd_source_send,          1, "source_send", "iiiiiiiiiii", "source_send <number> <data>*"},
     {cmd_wait,                 1, "wait",  "i", "wait <number of cycles>"},
     {cmd_wait_for_complete,    0, "wait_for_complete",  "", "wait_for_complete"},
     {sl_exec_file_cmd_none,    0, NULL, NULL, NULL }
};

/*v postbus_file_source_fns
 */
static t_sl_exec_file_fn postbus_file_source_fns[] =
{
     {fn_input,               "input",         'i', "s", "input(<name>)", ef_fn_eval_input },
     {sl_exec_file_fn_none, NULL,     0,   NULL, NULL },
};

/*f state_desc_postbus_testbench
*/
#define struct_offset( ptr, a ) (((char *)&(ptr->a))-(char *)ptr)
static t_postbus_testbench_posedge_int_clock_state *___postbus_testbench_posedge_int_clock__ptr;
static t_engine_state_desc state_desc_postbus_testbench_posedge_int_clock[] =
{
    {"src_fsm", engine_state_desc_type_bits, NULL, struct_offset(___postbus_testbench_posedge_int_clock__ptr, src_fsm), {2,0,0,0}, {NULL,NULL,NULL,NULL} },
    {"src_left", engine_state_desc_type_bits, NULL, struct_offset(___postbus_testbench_posedge_int_clock__ptr, src_left), {5,0,0,0}, {NULL,NULL,NULL,NULL} },
    {"fc_credit", engine_state_desc_type_bits, NULL, struct_offset(___postbus_testbench_posedge_int_clock__ptr, src_channel_state[0].fc_credit), {32,0,0,0}, {NULL,NULL,NULL,NULL} },
    {"", engine_state_desc_type_none, NULL, 0, {0,0,0,0}, {NULL,NULL,NULL,NULL} }
};

/*a Static wrapper functions for postbus_testbench
*/
/*f postbus_testbench_instance_fn
*/
static t_sl_error_level postbus_testbench_instance_fn( c_engine *engine, void *engine_handle )
{
    c_postbus_testbench *mod;
    mod = new c_postbus_testbench( engine, engine_handle );
    if (!mod)
        return error_level_fatal;
    return error_level_okay;
}

/*f postbus_testbench_delete_fn - simple callback wrapper for the main method
*/
static t_sl_error_level postbus_testbench_delete_fn( void *handle )
{
    c_postbus_testbench *mod;
    t_sl_error_level result;
    mod = (c_postbus_testbench *)handle;
    result = mod->delete_instance();
    delete( mod );
    return result;
}

/*f postbus_testbench_reset_fn
*/
static t_sl_error_level postbus_testbench_reset_fn( void *handle, int pass )
{
    c_postbus_testbench *mod;
    mod = (c_postbus_testbench *)handle;
    return mod->reset( pass );
}

/*f postbus_testbench_combinatorial_fn
*/
static t_sl_error_level postbus_testbench_combinatorial_fn( void *handle )
{
    c_postbus_testbench *mod;
    mod = (c_postbus_testbench *)handle;
    return mod->evaluate_combinatorials();
}

/*f postbus_testbench_preclock_posedge_int_clock_fn
*/
static t_sl_error_level postbus_testbench_preclock_posedge_int_clock_fn( void *handle )
{
    c_postbus_testbench *mod;
    mod = (c_postbus_testbench *)handle;
    return mod->preclock_posedge_int_clock();
}

/*f postbus_testbench_clock_posedge_int_clock_fn
*/
static t_sl_error_level postbus_testbench_clock_posedge_int_clock_fn( void *handle )
{
    c_postbus_testbench *mod;
    mod = (c_postbus_testbench *)handle;
    return mod->clock_posedge_int_clock();
}

/*a Exec file functions
 */
/*f ef_fn_eval_input
 */
static int ef_fn_eval_input( void *handle, t_sl_exec_file_data *file_data, t_sl_exec_file_value *args )
{
    return sl_exec_file_eval_fn_set_result( file_data, 0 );
}

/*f internal_exec_file_instantiate_callback
 */
static void internal_exec_file_instantiate_callback( void *handle, struct t_sl_exec_file_data *file_data )
{
     c_postbus_testbench *cif;
     cif = (c_postbus_testbench *)handle;
     cif->exec_file_instantiate_callback( file_data );
}

/*f c_postbus_testbench::exec_file_instantiate_callback
 */
void c_postbus_testbench::exec_file_instantiate_callback( struct t_sl_exec_file_data *file_data )
{

     sl_exec_file_set_environment_interrogation( file_data, (t_sl_get_environment_fn)sl_option_get_string, (void *)engine->get_option_list( engine_handle ) );

     engine->add_simulation_exec_file_enhancements( file_data );
     engine->waveform_add_exec_file_enhancements( file_data );
     engine->register_add_exec_file_enhancements( file_data, engine_handle );
}

/*f c_postbus_testbench::postbus_testbench_add_event
 */
t_postbus_testbench_event_data *c_postbus_testbench::postbus_testbench_add_event( const char *reason, int wait_for_complete, int timeout )
{
     t_postbus_testbench_event_data *event;
     char name[1024];

     event = (t_postbus_testbench_event_data *)malloc(sizeof(t_postbus_testbench_event_data));
     if (!event)
          return NULL;

     snprintf( name, sizeof(name), "__%s__%s__%d", sl_exec_file_command_threadname( source_exec_file_data ), reason, cycle_number );
     name[sizeof(name)-1] = 0;
     event->event = sl_exec_file_event_create( source_exec_file_data, name );
     sl_exec_file_event_reset( source_exec_file_data, event->event );

     event->wait_for_complete = wait_for_complete;
     event->timeout = -1;
     if (timeout>0)
     {
         event->timeout = cycle_number+timeout;
     }
     event->fired = 0;
     event->next_in_list = event_list;
     event_list = event;
     return event;
}

/*f c_postbus_testbench::run_exec_file
 */
void c_postbus_testbench::run_exec_file( void )
{
     int cmd, cmd_type;
     t_sl_exec_file_value *args;
     int i;
     t_postbus_testbench_event_data *event;

     if (source_exec_file_data)
     {
          while (1)
          {
               cmd_type = sl_exec_file_get_next_cmd( source_exec_file_data, &cmd, &args );
               if ( (cmd_type==0) || (cmd_type==2) ) // Break if nothing left to do
               {
                    break;
               }
               if (cmd_type==1) // If given command, handle that
               {
                    if (engine->simulation_handle_exec_file_command( source_exec_file_data, cmd, args ))
                    {
                    }
                    else if (engine->waveform_handle_exec_file_command( source_exec_file_data, cmd, args ))
                    {
                    }
                    else
                    {
                         switch (cmd)
                         {
                         case cmd_source_send:
                             source_exec_file_transaction_pending = 1;
                             source_exec_file_transaction_length = args[0].integer;
                             for (i=0; i<args[0].integer; i++)
                             {
                                 source_exec_file_transaction_data[i] = args[i+1].integer;
                             }
                             break;
                         case cmd_wait:
                             if ( (event = postbus_testbench_add_event( "wait", 0, (int)args[0].integer )) != NULL )
                             {
                                 sl_exec_file_wait_for_event( source_exec_file_data, NULL, event->event );
                             }
                             break;
                         case cmd_wait_for_complete:
                             if ( (event = postbus_testbench_add_event( "wait_for_complete", 1, -1 )) != NULL )
                             {
                                 sl_exec_file_wait_for_event( source_exec_file_data, NULL, event->event );
                             }
                             break;
                         }
                    }
               }
          }
     }
}

/*a Constructors and destructors for postbus_testbench
*/
/*f c_postbus_testbench::add_channel( t_fc_type type, int credit )
 */
void c_postbus_testbench::add_channel( t_fc_type type, int credit )
{
    if (num_channels<MAX_CHANNELS)
    {
        channels[num_channels].fc_type = type;
        channels[num_channels].fc_initial_credit = credit;
    }
    num_channels++;
}

/*f c_postbus_testbench::c_postbus_testbench
*/
c_postbus_testbench::c_postbus_testbench( class c_engine *eng, void *eng_handle )
{
    int i, j, k;
    char *option_string;
    char *string_copy;
    char *argv[256], *arg, *end;
    int argc;
    engine = eng;
    engine_handle = eng_handle;
    event_list = NULL;

    for (i=0; i<MAX_CHANNELS; i++)
    {
        channels[i].fc_type = fc_type_none;
        channels[i].fc_initial_credit = 0;
    }
    for (i=0; i<MAX_SOURCES; i++)
    {
        sources[i].interval = 0;
        sources[i].ready_to_clock = 1;
        sources[i].channel = 0;
        sources[i].data_stream = NULL;
    }

    num_channels = 0;
    option_string = engine->get_option_string( engine_handle, "channels", "" );
    argc = sl_str_split( option_string, &string_copy, sizeof(argv), argv, 0, 0, 1 ); // Split into strings, with potential lists
    for (i=0; i<argc; i++)
    {
        if (argv[i][0]=='(') // We expect FC type, initial FC credit
        {
            end = argv[i]+strlen(argv[i]); // Get end of argument
            arg = sl_token_next(0, argv[i]+1, end ); // get first token; this puts the ptr in arg, and puts a nul at the end of that first token
            j = (int)fc_type_none; // 'n' or default
            if (arg)
            {
                if (arg[0]=='t') j = (int)fc_type_transaction;
                if (arg[0]=='d') j = (int)fc_type_data;
            }
            arg = sl_token_next(1, arg, end ); // get continuation token;
            if (arg) sl_integer_from_token( arg, &k ); // size
            if (arg) add_channel( (t_fc_type) j, k );
        }
    }
    free(string_copy);

    num_sources = 0;
    option_string = engine->get_option_string( engine_handle, "sources", "" );
    argc = sl_str_split( option_string, &string_copy, sizeof(argv), argv, 0, 0, 1 ); // Split into strings, with potential lists
    for (i=0; (i<argc) && (num_sources<MAX_SOURCES); i++)
    {
        if (argv[i][0]=='(') // We expect interval, channel, stream type (i/~i,r/~r...), stream length, stream hdr, stream seed, stream value
        {
            end = argv[i]+strlen(argv[i]); // Get end of argument
            arg = sl_token_next(0, argv[i]+1, end ); // get first token; this puts the ptr in arg, and puts a nul at the end of that first token
            if (arg) sl_integer_from_token( arg, &sources[num_sources].interval );
            arg = sl_token_next(1, arg, end ); // get continuation token;
            if (arg) sl_integer_from_token( arg, &sources[num_sources].channel );
            arg = sl_token_next(1, arg, end ); // get continuation token;
            if (arg)
            {
                sources[num_sources].data_stream = sl_data_stream_create( arg );
            }
            if (sources[num_sources].data_stream)
            {
                if (sources[num_sources].channel>num_channels) sources[num_sources].channel=num_channels;
                if (sources[num_sources].channel<0) sources[num_sources].channel=0;
                num_sources++;
            }
        }
    }
    free(string_copy);

    num_sinks = 0;
    option_string = engine->get_option_string( engine_handle, "sinks", "" );
    argc = sl_str_split( option_string, &string_copy, sizeof(argv), argv, 0, 0, 1 ); // Split into strings, with potential lists
    for (i=0; (i<argc) && (num_sinks<MAX_SINKS); i++)
    {
        sinks[num_sinks].data_stream = sl_data_stream_create( argv[i] );
        if (sinks[num_sinks].data_stream)
        {
            num_sinks++;
        }
    }
    free(string_copy);

    source_exec_file_filename = engine->get_option_string( engine_handle, "source_exec", NULL );
    source_exec_file_data = NULL;
    if (source_exec_file_filename)
    {
        source_exec_file_filename = sl_str_alloc_copy( source_exec_file_filename );
    }

    has_target = engine->get_option_int( engine_handle, "has_target", 0 );
    if (num_sinks>0) has_target=1;

    monitor_level = engine->get_option_int( engine_handle, "monitor_level", 0 );

    engine->register_delete_function( engine_handle, (void *)this, postbus_testbench_delete_fn );
    engine->register_reset_function( engine_handle, (void *)this, postbus_testbench_reset_fn );

    engine->register_comb_fn( engine_handle, (void *)this, postbus_testbench_combinatorial_fn );
    engine->register_clock_fns( engine_handle, (void *)this, "int_clock", postbus_testbench_preclock_posedge_int_clock_fn, postbus_testbench_clock_posedge_int_clock_fn );

    engine->register_input_signal( engine_handle, "int_reset", 1, (int **)&inputs.int_reset );

    if (has_target)
    {
        engine->register_input_signal( engine_handle, "tgt_type", 2, (int **)&inputs.tgt_type );
        engine->register_input_used_on_clock( engine_handle, "tgt_type", "int_clock", 1 );
        engine->register_input_signal( engine_handle, "tgt_data", 32, (int **)&inputs.tgt_data );
        engine->register_input_used_on_clock( engine_handle, "tgt_data", "int_clock", 1 );
        engine->register_output_signal( engine_handle, "tgt_ack", 2, (int *)&combinatorials.tgt_ack );
        engine->register_output_generated_on_clock( engine_handle, "tgt_ack", "int_clock", 1 );
        combinatorials.tgt_ack = 0;
    }

    if ((num_sources>0) || (source_exec_file_filename))
    {
        engine->register_input_signal( engine_handle, "src_ack", 2, (int **)&inputs.src_ack );
        engine->register_input_used_on_clock( engine_handle, "src_ack", "int_clock", 1 );
        engine->register_output_signal( engine_handle, "src_data", 32, (int *)&combinatorials.src_data );
        engine->register_output_generated_on_clock( engine_handle, "src_data", "int_clock", 1 );
        engine->register_output_signal( engine_handle, "src_type", 2, (int *)&combinatorials.src_type );
        engine->register_output_generated_on_clock( engine_handle, "src_type", "int_clock", 1 );
        combinatorials.src_data = 0;
        combinatorials.src_type = 0;
    }

    if (monitor_level>0)
    {
        engine->register_input_signal( engine_handle, "mon_type", 2, (int **)&inputs.mon_type );
        engine->register_input_used_on_clock( engine_handle, "mon_type", "int_clock", 1 );
        engine->register_input_signal( engine_handle, "mon_data", 32, (int **)&inputs.mon_data );
        engine->register_input_used_on_clock( engine_handle, "mon_data", "int_clock", 1 );
        engine->register_input_signal( engine_handle, "mon_ack", 2, (int **)&inputs.mon_ack );
        engine->register_input_used_on_clock( engine_handle, "mon_ack", "int_clock", 1 );
        engine->register_output_signal( engine_handle, "mon_failure", 1, (int *)&posedge_int_clock_state.mon_failure );
        engine->register_output_generated_on_clock( engine_handle, "mon_failure", "int_clock", 1 );
        posedge_int_clock_state.mon_failure = 0;
    }

    engine->register_state_desc( engine_handle, 1, state_desc_postbus_testbench_posedge_int_clock, &posedge_int_clock_state, NULL );
    reset_active_high_int_reset();
    if (source_exec_file_filename)
    {
        sl_exec_file_allocate_and_read_exec_file( engine->error, engine->message, internal_exec_file_instantiate_callback, (void *)this, "source_exec", source_exec_file_filename, &source_exec_file_data, "Postbus source testbench", postbus_file_source_cmds, postbus_file_source_fns );
    }

}

/*f c_postbus_testbench::~c_postbus_testbench
*/
c_postbus_testbench::~c_postbus_testbench()
{
    delete_instance();
}

/*f c_postbus_testbench::delete_instance
*/
t_sl_error_level c_postbus_testbench::delete_instance( void )
{
    if (source_exec_file_data)
    {
        sl_exec_file_free( source_exec_file_data );
        source_exec_file_data = NULL;
    }
    return error_level_okay;
}

/*a Class reset/preclock/clock methods for postbus_testbench
*/
/*f c_postbus_testbench::reset
*/
t_sl_error_level c_postbus_testbench::reset( int pass )
{
    if (pass==0)
    {
        cycle_number = -1; // so any threads which do immediate waits get suitable event numbers that do not clash with the first preclock
        source_exec_file_transaction_pending = 0;
        source_exec_file_transaction_length = 0;
        if (source_exec_file_data)
        {
            sl_exec_file_reset( source_exec_file_data );
        }
        reset_active_high_int_reset();
        run_exec_file();
        cycle_number = 0; // so we are ready
    }
    else
    {
        evaluate_combinatorials();
    }
    return error_level_okay;
}

/*f c_postbus_testbench::reset_active_high_int_reset
*/
t_sl_error_level c_postbus_testbench::reset_active_high_int_reset( void )
{
    int i;

    posedge_int_clock_state.mon_fsm = mon_fsm_idle;
    posedge_int_clock_state.mon_failure = 0;

    posedge_int_clock_state.src_fsm = src_fsm_idle;
    posedge_int_clock_state.src_data = 0;
    posedge_int_clock_state.src_left = 0;
    posedge_int_clock_state.src_ptr = 0;

    for (i=0; i<MAX_CHANNELS; i++)
    {
        posedge_int_clock_state.src_channel_state[i].fc_credit = channels[i].fc_initial_credit;
        posedge_int_clock_state.src_channel_state[i].source = 0;
        posedge_int_clock_state.src_channel_state[i].pending = 0;
    }

    for (i=0; i<MAX_SOURCES; i++)
    {
        posedge_int_clock_state.src_source_state[i].counter = 0;
        posedge_int_clock_state.src_source_state[i].pending = 0;
        sources[i].ready_to_clock = 1;
    }

    for (i=0; i<MAX_SINKS; i++)
    {
        posedge_int_clock_state.tgt_sink_state[i].counter = 0;
        sinks[i].ready_to_clock = 1;
    }

    posedge_int_clock_state.tgt_fsm = tgt_fsm_idle;

    return error_level_okay;
}

/*f c_postbus_testbench::evaluate_combinatorials
*/
t_sl_error_level c_postbus_testbench::evaluate_combinatorials( void )
{
    if (inputs.int_reset[0]==1)
    {
        reset_active_high_int_reset();
    }

    if ((num_sources>0) || (source_exec_file_data))
    {
        switch (posedge_int_clock_state.src_fsm)
        {
        case src_fsm_idle:
            combinatorials.src_data = 0;
            combinatorials.src_type = postbus_word_type_idle;
            break;
        case src_fsm_first:
        case src_fsm_ef_first:
            combinatorials.src_data = posedge_int_clock_state.src_data;
            combinatorials.src_type = postbus_word_type_start;
            break;
        case src_fsm_data:
        case src_fsm_ef_data:
            combinatorials.src_data = posedge_int_clock_state.src_data;
            combinatorials.src_type = postbus_word_type_data;
            break;
        case src_fsm_last:
        case src_fsm_ef_last:
            combinatorials.src_data = posedge_int_clock_state.src_data;
            combinatorials.src_type = postbus_word_type_last;
            break;
        }
    }

    if ( has_target )
    {
        switch (posedge_int_clock_state.tgt_fsm)
        {
        case tgt_fsm_idle:
            combinatorials.tgt_ack = postbus_ack_taken;
            break;
        case tgt_fsm_data:
            combinatorials.tgt_ack = postbus_ack_taken;
            break;
        }
    }

    return error_level_okay;
}

/*f c_postbus_testbench::preclock_posedge_int_clock
*/
t_sl_error_level c_postbus_testbench::preclock_posedge_int_clock( void )
{
    t_postbus_testbench_event_data *event, **last_event_ptr;
    int src, chan;

    memcpy( &next_posedge_int_clock_state, &posedge_int_clock_state, sizeof(posedge_int_clock_state) );

    evaluate_combinatorials();

    /*b Fire any events that should, er, fire
     */
    for (event=event_list; event; event=event->next_in_list)
    {
        if (!event->fired)
        {
            if  ( (event->timeout!=-1) && (event->timeout==cycle_number) )
            {
                event->fired = 1;
            }
            if ( event->wait_for_complete &&
                 (!source_exec_file_transaction_pending) &&
                 (posedge_int_clock_state.src_fsm == src_fsm_idle) )
            {
                event->fired = 1;
            }
            if ( event->fired )
            {
                event->timeout = cycle_number; // record the time at which it fired
                if (event->event)
                {
                    sl_exec_file_event_fire( source_exec_file_data, event->event );
                }
            }
        }
    }

    /*b mon_fsm
    */
    if (monitor_level>0)
    {
        next_posedge_int_clock_state.last_mon_type = inputs.mon_type[0];
        next_posedge_int_clock_state.last_mon_data = inputs.mon_data[0];
        next_posedge_int_clock_state.last_mon_ack = inputs.mon_ack[0];
        switch (posedge_int_clock_state.mon_fsm)
        {
        case mon_fsm_idle:
        case mon_fsm_first_and_last:
        case mon_fsm_last:
            switch ((t_postbus_type)inputs.mon_type[0])
            {
            case postbus_word_type_idle:
                break;
            case postbus_word_type_start:
                if (inputs.mon_ack[0])
                {
                    if ((inputs.mon_data[0]>>postbus_command_last_bit)&1)
                    {
                        next_posedge_int_clock_state.mon_fsm = mon_fsm_first_and_last;
                    }
                    else
                    {
                        next_posedge_int_clock_state.mon_fsm = mon_fsm_first;
                    }
                }
                break;
            default:
                ASSERT( 1, "Protocol error: in idle/last data state, got non-idle and non-start transaction word" );
                break;
            }
            break;
        case mon_fsm_first:
            switch ((t_postbus_type)inputs.mon_type[0])
            {
            case postbus_word_type_data:
            case postbus_word_type_hold:
                break;
            case postbus_word_type_last:
                if (inputs.mon_ack[0])
                {
                    next_posedge_int_clock_state.mon_fsm = mon_fsm_last;
                }
                break;
            default:
                ASSERT( 1, "Protocol error: in data state, got start transaction word" );
                break;
            }
            break;
        }
    }

    /*b src_fsm
    */
    if (source_exec_file_data)
    {
        run_exec_file();
    }

    if ((num_sources>0) || (source_exec_file_data))
    {
        for (src=0; src<num_sources; src++)
        {
            next_posedge_int_clock_state.src_source_state[src].counter = posedge_int_clock_state.src_source_state[src].counter+1;
            if (posedge_int_clock_state.src_source_state[src].counter>=sources[src].interval)
            {
                next_posedge_int_clock_state.src_source_state[src].pending = 1;
                next_posedge_int_clock_state.src_source_state[src].counter = 0;
            }
        }
        for (src=0; src<num_sources; src++)
        {
            if (posedge_int_clock_state.src_source_state[src].pending)
            {
                chan = sources[src].channel;
                if (!posedge_int_clock_state.src_channel_state[chan].pending)
                {
                    next_posedge_int_clock_state.src_channel_state[chan].source = src;
                    next_posedge_int_clock_state.src_channel_state[chan].pending = 1;
                    if (sources[src].ready_to_clock)
                    {
                        sl_data_stream_start_packet( sources[src].data_stream );
                        sources[src].ready_to_clock = 0;
                    }
                }
            }
        }
        switch (posedge_int_clock_state.src_fsm)
        {
        case src_fsm_idle:
        {
            int fnd;
            for (chan=0, fnd=0; (!fnd) && (chan<num_channels); chan++)
            {
                src = posedge_int_clock_state.src_channel_state[chan].source;
                if (posedge_int_clock_state.src_channel_state[chan].pending)
                {
                    switch (channels[chan].fc_type)
                    {
                    case fc_type_none:
                        fnd = 1;
                        break;
                    case fc_type_data:
                        fnd = (posedge_int_clock_state.src_channel_state[chan].fc_credit >= sl_data_stream_packet_length(sources[src].data_stream));
                        break;
                    case fc_type_transaction:
                        fnd = (posedge_int_clock_state.src_channel_state[chan].fc_credit >= 1);
                        break;
                    }
                    if (fnd)
                    {
                        break;
                    }
                }
            }
            if (fnd)
            {
                src = posedge_int_clock_state.src_channel_state[chan].source;
                next_posedge_int_clock_state.src_channel = chan;

                next_posedge_int_clock_state.src_channel_state[chan].pending = 0;
                next_posedge_int_clock_state.src_source_state[src].pending = 0;

                next_posedge_int_clock_state.src_fsm = src_fsm_first;
                next_posedge_int_clock_state.src_left = sl_data_stream_packet_length(sources[src].data_stream);
                if (next_posedge_int_clock_state.src_left==0)
                {
                    next_posedge_int_clock_state.src_data = sl_data_stream_packet_header(sources[src].data_stream) | 1;
                }
                else
                {
                    next_posedge_int_clock_state.src_data = sl_data_stream_packet_header(sources[src].data_stream)& ~1;
                }
                switch (channels[chan].fc_type)
                {
                case fc_type_none:
                    break;
                case fc_type_data:
                    next_posedge_int_clock_state.src_channel_state[chan].fc_credit = posedge_int_clock_state.src_channel_state[chan].fc_credit - next_posedge_int_clock_state.src_left;
                    break;
                case fc_type_transaction:
                    next_posedge_int_clock_state.src_channel_state[chan].fc_credit = posedge_int_clock_state.src_channel_state[chan].fc_credit - 1;
                    break;
                }
            }
            else if (source_exec_file_transaction_pending)
            {
                next_posedge_int_clock_state.src_fsm = src_fsm_ef_first;
                next_posedge_int_clock_state.src_data = source_exec_file_transaction_data[0];
                next_posedge_int_clock_state.src_left = source_exec_file_transaction_length-1;
                next_posedge_int_clock_state.src_ptr = 1;
            }
            break;
        }
        case src_fsm_first:
            src = posedge_int_clock_state.src_channel_state[posedge_int_clock_state.src_channel].source;
            if (inputs.src_ack[0])
            {
                next_posedge_int_clock_state.src_left = posedge_int_clock_state.src_left-1;
                if (posedge_int_clock_state.src_left==0)
                {
                    next_posedge_int_clock_state.src_fsm = src_fsm_idle;
                }
                else if (posedge_int_clock_state.src_left==1)
                {
                    next_posedge_int_clock_state.src_fsm = src_fsm_last;
                    if (sources[src].ready_to_clock)
                    {
                        sl_data_stream_next_data_word( sources[src].data_stream );
                        sources[src].ready_to_clock = 0;
                    }
                    next_posedge_int_clock_state.src_data = sl_data_stream_packet_data_word(sources[src].data_stream);
                }
                else
                {
                    next_posedge_int_clock_state.src_fsm = src_fsm_data;
                    if (sources[src].ready_to_clock)
                    {
                        sl_data_stream_next_data_word( sources[src].data_stream );
                        sources[src].ready_to_clock = 0;
                    }
                    next_posedge_int_clock_state.src_data = sl_data_stream_packet_data_word(sources[src].data_stream);
                }
            }
            break;
        case src_fsm_data:
            src = posedge_int_clock_state.src_channel_state[posedge_int_clock_state.src_channel].source;
            if (inputs.src_ack[0])
            {
                next_posedge_int_clock_state.src_left = posedge_int_clock_state.src_left-1;
                if (posedge_int_clock_state.src_left==1)
                {
                    next_posedge_int_clock_state.src_fsm = src_fsm_last;
                    if (sources[src].ready_to_clock)
                    {
                        sl_data_stream_next_data_word( sources[src].data_stream );
                        sources[src].ready_to_clock = 0;
                    }
                    next_posedge_int_clock_state.src_data = sl_data_stream_packet_data_word(sources[src].data_stream);
                }
                else
                {
                    next_posedge_int_clock_state.src_fsm = src_fsm_data;
                    if (sources[src].ready_to_clock)
                    {
                        sl_data_stream_next_data_word( sources[src].data_stream );
                        sources[src].ready_to_clock = 0;
                    }
                    next_posedge_int_clock_state.src_data = sl_data_stream_packet_data_word(sources[src].data_stream);
                }
            }
            break;
        case src_fsm_last:
            src = posedge_int_clock_state.src_channel_state[posedge_int_clock_state.src_channel].source;
            if (inputs.src_ack[0])
            {
                next_posedge_int_clock_state.src_left = posedge_int_clock_state.src_left-1;
                next_posedge_int_clock_state.src_fsm = src_fsm_idle;
            }
            break;
        case src_fsm_ef_first:
            source_exec_file_transaction_pending = 0;
            if (inputs.src_ack[0])
            {
                next_posedge_int_clock_state.src_left = posedge_int_clock_state.src_left-1;
                next_posedge_int_clock_state.src_data = source_exec_file_transaction_data[posedge_int_clock_state.src_ptr];
                next_posedge_int_clock_state.src_ptr = posedge_int_clock_state.src_ptr+1;
                if (posedge_int_clock_state.src_left==0)
                {
                    next_posedge_int_clock_state.src_fsm = src_fsm_idle;
                }
                else if (posedge_int_clock_state.src_left==1)
                {
                    next_posedge_int_clock_state.src_fsm = src_fsm_ef_last;
                }
                else
                {
                    next_posedge_int_clock_state.src_fsm = src_fsm_ef_data;
                }
            }
            break;
        case src_fsm_ef_data:
            if (inputs.src_ack[0])
            {
                next_posedge_int_clock_state.src_left = posedge_int_clock_state.src_left-1;
                next_posedge_int_clock_state.src_data = source_exec_file_transaction_data[posedge_int_clock_state.src_ptr];
                next_posedge_int_clock_state.src_ptr = posedge_int_clock_state.src_ptr+1;
                if (posedge_int_clock_state.src_left==1)
                {
                    next_posedge_int_clock_state.src_fsm = src_fsm_ef_last;
                }
                else
                {
                    next_posedge_int_clock_state.src_fsm = src_fsm_ef_data;
                }
            }
            break;
        case src_fsm_ef_last:
            if (inputs.src_ack[0])
            {
                next_posedge_int_clock_state.src_left = posedge_int_clock_state.src_left-1;
                next_posedge_int_clock_state.src_fsm = src_fsm_idle;
            }
            break;
        }
    }

    /*b Now we can free any old fired events
     */
    last_event_ptr = &event_list;
    for (event=event_list; event; event=*last_event_ptr)
    {
        if ( (event->fired) && (cycle_number>event->timeout+5) )
        {
            if (event->event)
            {
                sl_exec_file_event_free( source_exec_file_data, event->event );
            }
            *last_event_ptr = event->next_in_list;
            free(event);
        }
        else
        {
            last_event_ptr = &event->next_in_list;
        }
     }

    /*b Done
     */
    return error_level_okay;
}

/*f c_postbus_testbench::clock_posedge_int_clock
*/
t_sl_error_level c_postbus_testbench::clock_posedge_int_clock( void )
{
    int i;

    /*b Copy next state to current
     */
    memcpy( &posedge_int_clock_state, &next_posedge_int_clock_state, sizeof(posedge_int_clock_state) );
    cycle_number++;
    for (i=0; i<MAX_SOURCES; i++)
    {
        sources[i].ready_to_clock = 1;
    }

    /*b Monitor the bus with a message if required
    */
//        fprintf( stderr, "(uses inputs - wrong)Type %d ack %d data %08x\n", inputs.mon_type[0], inputs.mon_ack[0], inputs.mon_data[0] );
    if (monitor_level&mon_level_verbose)
    {
        char buffer[256];
        sprintf( buffer, "(uses inputs - wrong)Type %d ack %d data %08x", posedge_int_clock_state.last_mon_type, posedge_int_clock_state.last_mon_ack, posedge_int_clock_state.last_mon_data );
        engine->message->add_error( NULL, error_level_info, error_number_se_dated_message, error_id_sl_exec_file_allocate_and_read_exec_file,
                                    error_arg_type_integer, engine->cycle(),
                                    error_arg_type_malloc_string, engine->get_instance_name(engine_handle), 
                                    error_arg_type_malloc_string, buffer,
                                    error_arg_type_none );
    }

    /*b Done
     */
    return evaluate_combinatorials();
}

/*a Initialization functions
*/
/*f c_postbus_testbench__init
*/
extern void c_postbus_testbench__init( void )
{
    se_external_module_register( 1, "postbus_testbench", postbus_testbench_instance_fn );
}

/*a Scripting support code
*/
/*f initpostbus_testbench
*/
extern "C" void initpostbus_testbench( void )
{
    c_postbus_testbench__init( );
    scripting_init_module( "postbus_testbench" );
}
