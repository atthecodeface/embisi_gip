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
    next_posedge_int_clock_state.mon_failure = ~posedge_int_clock_state.mon_failure; \
    engine->message->add_error( NULL, error_level_info, error_number_se_dated_assertion, error_id_sl_exec_file_allocate_and_read_exec_file, \
        error_arg_type_integer, engine->cycle(),\
        error_arg_type_malloc_string, engine->get_instance_name(engine_handle),  \
        error_arg_type_const_string, string,\
        error_arg_type_none );}}


/*a Types for c_uart
*/
/*t cmd_*
 */
enum
{
     cmd_uart_tx_string=sl_exec_file_cmd_first_external,
     cmd_uart_tx_byte,
     cmd_wait,
     cmd_wait_until_tx_nf,
     cmd_wait_until_rx_ne,
     cmd_wait_until_rx_has_string
};

/*t fn_*
 */
enum
{
    fn_rx_status=sl_exec_file_fn_first_internal,
    fn_rx_byte,
    fn_rx_string
};

/*t t_event_flag
 */
typedef enum 
{
    event_flag_none,
    event_flag_tx_nf,
    event_flag_rx_ne,
    event_flag_rx_has_string,
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
    unsigned char data;
} t_uart_byte;

/*t t_uart_posedge_int_clock_state
*/
typedef struct t_uart_posedge_int_clock_state
{
    t_tx_fsm tx_fsm;
    t_uart_byte tx_data;
    int tx_bit;
    int txd;
    unsigned int tx_baud_counter;

    t_rx_fsm rx_fsm;
    t_uart_byte rx_data;
    int rx_bit;
    int rxd;
    unsigned int rx_baud_counter;

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

/*t c_uart
*/
class c_uart
{
public:
    c_uart::c_uart( class c_engine *eng, void *eng_handle );
    c_uart::~c_uart();
    t_sl_error_level c_uart::delete_instance( void );
    t_sl_error_level c_uart::reset( int pass );
    t_sl_error_level c_uart::evaluate_combinatorials( void );
    t_sl_error_level c_uart::preclock_posedge_int_clock( void );
    t_sl_error_level c_uart::clock_posedge_int_clock( void );
    t_sl_error_level c_uart::reset_active_high_int_reset( void );
    void c_uart::exec_file_instantiate_callback( struct t_sl_exec_file_data *file_data );

    void c_uart::tx_byte( unsigned char byte );
private:
    void c_uart::run_exec_file( void );
    t_uart_event_data *c_uart::uart_add_event( const char *reason, t_event_flag event_flag, int value, int timeout );
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

/*a Statics
 */
/*v uart_file_cmds
 */
static t_sl_exec_file_cmd uart_file_cmds[] =
{
     {cmd_uart_tx_string,           1, "uart_tx_string", "s", "uart_tx_string <string> - Output a null-terminated string"},
     {cmd_uart_tx_byte,             1, "uart_tx_byte", "i", "uart_tx_byte <byte> - Output a specified byte"},
     {cmd_wait,                     1, "wait",  "i", "wait <number of cycles>"},
     {cmd_wait_until_tx_nf,         0, "wait_until_tx_nf",  "", "wait_until_tx_nf"},
     {cmd_wait_until_rx_ne,         0, "wait_until_rx_ne",  "", "wait_until_rx_ne"},
     {cmd_wait_until_rx_has_string, 1, "wait_until_rx_has_string",  "", "wait_until_rx_has_string"},
     {sl_exec_file_cmd_none,        0, NULL, NULL, NULL }
};

/*v uart_file_fns
 */
static t_sl_exec_file_fn uart_file_fns[] =
{
     {fn_rx_status,               "rx_status",         'i', "", "rx_status()", ef_fn_eval_rx_status },
     {fn_rx_byte,                 "rx_byte",           'i', "", "rx_byte()", ef_fn_eval_rx_byte },
     {fn_rx_string,               "rx_string",         's', "", "rx_string()", ef_fn_eval_rx_string },
     {sl_exec_file_fn_none, NULL,     0,   NULL, NULL },
};

/*f state_desc_uart
*/
#define struct_offset( ptr, a ) (((char *)&(ptr->a))-(char *)ptr)
static t_uart_posedge_int_clock_state *___uart_posedge_int_clock__ptr;
static t_engine_state_desc state_desc_uart_posedge_int_clock[] =
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
    c_uart *mod;
    mod = new c_uart( engine, engine_handle );
    if (!mod)
        return error_level_fatal;
    return error_level_okay;
}

/*f uart_delete_fn - simple callback wrapper for the main method
*/
static t_sl_error_level uart_delete_fn( void *handle )
{
    c_uart *mod;
    t_sl_error_level result;
    mod = (c_uart *)handle;
    result = mod->delete_instance();
    delete( mod );
    return result;
}

/*f uart_reset_fn
*/
static t_sl_error_level uart_reset_fn( void *handle, int pass )
{
    c_uart *mod;
    mod = (c_uart *)handle;
    return mod->reset( pass );
}

/*f uart_combinatorial_fn
*/
static t_sl_error_level uart_combinatorial_fn( void *handle )
{
    c_uart *mod;
    mod = (c_uart *)handle;
    return mod->evaluate_combinatorials();
}

/*f uart_preclock_posedge_int_clock_fn
*/
static t_sl_error_level uart_preclock_posedge_int_clock_fn( void *handle )
{
    c_uart *mod;
    mod = (c_uart *)handle;
    return mod->preclock_posedge_int_clock();
}

/*f uart_clock_posedge_int_clock_fn
*/
static t_sl_error_level uart_clock_posedge_int_clock_fn( void *handle )
{
    c_uart *mod;
    mod = (c_uart *)handle;
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
    return sl_exec_file_eval_fn_set_result( file_data, 0 );
}

/*f ef_fn_eval_rx_string
 */
static int ef_fn_eval_rx_string( void *handle, t_sl_exec_file_data *file_data, t_sl_exec_file_value *args )
{
    return sl_exec_file_eval_fn_set_result( file_data, 0 );
}

/*f internal_exec_file_instantiate_callback
 */
static void internal_exec_file_instantiate_callback( void *handle, struct t_sl_exec_file_data *file_data )
{
     c_uart *cif;
     cif = (c_uart *)handle;
     cif->exec_file_instantiate_callback( file_data );
}

/*f c_uart::exec_file_instantiate_callback
 */
void c_uart::exec_file_instantiate_callback( struct t_sl_exec_file_data *file_data )
{

     sl_exec_file_set_environment_interrogation( file_data, (t_sl_get_environment_fn)sl_option_get_string, (void *)engine->get_option_list( engine_handle ) );

     engine->add_simulation_exec_file_enhancements( file_data );
     engine->waveform_add_exec_file_enhancements( file_data );
     engine->register_add_exec_file_enhancements( file_data, engine_handle );
}

/*f c_uart::uart_add_event
 */
t_uart_event_data *c_uart::uart_add_event( const char *reason, t_event_flag event_flag, int value, int timeout )
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

/*f c_uart::run_exec_file
 */
void c_uart::run_exec_file( void )
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
                         }
                    }
               }
          }
     }
}

