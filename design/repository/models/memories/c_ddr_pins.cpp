/*a Copyright Gavin J Stark
 */

/*a Documentation

    This basically connects a ddr controller to a single ddr dram (it could be enhanced for many ddr drams - not yet!)
    The concept is of a ddr control signal clock which clocks controls and the data strobes
    Plus a ddr data clock that lags the control clock so that data capture and output can be performed properly.
    Note that this is really a cycle simulation 'hack' of sorts, but it does mirror reality.
    The strobes can be used to capture the data at the ddr, and if the DDR generates data on its ddr clock edge it would need to be captured on a delay locked clock
    The DDR controller drives out next_* signals, which are to be registered in the pins on the device; this model mimics those pins
    The DDR drives out actual values which are registered in the pins on the device; this model mimics those registers

  On rising ddr clock we drive the control outputs, registering the 'next_' signals - cke, select, ras, cas, we, ba, a
  On rising ddr clock we drive the data strobe outputs with next_dqs_high, next_dqoe; also store next_dqs_low, next_dq/dqm_high/low
  On rising ddr data clock ('just' after ddr clock rising) drive data and mask outputs with stored high values
  On rising ddr data clock ('just' after ddr clock rising) capture data_in as dq_in_high
  On falling ddr clock we drive the data strobe outputs with the stored dqs_low
  On falling ddr data clock ('just' after ddr clock falling) drive data and mask outputs with stored low values
  On falling ddr data clock ('just' after ddr clock falling) capture data_in as dq_in_low

  On both edges of ddr clock we test for two drivers of dqoe, and warn if both drive the bus
 */

/*a Includes
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "be_model_includes.h"

/*a Defines
 */
