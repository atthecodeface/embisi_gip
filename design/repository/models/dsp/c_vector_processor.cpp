/*a Copyright Gavin J Stark
 */

/*a Documentation The vector processor is an engine that is a high
  performance, highly pipelined, data processing engine for somewhat
  flexible vector processing

  The basic design is driven by the memory subsystem that is a pair of
  single read/write port SRAMs, whose accesses are prioritized for the
  vector processing accesses; cycles not used by the vector processing
  are available for a postbus interface to fill and empty the memory.

  The vector processing, then, consists of a memory interface that
  supports up to two read/writes simultaneously, and a collection of
  vector processing subsystems which perform autonomous processing on
  data words from the memory, and a small instruction memory that can
  be loaded with libraries of vector processing operations specific to
  the particular system required.
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

/*a Defines
 */
#define MAX_INT_WIDTH (8)
#define CIC_WIDTH (40)
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

/*a Types for c_vector_processor
*/
/*t t_phase_acc_fsm
 */
typedef enum
{
    phase_acc_fsm_idle,
    phase_acc_fsm_reading_cosine,
    phase_acc_fsm_reading_sine
} t_phase_acc_fsm;

/*t t_mixer_fsm
 */
typedef enum
{
    mixer_fsm_idle,
    mixer_fsm_cosine_done,
    mixer_fsm_sine_done
} t_mixer_fsm;

/*t t_vector_processor_inputs
  Reset
  Digital data in (probably should have some form of clock enable)
  Postbus for configuration and data out
*/
typedef struct t_vector_processor_inputs
{
    unsigned int *int_reset;

    unsigned int *data_0;
    unsigned int *data_1;

} t_vector_processor_inputs;

/*t t_vector_processor_data_clock_state
*/
typedef struct t_vector_processor_data_clock_state
{
    unsigned int data_0_reg; // input registers for data
    unsigned int data_1_reg;

    unsigned int selected_i; // mux select from data registers for in-phase for sync to internal clock domain
    unsigned int selected_q; // mux select from data registers for quadrature for sync to internal clock domain
    unsigned int selected_toggle; // toggled when the data in the selected registers becomes valid - an edge on this must be detected by the internal clock

} t_vector_processor_data_clock_state;

/*t t_vector_processor_internal_clock_sync_state
*/
typedef struct t_vector_processor_internal_clock_sync_state
{
    // we synchronize a toggle and register the data, all to the internal clock domain
    // the synchronizer for the toggle is a metastable flop, a stable flop, and a 'last value' flop; if the stable and last flops differ in value, then data is present in the register
    // if the external clock occurs near-simultaneously with the internal clock, then the metastability can occur
    // there are two cases to consider
    // 1. the data register clocks to a new value, while the metastable flop settles untoggled, on the clock collision
    // 2. the data register clocks an incorrect (or old) value, while the metastable flop settles on the toggle, on the clock collision
    // In case 1:
    //  during cycle 0 (internal cycle post clock collision) metastable is untoggled, data register has new data
    //  during cycle 1 metastable has toggled, stable has not
    //  during cycle 2 stable has toggled, last has not, and so valid data is expected in the register
    //  therefore the next external clock cannot occur prior to the end of cycle 2 - this gives a minimum of 3 internal clocks per external clock tick
    // In case 2:
    //  during cycle 0 metastable has toggled, data register has old or invalid data
    //  during cycle 1 stable has toggled, last has not, and data register has valid data, as required
    //  therefore there is no race condition for case 2, and no clocking limit
    unsigned int sync_reg_i_0; // internal clock version of sync of selected_ data
    unsigned int sync_reg_q_0;

    int sync_toggle_0; // first stage of sync of selected_toggle
    int sync_toggle_1; // second stage
    int sync_last_toggle; // if this differs from sync_toggle_1 then an edge occurred on the toggle and the sync register is required to have valid data

    unsigned int i_out; // output data
    unsigned int q_out;
    int iq_valid; // asserted if i_out and q_out are valid

} t_vector_processor_internal_clock_sync_state;

/*t t_phase_acc_rom_entry
 */
typedef struct
{
    unsigned int base_value;
    unsigned int delta;
    unsigned long error_bits;
} t_phase_acc_rom_entry;

/*t t_vector_processor_internal_clock_phase_acc_state
*/
typedef struct t_vector_processor_internal_clock_phase_acc_state
{
    unsigned int phase; // current phase
    unsigned int phase_step; // increment per tick
    t_phase_acc_fsm fsm; // idle, reading sin, reading cos
    struct
    {
        int rom_reading_cosine;
        int negate;
        int fine_phase;
        unsigned int rom_base_value;
        unsigned int rom_delta;
        unsigned int rom_error_bits;
    } rom_stage;
    struct
    {
        int negate;
        int fine_phase; // 4 bit value
        int valid_cosine;
        int base_value; // 14 bit value, 0.14
        int delta;      // 6 bit value, 0.15 bottom 6 bits
        int error_bit;  // 1 bit value, 0.15 bottom bit
    } calc_in_stage;
    struct
    {
        int value; // 15 bit value, 0.15
        int sign;
        int valid_cosine;
    } data_out_stage;
} t_vector_processor_internal_clock_phase_acc_state;

