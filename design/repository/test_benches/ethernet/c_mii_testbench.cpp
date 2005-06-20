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
#define MAX_CHANNELS (1)
#define MAX_SOURCES (1)
#define MAX_SINKS (1)
#define MAX_TGT_COLLISION_LIST (256)
#define MAX_SRC_COLLISION_LIST (256)
#define MAX_PACKET_LENGTH (2048)

#define ASSIGN_TO_BIT(vector,size,bit,value) se_cmodel_assist_assign_to_bit(vector,size,bit,value)
#define ASSIGN_TO_BIT_RANGE(vector,size,bit,length,value) se_cmodel_assist_assign_to_bit_range(vector,size,bit,length,value)
#define COVER_STATEMENT(stmt_number) {if(stmt_number>=0)se_cmodel_assist_coverage_statement_reached(coverage,stmt_number);}
#define ASSERT(expr,string) {if (!(expr)) {\
    engine->message->add_error( NULL, error_level_info, error_number_se_dated_assertion, error_id_sl_exec_file_allocate_and_read_exec_file, \
        error_arg_type_integer, engine->cycle(),\
        error_arg_type_malloc_string, engine->get_instance_name(engine_handle),  \
        error_arg_type_const_string, string,\
        error_arg_type_none );}}

#define FAIL(expr,string) {if ((expr)) {\
    next_posedge_int_clock_state.mon_failure = ~posedge_int_clock_state.mon_failure; \
    engine->message->add_error( NULL, error_level_info, error_number_se_dated_assertion, error_id_sl_exec_file_allocate_and_read_exec_file, \
        error_arg_type_integer, engine->cycle(),\
        error_arg_type_malloc_string, engine->get_instance_name(engine_handle),  \
        error_arg_type_const_string, string,\
        error_arg_type_none );}}


/*a Types for c_mii_testbench
*/
/*t t_src_fsm - RX MII driver FSM
*/
typedef enum t_src_fsm
{
    src_fsm_idle,
    src_fsm_preamble,
    src_fsm_end_sfd,
    src_fsm_data,
    src_fsm_error,
};

/*t t_tgt_fsm - TX MII sink FSM
*/
typedef enum t_tgt_fsm
{
    tgt_fsm_idle,
    tgt_fsm_preamble,
    tgt_fsm_data,
};

/*t t_mon_fsm - TX MII monitor
*/
typedef enum t_mon_fsm
{
    mon_fsm_idle,
    mon_fsm_preamble,
    mon_fsm_data,
    mon_fsm_collision,
};

/*t mon_level_*
 */
enum
{
    mon_level_none = 0,
    mon_level_verbose = 1, // Output a message on every clock with the signals on the bus
    mon_level_protocol = 2, // Check the mii protocol is adhered to
    mon_level_data = 4, // Check data according to type given in the transaction word (if length is nonzero)
};

/*t t_eth_packet
 */
typedef struct t_eth_packet
{
    int preclocked;
    int length;
    int complete;
    int next_nybble;
    int last_nybble;
    unsigned int fcs;
    unsigned char data[MAX_PACKET_LENGTH];
} t_eth_packet;

/*t t_source
*/
typedef struct t_source
{
    t_sl_data_stream *data_stream;
    int interval;
    int delay;
} t_source;

/*t t_sink
*/
typedef struct t_sink
{
    t_sl_data_stream *data_stream;
} t_sink;

/*t t_sink_state
*/
typedef struct t_sink_state
{
    int counter;
} t_sink_state;

/*t t_source_state
*/
typedef struct t_source_state
{
    int counter;
    int pending;
} t_source_state;

/*t t_mii_testbench_posedge_int_clock_state
*/
typedef struct t_mii_testbench_posedge_int_clock_state
{
    t_src_fsm src_fsm;
    unsigned int src_data;
    int src_left;
    int src_collide;
    int src_nybble_count;
    int src_collision_list_entry;
    int src_source;
    t_source_state src_source_state[MAX_SOURCES];

    t_tgt_fsm tgt_fsm;
    int tgt_collide;
    int tgt_nybble_count;
    int tgt_collision_list_entry;
    t_sink_state tgt_sink_state[MAX_SINKS];

    t_mon_fsm mon_fsm;
    int mon_count;
    unsigned int mon_failure;

} t_mii_testbench_posedge_int_clock_state;

/*t t_mii_testbench_inputs
*/
typedef struct t_mii_testbench_inputs
{
    unsigned int *int_reset;

    unsigned int *mon_mii_enable;
    unsigned int *mon_mii_data;
    unsigned int *mon_mii_crs;
    unsigned int *mon_mii_col;

    unsigned int *tx_mii_enable;
    unsigned int *tx_mii_data;

} t_mii_testbench_inputs;

/*t t_mii_testbench_combinatorials
*/
typedef struct t_mii_testbench_combinatorials
{
    unsigned int rx_mii_dv;
    unsigned int rx_mii_data;
    unsigned int rx_mii_err;
    unsigned int tx_mii_crs;
    unsigned int tx_mii_col;

} t_mii_testbench_combinatorials;

