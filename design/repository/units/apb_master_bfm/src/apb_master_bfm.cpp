/*a Includes
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sl_debug.h"
#include "sl_exec_file.h"
#include "se_external_module.h"
#include "se_scripting.h"

/*a Types
 */
/*t t_apb
 */
typedef struct t_apb
{
     int address;
     int select;
     int enable;
     int data_out;
     int read_not_write;
} t_apb;

/*t t_fsm_state
 */
typedef enum
{
     fsm_state_idle,
     fsm_state_nop,
     fsm_state_read_first,
     fsm_state_read_second,
     fsm_state_write_first,
     fsm_state_write_second,
} t_fsm_state;

/*t t_apb_state
 */
typedef struct t_apb_state
{
     t_apb apb;

     int cycle;
     int count;
     t_fsm_state fsm_state;

     int set_var;
} t_apb_state;

/*t cmd_*
 */
enum
{
     cmd_write=sl_exec_file_cmd_first_external,
     cmd_read,
     cmd_read_var,
     cmd_nop,
};

/*t	c_apb_master_bfm
*/
class c_apb_master_bfm
{
public:
     c_apb_master_bfm::c_apb_master_bfm( class c_engine *eng, void *engine_handle );
     c_apb_master_bfm::~c_apb_master_bfm();
     t_sl_error_level c_apb_master_bfm::delete_instance( void );
     t_sl_error_level c_apb_master_bfm::reset( void );
     t_sl_error_level c_apb_master_bfm::preclock( void );
     t_sl_error_level c_apb_master_bfm::clock( void );
     c_engine *engine;
     void *engine_handle;
private:
     void c_apb_master_bfm::set_next_state( t_fsm_state fsm, int count, int select, int enable, int address, int data_out, int read_not_write );
     int *data_in;
     char *var_ptr;

     t_apb_state next_state;
     t_apb_state state;

     const char *filename;
     t_sl_exec_file_data *exec_file_data;
};

/*a Static variables
 */
/*v file_cmds
 */
static t_sl_exec_file_cmd file_cmds[] =
{
     {cmd_write,          2, "write", "ii", "write <address> <data>"},
     {cmd_read,           1, "read", "i", "read <address>"},
     {cmd_read_var,       2, "read_var", "iv", "read_var <address> <variable>"},
     {cmd_nop,            1, "nop", "i", "nop <cycles>"},
     {sl_exec_file_cmd_none, 0, NULL, NULL, NULL }
};

/*f state_desc
 */
static t_apb_state *_ptr;
#define struct_offset( ptr, a ) (((char *)&(ptr->a))-(char *)ptr)
static char *fsm_state_names[] = {"idle", "nop", "read_first", "read_second", "write_first", "write_second"};
static t_engine_state_desc state_desc[] =
{
     {"cycle", engine_state_desc_type_bits, NULL, struct_offset(_ptr, cycle), {32,0,0,0}, {NULL,NULL,NULL,NULL} },
     {"count", engine_state_desc_type_bits, NULL, struct_offset(_ptr, count), {32,0,0,0}, {NULL,NULL,NULL,NULL} },
     {"fsm_state", engine_state_desc_type_fsm, NULL, struct_offset(_ptr, fsm_state), {3,fsm_state_write_second+1,0,0}, {(void *)fsm_state_names,NULL,NULL,NULL} },
     {"apb.address", engine_state_desc_type_bits, NULL, struct_offset(_ptr, apb.address), {32,0,0,0}, {NULL,NULL,NULL,NULL} },
     {"apb.select", engine_state_desc_type_bits, NULL, struct_offset(_ptr, apb.select), {1,0,0,0}, {NULL,NULL,NULL,NULL} },
     {"apb.enable", engine_state_desc_type_bits, NULL, struct_offset(_ptr, apb.enable), {1,0,0,0}, {NULL,NULL,NULL,NULL} },
     {"apb.data_out", engine_state_desc_type_bits, NULL, struct_offset(_ptr, apb.data_out), {32,0,0,0}, {NULL,NULL,NULL,NULL} },
     {"apb.read_not_write", engine_state_desc_type_bits, NULL, struct_offset(_ptr, apb.read_not_write), {1,0,0,0}, {NULL,NULL,NULL,NULL} },
     {"", engine_state_desc_type_none, NULL, 0, {0,0,0,0}, {NULL,NULL,NULL,NULL} }
};

/*a Static callback functions
 */

/*f instance_fn - simple callback wrapper for the main method
 */
static t_sl_error_level instance_fn( c_engine *engine, void *engine_handle )
{
     c_apb_master_bfm *mod;
     mod = new c_apb_master_bfm( engine, engine_handle );
     if (!mod)
          return error_level_fatal;
     return error_level_okay;
}