/*t t_vector_processor_internal_clock_mixer_state
*/
typedef struct t_vector_processor_internal_clock_mixer_state
{
    t_mixer_fsm fsm;
    unsigned int mult_a;
    unsigned int mult_b;
    unsigned int i_out;
    unsigned int q_out;
    int valid_out;
    
} t_vector_processor_internal_clock_mixer_state;

/*t t_vector_processor_cic_int_stage
*/
typedef struct t_vector_processor_cic_int_stage
{
    unsigned long long calc; // normally holds i; add to input, put result in out
    unsigned long long out; // normally holds q; pass to calc when integrating
    int valid_out;
} t_vector_processor_cic_int_stage;

/*t t_vector_processor_cic_comb_stage
*/
typedef struct t_vector_processor_cic_comb_stage
{
    unsigned long long pipe_0; // normally holds q; pass to pipe_1 when combing
    unsigned long long pipe_1; // normally holds i; subtract from input when combing
    unsigned long long value; // result of comb
    int valid_out;
} t_vector_processor_cic_comb_stage;

/*t t_vector_processor_internal_clock_cic_state
*/
typedef struct t_vector_processor_internal_clock_cic_state
{
    t_vector_processor_cic_int_stage int_stage[5];
    t_vector_processor_cic_comb_stage comb_stage[5];
    unsigned int q_store;
    struct
    {
        int valid_out;
        unsigned int count;
        unsigned int factor;
    } decimate;
} t_vector_processor_internal_clock_cic_state;

/*t t_vector_processor_internal_clock_state
*/
typedef struct t_vector_processor_internal_clock_state
{
    t_vector_processor_internal_clock_sync_state       sync;
    t_vector_processor_internal_clock_phase_acc_state  phase_acc;
    t_vector_processor_internal_clock_mixer_state      mixer;
    t_vector_processor_internal_clock_cic_state        cic;

    double pwr_calc;
    unsigned int result;
    int calc;
    int valid;
} t_vector_processor_internal_clock_state;

/*t c_vector_processor
*/
class c_vector_processor
{
public:
    c_vector_processor::c_vector_processor( class c_engine *eng, void *eng_handle );
    c_vector_processor::~c_vector_processor();
    t_sl_error_level c_vector_processor::delete_instance( void );
    t_sl_error_level c_vector_processor::reset( int pass );
    t_sl_error_level c_vector_processor::preclock_posedge_int_clock( void );
    t_sl_error_level c_vector_processor::clock_posedge_int_clock( void );
    t_sl_error_level c_vector_processor::preclock_posedge_data_clock( void );
    t_sl_error_level c_vector_processor::clock_posedge_data_clock( void );
private:
    c_engine *engine;
    void *engine_handle;

    void c_vector_processor::cic_int_stage_preclock( int stage, int valid_in, unsigned long long value );
    void c_vector_processor::cic_comb_stage_preclock( int stage, int valid_in, unsigned long long value );

    t_vector_processor_inputs inputs;

    t_vector_processor_internal_clock_state state;
    t_vector_processor_internal_clock_state next_state;

    t_vector_processor_data_clock_state data_state;
    t_vector_processor_data_clock_state next_data_state;

    int phase_resolution;
    int coarse_resolution;
    int fine_resolution;

};

/*a Static ROM
 */
static t_phase_acc_rom_entry rom[1024] =
{
#include "rom.txt"
};