/*t c_mii_testbench
*/
class c_mii_testbench
{
public:
    c_mii_testbench::c_mii_testbench( class c_engine *eng, void *eng_handle );
    c_mii_testbench::~c_mii_testbench();
    t_sl_error_level c_mii_testbench::delete_instance( void );
    t_sl_error_level c_mii_testbench::reset( int pass );
    t_sl_error_level c_mii_testbench::evaluate_combinatorials( void );
    t_sl_error_level c_mii_testbench::preclock_posedge_int_clock( void );
    t_sl_error_level c_mii_testbench::clock_posedge_int_clock( void );
    t_sl_error_level c_mii_testbench::reset_active_high_int_reset( void );
private:
    c_engine *engine;
    void *engine_handle;
    t_mii_testbench_inputs inputs;
    t_mii_testbench_combinatorials combinatorials;
    t_mii_testbench_posedge_int_clock_state next_posedge_int_clock_state;
    t_mii_testbench_posedge_int_clock_state posedge_int_clock_state;

    int num_sources;
    int num_sinks;
    int monitor_level;
    t_source sources[MAX_SOURCES];
    t_sink sinks[MAX_SINKS];
    int tgt_collision_list[MAX_TGT_COLLISION_LIST];
    int tgt_collision_list_length;
    int src_collision_list[MAX_SRC_COLLISION_LIST];
    int src_collision_list_length;

    t_eth_packet mon_packet;
    t_eth_packet tgt_packet;
    t_eth_packet src_packet;
};

/*a Static variables for mii_testbench
*/
/*f state_desc_mii_testbench
*/
#define struct_offset( ptr, a ) (((char *)&(ptr->a))-(char *)ptr)
static t_mii_testbench_posedge_int_clock_state *___mii_testbench_posedge_int_clock__ptr;
static t_engine_state_desc state_desc_mii_testbench_posedge_int_clock[] =
{
//    {"src_fsm", engine_state_desc_type_bits, NULL, struct_offset(___mii_testbench_posedge_int_clock__ptr, src_fsm), {2,0,0,0}, {NULL,NULL,NULL,NULL} },
//    {"src_left", engine_state_desc_type_bits, NULL, struct_offset(___mii_testbench_posedge_int_clock__ptr, src_left), {5,0,0,0}, {NULL,NULL,NULL,NULL} },
//    {"fc_credit", engine_state_desc_type_bits, NULL, struct_offset(___mii_testbench_posedge_int_clock__ptr, src_channel_state[0].fc_credit), {32,0,0,0}, {NULL,NULL,NULL,NULL} },
    {"", engine_state_desc_type_none, NULL, 0, {0,0,0,0}, {NULL,NULL,NULL,NULL} }
};

/*a Static ethernet packet functions
 */
/*f eth_packet_reset
 */
static void eth_packet_reset( t_eth_packet *packet )
{
    packet->length = 0;
    packet->fcs = 0xffffffff;
    packet->preclocked = 0;
}

/*f eth_packet_restart_preclock
 */
static void eth_packet_restart_preclock( t_eth_packet *packet )
{
    packet->preclocked = 1;
    packet->fcs = 0xffffffff;
    packet->next_nybble = -1;
}

/*f eth_packet_receive_preclock
 */
static void eth_packet_receive_preclock( t_eth_packet *packet, int nybble )
{
    packet->preclocked = 1;
    packet->next_nybble = nybble;
}

/*f eth_packet_done_preclock
 */
static void eth_packet_done_preclock( t_eth_packet *packet )
{
    packet->preclocked = 1;
    packet->next_nybble = -2;
}

/*f eth_packet_receive_fcs
  On the wire bit 0 of MII data goes first, so that is the first bit CRCd
 */
static void eth_packet_receive_fcs( t_eth_packet *packet, int nybble )
{
    int i;
    unsigned int next_fcs, fcs;
    int bit;
    fcs = packet->fcs;
    for (i=0; i<4; i++)
    {
        bit = (nybble>>i)&1;
        next_fcs = fcs<<1;
        if (((fcs&0x80000000)!=0) ^ (bit))
        {
            next_fcs ^= ( (1<< 0) |
                          (1<< 1) | 
                          (1<< 2) | 
                          (1<< 4) | 
                          (1<< 5) | 
                          (1<< 7) | 
                          (1<< 8) | 
                          (1<<10) | 
                          (1<<11) | 
                          (1<<12) | 
                          (1<<16) | 
                          (1<<22) | 
                          (1<<23) | 
                          (1<<26));
        }
        fcs = next_fcs;
    }
    packet->fcs  = fcs;
}

/*f eth_packet_receive_clock
 */
