/*a Copyright Gavin J Stark
 */

/*a Examples
# sinks is ( collision pattern) ( ( sink 0 ) (sink 1) (sink 2)... )
# collision pattern is one number per packet, where (nybble after start of preamble) collision should be asserted; 0 implies no collision for a packet
# a value of 0 means no collision in a packet
# a value of -1 means restart the list
# when the end of the list is reached, no more collisions will be put out; so for a cycle, use -1 at the end of the list
 */

/*a Includes
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "be_model_includes.h"
#include "sl_general.h"
#include "sl_token.h"
#include "sl_data_stream.h"
#include "postbus.h"

/*a Defines
 */
#define MAX_INT_WIDTH (8)
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


/*a Types for c_memory
*/
/*t t_memory_posedge_int_clock_state
*/
typedef struct t_memory_posedge_int_clock_state
{
    int writing;
    int byte_enables;
    int reading;
    unsigned int sram_read_address;
    unsigned int sram_write_address;
    unsigned int sram_read_data[MAX_INT_WIDTH];
    unsigned int sram_write_data[MAX_INT_WIDTH];
} t_memory_posedge_int_clock_state;

/*t t_memory_inputs
*/
typedef struct t_memory_inputs
{
    unsigned int *sram_read;
    unsigned int *sram_write;
    unsigned int *sram_byte_enables;
    unsigned int *sram_address;
    unsigned int *sram_read_address;
    unsigned int *sram_write_address;
    unsigned int *sram_write_data;
} t_memory_inputs;

/*t c_memory
*/
class c_memory
{
public:
    c_memory::c_memory( class c_engine *eng, void *eng_handle, int size, int width, int byte_enables, int shared_ports, int read_ports, int write_ports );
    c_memory::~c_memory();
    t_sl_error_level c_memory::delete_instance( void );
    t_sl_error_level c_memory::reset( int pass );
    t_sl_error_level c_memory::preclock_posedge_int_clock( void );
    t_sl_error_level c_memory::clock_posedge_int_clock( void );
private:
    c_engine *engine;
    void *engine_handle;
    t_memory_inputs inputs;
    t_memory_posedge_int_clock_state next_posedge_int_clock_state;
    t_memory_posedge_int_clock_state posedge_int_clock_state;

    unsigned int memory_size;
    int memory_log_size; // power of 2 >= memory_size
    int memory_width; // width in bits
    int memory_int_width; // width in ints
    int memory_byte_width; // width in bytes
    int memory_has_byte_enables;

    int shared_ports;
    int read_ports;
    int write_ports;

    unsigned char *data; // mallocked array of width in bytes * size
};

/*a Static wrapper functions for memory
*/
/*f memory_s_sp_2048_x_32_instance_fn
*/
static t_sl_error_level memory_s_sp_2048_x_32_instance_fn( c_engine *engine, void *engine_handle )
{
    c_memory *mod;
    mod = new c_memory( engine, engine_handle, 2048, 32, 0, 1, 0, 0); // 2048 entries of 32 bytes without byte enables; 1 shared r/w, 0 r, 0w
    if (!mod)
        return error_level_fatal;
    return error_level_okay;
}

/*f memory_s_sp_2048_x_4b8_instance_fn
*/
static t_sl_error_level memory_s_sp_2048_x_4b8_instance_fn( c_engine *engine, void *engine_handle )
{
    c_memory *mod;
    mod = new c_memory( engine, engine_handle, 2048, 32, 4, 1, 0, 0); // 2048 entries of 32 bytes with 4 byte enables; 1 shared r/w, 0 r, 0w
    if (!mod)
        return error_level_fatal;
    return error_level_okay;
}

/*f memory_s_dp_2048_x_32_instance_fn
*/
static t_sl_error_level memory_s_dp_2048_x_32_instance_fn( c_engine *engine, void *engine_handle )
{
    c_memory *mod;
    mod = new c_memory( engine, engine_handle, 2048, 32, 0, 0, 1, 1 ); // 2048 entries of 32 bytes without byte enables; 1 shared r/w, 0 r, 0w
    if (!mod)
        return error_level_fatal;
    return error_level_okay;
}

