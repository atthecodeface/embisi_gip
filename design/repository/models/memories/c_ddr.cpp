/*a Copyright Gavin J Stark
 */

/*a Includes
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "be_model_includes.h"
#include "sl_mif.h"

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
#define DDR_STATE_OUTPUT( name, width, clk )\
    { \
        engine->register_output_signal( engine_handle, #name, width, (int *)&biedge_int_clock_state.name ); \
        engine->register_output_generated_on_clock( engine_handle, #name, #clk, 1 ); \
    }
#define DDR_ARRAY_STATE_OUTPUT( name, width, clk )\
    { \
        engine->register_output_signal( engine_handle, #name, width, (int *)biedge_int_clock_state.name ); \
        engine->register_output_generated_on_clock( engine_handle, #name, #clk, 1 ); \
    }


/*a Constants
 */
enum
{
// Bank read is page 88, bank write is page 90; state diagram is 48; commands page 20
    tim_rcd = 2, // Trcd is active command to bank active
    tim_rp = 2, // Trp
    tim_rfc = 8, // Trfc
    tim_mrd = 2, // Tmrd, 12ns
    tim_read_to_active = 5,          // Trp+cas latency: 2.5 cycle, plus 20ns (2 cycles); total of 4.5 cycles 
    tim_last_write_data_to_active = 2, // Twr; note that write data occurs Tdqss, 1 cycle, after write command
    tim_wake = 2, // cycles to wake - actually none, but 2 is a bit of margin...
    tim_dll_lock = 200,

// Trcd =20ns is active to read/write
// Tras =40ns is RAS to data valid min (active to data valid; subsumed by active to read, Trcd, and Trp)
// Trp =20ns is precharge - cannot start before Tras from active, but from first data burst presented to Active
// Trap =20ns is active to auto-precharge read
// Trc =65ns is ras to ras (active to active)
// Trfc = 75ns is auto-refresh command period (auto-refresh to any other command)
// Twr = 15ns is write recovery time, last data to active
// Tac = 0.75ns
// Tdh, Tds (DQM/DQ setup/hold to DQS) = 0.5ns
// Tdqss (write command to first DQS latching = 0.75 to 1.25 clocks
// Tdss (DQS falling setup to CK) = 0.2 clocks - this is preamble?
// Tdsh (DQS falling hold from CK) = 0.2 clocks - this is postamble?
// Address/control input setup/hold Tis, Tih = 1ns
// load mode register command cycle time Tmrd = 15ns


};

/*a Types for c_ddr
*/
/*t t_ddr_command
 */
typedef enum 
{
    ddr_command_nop,
    ddr_command_auto_refresh,
    ddr_command_active,
    ddr_command_read,
    ddr_command_write,
    ddr_command_burst_terminate,
    ddr_command_precharge,
    ddr_command_load_mode_register,
    ddr_command_self_refresh,
    ddr_command_wake,
} t_ddr_command;

/*t t_ddr_bank_state
 */
typedef enum 
{
    ddr_bank_state_idle,
    ddr_bank_state_rcd, // can issue no command to the bank; Trcd is being met
    ddr_bank_state_active, // can issue a read, write or precharge; read/write with autoprecharge moves bank to a pre-precharge state, others leave it in active
    ddr_bank_state_rp, // doing precharge; Trp is being met
    ddr_bank_state_rfc, // doing refresh; Trfc being met
    ddr_bank_state_self_refresh, // cke low, running self refresh
    ddr_bank_state_wake, // entering idle
    ddr_bank_state_mrd, // waiting for mode register to take effect
} t_ddr_bank_state;

/*t t_ddr_negedge_int_clock_state
*/
typedef struct t_ddr_negedge_int_clock_state
{
} t_ddr_negedge_int_clock_state;

/*t t_ddr_write_data_state
 */
typedef enum
{
    write_data_state_none,
    write_data_state_pair,
} t_ddr_write_data_state;