/*a Static variables for state
*/
/*v struct_offset wrapper
*/
#define struct_offset( ptr, a ) (((char *)&(ptr->a))-(char *)ptr)
/*v state_desc_posedge_int_clock
*/
static t_vector_processor_internal_clock_state *___posedge_int_clock__ptr;
static t_engine_state_desc state_desc_posedge_int_clock[] =
{
    {"sync_toggle_0", engine_state_desc_type_bits, NULL, struct_offset(___posedge_int_clock__ptr, sync.sync_toggle_0), {1,0,0,0}, {NULL,NULL,NULL,NULL} },
    {"sync_toggle_1", engine_state_desc_type_bits, NULL, struct_offset(___posedge_int_clock__ptr, sync.sync_toggle_1), {1,0,0,0}, {NULL,NULL,NULL,NULL} },
    {"sync_last_toggle", engine_state_desc_type_bits, NULL, struct_offset(___posedge_int_clock__ptr, sync.sync_last_toggle), {1,0,0,0}, {NULL,NULL,NULL,NULL} },
    {"sync_iq_valid", engine_state_desc_type_bits, NULL, struct_offset(___posedge_int_clock__ptr, sync.iq_valid), {1,0,0,0}, {NULL,NULL,NULL,NULL} },
    {"sync_i_out", engine_state_desc_type_bits, NULL, struct_offset(___posedge_int_clock__ptr, sync.i_out), {16,0,0,0}, {NULL,NULL,NULL,NULL} },
    {"sync_q_out", engine_state_desc_type_bits, NULL, struct_offset(___posedge_int_clock__ptr, sync.q_out), {16,0,0,0}, {NULL,NULL,NULL,NULL} },

    {"phase", engine_state_desc_type_bits, NULL, struct_offset(___posedge_int_clock__ptr, phase_acc.phase), {24,0,0,0}, {NULL,NULL,NULL,NULL} },
    {"phase_fsm", engine_state_desc_type_bits, NULL, struct_offset(___posedge_int_clock__ptr, phase_acc.fsm), {2,0,0,0}, {NULL,NULL,NULL,NULL} },
    {"phase_rom_reading_cosine", engine_state_desc_type_bits, NULL, struct_offset(___posedge_int_clock__ptr, phase_acc.rom_stage.rom_reading_cosine), {1,0,0,0}, {NULL,NULL,NULL,NULL} },
    {"phase_rom_negate", engine_state_desc_type_bits, NULL, struct_offset(___posedge_int_clock__ptr, phase_acc.rom_stage.negate), {1,0,0,0}, {NULL,NULL,NULL,NULL} },
    {"phase_rom_fine_phase", engine_state_desc_type_bits, NULL, struct_offset(___posedge_int_clock__ptr, phase_acc.rom_stage.fine_phase), {4,0,0,0}, {NULL,NULL,NULL,NULL} },
    {"phase_rom_base_value", engine_state_desc_type_bits, NULL, struct_offset(___posedge_int_clock__ptr, phase_acc.rom_stage.rom_base_value), {14,0,0,0}, {NULL,NULL,NULL,NULL} },
    {"phase_rom_delta", engine_state_desc_type_bits, NULL, struct_offset(___posedge_int_clock__ptr, phase_acc.rom_stage.rom_delta), {6,0,0,0}, {NULL,NULL,NULL,NULL} },
    {"phase_rom_error_bits", engine_state_desc_type_bits, NULL, struct_offset(___posedge_int_clock__ptr, phase_acc.rom_stage.rom_error_bits), {16,0,0,0}, {NULL,NULL,NULL,NULL} },
    {"phase_calc_base_value", engine_state_desc_type_bits, NULL, struct_offset(___posedge_int_clock__ptr, phase_acc.calc_in_stage.base_value), {14,0,0,0}, {NULL,NULL,NULL,NULL} },
    {"phase_calc_delta", engine_state_desc_type_bits, NULL, struct_offset(___posedge_int_clock__ptr, phase_acc.calc_in_stage.delta), {6,0,0,0}, {NULL,NULL,NULL,NULL} },
    {"phase_calc_error_bit", engine_state_desc_type_bits, NULL, struct_offset(___posedge_int_clock__ptr, phase_acc.calc_in_stage.error_bit), {1,0,0,0}, {NULL,NULL,NULL,NULL} },
    {"phase_calc_valid_cosine", engine_state_desc_type_bits, NULL, struct_offset(___posedge_int_clock__ptr, phase_acc.calc_in_stage.valid_cosine), {1,0,0,0}, {NULL,NULL,NULL,NULL} },
    {"phase_calc_negate", engine_state_desc_type_bits, NULL, struct_offset(___posedge_int_clock__ptr, phase_acc.calc_in_stage.negate), {1,0,0,0}, {NULL,NULL,NULL,NULL} },
    {"phase_data_out_valid_cosine", engine_state_desc_type_bits, NULL, struct_offset(___posedge_int_clock__ptr, phase_acc.data_out_stage.valid_cosine), {1,0,0,0}, {NULL,NULL,NULL,NULL} },
    {"phase_data_out_value", engine_state_desc_type_bits, NULL, struct_offset(___posedge_int_clock__ptr, phase_acc.data_out_stage.value), {15,0,0,0}, {NULL,NULL,NULL,NULL} },
    {"phase_data_out_sign", engine_state_desc_type_bits, NULL, struct_offset(___posedge_int_clock__ptr, phase_acc.data_out_stage.sign), {1,0,0,0}, {NULL,NULL,NULL,NULL} },

    {"mixer_fsm", engine_state_desc_type_bits, NULL, struct_offset(___posedge_int_clock__ptr, mixer.fsm), {2,0,0,0}, {NULL,NULL,NULL,NULL} },
    {"mixer_valid_out", engine_state_desc_type_bits, NULL, struct_offset(___posedge_int_clock__ptr, mixer.valid_out), {1,0,0,0}, {NULL,NULL,NULL,NULL} },
    {"mixer_i_out", engine_state_desc_type_bits, NULL, struct_offset(___posedge_int_clock__ptr, mixer.i_out), {18,0,0,0}, {NULL,NULL,NULL,NULL} },
    {"mixer_q_out", engine_state_desc_type_bits, NULL, struct_offset(___posedge_int_clock__ptr, mixer.q_out), {18,0,0,0}, {NULL,NULL,NULL,NULL} },
    {"mixer_fsm", engine_state_desc_type_bits, NULL, struct_offset(___posedge_int_clock__ptr, mixer.fsm), {2,0,0,0}, {NULL,NULL,NULL,NULL} },

    {"cic_decimate_count", engine_state_desc_type_bits, NULL, struct_offset(___posedge_int_clock__ptr, cic.decimate.count), {6,0,0,0}, {NULL,NULL,NULL,NULL} },
    {"cic_decimate_factor", engine_state_desc_type_bits, NULL, struct_offset(___posedge_int_clock__ptr, cic.decimate.factor), {6,0,0,0}, {NULL,NULL,NULL,NULL} },
    {"cic_decimate_valid_out", engine_state_desc_type_bits, NULL, struct_offset(___posedge_int_clock__ptr, cic.decimate.valid_out), {1,0,0,0}, {NULL,NULL,NULL,NULL} },

    {"cic_0_int_out_valid", engine_state_desc_type_bits, NULL, struct_offset(___posedge_int_clock__ptr, cic.int_stage[0].valid_out), {1,0,0,0}, {NULL,NULL,NULL,NULL} },
    {"cic_1_int_out_valid", engine_state_desc_type_bits, NULL, struct_offset(___posedge_int_clock__ptr, cic.int_stage[1].valid_out), {1,0,0,0}, {NULL,NULL,NULL,NULL} },
    {"cic_2_int_out_valid", engine_state_desc_type_bits, NULL, struct_offset(___posedge_int_clock__ptr, cic.int_stage[2].valid_out), {1,0,0,0}, {NULL,NULL,NULL,NULL} },
    {"cic_3_int_out_valid", engine_state_desc_type_bits, NULL, struct_offset(___posedge_int_clock__ptr, cic.int_stage[3].valid_out), {1,0,0,0}, {NULL,NULL,NULL,NULL} },
    {"cic_4_int_out_valid", engine_state_desc_type_bits, NULL, struct_offset(___posedge_int_clock__ptr, cic.int_stage[4].valid_out), {1,0,0,0}, {NULL,NULL,NULL,NULL} },
    {"cic_0_int_out", engine_state_desc_type_bits, NULL, struct_offset(___posedge_int_clock__ptr, cic.int_stage[0].out), {CIC_WIDTH,0,0,0}, {NULL,NULL,NULL,NULL} },
    {"cic_1_int_out", engine_state_desc_type_bits, NULL, struct_offset(___posedge_int_clock__ptr, cic.int_stage[1].out), {CIC_WIDTH,0,0,0}, {NULL,NULL,NULL,NULL} },
    {"cic_2_int_out", engine_state_desc_type_bits, NULL, struct_offset(___posedge_int_clock__ptr, cic.int_stage[2].out), {CIC_WIDTH,0,0,0}, {NULL,NULL,NULL,NULL} },
    {"cic_3_int_out", engine_state_desc_type_bits, NULL, struct_offset(___posedge_int_clock__ptr, cic.int_stage[3].out), {CIC_WIDTH,0,0,0}, {NULL,NULL,NULL,NULL} },
    {"cic_4_int_out", engine_state_desc_type_bits, NULL, struct_offset(___posedge_int_clock__ptr, cic.int_stage[4].out), {CIC_WIDTH,0,0,0}, {NULL,NULL,NULL,NULL} },

    {"cic_0_comb_out_valid", engine_state_desc_type_bits, NULL, struct_offset(___posedge_int_clock__ptr, cic.comb_stage[0].valid_out), {1,0,0,0}, {NULL,NULL,NULL,NULL} },
    {"cic_1_comb_out_valid", engine_state_desc_type_bits, NULL, struct_offset(___posedge_int_clock__ptr, cic.comb_stage[1].valid_out), {1,0,0,0}, {NULL,NULL,NULL,NULL} },
    {"cic_2_comb_out_valid", engine_state_desc_type_bits, NULL, struct_offset(___posedge_int_clock__ptr, cic.comb_stage[2].valid_out), {1,0,0,0}, {NULL,NULL,NULL,NULL} },
    {"cic_3_comb_out_valid", engine_state_desc_type_bits, NULL, struct_offset(___posedge_int_clock__ptr, cic.comb_stage[3].valid_out), {1,0,0,0}, {NULL,NULL,NULL,NULL} },
    {"cic_4_comb_out_valid", engine_state_desc_type_bits, NULL, struct_offset(___posedge_int_clock__ptr, cic.comb_stage[4].valid_out), {1,0,0,0}, {NULL,NULL,NULL,NULL} },
    {"cic_0_comb_out", engine_state_desc_type_bits, NULL, struct_offset(___posedge_int_clock__ptr, cic.comb_stage[0].value), {CIC_WIDTH,0,0,0}, {NULL,NULL,NULL,NULL} },
    {"cic_1_comb_out", engine_state_desc_type_bits, NULL, struct_offset(___posedge_int_clock__ptr, cic.comb_stage[1].value), {CIC_WIDTH,0,0,0}, {NULL,NULL,NULL,NULL} },
    {"cic_2_comb_out", engine_state_desc_type_bits, NULL, struct_offset(___posedge_int_clock__ptr, cic.comb_stage[2].value), {CIC_WIDTH,0,0,0}, {NULL,NULL,NULL,NULL} },
    {"cic_3_comb_out", engine_state_desc_type_bits, NULL, struct_offset(___posedge_int_clock__ptr, cic.comb_stage[3].value), {CIC_WIDTH,0,0,0}, {NULL,NULL,NULL,NULL} },
    {"cic_4_comb_out", engine_state_desc_type_bits, NULL, struct_offset(___posedge_int_clock__ptr, cic.comb_stage[4].value), {CIC_WIDTH,0,0,0}, {NULL,NULL,NULL,NULL} },

    {"valid", engine_state_desc_type_bits, NULL, struct_offset(___posedge_int_clock__ptr, valid), {1,0,0,0}, {NULL,NULL,NULL,NULL} },
    {"result", engine_state_desc_type_bits, NULL, struct_offset(___posedge_int_clock__ptr, result), {16,0,0,0}, {NULL,NULL,NULL,NULL} },

    {"", engine_state_desc_type_none, NULL, 0, {0,0,0,0}, {NULL,NULL,NULL,NULL} }
};