/*f memory_delete_fn - simple callback wrapper for the main method
*/
static t_sl_error_level memory_delete_fn( void *handle )
{
    c_memory *mod;
    t_sl_error_level result;
    mod = (c_memory *)handle;
    result = mod->delete_instance();
    delete( mod );
    return result;
}

/*f memory_reset_fn
*/
static t_sl_error_level memory_reset_fn( void *handle, int pass )
{
    c_memory *mod;
    mod = (c_memory *)handle;
    return mod->reset( pass );
}

/*f memory_preclock_posedge_int_clock_fn
*/
static t_sl_error_level memory_preclock_posedge_int_clock_fn( void *handle )
{
    c_memory *mod;
    mod = (c_memory *)handle;
    return mod->preclock_posedge_int_clock();
}

/*f memory_clock_posedge_int_clock_fn
*/
static t_sl_error_level memory_clock_posedge_int_clock_fn( void *handle )
{
    c_memory *mod;
    mod = (c_memory *)handle;
    return mod->clock_posedge_int_clock();
}

/*a Constructors and destructors for memory
*/
/*f c_memory::c_memory
*/
c_memory::c_memory( class c_engine *eng, void *eng_handle, int size, int width, int byte_enables, int shared_ports, int read_ports, int write_ports )
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
    memory_size = size;
    memory_log_size = i;
    memory_width = width;
    memory_byte_width = (width+7)/8;
    memory_int_width = (width+sizeof(int)*8-1)/(sizeof(int)*8);
    memory_has_byte_enables = byte_enables;
    data = (unsigned char *)malloc(memory_byte_width*memory_size);
    this->shared_ports = shared_ports;
    this->read_ports = read_ports;
    this->write_ports = write_ports;
    
    /*b Instantiate module
     */
    engine->register_delete_function( engine_handle, (void *)this, memory_delete_fn );
    engine->register_reset_function( engine_handle, (void *)this, memory_reset_fn );
    engine->register_clock_fns( engine_handle, (void *)this, "sram_clock", memory_preclock_posedge_int_clock_fn, memory_clock_posedge_int_clock_fn );

    INPUT( sram_read, 1, sram_clock );
    INPUT( sram_write, 1, sram_clock );
    if (memory_has_byte_enables)
    {
        INPUT( sram_byte_enables, memory_byte_width, sram_clock );
    }
    if (shared_ports)
    {
        INPUT( sram_address, memory_log_size, sram_clock );
    }
    else
    {
        INPUT( sram_write_address, memory_log_size, sram_clock );
        INPUT( sram_read_address, memory_log_size, sram_clock );
    }
    INPUT( sram_write_data, memory_width, sram_clock );
    STATE_OUTPUT( sram_read_data, memory_width, sram_clock );

    /*b Register state then reset
     */
    //engine->register_state_desc( engine_handle, 1, state_desc_memory_posedge_int_clock, &posedge_int_clock_state, NULL );
    reset( 0 );

    /*b Done
     */

}

/*f c_memory::~c_memory
*/
c_memory::~c_memory()
{
    delete_instance();
}

/*f c_memory::delete_instance
*/
t_sl_error_level c_memory::delete_instance( void )
{
    return error_level_okay;
}

/*a Class reset/preclock/clock methods for memory
*/
/*f c_memory::reset
*/
t_sl_error_level c_memory::reset( int pass )
{
    int i;
    posedge_int_clock_state.writing = 0;
    posedge_int_clock_state.byte_enables = 0;
    posedge_int_clock_state.reading = 0;
    posedge_int_clock_state.sram_read_address = 0;
    posedge_int_clock_state.sram_write_address = 0;
    for (i=0; i<MAX_INT_WIDTH; i++)
    {
        posedge_int_clock_state.sram_read_data[i] = 0;
        posedge_int_clock_state.sram_write_data[i] = 0;
    }
    return error_level_okay;
}

