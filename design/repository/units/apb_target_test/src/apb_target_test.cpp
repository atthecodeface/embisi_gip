/*a Includes
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sl_debug.h"
#include "c_sl_error.h"
#include "c_se_engine.h"
#include "se_scripting.h"
#include "se_external_module.h"

/*a Types
 */
typedef struct t_main_clock_state
{
     int data_out;
     int test_store[8];
} t_main_clock_state;
typedef struct t_inputs
{
     int *enable;
     int *select;
     int *read_not_write;
     int *address;
     int *data_in;
} t_inputs;

/*t	c_apb_target_test
*/
class c_apb_target_test
{
public:
     c_apb_target_test::c_apb_target_test( class c_engine *eng, void *eng_handle );
     c_apb_target_test::~c_apb_target_test();
     t_sl_error_level c_apb_target_test::delete_instance( void );
     t_sl_error_level c_apb_target_test::reset( void );
     t_sl_error_level c_apb_target_test::preclock( void );
     t_sl_error_level c_apb_target_test::clock( void );
private:
     c_engine *engine;
     void *engine_handle;
     t_inputs inputs;
     t_main_clock_state next_main_clock_state;
     t_main_clock_state main_clock_state;
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
     {"test_store", engine_state_desc_type_memory, NULL, struct_offset(_ptr, test_store), {32,8,0,0}, {NULL,NULL,NULL,NULL} },
     {"", engine_state_desc_type_none, NULL, 0, {0,0,0,0}, {NULL,NULL,NULL,NULL} }
};

/*a Static wrapper functions
 */
/*f instance_fn
 */
static t_sl_error_level instance_fn( c_engine *engine, void *engine_handle )
{
     c_apb_target_test *mod;
     mod = new c_apb_target_test( engine, engine_handle );
     if (!mod)
          return error_level_fatal;
     return error_level_okay;
}

/*f delete_fn - simple callback wrapper for the main method
 */
static t_sl_error_level delete_fn( void *handle )
{
     c_apb_target_test *mod;
     t_sl_error_level result;
     mod = (c_apb_target_test *)handle;
     result = mod->delete_instance();
     delete( mod );
     return result;
}

/*f reset_fn
 */
static t_sl_error_level reset_fn( void *handle )
{
     c_apb_target_test *mod;
     mod = (c_apb_target_test *)handle;
     return mod->reset();
}

/*f preclock_fn
 */
static t_sl_error_level preclock_fn( void *handle )
{
     c_apb_target_test *mod;
     mod = (c_apb_target_test *)handle;
     return mod->preclock();
}

/*f clock_fn
 */
static t_sl_error_level clock_fn( void *handle )
{
     c_apb_target_test *mod;
     mod = (c_apb_target_test *)handle;
     return mod->clock();
}

/*a Constructors and destructors
 */
/*f c_apb_target_test::c_apb_target_test
 */
c_apb_target_test::c_apb_target_test( class c_engine *eng, void *eng_handle )
{
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
}

/*f c_apb_target_test::~c_apb_target_test
 */
c_apb_target_test::~c_apb_target_test()
{
     delete_instance();
}

/*f c_apb_target_test::delete_instance
 */
t_sl_error_level c_apb_target_test::delete_instance( void )
{
     return error_level_okay;
}

/*a Class reset/preclock/clock methods
 */
/*f c_apb_target_test::reset
 */
t_sl_error_level c_apb_target_test::reset( void )
{
     int i;
     for (i=0; i<8; i++)
     {
          main_clock_state.test_store[i] = 0;
     }
     main_clock_state.data_out = 0;
     return error_level_okay;
}

/*f c_apb_target_test::preclock
 */
t_sl_error_level c_apb_target_test::preclock( void )
{
     memcpy( &next_main_clock_state, &main_clock_state, sizeof(main_clock_state) );

     if (inputs.enable[0] && inputs.select[0] && !inputs.read_not_write[0])
     {
          next_main_clock_state.test_store[ inputs.address[0]&7 ] = inputs.data_in[0];
          DEBUG( ( debug_level_info,
                   "c_apb_target_test::preclock",
                   "APB target '%s' write address %d data %08x",
                   engine->get_instance_name( engine_handle),
                   inputs.address[0] & 7,
                   inputs.data_in[0] ) );
     }
     if (inputs.select[0] && inputs.read_not_write[0])
     {
          next_main_clock_state.data_out = main_clock_state.test_store[ inputs.address[0]&7 ];
          DEBUG( ( debug_level_info,
                   "c_apb_target_test::preclock",
                   "APB target '%s' read address %d data %08x",
                   engine->get_instance_name( engine_handle),
                   inputs.address[0] & 7,
                   next_main_clock_state.data_out ) );
     }
     return error_level_okay;
}

/*f c_apb_target_test::clock
 */
t_sl_error_level c_apb_target_test::clock( void )
{
     memcpy( &main_clock_state, &next_main_clock_state, sizeof(main_clock_state) );
     return error_level_okay;
}

/*a Initialization functions
 */
/*f apb_target_test__init
 */
extern void apb_target_test__init( void )
{
     se_external_module_register( 1, "apb_target_test", instance_fn );
}

/*a Scripting support code
 */
/*f initapb_target_test
 */
extern "C" void initapb_target_test( void )
{
     apb_target_test__init( );
     scripting_init_module( "apb_target_test" );
}