/*t t_ddr_posedge_int_clock_state
*/
typedef struct t_ddr_posedge_int_clock_state
{
    int power_down;
    int dll_lock_counter;
    t_ddr_command ddr_command;
    int bank_address; // -1 for all banks
    t_ddr_bank_state bank_state[4];
    t_ddr_bank_state bank_next_state[4]; // state it will be in (if it is in a timing state) when the counter hits 0
    int bank_counter[4];
    int bank_row_address[4];
    int bank_col_address[4];
    int args[2];

    t_ddr_write_data_state write_data_state; // state of the current cycle, i.e. whether and how to write the data on the bus for both edges of this cycle
    unsigned int write_address;
    t_ddr_write_data_state write_data_state_next_cycle; // state of the next cycle, i.e. whether and how to write the data on the bus for both edges of this cycle
    unsigned int write_address_next_cycle;
    
} t_ddr_posedge_int_clock_state;

/*t t_ddr_biedge_int_clock_state
*/
typedef struct t_ddr_biedge_int_clock_state
{
    unsigned int dq_out_fifo[8*8];
    unsigned int dqs_out_fifo[8];
    unsigned int dq_oe_fifo;
    unsigned int dq_out[8];
    unsigned int dqs_out;
    unsigned int dq_oe;
    unsigned int dq_in[8];
    unsigned int dqm_in;
} t_ddr_biedge_int_clock_state;

/*t t_ddr_inputs
*/
typedef struct t_ddr_inputs
{
    unsigned int *cke;
    unsigned int *select_n;
    unsigned int *ras_n;
    unsigned int *cas_n;
    unsigned int *we_n;
    unsigned int *a;
    unsigned int *ba;
    unsigned int *dqs_in;
    unsigned int *dq_in;
    unsigned int *dqm;
} t_ddr_inputs;

/*t c_ddr
*/
class c_ddr
{
public:
    c_ddr::c_ddr( class c_engine *eng, void *eng_handle, int size, int byte_width );
    c_ddr::~c_ddr();
    t_sl_error_level c_ddr::delete_instance( void );
    t_sl_error_level c_ddr::reset( int pass );
    t_sl_error_level c_ddr::preclock_posedge_int_clock( void );
    t_sl_error_level c_ddr::clock_posedge_int_clock( void );
    t_sl_error_level c_ddr::preclock_negedge_int_clock( void );
    t_sl_error_level c_ddr::clock_negedge_int_clock( void );
    void c_ddr::biedge_preclock( void );
    void c_ddr::biedge_clock( void );
private:
    void c_ddr::do_write_data( int offset );
    c_engine *engine;
    void *engine_handle;
    t_ddr_inputs inputs;
    t_ddr_posedge_int_clock_state next_posedge_int_clock_state;
    t_ddr_posedge_int_clock_state posedge_int_clock_state;
    t_ddr_negedge_int_clock_state next_negedge_int_clock_state;
    t_ddr_negedge_int_clock_state negedge_int_clock_state;
    t_ddr_biedge_int_clock_state next_biedge_int_clock_state;
    t_ddr_biedge_int_clock_state biedge_int_clock_state;

    char *filename;
    int debug;

    unsigned int ddr_size;
    int ddr_int_width; // width in ints
    int ddr_byte_width; // width in bytes

    int *memory; // mallocked array of width in bytes * size
};

/*a Static variables
 */
static char *ddr_command_name[] =
{
    "nop",
    "autorefresh",
    "active",
    "read",
    "write",
    "burst terminate",
    "precharge",
    "load mode register",
    "self refresh",
    "wake",
};

/*a Static wrapper functions for ddr
*/
/*f ddr_32m_x_16_instance_fn
*/
static t_sl_error_level ddr_32m_x_16_instance_fn( c_engine *engine, void *engine_handle )
{
    c_ddr *mod;
    mod = new c_ddr( engine, engine_handle, 32*1024*1024, 2 ); // 32M words of 4 bytes each
    if (!mod)
        return error_level_fatal;
    return error_level_okay;
}

/*f ddr_16m_x_32_instance_fn
*/
static t_sl_error_level ddr_16m_x_32_instance_fn( c_engine *engine, void *engine_handle )
{
    c_ddr *mod;
    mod = new c_ddr( engine, engine_handle, 16*1024*1024, 4 ); // 16M words of 4 bytes each
    if (!mod)
        return error_level_fatal;
    return error_level_okay;
}

