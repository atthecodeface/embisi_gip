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
/*t t_gipc_pipeline_trace_posedge_int_clock_state
*/
typedef struct t_gipc_pipeline_trace_posedge_int_clock_state
{
    unsigned int pc;
    int executing;
    int valid;

    unsigned int rfw_data;
    unsigned int rfw_reg;
    unsigned int rfw_is_reg;

    int gip_ins_class;
    int gip_ins_subclass;
    int gip_ins_cc;
    int gip_ins_rd;
    int gip_ins_rn;
    int gip_ins_rm;
    int rm_is_imm;
    unsigned int immediate;
    int k;
    int a;
    int f;
    int s_or_stack;
    int p_or_offset_is_shift;
    int d;
    int tag;

} t_gipc_pipeline_trace_posedge_int_clock_state;

/*t t_gipc_pipeline_trace_ptrs
*/
typedef struct t_gipc_pipeline_trace_ptrs
{
    unsigned int *pc;
    unsigned int *executing;
    unsigned int *valid;

    unsigned int *rfw_data;
    unsigned int *rfw_r;
    unsigned int *rfw_type;

    int gip_ins_class;
    int gip_ins_subclass;
    int gip_ins_cc;
    int gip_ins_rd;
    int gip_ins_rn;
    int gip_ins_rm;
    int rm_is_imm;
    unsigned int immediate;
    int k;
    int a;
    int f;
    int s_or_stack;
    int p_or_offset_is_shift;
    int d;
    int tag;

} t_gipc_pipeline_trace_ptrs;

/*t c_gipc_pipeline_trace
*/
class c_gipc_pipeline_trace
{
public:
    c_gipc_pipeline_trace::c_gipc_pipeline_trace( class c_engine *eng, void *eng_handle );
    c_gipc_pipeline_trace::~c_gipc_pipeline_trace();
    t_sl_error_level c_gipc_pipeline_trace::delete_instance( void );
    t_sl_error_level c_gipc_pipeline_trace::reset( int pass );
    t_sl_error_level c_gipc_pipeline_trace::preclock_posedge_int_clock( void );
    t_sl_error_level c_gipc_pipeline_trace::clock_posedge_int_clock( void );
    t_sl_error_level c_gipc_pipeline_trace::reset_active_high_int_reset( void );
private:
    unsigned int *c_gipc_pipeline_trace::find_vector_from_path( char *gipc_path, char *vector_name );

    c_engine *engine;
    void *engine_handle;

    char *gipc_path;

    int cycle_number;
    int verbose_level;

    t_gipc_pipeline_trace_ptrs ptrs;

    t_gipc_pipeline_trace_posedge_int_clock_state next_posedge_int_clock_state;
    t_gipc_pipeline_trace_posedge_int_clock_state posedge_int_clock_state;

};

/*a Statics
 */
/*v fetch_ops, prefetch_ops
 */
static char *fetch_ops[4];
static char *prefetch_ops[4];

/*f state_desc_gipc_pipeline_trace
*/
#define struct_offset( ptr, a ) (((char *)&(ptr->a))-(char *)ptr)
static t_gipc_pipeline_trace_posedge_int_clock_state *___gipc_pipeline_trace_posedge_int_clock__ptr;
static t_engine_state_desc state_desc_gipc_pipeline_trace_posedge_int_clock[] =
{
//    {"src_fsm", engine_state_desc_type_bits, NULL, struct_offset(___gipc_pipeline_trace_posedge_int_clock__ptr, src_fsm), {2,0,0,0}, {NULL,NULL,NULL,NULL} },
//    {"src_left", engine_state_desc_type_bits, NULL, struct_offset(___gipc_pipeline_trace_posedge_int_clock__ptr, src_left), {5,0,0,0}, {NULL,NULL,NULL,NULL} },
//    {"fc_credit", engine_state_desc_type_bits, NULL, struct_offset(___gipc_pipeline_trace_posedge_int_clock__ptr, src_channel_state[0].fc_credit), {32,0,0,0}, {NULL,NULL,NULL,NULL} },
    {"", engine_state_desc_type_none, NULL, 0, {0,0,0,0}, {NULL,NULL,NULL,NULL} }
};

