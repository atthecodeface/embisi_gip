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
#include "sl_exec_file.h"

/*a Defines
 */
#define MAX_FIFO_LENGTH (256)

#define ASSIGN_TO_BIT(vector,size,bit,value) se_cmodel_assist_assign_to_bit(vector,size,bit,value)
#define ASSIGN_TO_BIT_RANGE(vector,size,bit,length,value) se_cmodel_assist_assign_to_bit_range(vector,size,bit,length,value)
#define COVER_STATEMENT(stmt_number) {if(stmt_number>=0)se_cmodel_assist_coverage_statement_reached(coverage,stmt_number);}
#define ASSERT(expr,string) {if (!(expr)) {\
    engine->message->add_error( NULL, error_level_info, error_number_se_dated_assertion, error_id_sl_exec_file_allocate_and_read_exec_file, \
        error_arg_type_integer, engine->cycle(),\
        error_arg_type_malloc_string, engine->get_instance_name(engine_handle),  \
        error_arg_type_const_string, string,\
        error_arg_type_none );}}

#define FAIL(expr,string) {if ((expr)) {\
    engine->message->add_error( NULL, error_level_info, error_number_se_dated_assertion, error_id_sl_exec_file_allocate_and_read_exec_file, \
        error_arg_type_integer, engine->cycle(),\
        error_arg_type_malloc_string, engine->get_instance_name(engine_handle),  \
        error_arg_type_const_string, string,\
        error_arg_type_none );}}

//    next_posedge_int_clock_state.mon_failure = ~posedge_int_clock_state.mon_failure;


/*a Types for c_uart_testbench
*/
/*t cmd_*
 */
enum
{
     cmd_uart_tx_string=sl_exec_file_cmd_first_external,
     cmd_uart_tx_line,
     cmd_uart_tx_byte,
     cmd_wait,
     cmd_wait_until_tx_nf,
     cmd_wait_until_rx_ne,
     cmd_wait_until_rx_has_string,
     cmd_wait_until_rx_has_line
};

/*t fn_*
 */
enum
{
    fn_rx_status=sl_exec_file_fn_first_internal,
    fn_rx_byte,
    fn_rx_string,
    fn_rx_line
};

/*t t_event_flag
 */
typedef enum 
{
    event_flag_none,
    event_flag_tx_nf,
    event_flag_rx_ne,
    event_flag_rx_has_string,
    event_flag_rx_has_line,
} t_event_flag;

/*t t_uart_event_data
 */
typedef struct t_uart_event_data
{
    struct t_uart_event_data *next_in_list;
    t_sl_exec_file_event_ptr event;
    t_event_flag flag;
    int value;
    int timeout;
    int fired;
} t_uart_event_data;

/*t t_tx_fsm - UART transmitter
*/
typedef enum t_tx_fsm
{
    tx_fsm_idle,
    tx_fsm_start_bits,
    tx_fsm_data_bits,
    tx_fsm_stop_bits,
};

/*t t_rx_fsm - RX FSM
*/
typedef enum t_rx_fsm
{
    rx_fsm_idle,
    rx_fsm_start_bits,
    rx_fsm_data_bits,
    rx_fsm_stop_bits,
};

/*t mon_level_*
 */
enum
{
    mon_level_none = 0,
    mon_level_verbose = 1, // Output a message on every clock with the signals on the bus
    mon_level_data = 4, // Check data according to type given in the transaction word (if length is nonzero)
};

/*t t_uart_byte
 */
typedef struct t_uart_byte
{
    int start_bits;
    int stop_bits;
    int data_bits;
    unsigned int data;
} t_uart_byte;

/*t t_uart_posedge_int_clock_state
*/
typedef struct t_uart_posedge_int_clock_state
{
    t_tx_fsm tx_fsm;
    t_uart_byte tx_data;
    int tx_sub_bit;
    int tx_bit;
    int txd;
    int tx_baud_counter;
    int tx_get_data;

    t_rx_fsm rx_fsm;
    t_uart_byte rx_data;
    int rx_sub_bit;
    int rx_bit;
    int rxd;
    int rx_baud_counter;
    int expected_rxd;
    int rx_data_complete;

} t_uart_posedge_int_clock_state;

/*t t_uart_inputs
*/
typedef struct t_uart_inputs
{
    unsigned int *uart_reset;

    unsigned int *rxd;

} t_uart_inputs;

/*t t_uart_combinatorials
*/
typedef struct t_uart_combinatorials
{
} t_uart_combinatorials;