/*f ddr_delete_fn - simple callback wrapper for the main method
*/
static t_sl_error_level ddr_delete_fn( void *handle )
{
    c_ddr *mod;
    t_sl_error_level result;
    mod = (c_ddr *)handle;
    result = mod->delete_instance();
    delete( mod );
    return result;
}

/*f ddr_reset_fn
*/
static t_sl_error_level ddr_reset_fn( void *handle, int pass )
{
    c_ddr *mod;
    mod = (c_ddr *)handle;
    return mod->reset( pass );
}

/*f ddr_preclock_negedge_int_clock_fn
*/
static t_sl_error_level ddr_preclock_negedge_int_clock_fn( void *handle )
{
    c_ddr *mod;
    mod = (c_ddr *)handle;
    return mod->preclock_negedge_int_clock();
}

/*f ddr_clock_negedge_int_clock_fn
*/
static t_sl_error_level ddr_clock_negedge_int_clock_fn( void *handle )
{
    c_ddr *mod;
    mod = (c_ddr *)handle;
    return mod->clock_negedge_int_clock();
}

/*f ddr_preclock_posedge_int_clock_fn
*/
static t_sl_error_level ddr_preclock_posedge_int_clock_fn( void *handle )
{
    c_ddr *mod;
    mod = (c_ddr *)handle;
    return mod->preclock_posedge_int_clock();
}

/*f ddr_clock_posedge_int_clock_fn
*/
static t_sl_error_level ddr_clock_posedge_int_clock_fn( void *handle )
{
    c_ddr *mod;
    mod = (c_ddr *)handle;
    return mod->clock_posedge_int_clock();
}

/*a Constructors and destructors for ddr
*/
/*f c_ddr::c_ddr
*/
c_ddr::c_ddr( class c_engine *eng, void *eng_handle, int size, int byte_width )
{
    engine = eng;
    engine_handle = eng_handle;

    /*b Determine configuration
     */
    debug = engine->get_option_int( engine_handle, "debug", 0 );
    filename = engine->get_option_string( engine_handle, "filename", "" );
    ddr_size = size;
    ddr_byte_width = byte_width;
    ddr_int_width = (byte_width+sizeof(int)-1)/(sizeof(int));
    memory = NULL;
    
    /*b Instantiate module
     */
    engine->register_delete_function( engine_handle, (void *)this, ddr_delete_fn );
    engine->register_reset_function( engine_handle, (void *)this, ddr_reset_fn );
    engine->register_clock_fns( engine_handle, (void *)this, "ddr_clock", ddr_preclock_posedge_int_clock_fn, ddr_clock_posedge_int_clock_fn, ddr_preclock_negedge_int_clock_fn, ddr_clock_negedge_int_clock_fn );

    CLOCKED_INPUT( cke, 1, ddr_clock );
    CLOCKED_INPUT( select_n, 1, ddr_clock );
    CLOCKED_INPUT( ras_n, 1, ddr_clock );
    CLOCKED_INPUT( cas_n, 1, ddr_clock );
    CLOCKED_INPUT( we_n, 1, ddr_clock );
    CLOCKED_INPUT( ba, 2, ddr_clock );
    CLOCKED_INPUT( a, 13, ddr_clock );
    DDR_CLOCKED_INPUT( dqs_in, ddr_byte_width, ddr_clock );
    DDR_CLOCKED_INPUT( dq_in, ddr_byte_width*8, ddr_clock );
    DDR_CLOCKED_INPUT( dqm, ddr_byte_width, ddr_clock );

    DDR_STATE_OUTPUT( dq_oe, 1, ddr_clock ); // this lets it be changed by both edges of the clock
    DDR_STATE_OUTPUT( dqs_out, ddr_byte_width, ddr_clock ); // this lets it be changed by both edges of the clock
    DDR_ARRAY_STATE_OUTPUT( dq_out, ddr_byte_width*8, ddr_clock ); // this lets it be changed by both edges of the clock

    /*b Register state then reset
     */
    reset( 0 );

    /*b Done
     */

}