/*a Static wrapper functions for gipc_pipeline_trace
*/
/*f gipc_pipeline_trace_instance_fn
*/
static t_sl_error_level gipc_pipeline_trace_instance_fn( c_engine *engine, void *engine_handle )
{
    c_gipc_pipeline_trace *mod;
    mod = new c_gipc_pipeline_trace( engine, engine_handle );
    if (!mod)
        return error_level_fatal;
    return error_level_okay;
}

/*f gipc_pipeline_trace_delete_fn - simple callback wrapper for the main method
*/
static t_sl_error_level gipc_pipeline_trace_delete_fn( void *handle )
{
    c_gipc_pipeline_trace *mod;
    t_sl_error_level result;
    mod = (c_gipc_pipeline_trace *)handle;
    result = mod->delete_instance();
    delete( mod );
    return result;
}

/*f gipc_pipeline_trace_reset_fn
*/
static t_sl_error_level gipc_pipeline_trace_reset_fn( void *handle, int pass )
{
    c_gipc_pipeline_trace *mod;
    mod = (c_gipc_pipeline_trace *)handle;
    return mod->reset( pass );
}

/*f gipc_pipeline_trace_preclock_posedge_int_clock_fn
*/
static t_sl_error_level gipc_pipeline_trace_preclock_posedge_int_clock_fn( void *handle )
{
    c_gipc_pipeline_trace *mod;
    mod = (c_gipc_pipeline_trace *)handle;
    return mod->preclock_posedge_int_clock();
}

/*f gipc_pipeline_trace_clock_posedge_int_clock_fn
*/
static t_sl_error_level gipc_pipeline_trace_clock_posedge_int_clock_fn( void *handle )
{
    c_gipc_pipeline_trace *mod;
    mod = (c_gipc_pipeline_trace *)handle;
    return mod->clock_posedge_int_clock();
}

/*a Constructors and destructors for gipc_pipeline_trace
*/
/*f c_gipc_pipeline_trace::c_gipc_pipeline_trace
  option for verbosity
  option for path to GIP core
*/
c_gipc_pipeline_trace::c_gipc_pipeline_trace( class c_engine *eng, void *eng_handle )
{

    /*b Set main variables
     */
    engine = eng;
    engine_handle = eng_handle;

    /*b Done
     */
    verbose_level = engine->get_option_int( engine_handle, "verbose_level", 0 );
    gipc_path = engine->get_option_string( engine_handle, "gipc_path", NULL );

    engine->register_delete_function( engine_handle, (void *)this, gipc_pipeline_trace_delete_fn );
    engine->register_reset_function( engine_handle, (void *)this, gipc_pipeline_trace_reset_fn );

    engine->register_clock_fns( engine_handle, (void *)this, "int_clock", gipc_pipeline_trace_preclock_posedge_int_clock_fn, gipc_pipeline_trace_clock_posedge_int_clock_fn );

    ptrs.pc = NULL;

    cycle_number = 0;
    reset_active_high_int_reset();
}

/*f c_gipc_pipeline_trace::~c_gipc_pipeline_trace
*/
c_gipc_pipeline_trace::~c_gipc_pipeline_trace()
{
    delete_instance();
}

/*f c_gipc_pipeline_trace::delete_instance
*/
t_sl_error_level c_gipc_pipeline_trace::delete_instance( void )
{
    return error_level_okay;
}

/*a Class reset/preclock/clock methods for gipc_pipeline_trace
*/
/*f c_gipc_pipeline_trace::reset
*/
t_sl_error_level c_gipc_pipeline_trace::reset( int pass )
{
    if (pass==0)
    {
        cycle_number = -1;
        reset_active_high_int_reset();
        cycle_number = 0;
    }
    return error_level_okay;
}

/*f c_gipc_pipeline_trace::find_vector_from_path
*/
unsigned int *c_gipc_pipeline_trace::find_vector_from_path( char *gipc_path, char *vector_name )
{
    char buffer[1024];
    unsigned int *data_ptrs[4];
    int sizes[4];
    t_se_interrogation_handle handle;
    unsigned int *result;

    result = NULL;
    sprintf(buffer, "%s.%s", gipc_path, vector_name );
    handle = engine->find_entity( buffer );
    if (handle)
    {
        if (engine->interrogate_get_data_sizes_and_type( handle, (int **)data_ptrs, sizes )==engine_state_desc_type_bits)
        {
            result = data_ptrs[0];
        }
        engine->interrogation_handle_free( handle );
    }
    else
    {
        fprintf( stderr, "GIP core pipeline trace could not find the path '%s'\n", buffer );
    }
    return result;
}

