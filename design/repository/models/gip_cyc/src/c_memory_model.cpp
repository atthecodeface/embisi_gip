/*a Copyright Gavin J Stark and John Croft, 2003
 */

/*a Includes
 */
#include <stdlib.h>
#include <stdio.h>

#include "gdb_stub.h"
#include "c_memory_model.h"

/*a Defines
 */
#define MAP_SHIFT (16)
#define MAP_SIZE (1<<(32-MAP_SHIFT))
#define LOG(l,s,a,d,b ) {if (private_data->log_levels[(a>>MAP_SHIFT)]>=l){this->log(s,a,d,b);};}

/*a Types
 */
/*t enum log_level
 */
enum
{
    log_level_none = 0,
    log_level_serious = 1,
    log_level_transaction = 2,
    log_level_detail = 3,
};

/*t t_memory_map_fns
 */
typedef struct t_memory_map_fns
{
    struct t_memory_map_fns *next_in_list;
    void *handle;
    t_memory_model_read_fn *read_fn;
    t_memory_model_write_fn *write_fn;
    int log_level;
} t_memory_map_fns;

/*t t_memory_model_data
 */
typedef struct t_memory_model_data
{
    t_memory_map_fns *model_fns[ MAP_SIZE ];
    t_memory_map_fns *map_fns_list;
    void *debug_handle;
    t_memory_model_debug_fn *debug_fn;
    FILE *log_file;
    int log_levels[ MAP_SIZE ];
} t_memory_model_data;

/*a Constructors and destructors
 */
/*f c_memory_model::c_memory_model
 */
c_memory_model::c_memory_model( void )
{
	int i;

	private_data = (t_memory_model_data *)malloc(sizeof(t_memory_model_data));
    for (i=0; i<MAP_SIZE; i++)
    {
        private_data->model_fns[i] = NULL;
        private_data->log_levels[i] = log_level_none;
    }
    private_data->map_fns_list = NULL;
    private_data->debug_fn = NULL;
    private_data->log_file = NULL;
}

/*f c_memory_model::~c_memory_model
 */
c_memory_model::~c_memory_model( void )
{
}

/*a Mapping and debug functions
 */
/*f c_memory_model::map_memory
  Map memory using functions and handle for all the address starting at the base for the address_range_size, inclusive of the first address, exclusive of the last
 */
int c_memory_model::map_memory( void *handle, unsigned int base_address, unsigned int address_range_size, t_memory_model_write_fn write_fn, t_memory_model_read_fn read_fn )
{
    unsigned int i;
    t_memory_map_fns *map_fns;

    map_fns = NULL;
    //printf("c_memory_model::map_memory:mapping %08x %08x %d\n", base_address, address_range_size, MAP_SHIFT);
    for (i=base_address>>MAP_SHIFT; i<=(base_address+address_range_size-1)>>MAP_SHIFT; i++)
    {
        if (private_data->model_fns[i])
        {
            fprintf( stderr, "Attempt to doubly map memory at %08x\n", i<<MAP_SHIFT );
            return 0;
        }
        if (!map_fns)
        {
            map_fns = (t_memory_map_fns *)malloc(sizeof(t_memory_map_fns));
            map_fns->next_in_list = private_data->map_fns_list;
            private_data->map_fns_list = map_fns;
            map_fns->handle = handle;
            map_fns->write_fn = write_fn;
            map_fns->read_fn = read_fn;
        }
        private_data->model_fns[i] = map_fns;
    }
    return 1;
}

/*f c_memory_model::register_debug_handler
  Register a debugger to be called when a bad access occurs, a watchpoint is hit, and so on
 */
int c_memory_model::register_debug_handler( void *handle, t_memory_model_debug_fn debug_fn )
{
    private_data->debug_handle = handle;
    private_data->debug_fn = debug_fn;
    return 1;
}

/*a Read/write memory accesses
 */
/*f c_memory_model::read_memory
 */
unsigned int c_memory_model::read_memory( unsigned int address )
{
    t_memory_map_fns *map_fns;
    unsigned int read_data;

    map_fns = private_data->model_fns[(address>>MAP_SHIFT)&(MAP_SIZE-1)];
    if (!map_fns)
    {
        LOG( log_level_serious, "read unmapped memory", address, 0, 0 );
        if (private_data->debug_fn)
        {
            private_data->debug_fn( private_data->debug_handle, memory_model_debug_action_unmapped_memory, 0, address, 0, 0 );
        }
        else
        {
            fprintf( stderr, "c_memory_model::read_memory:access to unmapped memory %08x - no debug trap set\n", address );
        }
        return 0xdeadbeef;
    }
    read_data = map_fns->read_fn( map_fns->handle, address );
    LOG( log_level_transaction, "r", address, read_data, 0 );
    return read_data;
}