/*a UART Tx, Rx and status functions
 */
/*f c_uart::tx_byte
 */    
void c_uart::tx_byte( unsigned char byte )
{
    if (tx_bytes==MAX_FIFO_LENGTH)
    {
        fprintf(stderr,"c_uart::overflow in transmit\n");
        return;
    }
    tx_fifo[tx_bytes].start_bits = 1;
    tx_fifo[tx_bytes].stop_bits = 1;
    tx_fifo[tx_bytes].data_bits = 8;
    tx_fifo[tx_bytes].data = byte;
}

/*a Constructors and destructors for uart
*/
/*f c_uart::c_uart
*/
c_uart::c_uart( class c_engine *eng, void *eng_handle )
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

    /*b Register state then reset
     */
    engine->register_state_desc( engine_handle, 1, state_desc_uart_posedge_int_clock, &posedge_int_clock_state, NULL );
    cycle_number = 0;
    reset_active_high_int_reset();
    if (exec_file_filename)
    {
        sl_exec_file_allocate_and_read_exec_file( engine->error, engine->message, internal_exec_file_instantiate_callback, (void *)this, "exec_file", exec_file_filename, &exec_file_data, "Test harness", uart_file_cmds, uart_file_fns );
    }

    /*b Done
     */

}

/*f c_uart::~c_uart
*/
c_uart::~c_uart()
{
    delete_instance();
}

/*f c_uart::delete_instance
*/
t_sl_error_level c_uart::delete_instance( void )
{
    return error_level_okay;
}

/*a Class reset/preclock/clock methods for uart
*/
/*f c_uart::reset
*/
t_sl_error_level c_uart::reset( int pass )
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

/*f c_uart::reset_active_high_int_reset
*/
t_sl_error_level c_uart::reset_active_high_int_reset( void )
{
    posedge_int_clock_state.tx_fsm = tx_fsm_idle;
    posedge_int_clock_state.tx_bit = 0;
    posedge_int_clock_state.txd = 1;
    posedge_int_clock_state.tx_baud_counter = 0;

    posedge_int_clock_state.rx_fsm = rx_fsm_idle;
    posedge_int_clock_state.rx_bit = 0;
    posedge_int_clock_state.rxd = 0;
    posedge_int_clock_state.rx_baud_counter = 0;

    tx_bytes = 0;
    rx_bytes = 0;

    return error_level_okay;
}

/*f c_uart::evaluate_combinatorials
*/
t_sl_error_level c_uart::evaluate_combinatorials( void )
{
    if (inputs.uart_reset[0]==1)
    {
        reset_active_high_int_reset();
    }

    /*b Done
     */
    return error_level_okay;
}

/*f c_uart::preclock_posedge_int_clock
*/
t_sl_error_level c_uart::preclock_posedge_int_clock( void )
{

    /*b Copy current state to next state
     */
    memcpy( &next_posedge_int_clock_state, &posedge_int_clock_state, sizeof(posedge_int_clock_state) );

    /*b Done
     */
    return error_level_okay;
}

/*f c_uart::clock_posedge_int_clock
*/
t_sl_error_level c_uart::clock_posedge_int_clock( void )
{
    t_uart_event_data *event, **last_event_ptr;
    char buffer[256];

    /*b Copy next state to current
    */
    memcpy( &posedge_int_clock_state, &next_posedge_int_clock_state, sizeof(posedge_int_clock_state) );

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
            case event_flag_rx_has_string: v=-1; break;
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
        sprintf( buffer, "RxD %d TxD %d", inputs.rxd[0], 1 );
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
/*f c_uart__init
*/
extern void c_uart__init( void )
{
    se_external_module_register( 1, "uart", uart_instance_fn );
}

/*a Scripting support code
*/
/*f inituart
*/
extern "C" void inituart( void )
{
    c_uart__init( );
    scripting_init_module( "uart" );
}