static void eth_packet_receive_clock( t_eth_packet *packet, int verbose )
{
    int i, j;
    packet->complete = 0;
    if (packet->preclocked)
    {
        packet->preclocked = 0;
        switch (packet->next_nybble)
        {
        case -2: // packet received
            if (verbose)
            {
                fprintf(stderr,"Packet received length %d bytes (%d nybbles) CRC %s (%08x)\n", packet->length/2, packet->length, (packet->fcs==0xc704dd7b)?"ok":"!WRONG!", packet->fcs);
                for (i=0; i<packet->length/2; i+=32)
                {
                    for (j=0; (j<32) && (j+i<packet->length/2); j++)
                    {
                        fprintf(stderr,"%02x ", packet->data[i+j]);
                    }
                    fprintf(stderr,"\n" );
                }
            }
            packet->complete = 1;
            break;
        case -1: // reset
            packet->length=0;
            break;
        default:
            if ((packet->length&1)==1)
            {
                if (packet->length/2 < MAX_PACKET_LENGTH)
                {
                    packet->data[packet->length/2] = (packet->next_nybble<<4) | (packet->last_nybble<<0); // On the wire bits 0-3 come before 4-7, so last_nybble is 0 to 3, new nybble (next) is 4 to 7
                }
            }
            eth_packet_receive_fcs( packet, packet->next_nybble ); // FCS the nybble as we go;
            packet->last_nybble = packet->next_nybble;
            packet->length++;
            break;
        }
    }
}

/*f eth_packet_transmit_byte
 */
static void eth_packet_transmit_byte( t_eth_packet *packet, unsigned int byte )
{
    if (packet->length/2 < MAX_PACKET_LENGTH)
    {
        packet->data[packet->length/2] = byte;
    }
    eth_packet_receive_fcs( packet, (byte>>0)&0xf );
    eth_packet_receive_fcs( packet, (byte>>4)&0xf );
    packet->length += 2;
}

/*f eth_packet_transmit_word
 */
static void eth_packet_transmit_word( t_eth_packet *packet, unsigned int word )
{
    eth_packet_transmit_byte( packet, (word>>24)&0xff );
    eth_packet_transmit_byte( packet, (word>>16)&0xff );
    eth_packet_transmit_byte( packet, (word>> 8)&0xff );
    eth_packet_transmit_byte( packet, (word>> 0)&0xff );
}

/*f eth_packet_transmit_add_fcs
 */
static void eth_packet_transmit_add_fcs( t_eth_packet *packet )
{
    int i;
    unsigned int fcs;

    fcs = 0;
    for (i=0; i<32; i++)
    {
        fcs |= (((~packet->fcs)>>i)&1)<<(31-i);
    }
    eth_packet_transmit_byte( packet, (fcs>>0)&0xff );
    eth_packet_transmit_byte( packet, (fcs>>8)&0xff );
    eth_packet_transmit_byte( packet, (fcs>>16)&0xff );
    eth_packet_transmit_byte( packet, (fcs>>24)&0xff );
}

/*a Static wrapper functions for mii_testbench
*/
/*f mii_testbench_instance_fn
*/
static t_sl_error_level mii_testbench_instance_fn( c_engine *engine, void *engine_handle )
{
    c_mii_testbench *mod;
    mod = new c_mii_testbench( engine, engine_handle );
    if (!mod)
        return error_level_fatal;
    return error_level_okay;
}

/*f mii_testbench_delete_fn - simple callback wrapper for the main method
*/
static t_sl_error_level mii_testbench_delete_fn( void *handle )
{
    c_mii_testbench *mod;
    t_sl_error_level result;
    mod = (c_mii_testbench *)handle;
    result = mod->delete_instance();
    delete( mod );
    return result;
}

/*f mii_testbench_reset_fn
*/
static t_sl_error_level mii_testbench_reset_fn( void *handle, int pass )
{
    c_mii_testbench *mod;
    mod = (c_mii_testbench *)handle;
    return mod->reset( pass );
}

/*f mii_testbench_combinatorial_fn
*/
static t_sl_error_level mii_testbench_combinatorial_fn( void *handle )
{
    c_mii_testbench *mod;
    mod = (c_mii_testbench *)handle;
    return mod->evaluate_combinatorials();
}

/*f mii_testbench_preclock_posedge_int_clock_fn
*/
static t_sl_error_level mii_testbench_preclock_posedge_int_clock_fn( void *handle )
{
    c_mii_testbench *mod;
    mod = (c_mii_testbench *)handle;
    return mod->preclock_posedge_int_clock();
}

/*f mii_testbench_clock_posedge_int_clock_fn
*/
static t_sl_error_level mii_testbench_clock_posedge_int_clock_fn( void *handle )
{
    c_mii_testbench *mod;
    mod = (c_mii_testbench *)handle;
    return mod->clock_posedge_int_clock();
}