/*f c_ddr::~c_ddr
*/
c_ddr::~c_ddr()
{
    delete_instance();
}

/*f c_ddr::delete_instance
*/
t_sl_error_level c_ddr::delete_instance( void )
{
    return error_level_okay;
}

/*a Class reset/preclock/clock methods for ddr
*/
/*f c_ddr::reset
*/
t_sl_error_level c_ddr::reset( int pass )
{
    int i, j;

    if (pass==0)
    {
        if (memory)
        {
            free(memory);
            memory = NULL;
        }
        sl_mif_allocate_and_read_mif_file( engine->error,
                                           strcmp(filename,"")?filename:NULL,
                                           "se_internal_module__sram_srw",
                                           0, // address offset 0
                                           (int)ddr_size,
                                           ((ddr_byte_width<4)?32:(ddr_int_width*32)),
                                           0,
                                           2, // reset style 2
                                           0x11111111, // reset value, add 0x1111111 to each address when storing it in the data
                                           (int **)&memory,
                                           NULL,
                                           NULL );
    }
    posedge_int_clock_state.power_down = 1;
    posedge_int_clock_state.dll_lock_counter = 0;
    posedge_int_clock_state.ddr_command = ddr_command_nop;
    posedge_int_clock_state.bank_address = -1;
    for (i=0; i<4; i++)
    {
        posedge_int_clock_state.bank_row_address[i] = 0;
        posedge_int_clock_state.bank_col_address[i] = 0;
        posedge_int_clock_state.bank_counter[i] = 0;
        posedge_int_clock_state.bank_state[i] = ddr_bank_state_self_refresh;
        posedge_int_clock_state.bank_next_state[i] = ddr_bank_state_idle;
    }

    posedge_int_clock_state.write_data_state = write_data_state_none;
    posedge_int_clock_state.write_data_state_next_cycle = write_data_state_none;
    posedge_int_clock_state.write_address = 0;
    posedge_int_clock_state.write_address_next_cycle = 0;

    for (i=0; i<8; i++)
    {
        for (j=0; j<8; j++)
        {
            biedge_int_clock_state.dq_out_fifo[i*8+j] = 0xdeaddead;
        }
        biedge_int_clock_state.dqs_out_fifo[i] = 0;
    }
    for (j=0; j<8; j++)
    {
        biedge_int_clock_state.dq_out[j] = 0xdeaddead;
    }
    biedge_int_clock_state.dqs_out = 0;
    biedge_int_clock_state.dq_oe_fifo = 0; // shift register
    biedge_int_clock_state.dq_oe = 0;
    for (j=0; j<8; j++)
    {
        biedge_int_clock_state.dq_in[j] = 0;
    }
    biedge_int_clock_state.dqm_in = 0;
    return error_level_okay;
}

