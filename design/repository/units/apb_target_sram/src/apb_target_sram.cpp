/*a Copyright
  This file (c) Gavin J Stark, 2003. All rights reserved.
 */

/*a Includes
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sl_debug.h"
#include "sl_mif.h"
#include "se_external_module.h"
#include "se_scripting.h"

/*a Types
 */
/*t t_main_clock_state
 */
typedef struct t_main_clock_state
{
     int data_out;
     int reading;
     int writing;
     int address;
     int write_data;
} t_main_clock_state;

/*t t_inputs
 */
typedef struct t_inputs
{
     int *enable;
     int *select;
     int *read_not_write;
     int *address;
     int *data_in;
} t_inputs;

/*t	c_apb_target_sram
*/
class c_apb_target_sram
{
public:
     c_apb_target_sram::c_apb_target_sram( class c_engine *eng, void *eng_handle );
     c_apb_target_sram::~c_apb_target_sram();
     t_sl_error_level c_apb_target_sram::delete_instance( void );
     t_sl_error_level c_apb_target_sram::reset( void );
     t_sl_error_level c_apb_target_sram::preclock( void );
     t_sl_error_level c_apb_target_sram::clock( void );
private:
     c_engine *engine;
     void *engine_handle;
     t_inputs inputs;
     int memory_size;
     t_main_clock_state next_main_clock_state;
     t_main_clock_state main_clock_state;
     int *memory;
     char *memory_filename;
};

/*a Static variables
 */
/*f state_desc
 */
static t_main_clock_state *_ptr;
#define struct_offset( ptr, a ) (((char *)&(ptr->a))-(char *)ptr)
static t_engine_state_desc state_desc[] =
{
     {"data_out", engine_state_desc_type_bits, NULL, struct_offset(_ptr, data_out), {32,0,0,0}, {NULL,NULL,NULL,NULL} },
     {"reading", engine_state_desc_type_bits, NULL, struct_offset(_ptr, reading), {1,0,0,0}, {NULL,NULL,NULL,NULL} },
     {"writing", engine_state_desc_type_bits, NULL, struct_offset(_ptr, writing), {1,0,0,0}, {NULL,NULL,NULL,NULL} },
     {"address", engine_state_desc_type_bits, NULL, struct_offset(_ptr, address), {8,0,0,0}, {NULL,NULL,NULL,NULL} },
     {"write_data", engine_state_desc_type_bits, NULL, struct_offset(_ptr, write_data), {32,0,0,0}, {NULL,NULL,NULL,NULL} },
     {"", engine_state_desc_type_none, NULL, 0, {0,0,0,0}, {NULL,NULL,NULL,NULL} }
};

/*a Static wrapper functions
 */
/*f instance_fn
 */
static t_sl_error_level instance_fn( c_engine *engine, void *engine_handle )
{
     c_apb_target_sram *mod;
     mod = new c_apb_target_sram( engine, engine_handle );
     if (!mod)
          return error_level_fatal;
     return error_level_okay;
}

/*f delete_fn - simple callback wrapper for the main method
 */
static t_sl_error_level delete_fn( void *handle )
{
     c_apb_target_sram *mod;
     t_sl_error_level result;
     mod = (c_apb_target_sram *)handle;
     result = mod->delete_instance();
     delete( mod );
     return result;
}

/*f reset_fn
 */
static t_sl_error_level reset_fn( void *handle )
{
     c_apb_target_sram *mod;
     mod = (c_apb_target_sram *)handle;
     return mod->reset();
}

/*f preclock_fn
 */
static t_sl_error_level preclock_fn( void *handle )
{
     c_apb_target_sram *mod;
     mod = (c_apb_target_sram *)handle;
     return mod->preclock();
}

/*f clock_fn
 */
static t_sl_error_level clock_fn( void *handle )
{
     c_apb_target_sram *mod;
     mod = (c_apb_target_sram *)handle;
     return mod->clock();
}

/*a Constructors and destructors
 */
/*f c_apb_target_sram::c_apb_target_sram
 */
