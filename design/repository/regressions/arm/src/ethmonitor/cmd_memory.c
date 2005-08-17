/*a Includes
 */
#include "cmd.h"
#include "gip_support.h"

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
/*f command_mem_rep_read
  .. <address> <count> [<delay>]
 */
#ifndef TINY
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
#endif

/*f command_mem_rep_write
  .. <address> <data> <count> [<delay>]
 */
#ifndef TINY
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
#endif

/*f command_mem_fill
  .. <address> <size> <type>
 */
#ifndef TINY
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
#endif

/*f command_mem_verify
  .. <address> <size> <type>
 */
#ifndef TINY
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
#endif

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
            cmd_result_hex8( handle, (int)(ptr+i*sizeof(int)) );
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

/*f command_mem_push_phase
  .. <amount> <dirn>
 */
static int command_mem_push_phase( void *handle, int argc, unsigned int *args )
{
    int i;
    int dirn;
    if (argc<2) return 1;
    dirn = (args[1]!=0);
    GIP_LED_OUTPUT_CFG_WRITE(0);
    for (i=0; i<args[0]; i++)
    {
        if (dirn)
        {
            GIP_LED_OUTPUT_CFG_WRITE(0xf000); // set bit 7 to direction (1 is inc), bit 6 to make it go; then clear bit 6
        }
        else
        {
            GIP_LED_OUTPUT_CFG_WRITE(0x3000); // set bit 7 to direction (1 is inc), bit 6 to make it go; then clear bit 6
        }
        GIP_BLOCK_ALL();
        GIP_LED_OUTPUT_CFG_WRITE(0); // Clear psen
        GIP_BLOCK_ALL();
        DELAY(10000);
    }
    return 0;
}

/*f command_mem_rep_test
  .. <address> [<length in bytes / 0x1000>] [<count / 8>] [<max errors / 1>] [xor address]
  mmrt 80000000 20 100 0 1
  mmrt 80000000 20 10000
  mmrt 80000000 1000 60
 */
static int command_mem_rep_test( void *handle, int argc, unsigned int *args )
{
    int i, j, k;
    volatile unsigned int *ptr;
    int length, count, max_errors, xor_address;
    unsigned int value, read, diff;
    int errors, last_error;
    int errored_bits[32];

    static const unsigned int test_values[] = {0, 0xffffffff, 0x55555555, 0x00000001, 0xfffffffe, 0x11111111, 0xf, 0xff };

    if (argc<1)
        return 1;

    ptr = (unsigned int *)args[0];
    length = 0x1000;
    if (argc>1) length=args[1]/4;
    count = 8;
    if (argc>2) count=args[2];
    max_errors = 1;
    if (argc>3) max_errors=args[3];
    xor_address = 0;
    if (argc>4) xor_address=args[4];

    for (i=0; i<32; i++) errored_bits[i] = 0;
    errors = 0;
    last_error = 0;

    for (i=0; (i<count) && (errors<max_errors); i++)
    {
        value = test_values[i&7];
        for (j=0; j<length; j++)
        {
            ptr[j] = value ^ (xor_address?((unsigned int)(ptr+j)):0);
            value = ((value&0x80000000)?1:0) | (value<<1);
        }
        value = test_values[i&7];
        for (j=0; j<length; j++)
        {
            read = ptr[j];
            diff = read ^ value ^ (xor_address?((unsigned int)(ptr+j)):0);
            if (diff)
            {
                for (k=0; k<32; k++, diff>>=1)
                {
                    if (diff&1) errored_bits[k]++;
                }
                errors++;
                last_error = j;
            }
            value = ((value&0x80000000)?1:0) | (value<<1);
        }
    }

    if (errors)
    {
        cmd_result_string( handle, "!!ERRORS:" );
        cmd_result_hex8( handle, errors );
        cmd_result_string( handle, " last at ");
        cmd_result_hex8( handle, (unsigned int)(ptr+last_error) );
        cmd_result_nl( handle );
        for (i=0; i<32; i++)
        {
            cmd_result_hex2( handle, i );
            cmd_result_string( handle, ":" );
            cmd_result_hex8( handle, errored_bits[i] );
            cmd_result_string( handle, "    " );
        }
    }
    else
    {
        cmd_result_string( handle, "No errors" );
        cmd_result_nl( handle );
    }

    return 0;
}

/*f command_mem_find
  .. <address> <value> [<mask>] [<length in words / 0x400>] [end_after / 16]
 */
static int command_mem_find( void *handle, int argc, unsigned int *args )
{
    int i;
    volatile unsigned int *ptr;
    int length, finds, end_after;
    unsigned int value, mask;

    if (argc<2)
        return 1;

    ptr = (unsigned int *)args[0];
    value=args[1];
    mask = 0xffffffff;
    if (argc>2) mask=args[2];
    length = 0x400;
    if (argc>3) length=args[3];
    end_after = 16;
    if (argc>4) end_after=args[4];

    finds = 0;
    for (i=0; (i<length) && (finds<end_after); i++)
    {
        if ((ptr[i] & mask)==value)
        {
            cmd_result_string( handle, "Found at :" );
            cmd_result_hex8( handle, (unsigned int)(ptr+i) );
            cmd_result_string( handle, " value ");
            cmd_result_hex8( handle, ptr[i] );
            cmd_result_nl( handle );
            finds++;
        }
    }
    if (finds==0)
    {
        cmd_result_string_nl( handle, "Not found" );
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
#ifndef TINY
    {"mmrr", command_mem_rep_read},
    {"mmrw", command_mem_rep_write},
    {"mmfi", command_mem_fill},
    {"mmve", command_mem_verify},
#endif
    {"mmrt", command_mem_rep_test},
    {"mmfn", command_mem_find},
    {"mmpp", command_mem_push_phase},
    {(const char *)0, (t_command_fn *)0},
};

