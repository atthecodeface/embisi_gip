/*a Copyright Gavin J Stark and John Croft, 2003
 */

/*a Includes
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>

#include "arm_dis.h"
#include "gdb_stub.h"
#include "symbols.h"
#include "c_arm_model.h"
#include "c_memory_model.h"
#include "c_ram_model.h"
#include "c_mmio_model.h"
#include "console.h"
#include "ether.h"

/*a Statics
 */
int debug_level;
static c_arm_model *arm;
static c_memory_model *memory;
static c_ram_model *ram;
static c_ram_model *zero_page_ram;
static c_mmio_model *mmio;
// static int time_to_next_irq = 100000;

static int tty_fd;

/*
void swi_hack (void)
{
	time_to_next_irq += 1000;
}
*/

/*a Main routines
 */
/*f memory_model_debug
 */
static void memory_model_debug( void *handle, t_memory_model_debug_action action, int write_not_read, unsigned int address, unsigned int data, int bytes )
{
    printf("Wrapper:Memory model debug action %d wnr %d address %08x data %08x bytes %d\n", action, write_not_read, address, data, bytes );
    arm->debug(-1);
    arm->halt_cpu();
    gdb_trap(5);
}

static void handle_user2 (int signal)
{
	extern unsigned long write_trap;
	write_trap = 0xf29ed0;
	printf ("write-trap established\n");
    fprintf(stderr, "sigusr2\n");
}

int sigusr1 = 0;
static void handle_user1 (int signal)
{
    fprintf(stderr, "sigusr1\n");
	sigusr1 = 1;
}

/*f usage
 */
static void usage( void )
{
    printf("This is a wrapper around various ARM and memory models\n");
    printf("It is very powerful, and probably exceeds the scope of options to manage it\n");
    printf("However, primitive options are supplied\n");
    printf("-- -a -b -d -e -f -g -h -m -r -s -t -M -T\n");
    printf("\t--: use stdin for the code to load into the ARM\n");
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
    printf("\t-t <cycles>: if not running under gdb, run for this maximum number of cycles then exit\n");
    printf("\t-M <start> <end> <level>: turn on memory tracing for a region of memory at this level (1-3, 3 gives everything, 1 just errors); output is appended to file memory_model.log\n");
    printf("\t-T <region> <start> <end>: turn on execution tracing within a numbered region (0 to 7); output is appended to file arm_exec_trace.log\n");
}

#define TTY_BUF_SIZE 8
static char tty_in_buf [TTY_BUF_SIZE];
static int tty_in_read, tty_in_write;

/*f tty_in
 */
char tty_in (void)
{
	if (tty_in_read == tty_in_write)
	{
		printf ("Attempt to read from tty buffer when it's empty");
		return 0;
	}
	else
	{
		char res = tty_in_buf[tty_in_read];
		tty_in_read = (tty_in_read + 1) % TTY_BUF_SIZE;
		return res;
	}
}

/*f keyboard_poll
 */
int keyboard_poll (void)
{
	fd_set fd;
	FD_ZERO(&fd);
	FD_SET(tty_fd,&fd);
	struct timeval t;
	t.tv_sec = 0;
	t.tv_usec = 0;
	int n = select (tty_fd+1,&fd,0,0,&t);
	if (n == 0) return (tty_in_read != tty_in_write);
	
	char c;
	read (tty_fd, &c, 1);
//	printf ("%2.2x\n", c);
	int next = (tty_in_write + 1) % TTY_BUF_SIZE;
	if (next == tty_in_read)
	{
		printf ("Keyboard buffer full\n");
		return 1;
	}
	tty_in_buf [tty_in_write] = c;
	tty_in_write = next;
	return 1;
}

/*f tty_out
 */
void tty_out (char c)
{
//	printf ("***TTY***put %2.2x (%c)\n", c, c);
	write (tty_fd, &c, 1);
}

struct file_desc
{
    char *name;
	FILE * fp;
	unsigned int addr;
	int binary;
};

int get_ticks (void)
{
	struct timeval tv;
	gettimeofday (&tv, 0);
	return (tv.tv_sec * 100) + (tv.tv_usec / 10000);
}

/*f main
 */
