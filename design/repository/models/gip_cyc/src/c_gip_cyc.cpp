/*a To do
 */

/*a Defines
 */
#if 0
#define ENTRY() {fprintf (stderr, "Entry of %s at %s:%d\n", __func__, __FILE__, __LINE__);}
#else
#define ENTRY()
#endif

/*a Includes
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>

#include "arm_dis.h"
#include "gdb_stub.h"
#include "symbols.h"
#include "c_gip_full.h"
#include "c_memory_model.h"
#include "c_ram_model.h"
#include "c_mmio_model.h"
#include "console.h"
#include "ether.h"

#include "be_model_includes.h"

/*a Types
 */
/*t t_file_desc
 */
typedef struct t_file_desc
{
    char *name;
	FILE * fp;
	unsigned int addr;
	int binary;
};

/*t t_gip_cyc_inputs
*/
typedef struct t_gip_cyc_inputs
{
    unsigned int *postbus_rx_data_ptr;
    t_postbus_type *postbus_rx_type_ptr;
    t_postbus_ack *postbus_tx_ack_ptr;
} t_gip_cyc_inputs;

/*t t_gip_cyc_combinatorials
*/
typedef struct t_gip_cyc_combinatorials
{
    t_gip_comb_data gip;
} t_gip_cyc_combinatorials;

/*t t_gip_cyc_nets
*/
typedef struct t_gip_cyc_nets
{
} t_gip_cyc_nets;

/*t t_gip_cyc_coverage
*/
typedef struct t_gip_cyc_coverage
{
    unsigned char map[1];
} t_gip_cyc_coverage;

/*t c_gip_cyc
 */
class c_gip_cyc
{
public:
    c_gip_cyc::c_gip_cyc( class c_engine *eng, void *eng_handle );
    c_gip_cyc::~c_gip_cyc();
    t_sl_error_level c_gip_cyc::delete_instance( void );
    t_sl_error_level c_gip_cyc::reset( void );
    t_sl_error_level c_gip_cyc::preclock( void );
    t_sl_error_level c_gip_cyc::clock( void );
    t_sl_error_level c_gip_cyc::evaluate_combinatorials( void );
private:
    c_engine *engine;
    void *engine_handle;
    t_gip_cyc_inputs inputs;
    t_gip_cyc_combinatorials combinatorials;
    t_gip_cyc_nets nets;
    t_gip_cyc_coverage coverage;

    int gdb_enabled;
    unsigned int regs[32];
    char *symbol_map;
    unsigned int memory_regions[8][3];
    unsigned int execution_regions[8][3];
    t_file_desc files_to_load [8];
    int nfiles_to_load;

    int debug_level;
    c_execution_model_class *gip;
    c_memory_model *memory;
    c_ram_model *ram;
    c_ram_model *zero_page_ram;
    c_mmio_model *mmio;
};

/*a Support functions
 */
/*f memory_model_debug
 */
static void memory_model_debug( void *handle, t_memory_model_debug_action action, int write_not_read, unsigned int address, unsigned int data, int bytes )
{
    c_execution_model_class *gip;
    gip = (c_execution_model_class *)handle;

    printf("Wrapper:Memory model debug action %d wnr %d address %08x data %08x bytes %d\n", action, write_not_read, address, data, bytes );
//    gip->debug(-1);
//    gip->halt_cpu();
//    gdb_trap(5);
}

/*f usage
 */