/*f c_gipc_pipeline_trace::reset_active_high_int_reset
*/
t_sl_error_level c_gipc_pipeline_trace::reset_active_high_int_reset( void )
{
    if (!ptrs.pc)
    {
        ptrs.pc = find_vector_from_path( gipc_path, "alu.alu_inst__pc" );
        ptrs.valid = find_vector_from_path( gipc_path, "alu.alu_inst__valid" );
        ptrs.executing = find_vector_from_path( gipc_path, "gip_pipeline_executing" );
        ptrs.rfw_data = find_vector_from_path( gipc_path, "rf.rfw_data" );
        ptrs.rfw_r = find_vector_from_path( gipc_path, "rf.rfw_rd__r" );
        ptrs.rfw_type = find_vector_from_path( gipc_path, "rf.rfw_rd__type" );
    }
    posedge_int_clock_state.pc = 0;
    posedge_int_clock_state.valid = 0;
    posedge_int_clock_state.executing = 0;

    posedge_int_clock_state.rfw_data = 0;
    posedge_int_clock_state.rfw_reg = 0;
    posedge_int_clock_state.rfw_is_reg = 0;

    return error_level_okay;
}

/*f c_gipc_pipeline_trace::preclock_posedge_int_clock
*/
t_sl_error_level c_gipc_pipeline_trace::preclock_posedge_int_clock( void )
{
    /*b Copy current state to next
     */
    memcpy( &next_posedge_int_clock_state, &posedge_int_clock_state, sizeof(posedge_int_clock_state) );

    /*b Record inputs
     */
    if (ptrs.pc) next_posedge_int_clock_state.pc = ptrs.pc[0];
    if (ptrs.valid) next_posedge_int_clock_state.valid = ptrs.valid[0];
    if (ptrs.executing) next_posedge_int_clock_state.executing = ptrs.executing[0];
        
    if (ptrs.rfw_data) next_posedge_int_clock_state.rfw_data = ptrs.rfw_data[0];
    if (ptrs.rfw_r) next_posedge_int_clock_state.rfw_reg = ptrs.rfw_r[0];
    if (ptrs.rfw_type) next_posedge_int_clock_state.rfw_is_reg = (ptrs.rfw_type[0]==0);

    /*b Done
     */
    return error_level_okay;
}

/*f c_gipc_pipeline_trace::clock_posedge_int_clock
*/
t_sl_error_level c_gipc_pipeline_trace::clock_posedge_int_clock( void )
{

    /*b Copy next state to current
     */
    memcpy( &posedge_int_clock_state, &next_posedge_int_clock_state, sizeof(posedge_int_clock_state) );
    cycle_number++;

    /*b Print error message if required
     */
    if (posedge_int_clock_state.valid)
    {
        fprintf( stderr, "%s:%d:Pc %08x e %d\n", engine->get_instance_name(engine_handle), engine->cycle(), posedge_int_clock_state.pc-8, posedge_int_clock_state.executing );
    }
    if (posedge_int_clock_state.rfw_is_reg)
    {
        fprintf( stderr, "%s:%d:RFW %d %08x\n", engine->get_instance_name(engine_handle), engine->cycle(), posedge_int_clock_state.rfw_reg, posedge_int_clock_state.rfw_data );
    }

    /*b Return
     */
    return error_level_okay;
}

/*a Initialization functions
*/
/*f c_gipc_pipeline_trace__init
*/
extern void c_gipc_pipeline_trace__init( void )
{
    se_external_module_register( 1, "gipc_pipeline_trace", gipc_pipeline_trace_instance_fn );

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
/*f initgipc_pipeline_trace
*/
extern "C" void initgipc_pipeline_trace( void )
{
    c_gipc_pipeline_trace__init( );
    scripting_init_module( "gipc_pipeline_trace" );
}

/*a Editor preferences and notes
mode: c ***
c-basic-offset: 4 ***
c-default-style: (quote ((c-mode . "k&r") (c++-mode . "k&r"))) ***
outline-regexp: "/\\\*a\\\|[\t ]*\/\\\*[b-z][\t ]" ***
*/