extern int main( int argc, char **argv )
{
	int i, okay, gdb_enabled;
	int cycles;
//	FILE *f;
//	int binary;
    int r;
    unsigned int regs[16];
//    unsigned int base_address;
    char *symbol_map;
    char *eth_device;
    unsigned int memory_regions[8][3];
    unsigned int execution_regions[8][3];
    file_desc files_to_load [8];
    int nfiles_to_load = 0;
    int must_launch_minicom = 0;

    memset (files_to_load, 0, sizeof(files_to_load));

    /*b Set default values
     */
    for (i=0; i<8; i++)
    {
        memory_regions[i][0] = 0;
        memory_regions[i][1] = 0;
        memory_regions[i][2] = 100;
        execution_regions[i][0] = 0;
        execution_regions[i][1] = 0;
        execution_regions[i][2] = 0;
    }
//	f=stdin;
	cycles=100;
//	binary=0;
	gdb_enabled = 0;
//    base_address = 0;
    symbol_map = "../../os/linux/System.map";
    eth_device = "eth3";
    for (i=0; i<16; i++)
    {
        regs[i] = 0xdeadcafe;
    }
    
//    tty_fd = run_console ();

//    ether_init(eth_device);
    
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
        /*b '-e'
         */
        if (!strcmp( argv[i], "-e" ))
        {
            i++;
            if (i<argc)
            {
                eth_device = argv[i];
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
                files_to_load[nfiles_to_load].name = argv[i];
                files_to_load[nfiles_to_load++].fp = fp;
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
        /*b '-t'
         */
        if (!strcmp( argv[i], "-t" ))
        {
            i++;
            if (i<argc)
            {
                if (sscanf( argv[i], "%d", &cycles)!=1)
                {
                    fprintf( stderr, "Failed to parse '%s' for length of run\n", argv[i] );
                }
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
        /*b '-m'
         */
        if (!strcmp (argv[i], "-m"))
        {
            must_launch_minicom = 1;
        }
        /*b All done
         */
	}

    /*b Connect to system components (console, ethernet)
     */
    tty_fd = run_console();
    if (must_launch_minicom)
        launch_minicom();
    ether_init(eth_device);

    /*b Build system
     */
   	ram = new c_ram_model( 1<<24 ); // Create RAM of size 1<<24 = 16MB
   	memory = new c_memory_model();
	arm = new c_arm_model( memory );
   	zero_page_ram = new c_ram_model( 1<<24 ); // Create zero-pagee RAM of size 1<<24 = 16MB
   	mmio = new c_mmio_model( arm ); // Create our standard MMIO model

    zero_page_ram->register_with_memory_map( memory, 0x00000000, 0x10000000 ); // Register zero-page RAM at 0x00000000 to 0xd0000000
    ram->register_with_memory_map( memory, 0xc0000000, 0x10000000 ); // Register RAM at 0xc0000000 to 0xd0000000
    mmio->register_with_memory_map( memory, 0xff000000, 0x00ffffff ); // Register MMIO at 0xff000000 to 0xfffffff
    memory->register_debug_handler( NULL, memory_model_debug );

    microkernel * ukernel = new microkernel(arm);
    arm->register_microkernel (ukernel);
    
    /*b Load code and symbols
     */
    for (i = 0; i < nfiles_to_load; i++)
    {
	file_desc * fdp = files_to_load + i;
	if (fdp->binary)
	{
		printf ("Loading binary file '%s' at %08x\n", fdp->name, fdp->addr);
        arm->load_code_binary( fdp->fp, fdp->addr );
	}
	else
	{
		printf ("Loading text file '%s' at %08x\n", fdp->name, fdp->addr);
        arm->load_code( fdp->fp, fdp->addr );
	}
    }
    for (i=0; i<16; i++)
    {
        arm->set_register( i, regs[i] );
    }
    symbol_initialize( symbol_map );

    /*b Set a signal handler
     */
    signal (SIGUSR1, handle_user1);
    signal (SIGUSR2, handle_user2);
    
    /*b Enable memory logging and tracing of ARM
     */
    memory->set_log_file( "memory_model.log" );
    for (i=0; i<8; i++)
    {
        if (memory_regions[i][2]<4)
        {
            memory->set_log_level( memory_regions[i][0], memory_regions[i][1]-memory_regions[i][0], memory_regions[i][2] );
        }
    }

    arm->trace_set_file( "arm_exec_trace.log" );
    for (i=0; i<8; i++)
    {
        if (execution_regions[i][2])
        {
            arm->trace_region( i, execution_regions[i][0], execution_regions[i][1] ); // Trace execution in main ram
        }
    }

    /*b Initialize GDB server if required
     */
    if (gdb_enabled)
    {
	    printf ("Starting GDB interface...\n");
        gdb_stub_init( arm, memory, mmio );
    }
    else
    {
        gdb_stub_disable();
    }
    
    /*b Run until completion, or forever if under gdb
     */
//    time_to_next_irq = 100000;
    if (gdb_enabled)
    {
	int next_timer = get_ticks() + 1;
        int instruction_count;
        while (1)
        {
            if (sigusr1)
            {
                sigusr1 = 0;
                gdb_trap (5);
            }
            instruction_count = gdb_poll( okay );
            if (instruction_count<=0)
            {
                instruction_count = 100000;
            }

	    if (instruction_count > 50) instruction_count = 50;
//            if (instruction_count > time_to_next_irq)
//                instruction_count = time_to_next_irq;
	    
            int actual_count = arm->step_with_cache( &okay, instruction_count );

//            time_to_next_irq -= actual_count;

	    if (ether_poll())
		ukernel->handle_interrupt (2);
	    
	    if (keyboard_poll ())
		    ukernel->handle_interrupt (1);
	    
	    if (get_ticks() > next_timer)
	    {
		    next_timer++;
		    ukernel->handle_interrupt (0);

	    }
        }
    }
    else
    {
	int next_timer = get_ticks() + 1;
//	struct timeval tv;
        int instruction_count;
        int actual_count;
	int total_instructions = 0;
	int start_time = get_ticks();
	
        while (cycles>0)
        {
            if (sigusr1)
            {
                sigusr1 = 0;
            }

            instruction_count = cycles;

//            if (instruction_count > time_to_next_irq)
//              instruction_count = time_to_next_irq;
	   
//	    if (instruction_count > 500) instruction_count = 500;
	    if (instruction_count > 50) instruction_count = 50;
	    
            actual_count = arm->step_with_cache( &okay, instruction_count );
            cycles -= actual_count;
	    total_instructions += actual_count;
	    
	    if (ether_poll())
		    ukernel->handle_interrupt (2);

	    if (keyboard_poll ())
		    ukernel->handle_interrupt (1);

	    if (get_ticks() > next_timer)
	    {
		    next_timer++;
		    ukernel->handle_interrupt (0);

	    }
        }
    }

    /*b Done
     */
}