/*f c_ddr::preclock_posedge_int_clock
*/
t_sl_error_level c_ddr::preclock_posedge_int_clock( void )
{
    /*b First to the biedge stuff
     */
    biedge_preclock();

    /*b Copy current state to next state
     */
    memcpy( &next_posedge_int_clock_state, &posedge_int_clock_state, sizeof(posedge_int_clock_state) );

    /*b Interpret command on the bus
     */
    next_posedge_int_clock_state.ddr_command = ddr_command_nop;
    if (inputs.cke[0] == 0 )
    {
        if (posedge_int_clock_state.power_down==0)
        {
            next_posedge_int_clock_state.ddr_command = ddr_command_self_refresh;
            next_posedge_int_clock_state.bank_address = -1;
            next_posedge_int_clock_state.power_down = 1;
        }
    }
    else
    {
        if (posedge_int_clock_state.power_down)
        {
            next_posedge_int_clock_state.ddr_command = ddr_command_wake;
            next_posedge_int_clock_state.bank_address = -1;
            next_posedge_int_clock_state.power_down = 0;
        }
        else if (inputs.select_n[0]==0)
        {
            switch ( ((inputs.ras_n[0]&1)<<2) |
                     ((inputs.cas_n[0]&1)<<1) |
                     ((inputs.we_n[0]&1)<<0) )
            {
            case 0:
                next_posedge_int_clock_state.ddr_command = ddr_command_load_mode_register;
                next_posedge_int_clock_state.args[0] = inputs.ba[0];
                next_posedge_int_clock_state.args[1] = inputs.a[0];
                next_posedge_int_clock_state.bank_address = -1;
                break;
            case 1:
                next_posedge_int_clock_state.ddr_command = ddr_command_auto_refresh;
                next_posedge_int_clock_state.bank_address = -1;
                break;
            case 2:
                next_posedge_int_clock_state.ddr_command = ddr_command_precharge;
                if (inputs.a[0]&0x400)
                {
                    next_posedge_int_clock_state.bank_address = -1;
                }
                else
                {
                    next_posedge_int_clock_state.bank_address = inputs.ba[0];
                }
                break;
            case 3:
                next_posedge_int_clock_state.ddr_command = ddr_command_active;
                next_posedge_int_clock_state.bank_address = inputs.ba[0];
                break;
            case 4:
                next_posedge_int_clock_state.ddr_command = ddr_command_write;
                next_posedge_int_clock_state.bank_address = inputs.ba[0];
                break;
            case 5:
                next_posedge_int_clock_state.ddr_command = ddr_command_read;
                next_posedge_int_clock_state.bank_address = inputs.ba[0];
                break;
            case 6:
                next_posedge_int_clock_state.ddr_command = ddr_command_burst_terminate;
                next_posedge_int_clock_state.bank_address = inputs.ba[0];
                break;
            case 7:
                next_posedge_int_clock_state.ddr_command = ddr_command_nop;
                next_posedge_int_clock_state.bank_address = -1;
                break;
            }
        }
    }

    /*b Done
     */
    return error_level_okay;
}

/*f c_ddr::do_write_data
 */
void c_ddr::do_write_data( int offset )
{
    int j;
    unsigned int address = posedge_int_clock_state.write_address;
    // we were expecting strobe low before the edge, strobe high now. Difficult to verify. But data should be stable for the edge anyway in a cycle simulation
    // however, we certainly expect it to follow clock, so it should toggle on every edge here
    // ignore for now
    // this data is the first of the pair
    // we 
    if ( memory &&
         (address+offset<ddr_size) )
    {
        unsigned int mask;
        int dqm;
        dqm = biedge_int_clock_state.dqm_in; 
        if (ddr_byte_width>=4)
        {
            address+=offset;
            for (j=0; j<ddr_int_width; j++)
            {
                mask = 0;
                if (dqm&1) { mask|= 0xff; }
                if (dqm&2) { mask|= 0xff00; }
                if (dqm&4) { mask|= 0xff0000; }
                if (dqm&8) { mask|= 0xff000000; }
                memory[ address*ddr_int_width+j ] = (memory[ address*ddr_int_width+j ] & mask) | (~mask & biedge_int_clock_state.dq_in[j]);
                if (debug&4) fprintf(stderr,"Writing %08x/%08x:%08x\n",address, memory[ address*ddr_int_width+j],mask );
                dqm>>=4;
            }
        }
        else
        {
            mask = 0xff;
            offset *= ddr_byte_width*8;
            for (j=0; j<ddr_byte_width; j++)
            {
                if (!(dqm&1))
                {
                    memory[ address ] = (memory[ address ] & ~(mask<<offset)) | ((mask & biedge_int_clock_state.dq_in[0])<<offset);
                    if (debug&4) fprintf(stderr,"Writing byte %d %08x/%08x:%08x:%d\n",j, address, memory[ address ],mask<<offset,dqm&1 );
                }
                dqm>>=1;
                mask<<=8;
            }
        }
    }
}