/*t c_uart_testbench
*/
class c_uart_testbench
{
public:
    c_uart_testbench::c_uart_testbench( class c_engine *eng, void *eng_handle );
    c_uart_testbench::~c_uart_testbench();
    t_sl_error_level c_uart_testbench::delete_instance( void );
    t_sl_error_level c_uart_testbench::reset( int pass );
    t_sl_error_level c_uart_testbench::evaluate_combinatorials( void );
    t_sl_error_level c_uart_testbench::preclock_posedge_int_clock( void );
    t_sl_error_level c_uart_testbench::clock_posedge_int_clock( void );
    t_sl_error_level c_uart_testbench::reset_active_high_int_reset( void );
    void c_uart_testbench::exec_file_instantiate_callback( struct t_sl_exec_file_data *file_data );

    void c_uart_testbench::tx_byte( unsigned char byte );
    int c_uart_testbench::rx_has_string( void );
    int c_uart_testbench::rx_has_line( void );
    int c_uart_testbench::rx_get_string( char *buffer );
    int c_uart_testbench::rx_get_line( char *buffer );
    int c_uart_testbench::pop_rx_byte( t_uart_byte *byte );
private:
    void c_uart_testbench::run_exec_file( void );
    t_uart_event_data *c_uart_testbench::uart_add_event( const char *reason, t_event_flag event_flag, int value, int timeout );

    c_engine *engine;
    void *engine_handle;
    struct t_sl_exec_file_data *exec_file_data;
    char *exec_file_filename;

    int cycle_number;

    t_uart_inputs inputs;
    t_uart_combinatorials combinatorials;
    t_uart_posedge_int_clock_state next_posedge_int_clock_state;
    t_uart_posedge_int_clock_state posedge_int_clock_state;

    int monitor_level;

    int rx_baud_subtract;
    int rx_baud_add;
    int rx_bytes;
    t_uart_byte rx_fifo[MAX_FIFO_LENGTH];

    int tx_baud_subtract;
    int tx_baud_add;
    int tx_bytes;
    t_uart_byte tx_fifo[MAX_FIFO_LENGTH];

    t_uart_event_data *event_list;

};

/*a Static function declarations
 */
static t_sl_exec_file_eval_fn ef_fn_eval_rx_status;
static t_sl_exec_file_eval_fn ef_fn_eval_rx_byte;
static t_sl_exec_file_eval_fn ef_fn_eval_rx_string;
static t_sl_exec_file_eval_fn ef_fn_eval_rx_line;

/*a Statics
 */
/*v uart_file_cmds
 */
static t_sl_exec_file_cmd uart_file_cmds[] =
{
     {cmd_uart_tx_string,           1, "uart_tx_string", "s", "uart_tx_string <string> - Output a NUL-terminated string"},
     {cmd_uart_tx_line,             1, "uart_tx_line", "s", "uart_tx_line <string> - Output a newline-terminated string"},
     {cmd_uart_tx_byte,             1, "uart_tx_byte", "i", "uart_tx_byte <byte> - Output a specified byte"},
     {cmd_wait,                     1, "wait",  "i", "wait <number of cycles>"},
     {cmd_wait_until_tx_nf,         0, "wait_until_tx_nf",  "", "wait_until_tx_nf"},
     {cmd_wait_until_rx_ne,         0, "wait_until_rx_ne",  "", "wait_until_rx_ne"},
     {cmd_wait_until_rx_has_string, 0, "wait_until_rx_has_string",  "", "wait_until_rx_has_string - wait until a NUL is received"},
     {cmd_wait_until_rx_has_line,   0, "wait_until_rx_has_line",  "", "wait_until_rx_has_line - wait until a newline is received"},
     {sl_exec_file_cmd_none,        0, NULL, NULL, NULL }
};

/*v uart_file_fns
 */
static t_sl_exec_file_fn uart_file_fns[] =
{
     {fn_rx_status,               "rx_status",         'i', "", "rx_status()", ef_fn_eval_rx_status },
     {fn_rx_byte,                 "rx_byte",           'i', "", "rx_byte()", ef_fn_eval_rx_byte },
     {fn_rx_string,               "rx_string",         's', "", "rx_string()", ef_fn_eval_rx_string },
     {fn_rx_line,                 "rx_line",           's', "", "rx_line()", ef_fn_eval_rx_line },
     {sl_exec_file_fn_none, NULL,     0,   NULL, NULL },
};