/*a Constructors and destructors for mii_testbench
*/
/*f c_mii_testbench::c_mii_testbench
*/
c_mii_testbench::c_mii_testbench( class c_engine *eng, void *eng_handle )
{
    int i, j;
    char *option_string;
    char *string_copy, *sub_string_copy;
    char *argv[256], *sub_argv[256];
    int argc, sub_argc;
    engine = eng;
    engine_handle = eng_handle;

    /*b Create sources
     */
    for (i=0; i<MAX_SOURCES; i++)
    {
        sources[i].interval = 100;
        sources[i].delay = 0x7fffffff;
        sources[i].data_stream = NULL;
    }

    num_sources = 0;
    src_collision_list_length = 0;
    option_string = engine->get_option_string( engine_handle, "sources", "" );
    argc = sl_str_split( option_string, &string_copy, sizeof(argv), argv, 0, 0, 1 ); // Split into strings, with potential lists
    for (i=0; (i<argc) && (num_sources<MAX_SOURCES); i++)
    {
        if (argv[i][0]!='(') continue;
        sub_argc = sl_str_split( argv[i]+1, &sub_string_copy, sizeof(sub_argv), sub_argv, 0, 0, 1 ); // Split into strings, with potential lists
        switch (i)
        {
        case 0: // argv[0] is collision list
            for (j=0; (j<sub_argc) && (src_collision_list_length<MAX_SRC_COLLISION_LIST); j++)
            {
                if (sscanf( sub_argv[j], "%d", &src_collision_list[src_collision_list_length] )==1)
                {
                    src_collision_list_length++;
                }
            }
            break;
        case 1: // argv[1] is sources list
            for (j=0; (j<sub_argc) && (num_sources<MAX_SOURCES); j++)
            {
                //fprintf(stderr,"Source %s\n",sub_argv[j]);
                sources[num_sources].interval = 200;
                sources[num_sources].delay = 170;
                sources[num_sources].data_stream = sl_data_stream_create( sub_argv[j] );
                if (sources[num_sources].data_stream)
                {
                    unsigned int *args;
                    int k;
                    k = sl_data_stream_args(sources[num_sources].data_stream, &args);
                    if (k>0) sources[num_sources].interval = args[0];
                    if (k>1) sources[num_sources].delay = args[1];
                    num_sources++;
                }
            }
            break;
        }
    }
    free(string_copy);

    /*b Create sinks
     */
    num_sinks = 0;
    tgt_collision_list_length = 0;
    option_string = engine->get_option_string( engine_handle, "sinks", "" );
    argc = sl_str_split( option_string, &string_copy, sizeof(argv), argv, 0, 0, 1 ); // Split into strings, with potential lists
    for (i=0; (i<argc) && (num_sinks<MAX_SINKS); i++)
    {
        if (argv[i][0]!='(') continue;
        sub_argc = sl_str_split( argv[i]+1, &sub_string_copy, sizeof(sub_argv), sub_argv, 0, 0, 1 ); // Split into strings, with potential lists
        switch (i)
        {
        case 0: // argv[0] is collision list
            for (j=0; (j<sub_argc) && (tgt_collision_list_length<MAX_TGT_COLLISION_LIST); j++)
            {
                if (sscanf( sub_argv[j], "%d", &tgt_collision_list[tgt_collision_list_length] )==1)
                {
                    tgt_collision_list_length++;
                }
            }
            break;
        case 1: // argv[1] is sinks list
            for (j=0; (j<sub_argc) && (num_sinks<MAX_SINKS); j++)
            {
                sinks[num_sinks].data_stream = sl_data_stream_create( sub_argv[j] );
                if (sinks[num_sinks].data_stream)
                {
                    num_sinks++;
                }
            }
            break;
        }
    }
    free(string_copy);

    /*b Determine monitor options
     */
    monitor_level = engine->get_option_int( engine_handle, "monitor_level", 0 );

    /*b Instantiate module
     */
    engine->register_delete_function( engine_handle, (void *)this, mii_testbench_delete_fn );
    engine->register_reset_function( engine_handle, (void *)this, mii_testbench_reset_fn );

    engine->register_comb_fn( engine_handle, (void *)this, mii_testbench_combinatorial_fn );
    engine->register_clock_fns( engine_handle, (void *)this, "int_clock", mii_testbench_preclock_posedge_int_clock_fn, mii_testbench_clock_posedge_int_clock_fn );

    engine->register_input_signal( engine_handle, "int_reset", 1, (int **)&inputs.int_reset );

    /*b Add target signals if required
     */
    if (num_sinks>0)
    {
        engine->register_input_signal( engine_handle, "tx_mii_enable", 1, (int **)&inputs.tx_mii_enable );
        engine->register_input_used_on_clock( engine_handle, "tx_mii_enable", "int_clock", 1 );
        engine->register_input_signal( engine_handle, "tx_mii_data", 4, (int **)&inputs.tx_mii_data );
        engine->register_input_used_on_clock( engine_handle, "tx_mii_data", "int_clock", 1 );
        engine->register_output_signal( engine_handle, "tx_mii_col", 1, (int *)&combinatorials.tx_mii_col );
        engine->register_output_generated_on_clock( engine_handle, "tx_mii_col", "int_clock", 1 );
        engine->register_output_signal( engine_handle, "tx_mii_crs", 1, (int *)&combinatorials.tx_mii_crs );
        engine->register_output_generated_on_clock( engine_handle, "tx_mii_crs", "int_clock", 1 );
        combinatorials.tx_mii_col = 0;
        combinatorials.tx_mii_crs = 0;
    }

    /*b Add source signals if required
     */
    if (num_sources>0)
    {
        engine->register_output_signal( engine_handle, "rx_mii_dv", 1, (int *)&combinatorials.rx_mii_dv );
        engine->register_output_generated_on_clock( engine_handle, "rx_mii_dv", "int_clock", 1 );
        engine->register_output_signal( engine_handle, "rx_mii_data", 4, (int *)&combinatorials.rx_mii_data );
        engine->register_output_generated_on_clock( engine_handle, "rx_mii_data", "int_clock", 1 );
        engine->register_output_signal( engine_handle, "rx_mii_err", 1, (int *)&combinatorials.rx_mii_err );
        engine->register_output_generated_on_clock( engine_handle, "rx_mii_err", "int_clock", 1 );
        combinatorials.rx_mii_dv = 0;
        combinatorials.rx_mii_data = 0;
        combinatorials.rx_mii_err = 0;
    }

    /*b Add monitor signals if required
     */
    if (monitor_level>0)
    {
        engine->register_input_signal( engine_handle, "mon_mii_enable", 1, (int **)&inputs.mon_mii_enable );
        engine->register_input_used_on_clock( engine_handle, "mon_mii_enable", "int_clock", 1 );
        engine->register_input_signal( engine_handle, "mon_mii_data", 4, (int **)&inputs.mon_mii_data );
        engine->register_input_used_on_clock( engine_handle, "mon_mii_data", "int_clock", 1 );
        engine->register_input_signal( engine_handle, "mon_mii_crs", 1, (int **)&inputs.mon_mii_crs );
        engine->register_input_used_on_clock( engine_handle, "mon_mii_crs", "int_clock", 1 );
        engine->register_input_signal( engine_handle, "mon_mii_col", 1, (int **)&inputs.mon_mii_col );
        engine->register_input_used_on_clock( engine_handle, "mon_mii_col", "int_clock", 1 );
        engine->register_output_signal( engine_handle, "mon_failure", 1, (int *)&posedge_int_clock_state.mon_failure );
        engine->register_output_generated_on_clock( engine_handle, "mon_failure", "int_clock", 1 );
        posedge_int_clock_state.mon_failure = 0;
    }

    /*b Register state then reset
     */
    engine->register_state_desc( engine_handle, 1, state_desc_mii_testbench_posedge_int_clock, &posedge_int_clock_state, NULL );
    reset_active_high_int_reset();

    /*b Done
     */

}