/*f c_memory::preclock_posedge_int_clock
*/
t_sl_error_level c_memory::preclock_posedge_int_clock( void )
{
    int i;

    /*b Copy current state to next state
     */
    memcpy( &next_posedge_int_clock_state, &posedge_int_clock_state, sizeof(posedge_int_clock_state) );

    /*b Record inputs
     */
    next_posedge_int_clock_state.reading = inputs.sram_read[0];
    next_posedge_int_clock_state.writing = inputs.sram_write[0];
    if (shared_ports)
    {
        next_posedge_int_clock_state.sram_read_address = inputs.sram_address[0];
        next_posedge_int_clock_state.sram_write_address = inputs.sram_address[0];
    }
    else
    {
        next_posedge_int_clock_state.sram_read_address = inputs.sram_read_address[0];
        next_posedge_int_clock_state.sram_write_address = inputs.sram_write_address[0];
    }
    if (memory_has_byte_enables)
        next_posedge_int_clock_state.byte_enables = inputs.sram_byte_enables[0];
    for (i=0; i<memory_int_width; i++)
        next_posedge_int_clock_state.sram_write_data[i] = inputs.sram_write_data[i];

    /*b Done
     */
    return error_level_okay;
}

/*f c_memory::clock_posedge_int_clock
*/
t_sl_error_level c_memory::clock_posedge_int_clock( void )
{
    int i;

    /*b Copy next state to current
    */
    memcpy( &posedge_int_clock_state, &next_posedge_int_clock_state, sizeof(posedge_int_clock_state) );

    /*b Read and write the memory if required
     */
    if (posedge_int_clock_state.writing)
    {
//        fprintf(stderr,"c_memory(%p)::clock_posedge_int_clock:Writing address %04x data %08x\n", this, posedge_int_clock_state.sram_write_address, posedge_int_clock_state.sram_write_data[0] );
        if (posedge_int_clock_state.sram_write_address<memory_size)
        {
            if (data)
            {
                if (memory_has_byte_enables)
                {
                    for (i=0; i<memory_byte_width; i++)
                    {
                        if (posedge_int_clock_state.byte_enables&(1<<i))
                        {
                            data[posedge_int_clock_state.sram_write_address*memory_byte_width+i] = ((char *)(posedge_int_clock_state.sram_write_data))[i];
                        }
                    }
                }
                else
                {
                    for (i=0; i<memory_byte_width; i++)
                    {
                        data[posedge_int_clock_state.sram_write_address*memory_byte_width+i] = ((char *)(posedge_int_clock_state.sram_write_data))[i];
                    }
                }
            }
        }
        else
        {
            fprintf(stderr,"c_memory::Out of range write\n");
        }
    }
    if (posedge_int_clock_state.reading)
    {
        if (posedge_int_clock_state.sram_read_address<memory_size)
        {
            if (data)
            {
                for (i=0; i<memory_byte_width; i++)
                {
                    ((char *)(posedge_int_clock_state.sram_read_data))[i] = data[posedge_int_clock_state.sram_read_address*memory_byte_width+i];
                }
            }
        }
        else
        {
            fprintf(stderr,"c_memory::Out of range read\n");
        }
        //fprintf(stderr,"c_memory(%p)::clock_posedge_int_clock:Reading address %04x data %08x\n", this, posedge_int_clock_state.sram_read_address, posedge_int_clock_state.sram_read_data[0] );
    }

    /*b Done
     */
    return error_level_okay;
}

/*a Initialization functions
*/
/*f c_memory__init
*/
extern void c_memory__init( void )
{
    se_external_module_register( 1, "memory_s_sp_2048_x_32", memory_s_sp_2048_x_32_instance_fn );
    se_external_module_register( 1, "memory_s_sp_2048_x_4b8", memory_s_sp_2048_x_4b8_instance_fn );
    se_external_module_register( 1, "memory_s_dp_2048_x_32", memory_s_dp_2048_x_32_instance_fn );
}

/*a Scripting support code
*/
/*f initmemory
*/
extern "C" void initmemory( void )
{
    c_memory__init( );
    scripting_init_module( "memory" );
}