/*f state_desc_uart_testbench
*/
#define struct_offset( ptr, a ) (((char *)&(ptr->a))-(char *)ptr)
static t_uart_posedge_int_clock_state *___uart_posedge_int_clock__ptr;
static t_engine_state_desc state_desc_uart_testbench_posedge_int_clock[] =
{
//    {"src_fsm", engine_state_desc_type_bits, NULL, struct_offset(___uart_posedge_int_clock__ptr, src_fsm), {2,0,0,0}, {NULL,NULL,NULL,NULL} },
//    {"src_left", engine_state_desc_type_bits, NULL, struct_offset(___uart_posedge_int_clock__ptr, src_left), {5,0,0,0}, {NULL,NULL,NULL,NULL} },
//    {"fc_credit", engine_state_desc_type_bits, NULL, struct_offset(___uart_posedge_int_clock__ptr, src_channel_state[0].fc_credit), {32,0,0,0}, {NULL,NULL,NULL,NULL} },
    {"", engine_state_desc_type_none, NULL, 0, {0,0,0,0}, {NULL,NULL,NULL,NULL} }
};

/*a Static wrapper functions for uart
*/
/*f uart_instance_fn
*/
static t_sl_error_level uart_instance_fn( c_engine *engine, void *engine_handle )
{
    c_uart_testbench *mod;
    mod = new c_uart_testbench( engine, engine_handle );
    if (!mod)
        return error_level_fatal;
    return error_level_okay;
}

/*f uart_delete_fn - simple callback wrapper for the main method
*/
static t_sl_error_level uart_delete_fn( void *handle )
{
    c_uart_testbench *mod;
    t_sl_error_level result;
    mod = (c_uart_testbench *)handle;
    result = mod->delete_instance();
    delete( mod );
    return result;
}

/*f uart_reset_fn
*/
static t_sl_error_level uart_reset_fn( void *handle, int pass )
{
    c_uart_testbench *mod;
    mod = (c_uart_testbench *)handle;
    return mod->reset( pass );
}

/*f uart_combinatorial_fn
*/
static t_sl_error_level uart_combinatorial_fn( void *handle )
{
    c_uart_testbench *mod;
    mod = (c_uart_testbench *)handle;
    return mod->evaluate_combinatorials();
}

/*f uart_preclock_posedge_int_clock_fn
*/
static t_sl_error_level uart_preclock_posedge_int_clock_fn( void *handle )
{
    c_uart_testbench *mod;
    mod = (c_uart_testbench *)handle;
    return mod->preclock_posedge_int_clock();
}

/*f uart_clock_posedge_int_clock_fn
*/
static t_sl_error_level uart_clock_posedge_int_clock_fn( void *handle )
{
    c_uart_testbench *mod;
    mod = (c_uart_testbench *)handle;
    return mod->clock_posedge_int_clock();
}

/*a Exec file functions
 */
/*f ef_fn_eval_rx_status
 */
static int ef_fn_eval_rx_status( void *handle, t_sl_exec_file_data *file_data, t_sl_exec_file_value *args )
{
    return sl_exec_file_eval_fn_set_result( file_data, 0 );
}

/*f ef_fn_eval_rx_byte
 */
static int ef_fn_eval_rx_byte( void *handle, t_sl_exec_file_data *file_data, t_sl_exec_file_value *args )
{
    int valid;
    t_uart_byte rx_byte;
    c_uart_testbench *cif;
    cif = (c_uart_testbench *)handle;

    valid = cif->pop_rx_byte( &rx_byte );
    if (!valid)
        return 0;
    return sl_exec_file_eval_fn_set_result( file_data, (int)rx_byte.data );
}

/*f ef_fn_eval_rx_string
 */
static int ef_fn_eval_rx_string( void *handle, t_sl_exec_file_data *file_data, t_sl_exec_file_value *args )
{
    char buffer[MAX_FIFO_LENGTH+1];
    int valid;
    c_uart_testbench *cif;
    cif = (c_uart_testbench *)handle;

    valid = cif->rx_get_string( buffer );
    if (!valid)
        return 0;

    return sl_exec_file_eval_fn_set_result( file_data, buffer, 1 );
}

/*f ef_fn_eval_rx_line
 */
static int ef_fn_eval_rx_line( void *handle, t_sl_exec_file_data *file_data, t_sl_exec_file_value *args )
{
    char buffer[MAX_FIFO_LENGTH+1];
    int valid;
    c_uart_testbench *cif;
    cif = (c_uart_testbench *)handle;

    valid = cif->rx_get_line( buffer );
    if (!valid)
        return 0;

    return sl_exec_file_eval_fn_set_result( file_data, buffer, 1 );
}

/*f internal_exec_file_instantiate_callback
 */
static void internal_exec_file_instantiate_callback( void *handle, struct t_sl_exec_file_data *file_data )
{
     c_uart_testbench *cif;
     cif = (c_uart_testbench *)handle;
     cif->exec_file_instantiate_callback( file_data );
}

/*f c_uart_testbench::exec_file_instantiate_callback
 */
