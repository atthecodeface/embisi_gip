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
#include "sl_mif.h"
#include "sl_general.h"
#include "be_model_includes.h"
#include "gip.h"

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
/*t t_gipc_prefetch_if_comp_posedge_int_clock_state
*/
typedef struct t_gipc_prefetch_if_comp_posedge_int_clock_state
{
    unsigned int address;
    unsigned int last_prefetch_address;
    int data_valid;
    unsigned int data;

    int fetches_not_expected;

    unsigned int fetch_16;
    unsigned int fetch_op;

    unsigned int fetch_data_valid;
    unsigned int fetch_data;
    unsigned int fetch_pc;

    unsigned int prefetch_op;
    unsigned int prefetch_address;
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

/*t c_gipc_prefetch_if_comp
*/
class c_gipc_prefetch_if_comp
{
public:
    c_gipc_prefetch_if_comp::c_gipc_prefetch_if_comp( class c_engine *eng, void *eng_handle );
    c_gipc_prefetch_if_comp::~c_gipc_prefetch_if_comp();
    t_sl_error_level c_gipc_prefetch_if_comp::delete_instance( void );
    t_sl_error_level c_gipc_prefetch_if_comp::reset( int pass );
    t_sl_error_level c_gipc_prefetch_if_comp::preclock_posedge_int_clock( void );
    t_sl_error_level c_gipc_prefetch_if_comp::clock_posedge_int_clock( void );
    t_sl_error_level c_gipc_prefetch_if_comp::reset_active_high_int_reset( void );
private:
    int c_gipc_prefetch_if_comp::check_memory_value( unsigned int data, unsigned int address );

    c_engine *engine;
    void *engine_handle;

    char *mif_filename;
    unsigned int memory_size;
    unsigned int *memory;

    int cycle_number;
    int verbose_level;
    const char *failure;
    int fetch_unexpected;

    t_gipc_prefetch_if_comp_inputs inputs;

    t_gipc_prefetch_if_comp_posedge_int_clock_state next_posedge_int_clock_state;
    t_gipc_prefetch_if_comp_posedge_int_clock_state posedge_int_clock_state;

};

/*a Static function declarations
 */

/*a Statics
 */
/*v fetch_ops, prefetch_ops
 */
static char *fetch_ops[4];
static char *prefetch_ops[4];

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

/*a Constructors and destructors for gipc_prefetch_if_comp
*/
/*f c_gipc_prefetch_if_comp::c_gipc_prefetch_if_comp
  option for verbosity
  option for size of memory, only useful if mif filename is given
  option for mif filename for loading to compare with returned data
*/
c_gipc_prefetch_if_comp::c_gipc_prefetch_if_comp( class c_engine *eng, void *eng_handle )
{

    /*b Set main variables
     */
    engine = eng;
    engine_handle = eng_handle;

    /*b Done
     */
    verbose_level = engine->get_option_int( engine_handle, "verbose_level", 0 );
    memory_size = engine->get_option_int( engine_handle, "memory_size", 0 );
    mif_filename = engine->get_option_string( engine_handle, "mif_file", NULL );
    if (mif_filename)
    {
        mif_filename = sl_str_alloc_copy( mif_filename );
    }
    memory = NULL;
    if (mif_filename && memory_size)
    {
        sl_mif_allocate_and_read_mif_file( engine->error,
                                           strcmp(mif_filename,"")?mif_filename:NULL,
                                           "gipc_prefetch_if_comp",
                                           memory_size,
                                           32,
                                           0,
                                           (int **)&memory,
                                           NULL,
                                           NULL );
    }

    engine->register_delete_function( engine_handle, (void *)this, gipc_prefetch_if_comp_delete_fn );
    engine->register_reset_function( engine_handle, (void *)this, gipc_prefetch_if_comp_reset_fn );

    engine->register_clock_fns( engine_handle, (void *)this, "int_clock", gipc_prefetch_if_comp_preclock_posedge_int_clock_fn, gipc_prefetch_if_comp_clock_posedge_int_clock_fn );

    engine->register_input_signal( engine_handle, "int_reset", 1, (int **)&inputs.int_reset );

    INPUT( fetch_16, 1, int_clock );
    INPUT( fetch_op, 2, int_clock );
    INPUT( fetch_data_valid, 1, int_clock );
    INPUT( fetch_data, 32, int_clock );
    INPUT( fetch_pc, 32, int_clock );

    INPUT( prefetch_op, 2, int_clock );
    INPUT( prefetch_address, 32, int_clock );

//    engine->register_state_desc( engine_handle, 1, state_desc_gipc_prefetch_if_comp_posedge_int_clock, &posedge_int_clock_state, NULL );
    cycle_number = 0;
    reset_active_high_int_reset();
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
    if (memory)
    {
        free(memory);
        memory = NULL;
    }
    return error_level_okay;
}

/*a Class reset/preclock/clock methods for gipc_prefetch_if_comp
*/
/*f c_gipc_prefetch_if_comp::reset
*/
t_sl_error_level c_gipc_prefetch_if_comp::reset( int pass )
{
    if (pass==0)
    {
        cycle_number = -1;
        reset_active_high_int_reset();
        cycle_number = 0;
    }
    return error_level_okay;
}

/*f c_gipc_prefetch_if_comp::reset_active_high_int_reset
*/
t_sl_error_level c_gipc_prefetch_if_comp::reset_active_high_int_reset( void )
{
    posedge_int_clock_state.address = 0;
    posedge_int_clock_state.data_valid = 0;
    posedge_int_clock_state.data = 0;
    posedge_int_clock_state.fetches_not_expected = 0;

    failure = NULL;
    return error_level_okay;
}

/*f c_gipc_prefetch_if_comp::check_memory_value
  Returns 0 if the data matches (or is not present); 1 if it does not
 */
int c_gipc_prefetch_if_comp::check_memory_value( unsigned int data, unsigned int address )
{
    if (memory)
    {
        address = address>>2;
        if (address<memory_size)
        {
            return (memory[address] != data);
        }
    }
    return 0;
}

/*f c_gipc_prefetch_if_comp::preclock_posedge_int_clock
*/
t_sl_error_level c_gipc_prefetch_if_comp::preclock_posedge_int_clock( void )
{
    /*b Copy current state to next
     */
    memcpy( &next_posedge_int_clock_state, &posedge_int_clock_state, sizeof(posedge_int_clock_state) );

    /*b Monitor fetch
     */
    failure = NULL;
    fetch_unexpected = 0;
    if (posedge_int_clock_state.fetches_not_expected & (1<<inputs.fetch_op[0]))
    {
        fetch_unexpected = 1;
    }
    switch (inputs.fetch_op[0])
    {
    case gip_fetch_op_this_prefetch:
        next_posedge_int_clock_state.fetches_not_expected = 0;
        if (inputs.fetch_data_valid[0])
        {
            failure = "unexpected data valid on 'this' fetch";
        }
        next_posedge_int_clock_state.address = 0;
        if (inputs.prefetch_op[0] == gip_prefetch_op_new_address)
        {
            next_posedge_int_clock_state.address = inputs.prefetch_address[0];
        }
        break;
    case gip_fetch_op_last_prefetch:
        if (inputs.fetch_data_valid[0])
        {
            next_posedge_int_clock_state.fetches_not_expected = 1<<gip_fetch_op_last_prefetch;
            if (inputs.fetch_pc[0] != posedge_int_clock_state.address)
            {
                failure = "unexpected address on 'last' fetch";
            }
            if (check_memory_value(inputs.fetch_data[0],posedge_int_clock_state.address))
            {
                if (failure)
                {
                    failure = "unexpected address and incorrect data on 'last' fetch";
                }
                else
                {
                    failure = "incorrect data on 'last' fetch";
                }
            }
        }
        next_posedge_int_clock_state.address = posedge_int_clock_state.last_prefetch_address;
        break;
    case gip_fetch_op_hold:
        if (inputs.fetch_data[0])
        {
            next_posedge_int_clock_state.fetches_not_expected = 0;
        }
        if (!inputs.fetch_data_valid[0] && posedge_int_clock_state.data_valid)
        {
            failure = "data valid not held for 'hold' fetch";
        }
        if ( posedge_int_clock_state.data_valid )
        {
            if ( inputs.fetch_data[0] != posedge_int_clock_state.data )
            {
                failure = "data changed despite 'hold' fetch";
            }
            if ( inputs.fetch_pc[0] != posedge_int_clock_state.address )
            {
                failure = "address changed despite 'hold' fetch";
            }
        }
        break;
    case gip_fetch_op_sequential:
        if (inputs.fetch_data_valid[0])
        {
            next_posedge_int_clock_state.fetches_not_expected = 0;
            next_posedge_int_clock_state.address = posedge_int_clock_state.address+(inputs.fetch_16[0] ? 2 : 4 );
            if (inputs.fetch_pc[0] != next_posedge_int_clock_state.address)
            {
                failure = "unexpected address on 'seq' fetch";
            }
            if (check_memory_value(inputs.fetch_data[0],next_posedge_int_clock_state.address))
            {
                if (failure)
                {
                    failure = "unexpected address and incorrect data on 'seq' fetch";
                }
                else
                {
                    failure = "incorrect data on 'seq' fetch";
                }
            }
        }
        break;
    }
    if (inputs.prefetch_op[0] == gip_prefetch_op_new_address)
    {
        next_posedge_int_clock_state.last_prefetch_address = inputs.prefetch_address[0];
    }
    next_posedge_int_clock_state.data_valid = inputs.fetch_data_valid[0];
    next_posedge_int_clock_state.data = inputs.fetch_data[0];

    /*b Record inputs
     */
    next_posedge_int_clock_state.fetch_16 = inputs.fetch_16[0];
    next_posedge_int_clock_state.fetch_op = inputs.fetch_op[0];
    next_posedge_int_clock_state.fetch_data_valid = inputs.fetch_data_valid[0];
    next_posedge_int_clock_state.fetch_data = inputs.fetch_data[0];
    next_posedge_int_clock_state.fetch_pc = inputs.fetch_pc[0];
    next_posedge_int_clock_state.prefetch_op = inputs.prefetch_op[0];
    next_posedge_int_clock_state.prefetch_address = inputs.prefetch_address[0];

    /*b Done
     */
    return error_level_okay;
}

/*f c_gipc_prefetch_if_comp::clock_posedge_int_clock
*/
t_sl_error_level c_gipc_prefetch_if_comp::clock_posedge_int_clock( void )
{

    /*b Copy next state to current
     */
    memcpy( &posedge_int_clock_state, &next_posedge_int_clock_state, sizeof(posedge_int_clock_state) );
    cycle_number++;

    /*b Print error message if required
     */
    if (fetch_unexpected)
    {
        fprintf( stderr, "%s:%d:Unexpected fetch request from prefetch of GIP core\n", engine->get_instance_name(engine_handle), engine->cycle());
    }
    if (failure)
    {
        fprintf( stderr, "%s:%d:%s\n", engine->get_instance_name(engine_handle), engine->cycle(), failure);
    }
    if ( (verbose_level>0) || failure || fetch_unexpected)
    {
        fprintf( stderr, "%s:%d:f_op %s f_data (%c) %08x f_pc %08x add %08x p_op %s p_add %08x\n", engine->get_instance_name(engine_handle), engine->cycle(),
                 fetch_ops[posedge_int_clock_state.fetch_op],
                 posedge_int_clock_state.fetch_data_valid?'v':'i',
                 posedge_int_clock_state.fetch_data,
                 posedge_int_clock_state.fetch_pc,
                 posedge_int_clock_state.address,
                 prefetch_ops[posedge_int_clock_state.prefetch_op],
                 posedge_int_clock_state.prefetch_address );
    }

    /*b Return
     */
    return error_level_okay;
}

/*a Initialization functions
*/
/*f c_gipc_prefetch_if_comp__init
*/
extern void c_gipc_prefetch_if_comp__init( void )
{
    se_external_module_register( 1, "gipc_prefetch_if_comp", gipc_prefetch_if_comp_instance_fn );

    fetch_ops[gip_fetch_op_hold] = "hold";
    fetch_ops[gip_fetch_op_last_prefetch] = "last";
    fetch_ops[gip_fetch_op_this_prefetch] = "this";
    fetch_ops[gip_fetch_op_sequential] = "seq ";

    prefetch_ops[gip_prefetch_op_new_address] = "new ";
    prefetch_ops[gip_prefetch_op_none] = "none";
    prefetch_ops[gip_prefetch_op_sequential] = "seq ";
    prefetch_ops[gip_prefetch_op_hold] = "hold";
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