/*v struct_offset unwrapper
*/
#undef struct_offset

/*a Static wrapper functions for vector_processor
*/
/*f vector_processor_1r_1w_32_32_instance_fn
*/
static t_sl_error_level vector_processor_instance_fn( c_engine *engine, void *engine_handle )
{
    c_vector_processor *mod;
    mod = new c_vector_processor( engine, engine_handle );
    if (!mod)
        return error_level_fatal;
    return error_level_okay;
}

/*f vector_processor_delete_fn - simple callback wrapper for the main method
*/
static t_sl_error_level vector_processor_delete_fn( void *handle )
{
    c_vector_processor *mod;
    t_sl_error_level result;
    mod = (c_vector_processor *)handle;
    result = mod->delete_instance();
    delete( mod );
    return result;
}

/*f vector_processor_reset_fn
*/
static t_sl_error_level vector_processor_reset_fn( void *handle, int pass )
{
    c_vector_processor *mod;
    mod = (c_vector_processor *)handle;
    return mod->reset( pass );
}

/*f vector_processor_preclock_posedge_int_clock_fn
*/
static t_sl_error_level vector_processor_preclock_posedge_int_clock_fn( void *handle )
{
    c_vector_processor *mod;
    mod = (c_vector_processor *)handle;
    return mod->preclock_posedge_int_clock();
}

