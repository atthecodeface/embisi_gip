/*a Includes
 */
#include "uart.h"
#include "monitor.h"

/*a Defines
 */
#define DELAY(a) {int i;for (i=0; i<a; i++);}

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
/*f command_mem_rep_read
  .. <address> <count> [<delay>]
 */
static int command_mem_rep_read( int argc, unsigned int *args )
{
    volatile unsigned int *ptr;
    int i;
    int count, delay;
    int j;
    if (argc<2)
        return 1;
    delay = 0;
    if (argc>2) delay=args[2];
    ptr = (unsigned int *)args[0];
    count = args[1];
    for (i=0; i<count; i++)
    {
        j = *ptr;
        DELAY(delay);
    }

    return 0;
}

/*f command_mem_rep_write
  .. <address> <data> <count> [<delay>]
 */
static int command_mem_rep_write( int argc, unsigned int *args )
{
    volatile unsigned int *ptr;
    int i;
    unsigned int value;
    int count, delay;
    if (argc<3)
        return 1;
    delay = 0;
    if (argc>3) delay=args[3];
    ptr = (unsigned int *)args[0];
    value = args[1];
    count = args[2];
    for (i=0; i<count; i++)
    {
        *ptr = value;
        DELAY(delay);
    }

    return 0;
}

/*f command_mem_fill
  .. <address> <size> <type>
 */
static int command_mem_fill( int argc, unsigned int *args )
{
    unsigned int *ptr;
    unsigned int i;
    unsigned int size, type;
    if (argc<3)
        return 1;
    ptr = (unsigned int *)args[0];
    size = args[1];
    type = args[2];
    if (type>MAX_DATA_TYPES) type=0;
    data_type[type].init_fn(ptr, size);
    for (i=0; i<size; i++)
    {
        *ptr++ = data_type[type].data_fn();
    }

    return 0;
}

/*f command_mem_verify
  .. <address> <size> <type>
 */
static int command_mem_verify( int argc, unsigned int *args )
{
    volatile unsigned int *ptr;
    unsigned int i, j, k, l;
    unsigned int size, type;
    if (argc<3)
        return 1;
    ptr = (unsigned int *)args[0];
    size = args[1];
    type = args[2];
    if (type>MAX_DATA_TYPES) type=0;
    data_type[type].init_fn( (unsigned int *)ptr, size);
    l = 0;
    for (i=0; (l<10) && (i<size); i++)
    {
        j = *ptr;
        k=data_type[type].data_fn();
        if (j!=k)
        {
            uart_tx_string( "Diff at " );
            uart_tx_hex8( (unsigned int)ptr );
            uart_tx_string(" : ");
            uart_tx_hex8( j );
            uart_tx_string(" : ");
            uart_tx_hex8( k );
            uart_tx_nl();
            l++;
        }
        ptr++;
    }

    return 0;
}

/*a External variables
 */
/*v monitor_memory_chain
 */
extern const t_command monitor_memory_cmds[];
const t_command monitor_memory_cmds[] =
{
    {"mmrr", command_mem_rep_read},
    {"mmrw", command_mem_rep_write},
    {"mmf", command_mem_fill},
    {"mmv", command_mem_verify},
    {(const char *)0, (t_command_fn *)0},
};