void c_uart_testbench::exec_file_instantiate_callback( struct t_sl_exec_file_data *file_data )
{

     sl_exec_file_set_environment_interrogation( file_data, (t_sl_get_environment_fn)sl_option_get_string, (void *)engine->get_option_list( engine_handle ) );

     engine->add_simulation_exec_file_enhancements( file_data );
     engine->waveform_add_exec_file_enhancements( file_data );
     engine->register_add_exec_file_enhancements( file_data, engine_handle );
}

/*f c_uart_testbench::uart_add_event
 */
t_uart_event_data *c_uart_testbench::uart_add_event( const char *reason, t_event_flag event_flag, int value, int timeout )
{
     t_uart_event_data *event;
     char name[1024];

     event = (t_uart_event_data *)malloc(sizeof(t_uart_event_data));
     if (!event)
          return NULL;

     snprintf( name, sizeof(name), "__%s__%s__%d", sl_exec_file_command_threadname( exec_file_data ), reason, cycle_number );
     name[sizeof(name)-1] = 0;
     event->event = sl_exec_file_event_create( exec_file_data, name );
     sl_exec_file_event_reset( exec_file_data, event->event );

     event->flag = event_flag;
     event->value = value;
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

/*f c_uart_testbench::run_exec_file
 */
void c_uart_testbench::run_exec_file( void )
{
     int cmd, cmd_type;
     t_sl_exec_file_value *args;
     int i;
     t_uart_event_data *event;

     if (exec_file_data)
     {
          while (1)
          {
               cmd_type = sl_exec_file_get_next_cmd( exec_file_data, &cmd, &args );
               if ( (cmd_type==0) || (cmd_type==2) ) // Break if nothing left to do
               {
                    break;
               }
               if (cmd_type==1) // If given command, handle that
               {
                    if (engine->simulation_handle_exec_file_command( exec_file_data, cmd, args ))
                    {
                    }
                    else if (engine->waveform_handle_exec_file_command( exec_file_data, cmd, args ))
                    {
                    }
                    else
                    {
                         switch (cmd)
                         {
                         case cmd_uart_tx_string:
                             for (i=0; args[0].p.string[i]; i++)
                             {
                                 tx_byte( args[0].p.string[i] );
                             }
                             tx_byte( 0 );
                             break;
                         case cmd_uart_tx_line:
                             for (i=0; args[0].p.string[i]; i++)
                             {
                                 tx_byte( args[0].p.string[i] );
                             }
                             tx_byte( 10 );
                             break;
                         case cmd_uart_tx_byte:
                             tx_byte( args[0].integer );
                             break;
                         case cmd_wait:
                             if ( (event = uart_add_event( "wait", event_flag_none, 0, args[0].integer )) != NULL )
                             {
                                 fprintf(stderr,"Waiting for time %d\n", args[0].integer);
                                 sl_exec_file_wait_for_event( exec_file_data, NULL, event->event );
                             }
                             break;
                         case cmd_wait_until_rx_ne:
                             if ( (event = uart_add_event( "wait_until_rx_ne", event_flag_rx_ne, 1, -1 )) != NULL )
                             {
                                 sl_exec_file_wait_for_event( exec_file_data, NULL, event->event );
                             }
                             break;
                         case cmd_wait_until_rx_has_string:
                             if ( (event = uart_add_event( "wait_until_rx_has_string", event_flag_rx_has_string, 1, -1 )) != NULL )
                             {
                                 sl_exec_file_wait_for_event( exec_file_data, NULL, event->event );
                             }
                             break;
                         case cmd_wait_until_rx_has_line:
                             if ( (event = uart_add_event( "wait_until_rx_has_line", event_flag_rx_has_line, 1, -1 )) != NULL )
                             {
                                 sl_exec_file_wait_for_event( exec_file_data, NULL, event->event );
                             }
                             break;
                         }
                    }
               }
          }
     }
}

/*a UART Tx, Rx and status functions
 */
/*f c_uart_testbench::tx_byte
 */    
void c_uart_testbench::tx_byte( unsigned char byte )
{

    if (tx_bytes==MAX_FIFO_LENGTH)
    {
        fprintf(stderr,"c_uart_testbench::overflow in transmit\n");
        return;
    }
    tx_fifo[tx_bytes].start_bits = 1;
    tx_fifo[tx_bytes].stop_bits = 1;
    tx_fifo[tx_bytes].data_bits = 8;
    tx_fifo[tx_bytes].data = byte;
    tx_bytes++;
}

/*f c_uart_testbench::rx_has_string
 */
int c_uart_testbench::rx_has_string( void )
{
    int i;
    for (i=0; i<rx_bytes; i++)
    {
        if (rx_fifo[i].data==0)
        {
            return 1;
        }
    }
    return 0;
}

/*f c_uart_testbench::rx_get_string
 */
int c_uart_testbench::rx_get_string( char *buffer )
{
    int i, j;
    for (i=0; i<rx_bytes; i++)
    {
        buffer[i] = rx_fifo[i].data;
        if (rx_fifo[i].data==0)
        {
            for (j=i+1; j<MAX_FIFO_LENGTH; j++)
            {
                rx_fifo[j-i] = rx_fifo[j];
            }
            rx_bytes -= i+1;
            return 1;
        }
    }
    return 0;
}

/*f c_uart_testbench::rx_has_line
 */
int c_uart_testbench::rx_has_line( void )
{
    int i;
    for (i=0; i<rx_bytes; i++)
    {
        if (rx_fifo[i].data==10)
        {
            return 1;
        }
    }
    return 0;
}

/*f c_uart_testbench::rx_get_line
 */
int c_uart_testbench::rx_get_line( char *buffer )
{
    int i, j;
    for (i=0; i<rx_bytes; i++)
    {
        buffer[i] = rx_fifo[i].data;
        if (rx_fifo[i].data==10)
        {
            buffer[i] = 0;
            for (j=i+1; j<MAX_FIFO_LENGTH; j++)
            {
                rx_fifo[j-i] = rx_fifo[j];
            }
            rx_bytes -= i+1;
            return 1;
        }
    }
    return 0;
}

/*f c_uart_testbench::pop_rx_byte
 */
int c_uart_testbench::pop_rx_byte( t_uart_byte *byte )
{
    int i;
    if (rx_bytes<0)
    {
        return 0;
    }
    *byte = rx_fifo[0];
    for (i=0; i<MAX_FIFO_LENGTH-1; i++)
    {
        rx_fifo[i] = rx_fifo[i+1];
    }
    return 1;
}

/*a Constructors and destructors for uart
*/
/*f c_uart_testbench::c_uart_testbench
*/
c_uart_testbench::c_uart_testbench( class c_engine *eng, void *eng_handle )
{

    /*b Set main variables
     */
    engine = eng;
    engine_handle = eng_handle;
    event_list = NULL;

    exec_file_filename = engine->get_option_string( engine_handle, "exec_file", NULL );
    if (exec_file_filename)
    {
        exec_file_filename = sl_str_alloc_copy( exec_file_filename );
    }
    tx_baud_subtract = engine->get_option_int( engine_handle, "tx_baud_subtract", 0 );
    tx_baud_add = engine->get_option_int( engine_handle, "tx_baud_add", 0 );
    rx_baud_subtract = engine->get_option_int( engine_handle, "rx_baud_subtract", 0 );
    rx_baud_add = engine->get_option_int( engine_handle, "rx_baud_add", 0 );
    rx_bytes = 0;
    tx_bytes = 0;

    /*b Determine monitor options
     */
    monitor_level = engine->get_option_int( engine_handle, "monitor_level", 0 );

    /*b Instantiate module
     */
    engine->register_delete_function( engine_handle, (void *)this, uart_delete_fn );
    engine->register_reset_function( engine_handle, (void *)this, uart_reset_fn );

    engine->register_comb_fn( engine_handle, (void *)this, uart_combinatorial_fn );
    engine->register_clock_fns( engine_handle, (void *)this, "uart_clock", uart_preclock_posedge_int_clock_fn, uart_clock_posedge_int_clock_fn );

    engine->register_input_signal( engine_handle, "uart_reset", 1, (int **)&inputs.uart_reset );

    engine->register_input_signal( engine_handle, "rxd", 1, (int **)&inputs.rxd );
    engine->register_input_used_on_clock( engine_handle, "rxd", "uart_clock", 1 );

    engine->register_output_signal( engine_handle, "txd", 1, (int *)&posedge_int_clock_state.txd );
    engine->register_output_generated_on_clock( engine_handle, "txd", "uart_clock", 1 );

    /*b Register state then reset
     */
    engine->register_state_desc( engine_handle, 1, state_desc_uart_testbench_posedge_int_clock, &posedge_int_clock_state, NULL );
    cycle_number = 0;
    reset_active_high_int_reset();
    if (exec_file_filename)
    {
        sl_exec_file_allocate_and_read_exec_file( engine->error, engine->message, internal_exec_file_instantiate_callback, (void *)this, "exec_file", exec_file_filename, &exec_file_data, "Test harness", uart_file_cmds, uart_file_fns );
    }

    /*b Done
     */

}

/*f c_uart_testbench::~c_uart_testbench
*/
c_uart_testbench::~c_uart_testbench()
{
    delete_instance();
}

/*f c_uart_testbench::delete_instance
*/
t_sl_error_level c_uart_testbench::delete_instance( void )
{
    return error_level_okay;
}

/*a Class reset/preclock/clock methods for uart
*/
/*f c_uart_testbench::reset
*/
t_sl_error_level c_uart_testbench::reset( int pass )
{
    if (pass==0)
    {
        if (exec_file_data)
        {
            sl_exec_file_reset( exec_file_data );
        }
        cycle_number = -1;
        reset_active_high_int_reset();
        run_exec_file();
        cycle_number = 0;
    }
    else
    {
        evaluate_combinatorials();
    }
    return error_level_okay;
}

/*f c_uart_testbench::reset_active_high_int_reset
*/
t_sl_error_level c_uart_testbench::reset_active_high_int_reset( void )
{
    posedge_int_clock_state.tx_fsm = tx_fsm_idle;
    posedge_int_clock_state.tx_sub_bit = 0;
    posedge_int_clock_state.tx_bit = 0;
    posedge_int_clock_state.txd = 1;
    posedge_int_clock_state.tx_baud_counter = 0;
    posedge_int_clock_state.tx_get_data = 0;

    posedge_int_clock_state.rx_fsm = rx_fsm_idle;
    posedge_int_clock_state.rx_bit = 0;
    posedge_int_clock_state.rxd = 0;
    posedge_int_clock_state.rx_baud_counter = 0;
    posedge_int_clock_state.expected_rxd = 0;
    posedge_int_clock_state.rx_data_complete = 0;

    tx_bytes = 0;
    rx_bytes = 0;

    return error_level_okay;
}

/*f c_uart_testbench::evaluate_combinatorials
*/
t_sl_error_level c_uart_testbench::evaluate_combinatorials( void )
{
    if (inputs.uart_reset[0]==1)
    {
        reset_active_high_int_reset();
    }

    /*b Done
     */
    return error_level_okay;
}

/*f c_uart_testbench::preclock_posedge_int_clock
*/
t_sl_error_level c_uart_testbench::preclock_posedge_int_clock( void )
{
    int rxd;

    /*b Copy current state to next state
     */
    memcpy( &next_posedge_int_clock_state, &posedge_int_clock_state, sizeof(posedge_int_clock_state) );

    /*b Handle rx
     */
    next_posedge_int_clock_state.rx_data_complete = 0;
    if (posedge_int_clock_state.rx_baud_counter>=0)
    {
        next_posedge_int_clock_state.rx_baud_counter -= rx_baud_subtract;
        rxd = inputs.rxd[0];
        switch (posedge_int_clock_state.rx_fsm)
        {
        case rx_fsm_idle:
            if (rxd==0)
            {
                next_posedge_int_clock_state.rx_fsm = rx_fsm_start_bits;
                next_posedge_int_clock_state.rx_sub_bit = 0;
                next_posedge_int_clock_state.rx_bit = 0;
                next_posedge_int_clock_state.rx_data.start_bits = 1;
                next_posedge_int_clock_state.rx_data.data_bits = 8;
                next_posedge_int_clock_state.rx_data.stop_bits = 1;
                next_posedge_int_clock_state.rx_data.data = 0;
            }
            break;
        case rx_fsm_start_bits:
            next_posedge_int_clock_state.rx_sub_bit = posedge_int_clock_state.rx_sub_bit+1;
            if (posedge_int_clock_state.rx_sub_bit==15)
            {
                next_posedge_int_clock_state.rx_sub_bit = 0;
                next_posedge_int_clock_state.rx_bit = posedge_int_clock_state.rx_bit+1;
                if (posedge_int_clock_state.rx_bit==posedge_int_clock_state.rx_data.start_bits-1)
                {
                    next_posedge_int_clock_state.rx_fsm = rx_fsm_data_bits;
                    next_posedge_int_clock_state.rx_bit = 0;
                }
            }
            if ((posedge_int_clock_state.rx_sub_bit>5) && (posedge_int_clock_state.rx_sub_bit<10))
            {
                if (rxd!=0)
                {
                    FAIL(1, "Framing error in received data - start bit not stable" );
                    next_posedge_int_clock_state.rx_fsm = rx_fsm_idle;
                }
            }
            break;
        case rx_fsm_data_bits:
            next_posedge_int_clock_state.rx_sub_bit = posedge_int_clock_state.rx_sub_bit+1;
            if (posedge_int_clock_state.rx_sub_bit==15)
            {
                next_posedge_int_clock_state.rx_sub_bit = 0;
                next_posedge_int_clock_state.rx_bit = posedge_int_clock_state.rx_bit+1;
                if (posedge_int_clock_state.rx_bit==posedge_int_clock_state.rx_data.data_bits-1)
                {
                    next_posedge_int_clock_state.rx_fsm = rx_fsm_stop_bits;
                    next_posedge_int_clock_state.rx_bit = 0;
                }
            }
            if (posedge_int_clock_state.rx_sub_bit==5)
            {
                next_posedge_int_clock_state.rx_data.data = (posedge_int_clock_state.rx_data.data>>1) | (rxd<<posedge_int_clock_state.rx_data.data_bits-1);
                next_posedge_int_clock_state.expected_rxd = rxd;
            }
            if ((posedge_int_clock_state.rx_sub_bit>5) && (posedge_int_clock_state.rx_sub_bit<10))
            {
                if (rxd!=posedge_int_clock_state.expected_rxd)
                {
                    FAIL(1, "Framing error in received data - data bit not stable" );
                    next_posedge_int_clock_state.rx_fsm = rx_fsm_idle;
                }
            }
            break;
        case rx_fsm_stop_bits:
            next_posedge_int_clock_state.rx_sub_bit = posedge_int_clock_state.rx_sub_bit+1;
            if (posedge_int_clock_state.rx_sub_bit==15)
            {
                next_posedge_int_clock_state.rx_sub_bit = 0;
                next_posedge_int_clock_state.rx_bit = posedge_int_clock_state.rx_bit+1;
                if (posedge_int_clock_state.rx_bit==posedge_int_clock_state.rx_data.stop_bits-1)
                {
                    next_posedge_int_clock_state.rx_fsm = rx_fsm_idle;
                    next_posedge_int_clock_state.rx_data_complete = 1;
                }
            }
            if ((posedge_int_clock_state.rx_sub_bit>5) && (posedge_int_clock_state.rx_sub_bit<10))
            {
                if (rxd!=1)
                {
                    FAIL(1, "Framing error in received data - stop bit not stable" );
                    next_posedge_int_clock_state.rx_fsm = rx_fsm_idle;
                }
            }
            break;
        }
    }
    else
    {
        next_posedge_int_clock_state.rx_baud_counter += rx_baud_add;
    }

    /*b Handle tx
     */
    next_posedge_int_clock_state.tx_get_data = 0;
    if (posedge_int_clock_state.tx_baud_counter>=0)
    {
        next_posedge_int_clock_state.tx_baud_counter -= tx_baud_subtract;
        switch (posedge_int_clock_state.tx_fsm)
        {
        case tx_fsm_idle:
            next_posedge_int_clock_state.txd = 1;
            if (tx_bytes>0)
            {
                next_posedge_int_clock_state.tx_fsm = tx_fsm_start_bits;
                next_posedge_int_clock_state.tx_sub_bit = 0;
                next_posedge_int_clock_state.tx_bit = 0;
                next_posedge_int_clock_state.tx_get_data = 1;
            }
            break;
        case tx_fsm_start_bits:
            next_posedge_int_clock_state.txd = 0;
            next_posedge_int_clock_state.tx_sub_bit = posedge_int_clock_state.tx_sub_bit+1;
            if (posedge_int_clock_state.tx_sub_bit==15)
            {
                next_posedge_int_clock_state.tx_sub_bit = 0;
                next_posedge_int_clock_state.tx_bit = posedge_int_clock_state.tx_bit+1;
                if (posedge_int_clock_state.tx_bit==posedge_int_clock_state.tx_data.start_bits-1)
                {
                    next_posedge_int_clock_state.tx_fsm = tx_fsm_data_bits;
                    next_posedge_int_clock_state.tx_bit = 0;
                }
            }
            break;
        case tx_fsm_data_bits:
            next_posedge_int_clock_state.txd = posedge_int_clock_state.tx_data.data&1;
            next_posedge_int_clock_state.tx_sub_bit = posedge_int_clock_state.tx_sub_bit+1;
            if (posedge_int_clock_state.tx_sub_bit==15)
            {
                next_posedge_int_clock_state.tx_sub_bit = 0;
                next_posedge_int_clock_state.tx_bit = posedge_int_clock_state.tx_bit+1;
                next_posedge_int_clock_state.tx_data.data = posedge_int_clock_state.tx_data.data>>1;
                if (posedge_int_clock_state.tx_bit==posedge_int_clock_state.tx_data.data_bits-1)
                {
                    next_posedge_int_clock_state.tx_fsm = tx_fsm_stop_bits;
                    next_posedge_int_clock_state.tx_bit = 0;
                }
            }
            break;
        case tx_fsm_stop_bits:
            next_posedge_int_clock_state.txd = 1;
            next_posedge_int_clock_state.tx_sub_bit = posedge_int_clock_state.tx_sub_bit+1;
            if (posedge_int_clock_state.tx_sub_bit==15)
            {
                next_posedge_int_clock_state.tx_sub_bit = 0;
                next_posedge_int_clock_state.tx_bit = posedge_int_clock_state.tx_bit+1;
                if (posedge_int_clock_state.tx_bit==posedge_int_clock_state.tx_data.stop_bits-1)
                {
                    next_posedge_int_clock_state.tx_fsm = tx_fsm_idle;
                }
            }
            break;
        }
    }
    else
    {
        next_posedge_int_clock_state.tx_baud_counter += tx_baud_add;
    }

    /*b Done
     */
    return error_level_okay;
}

/*f c_uart_testbench::clock_posedge_int_clock
*/
t_sl_error_level c_uart_testbench::clock_posedge_int_clock( void )
{
    t_uart_event_data *event, **last_event_ptr;
    char buffer[256];

    /*b Copy next state to current
    */
    memcpy( &posedge_int_clock_state, &next_posedge_int_clock_state, sizeof(posedge_int_clock_state) );
    cycle_number++;

    /*b Receive any byte required
     */
    if (posedge_int_clock_state.rx_data_complete)
    {
        if (rx_bytes<MAX_FIFO_LENGTH)
        {
            rx_fifo[rx_bytes] = posedge_int_clock_state.rx_data;
            rx_bytes++;
            fprintf(stderr, "Uart received byte %02x\n", posedge_int_clock_state.rx_data.data);
        }
        else
        {
            fprintf(stderr, "Uart receive FIFO overrun\n");
        }
    }

    /*b Prepare to tx any byte required
     */
    if (posedge_int_clock_state.tx_get_data && (tx_bytes>0))
    {
        int i;
        posedge_int_clock_state.tx_data = tx_fifo[0];
        fprintf(stderr, "Uart start to transmit byte %02x\n", posedge_int_clock_state.tx_data.data);
        for (i=0; i<MAX_FIFO_LENGTH-1; i++)
        {
            tx_fifo[i] = tx_fifo[i+1];
        }
        tx_bytes--;
    }

    /*b Fire any events that should, er, fire
     */
    for (event=event_list; event; event=event->next_in_list)
    {
        //fprintf(stderr,"Preclocking event %p/%d\n", event, event->fired );
        if (!event->fired)
        {
            int v;
            if  ( (event->timeout!=-1) && (event->timeout==cycle_number) )
            {
                event->fired = 1;
            }
            //fprintf(stderr,"Fifo %d/%d flag %d value %d timeout %d\n", event->fifo, event->fifo_number, event->flag, event->value, event->timeout );
            switch (event->flag)
            {
            case event_flag_none:          v=-1; break;
            case event_flag_rx_has_string: v=rx_has_string(); break;
            case event_flag_rx_has_line:   v=rx_has_line(); break;
            case event_flag_rx_ne:         v=(rx_bytes!=0); break;
            case event_flag_tx_nf:         v=(tx_bytes!=MAX_FIFO_LENGTH); break;
            }
            if (v==event->value)
            {
                event->fired = 1;
            }
            if ( event->fired )
            {
                event->timeout = cycle_number; // record the time at which it fired
                if (event->event)
                {
                    sl_exec_file_event_fire( exec_file_data, event->event );
                }
            }
        }
    }

    /*b Run exec file
     */
    if (exec_file_data)
    {
        run_exec_file();
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
                sl_exec_file_event_free( exec_file_data, event->event );
            }
            *last_event_ptr = event->next_in_list;
            free(event);
        }
        else
        {
            last_event_ptr = &event->next_in_list;
        }
     }

    /*b Monitor the bus with a message if required
    */
    if (monitor_level&mon_level_verbose)
    {
        sprintf( buffer, "RxD %d TxD %d bc %d txfsm %d bytes %d bit %d sb %d", inputs.rxd[0], posedge_int_clock_state.txd, posedge_int_clock_state.tx_baud_counter, posedge_int_clock_state.tx_fsm, tx_bytes, posedge_int_clock_state.tx_bit, posedge_int_clock_state.tx_sub_bit );
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
/*f c_uart_testbench__init
*/
extern void c_uart_testbench__init( void )
{
    se_external_module_register( 1, "uart_testbench", uart_instance_fn );
}

/*a Scripting support code
*/
/*f inituart_testbench
*/
extern "C" void inituart_testbench( void )
{
    c_uart_testbench__init( );
    scripting_init_module( "uart_testbench" );
}