/*f c_mii_testbench::~c_mii_testbench
*/
c_mii_testbench::~c_mii_testbench()
{
    delete_instance();
}

/*f c_mii_testbench::delete_instance
*/
t_sl_error_level c_mii_testbench::delete_instance( void )
{
    return error_level_okay;
}

/*a Class reset/preclock/clock methods for mii_testbench
*/
/*f c_mii_testbench::reset
*/
t_sl_error_level c_mii_testbench::reset( int pass )
{
    reset_active_high_int_reset();
    if (pass>0)
    {
        evaluate_combinatorials();
    }
    return error_level_okay;
}

/*f c_mii_testbench::reset_active_high_int_reset
*/
t_sl_error_level c_mii_testbench::reset_active_high_int_reset( void )
{
    int i;

    posedge_int_clock_state.mon_fsm = mon_fsm_idle;
    posedge_int_clock_state.mon_failure = 0;

    posedge_int_clock_state.src_fsm = src_fsm_idle;
    posedge_int_clock_state.src_data = 0;
    posedge_int_clock_state.src_left = 0;
    posedge_int_clock_state.src_collide = 0;
    posedge_int_clock_state.src_nybble_count = 0;
    posedge_int_clock_state.src_collision_list_entry = 0;
    posedge_int_clock_state.src_source = 0;

    posedge_int_clock_state.tgt_fsm = tgt_fsm_idle;
    posedge_int_clock_state.tgt_collide = 0;
    posedge_int_clock_state.tgt_nybble_count = 0;
    posedge_int_clock_state.tgt_collision_list_entry = 0;

    for (i=0; i<MAX_SOURCES; i++)
    {
        posedge_int_clock_state.src_source_state[i].counter = sources[i].interval - sources[i].delay;
        posedge_int_clock_state.src_source_state[i].pending = 0;
    }

    for (i=0; i<MAX_SINKS; i++)
    {
        posedge_int_clock_state.tgt_sink_state[i].counter = 0;
    }

    eth_packet_reset( &mon_packet );
    eth_packet_reset( &tgt_packet );
    eth_packet_reset( &src_packet );

    return error_level_okay;
}