static void usage( void )
{
    printf("This is a wrapper around various GIP and memory models\n");
    printf("It is very powerful, and probably exceeds the scope of options to manage it\n");
    printf("However, primitive options are supplied\n");
    printf("-- -a -b -d -e -f -g -h -m -r -s -S -t -M -T\n");
    printf("\t--: use stdin for the code to load into the GIP\n");
    printf("\t-a <hex>: set base address of code to load; precede '-f' or '--'\n");
    printf("\t-b: load code as binary, not as a MIF file (which is hex data, or address:hex, in text form); precede '-f' or '--'\n");
    printf("\t-d <n>: set debug level and enable debugging\n");
    printf("\t-e <eth>: connect to <eth> as the device (john default: eth3)\n");
    printf("\t-f <file>: load code from the specified file\n");
    printf("\t-g: run as a process that requires a gdb to attach to it for debugging; don't execute other than through gdb\n");
    printf("\t-h: display this help message\n");
    printf("\t-m: launch minicom\n");
    printf("\t-r<n> <hex>: set register n (decimal) to the hex value given at initialization\n");
    printf("\t-s <symbol.map>: set the filename of the symbol file - usually 'System.map' if run from the linux directory\n");
    printf("\t-S: NO LONGER WORKS - use the single cycle GIP internal model - this runs faster, but less accurately than the default full pipeline model\n");
    printf("\t-t <cycles>: if not running under gdb, run for this maximum number of cycles then exit\n");
    printf("\t-M <start> <end> <level>: turn on memory tracing for a region of memory at this level (1-3, 3 gives everything, 1 just errors); output is appended to file memory_model.log\n");
    printf("\t-T <region> <start> <end>: turn on execution tracing within a numbered region (0 to 7); output is appended to file gip_exec_trace.log\n");
}

/*f get_ticks
 */
int get_ticks (void)
{
	struct timeval tv;
	gettimeofday (&tv, 0);
	return (tv.tv_sec * 100) + (tv.tv_usec / 10000);
}

/*a Static wrapper functions for gip_cyc
*/
/*f gip_cyc_instance_fn
*/
static t_sl_error_level gip_cyc_instance_fn( c_engine *engine, void *engine_handle )
{
    c_gip_cyc *mod;

    mod = new c_gip_cyc( engine, engine_handle );
    if (!mod)
        return error_level_fatal;

    return error_level_okay;
}

/*f gip_cyc_delete_fn - simple callback wrapper for the main method
*/
static t_sl_error_level gip_cyc_delete_fn( void *handle )
{
    c_gip_cyc *mod;
    t_sl_error_level result;
    mod = (c_gip_cyc *)handle;
    result = mod->delete_instance();
    delete( mod );
    return result;
}

/*f gip_cyc_reset_fn
*/
static t_sl_error_level gip_cyc_reset_fn( void *handle )
{
    c_gip_cyc *mod;
    mod = (c_gip_cyc *)handle;
    return mod->reset();
}

/*f gip_cyc_preclock_fn
*/
static t_sl_error_level gip_cyc_preclock_fn( void *handle )
{
    c_gip_cyc *mod;
    mod = (c_gip_cyc *)handle;
    return mod->preclock();
}

/*f gip_cyc_clock_fn
*/
static t_sl_error_level gip_cyc_clock_fn( void *handle )
{
    c_gip_cyc *mod;
    mod = (c_gip_cyc *)handle;
    return mod->clock();
}