unsigned long write_trap;

/*f c_memory_model::write_memory
 */
void c_memory_model::write_memory( unsigned int address, unsigned int data, int bytes )
{
    t_memory_map_fns *map_fns;
    
    if (write_trap && address == write_trap)
    {
	    printf ("Write trap at %08x\n", address);
	    gdb_trap(5);
    }

    map_fns = private_data->model_fns[(address>>MAP_SHIFT)&(MAP_SIZE-1)];
    if (!map_fns)
    {
        LOG( log_level_serious, "write unmapped memory", address, 0, 0 );
        if (private_data->debug_fn)
        {
            private_data->debug_fn( private_data->debug_handle, memory_model_debug_action_unmapped_memory, 1, address, data, bytes );
        }
        else
        {
            fprintf( stderr, "c_memory_model::write_memory:access to unmapped memory %08x - no debug trap set\n", address );
        }
        return;
    }
    LOG( log_level_transaction, "w", address, data, bytes );
    map_fns->write_fn( map_fns->handle, address, data, bytes );
}

/*f c_memory_model::copy_string
 */
void c_memory_model::copy_string( char *dest, unsigned int address, int max_length )
{
    int i;
    t_memory_map_fns *map_fns;
    unsigned int read_data;

    for (i=0; i<max_length; i++, address++)
    {
        map_fns = private_data->model_fns[(address>>MAP_SHIFT)&(MAP_SIZE-1)];
        if (!map_fns)
        {
            LOG( log_level_serious, "read unmapped memory for string copy", address, 0, 0 );
            if (private_data->debug_fn)
            {
                private_data->debug_fn( private_data->debug_handle, memory_model_debug_action_unmapped_memory, 0, address, 0, 0 );
            }
            else
            {
                fprintf( stderr, "c_memory_model::read_memory:access to unmapped memory for string copy %08x - no debug trap set\n", address );
            }
            dest[0] = 0;
            return;
        }
        read_data = map_fns->read_fn( map_fns->handle, address );
        dest[i] = read_data>>((address&3)*8);
        if (dest[i]==0)
        {
            break;
        }
//        fprintf(stderr, "Read char %02x at address %08x (data %08x)\n", dest[i], address, read_data );
    }
    dest[max_length-1] = 0;
}

/*a Memory exception handling
 */
/*f c_memory_model::raise_memory_exception
 */
void c_memory_model::raise_memory_exception( t_memory_model_debug_action action, int write_not_read, unsigned int address, unsigned int data, int bytes )
{
    LOG( log_level_serious, "memory exception raised", address, data, action );
    if (private_data->debug_fn)
    {
        private_data->debug_fn( private_data->debug_handle, action, write_not_read, address, data, bytes );
    }
}

/*a Log capability
 */
/*f c_memory_model::set_log_file
 */
int c_memory_model::set_log_file( const char *filename )
{
    if (private_data->log_file)
    {
        fclose( private_data->log_file );
        private_data->log_file = NULL;
    }
    if (filename)
    {
        private_data->log_file = fopen( filename, "w+" );
    }
    return (private_data->log_file!=NULL);
}

/*f c_memory_model::set_log_level( level )
 */
void c_memory_model::set_log_level( int log_level )
{
    set_log_level( 0, 0xffffffff, log_level );
}

/*f c_memory_model::set_log_level( address, size, level )
 */
void c_memory_model::set_log_level( unsigned int base_address, unsigned int size, int log_level )
{
    unsigned int i;

    for (i=base_address>>MAP_SHIFT; i<=(base_address+size-1)>>MAP_SHIFT; i++)
    {
        private_data->log_levels[i] = log_level;
    }
}

/*f c_memory_model::log
 */
void c_memory_model::log( const char *string, unsigned int address, unsigned int data, int bytes )
{
    FILE *f;
    f = stdout;
    if (private_data->log_file)
    {
        f = private_data->log_file;
    }
    fprintf( f, "m:%08x:%08x:%02x:%s\n", address, data, bytes, string );
}