/*f vector_processor_clock_posedge_int_clock_fn
*/
static t_sl_error_level vector_processor_clock_posedge_int_clock_fn( void *handle )
{
    c_vector_processor *mod;
    mod = (c_vector_processor *)handle;
    return mod->clock_posedge_int_clock();
}

/*f vector_processor_preclock_posedge_data_clock_fn
*/
static t_sl_error_level vector_processor_preclock_posedge_data_clock_fn( void *handle )
{
    c_vector_processor *mod;
    mod = (c_vector_processor *)handle;
    return mod->preclock_posedge_data_clock();
}

/*f vector_processor_clock_posedge_data_clock_fn
*/
static t_sl_error_level vector_processor_clock_posedge_data_clock_fn( void *handle )
{
    c_vector_processor *mod;
    mod = (c_vector_processor *)handle;
    return mod->clock_posedge_data_clock();
}

/*a Constructors and destructors for vector_processor
*/
/*f c_vector_processor::c_vector_processor
*/
c_vector_processor::c_vector_processor( class c_engine *eng, void *eng_handle )
{
    engine = eng;
    engine_handle = eng_handle;

    phase_resolution = 24; // resolution of the phase accumulator itself
    coarse_resolution = 10; // coarse resolution - resolution of the ROM
    fine_resolution = 4; // steps per ROM entry

    /*b Instantiate module
     */
    engine->register_delete_function( engine_handle, (void *)this, vector_processor_delete_fn );
    engine->register_reset_function( engine_handle, (void *)this, vector_processor_reset_fn );
    engine->register_clock_fns( engine_handle, (void *)this, "int_clock", vector_processor_preclock_posedge_int_clock_fn, vector_processor_clock_posedge_int_clock_fn );
    engine->register_clock_fns( engine_handle, (void *)this, "data_clock", vector_processor_preclock_posedge_data_clock_fn, vector_processor_clock_posedge_data_clock_fn );

    CLOCKED_INPUT( int_reset, 1, int_clock );
    CLOCKED_INPUT( data_0, 16, data_clock );
    CLOCKED_INPUT( data_1, 16, data_clock );
    engine->register_output_signal( engine_handle, "sync_iq_valid", 1, (int *)&state.sync.iq_valid );
    engine->register_output_generated_on_clock( engine_handle, "sync_iq_valid", "int_clock", 1 );
    engine->register_output_signal( engine_handle, "sync_i_out", 16, (int *)&state.sync.i_out );
    engine->register_output_generated_on_clock( engine_handle, "sync_i_out", "int_clock", 1 );
    engine->register_output_signal( engine_handle, "sync_q_out", 16, (int *)&state.sync.q_out );
    engine->register_output_generated_on_clock( engine_handle, "sync_q_out", "int_clock", 1 );

    /*b Register state then reset
     */
    engine->register_state_desc( engine_handle, 1, state_desc_posedge_int_clock, &state, NULL );
    reset( 0 );

    /*b Done
     */

}

/*f c_vector_processor::~c_vector_processor
*/
c_vector_processor::~c_vector_processor()
{
    delete_instance();
}

/*f c_vector_processor::delete_instance
*/
t_sl_error_level c_vector_processor::delete_instance( void )
{
    return error_level_okay;
}