/*f delete_fn - simple callback wrapper for the main method
 */
static t_sl_error_level delete_fn( void *handle )
{
     c_apb_master_bfm *mod;
     t_sl_error_level result;
     mod = (c_apb_master_bfm *)handle;
     result = mod->delete_instance();
     delete( mod );
     return result;
}

/*f reset_fn - simple callback wrapper for the main method
 */
static t_sl_error_level reset_fn( void *handle )
{
     c_apb_master_bfm *mod;
     mod = (c_apb_master_bfm *)handle;
     return mod->reset();
}

/*f preclock_fn - simple callback wrapper for the main method
 */
static t_sl_error_level preclock_fn( void *handle )
{
     c_apb_master_bfm *mod;
     mod = (c_apb_master_bfm *)handle;
     return mod->preclock();
}

/*f clock_fn - simple callback wrapper for the main method
 */
static t_sl_error_level clock_fn( void *handle )
{
     c_apb_master_bfm *mod;
     mod = (c_apb_master_bfm *)handle;
     return mod->clock();
}

/*f read_enviroment_option
 */
static char *read_enviroment_option( void *handle, const char *name )
{
     c_apb_master_bfm *mod;
     mod = (c_apb_master_bfm *)handle;
     return mod->engine->get_option_string( mod->engine_handle, name, "<no default>" );
}

/*a Static sl functions
 */
/*f set_next_state
 */
void c_apb_master_bfm::set_next_state( t_fsm_state fsm, int count, int select, int enable, int address, int data_out, int read_not_write )
{
     next_state.fsm_state = fsm;
     next_state.count = count;
     next_state.apb.enable = enable;
     next_state.apb.select = select;
     next_state.apb.address = address;
     next_state.apb.data_out = data_out;
     next_state.apb.read_not_write = read_not_write;
}

/*a Constructors/destructors
 */

/*f exec_file_instantiate_callback
 */
static void exec_file_instantiate_callback( void *handle, struct t_sl_exec_file_data *file_data )
{
     c_apb_master_bfm *bfm;

     bfm = (c_apb_master_bfm *)handle;

     sl_exec_file_set_environment_interrogation( file_data, read_enviroment_option, handle );
     bfm->engine->add_simulation_exec_file_enhancements( file_data );
     bfm->engine->waveform_add_exec_file_enhancements( file_data );
     bfm->engine->register_add_exec_file_enhancements( file_data, bfm->engine_handle );
}

/*f c_apb_master_bfm::c_apb_master_bfm
 */
c_apb_master_bfm::c_apb_master_bfm( class c_engine *eng, void *eng_handle )
{
     t_sl_error_level error;

     this->engine = eng;
     this->engine_handle = eng_handle;
     DEBUG((debug_level_info, "c_apb_master_bfm::c_apb_master_bfm", "New instance %p", eng ));

     memset( &state, 0, sizeof(state) );
     memset( &next_state, 0, sizeof(next_state) );

     engine->register_delete_function( engine_handle, (void *)this, delete_fn );
     engine->register_reset_function( engine_handle, (void *)this, reset_fn );
     engine->register_clock_fns( engine_handle, (void *)this, "main_clock", preclock_fn, clock_fn );
     engine->register_output_signal( engine_handle, "apb.paddr", 16, &state.apb.address );
     engine->register_output_generated_on_clock( engine_handle, "apb.paddr", "main_clock", 1 );
     engine->register_output_signal( engine_handle, "apb.penable", 1, &state.apb.enable );
     engine->register_output_generated_on_clock( engine_handle, "apb.penable", "main_clock", 1 );
     engine->register_output_signal( engine_handle, "apb.pselect", 1, &state.apb.select );
     engine->register_output_generated_on_clock( engine_handle, "apb.pselect", "main_clock", 1 );
     engine->register_output_signal( engine_handle, "apb.pwdata", 32, &state.apb.data_out );
     engine->register_output_generated_on_clock( engine_handle, "apb.pwdata", "main_clock", 1 );
     engine->register_output_signal( engine_handle, "apb.prnw", 1, &state.apb.read_not_write );
     engine->register_output_generated_on_clock( engine_handle, "apb.prnw", "main_clock", 1 );
     engine->register_input_signal( engine_handle, "prdata", 32, &data_in );
     engine->register_input_used_on_clock( engine_handle, "prdata", "main_clock", 1 );

     engine->register_state_desc( engine_handle, 1, state_desc, &state, NULL );

     filename = engine->get_option_string( engine_handle, "filename", "apb_bfm.txt" );
     exec_file_data = NULL;
     error = sl_exec_file_allocate_and_read_exec_file( engine->error, engine->message, exec_file_instantiate_callback, (void *)this, "exec_file", filename, &exec_file_data, "APB master BFM", file_cmds, NULL );

}