c_apb_target_sram::c_apb_target_sram( class c_engine *eng, void *eng_handle )
{
     t_engine_state_desc local_state_desc[2];

     engine = eng;
     engine_handle = eng_handle;

     engine->register_delete_function( engine_handle, (void *)this, delete_fn );
     engine->register_reset_function( engine_handle, (void *)this, reset_fn );
     engine->register_clock_fns( engine_handle, (void *)this, "main_clock", preclock_fn, clock_fn );

     engine->register_input_signal( engine_handle, "address", 16, &inputs.address );
     engine->register_input_used_on_clock( engine_handle, "address", "main_clock", 1 );
     engine->register_input_signal( engine_handle, "enable", 1, &inputs.enable );
     engine->register_input_used_on_clock( engine_handle, "enable", "main_clock", 1 );
     engine->register_input_signal( engine_handle, "select", 1, &inputs.select );
     engine->register_input_used_on_clock( engine_handle, "select", "main_clock", 1 );
     engine->register_input_signal( engine_handle, "read_not_write", 1, &inputs.read_not_write );
     engine->register_input_used_on_clock( engine_handle, "read_not_write", "main_clock", 1 );
     engine->register_input_signal( engine_handle, "data_in", 32, &inputs.data_in );
     engine->register_input_used_on_clock( engine_handle, "data_in", "main_clock", 1 );
     engine->register_output_signal( engine_handle, "data_out", 32, &main_clock_state.data_out );
     engine->register_output_generated_on_clock( engine_handle, "data_out", "main_clock", 1 );

     engine->register_state_desc( engine_handle, 1, state_desc, &main_clock_state, NULL );

     memory_size = 8;
     memory = NULL;
     memory_filename = engine->get_option_string( engine_handle, "load_file", "sram.mif" );
     sl_mif_allocate_and_read_mif_file( engine->error, memory_filename, "APB target SRAM", 256, 32, 0, &memory );

     local_state_desc[0].name = "memory";
     local_state_desc[0].type = engine_state_desc_type_memory;
     local_state_desc[0].ptr = NULL;
     local_state_desc[0].offset = 0;
     local_state_desc[0].args[0] = 32;
     local_state_desc[0].args[1] = 256;
     local_state_desc[1].type = engine_state_desc_type_none;
     engine->register_state_desc( engine_handle, 0, local_state_desc, (void *)&memory, NULL );
}

/*f c_apb_target_sram::~c_apb_target_sram
 */
c_apb_target_sram::~c_apb_target_sram()
{
     delete_instance();
}

/*f c_apb_target_sram::delete_instance
 */
t_sl_error_level c_apb_target_sram::delete_instance( void )
{
     if (memory)
     {
          free(memory);
          memory = NULL;
     }
     return error_level_okay;
}

/*a Class reset/preclock/clock methods
 */
/*f c_apb_target_sram::reset
 */
t_sl_error_level c_apb_target_sram::reset( void )
{
     if (memory)
     {
          free(memory);
     }
     memory = NULL;
     sl_mif_allocate_and_read_mif_file( engine->error, memory_filename, "APB target SRAM", 256, 32, 0, &memory );

     main_clock_state.data_out = 0;
     main_clock_state.reading = 0;
     main_clock_state.writing = 0;
     main_clock_state.address = 0;
     main_clock_state.write_data = 0xdeaddead;

     return error_level_okay;
}

/*f c_apb_target_sram::preclock
 */
t_sl_error_level c_apb_target_sram::preclock( void )
{
     memcpy( &next_main_clock_state, &main_clock_state, sizeof(main_clock_state) );

     next_main_clock_state.reading = 0;
     next_main_clock_state.writing = 0;
     if (inputs.enable[0] && inputs.select[0] && !inputs.read_not_write[0])
     {
          next_main_clock_state.writing = 1;
          next_main_clock_state.address = inputs.address[0]&0xff;
          next_main_clock_state.write_data = inputs.data_in[0];
          DEBUG( ( debug_level_info,
                   "c_apb_target_sram::preclock",
                   "APB target_sram '%s' write address 0x%02x data 0x%08x",
                   engine->get_instance_name( engine_handle),
                   inputs.address[0] & 0xff,
                   inputs.data_in[0] ) );
     }
     if (inputs.select[0] && inputs.read_not_write[0])
     {
          next_main_clock_state.reading = 1;
          next_main_clock_state.address = inputs.address[0]&0xff;
          if (memory)
          {
               next_main_clock_state.data_out = memory[next_main_clock_state.address];
          }
          else
          {
               next_main_clock_state.data_out = 0xdeadface;
          }

          DEBUG( ( debug_level_info,
                   "c_apb_target_sram::preclock",
                   "APB target_sram '%s' read address %d data %08x",
                   engine->get_instance_name( engine_handle),
                   inputs.address[0] & 0xff,
                   next_main_clock_state.data_out ) );
     }
     return error_level_okay;
}

/*f c_apb_target_sram::clock
 */
t_sl_error_level c_apb_target_sram::clock( void )
{
     memcpy( &main_clock_state, &next_main_clock_state, sizeof(main_clock_state) );
     if ( (main_clock_state.writing) && memory )
     {
          memory[main_clock_state.address] = main_clock_state.write_data;
     }
     return error_level_okay;
}

/*a Initialization functions
 */
/*f apb_target_sram__init
 */
extern void apb_target_sram__init( void )
{
     se_external_module_register( 1, "apb_target_sram", instance_fn );
}

/*a Scripting support code
 */
/*f initapb_target_sram
 */
extern "C" void initapb_target_sram( void )
{
     apb_target_sram__init( );
     scripting_init_module( "apb_target_sram" );
}
