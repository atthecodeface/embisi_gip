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

/*a Types for pcostbus_testbench
*/
/*t t_src_fsm
*/
typedef enum t_src_fsm
{
    src_fsm_idle,
    src_fsm_first,
    src_fsm_data,
    src_fsm_last,
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
    mon_level_protocol = 2, // Check the mii protocol is adhered to
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

/*t t_mii_testbench_posedge_int_clock_state
*/
typedef struct t_mii_testbench_posedge_int_clock_state
{
    t_src_fsm src_fsm;
    unsigned int src_data;
    int src_left;
    int src_channel;
    t_channel_state src_channel_state[MAX_CHANNELS];
    t_source_state src_source_state[MAX_SOURCES];

    t_tgt_fsm tgt_fsm;
    int tgt_sink;
    t_sink_state tgt_sink_state[MAX_SINKS];

    t_mon_fsm mon_fsm;
    unsigned int mon_failure;

} t_mii_testbench_posedge_int_clock_state;

/*t t_mii_testbench_inputs
*/
typedef struct t_mii_testbench_inputs
{
    unsigned int *int_reset;

    unsigned int *tgt_type;
    unsigned int *tgt_data;

    unsigned int *src_ack;

    unsigned int *mon_type;
    unsigned int *mon_data;
    unsigned int *mon_ack;

} t_mii_testbench_inputs;

/*t t_mii_testbench_combinatorials
*/
typedef struct t_mii_testbench_combinatorials
{
    unsigned int tgt_ack;
    unsigned int src_type;
    unsigned int src_data;

} t_mii_testbench_combinatorials;

/*t c_mii_testbench
*/
class c_mii_testbench
{
public:
    c_mii_testbench::c_mii_testbench( class c_engine *eng, void *eng_handle );
    c_mii_testbench::~c_mii_testbench();
    t_sl_error_level c_mii_testbench::delete_instance( void );
    t_sl_error_level c_mii_testbench::reset( void );
    t_sl_error_level c_mii_testbench::evaluate_combinatorials( void );
    t_sl_error_level c_mii_testbench::preclock_posedge_int_clock( void );
    t_sl_error_level c_mii_testbench::clock_posedge_int_clock( void );
    t_sl_error_level c_mii_testbench::reset_active_high_int_reset( void );
private:
    c_engine *engine;
    void *engine_handle;
    t_mii_testbench_inputs inputs;
    t_mii_testbench_combinatorials combinatorials;
    t_mii_testbench_posedge_int_clock_state next_posedge_int_clock_state;
    t_mii_testbench_posedge_int_clock_state posedge_int_clock_state;

    void c_mii_testbench::add_channel( t_fc_type type, int credit );

    int num_channels;
    int num_sources;
    int num_sinks;
    int has_target;
    int monitor_level;
    t_source sources[MAX_SOURCES];
    t_sink sinks[MAX_SINKS];
    t_channel channels[MAX_CHANNELS];
};

/*a Static variables for mii_testbench
*/
/*f state_desc_mii_testbench
*/
#define struct_offset( ptr, a ) (((char *)&(ptr->a))-(char *)ptr)
static t_mii_testbench_posedge_int_clock_state *___mii_testbench_posedge_int_clock__ptr;
static t_engine_state_desc state_desc_mii_testbench_posedge_int_clock[] =
{
    {"src_fsm", engine_state_desc_type_bits, NULL, struct_offset(___mii_testbench_posedge_int_clock__ptr, src_fsm), {2,0,0,0}, {NULL,NULL,NULL,NULL} },
    {"src_left", engine_state_desc_type_bits, NULL, struct_offset(___mii_testbench_posedge_int_clock__ptr, src_left), {5,0,0,0}, {NULL,NULL,NULL,NULL} },
    {"fc_credit", engine_state_desc_type_bits, NULL, struct_offset(___mii_testbench_posedge_int_clock__ptr, src_channel_state[0].fc_credit), {32,0,0,0}, {NULL,NULL,NULL,NULL} },
    {"", engine_state_desc_type_none, NULL, 0, {0,0,0,0}, {NULL,NULL,NULL,NULL} }
};

/*a Static wrapper functions for mii_testbench
*/
/*f mii_testbench_instance_fn
*/
static t_sl_error_level mii_testbench_instance_fn( c_engine *engine, void *engine_handle )
{
    c_mii_testbench *mod;
    mod = new c_mii_testbench( engine, engine_handle );
    if (!mod)
        return error_level_fatal;
    return error_level_okay;
}

/*f mii_testbench_delete_fn - simple callback wrapper for the main method
*/
static t_sl_error_level mii_testbench_delete_fn( void *handle )
{
    c_mii_testbench *mod;
    t_sl_error_level result;
    mod = (c_mii_testbench *)handle;
    result = mod->delete_instance();
    delete( mod );
    return result;
}

/*f mii_testbench_reset_fn
*/
static t_sl_error_level mii_testbench_reset_fn( void *handle )
{
    c_mii_testbench *mod;
    mod = (c_mii_testbench *)handle;
    return mod->reset();
}

/*f mii_testbench_combinatorial_fn
*/
static t_sl_error_level mii_testbench_combinatorial_fn( void *handle )
{
    c_mii_testbench *mod;
    mod = (c_mii_testbench *)handle;
    return mod->evaluate_combinatorials();
}

/*f mii_testbench_preclock_posedge_int_clock_fn
*/
static t_sl_error_level mii_testbench_preclock_posedge_int_clock_fn( void *handle )
{
    c_mii_testbench *mod;
    mod = (c_mii_testbench *)handle;
    return mod->preclock_posedge_int_clock();
}

/*f mii_testbench_clock_posedge_int_clock_fn
*/
static t_sl_error_level mii_testbench_clock_posedge_int_clock_fn( void *handle )
{
    c_mii_testbench *mod;
    mod = (c_mii_testbench *)handle;
    return mod->clock_posedge_int_clock();
}

/*a Constructors and destructors for mii_testbench
*/
/*f c_mii_testbench::add_channel( t_fc_type type, int credit )
 */
void c_mii_testbench::add_channel( t_fc_type type, int credit )
{
    if (num_channels<MAX_CHANNELS)
    {
        channels[num_channels].fc_type = type;
        channels[num_channels].fc_initial_credit = credit;
    }
    num_channels++;
}

/*f c_mii_testbench::c_mii_testbench
*/
c_mii_testbench::c_mii_testbench( class c_engine *eng, void *eng_handle )
{
    int i, j, k;
    char *option_string;
    char *string_copy;
    char *argv[256], *arg, *end;
    int argc;
    engine = eng;
    engine_handle = eng_handle;

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
    string_copy = (char *)malloc(strlen(option_string)+1);
    argc = sl_str_split( option_string, string_copy, sizeof(argv), argv, 0, 0, 1 ); // Split into strings, with potential lists
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
    string_copy = (char *)malloc(strlen(option_string)+1);
    argc = sl_str_split( option_string, string_copy, sizeof(argv), argv, 0, 0, 1 ); // Split into strings, with potential lists
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
    string_copy = (char *)malloc(strlen(option_string)+1);
    argc = sl_str_split( option_string, string_copy, sizeof(argv), argv, 0, 0, 1 ); // Split into strings, with potential lists
    for (i=0; (i<argc) && (num_sinks<MAX_SINKS); i++)
    {
        sinks[num_sinks].data_stream = sl_data_stream_create( argv[i] );
        if (sinks[num_sinks].data_stream)
        {
            num_sinks++;
        }
    }
    free(string_copy);

    has_target = engine->get_option_int( engine_handle, "has_target", 0 );
    if (num_sinks>0) has_target=1;

    monitor_level = engine->get_option_int( engine_handle, "monitor_level", 0 );

    engine->register_delete_function( engine_handle, (void *)this, mii_testbench_delete_fn );
    engine->register_reset_function( engine_handle, (void *)this, mii_testbench_reset_fn );

    engine->register_comb_fn( engine_handle, (void *)this, mii_testbench_combinatorial_fn );
    engine->register_clock_fns( engine_handle, (void *)this, "int_clock", mii_testbench_preclock_posedge_int_clock_fn, mii_testbench_clock_posedge_int_clock_fn );

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

    if (num_sources>0)
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

    engine->register_state_desc( engine_handle, 1, state_desc_mii_testbench_posedge_int_clock, &posedge_int_clock_state, NULL );
    reset_active_high_int_reset();

}

/*f c_mii_testbench::~c_mii_testbench
*/
c_mii_testbench::~c_mii_testbench()
{
    delete_instance();
}

/*f c_mii_testbench::delete_instance
*/
t_sl_error_level c_mii_testbench::delete_instance( void )
{
    return error_level_okay;
}

/*a Class reset/preclock/clock methods for mii_testbench
*/
/*f c_mii_testbench::reset
*/
t_sl_error_level c_mii_testbench::reset( void )
{
    reset_active_high_int_reset();
    evaluate_combinatorials();
    return error_level_okay;
}

/*f c_mii_testbench::reset_active_high_int_reset
*/
t_sl_error_level c_mii_testbench::reset_active_high_int_reset( void )
{
    int i;

    posedge_int_clock_state.mon_fsm = mon_fsm_idle;
    posedge_int_clock_state.mon_failure = 0;

    posedge_int_clock_state.src_fsm = src_fsm_idle;
    posedge_int_clock_state.src_data = 0;
    posedge_int_clock_state.src_left = 0;

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

/*f c_mii_testbench::evaluate_combinatorials
*/
t_sl_error_level c_mii_testbench::evaluate_combinatorials( void )
{
    if (inputs.int_reset[0]==1)
    {
        reset_active_high_int_reset();
    }

    if (num_sources>0)
    {
        switch (posedge_int_clock_state.src_fsm)
        {
        case src_fsm_idle:
            combinatorials.src_data = 0;
            combinatorials.src_type = postbus_word_type_idle;
            break;
        case src_fsm_first:
            combinatorials.src_data = posedge_int_clock_state.src_data;
            combinatorials.src_type = postbus_word_type_start;
            break;
        case src_fsm_data:
            combinatorials.src_data = posedge_int_clock_state.src_data;
            combinatorials.src_type = postbus_word_type_data;
            break;
        case src_fsm_last:
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

/*f c_mii_testbench::preclock_posedge_int_clock
*/
t_sl_error_level c_mii_testbench::preclock_posedge_int_clock( void )
{
    int src, chan;

    memcpy( &next_posedge_int_clock_state, &posedge_int_clock_state, sizeof(posedge_int_clock_state) );

    evaluate_combinatorials();

    /*b mon_fsm
    */
    if (monitor_level>0)
    {
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
    if (num_sources>0)
    {
        for (src=0; src<MAX_SOURCES; src++)
        {
            next_posedge_int_clock_state.src_source_state[src].counter = posedge_int_clock_state.src_source_state[src].counter+1;
            if (posedge_int_clock_state.src_source_state[src].counter>=sources[src].interval)
            {
                next_posedge_int_clock_state.src_source_state[src].pending = 1;
                next_posedge_int_clock_state.src_source_state[src].counter = 0;
            }
        }
        for (src=0; src<MAX_SOURCES; src++)
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
            for (chan=0, fnd=0; (!fnd) && (chan<MAX_CHANNELS); chan++)
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
                        sl_data_stream_next_data( sources[src].data_stream );
                        sources[src].ready_to_clock = 0;
                    }
                    next_posedge_int_clock_state.src_data = sl_data_stream_packet_data(sources[src].data_stream);
                }
                else
                {
                    next_posedge_int_clock_state.src_fsm = src_fsm_data;
                    if (sources[src].ready_to_clock)
                    {
                        sl_data_stream_next_data( sources[src].data_stream );
                        sources[src].ready_to_clock = 0;
                    }
                    next_posedge_int_clock_state.src_data = sl_data_stream_packet_data(sources[src].data_stream);
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
                        sl_data_stream_next_data( sources[src].data_stream );
                        sources[src].ready_to_clock = 0;
                    }
                    next_posedge_int_clock_state.src_data = sl_data_stream_packet_data(sources[src].data_stream);
                }
                else
                {
                    next_posedge_int_clock_state.src_fsm = src_fsm_data;
                    if (sources[src].ready_to_clock)
                    {
                        sl_data_stream_next_data( sources[src].data_stream );
                        sources[src].ready_to_clock = 0;
                    }
                    next_posedge_int_clock_state.src_data = sl_data_stream_packet_data(sources[src].data_stream);
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
        }
    }

    /*b Done
     */
    return error_level_okay;
}

/*f c_mii_testbench::clock_posedge_int_clock
*/
t_sl_error_level c_mii_testbench::clock_posedge_int_clock( void )
{
    int i;
    memcpy( &posedge_int_clock_state, &next_posedge_int_clock_state, sizeof(posedge_int_clock_state) );
    for (i=0; i<MAX_SOURCES; i++)
    {
        sources[i].ready_to_clock = 1;
    }

    /*b Monitor the bus with a message if required
    */
    if (monitor_level&mon_level_verbose)
    {
        char buffer[256];
        sprintf( buffer, "Type %d ack %d data %08x", inputs.mon_type[0], inputs.mon_ack[0], inputs.mon_data[0] );
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
/*f c_mii_testbench__init
*/
extern void c_mii_testbench__init( void )
{
    se_external_module_register( 1, "mii_testbench", mii_testbench_instance_fn );
}

/*a Scripting support code
*/
/*f initmii_testbench
*/
extern "C" void initmii_testbench( void )
{
    c_mii_testbench__init( );
    scripting_init_module( "mii_testbench" );
}
