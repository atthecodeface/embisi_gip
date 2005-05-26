/*a Includes
 */
#include "cmd.h"

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
static int command_mem_rep_read( void *handle, int argc, unsigned int *args )
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
static int command_mem_rep_write( void *handle, int argc, unsigned int *args )
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
static int command_mem_fill( void *handle, int argc, unsigned int *args )
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
static int command_mem_verify( void *handle, int argc, unsigned int *args )
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
            cmd_result_string( handle, "Diff at " );
            cmd_result_hex8( handle, (unsigned int)ptr );
            cmd_result_string( handle, " : ");
            cmd_result_hex8( handle, j );
            cmd_result_string( handle, " : ");
            cmd_result_hex8( handle, k );
            cmd_result_nl(handle);
            l++;
        }
        ptr++;
    }

    return 0;
}

/*f command_mem_read_location
 */
static int command_mem_read_location( void *handle, int argc, unsigned int *args )
{
    unsigned int *ptr;
    unsigned int v;
    int i, j, max;
    if (argc<1)
        return 1;
    max = 8;
    if (argc>1)
    {
        max = args[1];
    }
    max = (max>256)?256:max;

    ptr = (unsigned int *)args[0];

    for (i=j=0; (i<max); i++)
    {
        if (j==0)
        {
            if (i>0)
            {
                cmd_result_nl( handle );
            }
            cmd_result_hex8( handle, ptr+i*sizeof(int) );
            cmd_result_string( handle, ":" );
        }
        else
        {
            cmd_result_string( handle, " " );
        }
        v = ptr[i];
        cmd_result_hex8( handle, v );
        j++;
        if (j==8) j=0;
    }
    return 0;
}

/*f command_mem_write_location
 */
static int command_mem_write_location( void *handle, int argc, unsigned int *args )
{
    unsigned int *ptr;
    int i;
    if (argc<2)
        return 1;

    ptr = (unsigned int *)args[0];

    for (i=1; i<argc; i++)
    {
        ptr[i-1] = args[i];
    }
    return 0;
}

/*a External variables
 */
/*v monitor_memory_chain
 */
extern const t_command monitor_cmds_memory[];
const t_command monitor_cmds_memory[] =
{
    {"mr", command_mem_read_location},
    {"mw", command_mem_write_location},
    {"mmrr", command_mem_rep_read},
    {"mmrw", command_mem_rep_write},
    {"mmf", command_mem_fill},
    {"mmv", command_mem_verify},
    {(const char *)0, (t_command_fn *)0},
};

