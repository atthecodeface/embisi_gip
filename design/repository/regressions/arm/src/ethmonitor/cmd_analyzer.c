/*a Includes
 */
#include "cmd.h"
#include "gip_support.h"
#include "../drivers/uart.h"

/*a Defines
 */
#define DELAY(a) {int i;for (i=0; i<a; i++) __asm__ volatile("mov r0, r0");}

/*a Types
 */
/*t t_data_fn, t_data_init_fn
 */
typedef void (t_data_init_fn)(unsigned int *ptr, unsigned int size );
typedef unsigned int (t_data_fn)(void);

/*t t_data_type
 */
typedef struct t_data_type
{
    t_data_init_fn *init_fn;
    t_data_fn *data_fn;
} t_data_type;

/*a Data functions
 */
static unsigned int data_type_args[4];
static void data_type_null_init( unsigned int *ptr, unsigned int size )
{
}
static unsigned int data_type_null_data( void )
{
    return 0;
}

static void data_type_incrementing_init( unsigned int *ptr, unsigned int size )
{
    data_type_args[0] = 0;
}
static unsigned int data_type_incrementing_data( void )
{
    return data_type_args[0]++;
}

static void data_type_address_init( unsigned int *ptr, unsigned int size )
{
    data_type_args[0] = (unsigned int)ptr;
}
static unsigned int data_type_address_data( void )
{
    return data_type_args[0]++;
}

static void data_type_random_init( unsigned int *ptr, unsigned int size )
{
    data_type_args[0] = (unsigned int)ptr * 0xdeadbeef + size;
    data_type_args[1] = (unsigned int)ptr * 0xcaf76451;
}
static unsigned int data_type_random_data( void )
{
    data_type_args[0] = (data_type_args[0] << 1) ^ ((data_type_args[0]&0x80000000)==0) ^ ((data_type_args[0]&0x08000000)==0);
    data_type_args[1] = (data_type_args[1] << 13) ^ (data_type_args[1]>>19) ^ ((data_type_args[1]&0x80000000)==0) ^ ((data_type_args[1]&0x008000000)==0);
    return data_type_args[0] ^ data_type_args[1];
}
#define MAX_DATA_TYPES (3)
static const t_data_type data_type[] = 
{
    {data_type_null_init, data_type_null_data},
    {data_type_incrementing_init, data_type_incrementing_data},
    {data_type_address_init, data_type_address_data},
    {data_type_random_init, data_type_random_data}
};

/*a Static functions
 */
/*f command_analyzer_read
  .. <n>
 */
static int command_analyzer_read( void *handle, int argc, unsigned int *args )
{
    unsigned int s;
    if (argc<1) return 1;
    if (!args[0])
    {
        GIP_ANALYZER_READ_CONTROL(s);
    }
    else
    {
        GIP_ANALYZER_READ_DATA(s);
    }
    cmd_result_string( handle, "Analyzer read :" );
    cmd_result_hex8( handle, s );
    cmd_result_nl(handle);
    return 0;
}

/*f command_analyzer_write
  .. <n> <v>
 */
static int command_analyzer_write( void *handle, int argc, unsigned int *args )
{
    if (argc<2) return 1;
    switch (args[0])
    {
    case 0: GIP_ANALYZER_WRITE(0,args[1]); break;
    case 1: GIP_ANALYZER_WRITE(1,args[1]); break;
    case 2: GIP_ANALYZER_WRITE(2,args[1]); break;
    case 3: GIP_ANALYZER_WRITE(3,args[1]); break;
    case 4: GIP_ANALYZER_WRITE(4,args[1]); break;
    }
    return 0;
}

/*f command_analyzer_read_trace
  .. <max>
 */
static int command_analyzer_read_trace( void *handle, int argc, unsigned int *args )
{
    int i;
    unsigned int s;
    if (argc<1) return 1;
    GIP_ANALYZER_READ_CONTROL(s);
    GIP_ANALYZER_WRITE(0,s|4); // enable readback
    cmd_result_string( handle, "Analyzer trace..." );
    for (i=0; i<args[0]; i++)
    {
        GIP_ANALYZER_READ_CONTROL(s);
        if (!(s&8)) break;
        GIP_ANALYZER_READ_DATA(s);
        if ((i&7)==0)
        {
            cmd_result_nl(handle);
            cmd_result_hex8( handle, i );
            cmd_result_string( handle, " : " );
        }
        else
        {
            cmd_result_string( handle, " " );
        }
        cmd_result_hex8( handle, s );
        if ((i&63)==0)// && (!handle))
        {
            int j;
            for (j=0; j<300000; j++) {uart_rx_poll();}
        }
    }
    cmd_result_nl(handle);
    return 0;
}

/*a External variables
 */
/*v monitor_analyzer_chain
 */
extern const t_command monitor_cmds_analyzer[];
const t_command monitor_cmds_analyzer[] =
{
    {"anrd",  command_analyzer_read},
    {"anwr", command_analyzer_write},
    {"antr", command_analyzer_read_trace},
    {(const char *)0, (t_command_fn *)0},
};