/*a Class reset/preclock/clock methods for vector_processor
*/
/*f c_vector_processor::reset
*/
t_sl_error_level c_vector_processor::reset( int pass )
{
    data_state.selected_toggle = 0;

    state.sync.sync_toggle_0 = 0;
    state.sync.sync_toggle_1 = 0;
    state.sync.sync_last_toggle = 0;
    state.sync.iq_valid = 0;

    state.phase_acc.phase = 0;
    state.phase_acc.phase_step = 7953548; // 9.48/20.0*1<<24 - actually 0x795cbc
    state.phase_acc.phase_step = 7952400; // 9.48/20.0*1<<24 - actually 0x795810
    state.phase_acc.phase_step = 0x7a0000; // beat of 428ns
    state.phase_acc.phase_step = 0x7b0000; // beat of 520ns
    state.phase_acc.phase_step = 0x760000; // beat of 260ns - looking for 307
    state.phase_acc.phase_step = 0x770000; // beat of 288ns - looking for 307
    state.phase_acc.phase_step = 0x77e000; // beat of 315ns - looking for 307
    state.phase_acc.phase_step = 0x77a000; // beat of 306ns - looking for 307
//    state.phase_acc.phase_step = 0;
    state.phase_acc.phase = 0x400000;
    state.phase_acc.fsm = phase_acc_fsm_idle;
    state.phase_acc.rom_stage.rom_reading_cosine = 0;
    state.phase_acc.data_out_stage.valid_cosine = 0;
    state.phase_acc.calc_in_stage.valid_cosine = 0;

    state.mixer.fsm = mixer_fsm_idle;
    state.mixer.valid_out = 0;

    state.cic.int_stage[0].valid_out = 0;
    state.cic.int_stage[1].valid_out = 0;
    state.cic.int_stage[2].valid_out = 0;
    state.cic.int_stage[3].valid_out = 0;
    state.cic.int_stage[4].valid_out = 0;
    state.cic.decimate.count = state.cic.decimate.factor = 9;

    return error_level_okay;
}

/*f c_vector_processor::preclock_posedge_data_clock
*/
t_sl_error_level c_vector_processor::preclock_posedge_data_clock( void )
{
    /*b Copy current to next
     */
    memcpy( &next_data_state, &data_state, sizeof(data_state));

    /*b Synchronizer and input
     */
    next_data_state.data_0_reg = inputs.data_0[0];
    next_data_state.data_1_reg = inputs.data_1[0];
    next_data_state.selected_i = data_state.data_0_reg; // no muxing yet!
    next_data_state.selected_q = data_state.data_1_reg; // no muxing yet!
    next_data_state.selected_toggle = !data_state.selected_toggle;

    /*b Done
     */
    return error_level_okay;
}

/*f c_vector_processor::clock_posedge_data_clock
*/
t_sl_error_level c_vector_processor::clock_posedge_data_clock( void )
{

    /*b Copy next to current
     */
    memcpy( &data_state, &next_data_state, sizeof(data_state));

    /*b Done
     */
    return error_level_okay;
}

/*f c_vector_processor::cic_int_stage_preclock
 */
#define sign_extend(v,w) { (v) |= (v>>(w-1)) ? ((0xffffffffffffffffULL)<<w):0; }
static signed int mult_u16_x_u15_sign_s18( unsigned int a, unsigned int b, int sign )
{
    unsigned int r;
    r = (a*b)>>15; // go from 16+15 = 31 unsigned bits to 16 unsigned bits
    if (sign) // sign extend to 18 signed bits
    {
        r = -r;
    }
    r = r&0x3ffff;
    return r;
}
void c_vector_processor::cic_int_stage_preclock( int stage, int valid_in, unsigned long long value )
{
    stage--;

    next_state.cic.int_stage[stage].calc = state.cic.int_stage[stage].calc;
    next_state.cic.int_stage[stage].out = state.cic.int_stage[stage].out;

    if (valid_in || state.cic.int_stage[stage].valid_out)
    {
        unsigned long long mask;
        mask = 0xffffffffffffffffULL;
        mask <<= CIC_WIDTH;
        next_state.cic.int_stage[stage].out = (state.cic.int_stage[stage].calc + value) &~ mask;
        next_state.cic.int_stage[stage].calc = state.cic.int_stage[stage].out;
    }

    next_state.cic.int_stage[stage].valid_out = valid_in;
}

/*f c_vector_processor::cic_comb_stage_preclock
 */
void c_vector_processor::cic_comb_stage_preclock( int stage, int valid_in, unsigned long long value )
{
    stage--;

    next_state.cic.comb_stage[stage].pipe_0 = state.cic.comb_stage[stage].pipe_0;
    next_state.cic.comb_stage[stage].pipe_1 = state.cic.comb_stage[stage].pipe_1;
    next_state.cic.comb_stage[stage].value  = state.cic.comb_stage[stage].value;

    if (valid_in || state.cic.comb_stage[stage].valid_out)
    {
        unsigned long long mask;
        mask = 0xffffffffffffffffULL;
        mask <<= CIC_WIDTH;
        next_state.cic.comb_stage[stage].pipe_0 = value;
        next_state.cic.comb_stage[stage].pipe_1 = state.cic.comb_stage[stage].pipe_0;
        next_state.cic.comb_stage[stage].value = (value - state.cic.comb_stage[stage].pipe_1) &~ mask;
    }

    next_state.cic.comb_stage[stage].valid_out = valid_in;
}