/*f c_apb_master_bfm::~c_apb_master_bfm
 */
c_apb_master_bfm::~c_apb_master_bfm()
{
     delete_instance();
}

/*f c_apb_master_bfm::delete_instance
 */
t_sl_error_level c_apb_master_bfm::delete_instance( void )
{
     sl_exec_file_free( exec_file_data );
     exec_file_data = NULL;
     return error_level_okay;
}

/*a Engine invoked methods
 */
/*f c_apb_master_bfm::reset
 */
t_sl_error_level c_apb_master_bfm::reset( void )
{
     DEBUG( ( debug_level_info,
              "c_apb_master_bfm::reset",
              "reset" ));
     state.cycle = 0;
     state.fsm_state = fsm_state_idle;
     state.count = 0;

     state.apb.address = 0;
     state.apb.enable = 0;
     state.apb.select = 0;
     state.apb.data_out = 0;
     state.apb.read_not_write = 0;

     if (exec_file_data)
     {
          sl_exec_file_reset( exec_file_data );
     }
     else
     {
          return error_level_serious;
//          return engine->add_error( error_serious, "No exec file for APB master BFM (file '%s'?) at reset", filename );
     }

     return error_level_okay;
}

/*f c_apb_master_bfm::preclock
 */
t_sl_error_level c_apb_master_bfm::preclock( void )
{
     int new_trans;
     int cmd, cmd_type;
     t_sl_exec_file_value *args;

     memcpy( &next_state, &state, sizeof(state) );
     if (!exec_file_data)
          return error_level_okay;

     next_state.cycle = state.cycle+1;

     new_trans = 0;

     switch (state.fsm_state)
     {
     case fsm_state_idle:
          new_trans=1;
          break;
     case fsm_state_nop:
          if (state.count>0)
          {
               next_state.count = state.count-1;
          }
          else
          {
               new_trans=1;
          }
          break;
     case fsm_state_read_first:
          next_state.fsm_state = fsm_state_read_second;
          next_state.apb.enable = 1;
          break;
     case fsm_state_read_second:
          new_trans=1;
          if (state.set_var)
          {
               sl_exec_file_set_variable( exec_file_data, var_ptr, data_in );
          }
          break;
     case fsm_state_write_first:
          next_state.fsm_state = fsm_state_write_second;
          next_state.apb.enable = 1;
          break;
     case fsm_state_write_second:
          new_trans=1;
          break;
     }
     while (new_trans)
     {
          new_trans = 0;
          set_next_state( fsm_state_nop, 1<<28, 0, 0, 0xdead, 0xdeaddead, 1 );
          next_state.set_var = 0;
          cmd_type = sl_exec_file_get_next_cmd( exec_file_data, &cmd, &args );
          if (cmd_type==2) // no command ready from exec_file - threads all paused - so let's NOP for 1
          {
               set_next_state( fsm_state_nop, 1, 0, 0, 0xdead, 0xdeaddead, 1 );
          }
          else if (cmd_type==1) // real command
          {
               if (engine->handle_exec_file_command( exec_file_data, cmd, args ))
               {
                    new_trans = 1;
               }
               else
               {
                    switch (cmd)
                    {
                    case cmd_write:
                         set_next_state( fsm_state_write_first, 1, 1, 0, args[0].integer, args[1].integer, 0 );
                         break;
                    case cmd_read:
                    case cmd_read_var:
                         set_next_state( fsm_state_read_first, 1, 1, 0, args[0].integer, 0xdeaddead, 1 );
                         var_ptr = args[1].p.string;
                         next_state.set_var = (cmd==cmd_read_var);
                         break;
                    case cmd_nop:
                         set_next_state( fsm_state_nop, args[0].integer, 0, 0, 0xdead, 0xdeaddead, 1 );
                         break;
                    default:
                         break;
                    }
               }
          }
     }
     return error_level_okay;
}

/*f c_apb_master_bfm::clock
 */
t_sl_error_level c_apb_master_bfm::clock( void )
{
     memcpy( &state, &next_state, sizeof(state) );
     return error_level_okay;
}

/*a Initialization functions
 */
/*f apb_master_bfm__init
 */
extern void apb_master_bfm__init( void )
{
     se_external_module_register( 1, "apb_master_bfm", instance_fn );
}

/*a Scripting support code
 */
/*f initc_apb_master_bfm
 */
extern "C" void initc_apb_master_bfm( void )
{
     apb_master_bfm__init( );
     scripting_init_module( "apb_master_bfm" );
}
