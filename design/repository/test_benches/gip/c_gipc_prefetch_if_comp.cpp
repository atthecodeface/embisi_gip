/*a Copyright
  
  This file 'c_gipc_prefetch_if_comp.cpp' copyright Embisi 2003, 2004
  
*/

/*a Includes
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "sl_debug.h"
#include "sl_exec_file.h"
#include "sl_general.h"
#include "sl_token.h"
#include "be_model_includes.h"
#include "sl_fifo.h"
#include "io_cmd.h"

/*a Defines
 */
#define INPUT( name, width, clk ) \
    { \
        engine->register_input_signal( engine_handle, #name, width, (int **)&inputs.name ); \
        engine->register_input_used_on_clock( engine_handle, #name, #clk, 1 ); \
    }

#define COMB_OUTPUT( name, width, clk )\
    { \
        engine->register_output_signal( engine_handle, #name, width, (int *)&combinatorials.name ); \
        engine->register_output_generated_on_clock( engine_handle, #name, #clk, 1 ); \
        combinatorials.name = 0; \
    }

#define STATE_OUTPUT( name, width, clk )\
    { \
        engine->register_output_signal( engine_handle, #name, width, (int *)&posedge_int_clock_state.name ); \
        engine->register_output_generated_on_clock( engine_handle, #name, #clk, 1 ); \
    }

/*a Types
 */
/*t cmd_*
 */
enum
{
     cmd_fifo_reset=sl_exec_file_cmd_first_external,
};

/*t fn_*
 */
enum
{
    fn_input=sl_exec_file_fn_first_internal,
};

/*t t_gipc_prefetch_if_comp_posedge_int_clock_state
*/
typedef struct t_gipc_prefetch_if_comp_posedge_int_clock_state
{
    unsigned int address;
} t_gipc_prefetch_if_comp_posedge_int_clock_state;

/*t t_gipc_prefetch_if_comp_inputs
*/
typedef struct t_gipc_prefetch_if_comp_inputs
{
    unsigned int *int_reset;

    unsigned int *fetch_16;
    unsigned int *fetch_op;

    unsigned int *fetch_data_valid;
    unsigned int *fetch_data;
    unsigned int *fetch_pc;

    unsigned int *prefetch_op;
    unsigned int *prefetch_address;

} t_gipc_prefetch_if_comp_inputs;

/*t t_gipc_prefetch_if_comp_combinatorials
*/
typedef struct t_gipc_prefetch_if_comp_combinatorials
{
    unsigned int cmd_fifo_empty; // bus, one per fifo
    unsigned int tx_data_fifo_empty; // bus, one per fifo
    unsigned int status_fifo_full; // bus, one per fifo
    unsigned int rx_data_fifo_full; // bus, one per fifo
} t_gipc_prefetch_if_comp_combinatorials;

/*t c_gipc_prefetch_if_comp
*/
class c_gipc_prefetch_if_comp
{
public:
    c_gipc_prefetch_if_comp::c_gipc_prefetch_if_comp( class c_engine *eng, void *eng_handle );
    c_gipc_prefetch_if_comp::~c_gipc_prefetch_if_comp();
    t_sl_error_level c_gipc_prefetch_if_comp::delete_instance( void );
    t_sl_error_level c_gipc_prefetch_if_comp::reset( int pass );
    t_sl_error_level c_gipc_prefetch_if_comp::evaluate_combinatorials( void );
    t_sl_error_level c_gipc_prefetch_if_comp::preclock_posedge_int_clock( void );
    t_sl_error_level c_gipc_prefetch_if_comp::clock_posedge_int_clock( void );
    t_sl_error_level c_gipc_prefetch_if_comp::reset_active_high_int_reset( void );
    void c_gipc_prefetch_if_comp::exec_file_instantiate_callback( struct t_sl_exec_file_data *file_data );
private:
    void c_gipc_prefetch_if_comp::run_exec_file( void );
    t_gipc_prefetch_if_comp_event_data *c_gipc_prefetch_if_comp::gipc_prefetch_if_comp_add_event( const char *reason, t_event_fifo event_fifo, int fifo_number, t_event_flag event_flag, int value, int timeout );
    c_engine *engine;
    void *engine_handle;
    struct t_sl_exec_file_data *exec_file_data;
    char *mif_filename;

    int cycle_number;
    int verbose_level;

    int num_cmd_fifos;
    int num_status_fifos;
    int num_tx_data_fifos;
    int num_rx_data_fifos;
    t_sl_fifo *cmd_fifos[MAX_CMD_FIFOS];
    t_sl_fifo *status_fifos[MAX_STATUS_FIFOS];
    t_sl_fifo *tx_data_fifos[MAX_TX_DATA_FIFOS];
    t_sl_fifo *rx_data_fifos[MAX_RX_DATA_FIFOS];

    t_gipc_prefetch_if_comp_event_data *event_list;

    t_gipc_prefetch_if_comp_inputs inputs;
    t_gipc_prefetch_if_comp_combinatorials combinatorials;
    t_gipc_prefetch_if_comp_posedge_int_clock_state next_posedge_int_clock_state;
    t_gipc_prefetch_if_comp_posedge_int_clock_state posedge_int_clock_state;

};

/*a Static function declarations
 */
static t_sl_exec_file_eval_fn ef_fn_eval_input;

/*a Statics
 */
/*v gipc_prefetch_if_comp_file_cmds
 */
static t_sl_exec_file_cmd gipc_prefetch_if_comp_file_cmds[] =
{
     {cmd_fifo_reset,           2, "fifo_reset", "si", "fifo_reset <type> <number> - asynchronous reset of one of the FIFOs, use at initialization only"},
     {cmd_fifo_cmd_add,         3, "fifo_cmd_add", "iii", "fifo_cmd_add <number> <time> <command> - add a timed command to the given command FIFO"},
     {cmd_fifo_tx_data_add,     2, "fifo_tx_data_add", "iiiiiiiiiiiiiiii", "fifo_tx_data_add <number> <data> [<data>*] - add data to a transmit FIFO"},
     {cmd_fifo_status_check,    3, "fifo_status_check", "iii", "fifo_status_check <number> <time> <data> - check the data in the FIFO is given (time==0 means ignore)"},
     {cmd_fifo_status_read,     3, "fifo_status_read", "ivv", "fifo_status_read <number> <time variable> <data variable> - read the data in the FIFO to two variables"},
     {cmd_fifo_rx_data_check,   3, "fifo_rx_data_check", "iii", "fifo_rx_data_check <number> <number of data> <data> [<data>*] - check the data in the FIFO is given"},
     {cmd_fifo_rx_data_check_x, 4, "fifo_rx_data_check_x", "iiii", "fifo_rx_data_check_x <number> <number of data> <mask> <data> [(<mask> <data>)*] - check the data in the FIFO is given"},
     {cmd_fifo_rx_data_read,    2, "fifo_rx_data_read", "iv", "fifo_rx_data_read <number> <data variable> - read the first data in the FIFO to a variable"},
     {cmd_wait,                 1, "wait",  "i", "wait <number of cycles>"},
     {cmd_wait_until_cmd_e,     1, "wait_until_cmd_e",  "i", "wait_until_cmd_e <number>"},
     {cmd_wait_until_tx_data_e, 1, "wait_until_tx_data_e",  "i", "wait_until_tx_data_e <number>"},
     {cmd_wait_until_status_ne, 1, "wait_until_status_ne",  "i", "wait_until_status_ne <number>"},
     {sl_exec_file_cmd_none,    0, NULL, NULL, NULL }
};

/*v gipc_prefetch_if_comp_file_fns
 */
static t_sl_exec_file_fn gipc_prefetch_if_comp_file_fns[] =
{
     {fn_input,               "input",         'i', "s", "input(<name>)", ef_fn_eval_input },
     {sl_exec_file_fn_none, NULL,     0,   NULL, NULL },
};

/*f state_desc_gipc_prefetch_if_comp
*/
#define struct_offset( ptr, a ) (((char *)&(ptr->a))-(char *)ptr)
static t_gipc_prefetch_if_comp_posedge_int_clock_state *___gipc_prefetch_if_comp_posedge_int_clock__ptr;
static t_engine_state_desc state_desc_gipc_prefetch_if_comp_posedge_int_clock[] =
{
//    {"src_fsm", engine_state_desc_type_bits, NULL, struct_offset(___gipc_prefetch_if_comp_posedge_int_clock__ptr, src_fsm), {2,0,0,0}, {NULL,NULL,NULL,NULL} },
//    {"src_left", engine_state_desc_type_bits, NULL, struct_offset(___gipc_prefetch_if_comp_posedge_int_clock__ptr, src_left), {5,0,0,0}, {NULL,NULL,NULL,NULL} },
//    {"fc_credit", engine_state_desc_type_bits, NULL, struct_offset(___gipc_prefetch_if_comp_posedge_int_clock__ptr, src_channel_state[0].fc_credit), {32,0,0,0}, {NULL,NULL,NULL,NULL} },
    {"", engine_state_desc_type_none, NULL, 0, {0,0,0,0}, {NULL,NULL,NULL,NULL} }
};

/*a Static wrapper functions for gipc_prefetch_if_comp
*/
/*f gipc_prefetch_if_comp_instance_fn
*/
static t_sl_error_level gipc_prefetch_if_comp_instance_fn( c_engine *engine, void *engine_handle )
{
    c_gipc_prefetch_if_comp *mod;
    mod = new c_gipc_prefetch_if_comp( engine, engine_handle );
    if (!mod)
        return error_level_fatal;
    return error_level_okay;
}

/*f gipc_prefetch_if_comp_delete_fn - simple callback wrapper for the main method
*/
static t_sl_error_level gipc_prefetch_if_comp_delete_fn( void *handle )
{
    c_gipc_prefetch_if_comp *mod;
    t_sl_error_level result;
    mod = (c_gipc_prefetch_if_comp *)handle;
    result = mod->delete_instance();
    delete( mod );
    return result;
}

/*f gipc_prefetch_if_comp_reset_fn
*/
static t_sl_error_level gipc_prefetch_if_comp_reset_fn( void *handle, int pass )
{
    c_gipc_prefetch_if_comp *mod;
    mod = (c_gipc_prefetch_if_comp *)handle;
    return mod->reset( pass );
}

/*f gipc_prefetch_if_comp_combinatorial_fn
*/
static t_sl_error_level gipc_prefetch_if_comp_combinatorial_fn( void *handle )
{
    c_gipc_prefetch_if_comp *mod;
    mod = (c_gipc_prefetch_if_comp *)handle;
    return mod->evaluate_combinatorials();
}

/*f gipc_prefetch_if_comp_preclock_posedge_int_clock_fn
*/
static t_sl_error_level gipc_prefetch_if_comp_preclock_posedge_int_clock_fn( void *handle )
{
    c_gipc_prefetch_if_comp *mod;
    mod = (c_gipc_prefetch_if_comp *)handle;
    return mod->preclock_posedge_int_clock();
}

/*f gipc_prefetch_if_comp_clock_posedge_int_clock_fn
*/
static t_sl_error_level gipc_prefetch_if_comp_clock_posedge_int_clock_fn( void *handle )
{
    c_gipc_prefetch_if_comp *mod;
    mod = (c_gipc_prefetch_if_comp *)handle;
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
     c_gipc_prefetch_if_comp *cif;
     cif = (c_gipc_prefetch_if_comp *)handle;
     cif->exec_file_instantiate_callback( file_data );
}

/*f c_gipc_prefetch_if_comp::exec_file_instantiate_callback
 */
void c_gipc_prefetch_if_comp::exec_file_instantiate_callback( struct t_sl_exec_file_data *file_data )
{

     sl_exec_file_set_environment_interrogation( file_data, (t_sl_get_environment_fn)sl_option_get_string, (void *)engine->get_option_list( engine_handle ) );

     engine->add_simulation_exec_file_enhancements( file_data );
     engine->waveform_add_exec_file_enhancements( file_data );
     engine->register_add_exec_file_enhancements( file_data, engine_handle );
}

/*f c_gipc_prefetch_if_comp::gipc_prefetch_if_comp_add_event
 */
t_gipc_prefetch_if_comp_event_data *c_gipc_prefetch_if_comp::gipc_prefetch_if_comp_add_event( const char *reason, t_event_fifo event_fifo, int fifo_number, t_event_flag event_flag, int value, int timeout )
{
     t_gipc_prefetch_if_comp_event_data *event;
     char name[1024];

     event = (t_gipc_prefetch_if_comp_event_data *)malloc(sizeof(t_gipc_prefetch_if_comp_event_data));
     if (!event)
          return NULL;

     snprintf( name, sizeof(name), "__%s__%s__%d", sl_exec_file_command_threadname( exec_file_data ), reason, cycle_number );
     name[sizeof(name)-1] = 0;
     event->event = sl_exec_file_event_create( exec_file_data, name );
     sl_exec_file_event_reset( exec_file_data, event->event );

     event->fifo = event_fifo;
     event->fifo_number = fifo_number;
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

/*f c_gipc_prefetch_if_comp::run_exec_file
 */
void c_gipc_prefetch_if_comp::run_exec_file( void )
{
     int cmd, cmd_type;
     t_sl_exec_file_value *args;
     int i;
     unsigned int d[2];
     t_gipc_prefetch_if_comp_event_data *event;
     char buffer[256];

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
                         case cmd_fifo_reset:
                             if (!strcmp(args[0].p.string, "cmd"))
                             {
                                 sl_fifo_reset( cmd_fifos[args[1].integer] );
                             }
                             if (!strcmp(args[0].p.string, "status"))
                             {
                                 sl_fifo_reset( status_fifos[args[1].integer] );
                             }
                             if (!strcmp(args[0].p.string, "txd"))
                             {
                                 sl_fifo_reset( tx_data_fifos[args[1].integer] );
                             }
                             if (!strcmp(args[0].p.string, "rxd"))
                             {
                                 sl_fifo_reset( rx_data_fifos[args[1].integer] );
                             }
                             break;
                         case cmd_fifo_cmd_add:
                             d[0] = args[1].integer;
                             d[1] = args[2].integer;
                             sl_fifo_add_entry( cmd_fifos[args[0].integer], d );
                             if (verbose_level>0)
                             {
                                 fprintf(stderr, "Added command 0x%08x 0x%08x to command fifo %d\n", d[0], d[1], args[0].integer );
                             }
                             break;
                         case cmd_fifo_tx_data_add:
                             for (i=0; i<args[1].integer; i++)
                             {
                                 sl_fifo_add_entry( tx_data_fifos[args[0].integer], (unsigned int *)&args[i+2].integer );
                                if (verbose_level>0)
                                {
                                    fprintf(stderr, "Added data 0x%08x to data fifo %d\n", args[i+2].integer, args[0].integer );
                                }
                             }
                             break;
                         case cmd_fifo_status_check:
                             if (sl_fifo_remove_entry( status_fifos[args[0].integer], d ))
                             {
                                 sl_fifo_commit_reads( status_fifos[args[0].integer] );
                                 if (verbose_level>0)
                                 {
                                     fprintf(stderr, "Removed status data %08x/%08x from status fifo %d\n", d[0], d[1], args[0].integer );
                                 }
                                 if ((args[1].integer!=0) && (args[1].integer!=(int)d[0]))
                                 {
                                     sprintf( buffer, "c_gipc_prefetch_if_comp:Status fifo entry %08x/%08x from status fifo %d has incorrect time (expected %08x)", d[0], d[1], args[0].integer, args[1].integer );
                                     engine->message->add_error( NULL, error_level_info, error_number_sl_fail, error_id_sl_exec_file_allocate_and_read_exec_file,
                                                               error_arg_type_integer, engine->cycle(),
                                                               error_arg_type_malloc_string, buffer,
                                                               error_arg_type_malloc_string, engine->get_instance_name(engine_handle), 
                                                               error_arg_type_none );
                                 }
                                 if (args[2].integer!=(int)d[1])
                                 {
                                     sprintf( buffer, "c_gipc_prefetch_if_comp:Status fifo entry %08x/%08x from status fifo %d has incorrect data (expected %08x)", d[0], d[1], args[0].integer, args[2].integer );
                                     engine->message->add_error( NULL, error_level_info, error_number_sl_fail, error_id_sl_exec_file_allocate_and_read_exec_file,
                                                               error_arg_type_integer, engine->cycle(),
                                                               error_arg_type_malloc_string, buffer,
                                                               error_arg_type_malloc_string, engine->get_instance_name(engine_handle), 
                                                               error_arg_type_none );
                                 }
                             }
                             else
                             {
                                 sprintf( buffer, "c_gipc_prefetch_if_comp:Status fifo %d was empty", args[0].integer);
                                 engine->message->add_error( NULL, error_level_info, error_number_sl_fail, error_id_sl_exec_file_allocate_and_read_exec_file,
                                                           error_arg_type_integer, engine->cycle(),
                                                           error_arg_type_malloc_string, buffer,
                                                           error_arg_type_malloc_string, engine->get_instance_name(engine_handle), 
                                                           error_arg_type_none );
                             }
                             break;
                         case cmd_fifo_status_read:
                             if (sl_fifo_remove_entry( status_fifos[args[0].integer], d ))
                             {
                                 sl_fifo_commit_reads( status_fifos[args[0].integer] );
                                 if (verbose_level>0)
                                 {
                                     fprintf(stderr, "Removed status data %08x/%08x from status fifo %d\n", d[0], d[1], args[0].integer );
                                 }
                                 sl_exec_file_set_integer_variable( exec_file_data, args[1].p.string, (int *)&d[0] );
                                 sl_exec_file_set_integer_variable( exec_file_data, args[2].p.string, (int *)&d[1] );
                             }
                             else
                             {
                                 sprintf( buffer, "c_gipc_prefetch_if_comp:Status fifo %d was empty", args[0].integer);
                                 engine->message->add_error( NULL, error_level_info, error_number_sl_fail, error_id_sl_exec_file_allocate_and_read_exec_file,
                                                           error_arg_type_integer, engine->cycle(),
                                                           error_arg_type_malloc_string, buffer,
                                                           error_arg_type_malloc_string, engine->get_instance_name(engine_handle), 
                                                           error_arg_type_none );
                             }
                             break;
                         case cmd_fifo_rx_data_check:
                             for (i=0; i<args[1].integer; i++)
                             {
                                 if (sl_fifo_remove_entry( rx_data_fifos[args[0].integer], d ))
                                 {
                                     sl_fifo_commit_reads( rx_data_fifos[args[0].integer] );
                                     if (verbose_level>0)
                                     {
                                         fprintf(stderr, "Removed rx data %08x from rx data fifo %d\n", d[0], args[0].integer );
                                     }
                                     if (d[0]!=(unsigned int)args[i+2].integer)
                                     {
                                         sprintf( buffer, "c_gipc_prefetch_if_comp:Rx data fifo entry %08x from rx data fifo %d has incorrect data (expected %08x)", d[0], args[0].integer, args[2+i].integer );
                                         engine->message->add_error( NULL, error_level_info, error_number_sl_fail, error_id_sl_exec_file_allocate_and_read_exec_file,
                                                                     error_arg_type_integer, engine->cycle(),
                                                                     error_arg_type_malloc_string, buffer,
                                                                     error_arg_type_malloc_string, engine->get_instance_name(engine_handle), 
                                                                     error_arg_type_none );
                                     }
                                 }
                                 else
                                 {
                                     sprintf( buffer, "c_gipc_prefetch_if_comp:Rx data fifo %d was empty", args[0].integer);
                                     engine->message->add_error( NULL, error_level_info, error_number_sl_fail, error_id_sl_exec_file_allocate_and_read_exec_file,
                                                                 error_arg_type_integer, engine->cycle(),
                                                                 error_arg_type_malloc_string, buffer,
                                                                 error_arg_type_malloc_string, engine->get_instance_name(engine_handle), 
                                                                 error_arg_type_none );
                                     break;
                                 }
                             }
                             break;
                         case cmd_fifo_rx_data_check_x:
                             for (i=0; i<args[1].integer; i+=2)
                             {
                                 if (sl_fifo_remove_entry( rx_data_fifos[args[0].integer], d ))
                                 {
                                     sl_fifo_commit_reads( rx_data_fifos[args[0].integer] );
                                     if (verbose_level>0)
                                     {
                                         fprintf(stderr, "Removed rx data %08x from rx data fifo %d\n", d[0], args[0].integer );
                                     }
                                     if ((d[0]&(unsigned int)args[i+2].integer)!=(((unsigned int)args[i+3].integer)&((unsigned int)args[i+2].integer)))
                                     {
                                         sprintf( buffer, "c_gipc_prefetch_if_comp:Rx data fifo entry %08x from rx data fifo %d has incorrect data (expected %08x/%08x)", d[0], args[0].integer, args[2+i].integer, args[3+i].integer );
                                         engine->message->add_error( NULL, error_level_info, error_number_sl_fail, error_id_sl_exec_file_allocate_and_read_exec_file,
                                                                     error_arg_type_integer, engine->cycle(),
                                                                     error_arg_type_malloc_string, buffer,
                                                                     error_arg_type_malloc_string, engine->get_instance_name(engine_handle), 
                                                                     error_arg_type_none );
                                     }
                                 }
                                 else
                                 {
                                     sprintf( buffer, "c_gipc_prefetch_if_comp:Rx data fifo %d was empty", args[0].integer);
                                     engine->message->add_error( NULL, error_level_info, error_number_sl_fail, error_id_sl_exec_file_allocate_and_read_exec_file,
                                                                 error_arg_type_integer, engine->cycle(),
                                                                 error_arg_type_malloc_string, buffer,
                                                                 error_arg_type_malloc_string, engine->get_instance_name(engine_handle), 
                                                                 error_arg_type_none );
                                     break;
                                 }
                             }
                             break;
                         case cmd_fifo_rx_data_read:
                             if (sl_fifo_remove_entry( rx_data_fifos[args[0].integer], d ))
                             {
                                 sl_fifo_commit_reads( rx_data_fifos[args[0].integer] );
                                 if (verbose_level>0)
                                 {
                                     fprintf(stderr, "Removed rx data %08x from rx data fifo %d\n", d[0], args[0].integer );
                                 }
                                 sl_exec_file_set_integer_variable( exec_file_data, args[1].p.string, (int *)&d[0] );
                             }
                             else
                             {
                                 sprintf( buffer, "c_gipc_prefetch_if_comp:Rx data fifo %d was empty", args[0].integer);
                                 engine->message->add_error( NULL, error_level_info, error_number_sl_fail, error_id_sl_exec_file_allocate_and_read_exec_file,
                                                             error_arg_type_integer, engine->cycle(),
                                                             error_arg_type_malloc_string, buffer,
                                                             error_arg_type_malloc_string, engine->get_instance_name(engine_handle), 
                                                             error_arg_type_none );
                             }
                             break;
                         case cmd_wait:
                             if ( (event = gipc_prefetch_if_comp_add_event( "wait", event_fifo_none, 0, event_flag_empty, 0, args[0].integer )) != NULL )
                             {
                                 fprintf(stderr,"Waiting for time %d\n", args[0].integer);
                                 sl_exec_file_wait_for_event( exec_file_data, NULL, event->event );
                             }
                             break;
                         case cmd_wait_until_cmd_e:
                             if ( (event = gipc_prefetch_if_comp_add_event( "wait_until_cmd_e", event_fifo_cmd, args[0].integer, event_flag_empty, 1, -1 )) != NULL )
                             {
                                 sl_exec_file_wait_for_event( exec_file_data, NULL, event->event );
                             }
                             break;
                         case cmd_wait_until_tx_data_e:
                             if ( (event = gipc_prefetch_if_comp_add_event( "wait_until_tx_data_e", event_fifo_tx_data, args[0].integer, event_flag_empty, 1, -1 )) != NULL )
                             {
                                 sl_exec_file_wait_for_event( exec_file_data, NULL, event->event );
                             }
                             break;
                         case cmd_wait_until_status_ne:
                             if ( (event = gipc_prefetch_if_comp_add_event( "wait_until_status_ne", event_fifo_status, args[0].integer, event_flag_empty, 0, -1 )) != NULL )
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

/*a Constructors and destructors for gipc_prefetch_if_comp
*/
/*f c_gipc_prefetch_if_comp::c_gipc_prefetch_if_comp
  options for number of each type of FIFOs
  option for exec file for testing (optionally per FIFO, else all FIFOs in one exec file)
*/
c_gipc_prefetch_if_comp::c_gipc_prefetch_if_comp( class c_engine *eng, void *eng_handle )
{
    int i;
    char *option_string, *string_copy;
    char *argv[256];
    int argc;

    /*b Set main variables
     */
    engine = eng;
    engine_handle = eng_handle;
    event_list = NULL;

    /*b Parse cmd_fifos option
     */
    num_cmd_fifos = 0;
    option_string = engine->get_option_string( engine_handle, "cmd_fifos", "" );
    argc = sl_str_split( option_string, &string_copy, sizeof(argv), argv, 0, 0, 1 ); // Split into strings, with potential lists
    for (i=0; i<argc; i++)
    {
        int size, newm, nfwm;
        // We expect for each CMD fifo a list with size, nearly empty watermark, nearly full watermark - just use sscanf to get these
        if (sscanf( argv[i], "( %d %d %d )", &size, &newm, &nfwm)==3)
        {
            if (num_cmd_fifos<MAX_CMD_FIFOS)
            {
                cmd_fifos[num_cmd_fifos] = sl_fifo_create( 8, size, newm, nfwm, 1 ); // Create FIFO with 2 words (8 bytes) per entry, of specified size and flags with data
            }
            num_cmd_fifos++;
        }
    }
    free(string_copy);

    /*b Parse status_fifos option
     */
    num_status_fifos = 0;
    option_string = engine->get_option_string( engine_handle, "status_fifos", "" );
    argc = sl_str_split( option_string, &string_copy, sizeof(argv), argv, 0, 0, 1 ); // Split into strings, with potential lists
    for (i=0; i<argc; i++)
    {
        int size, newm, nfwm;
        // We expect for each CMD fifo a list with size, nearly empty watermark, nearly full watermark - just use sscanf to get these
        if (sscanf( argv[i], "( %d %d %d )", &size, &newm, &nfwm)==3)
        {
            if (num_status_fifos<MAX_STATUS_FIFOS)
            {
                status_fifos[num_status_fifos] = sl_fifo_create( 8, size, newm, nfwm, 1 ); // Create FIFO with 2 words (8 bytes) per entry, of specified size and flags with data
            }
            num_status_fifos++;
        }
    }
    free(string_copy);

    /*b Parse tx_data_fifos option
     */
    num_tx_data_fifos = 0;
    option_string = engine->get_option_string( engine_handle, "tx_data_fifos", "" );
    argc = sl_str_split( option_string, &string_copy, sizeof(argv), argv, 0, 0, 1 ); // Split into strings, with potential lists
    for (i=0; i<argc; i++)
    {
        int size, newm, nfwm;
        // We expect for each CMD fifo a list with size, nearly empty watermark, nearly full watermark - just use sscanf to get these
        if (sscanf( argv[i], "( %d %d %d )", &size, &newm, &nfwm)==3)
        {
            if (num_tx_data_fifos<MAX_TX_DATA_FIFOS)
            {
                tx_data_fifos[num_tx_data_fifos] = sl_fifo_create( 4, size, newm, nfwm, 1 ); // Create FIFO with 1 words (4 bytes) per entry, of specified size and flags with data
            }
            num_tx_data_fifos++;
        }
    }
    free(string_copy);

    /*b Parse rx_data_fifos option
     */
    num_rx_data_fifos = 0;
    option_string = engine->get_option_string( engine_handle, "rx_data_fifos", "" );
    argc = sl_str_split( option_string, &string_copy, sizeof(argv), argv, 0, 0, 1 ); // Split into strings, with potential lists
    for (i=0; i<argc; i++)
    {
        int size, newm, nfwm;
        // We expect for each CMD fifo a list with size, nearly empty watermark, nearly full watermark - just use sscanf to get these
        if (sscanf( argv[i], "( %d %d %d )", &size, &newm, &nfwm)==3)
        {
            if (num_rx_data_fifos<MAX_RX_DATA_FIFOS)
            {
                rx_data_fifos[num_rx_data_fifos] = sl_fifo_create( 4, size, newm, nfwm, 1 ); // Create FIFO with 1 words (4 bytes) per entry, of specified size and flags with data
            }
            num_rx_data_fifos++;
        }
    }
    free(string_copy);

    /*b Done
     */
    verbose_level = engine->get_option_int( engine_handle, "verbose_level", 0 );
    exec_file_filename = engine->get_option_string( engine_handle, "exec_file", NULL );
    if (exec_file_filename)
    {
        exec_file_filename = sl_str_alloc_copy( exec_file_filename );
    }

    engine->register_delete_function( engine_handle, (void *)this, gipc_prefetch_if_comp_delete_fn );
    engine->register_reset_function( engine_handle, (void *)this, gipc_prefetch_if_comp_reset_fn );

    engine->register_comb_fn( engine_handle, (void *)this, gipc_prefetch_if_comp_combinatorial_fn );
    engine->register_clock_fns( engine_handle, (void *)this, "int_clock", gipc_prefetch_if_comp_preclock_posedge_int_clock_fn, gipc_prefetch_if_comp_clock_posedge_int_clock_fn );

    engine->register_input_signal( engine_handle, "int_reset", 1, (int **)&inputs.int_reset );

    if (num_cmd_fifos>0)
    {
        INPUT( cmd_fifo_cmd_toggle, 1, int_clock );
        INPUT( cmd_fifo_cmd, 4, int_clock );
        INPUT( cmd_fifo_to_access, 4, int_clock );
        STATE_OUTPUT( cmd_fifo_read_data, 32, int_clock );
        COMB_OUTPUT( cmd_fifo_empty, num_cmd_fifos, int_clock );
    }

    if (num_status_fifos>0)
    {
        INPUT( status_fifo_cmd_toggle, 1, int_clock );
        INPUT( status_fifo_cmd, 4, int_clock );
        INPUT( status_fifo_to_access, 4, int_clock );
        INPUT( status_fifo_write_data, 32, int_clock );
        COMB_OUTPUT( status_fifo_full, num_status_fifos, int_clock );
    }

    if (num_tx_data_fifos>0)
    {
        INPUT( tx_data_fifo_cmd_toggle, 1, int_clock );
        INPUT( tx_data_fifo_cmd, 4, int_clock );
        INPUT( tx_data_fifo_to_access, 4, int_clock );
        STATE_OUTPUT( tx_data_fifo_read_data, 32, int_clock );
        COMB_OUTPUT( tx_data_fifo_empty, num_tx_data_fifos, int_clock );
    }

    if (num_rx_data_fifos>0)
    {
        INPUT( rx_data_fifo_cmd_toggle, 1, int_clock );
        INPUT( rx_data_fifo_cmd, 4, int_clock );
        INPUT( rx_data_fifo_to_access, 4, int_clock );
        INPUT( rx_data_fifo_write_data, 32, int_clock );
        COMB_OUTPUT( rx_data_fifo_full, num_rx_data_fifos, int_clock );
    }


//    engine->register_state_desc( engine_handle, 1, state_desc_gipc_prefetch_if_comp_posedge_int_clock, &posedge_int_clock_state, NULL );
    cycle_number = 0;
    reset_active_high_int_reset();
    if (exec_file_filename)
    {
        sl_exec_file_allocate_and_read_exec_file( engine->error, engine->message, internal_exec_file_instantiate_callback, (void *)this, "exec_file", exec_file_filename, &exec_file_data, "Test harness", gipc_prefetch_if_comp_file_cmds, gipc_prefetch_if_comp_file_fns );
    }
}

/*f c_gipc_prefetch_if_comp::~c_gipc_prefetch_if_comp
*/
c_gipc_prefetch_if_comp::~c_gipc_prefetch_if_comp()
{
    delete_instance();
}

/*f c_gipc_prefetch_if_comp::delete_instance
*/
t_sl_error_level c_gipc_prefetch_if_comp::delete_instance( void )
{
    if (exec_file_data)
    {
        sl_exec_file_free( exec_file_data );
        exec_file_data = NULL;
    }
    return error_level_okay;
}

/*a Class reset/preclock/clock methods for gipc_prefetch_if_comp
*/
/*f c_gipc_prefetch_if_comp::reset
*/
t_sl_error_level c_gipc_prefetch_if_comp::reset( int pass )
{
    int i;
    if (pass==0)
    {
        if (exec_file_data)
        {
            sl_exec_file_reset( exec_file_data );
        }
        cycle_number = -1;
        for (i=0; i<num_cmd_fifos; i++)     sl_fifo_reset( cmd_fifos[i] );
        for (i=0; i<num_status_fifos; i++)  sl_fifo_reset( status_fifos[i] );
        for (i=0; i<num_tx_data_fifos; i++) sl_fifo_reset( tx_data_fifos[i] );
        for (i=0; i<num_rx_data_fifos; i++) sl_fifo_reset( rx_data_fifos[i] );
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

/*f c_gipc_prefetch_if_comp::reset_active_high_int_reset
*/
t_sl_error_level c_gipc_prefetch_if_comp::reset_active_high_int_reset( void )
{
    posedge_int_clock_state.cmd_fifo_read_data = 0;
    posedge_int_clock_state.tx_data_fifo_read_data = 0;

    posedge_int_clock_state.cmd_fifo_toggle = 0;
    posedge_int_clock_state.cmd_fifo_do_cmd = 0;

    posedge_int_clock_state.status_fifo_toggle = 0;
    posedge_int_clock_state.status_fifo_do_cmd = 0;

    posedge_int_clock_state.tx_data_fifo_toggle = 0;
    posedge_int_clock_state.tx_data_fifo_do_cmd = 0;

    posedge_int_clock_state.rx_data_fifo_toggle = 0;
    posedge_int_clock_state.rx_data_fifo_do_cmd = 0;

    return error_level_okay;
}

/*f c_gipc_prefetch_if_comp::evaluate_combinatorials
*/
t_sl_error_level c_gipc_prefetch_if_comp::evaluate_combinatorials( void )
{
    int i;

    if (inputs.int_reset[0]==1)
    {
        reset_active_high_int_reset();
    }
    combinatorials.cmd_fifo_empty = 0;
    for (i=0; i<num_cmd_fifos; i++)
    {
        int e;
        sl_fifo_flags( cmd_fifos[i], &e, NULL, NULL, NULL );
        combinatorials.cmd_fifo_empty |= (e<<i);
    }
    combinatorials.tx_data_fifo_empty = 0;
    for (i=0; i<num_tx_data_fifos; i++)
    {
        int e;
        sl_fifo_flags( tx_data_fifos[i], &e, NULL, NULL, NULL );
        combinatorials.tx_data_fifo_empty |= (e<<i);
    }

    return error_level_okay;
}

/*f c_gipc_prefetch_if_comp::preclock_posedge_int_clock
*/
t_sl_error_level c_gipc_prefetch_if_comp::preclock_posedge_int_clock( void )
{
    /*b Copy current state to next
     */
    memcpy( &next_posedge_int_clock_state, &posedge_int_clock_state, sizeof(posedge_int_clock_state) );

    /*b Handle fifo control inputs
     */
    if (num_cmd_fifos>0)
    {
        next_posedge_int_clock_state.cmd_fifo_toggle = inputs.cmd_fifo_cmd_toggle[0];
        next_posedge_int_clock_state.cmd_fifo_do_cmd = (next_posedge_int_clock_state.cmd_fifo_toggle != posedge_int_clock_state.cmd_fifo_toggle);
    }
    if (num_status_fifos>0)
    {
        next_posedge_int_clock_state.status_fifo_toggle = inputs.status_fifo_cmd_toggle[0];
        next_posedge_int_clock_state.status_fifo_do_cmd = (next_posedge_int_clock_state.status_fifo_toggle != posedge_int_clock_state.status_fifo_toggle);
    }
    if (num_tx_data_fifos>0)
    {
        next_posedge_int_clock_state.tx_data_fifo_toggle = inputs.tx_data_fifo_cmd_toggle[0];
        next_posedge_int_clock_state.tx_data_fifo_do_cmd = (next_posedge_int_clock_state.tx_data_fifo_toggle != posedge_int_clock_state.tx_data_fifo_toggle);
    }
    if (num_rx_data_fifos>0)
    {
        next_posedge_int_clock_state.rx_data_fifo_toggle = inputs.rx_data_fifo_cmd_toggle[0];
        next_posedge_int_clock_state.rx_data_fifo_do_cmd = (next_posedge_int_clock_state.rx_data_fifo_toggle != posedge_int_clock_state.rx_data_fifo_toggle);
    }

    /*b Done
     */
    return error_level_okay;
}

/*f c_gipc_prefetch_if_comp::clock_posedge_int_clock
*/
t_sl_error_level c_gipc_prefetch_if_comp::clock_posedge_int_clock( void )
{
    t_gipc_prefetch_if_comp_event_data *event, **last_event_ptr;

    /*b Copy next state to current
     */
    memcpy( &posedge_int_clock_state, &next_posedge_int_clock_state, sizeof(posedge_int_clock_state) );
    cycle_number++;

    /*b Fire any events that should, er, fire
     */
    for (event=event_list; event; event=event->next_in_list)
    {
        //fprintf(stderr,"Preclocking event %p/%d\n", event, event->fired );
        if (!event->fired)
        {
            int e, f, ne, nf, v;
            if  ( (event->timeout!=-1) && (event->timeout==cycle_number) )
            {
                event->fired = 1;
            }
            //fprintf(stderr,"Fifo %d/%d flag %d value %d timeout %d\n", event->fifo, event->fifo_number, event->flag, event->value, event->timeout );
            switch (event->fifo)
            {
            case event_fifo_none:
                e = f = ne = nf = !event->value;
                break;
            case event_fifo_cmd:
                sl_fifo_flags( cmd_fifos[event->fifo_number], &e, &f, &ne, &nf );
                break;
            case event_fifo_status:
                sl_fifo_flags( status_fifos[event->fifo_number], &e, &f, &ne, &nf );
                break;
            case event_fifo_tx_data:
                sl_fifo_flags( tx_data_fifos[event->fifo_number], &e, &f, &ne, &nf );
                break;
            case event_fifo_rx_data:
                sl_fifo_flags( rx_data_fifos[event->fifo_number], &e, &f, &ne, &nf );
                break;
            }
            switch (event->flag)
            {
            case event_flag_empty:        v=e; break;
            case event_flag_full:         v=f; break;
            case event_flag_nearly_empty: v=ne; break;
            case event_flag_nearly_full:  v=nf; break;
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

    /*b Handle cmd FIFO ops directly to current output state (guaranteed to only read once per clock this way)
     */
    if (num_cmd_fifos>0)
    {
        unsigned int d[2];
        if (posedge_int_clock_state.cmd_fifo_do_cmd)
        {
            switch ((t_io_cmd_fifo_cmd)(inputs.cmd_fifo_cmd[0]))
            {
            case io_cmd_fifo_cmd_read_fifo:
                sl_fifo_remove_entry( cmd_fifos[inputs.cmd_fifo_to_access[0]], d );
                sl_fifo_commit_reads( cmd_fifos[inputs.cmd_fifo_to_access[0]] );
                if (verbose_level>0)
                {
                    fprintf(stderr, "Removed command 0x%08x 0x%08x from command fifo %d\n", d[0], d[1], inputs.cmd_fifo_to_access[0] );
                }
                posedge_int_clock_state.cmd_fifo_read_data = d[1];
                break;
            default:
                fprintf(stderr, "Unknown IO FIFO command\n");
                break;
            }
        }
    }

    /*b Handle status FIFO ops directly to current output state (guaranteed to only read once per clock this way)
     */
    if (num_status_fifos>0)
    {
        unsigned int d[2];
        if (posedge_int_clock_state.status_fifo_do_cmd)
        {
            switch ((t_io_status_fifo_cmd)(inputs.status_fifo_cmd[0]))
            {
            case io_status_fifo_cmd_write_fifo:
                d[0] = cycle_number;
                d[1] = inputs.status_fifo_write_data[0];
                sl_fifo_add_entry( status_fifos[inputs.status_fifo_to_access[0]], d );
                if (verbose_level>0)
                {
                    fprintf(stderr, "Added status 0x%08x/0x%08x to status fifo %d\n", d[0], d[1], inputs.cmd_fifo_to_access[0] );
                }
                break;
            default:
                fprintf(stderr, "Unknown IO FIFO command\n");
                break;
            }
        }
    }

    /*b Handle tx data FIFO ops directly to current output state (guaranteed to only read once per clock this way)
     */
    if (num_tx_data_fifos>0)
    {
        unsigned int d[2];
        if (posedge_int_clock_state.tx_data_fifo_do_cmd)
        {
            switch ((t_io_tx_data_fifo_cmd)(inputs.tx_data_fifo_cmd[0]))
            {
            case io_tx_data_fifo_cmd_commit_fifo:
                sl_fifo_commit_reads( tx_data_fifos[inputs.tx_data_fifo_to_access[0]] );
                if (verbose_level>0)
                {
                    fprintf(stderr, "Committed reads so far on tx data fifo %d\n", inputs.tx_data_fifo_to_access[0] );
                }
                break;
            case io_tx_data_fifo_cmd_read_fifo:
                sl_fifo_remove_entry( tx_data_fifos[inputs.tx_data_fifo_to_access[0]], d );
                if (verbose_level>0)
                {
                    fprintf(stderr, "Removed data 0x%08x from tx data fifo %d\n", d[0], inputs.tx_data_fifo_to_access[0] );
                }
                posedge_int_clock_state.tx_data_fifo_read_data = d[0];
                break;
            case io_tx_data_fifo_cmd_read_and_commit_fifo:
                sl_fifo_remove_entry( tx_data_fifos[inputs.tx_data_fifo_to_access[0]], d );
                sl_fifo_commit_reads( tx_data_fifos[inputs.tx_data_fifo_to_access[0]] );
                if (verbose_level>0)
                {
                    fprintf(stderr, "Removed data 0x%08x and committed reads from tx data fifo %d\n", d[0], inputs.tx_data_fifo_to_access[0] );
                }
                posedge_int_clock_state.tx_data_fifo_read_data = d[0];
                break;
            case io_tx_data_fifo_cmd_revert_fifo:
                sl_fifo_revert_reads( tx_data_fifos[inputs.tx_data_fifo_to_access[0]] );
                if (verbose_level>0)
                {
                    fprintf(stderr, "Reverted tx data fifo %d\n", inputs.tx_data_fifo_to_access[0] );
                }
                break;
            default:
                fprintf(stderr, "Unknown IO FIFO command\n");
                break;
            }
        }
    }

    /*b Handle rx data FIFO ops directly to current output state (guaranteed to only read once per clock this way)
     */
    if (num_rx_data_fifos>0)
    {
        unsigned int d[2];
        if (posedge_int_clock_state.rx_data_fifo_do_cmd)
        {
            switch ((t_io_rx_data_fifo_cmd)(inputs.rx_data_fifo_cmd[0]))
            {
            case io_rx_data_fifo_cmd_write_fifo:
                d[0] = inputs.rx_data_fifo_write_data[0];
                sl_fifo_add_entry( rx_data_fifos[inputs.rx_data_fifo_to_access[0]], d );
                if (verbose_level>0)
                {
                    fprintf(stderr, "Added rx data 0x%08x to rx data fifo %d\n", d[0], inputs.cmd_fifo_to_access[0] );
                }
                break;
            default:
                fprintf(stderr, "Unknown IO FIFO command\n");
                break;
            }
        }
    }

    /*b Evaluate combinatorials and return
     */
    return evaluate_combinatorials();
}

/*a Initialization functions
*/
/*f c_gipc_prefetch_if_comp__init
*/
extern void c_gipc_prefetch_if_comp__init( void )
{
    se_external_module_register( 1, "gipc_prefetch_if_comp", gipc_prefetch_if_comp_instance_fn );
}

/*a Scripting support code
*/
/*f initgipc_prefetch_if_comp
*/
extern "C" void initgipc_prefetch_if_comp( void )
{
    c_gipc_prefetch_if_comp__init( );
    scripting_init_module( "gipc_prefetch_if_comp" );
}

/*a Editor preferences and notes
mode: c ***
c-basic-offset: 4 ***
c-default-style: (quote ((c-mode . "k&r") (c++-mode . "k&r"))) ***
outline-regexp: "/\\\*a\\\|[\t ]*\/\\\*[b-z][\t ]" ***
*/