/*f c_mii_testbench::evaluate_combinatorials
*/
t_sl_error_level c_mii_testbench::evaluate_combinatorials( void )
{
    if (inputs.int_reset[0]==1)
    {
        reset_active_high_int_reset();
    }

    /*b source combinatorials
     */
    if (num_sources>0)
    {
        switch (posedge_int_clock_state.src_fsm)
        {
        case src_fsm_idle:
            combinatorials.rx_mii_dv = 0;
            combinatorials.rx_mii_err = 0;
            combinatorials.rx_mii_data = 0;
            break;
        case src_fsm_preamble:
            combinatorials.rx_mii_dv = 1;
            combinatorials.rx_mii_err = 0;
            combinatorials.rx_mii_data = 0x5; // preamble
            break;
        case src_fsm_end_sfd:
            combinatorials.rx_mii_dv = 1;
            combinatorials.rx_mii_err = 0;
            combinatorials.rx_mii_data = 0xd; // sfd
            break;
        case src_fsm_data:
            combinatorials.rx_mii_dv = 1;
            combinatorials.rx_mii_err = 0;
            combinatorials.rx_mii_data = (src_packet.data[posedge_int_clock_state.src_nybble_count/2]>>((posedge_int_clock_state.src_nybble_count%2)*4))&0xf;
            break;
        case src_fsm_error:
            combinatorials.rx_mii_dv = 1;
            combinatorials.rx_mii_err = 1;
            combinatorials.rx_mii_data = 0xf;
            fprintf(stderr, "Handle arbitrary data during rx error\n");
            break;
        }
    }

    /*b target combinatorials
     */
    if ( num_sinks>0 )
    {
        combinatorials.tx_mii_col = posedge_int_clock_state.tgt_collide;
        combinatorials.tx_mii_crs = combinatorials.tx_mii_col; // CRS always extends as longs as COL
        switch (posedge_int_clock_state.tgt_fsm)
        {
        case tgt_fsm_idle:
            break;
        case tgt_fsm_preamble:
        case tgt_fsm_data:
            combinatorials.tx_mii_crs = 1;
            break;
        }
    }

    /*b Done
     */
    return error_level_okay;
}