/*f c_ddr::clock_posedge_int_clock
*/
t_sl_error_level c_ddr::clock_posedge_int_clock( void )
{
    int i, j, k;
    int cmd_to_bank;

    /*b First to the biedge stuff
     */
    biedge_clock();

    /*b Copy next state to current
    */
    memcpy( &posedge_int_clock_state, &next_posedge_int_clock_state, sizeof(posedge_int_clock_state) );

    /*b Handle write data
     */
    posedge_int_clock_state.write_data_state = posedge_int_clock_state.write_data_state_next_cycle;
    posedge_int_clock_state.write_address = posedge_int_clock_state.write_address_next_cycle;
    posedge_int_clock_state.write_data_state_next_cycle = write_data_state_none;
    if (posedge_int_clock_state.write_data_state == write_data_state_pair)
    {
        do_write_data( 0 );
    }

    /*b Handle DLL
     */
    if (posedge_int_clock_state.dll_lock_counter>0)
    {
        posedge_int_clock_state.dll_lock_counter = posedge_int_clock_state.dll_lock_counter-1;
    }

    /*b Handle the banks
     */
    for (i=0; i<4; i++)
    {
        /*b Decrement counter
         */
        if (posedge_int_clock_state.bank_counter[i]==1)
        {
            posedge_int_clock_state.bank_counter[i] = 0;
            posedge_int_clock_state.bank_state[i] = posedge_int_clock_state.bank_next_state[i];
        }
        else if (posedge_int_clock_state.bank_counter[i]>0)
        {
            posedge_int_clock_state.bank_counter[i] = posedge_int_clock_state.bank_counter[i]-1;
        }

        /*b Handle command
         */
        cmd_to_bank = 0;
        if ( (posedge_int_clock_state.ddr_command!=ddr_command_nop) &&
             ((posedge_int_clock_state.bank_address==i) || (posedge_int_clock_state.bank_address==-1)) )
        {
            cmd_to_bank = 1;
        }
        switch (posedge_int_clock_state.bank_state[i])
        {
            /*b Self refresh
             */
        case ddr_bank_state_self_refresh:
            if (cmd_to_bank)
            {
                if (posedge_int_clock_state.ddr_command == ddr_command_wake)
                {
                    posedge_int_clock_state.bank_state[i] = ddr_bank_state_wake;
                    posedge_int_clock_state.bank_next_state[i] = ddr_bank_state_idle;
                    posedge_int_clock_state.bank_counter[i] = tim_wake;
                    posedge_int_clock_state.dll_lock_counter = tim_dll_lock;
                }
                else
                {
                    ERROR("unexpected DDR command '%s' when bank was in self refresh", ddr_command_name[posedge_int_clock_state.ddr_command]);
                }
            }
            break;
            /*b Idle
             */
        case ddr_bank_state_idle:
            if (cmd_to_bank)
            {
                switch (posedge_int_clock_state.ddr_command)
                {
                case ddr_command_active:
                    posedge_int_clock_state.bank_row_address[i] = inputs.a[0]&0x1fff;
                    posedge_int_clock_state.bank_state[i] = ddr_bank_state_rcd;
                    posedge_int_clock_state.bank_next_state[i] = ddr_bank_state_active;
                    posedge_int_clock_state.bank_counter[i] = tim_rcd;
                    break;
                case ddr_command_precharge:
                    posedge_int_clock_state.bank_state[i] = ddr_bank_state_rp;
                    posedge_int_clock_state.bank_next_state[i] = ddr_bank_state_idle;
                    posedge_int_clock_state.bank_counter[i] = tim_rp;
                    break;
                case ddr_command_auto_refresh:
                    posedge_int_clock_state.bank_state[i] = ddr_bank_state_rfc;
                    posedge_int_clock_state.bank_next_state[i] = ddr_bank_state_idle;
                    posedge_int_clock_state.bank_counter[i] = tim_rfc;
                    break;
                case ddr_command_load_mode_register:
                    posedge_int_clock_state.bank_state[i] = ddr_bank_state_mrd;
                    posedge_int_clock_state.bank_next_state[i] = ddr_bank_state_idle;
                    posedge_int_clock_state.bank_counter[i] = tim_mrd;
                    switch (posedge_int_clock_state.args[0]&3)
                    {
                    case 0: // mode reg
                        //fprintf(stderr,"%s:%d:DDR programming mode register %08x\n", engine->get_instance_name(engine_handle), engine->cycle(), posedge_int_clock_state.args[1] );
                        if (posedge_int_clock_state.args[1]&1)
                        {
                            posedge_int_clock_state.dll_lock_counter = tim_dll_lock;
                        }
                        break;
                    case 1: // ext mode reg
                        //fprintf(stderr,"%s:%d:DDR programming ext mode register %08x\n", engine->get_instance_name(engine_handle), engine->cycle(), posedge_int_clock_state.args[1] );
                        break;
                    default: // error
                        ERROR("unexpected mode register number (%d) in load mode command\n", posedge_int_clock_state.args[0]&3);
                        break;
                    }
                    break;
                default:
                    ERROR("unexpected DDR command '%s' when bank was idle", ddr_command_name[posedge_int_clock_state.ddr_command]);
                    break;
                }
            }
            break;
            /*b Rcd, rp, rfc, wake, mrd
             */
        case ddr_bank_state_wake:
        case ddr_bank_state_rcd:
        case ddr_bank_state_rfc:
        case ddr_bank_state_rp:
        case ddr_bank_state_mrd:
            if (cmd_to_bank)
            {
                ERROR( "unexpected DDR command '%s' when command timing was being met (still had %d to go)", ddr_command_name[posedge_int_clock_state.ddr_command], posedge_int_clock_state.bank_counter[i] );
                posedge_int_clock_state.bank_state[i] = ddr_bank_state_idle;
            }
            break;
            /*b Active
             */
        case ddr_bank_state_active:
            if (cmd_to_bank)
            {
                unsigned int address;
                switch (posedge_int_clock_state.ddr_command)
                {
                case ddr_command_read:
                    if (posedge_int_clock_state.dll_lock_counter>0)
                    {
                        ERROR( "read when DLL had not locked - still had %d to go", posedge_int_clock_state.dll_lock_counter);
                    }
                    posedge_int_clock_state.bank_col_address[i] = inputs.a[0]&0x1ff;
                    if (inputs.a[0]&0x400) // autoprecharge
                    {
                        posedge_int_clock_state.bank_state[i] = ddr_bank_state_rp;
                        posedge_int_clock_state.bank_counter[i] = tim_rp;
                        posedge_int_clock_state.bank_next_state[i] = ddr_bank_state_idle;
                    }
                    address = ((posedge_int_clock_state.bank_row_address[i]<<9) | (posedge_int_clock_state.bank_col_address[i]));
                    biedge_int_clock_state.dqs_out_fifo[4] = 0;
                    biedge_int_clock_state.dq_oe_fifo = biedge_int_clock_state.dq_oe_fifo | 0xf8; // oe fires after 3 half-ticks, for 5 ticks (see pg 27)
                    for (k=0; k<2; k++) // burst of 2
                    {
                        if ( memory &&
                             (address<ddr_size) )
                        {
                            biedge_int_clock_state.dqs_out_fifo[k+5] = (k&1)?0x0:0xf;
                            if (ddr_byte_width>=4)
                            {
                                for (j=0; j<ddr_int_width; j++)
                                {
                                    biedge_int_clock_state.dq_out_fifo[(k+5)*8+j] = memory[ (address+k)*ddr_int_width+j ];
                                    if (debug&2) fprintf(stderr,"Reading %08x/%08x\n",(address+k),memory[ (address+k)*ddr_int_width+j ]);
                                }
                            }
                            else
                            {
                                biedge_int_clock_state.dq_out_fifo[(k+5)*8] = memory[ address ] >> (k*8*ddr_byte_width);
                                if (debug&2) fprintf(stderr,"Reading %08x/%08x\n",address,memory[ address ]);
                            }
                        }
                    }
                    break;
                case ddr_command_write:
                    posedge_int_clock_state.bank_col_address[i] = inputs.a[0]&0x1ff;
                    if (inputs.a[0]&0x400) // autoprecharge
                    {
                        posedge_int_clock_state.bank_state[i] = ddr_bank_state_rp;
                        posedge_int_clock_state.bank_counter[i] = tim_rp;
                        posedge_int_clock_state.bank_next_state[i] = ddr_bank_state_idle;
                    }
                    posedge_int_clock_state.write_data_state_next_cycle = write_data_state_pair;
                    posedge_int_clock_state.write_address_next_cycle = ((posedge_int_clock_state.bank_row_address[i]<<9) | (posedge_int_clock_state.bank_col_address[i]));
                    break;
                case ddr_command_precharge:
                    posedge_int_clock_state.bank_state[i] = ddr_bank_state_rp;
                    posedge_int_clock_state.bank_next_state[i] = ddr_bank_state_idle;
                    posedge_int_clock_state.bank_counter[i] = tim_rp;
                    break;
                default:
                    ERROR( "unexpected DDR command '%s' when in active state", ddr_command_name[posedge_int_clock_state.ddr_command] );
                    posedge_int_clock_state.bank_state[i] = ddr_bank_state_idle;
                    break;
                }
                break;
            }
        }
    }

    /*b Verbose
     */
    if (debug&1)
    {
        if (posedge_int_clock_state.ddr_command != ddr_command_nop)
        {
            fprintf(stderr,"%s:%d:DDR command %s\n", engine->get_instance_name(engine_handle), engine->cycle(), ddr_command_name[posedge_int_clock_state.ddr_command]);
        }
    }
    /*b Done
     */
    return error_level_okay;
}