#define ERROR(a,args...) {fprintf(stderr,"%s:%d:"  a "\n", engine->get_instance_name(engine_handle), engine->cycle(), args );}
#define MAX_INT_WIDTH (8)
#define CLOCKED_INPUT( name, width, clk ) \
    { \
        engine->register_input_signal( engine_handle, #name, width, (int **)&inputs.name ); \
        engine->register_input_used_on_clock( engine_handle, #name, #clk, 1 ); \
    }

#define DDR_CLOCKED_INPUT( name, width, clk ) \
    { \
        engine->register_input_signal( engine_handle, #name, width, (int **)&inputs.name ); \
        engine->register_input_used_on_clock( engine_handle, #name, #clk, 1 ); \
        engine->register_input_used_on_clock( engine_handle, #name, #clk, 0 ); \
    }

#define DDR_CLOCK_STATE_OUTPUT( name, width, clk )\
    { \
        engine->register_output_signal( engine_handle, #name, width, (int *)&posedge_ddr_clock_state.name ); \
        engine->register_output_generated_on_clock( engine_handle, #name, #clk, 1 ); \
    }

#define DDR_BIEDGE_CLOCK_STATE_OUTPUT( name, width, clk )\
    { \
        engine->register_output_signal( engine_handle, #name, width, (int *)&biedge_ddr_clock_state.name ); \
        engine->register_output_generated_on_clock( engine_handle, #name, #clk, 1 ); \
    }

#define DDR_DATA_POSEDGE_CLOCK_STATE_OUTPUT( name, width, clk )\
    { \
        engine->register_output_signal( engine_handle, #name, width, (int *)&posedge_ddr_data_clock_state.name ); \
        engine->register_output_generated_on_clock( engine_handle, #name, #clk, 1 ); \
    }

#define DDR_DATA_NEGEDGE_CLOCK_STATE_OUTPUT( name, width, clk )\
    { \
        engine->register_output_signal( engine_handle, #name, width, (int *)&negedge_ddr_data_clock_state.name ); \
        engine->register_output_generated_on_clock( engine_handle, #name, #clk, 0 ); \
    }

#define DDR_DATA_BIEDGE_CLOCK_STATE_OUTPUT( name, width, clk )\
    { \
        engine->register_output_signal( engine_handle, #name, width, (int *)&biedge_ddr_data_clock_state.name ); \
        engine->register_output_generated_on_clock( engine_handle, #name, #clk, 1 ); \
    }


/*a Constants
 */

/*a Types for c_ddr_pins
*/
/*t t_ddr_pins_posedge_ddr_clock_state
*/
typedef struct t_ddr_pins_posedge_ddr_clock_state
{
    unsigned int cke;
    unsigned int select_n;
    unsigned int ras_n;
    unsigned int cas_n;
    unsigned int we_n;
    unsigned int ba;
    unsigned int a;
    unsigned int dqoe_out;
    unsigned int next_dqs_out_low;
    unsigned int next_dq_out_high;
    unsigned int next_dqm_out_high;
    unsigned int next_dq_out_low;
    unsigned int next_dqm_out_low;
} t_ddr_pins_posedge_ddr_clock_state;

/*t t_ddr_pins_biedge_ddr_clock_state
*/
typedef struct t_ddr_pins_biedge_ddr_clock_state
{
    unsigned int dqs_out;
} t_ddr_pins_biedge_ddr_clock_state;

/*t t_ddr_pins_negedge_ddr_data_clock_state
*/
typedef struct t_ddr_pins_negedge_ddr_data_clock_state
{
    unsigned int dq_in_low;
} t_ddr_pins_negedge_ddr_data_clock_state;

/*t t_ddr_pins_posedge_ddr_data_clock_state
*/
typedef struct t_ddr_pins_posedge_ddr_data_clock_state
{
    unsigned int dq_in_high;
} t_ddr_pins_posedge_ddr_data_clock_state;

/*t t_ddr_pins_biedge_ddr_data_clock_state
*/
typedef struct t_ddr_pins_biedge_ddr_data_clock_state
{
    unsigned int dq_out;
    unsigned int dqm_out;
} t_ddr_pins_biedge_ddr_data_clock_state;

/*t t_ddr_pins_inputs
*/
typedef struct t_ddr_pins_inputs
{
    unsigned int *next_cke;
    unsigned int *next_select_n;
    unsigned int *next_ras_n;
    unsigned int *next_cas_n;
    unsigned int *next_we_n;
    unsigned int *next_a;
    unsigned int *next_ba;
    unsigned int *next_dqoe_out;
    unsigned int *next_dqs_out_high;
    unsigned int *next_dqs_out_low;
    unsigned int *next_dq_out_high;
    unsigned int *next_dq_out_low;
    unsigned int *next_dqm_out_high;
    unsigned int *next_dqm_out_low;
    unsigned int *dqoe_in;
    unsigned int *dq_in;
} t_ddr_pins_inputs;

/*t c_ddr_pins
*/
class c_ddr_pins
{
public:
    c_ddr_pins::c_ddr_pins( class c_engine *eng, void *eng_handle );
    c_ddr_pins::~c_ddr_pins();
    t_sl_error_level c_ddr_pins::delete_instance( void );
    t_sl_error_level c_ddr_pins::reset( int pass );
    t_sl_error_level c_ddr_pins::preclock_posedge_ddr_clock( void );
    t_sl_error_level c_ddr_pins::clock_posedge_ddr_clock( void );
    t_sl_error_level c_ddr_pins::preclock_negedge_ddr_clock( void );
    t_sl_error_level c_ddr_pins::clock_negedge_ddr_clock( void );
    void c_ddr_pins::biedge_ddr_preclock( int edge );
    void c_ddr_pins::biedge_ddr_clock( void );
    t_sl_error_level c_ddr_pins::preclock_posedge_ddr_data_clock( void );
    t_sl_error_level c_ddr_pins::clock_posedge_ddr_data_clock( void );
    t_sl_error_level c_ddr_pins::preclock_negedge_ddr_data_clock( void );
    t_sl_error_level c_ddr_pins::clock_negedge_ddr_data_clock( void );
    void c_ddr_pins::biedge_ddr_data_preclock( int edge );
    void c_ddr_pins::biedge_ddr_data_clock( void );
private:
    c_engine *engine;
    void *engine_handle;
    t_ddr_pins_inputs inputs;
    t_ddr_pins_posedge_ddr_clock_state next_posedge_ddr_clock_state;
    t_ddr_pins_posedge_ddr_clock_state posedge_ddr_clock_state;
    t_ddr_pins_biedge_ddr_clock_state next_biedge_ddr_clock_state;
    t_ddr_pins_biedge_ddr_clock_state biedge_ddr_clock_state;
    t_ddr_pins_posedge_ddr_data_clock_state next_posedge_ddr_data_clock_state;
    t_ddr_pins_posedge_ddr_data_clock_state posedge_ddr_data_clock_state;
    t_ddr_pins_negedge_ddr_data_clock_state next_negedge_ddr_data_clock_state;
    t_ddr_pins_negedge_ddr_data_clock_state negedge_ddr_data_clock_state;
    t_ddr_pins_biedge_ddr_data_clock_state next_biedge_ddr_data_clock_state;
    t_ddr_pins_biedge_ddr_data_clock_state biedge_ddr_data_clock_state;

    char *filename;
    int debug;

    unsigned int ddr_size;
    int ddr_int_width; // width in ints
    int ddr_byte_width; // width in bytes

    int *memory; // mallocked array of width in bytes * size
};

/*a Static variables
 */

/*a Static wrapper functions for ddr
*/
/*f ddr_pins_instance_fn
*/
static t_sl_error_level ddr_pins_instance_fn( c_engine *engine, void *engine_handle )
{
    c_ddr_pins *mod;
    mod = new c_ddr_pins( engine, engine_handle );
    if (!mod)
        return error_level_fatal;
    return error_level_okay;
}

/*f ddr_pins_delete_fn - simple callback wrapper for the main method
*/
static t_sl_error_level ddr_pins_delete_fn( void *handle )
{
    c_ddr_pins *mod;
    t_sl_error_level result;
    mod = (c_ddr_pins *)handle;
    result = mod->delete_instance();
    delete( mod );
    return result;
}

/*f ddr_pins_reset_fn
*/
static t_sl_error_level ddr_pins_reset_fn( void *handle, int pass )
{
    c_ddr_pins *mod;
    mod = (c_ddr_pins *)handle;
    return mod->reset( pass );
}

/*f ddr_pins_preclock_negedge_ddr_clock_fn
*/
static t_sl_error_level ddr_pins_preclock_negedge_ddr_clock_fn( void *handle )
{
    c_ddr_pins *mod;
    mod = (c_ddr_pins *)handle;
    return mod->preclock_negedge_ddr_clock();
}

/*f ddr_pins_clock_negedge_ddr_clock_fn
*/
static t_sl_error_level ddr_pins_clock_negedge_ddr_clock_fn( void *handle )
{
    c_ddr_pins *mod;
    mod = (c_ddr_pins *)handle;
    return mod->clock_negedge_ddr_clock();
}

/*f ddr_pins_preclock_posedge_ddr_clock_fn
*/
static t_sl_error_level ddr_pins_preclock_posedge_ddr_clock_fn( void *handle )
{
    c_ddr_pins *mod;
    mod = (c_ddr_pins *)handle;
    return mod->preclock_posedge_ddr_clock();
}

/*f ddr_pins_clock_posedge_ddr_clock_fn
*/
static t_sl_error_level ddr_pins_clock_posedge_ddr_clock_fn( void *handle )
{
    c_ddr_pins *mod;
    mod = (c_ddr_pins *)handle;
    return mod->clock_posedge_ddr_clock();
}

/*f ddr_pins_preclock_negedge_ddr_data_clock_fn
*/
static t_sl_error_level ddr_pins_preclock_negedge_ddr_data_clock_fn( void *handle )
{
    c_ddr_pins *mod;
    mod = (c_ddr_pins *)handle;
    return mod->preclock_negedge_ddr_data_clock();
}

/*f ddr_pins_clock_negedge_ddr_data_clock_fn
*/
static t_sl_error_level ddr_pins_clock_negedge_ddr_data_clock_fn( void *handle )
{
    c_ddr_pins *mod;
    mod = (c_ddr_pins *)handle;
    return mod->clock_negedge_ddr_data_clock();
}

/*f ddr_pins_preclock_posedge_ddr_data_clock_fn
*/
static t_sl_error_level ddr_pins_preclock_posedge_ddr_data_clock_fn( void *handle )
{
    c_ddr_pins *mod;
    mod = (c_ddr_pins *)handle;
    return mod->preclock_posedge_ddr_data_clock();
}

/*f ddr_pins_clock_posedge_ddr_data_clock_fn
*/
static t_sl_error_level ddr_pins_clock_posedge_ddr_data_clock_fn( void *handle )
{
    c_ddr_pins *mod;
    mod = (c_ddr_pins *)handle;
    return mod->clock_posedge_ddr_data_clock();
}

/*a Constructors and destructors for ddr
*/
/*f c_ddr_pins::c_ddr_pins
*/
c_ddr_pins::c_ddr_pins( class c_engine *eng, void *eng_handle )
{
    engine = eng;
    engine_handle = eng_handle;

    /*b Record configuration
     */

    /*b Instantiate module
     */
    engine->register_delete_function( engine_handle, (void *)this, ddr_pins_delete_fn );
    engine->register_reset_function( engine_handle, (void *)this, ddr_pins_reset_fn );
    engine->register_clock_fns( engine_handle, (void *)this, "ddr_clock", ddr_pins_preclock_posedge_ddr_clock_fn, ddr_pins_clock_posedge_ddr_clock_fn, ddr_pins_preclock_negedge_ddr_clock_fn, ddr_pins_clock_negedge_ddr_clock_fn );
    engine->register_clock_fns( engine_handle, (void *)this, "ddr_data_clock", ddr_pins_preclock_posedge_ddr_data_clock_fn, ddr_pins_clock_posedge_ddr_data_clock_fn, ddr_pins_preclock_negedge_ddr_data_clock_fn, ddr_pins_clock_negedge_ddr_data_clock_fn );

    CLOCKED_INPUT( next_cke, 1, ddr_clock );
    CLOCKED_INPUT( next_select_n, 2, ddr_clock );
    CLOCKED_INPUT( next_ras_n, 1, ddr_clock );
    CLOCKED_INPUT( next_cas_n, 1, ddr_clock );
    CLOCKED_INPUT( next_we_n, 1, ddr_clock );
    CLOCKED_INPUT( next_ba, 2, ddr_clock );
    CLOCKED_INPUT( next_a, 13, ddr_clock );
    CLOCKED_INPUT( next_dqoe_out, 1, ddr_clock );
    CLOCKED_INPUT( next_dqs_out_high, 4, ddr_clock );
    CLOCKED_INPUT( next_dqs_out_low, 4, ddr_clock );
    CLOCKED_INPUT( next_dq_out_high, 16, ddr_clock );
    CLOCKED_INPUT( next_dq_out_low, 16, ddr_clock );
    CLOCKED_INPUT( next_dqm_out_high, 2, ddr_clock );
    CLOCKED_INPUT( next_dqm_out_low, 2, ddr_clock );

    DDR_CLOCKED_INPUT( dqoe_in, 1, ddr_data_clock );
    DDR_CLOCKED_INPUT( dq_in, 16, ddr_data_clock );

    DDR_CLOCK_STATE_OUTPUT( cke, 1, ddr_clock );
    DDR_CLOCK_STATE_OUTPUT( select_n, 2, ddr_clock );
    DDR_CLOCK_STATE_OUTPUT( ras_n, 1, ddr_clock );
    DDR_CLOCK_STATE_OUTPUT( cas_n, 1, ddr_clock );
    DDR_CLOCK_STATE_OUTPUT( we_n, 1, ddr_clock );
    DDR_CLOCK_STATE_OUTPUT( ba, 2, ddr_clock );
    DDR_CLOCK_STATE_OUTPUT( a, 13, ddr_clock );
    DDR_CLOCK_STATE_OUTPUT( dqoe_out, 1, ddr_clock );

    DDR_BIEDGE_CLOCK_STATE_OUTPUT( dqs_out, 2, ddr_clock );

    DDR_DATA_NEGEDGE_CLOCK_STATE_OUTPUT( dq_in_low, 16, ddr_data_clock );
    DDR_DATA_POSEDGE_CLOCK_STATE_OUTPUT( dq_in_high, 16, ddr_data_clock );
    DDR_DATA_BIEDGE_CLOCK_STATE_OUTPUT( dq_out, 16, ddr_data_clock );
    DDR_DATA_BIEDGE_CLOCK_STATE_OUTPUT( dqm_out, 2, ddr_data_clock );

    /*b Register state then reset
     */
    reset( 0 );

    /*b Done
     */

}

/*f c_ddr_pins::~c_ddr_pins
*/
c_ddr_pins::~c_ddr_pins()
{
    delete_instance();
}

/*f c_ddr_pins::delete_instance
*/
t_sl_error_level c_ddr_pins::delete_instance( void )
{
    return error_level_okay;
}

/*a Class reset/preclock/clock methods for ddr
*/
/*f c_ddr_pins::reset
*/
t_sl_error_level c_ddr_pins::reset( int pass )
{
    memset( &posedge_ddr_clock_state, 0, sizeof(posedge_ddr_clock_state) );
    memset( &biedge_ddr_clock_state, 0, sizeof(biedge_ddr_clock_state) );
    memset( &posedge_ddr_data_clock_state, 0, sizeof(posedge_ddr_data_clock_state) );
    memset( &negedge_ddr_data_clock_state, 0, sizeof(negedge_ddr_data_clock_state) );
    memset( &biedge_ddr_data_clock_state, 0, sizeof(biedge_ddr_data_clock_state) );

    return error_level_okay;
}

/*f c_ddr_pins::biedge_ddr_preclock
*/
void c_ddr_pins::biedge_ddr_preclock( int posedge )
{
    /*b Copy current state to next state
     */
    memcpy( &next_biedge_ddr_clock_state, &biedge_ddr_clock_state, sizeof(biedge_ddr_clock_state) );

    /*b Test dqoe's are not both driven
     */
    if (inputs.dqoe_in[0] && posedge_ddr_clock_state.dqoe_out)
    {
        ERROR("DDR pins simultaneously data driving by controller and memory (%d,%d)",1,1);
    }
    
    /*b Drive dqs
     */
    if (posedge)
    {
        next_biedge_ddr_clock_state.dqs_out = inputs.next_dqs_out_high[0];
    }
    else
    {
        next_biedge_ddr_clock_state.dqs_out = posedge_ddr_clock_state.next_dqs_out_low;
    }
}

/*f c_ddr_pins::biedge_ddr_clock
*/
void c_ddr_pins::biedge_ddr_clock( void )
{
    /*b Copy next state to current
    */
    memcpy( &biedge_ddr_clock_state, &next_biedge_ddr_clock_state, sizeof(biedge_ddr_clock_state) );

}

/*f c_ddr_pins::preclock_posedge_ddr_clock
*/
t_sl_error_level c_ddr_pins::preclock_posedge_ddr_clock( void )
{
    /*b First to the biedge stuff
     */
    biedge_ddr_preclock( 1 );

    /*b Copy current state to next state
     */
    memcpy( &next_posedge_ddr_clock_state, &posedge_ddr_clock_state, sizeof(posedge_ddr_clock_state) );

    /*b Record state
     */
    next_posedge_ddr_clock_state.cke = inputs.next_cke[0];
    next_posedge_ddr_clock_state.select_n = inputs.next_select_n[0];
    next_posedge_ddr_clock_state.ras_n = inputs.next_ras_n[0];
    next_posedge_ddr_clock_state.cas_n = inputs.next_cas_n[0];
    next_posedge_ddr_clock_state.we_n = inputs.next_we_n[0];
    next_posedge_ddr_clock_state.ba = inputs.next_ba[0];
    next_posedge_ddr_clock_state.a = inputs.next_a[0];
    next_posedge_ddr_clock_state.dqoe_out = inputs.next_dqoe_out[0];

    /*b Capture state in pin registers
     */
    next_posedge_ddr_clock_state.next_dqs_out_low = inputs.next_dqs_out_low[0];

    next_posedge_ddr_clock_state.next_dq_out_high = inputs.next_dq_out_high[0];
    next_posedge_ddr_clock_state.next_dq_out_low = inputs.next_dq_out_low[0];
    next_posedge_ddr_clock_state.next_dqm_out_high = inputs.next_dqm_out_high[0];
    next_posedge_ddr_clock_state.next_dqm_out_low = inputs.next_dqm_out_low[0];

    /*b Done
     */
    return error_level_okay;
}

/*f c_ddr_pins::clock_posedge_ddr_clock
*/
t_sl_error_level c_ddr_pins::clock_posedge_ddr_clock( void )
{

    /*b First to the biedge stuff
     */
    biedge_ddr_clock();

    /*b Copy next state to current
    */
    memcpy( &posedge_ddr_clock_state, &next_posedge_ddr_clock_state, sizeof(posedge_ddr_clock_state) );

    /*b Done
     */
    return error_level_okay;
}

/*f c_ddr_pins::preclock_negedge_ddr_clock
*/
t_sl_error_level c_ddr_pins::preclock_negedge_ddr_clock( void )
{
    /*b First to the biedge stuff
     */
    biedge_ddr_preclock(0);

    /*b Done
     */
    return error_level_okay;
}

/*f c_ddr_pins::clock_negedge_ddr_clock
*/
t_sl_error_level c_ddr_pins::clock_negedge_ddr_clock( void )
{

    /*b First to the biedge stuff
     */
    biedge_ddr_clock();

    /*b Done
     */
    return error_level_okay;
}

/*f c_ddr_pins::biedge_ddr_data_preclock
*/
void c_ddr_pins::biedge_ddr_data_preclock( int posedge )
{
    /*b Copy current state to next state
     */
    memcpy( &next_biedge_ddr_data_clock_state, &biedge_ddr_data_clock_state, sizeof(biedge_ddr_data_clock_state) );
    
    /*b Drive data and masks
     */
    if (posedge)
    {
        next_biedge_ddr_data_clock_state.dq_out = posedge_ddr_clock_state.next_dq_out_high;
        next_biedge_ddr_data_clock_state.dqm_out = posedge_ddr_clock_state.next_dqm_out_high;
    }
    else
    {
        next_biedge_ddr_data_clock_state.dq_out = posedge_ddr_clock_state.next_dq_out_low;
        next_biedge_ddr_data_clock_state.dqm_out = posedge_ddr_clock_state.next_dqm_out_low;
    }
}

/*f c_ddr_pins::biedge_ddr_data_clock
*/
void c_ddr_pins::biedge_ddr_data_clock( void )
{
    /*b Copy next state to current
    */
    memcpy( &biedge_ddr_data_clock_state, &next_biedge_ddr_data_clock_state, sizeof(biedge_ddr_data_clock_state) );

}

/*f c_ddr_pins::preclock_posedge_ddr_data_clock
*/
t_sl_error_level c_ddr_pins::preclock_posedge_ddr_data_clock( void )
{
    /*b First to the biedge stuff
     */
    biedge_ddr_data_preclock( 1 );

    /*b Copy current state to next state
     */
    memcpy( &next_posedge_ddr_data_clock_state, &posedge_ddr_data_clock_state, sizeof(posedge_ddr_data_clock_state) );

    /*b Record state
     */
    next_posedge_ddr_data_clock_state.dq_in_high = inputs.dq_in[0];

    /*b Done
     */
    return error_level_okay;
}

/*f c_ddr_pins::clock_posedge_ddr_data_clock
*/
t_sl_error_level c_ddr_pins::clock_posedge_ddr_data_clock( void )
{

    /*b First to the biedge stuff
     */
    biedge_ddr_data_clock();

    /*b Copy next state to current
    */
    memcpy( &posedge_ddr_data_clock_state, &next_posedge_ddr_data_clock_state, sizeof(posedge_ddr_data_clock_state) );

    /*b Done
     */
    return error_level_okay;
}

/*f c_ddr_pins::preclock_negedge_ddr_data_clock
*/
t_sl_error_level c_ddr_pins::preclock_negedge_ddr_data_clock( void )
{
    /*b First to the biedge stuff
     */
    biedge_ddr_data_preclock(0);

    /*b Record state
     */
    next_negedge_ddr_data_clock_state.dq_in_low = inputs.dq_in[0];

    /*b Done
     */
    return error_level_okay;
}

/*f c_ddr_pins::clock_negedge_ddr_data_clock
*/
t_sl_error_level c_ddr_pins::clock_negedge_ddr_data_clock( void )
{

    /*b First to the biedge stuff
     */
    biedge_ddr_data_clock();

    /*b Copy next state to current
    */
    memcpy( &negedge_ddr_data_clock_state, &next_negedge_ddr_data_clock_state, sizeof(negedge_ddr_data_clock_state) );

    /*b Done
     */
    return error_level_okay;
}

/*a Initialization functions
*/
/*f c_ddr_pins__init
*/
extern void c_ddr_pins__init( void )
{
    se_external_module_register( 1, "ddr_pins", ddr_pins_instance_fn );
}

/*a Scripting support code
*/
/*f initddr_pins
*/
extern "C" void initddr_pins( void )
{
    c_ddr_pins__init( );
    scripting_init_module( "ddr_pins" );
}
