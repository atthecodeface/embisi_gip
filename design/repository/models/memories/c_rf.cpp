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

/*a Defines
 */
#define MAX_INT_WIDTH (8)
#define CLOCKED_INPUT( name, width, clk ) \
    { \
        engine->register_input_signal( engine_handle, #name, width, (int **)&inputs.name ); \
        engine->register_input_used_on_clock( engine_handle, #name, #clk, 1 ); \
    }

#define COMB_INPUT( name, width ) \
    { \
        engine->register_input_signal( engine_handle, #name, width, (int **)&inputs.name ); \
        engine->register_comb_input( engine_handle, #name ); \
    }

#define COMB_OUTPUT( name, width )\
    { \
        engine->register_output_signal( engine_handle, #name, width, (int *)&combinatorials.name ); \
        engine->register_comb_output( engine_handle, #name ); \
    }

#define CLOCKED_OUTPUT( name, width, clk )\
    { \
        engine->register_output_signal( engine_handle, #name, width, (int *)&fast_clock_state.name ); \
        engine->register_output_generated_on_clock( engine_handle, #name, #clk, 1 ); \
    }


/*a Types for c_rf
*/
/*t t_rf_inputs
*/
typedef struct t_rf_inputs
{
    unsigned int *rf_reset;

    unsigned int *rf_clock_phase;

    unsigned int *rf_rd_addr_0;
    unsigned int *rf_rd_addr_1;

    unsigned int *rf_wr_enable;
    unsigned int *rf_wr_addr;
    unsigned int *rf_wr_data;
} t_rf_inputs;

/*t t_rf_combinatorials
*/
typedef struct t_rf_combinatorials
{
    unsigned int rf_rd_data_0[MAX_INT_WIDTH];
    unsigned int rf_rd_data_1[MAX_INT_WIDTH];
} t_rf_combinatorials;

/*t t_rf_fast_clock_state
*/
typedef struct t_rf_fast_clock_state
{
    unsigned int rf_rd_addr_0;
    unsigned int rf_rd_addr_1;
    unsigned int rf_rd_data_0[MAX_INT_WIDTH];
    unsigned int rf_rd_data_1[MAX_INT_WIDTH];
} t_rf_fast_clock_state;

/*t c_rf
*/
class c_rf
{
public:
    c_rf::c_rf( class c_engine *eng, void *eng_handle, int fast_clock, int size, int width, int read_ports, int write_ports );
    c_rf::~c_rf();
    t_sl_error_level c_rf::delete_instance( void );
    t_sl_error_level c_rf::reset( int pass );
    t_sl_error_level c_rf::comb( void );
    t_sl_error_level c_rf::preclock_posedge_int_clock( void );
    t_sl_error_level c_rf::clock_posedge_int_clock( void );
    t_sl_error_level c_rf::preclock_posedge_fast_clock( void );
    t_sl_error_level c_rf::clock_posedge_fast_clock( void );
private:
    c_engine *engine;
    void *engine_handle;
    t_rf_inputs inputs;
    t_rf_combinatorials combinatorials;
    t_rf_fast_clock_state fast_clock_state;

    int rf_fast_clock;

    int rf_read_ports;
    int rf_write_ports;
    unsigned int rf_size;
    int rf_log_size; // power of 2 >= rf_size
    int rf_width; // width in bits
    int rf_int_width; // width in ints
    int rf_byte_width; // width in bytes
    unsigned char *data; // mallocked array of width in bytes * size

    int rf_wr_enable;
    unsigned int rf_wr_addr;
    unsigned int rf_wr_data[MAX_INT_WIDTH];
};

/*a Static wrapper functions for rf
*/
/*f rf_1r_1w_32_32_instance_fn
*/
static t_sl_error_level rf_1r_1w_32_32_instance_fn( c_engine *engine, void *engine_handle )
{
    c_rf *mod;
    mod = new c_rf( engine, engine_handle, 0, 32, 32, 1, 1 );
    if (!mod)
        return error_level_fatal;
    return error_level_okay;
}

/*f rf_1r_1w_16_32_instance_fn
*/
static t_sl_error_level rf_1r_1w_16_32_instance_fn( c_engine *engine, void *engine_handle )
{
    c_rf *mod;
    mod = new c_rf( engine, engine_handle, 0, 16, 32, 1, 1 );
    if (!mod)
        return error_level_fatal;
    return error_level_okay;
}

/*f rf_1r_1w_16_4_instance_fn
*/
static t_sl_error_level rf_1r_1w_16_4_instance_fn( c_engine *engine, void *engine_handle )
{
    c_rf *mod;
    mod = new c_rf( engine, engine_handle, 0, 16, 4, 1, 1 );
    if (!mod)
        return error_level_fatal;
    return error_level_okay;
}

/*f rf_2r_1w_32_32_instance_fn
*/
static t_sl_error_level rf_2r_1w_32_32_instance_fn( c_engine *engine, void *engine_handle )
{
    c_rf *mod;
    mod = new c_rf( engine, engine_handle, 0, 32, 32, 2, 1 );
    if (!mod)
        return error_level_fatal;
    return error_level_okay;
}

/*f rf_2r_1w_32_32_fc_instance_fn
*/
static t_sl_error_level rf_2r_1w_32_32_fc_instance_fn( c_engine *engine, void *engine_handle )
{
    c_rf *mod;
    mod = new c_rf( engine, engine_handle, 1, 32, 32, 2, 1 );
    if (!mod)
        return error_level_fatal;
    return error_level_okay;
}

/*f rf_delete_fn - simple callback wrapper for the main method
*/
static t_sl_error_level rf_delete_fn( void *handle )
{
    c_rf *mod;
    t_sl_error_level result;
    mod = (c_rf *)handle;
    result = mod->delete_instance();
    delete( mod );
    return result;
}

/*f rf_reset_fn
*/
static t_sl_error_level rf_reset_fn( void *handle, int pass )
{
    c_rf *mod;
    mod = (c_rf *)handle;
    return mod->reset( pass );
}

/*f rf_comb_fn
*/
static t_sl_error_level rf_comb_fn( void *handle )
{
    c_rf *mod;
    mod = (c_rf *)handle;
    return mod->comb();
}

/*f rf_preclock_posedge_fast_clock_fn
*/
static t_sl_error_level rf_preclock_posedge_fast_clock_fn( void *handle )
{
    c_rf *mod;
    mod = (c_rf *)handle;
    return mod->preclock_posedge_fast_clock();
}

/*f rf_clock_posedge_fast_clock_fn
*/
static t_sl_error_level rf_clock_posedge_fast_clock_fn( void *handle )
{
    c_rf *mod;
    mod = (c_rf *)handle;
    return mod->clock_posedge_fast_clock();
}

/*f rf_preclock_posedge_int_clock_fn
*/
static t_sl_error_level rf_preclock_posedge_int_clock_fn( void *handle )
{
    c_rf *mod;
    mod = (c_rf *)handle;
    return mod->preclock_posedge_int_clock();
}

/*f rf_clock_posedge_int_clock_fn
*/
static t_sl_error_level rf_clock_posedge_int_clock_fn( void *handle )
{
    c_rf *mod;
    mod = (c_rf *)handle;
    return mod->clock_posedge_int_clock();
}

/*a Constructors and destructors for rf
*/
/*f c_rf::c_rf
*/
c_rf::c_rf( class c_engine *eng, void *eng_handle, int fast_clock, int size, int width, int read_ports, int write_ports )
{
    int i;

    engine = eng;
    engine_handle = eng_handle;

    /*b Determine configuration
     */
    for (i=0; i<32; i++)
    {
        if (size<=(1<<i))
            break;
    }
    rf_size = size;
    rf_log_size = i;
    rf_width = width;
    rf_byte_width = (width+7)/8;
    rf_int_width = (width+sizeof(int)*8-1)/(sizeof(int)*8);
    data = (unsigned char *)malloc(rf_byte_width*rf_size);

    rf_fast_clock = fast_clock;
    rf_read_ports = read_ports;
    rf_write_ports = write_ports;
    
    /*b Instantiate module
     */
    engine->register_delete_function( engine_handle, (void *)this, rf_delete_fn );
    engine->register_reset_function( engine_handle, (void *)this, rf_reset_fn );
    engine->register_clock_fns( engine_handle, (void *)this, "rf_clock", rf_preclock_posedge_int_clock_fn, rf_clock_posedge_int_clock_fn );
    if (fast_clock)
    {
        engine->register_clock_fns( engine_handle, (void *)this, "rf_fast_clock", rf_preclock_posedge_fast_clock_fn, rf_clock_posedge_fast_clock_fn );
    }
    engine->register_comb_fn( engine_handle, (void *)this, rf_comb_fn );

    CLOCKED_INPUT( rf_reset, 1, rf_clock );
    if (!rf_fast_clock)
    {
        CLOCKED_INPUT( rf_wr_enable, 1, rf_clock );
        CLOCKED_INPUT( rf_wr_addr, rf_log_size, rf_clock );
        CLOCKED_INPUT( rf_wr_data, rf_width, rf_clock );

        COMB_INPUT( rf_rd_addr_0, rf_log_size );
        COMB_OUTPUT( rf_rd_data_0, rf_width );
        if (read_ports>=2)
        {
            COMB_INPUT( rf_rd_addr_1, rf_log_size );
            COMB_OUTPUT( rf_rd_data_1, rf_width );
        }
    }
    else
    {
        CLOCKED_INPUT( rf_clock_phase, 1, rf_fast_clock );

        CLOCKED_INPUT( rf_wr_enable, 1, rf_fast_clock );
        CLOCKED_INPUT( rf_wr_addr, rf_log_size, rf_fast_clock );
        CLOCKED_INPUT( rf_wr_data, rf_width, rf_fast_clock );

        CLOCKED_INPUT( rf_rd_addr_0, rf_log_size, rf_fast_clock );
        CLOCKED_OUTPUT( rf_rd_data_0, rf_width, rf_fast_clock );
        if (read_ports>=2)
        {
            CLOCKED_INPUT( rf_rd_addr_1, rf_log_size, rf_fast_clock );
            CLOCKED_OUTPUT( rf_rd_data_1, rf_width, rf_fast_clock );
        }
    }

    /*b Register state then reset
     */
    //engine->register_state_desc( engine_handle, 1, state_desc_rf_posedge_int_clock, &posedge_int_clock_state, NULL );
    reset( 0 );

    /*b Done
     */

}

/*f c_rf::~c_rf
*/
c_rf::~c_rf()
{
    delete_instance();
}

/*f c_rf::delete_instance
*/
t_sl_error_level c_rf::delete_instance( void )
{
    return error_level_okay;
}

/*a Class reset/preclock/clock methods for rf
*/
/*f c_rf::reset
*/
t_sl_error_level c_rf::reset( int pass )
{
    return error_level_okay;
}

/*f c_rf::comb
*/
t_sl_error_level c_rf::comb( void )
{
    int i;

    if (rf_read_ports>0)
    {
        if (inputs.rf_rd_addr_0[0]<rf_size)
        {
            if (data)
            {
                for (i=0; i<rf_byte_width; i++)
                {
                    ((char *)(combinatorials.rf_rd_data_0))[i] = data[inputs.rf_rd_addr_0[0]*rf_byte_width+i];
                }
            }
        }
        else
        {
            fprintf(stderr,"c_rf::Out of range read port 0 (wanted %d of %d)\n", inputs.rf_rd_addr_0[0], rf_size);
        }
        //fprintf(stderr,"c_rf(%p)::clock_posedge_int_clock:Reading address %04x data %08x\n", this, posedge_int_clock_state.sram_address, posedge_int_clock_state.sram_read_data[0] );
    }
    if (rf_read_ports>1)
    {
        if (inputs.rf_rd_addr_1[0]<rf_size)
        {
            if (data)
            {
                for (i=0; i<rf_byte_width; i++)
                {
                    ((char *)(combinatorials.rf_rd_data_1))[i] = data[inputs.rf_rd_addr_1[0]*rf_byte_width+i];
                }
            }
        }
        else
        {
            fprintf(stderr,"c_rf::Out of range read port 1 (wanted %08x of %d)\n", inputs.rf_rd_addr_1[0], rf_size);
        }
        //fprintf(stderr,"c_rf(%p)::clock_posedge_int_clock:Reading address %04x data %08x\n", this, posedge_int_clock_state.sram_address, posedge_int_clock_state.sram_read_data[0] );
    }
    return error_level_okay;
}

/*f c_rf::preclock_posedge_fast_clock
*/
t_sl_error_level c_rf::preclock_posedge_fast_clock( void )
{
    int i;

    /*b Record inputs
     */
    rf_wr_enable = 0;
    if (inputs.rf_clock_phase[0])
    {
        rf_wr_enable = inputs.rf_wr_enable[0];
        rf_wr_addr = inputs.rf_wr_addr[0];
        for (i=0; i<rf_int_width; i++)
            rf_wr_data[i] = inputs.rf_wr_data[i];
    }

    fast_clock_state.rf_rd_addr_0 = inputs.rf_rd_addr_0[0];
    fast_clock_state.rf_rd_addr_1 = inputs.rf_rd_addr_1[0];

    /*b Done
     */
    return error_level_okay;
}

/*f c_rf::clock_posedge_fast_clock
*/
t_sl_error_level c_rf::clock_posedge_fast_clock( void )
{
    int i;

    /*b Read and write the rf if required
     */
    if (rf_wr_enable)
    {
        if (rf_wr_addr<rf_size)
        {
            if (data)
            {
                for (i=0; i<rf_byte_width; i++)
                {
                    data[rf_wr_addr*rf_byte_width+i] = ((char *)(rf_wr_data))[i];
                }
            }
        }
        else
        {
            fprintf(stderr,"c_rf::Out of range write\n");
        }
    }
    if (rf_read_ports>0)
    {
        if (fast_clock_state.rf_rd_addr_0<rf_size)
        {
            if (data)
            {
                for (i=0; i<rf_byte_width; i++)
                {
                    ((char *)(fast_clock_state.rf_rd_data_0))[i] = data[fast_clock_state.rf_rd_addr_0*rf_byte_width+i];
                }
            }
        }
        else
        {
            fprintf(stderr,"c_rf::Out of range read port 0 (wanted %d of %d)\n", fast_clock_state.rf_rd_addr_0, rf_size);
        }
        //fprintf(stderr,"c_rf(%p)::clock_posedge_int_clock:Reading address %04x data %08x\n", this, posedge_int_clock_state.sram_address, posedge_int_clock_state.sram_read_data[0] );
    }
    if (rf_read_ports>1)
    {
        if (fast_clock_state.rf_rd_addr_1<rf_size)
        {
            if (data)
            {
                for (i=0; i<rf_byte_width; i++)
                {
                    ((char *)(fast_clock_state.rf_rd_data_1))[i] = data[fast_clock_state.rf_rd_addr_1*rf_byte_width+i];
                }
            }
        }
        else
        {
            fprintf(stderr,"c_rf::Out of range read port 1 (wanted %08x of %d)\n", fast_clock_state.rf_rd_addr_1, rf_size);
        }
        //fprintf(stderr,"c_rf(%p)::clock_posedge_int_clock:Reading address %04x data %08x\n", this, posedge_int_clock_state.sram_address, posedge_int_clock_state.sram_read_data[0] );
    }

    /*b Done
     */
    return error_level_okay;
}

/*f c_rf::preclock_posedge_int_clock
*/
t_sl_error_level c_rf::preclock_posedge_int_clock( void )
{
    int i;

    /*b Record inputs
     */
    if (!rf_fast_clock)
    {
        rf_wr_enable = inputs.rf_wr_enable[0];
        rf_wr_addr = inputs.rf_wr_addr[0];
        for (i=0; i<rf_int_width; i++)
            rf_wr_data[i] = inputs.rf_wr_data[i];
    }

    /*b Done
     */
    return error_level_okay;
}

/*f c_rf::clock_posedge_int_clock
*/
t_sl_error_level c_rf::clock_posedge_int_clock( void )
{
    int i;

    /*b Read and write the rf if required
     */
    if (rf_fast_clock && rf_wr_enable)
    {
        if (rf_wr_addr<rf_size)
        {
            if (data)
            {
                for (i=0; i<rf_byte_width; i++)
                {
                    data[rf_wr_addr*rf_byte_width+i] = ((char *)(rf_wr_data))[i];
                }
            }
        }
        else
        {
            fprintf(stderr,"c_rf::Out of range write\n");
        }
    }

    /*b Done
     */
    return error_level_okay;
}

/*a Initialization functions
*/
/*f c_rf__init
*/
extern void c_rf__init( void )
{
    se_external_module_register( 1, "rf_1r_1w_32_32", rf_1r_1w_32_32_instance_fn );
    se_external_module_register( 1, "rf_1r_1w_16_32", rf_1r_1w_16_32_instance_fn );
    se_external_module_register( 1, "rf_1r_1w_16_4",  rf_1r_1w_16_4_instance_fn );
    se_external_module_register( 1, "rf_2r_1w_32_32", rf_2r_1w_32_32_instance_fn );
    se_external_module_register( 1, "rf_2r_1w_32_32_fc", rf_2r_1w_32_32_fc_instance_fn );
}

/*a Scripting support code
*/
/*f initrf
*/
extern "C" void initrf( void )
{
    c_rf__init( );
    scripting_init_module( "rf" );
}