/*f c_ddr::preclock_negedge_int_clock
*/
t_sl_error_level c_ddr::preclock_negedge_int_clock( void )
{
    /*b First to the biedge stuff
     */
    biedge_preclock();

    /*b Copy current state to next state
     */
    memcpy( &next_negedge_int_clock_state, &negedge_int_clock_state, sizeof(negedge_int_clock_state) );

    /*b Done
     */
    return error_level_okay;
}

/*f c_ddr::clock_negedge_int_clock
*/
t_sl_error_level c_ddr::clock_negedge_int_clock( void )
{

    /*b First to the biedge stuff
     */
    biedge_clock();

    /*b Copy next state to current
    */
    memcpy( &negedge_int_clock_state, &next_negedge_int_clock_state, sizeof(negedge_int_clock_state) );

    /*b Handle write data
     */
    if (posedge_int_clock_state.write_data_state == write_data_state_pair)
    {
        do_write_data( 1 );
    }

    /*b Done
     */
    return error_level_okay;
}

/*f c_ddr::biedge_preclock
*/
void c_ddr::biedge_preclock( void )
{
    int j;
    for (j=0; j<(int)ddr_int_width; j++)
    {
        biedge_int_clock_state.dq_in[j] = inputs.dq_in[j];
    }
    biedge_int_clock_state.dqm_in = inputs.dqm[0];
}