/*a Constructors and destructors for gip_cyc
*/
/*f c_gip_cyc::c_gip_cyc
*/
c_gip_cyc::c_gip_cyc( class c_engine *eng, void *eng_handle )
{
	int i, r;
    int argc;
    char *args_string;
    char *argv[32];

    engine = eng;
    engine_handle = eng_handle;

    /*b Register simulation functions
     */
    engine->register_delete_function( engine_handle, (void *)this, gip_cyc_delete_fn );
    engine->register_reset_function( engine_handle, (void *)this, gip_cyc_reset_fn );
    engine->register_clock_fns( engine_handle, (void *)this, "cpu_clock", gip_cyc_preclock_fn, gip_cyc_clock_fn );
//     void c_engine::register_comb_fn( void *engine_handle, void *handle, t_engine_callback_fn comb_fn );

    engine->register_output_signal( engine_handle, "postbus_tx_data", 32, (int *)&combinatorials.gip.postbus_tx_data );
    engine->register_output_generated_on_clock( engine_handle, "postbus_tx_data", "cpu_clock", 1 );
    engine->register_output_signal( engine_handle, "postbus_tx_type", 2, (int *)&combinatorials.gip.postbus_tx_type );
    engine->register_output_generated_on_clock( engine_handle, "postbus_tx_type", "cpu_clock", 1 );
    engine->register_output_signal( engine_handle, "postbus_rx_ack", 2, (int *)&combinatorials.gip.postbus_rx_ack );
    engine->register_output_generated_on_clock( engine_handle, "postbus_rx_ack", "cpu_clock", 1 );

    engine->register_input_signal( engine_handle, "postbus_rx_data", 32, (int **)&inputs.postbus_rx_data_ptr );
    engine->register_input_used_on_clock( engine_handle, "postbus_rx_data", "cpu_clock", 1 );
    engine->register_input_signal( engine_handle, "postbus_rx_type", 2, (int **)&inputs.postbus_rx_type_ptr );
    engine->register_input_used_on_clock( engine_handle, "postbus_rx_type", "cpu_clock", 1 );
    engine->register_input_signal( engine_handle, "postbus_tx_ack", 2, (int **)&inputs.postbus_tx_ack_ptr );
    engine->register_input_used_on_clock( engine_handle, "postbus_tx_ack", "cpu_clock", 1 );

    combinatorials.gip.postbus_tx_type = postbus_word_type_idle;
    combinatorials.gip.postbus_tx_data = 0;
    combinatorials.gip.postbus_rx_ack = postbus_ack_taken;

//     void c_engine::register_comb_input( void *engine_handle, const char *name );
//     void c_engine::register_comb_output( void *engine_handle, const char *name );
//     void c_engine::register_state_desc( void *engine_handle, int static_desc, t_engine_state_desc *state_desc, void *data, const char *prefix );

    /*b Copy options and split for argc/argv
     */
    argv[0] = engine->get_option_string( engine_handle, "options", "" );
    args_string = sl_str_alloc_copy(argv[0] );
    i=0; r=0; argc=1;
    while (args_string[i])
    {
        if (args_string[i]==' ')
        {
            if (r)
            {
                args_string[i]=0;
                r = 0;
            }
            i++;
        }
        else if (args_string[i]!=0)
        {
            if (!r)
            {
                argv[argc] = args_string+i;
                r = 1;
                argc++;
                if (argc>=32) argc=31;
            }
            i++;
        }
    }

    /*b Set default values
     */
    nfiles_to_load = 0;
    memset (files_to_load, 0, sizeof(files_to_load));
    for (i=0; i<8; i++)
    {
        memory_regions[i][0] = 0;
        memory_regions[i][1] = 0;
        memory_regions[i][2] = 100;
        execution_regions[i][0] = 0;
        execution_regions[i][1] = 0;
        execution_regions[i][2] = 0;
    }
	gdb_enabled = 0;
    symbol_map = "../../os/linux/System.map";
    for (i=0; i<32; i++)
    {
        regs[i] = 0xdeadcafe;
    }
    

    /*b Parse arguments
     */
	for (i=1; i<argc; i++)
	{
        /*b '--'
         */
        if (!strcmp( argv[i], "--" ))
        {
            files_to_load[nfiles_to_load++].fp = stdin;
            continue;
        }
        /*b '-a'
         */
        if (!strcmp( argv[i], "-a" ))
        {
            i++;
            if (i<argc)
            {
                int base;
                if (sscanf( argv[i], "%x", &base)!=1)
                {
                    fprintf( stderr, "Failed to parse '%s' for base address of load\n", argv[i] );
                }
                files_to_load[nfiles_to_load].addr = base;
            }
            continue;
        }
        /*b '-b'
         */
        if (!strcmp( argv[i], "-b" ))
        {
            files_to_load[nfiles_to_load].binary=1;
            continue;
        }
        /*b '-d'
         */
        if (!strcmp( argv[i], "-d" ))
        {
            i++;
            if (i<argc)
            {
                if (sscanf( argv[i], "%d", &debug_level)!=1)
                {
                    fprintf( stderr, "Failed to parse '%s' for debug mask\n", argv[i] );
                }
            }
            continue;
        }
        /*b '-f'
         */
        if (!strcmp( argv[i], "-f" ))
        {
            i++;
            if (i<argc)
            {
                FILE * fp = fopen(argv[i],"r");
                if (!fp)
                {
                    fprintf( stderr, "Failed to open file '%s'\n", argv[i] );
                }
                else
                {
                    files_to_load[nfiles_to_load].name = argv[i];
                    files_to_load[nfiles_to_load++].fp = fp;
                }
            }
            continue;
        }
        /*b '-g'
         */
        if (!strncmp (argv[i], "-g", 2))
        {
            gdb_enabled = 1;
            continue;
        }
        /*b '-h'
         */
        if (!strncmp (argv[i], "-h", 2))
        {
            usage();
            exit(0);
        }
        /*b '-r'
         */
        if (!strncmp( argv[i], "-r", 2 ))
        {
            i++;
            if (i<argc)
            {
                if (sscanf( argv[i-1], "-r%d", &r)!=1)
                {
                    fprintf( stderr, "Failed to parse '%s' for register initialization\n", argv[i] );
                }
                regs[r&0xf] = strtoul(argv[i], NULL, 0);
            }
            continue;
        }
        /*b '-s'
         */
        if (!strncmp( argv[i], "-s", 2 ))
        {
            i++;
            if (i<argc)
            {
                symbol_map = argv[i];
            }
            continue;
        }
        /*b '-M'
         */
        if (!strcmp( argv[i], "-M" ))
        {
            i+=3;
            if (i<argc)
            {
                int j;
                for (j=0; j<8; j++)
                {
                    if (memory_regions[j][2]>4)
                    {
                        break;
                    }
                }
                if (j<8)
                {
                    if ( (sscanf( argv[i-2], "%08x", &memory_regions[j][0])!=1) ||
                         (sscanf( argv[i-1], "%08x", &memory_regions[j][1])!=1) ||
                         (sscanf( argv[i], "%d", &memory_regions[j][2])!=1) )
                    {
                        fprintf( stderr, "Failed to parse arguments (hex start, hex end, int level) for memory tracing\n" );
                    }
                }
            }
            continue;
        }
        /*b '-T'
         */
        if (!strcmp( argv[i], "-T" ))
        {
            i+=3;
            if (i<argc)
            {
                int j;
                if ((sscanf( argv[i-2], "%d", &j )==1) && (j>=0) && (j<8))
                {
                    if ( (sscanf( argv[i-1], "%08x", &execution_regions[j][0])!=1) ||
                         (sscanf( argv[i], "%08x", &execution_regions[j][1])!=1) )
                    {
                        fprintf( stderr, "Failed to parse arguments (region, hex start, hex end) for exec tracing\n" );
                    }
                    else
                    {
                        execution_regions[j][2] = 1;
                    }
                }
            }
            continue;
        }
        /*b All done
         */
	}

    /*b Build system
     */
   	ram = new c_ram_model( 1<<24 ); // Create RAM of size 1<<24 = 16MB
   	memory = new c_memory_model();
    c_gip_full *gip_full;
    gip_full = new c_gip_full( memory );
    gip = (c_execution_model_class *)gip_full;

   	zero_page_ram = new c_ram_model( 1<<24 ); // Create zero-pagee RAM of size 1<<24 = 16MB
   	mmio = new c_mmio_model( gip ); // Create our standard MMIO model

    zero_page_ram->register_with_memory_map( memory, 0x00000000, 0x10000000 ); // Register zero-page RAM at 0x00000000 to 0xd0000000
    ram->register_with_memory_map( memory, 0xc0000000, 0x10000000 ); // Register RAM at 0xc0000000 to 0xd0000000
    mmio->register_with_memory_map( memory, 0xff000000, 0x00ffffff ); // Register MMIO at 0xff000000 to 0xfffffff
    memory->register_debug_handler( (void *)gip, memory_model_debug );

    /*b Load code and symbols
     */
    for (i = 0; i < nfiles_to_load; i++)
    {
        if (files_to_load[i].binary)
        {
            fprintf( stderr, "Loading binary file '%s' at %08x\n", files_to_load[i].name, files_to_load[i].addr);
            gip->load_code_binary( files_to_load[i].fp, files_to_load[i].addr );
        }
        else
        {
            fprintf( stderr, "Loading text file '%s' at %08x\n", files_to_load[i].name, files_to_load[i].addr);
            gip->load_code( files_to_load[i].fp, files_to_load[i].addr );
        }
    }
    for (i=0; i<16; i++)
    {
        gip->set_register( i, regs[i] );
    }
    symbol_initialize( symbol_map );

    /*b Enable memory logging and tracing of GIP
     */
//    memory->set_log_file( "memory_model.log" );
    for (i=0; i<8; i++)
    {
        if (memory_regions[i][2]<4)
        {
            memory->set_log_level( memory_regions[i][0], memory_regions[i][1]-memory_regions[i][0], memory_regions[i][2] );
        }
    }

    gip->trace_set_file( "gip_exec_trace.log" );
    for (i=0; i<8; i++)
    {
        if (execution_regions[i][2])
        {
            gip->trace_region( i, execution_regions[i][0], execution_regions[i][1] ); // Trace execution in main ram
        }
    }

    /*b Initialize GDB server if required
     */
    if (gdb_enabled)
    {
	    printf ("Starting GDB interface...\n");
        //gdb_stub_init( gip, memory, mmio );
    }
    else
    {
        //gdb_stub_disable();
    }
    

    /*b Register coverage
     */
    se_cmodel_assist_coverage_register( engine, engine_handle, coverage );

}