/*f c_mii_testbench::preclock_posedge_int_clock
*/
t_sl_error_level c_mii_testbench::preclock_posedge_int_clock( void )
{
    int i;

    /*b Copy current state to next state
     */
    memcpy( &next_posedge_int_clock_state, &posedge_int_clock_state, sizeof(posedge_int_clock_state) );

    /*b mon_fsm
    */
    if (monitor_level>0)
    {
        switch (posedge_int_clock_state.mon_fsm)
        {
        case mon_fsm_idle:
            if ( (inputs.mon_mii_enable[0]) && (!inputs.mon_mii_col[0]) )
            {
                if (inputs.mon_mii_data[0]==0x5)
                {
                    next_posedge_int_clock_state.mon_fsm = mon_fsm_preamble;
                    next_posedge_int_clock_state.mon_count = 0;
                }
                FAIL( (inputs.mon_mii_data[0]!=0x5), "Bad data from idle, expected 5 for the preamble" );
            }
            break;
        case mon_fsm_preamble:
            if (inputs.mon_mii_col[0])
            {
                next_posedge_int_clock_state.mon_fsm = mon_fsm_collision;
                next_posedge_int_clock_state.mon_count = 0;
            }
            else if (inputs.mon_mii_enable[0])
            {
                next_posedge_int_clock_state.mon_count = posedge_int_clock_state.mon_count+1;
                if (inputs.mon_mii_data[0]==0xd)
                {
                    if (posedge_int_clock_state.mon_count==14)
                    {
                        eth_packet_restart_preclock( &mon_packet );
                        next_posedge_int_clock_state.mon_fsm = mon_fsm_data;
                        next_posedge_int_clock_state.mon_count = 0;
                    }
                    else
                    {
                        FAIL( 1, "Bad data in preamble, got SFD (0xd) at wrong point in the preamble; expected full tx of 14 preamble +1 SFD 0x5 nybbles" );
                        next_posedge_int_clock_state.mon_fsm = mon_fsm_idle;
                    }
                }
                else
                {
                    if ( (inputs.mon_mii_data[0]!=0x5) || 
                         (posedge_int_clock_state.mon_count==15) )
                    {
                        FAIL( 1, "Bad data in preamble, either the SFD didn't occur at nybble 15 or the preamble data was not 0x5" );
                        next_posedge_int_clock_state.mon_fsm = mon_fsm_idle;
                    }
                }
            }
            else
            {
                FAIL( 1, "Unexpected EOP during preamble" );
                next_posedge_int_clock_state.mon_fsm = mon_fsm_idle;
            }
            break;
        case mon_fsm_data:
            if (inputs.mon_mii_col[0])
            {
                next_posedge_int_clock_state.mon_fsm = mon_fsm_collision;
                next_posedge_int_clock_state.mon_count = 0;
            }
            else if (inputs.mon_mii_enable[0])
            {
                eth_packet_receive_preclock( &mon_packet, inputs.mon_mii_data[0]&0xf );
                next_posedge_int_clock_state.mon_count = posedge_int_clock_state.mon_count+1;
            }
            else
            {
                eth_packet_done_preclock( &mon_packet );
                next_posedge_int_clock_state.mon_fsm = mon_fsm_idle;
            }
            break;
        case mon_fsm_collision:
            if (inputs.mon_mii_enable[0])
            {
                next_posedge_int_clock_state.mon_count = posedge_int_clock_state.mon_count+1;
                if (posedge_int_clock_state.mon_count>4)
                {
                    FAIL( (inputs.mon_mii_data[0]!=9), "Expected jamming using data of 0x9 in collision" );
                }
            }
            else
            {
                if (posedge_int_clock_state.mon_count<8)
                {
                    FAIL( 1, "Expected more jamming in collision" );
                }
                next_posedge_int_clock_state.mon_fsm = mon_fsm_idle;
            }
            break;
        }
    }

    /*b tgt_fsm
    */
    if (num_sinks>0)
    {
        next_posedge_int_clock_state.tgt_nybble_count = posedge_int_clock_state.tgt_nybble_count+1;
        if (posedge_int_clock_state.tgt_collision_list_entry<tgt_collision_list_length)
        {
            if ( (tgt_collision_list[posedge_int_clock_state.tgt_collision_list_entry]!=0) &&
                 (tgt_collision_list[posedge_int_clock_state.tgt_collision_list_entry]<posedge_int_clock_state.tgt_nybble_count) )
            {
                next_posedge_int_clock_state.tgt_collide = 1;
            }
        }
        switch (posedge_int_clock_state.tgt_fsm)
        {
        case tgt_fsm_idle:
            next_posedge_int_clock_state.tgt_collide = 0;
            next_posedge_int_clock_state.tgt_nybble_count = 0;
            if (inputs.tx_mii_enable[0])
            {
                if (inputs.tx_mii_data[0]==0x5)
                {
                    next_posedge_int_clock_state.tgt_fsm = tgt_fsm_preamble;
                }
            }
            break;
        case tgt_fsm_preamble:
            if (inputs.tx_mii_enable[0])
            {
                switch (inputs.tx_mii_data[0])
                {
                case 0xd:
                    eth_packet_restart_preclock( &tgt_packet );
                    next_posedge_int_clock_state.tgt_fsm = tgt_fsm_data;
                    break;
                case 0x5:
                    next_posedge_int_clock_state.tgt_fsm = tgt_fsm_preamble;
                    break;
                default:
                    next_posedge_int_clock_state.tgt_fsm = tgt_fsm_idle;
                    break;
                }
            }
            else
            {
                next_posedge_int_clock_state.tgt_fsm = tgt_fsm_idle;
            }
            break;
        case tgt_fsm_data:
            if (inputs.tx_mii_enable[0])
            {
                eth_packet_receive_preclock( &tgt_packet, inputs.tx_mii_data[0]&0xf );
            }
            else
            {
                if (!posedge_int_clock_state.tgt_collide)
                    eth_packet_done_preclock( &tgt_packet );
                next_posedge_int_clock_state.tgt_fsm = tgt_fsm_idle;
            }
            break;
        }
        if ( (posedge_int_clock_state.tgt_fsm != tgt_fsm_idle) &&
             (next_posedge_int_clock_state.tgt_fsm == tgt_fsm_idle) )
        {
            if (posedge_int_clock_state.tgt_collision_list_entry<tgt_collision_list_length)
            {
                next_posedge_int_clock_state.tgt_collision_list_entry = posedge_int_clock_state.tgt_collision_list_entry+1;
                if ( (next_posedge_int_clock_state.tgt_collision_list_entry<tgt_collision_list_length) &&
                     (tgt_collision_list[next_posedge_int_clock_state.tgt_collision_list_entry]==-1) )
                {
                    next_posedge_int_clock_state.tgt_collision_list_entry = 0;
                }
            }

        }
    }

    /*b src_fsm
    */
    if (num_sources>0)
    {
        for (i=0; i<num_sources; i++)
        {
            next_posedge_int_clock_state.src_source_state[i].counter = posedge_int_clock_state.src_source_state[i].counter+1;
//            fprintf(stderr,"Checking source %d.%d.%d\n",i,posedge_int_clock_state.src_source_state[i].counter,sources[i].interval );
            if ( (!posedge_int_clock_state.src_source_state[i].pending) && (posedge_int_clock_state.src_source_state[i].counter>=sources[i].interval) )
            {
                next_posedge_int_clock_state.src_source_state[i].pending = 1;
                next_posedge_int_clock_state.src_source_state[i].counter = 0;
            }
        }
        switch (posedge_int_clock_state.src_fsm)
        {
        case src_fsm_idle:
            for (i=0; i<num_sources; i++)
            {
                if (posedge_int_clock_state.src_source_state[i].pending)
                {
                    break;
                }
            }
            if (i<num_sources)
            {
                int length, data, j;
                next_posedge_int_clock_state.src_source_state[i].pending = 0;
                next_posedge_int_clock_state.src_source = i;
                next_posedge_int_clock_state.src_fsm = src_fsm_preamble;
                next_posedge_int_clock_state.src_nybble_count = 0;
                next_posedge_int_clock_state.src_collide = 0;
                eth_packet_reset( &src_packet );
                sl_data_stream_start_packet( sources[i].data_stream );
                length = sl_data_stream_packet_length( sources[i].data_stream );
                data = sl_data_stream_packet_header( sources[i].data_stream );
                eth_packet_transmit_word( &src_packet, data );
                for (j=0; j<length-8; j++)
                {
                    sl_data_stream_next_data_byte( sources[i].data_stream );
                    data = sl_data_stream_packet_data_byte( sources[i].data_stream );
                    eth_packet_transmit_byte( &src_packet, data );
                }
                eth_packet_transmit_add_fcs( &src_packet );
                fprintf(stderr,"Handle collisions in src, provide for arbitrary preamble length\n");
            }
            break;
        case src_fsm_preamble:
            next_posedge_int_clock_state.src_nybble_count = posedge_int_clock_state.src_nybble_count+1;
            if (posedge_int_clock_state.src_nybble_count==14)
            {
                next_posedge_int_clock_state.src_fsm = src_fsm_end_sfd;
            }
            break;
        case src_fsm_end_sfd:
            next_posedge_int_clock_state.src_nybble_count = 0;
            next_posedge_int_clock_state.src_fsm = src_fsm_data;
            break;
        case src_fsm_data:
            next_posedge_int_clock_state.src_nybble_count = posedge_int_clock_state.src_nybble_count+1;
            if (next_posedge_int_clock_state.src_nybble_count>=src_packet.length)
            {
                next_posedge_int_clock_state.src_fsm = src_fsm_idle;
            }
            break;
        case src_fsm_error:
            if (posedge_int_clock_state.src_nybble_count==16)
            {
                fprintf(stderr,"Handle variable length of error\n");
            }
            break;
        }
    }

    /*b Done
     */
    return error_level_okay;
}