/*f c_ddr::biedge_clock
*/
void c_ddr::biedge_clock( void )
{
    int j;
    memcpy( biedge_int_clock_state.dq_out_fifo, &(biedge_int_clock_state.dq_out_fifo[8]), sizeof(biedge_int_clock_state.dq_out_fifo)-8*sizeof(int));
    memcpy( biedge_int_clock_state.dqs_out_fifo, &(biedge_int_clock_state.dqs_out_fifo[1]), sizeof(biedge_int_clock_state.dqs_out_fifo)-sizeof(int));
    biedge_int_clock_state.dq_oe_fifo = biedge_int_clock_state.dq_oe_fifo>>1;
    for (j=0; j<(int)ddr_int_width; j++)
    {
        biedge_int_clock_state.dq_out[j] = biedge_int_clock_state.dq_out_fifo[j];
    }
    biedge_int_clock_state.dqs_out = biedge_int_clock_state.dqs_out_fifo[0];
    biedge_int_clock_state.dq_oe = biedge_int_clock_state.dq_oe_fifo&1;
}

/*a Initialization functions
*/
/*f c_ddr__init
*/
extern void c_ddr__init( void )
{
    se_external_module_register( 1, "ddr_16m_x_32", ddr_16m_x_32_instance_fn );
    se_external_module_register( 1, "ddr_32m_x_16", ddr_32m_x_16_instance_fn );
}

/*a Scripting support code
*/
/*f initddr
*/
extern "C" void initddr( void )
{
    c_ddr__init( );
    scripting_init_module( "ddr" );
}