/*f c_gip_cyc::~c_gip_cyc
*/
c_gip_cyc::~c_gip_cyc()
{
    delete_instance();
}

/*f c_gip_cyc::delete_instance
*/
t_sl_error_level c_gip_cyc::delete_instance( void )
{
    return error_level_okay;
}

/*a Class reset/preclock/clock methods for gip_cyc
*/
/*f c_gip_cyc::reset
*/
t_sl_error_level c_gip_cyc::reset( void )
{
    ENTRY();
    gip->reset();
    evaluate_combinatorials();
    return error_level_okay;
}

/*f c_gip_cyc::preclock
*/
t_sl_error_level c_gip_cyc::preclock( void )
{
    ENTRY();
    evaluate_combinatorials();
    gip->preclock();
    return error_level_okay;
}

/*f c_gip_cyc::clock
*/
t_sl_error_level c_gip_cyc::clock( void )
{
    ENTRY();
    gip->clock();
    evaluate_combinatorials();
    return error_level_okay;
}

/*f c_gip_cyc::evaluate_combinatorials
*/
t_sl_error_level c_gip_cyc::evaluate_combinatorials( void )
{
    c_gip_full *gip_full;
    ENTRY();

    /*b Get inputs
     */
    gip_full = (c_gip_full *)gip;
    gip_full->inputs.postbus_rx_data = inputs.postbus_rx_data_ptr[0];
    gip_full->inputs.postbus_rx_type = inputs.postbus_rx_type_ptr[0];
    gip_full->inputs.postbus_tx_ack = inputs.postbus_tx_ack_ptr[0];

    /*b Call GIP
    */
    gip->comb( (void *)&combinatorials.gip );

    return error_level_okay;
}

/*a Initialization functions
*/
/*f c_gip_cyc__init
*/
extern void c_gip_cyc__init( void )
{
    se_external_module_register( 1, "gip_cyc", gip_cyc_instance_fn );
}

/*a Scripting support code
*/
/*f initgip_cyc
*/
extern "C" void initgip_cyc( void )
{
    c_gip_cyc__init( );
    scripting_init_module( "gip_cyc" );
}