/*f c_mii_testbench::clock_posedge_int_clock
*/
t_sl_error_level c_mii_testbench::clock_posedge_int_clock( void )
{
    int i, data;
    char buffer[256];

    /*b Copy next state to current
    */
    memcpy( &posedge_int_clock_state, &next_posedge_int_clock_state, sizeof(posedge_int_clock_state) );

    /*b Monitor the bus with a message if required
    */
    if (monitor_level&mon_level_verbose)
    {
        sprintf( buffer, "E/Data %d/%x CRS %d COL %d", inputs.mon_mii_enable[0], inputs.mon_mii_data[0], inputs.mon_mii_crs[0], inputs.mon_mii_col[0] );
        engine->message->add_error( NULL, error_level_info, error_number_se_dated_message, error_id_sl_exec_file_allocate_and_read_exec_file,
                                    error_arg_type_integer, engine->cycle(),
                                    error_arg_type_malloc_string, engine->get_instance_name(engine_handle), 
                                    error_arg_type_malloc_string, buffer,
                                    error_arg_type_none );
    }
    if (monitor_level>0)
    {
        eth_packet_receive_clock( &mon_packet, monitor_level>0 );
    }

    /*b Manage the target
     */
    if (num_sinks>0)
    {
        eth_packet_receive_clock( &tgt_packet, 1 );
        if (tgt_packet.complete)
        {
            sl_data_stream_start_packet( sinks[0].data_stream );
            if (tgt_packet.length!=8+2*sl_data_stream_packet_length( sinks[0].data_stream ))
            {
                sprintf( buffer, "c_mii_testbench:Expected TX ethernet packet length %d", sl_data_stream_packet_length( sinks[0].data_stream ) );
                engine->error->add_error( NULL, error_level_info, error_number_sl_fail, error_id_sl_exec_file_allocate_and_read_exec_file,
                                          error_arg_type_integer, engine->cycle(),
                                          error_arg_type_malloc_string, buffer,
                                          error_arg_type_malloc_string, engine->get_instance_name(engine_handle), 
                                          error_arg_type_none );
            }
            data = sl_data_stream_packet_header( sinks[0].data_stream );
            for (i=0; i<tgt_packet.length/2-8; i++)
            {
                sl_data_stream_next_data_byte( sinks[0].data_stream );
                data = sl_data_stream_packet_data_byte( sinks[0].data_stream );
                if ( data != tgt_packet.data[i+4] )
                {
                    sprintf( buffer, "c_mii_testbench:Expected TX ethernet data %02x, got data %02x at octet %d of packet", data, tgt_packet.data[i+4], i );
                    engine->error->add_error( NULL, error_level_info, error_number_sl_fail, error_id_sl_exec_file_allocate_and_read_exec_file,
                                              error_arg_type_integer, engine->cycle(),
                                              error_arg_type_malloc_string, buffer,
                                              error_arg_type_malloc_string, engine->get_instance_name(engine_handle), 
                                              error_arg_type_none );
                }
            }
        }
    }

    /*b Done
     */
    return evaluate_combinatorials();
}

/*a Initialization functions
*/
/*f c_mii_testbench__init
*/
extern void c_mii_testbench__init( void )
{
    se_external_module_register( 1, "mii_testbench", mii_testbench_instance_fn );
}

/*a Scripting support code
*/
/*f initmii_testbench
*/
extern "C" void initmii_testbench( void )
{
    c_mii_testbench__init( );
    scripting_init_module( "mii_testbench" );
}