/*f c_vector_processor::preclock_posedge_int_clock
*/
t_sl_error_level c_vector_processor::preclock_posedge_int_clock( void )
{
    int phase_value_taken;

    /*b Copy current to next
     */
    memcpy( &next_state, &state, sizeof(state));

    /*b Mixer - generates phase_value_taken
     */
    if (1)
    {
        phase_value_taken = 0;
        switch (state.mixer.fsm)
        {
        case mixer_fsm_idle:
            if (state.sync.iq_valid && state.phase_acc.data_out_stage.valid_cosine)
            {
                phase_value_taken = 1;
                next_state.mixer.fsm = mixer_fsm_cosine_done;
            }
            break;
        case mixer_fsm_cosine_done:
            next_state.mixer.fsm = mixer_fsm_sine_done;
            break;
        case mixer_fsm_sine_done:
            next_state.mixer.fsm = mixer_fsm_idle;
            break;
        }

        next_state.mixer.mult_a = mult_u16_x_u15_sign_s18( state.sync.i_out, state.phase_acc.data_out_stage.value, state.phase_acc.data_out_stage.sign );
        next_state.mixer.mult_b = mult_u16_x_u15_sign_s18( state.sync.q_out, state.phase_acc.data_out_stage.value, state.phase_acc.data_out_stage.sign );

        next_state.mixer.valid_out = 0;
        switch (state.mixer.fsm)
        {
        case mixer_fsm_cosine_done:
            next_state.mixer.i_out = state.mixer.mult_a;
            next_state.mixer.q_out = state.mixer.mult_b;
            break;
        case mixer_fsm_sine_done:
            next_state.mixer.i_out = (state.mixer.i_out - state.mixer.mult_b) &0x3ffff;
            next_state.mixer.q_out = (state.mixer.q_out + state.mixer.mult_a) &0x3ffff;
            next_state.mixer.valid_out = 1;
            //next_state.mixer.i_out = next_state.mixer.i_out^0x20000;
            //next_state.mixer.q_out = next_state.mixer.q_out^0x20000;
            break;
        default: break;
        }
    }

    /*b Phase accumulator - needs phase_value_taken
     */
    if (1)
    {
        int clock_phase;
        int read_rom;
        int read_cosine;
        int read_quad;
        int read_phase;
        int rom_address;
        int fine_phase;

        clock_phase = 0;
        read_rom = 0;
        read_cosine = 0;
        switch (state.phase_acc.fsm)
        {
        case phase_acc_fsm_idle:
            if ( phase_value_taken ||
                 (!state.phase_acc.data_out_stage.valid_cosine) )
            {
                read_rom = 1;
                read_cosine = 1;
                clock_phase = 0;
                next_state.phase_acc.fsm = phase_acc_fsm_reading_cosine;
            }
            break;
        case phase_acc_fsm_reading_cosine:
            read_rom = 1;
            read_cosine = 0;
            clock_phase = 0;
            next_state.phase_acc.fsm = phase_acc_fsm_reading_sine;
            break;
        case phase_acc_fsm_reading_sine:
            read_rom = 0;
            read_cosine = 0;
            clock_phase = 1;
            next_state.phase_acc.fsm = phase_acc_fsm_idle;
            break;
        }

        if (clock_phase)
        {
            next_state.phase_acc.phase = (state.phase_acc.phase + state.phase_acc.phase_step) &~ (-1<<phase_resolution);
        }

        if (read_cosine)
        {
            read_quad = ((state.phase_acc.phase >> (phase_resolution-2))+1)&3;
        }
        else
        {
            read_quad = state.phase_acc.phase >> (phase_resolution-2);
        }
        read_phase = state.phase_acc.phase >> (phase_resolution-(coarse_resolution+fine_resolution+2));
        if (read_quad&1)
        {
            read_phase = ~read_phase;
        }
        read_phase = read_phase &~ (-1<<(coarse_resolution+fine_resolution));
        rom_address = read_phase >> fine_resolution;
        fine_phase = read_phase &~ (-1<<fine_resolution);

        {
            next_state.phase_acc.rom_stage.rom_reading_cosine = read_cosine;
            next_state.phase_acc.rom_stage.negate = ((read_quad&2)!=0);
            next_state.phase_acc.rom_stage.fine_phase = fine_phase;
            if (read_rom)
            {
                next_state.phase_acc.rom_stage.rom_base_value = rom[rom_address].base_value;
                next_state.phase_acc.rom_stage.rom_delta      = rom[rom_address].delta;
                next_state.phase_acc.rom_stage.rom_error_bits = rom[rom_address].error_bits;
            }
            else
            {
                next_state.phase_acc.rom_stage.rom_base_value = 0xdead;
                next_state.phase_acc.rom_stage.rom_delta      = 0xdead;
                next_state.phase_acc.rom_stage.rom_error_bits = 0xdead;
            }
        }

        {
            if (!state.phase_acc.data_out_stage.valid_cosine)
            {
                next_state.phase_acc.calc_in_stage.negate     = state.phase_acc.rom_stage.negate;
                next_state.phase_acc.calc_in_stage.fine_phase = state.phase_acc.rom_stage.fine_phase;
                next_state.phase_acc.calc_in_stage.valid_cosine = state.phase_acc.rom_stage.rom_reading_cosine;
                next_state.phase_acc.calc_in_stage.base_value = state.phase_acc.rom_stage.rom_base_value;
                next_state.phase_acc.calc_in_stage.delta      = state.phase_acc.rom_stage.rom_delta;
                next_state.phase_acc.calc_in_stage.error_bit  = (state.phase_acc.rom_stage.rom_error_bits>>(15-state.phase_acc.rom_stage.fine_phase))&1;
            }
        }

        {
            unsigned int delta_sum, value;
            delta_sum = 0;
            if (state.phase_acc.calc_in_stage.fine_phase & (1<<(fine_resolution-1)))
                delta_sum = state.phase_acc.calc_in_stage.delta<<1;
            if (state.phase_acc.calc_in_stage.fine_phase & (1<<(fine_resolution-2)))
                delta_sum += state.phase_acc.calc_in_stage.delta<<0;
            if (state.phase_acc.calc_in_stage.fine_phase & (1<<(fine_resolution-3)))
                delta_sum += state.phase_acc.calc_in_stage.delta>>1;
            if (state.phase_acc.calc_in_stage.fine_phase & (1<<(fine_resolution-4)))
                delta_sum += state.phase_acc.calc_in_stage.delta>>2;

            value = ((state.phase_acc.calc_in_stage.base_value<<1) | state.phase_acc.calc_in_stage.error_bit) + (delta_sum>>1) + (delta_sum&1);

            if ( (!state.phase_acc.data_out_stage.valid_cosine) ||
                 (phase_value_taken) )
            {
                next_state.phase_acc.data_out_stage.value = value;
                next_state.phase_acc.data_out_stage.sign = state.phase_acc.calc_in_stage.negate;
                next_state.phase_acc.data_out_stage.valid_cosine = state.phase_acc.calc_in_stage.valid_cosine;
            }
        }
    }

    /*b Synchronizer
     */
    next_state.sync.sync_reg_i_0 = data_state.selected_i;
    next_state.sync.sync_reg_q_0 = data_state.selected_q;
    next_state.sync.sync_toggle_0 = data_state.selected_toggle;
    next_state.sync.sync_toggle_1 = state.sync.sync_toggle_0;
    next_state.sync.sync_last_toggle = state.sync.sync_toggle_1;
    next_state.sync.iq_valid = 0;
    if (state.sync.sync_last_toggle != state.sync.sync_toggle_1)
    {
        next_state.sync.i_out = state.sync.sync_reg_i_0;
        next_state.sync.q_out = state.sync.sync_reg_q_0;
        next_state.sync.iq_valid = 1;
    }

    /*b CIC filter
     */
    if (1)
    {
        signed long long cic_in;
        cic_in = (signed int) state.cic.q_store;
        if (state.mixer.valid_out)
        {
            next_state.cic.q_store = state.mixer.q_out;
            cic_in = (signed int) state.mixer.i_out;
        }

        sign_extend( cic_in, 17 ); // sign extend
    
        cic_int_stage_preclock( 1, state.mixer.valid_out,            cic_in                     );
        cic_int_stage_preclock( 2, state.cic.int_stage[0].valid_out, state.cic.int_stage[0].out );
        cic_int_stage_preclock( 3, state.cic.int_stage[1].valid_out, state.cic.int_stage[1].out );
        cic_int_stage_preclock( 4, state.cic.int_stage[2].valid_out, state.cic.int_stage[2].out );
        cic_int_stage_preclock( 5, state.cic.int_stage[3].valid_out, state.cic.int_stage[3].out );

        cic_comb_stage_preclock( 5, state.cic.decimate.valid_out, state.cic.int_stage[4].out );
        cic_comb_stage_preclock( 4, state.cic.comb_stage[4].valid_out, state.cic.comb_stage[4].value );
        cic_comb_stage_preclock( 3, state.cic.comb_stage[3].valid_out, state.cic.comb_stage[3].value );
        cic_comb_stage_preclock( 2, state.cic.comb_stage[2].valid_out, state.cic.comb_stage[2].value );
        cic_comb_stage_preclock( 1, state.cic.comb_stage[1].valid_out, state.cic.comb_stage[1].value );

        next_state.cic.decimate.valid_out = 0;
        if (next_state.cic.int_stage[4].valid_out)
        {
            if (state.cic.decimate.count==0)
            {
                next_state.cic.decimate.valid_out = 1;
                next_state.cic.decimate.count = state.cic.decimate.factor;
            }
            else
            {
                next_state.cic.decimate.count = state.cic.decimate.count-1;
            }
        }

        next_state.calc = state.cic.comb_stage[0].valid_out;
        next_state.valid = state.calc;
        if (next_state.calc || next_state.valid)
        {
            unsigned long long a;
            double x;
            a = state.cic.comb_stage[0].value>>(CIC_WIDTH-16);
            x = (double)(a&0x7fff)/32768.0;
            if (a&0x8000) x=x-1;
            if (next_state.calc)
            {
                next_state.pwr_calc = x*x;
            }
            else
            {
                //printf("%lf %lf %lf\n", sqrt(state.pwr_calc), x, sqrt(state.pwr_calc + x*x) );
                x = sqrt(state.pwr_calc + x*x);
                next_state.result = (unsigned int)(32768*x);
            }
        }
    }

    /*b Done
     */
    return error_level_okay;
}

/*f c_vector_processor::clock_posedge_int_clock
*/
t_sl_error_level c_vector_processor::clock_posedge_int_clock( void )
{

    /*b Copy next to current
     */
    memcpy( &state, &next_state, sizeof(state));

    /*b Done
     */
    return error_level_okay;
}

/*a Initialization functions
*/
/*f c_vector_processor__init
*/
extern void c_vector_processor__init( void )
{
    se_external_module_register( 1, "vector_processor", vector_processor_instance_fn );
}

/*a Scripting support code
*/
/*f initvector_processor
*/
extern "C" void initvector_processor( void )
{
    c_vector_processor__init( );
    scripting_init_module( "vector_processor" );
}
