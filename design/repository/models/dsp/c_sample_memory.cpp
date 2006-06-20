/*a Copyright Gavin J Stark
 */

/*a Examples
 */

/*a Includes
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "be_model_includes.h"
#include "sl_general.h"
#include "sl_token.h"

#include "postbus.h"

/*a Defines
 */
#define BLOCK_SIZE (5)
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

#define STATE_OUTPUT( name, width, clk )\
    { \
        engine->register_output_signal( engine_handle, #name, width, (int *)&posedge_int_clock_state.name ); \
        engine->register_output_generated_on_clock( engine_handle, #name, #clk, 1 ); \
    }

/*a Types for c_sample_memory
*/
/*t t_ram_write_type
 */
typedef enum
{
    ram_write_none,
    ram_write_record_buffer_0,
    ram_write_record_buffer_1,
    ram_write_playback_buffer_0
} t_ram_write_type;

/*t t_ram_read_type
 */
typedef enum
{
    ram_read_none,
    ram_read_record_buffer, // read record buffer given in postbus_tx_buffer
    ram_read_playback_buffer_0
} t_ram_read_type;

/*t t_ram_op
 */
typedef struct t_ram_op
{
    t_ram_write_type write_type;
    unsigned int write_data;
    t_ram_read_type read_type;
    int last_read_record_buffer; // asserted if at the last posedge clock the read_type was for a record buffer
    unsigned int read_data; // data from the read occurring in this cycle, i.e. due to read_type at the last posedge clock
} t_ram_op;

/*t t_sample_memory_inputs
*/
typedef struct t_sample_memory_inputs
{
    int *int_reset;
    int *sample_valid; // max of once every three internal clock cycles
    int *sample_i; // must be stable when valid and for one cycle afterwards
    int *sample_q; // must be stable when valid and for one cycle afterwards
    int *postbus_tx_ack;
} t_sample_memory_inputs;

/*t t_buffer_fsm
 */
typedef enum
{
    buffer_fsm_idle,
    buffer_fsm_push_pending,
    buffer_fsm_push_in_progress
} t_buffer_fsm;

/*t t_sample_memory_buffer
  circular buffer, sized in 'blocks' of 32 words
*/
typedef struct t_sample_memory_buffer
{
    t_buffer_fsm fsm; // state of the bulk data port
    int start_block; // which block the circular buffer starts at (inclusive)
    int end_block;   // which block the circular buffer ends at (inclusive)
    int read_block;  // which block data comes out from
    int write_block; // which block data writes in to
    //int empty;       // asserted if buffer is empty
    int overflowed;  // asserted if buffer has overflowed
    int posn;        // word within block next data should be read from or written to - for the 'word-wise' access side
} t_sample_memory_buffer;

/*t t_postbus_tx_fsm
 */
typedef enum
{
    postbus_tx_fsm_idle,
    postbus_tx_fsm_push_out,
    postbus_tx_fsm_push_last,
    postbus_tx_fsm_push_complete
} t_postbus_tx_fsm;

/*t t_sample_memory_postbus_tx
*/
typedef struct t_sample_memory_postbus_tx
{
//    t_postbus_ack ack;
    t_postbus_type type;
    unsigned int data;

    t_postbus_tx_fsm fsm;
    unsigned int holding_register;
    int holding_register_full;
    int count;
    int buffer;
} t_sample_memory_postbus_tx;

/*t t_iq_shaper_mode
 */
typedef enum
{
    iq_shaper_mode_off,
    iq_shaper_mode_iq,
    iq_shaper_mode_i
} t_iq_shaper_mode;

/*t t_sample_memory_iq_shaper
*/
typedef struct t_sample_memory_iq_shaper
{
    unsigned int iq; // i in bottom half, q in top half, if both captured; oldest in top half if just i captured (big-endian)
    int valid; // asserted if iq is full of data
    int half_valid; // asserted if only i captured, and if only half is ready
    t_iq_shaper_mode mode;
} t_sample_memory_iq_shaper;

/*t t_sample_memory_internal_clock_state
*/
typedef struct t_sample_memory_internal_clock_state
{
    t_sample_memory_iq_shaper shaper;
    t_sample_memory_buffer record_buffer[2];
    t_sample_memory_buffer playback_buffer;
    t_sample_memory_postbus_tx postbus_tx;
    unsigned int record_postbus_header[2]; // postbus header used for each of the record buffers when pushing data out
} t_sample_memory_internal_clock_state;

/*t c_sample_memory
*/
class c_sample_memory
{
public:
    c_sample_memory::c_sample_memory( class c_engine *eng, void *eng_handle );
    c_sample_memory::~c_sample_memory();
    t_sl_error_level c_sample_memory::delete_instance( void );
    t_sl_error_level c_sample_memory::reset( int pass );
    t_sl_error_level c_sample_memory::preclock_posedge_int_clock( void );
    t_sl_error_level c_sample_memory::clock_posedge_int_clock( void );
private:
    c_engine *engine;
    void *engine_handle;

    t_sample_memory_inputs inputs;

    t_sample_memory_internal_clock_state state;
    t_sample_memory_internal_clock_state next_state;
    t_ram_op ram_op; // RAM operation to be performed at the clock edge

    int buffer_ram_size; // size in words
    unsigned int *ram_buffer; // mallocked RAM

};

/*a Static variables for state
*/
/*v struct_offset wrapper
*/
#define struct_offset( ptr, a ) (((char *)&(ptr->a))-(char *)ptr)
/*v state_desc_posedge_int_clock
*/
static t_sample_memory_internal_clock_state *___posedge_int_clock__ptr;
static t_engine_state_desc state_desc_posedge_int_clock[] =
{
//    {"valid", engine_state_desc_type_bits, NULL, struct_offset(___posedge_int_clock__ptr, valid), {1,0,0,0}, {NULL,NULL,NULL,NULL} },
//    {"result", engine_state_desc_type_bits, NULL, struct_offset(___posedge_int_clock__ptr, result), {16,0,0,0}, {NULL,NULL,NULL,NULL} },

    {"", engine_state_desc_type_none, NULL, 0, {0,0,0,0}, {NULL,NULL,NULL,NULL} }
};

/*v struct_offset unwrapper
*/
#undef struct_offset

/*a Static wrapper functions for sample_memory
*/
/*f sample_memory_instance_fn
*/
static t_sl_error_level sample_memory_instance_fn( c_engine *engine, void *engine_handle )
{
    c_sample_memory *mod;
    mod = new c_sample_memory( engine, engine_handle );
    if (!mod)
        return error_level_fatal;
    return error_level_okay;
}

/*f sample_memory_delete_fn - simple callback wrapper for the main method
*/
static t_sl_error_level sample_memory_delete_fn( void *handle )
{
    c_sample_memory *mod;
    t_sl_error_level result;
    mod = (c_sample_memory *)handle;
    result = mod->delete_instance();
    delete( mod );
    return result;
}

/*f sample_memory_reset_fn
*/
static t_sl_error_level sample_memory_reset_fn( void *handle, int pass )
{
    c_sample_memory *mod;
    mod = (c_sample_memory *)handle;
    return mod->reset( pass );
}

/*f sample_memory_preclock_posedge_int_clock_fn
*/
static t_sl_error_level sample_memory_preclock_posedge_int_clock_fn( void *handle )
{
    c_sample_memory *mod;
    mod = (c_sample_memory *)handle;
    return mod->preclock_posedge_int_clock();
}

/*f sample_memory_clock_posedge_int_clock_fn
*/
static t_sl_error_level sample_memory_clock_posedge_int_clock_fn( void *handle )
{
    c_sample_memory *mod;
    mod = (c_sample_memory *)handle;
    return mod->clock_posedge_int_clock();
}

/*a Constructors and destructors for sample_memory
*/
/*f c_sample_memory::c_sample_memory
*/
c_sample_memory::c_sample_memory( class c_engine *eng, void *eng_handle )
{
    engine = eng;
    engine_handle = eng_handle;

    buffer_ram_size = 2048; // 2k words
    ram_buffer = (unsigned int *)malloc(sizeof(unsigned int)*buffer_ram_size);

    /*b Instantiate module
     */
    engine->register_delete_function( engine_handle, (void *)this, sample_memory_delete_fn );
    engine->register_reset_function( engine_handle, (void *)this, sample_memory_reset_fn );
    engine->register_clock_fns( engine_handle, (void *)this, "int_clock", sample_memory_preclock_posedge_int_clock_fn, sample_memory_clock_posedge_int_clock_fn );

    CLOCKED_INPUT( int_reset, 1, int_clock );
    CLOCKED_INPUT( sample_i, 16, int_clock );
    CLOCKED_INPUT( sample_q, 16, int_clock );
    CLOCKED_INPUT( sample_valid, 1, int_clock );
    CLOCKED_INPUT( postbus_tx_ack, 1, int_clock );

    engine->register_output_signal( engine_handle, "postbus_tx_type", 2, (int *)&state.postbus_tx.type );
    engine->register_output_generated_on_clock( engine_handle, "postbus_tx_type", "int_clock", 1 );
    engine->register_output_signal( engine_handle, "postbus_tx_data", 32, (int *)&state.postbus_tx.data );
    engine->register_output_generated_on_clock( engine_handle, "postbus_tx_data", "int_clock", 1 );

    /*b Register state then reset
     */
    engine->register_state_desc( engine_handle, 1, state_desc_posedge_int_clock, &state, NULL );
    reset( 0 );

    /*b Done
     */

}

/*f c_sample_memory::~c_sample_memory
*/
c_sample_memory::~c_sample_memory()
{
    if (ram_buffer)
    {
        free(ram_buffer);
        ram_buffer = NULL;
    }
    delete_instance();
}

/*f c_sample_memory::delete_instance
*/
t_sl_error_level c_sample_memory::delete_instance( void )
{
    return error_level_okay;
}

/*a Class reset/preclock/clock methods for sample_memory
*/
/*f c_sample_memory::reset
*/
t_sl_error_level c_sample_memory::reset( int pass )
{
    int i;
    for (i=0; i<2; i++)
    {
        state.record_buffer[i].fsm = buffer_fsm_idle;
        state.record_buffer[i].overflowed = 1;
        state.record_buffer[i].start_block = 0;
        state.record_buffer[i].end_block = 0;
        state.record_buffer[i].read_block = 0;
        state.record_buffer[i].write_block = 0;
        state.record_buffer[i].posn = 0;

        state.record_postbus_header[i] = 0;
    }

    state.playback_buffer.fsm = buffer_fsm_idle;
    state.playback_buffer.overflowed = 0;
    state.playback_buffer.start_block = 0;
    state.playback_buffer.end_block = 0;
    state.playback_buffer.read_block = 0;
    state.playback_buffer.write_block = 0;
    state.playback_buffer.posn = 0;


    state.record_buffer[0].overflowed = 0;
    state.record_buffer[0].start_block = 0;
    state.record_buffer[0].end_block = 0x10;
    state.record_buffer[0].read_block = 0;
    state.record_buffer[0].write_block = 0;
    state.record_buffer[0].posn = 0;
    state.record_postbus_header[0] = 0x14000000; // 25 to 31 are dest; 2;25=>10 for push, 2;27=2 for circular buffer 0

    state.shaper.mode = iq_shaper_mode_iq;
    state.shaper.valid = 0;
    state.shaper.half_valid = 0;

    return error_level_okay;
}

/*f c_sample_memory::cic_int_stage_preclock
 */
/*f c_sample_memory::preclock_posedge_int_clock
*/
t_sl_error_level c_sample_memory::preclock_posedge_int_clock( void )
{

    /*b Copy current to next
     */
    memcpy( &next_state, &state, sizeof(state));

    /*b Clear RAM operation
     */
    ram_op.write_type = ram_write_none;
    ram_op.read_type = ram_read_none;

    /*b Sample input shaper
     */
    switch (state.shaper.mode)
    {
    case iq_shaper_mode_off:
        next_state.shaper.valid = 0;
        next_state.shaper.half_valid = 0;
        break;
    case iq_shaper_mode_iq:
        next_state.shaper.valid = inputs.sample_valid[0];
        next_state.shaper.half_valid = 0;
        next_state.shaper.iq = inputs.sample_i[0] | (inputs.sample_q[0]<<16);
        break;
    case iq_shaper_mode_i:
        next_state.shaper.valid = (inputs.sample_valid[0] && state.shaper.half_valid);
        next_state.shaper.half_valid = state.shaper.half_valid ^ inputs.sample_valid[0];
        if (inputs.sample_valid[0])
        {
            next_state.shaper.iq = (state.shaper.iq<<16) | inputs.sample_i[0];
        }
        break;
    }

    /*b Handle record buffer 0
     */
    {
        if (state.shaper.valid) // want to write sample_data to memory at our location - should and with enable, and should have data shaper in front anyway (take i only, take i and q, take none)
        {
            next_state.record_buffer[0].posn = (state.record_buffer[0].posn+1)&~((-1)<<BLOCK_SIZE);
            if (next_state.record_buffer[0].posn==0)
            {
                if (state.record_buffer[0].write_block == state.record_buffer[0].end_block)
                {
                    next_state.record_buffer[0].write_block = state.record_buffer[0].start_block;
                }
                else
                {
                    next_state.record_buffer[0].write_block = state.record_buffer[0].write_block+1;
                }
                if (next_state.record_buffer[0].write_block == state.record_buffer[0].read_block)
                {
                    next_state.record_buffer[0].overflowed = 1;
                }
                if ((state.record_buffer[0].fsm == buffer_fsm_idle) && (state.record_postbus_header[0]!=0))
                {
                    next_state.record_buffer[0].fsm = buffer_fsm_push_pending;
                }
            }
            ram_op.write_type = ram_write_record_buffer_0;
            ram_op.write_data = state.shaper.iq;
        }
    }

    /*b Handle postbus
     */
    {
        switch (state.postbus_tx.fsm)
        {
        case postbus_tx_fsm_idle:
            next_state.postbus_tx.type = postbus_word_type_idle;
            if (state.record_buffer[0].fsm==buffer_fsm_push_pending)
            {
                next_state.record_buffer[0].fsm = buffer_fsm_push_in_progress;
                next_state.postbus_tx.type = postbus_word_type_start;
                next_state.postbus_tx.data = state.record_postbus_header[0]; // should denote length 32
                next_state.postbus_tx.fsm = postbus_tx_fsm_push_out;
                next_state.postbus_tx.buffer = 0; // record buffer 0
                next_state.postbus_tx.count = 0; // about to push out the first word
                next_state.postbus_tx.holding_register_full = 0; // start off with the holding register empty
                ram_op.read_type = ram_read_record_buffer;
            }
            break;
        case postbus_tx_fsm_push_out: // driving type, data; hopefully reading RAM, although that depends on write operation and holding register
            if (inputs.postbus_tx_ack[0]==postbus_ack_taken)
            {
                // data taken; present data in holding register as first priority
                if (state.postbus_tx.holding_register_full)
                {
                    next_state.postbus_tx.type = postbus_word_type_data;
                    next_state.postbus_tx.data = state.postbus_tx.holding_register;
                    next_state.postbus_tx.holding_register_full = 0;
                    ram_op.read_type = ram_read_record_buffer;
                    //next_state.postbus_tx.count = state.postbus_tx.count+1;
                    //if (next_state.postbus_tx.count==32)
                    //{
                    //   next_state.postbus_tx.fsm = postbus_tx_fsm_push_last; // push out the data we are now reading as the last word
                    //}
                }
                else if (ram_op.last_read_record_buffer) // if were reading, present that data
                {
                    next_state.postbus_tx.type = postbus_word_type_data;
                    next_state.postbus_tx.data = ram_op.read_data;
                    next_state.postbus_tx.count = state.postbus_tx.count+1;
                    ram_op.read_type = ram_read_record_buffer;
                    if (next_state.postbus_tx.count==32)
                    {
                        next_state.postbus_tx.fsm = postbus_tx_fsm_push_last; // push out the data we are now reading as the last word
                    }
                }
                else // taken, no pending data, RAM didn't read - hold that bus!
                {
                    next_state.postbus_tx.type = postbus_word_type_hold;
                    ram_op.read_type = ram_read_record_buffer;
                }
            }
            else // not taken, so hold and store any read data in the holding register; note that if we were holding the bus, and a read occurred, we could actually change our bus to the output data
            {
                if (ram_op.last_read_record_buffer) // not taken, RAM read, so put RAM read data in holding register
                {
                    next_state.postbus_tx.holding_register = ram_op.read_data;
                    next_state.postbus_tx.holding_register_full = 1;
                    // holding register will be full, so don't read as we may have nowhere to put the data!
                }
                else // not taken, not read data... everything is as was, and read the data this time pretty please
                {
                    ram_op.read_type = ram_read_record_buffer;
                }
            }
            break;
        case postbus_tx_fsm_push_last: // currently pushing out next-to-last word, reading the last word (we hope); could be pending data if we were here last cycle and read occurred but take did not
            if (inputs.postbus_tx_ack[0]==postbus_ack_taken) // took next-to-last word, so push out last word if we have it
            {
                if (state.postbus_tx.holding_register_full) // can only be if we were here last cycle and a read of the last data word has happened
                {
                    next_state.postbus_tx.type = postbus_word_type_last;
                    next_state.postbus_tx.data = state.postbus_tx.holding_register;
                    next_state.postbus_tx.fsm = postbus_tx_fsm_push_complete;
                }
                else if (ram_op.last_read_record_buffer) // if were reading, present that data as it is the last word
                {
                    next_state.postbus_tx.type = postbus_word_type_last;
                    next_state.postbus_tx.data = ram_op.read_data;
                    next_state.postbus_tx.fsm = postbus_tx_fsm_push_complete;
                }
                else // taken next-to-last, no pending data, RAM didn't read - hold that bus, try again
                {
                    next_state.postbus_tx.type = postbus_word_type_hold;
                    ram_op.read_type = ram_read_record_buffer;
                    next_state.postbus_tx.fsm = postbus_tx_fsm_push_last;
                }
            }
            else // not taken, so hold and store any read data in the holding register
            {
                if (ram_op.last_read_record_buffer) // not taken, RAM read, so put RAM read data in holding register
                {
                    next_state.postbus_tx.holding_register = ram_op.read_data;
                    next_state.postbus_tx.holding_register_full = 1;
                }
                else // not taken, not read data... everything is as was, and read the data this time pretty please
                {
                    ram_op.read_type = ram_read_record_buffer;
                }
            }
            break;
        case postbus_tx_fsm_push_complete: // currently pushing out last word; wait for ack
            if (inputs.postbus_tx_ack[0]==postbus_ack_taken) // took last word, so idle (and update state of record buffer that invoked us)
            {
                next_state.postbus_tx.type = postbus_word_type_idle;
                next_state.postbus_tx.fsm = postbus_tx_fsm_idle;
                if (state.record_buffer[0].read_block==state.record_buffer[0].end_block)
                {
                    next_state.record_buffer[0].read_block = state.record_buffer[0].start_block;
                }
                else
                {
                    next_state.record_buffer[0].read_block = state.record_buffer[0].read_block+1;
                }
                if (state.record_buffer[0].write_block != next_state.record_buffer[0].read_block)
                {
                    next_state.record_buffer[0].fsm = buffer_fsm_push_pending;
                }
                else
                {
                    next_state.record_buffer[0].fsm = buffer_fsm_idle;
                }
            }
            else // not taken, so try again
            {
            }
            break;
        default:
            break;
        }
    }

    /*b Done
     */
    return error_level_okay;
}

/*f c_sample_memory::clock_posedge_int_clock
*/
t_sl_error_level c_sample_memory::clock_posedge_int_clock( void )
{

    /*b Perform RAM operation
     */
    {
        int write_enable, read_enable;
        unsigned int write_address;
        unsigned int read_address;
        write_enable = write_address = 0;
        read_enable = read_address = 0;
        ram_op.last_read_record_buffer = 0;
        switch (ram_op.write_type)
        {
        case ram_write_record_buffer_0:
            write_address = (state.record_buffer[0].write_block<<BLOCK_SIZE) | state.record_buffer[0].posn;
            write_enable = 1;
            break;
        default:
            break;
        }
        switch (ram_op.read_type)
        {
        case ram_read_record_buffer:
            read_address = (state.record_buffer[0].read_block<<BLOCK_SIZE) | next_state.postbus_tx.count;
            read_enable = 1;
            break;
        default:
            break;
        }
        if (write_enable)
        {
            ram_buffer[write_address] = ram_op.write_data;
            read_enable = 0;
            //printf("Wrote to %d data %08x\n", write_address, ram_op.write_data );
        }
        if (read_enable)
        {
            ram_op.read_data = ram_buffer[read_address];
            ram_op.last_read_record_buffer = 1;
            //printf("Read from %d data %08x\n", read_address, ram_op.read_data );
        }
    }

    /*b Copy next to current
     */
    memcpy( &state, &next_state, sizeof(state));

    /*b Done
     */
    return error_level_okay;
}

/*a Initialization functions
*/
/*f c_sample_memory__init
*/
extern void c_sample_memory__init( void )
{
    se_external_module_register( 1, "sample_memory", sample_memory_instance_fn );
}

/*a Scripting support code
*/
/*f initsample_memory
*/
extern "C" void initsample_memory( void )
{
    c_sample_memory__init( );
    scripting_init_module( "sample_memory" );
}
