/*a Copyright Gavin J Stark and John Croft, 2003
 */

/*a Includes
 */
#include <stdlib.h>
#include <stdio.h>
#include "c_ram_model.h"
#include "c_memory_model.h"

/*a Defines
 */
#define HASH_SIZE 256
#define PAGE_BYTE_SIZE (1<<16)
#define PAGE_OCTWORD_FLAG_SIZE (PAGE_BYTE_SIZE/sizeof(unsigned int)/8)
#define LOG2_PAGE_BYTE_SIZE (16)
#define SHF_SIZE (8)
#define HASH_ENTRY(a) ( ( ((a)>>LOG2_PAGE_BYTE_SIZE) ^ (((a)>>LOG2_PAGE_BYTE_SIZE)>>SHF_SIZE) ) % HASH_SIZE )

/*a Types
 */
/*t t_ram_page
 */
typedef struct t_ram_page
{
    struct t_ram_page *next_in_order;
    struct t_ram_page *next_hashed;
    unsigned int base_address;
    unsigned char *valid_flags;
    unsigned char *accessed_flags;
    unsigned int *data;
} t_ram_page;

/*t t_ram_model_data
 */
typedef struct t_ram_model_data
{
    unsigned int memory_size;
    t_ram_page *page_list;
    t_ram_page *hashed_pages[ HASH_SIZE ];
} t_ram_model_data;

/*a Static function wrappers for class functions
 */
/*f write_ram
 */
static void write_ram( void *handle, unsigned int address, unsigned int data, int bytes )
{
    c_ram_model *ram = (c_ram_model *)handle;
    ram->write( address, data, bytes );
}

/*f read_ram
 */
static unsigned int read_ram( void *handle, unsigned int address )
{
    c_ram_model *ram = (c_ram_model *)handle;
    return ram->read( address );
}

/*a Constructors and destructors
 */
/*f c_ram_model::c_ram_model
 */
c_ram_model::c_ram_model( unsigned int memory_size )
{
    int i;

    private_data = (t_ram_model_data *)malloc(sizeof(t_ram_model_data));
    private_data->memory_size = memory_size;
    private_data->page_list = NULL;
    for (i=0; i<HASH_SIZE; i++)
    {
        private_data->hashed_pages[i] = NULL;
    }
}

/*f c_ram_model::~c_ram_model
 */
c_ram_model::~c_ram_model( void )
{
}

/*a Memory map registration
 */
int c_ram_model::register_with_memory_map( class c_memory_model *memory, unsigned int base_address, unsigned int address_range_size )
{
    this->memory = memory;
    return memory->map_memory( (void *)this, base_address, address_range_size, write_ram, read_ram );
}

/*a Read/write ram page allocation and finding
 */
/*f c_ram_model::find_page
 */
struct t_ram_page *c_ram_model::find_page( unsigned int address )
{
    unsigned int hash_entry;
    unsigned int base_address;
    t_ram_page *page;

    hash_entry = HASH_ENTRY( address );
    base_address = address &~ (PAGE_BYTE_SIZE-1);
    for (page=private_data->hashed_pages[hash_entry]; page; page=page->next_hashed)
    {
        if (page->base_address == base_address)
        {
            return page;
        }
    }
    //printf("c_ram_model::find_page:no page found for address %08x\n", address);
    return NULL;
}

/*f c_ram_model::allocate_page
 */
struct t_ram_page *c_ram_model::allocate_page( unsigned int address )
{
    unsigned int i;
    unsigned int hash_entry;
    unsigned int base_address;
    t_ram_page *page, *last_page, *new_page;

    hash_entry = HASH_ENTRY( address );
    base_address = address &~ (PAGE_BYTE_SIZE-1);

    last_page = NULL;
    for (page=private_data->hashed_pages[hash_entry]; page; page=page->next_hashed)
    {
        if (page->base_address == base_address)
        {
            return page;
        }
        if (page->base_address > base_address)
        {
            break;
        }
        last_page = page;
    }
    new_page = (t_ram_page *)malloc(sizeof(t_ram_page));
    new_page->base_address = base_address;
    new_page->valid_flags = (unsigned char *)malloc(PAGE_OCTWORD_FLAG_SIZE);
    new_page->accessed_flags = (unsigned char *)malloc(PAGE_OCTWORD_FLAG_SIZE);
    for (i=0; i<PAGE_OCTWORD_FLAG_SIZE; i++)
    {
        new_page->valid_flags[i] = 0;
        new_page->accessed_flags[i] = 0;
    }
    new_page->data = (unsigned int *)malloc(PAGE_BYTE_SIZE);
    if (last_page)
    {
        last_page->next_hashed = new_page;
    }
    else
    {
        private_data->hashed_pages[hash_entry] = new_page;
    }
    new_page->next_hashed = page;

    last_page = NULL;
    for (page=private_data->page_list; page; page=page->next_in_order)
    {
        if (page->base_address > base_address)
        {
            break;
        }
        last_page = page;
    }
    if (last_page)
    {
        last_page->next_in_order = new_page;
    }
    else
    {
        private_data->page_list = new_page;
    }
    new_page->next_in_order = page;
    return new_page;
}

/*a Read/write ram accesses
 */
/*f c_ram_model::read
 */
unsigned int c_ram_model::read( unsigned int address )
{
    t_ram_page *data_page;
    unsigned char accessed, valid;

    address = address % private_data->memory_size;
    data_page = find_page(address);
    if (!data_page)
    {
//        memory->raise_memory_exception( memory_model_debug_action_read_of_undefined_memory, 0, address, 0, 0 );
        return 0xdeadcafe;
    }

    accessed = data_page->accessed_flags[ (address>>5) & (PAGE_OCTWORD_FLAG_SIZE-1) ]; // Shift down by 5 as we have 2^3=8 valid bits per index (char) in our array, and each bit validates 2^2 addressable bytes
    accessed |= 1<<((address>>2)&7); // Add in bit that corresponds to the word we are reading
    data_page->accessed_flags[ (address>>5) & (PAGE_OCTWORD_FLAG_SIZE-1) ] = accessed;

    valid = data_page->valid_flags[ (address>>5) & (PAGE_OCTWORD_FLAG_SIZE-1) ]; // Shift down by 5 as we have 2^3=8 valid bits per index (char) in our array, and each bit validates 2^2 addressable bytes
    valid >>= (address>>2)&7;
    if (!(valid&1))
    {
//        memory->raise_memory_exception( memory_model_debug_action_read_of_undefined_memory, 0, address, 0, 0 );
        return 0xdeadcafe;
    }

    return data_page->data[ (address & (PAGE_BYTE_SIZE-1)) / sizeof(unsigned int) ];
}

/*f c_ram_model::write
 */
void c_ram_model::write( unsigned int address, unsigned int data, int bytes )
{
    t_ram_page *data_page;
    unsigned int new_value, old_value;
    unsigned char old_valid, accessed;
    int valid_shift;

    address = address % private_data->memory_size;
    data_page = find_page( address );
    if (!data_page)
    {
        data_page = allocate_page( address );
        //printf("c_ram_model::write:Allocated page %p for address %08x\n", data_buf, address );
    }

    accessed = data_page->accessed_flags[ (address>>5) & (PAGE_OCTWORD_FLAG_SIZE-1) ]; // Shift down by 5 as we have 2^3=8 valid bits per index (char) in our array, and each bit validates 2^2 addressable bytes
    accessed |= 1<<(address>>2)&7; // Add in bit that corresponds to the word we are reading
    data_page->accessed_flags[ (address>>5) & (PAGE_OCTWORD_FLAG_SIZE-1) ] = accessed;

    old_valid = data_page->valid_flags[ (address>>5) & (PAGE_OCTWORD_FLAG_SIZE-1) ]; // Shift down by 5 as we have 2^3=8 valid bits per index (char) in our array, and each bit validates 2^2 addressable bytes
    valid_shift = (address>>2)&7; // Select bit of old_valid that corresponds to the word we are writing

    if (old_valid & (1<<valid_shift))
    {
        old_value = data_page->data[ (address & (PAGE_BYTE_SIZE-1)) / sizeof(unsigned int) ];
    }
    else
    {
        old_value = 0;
        data_page->valid_flags[ (address>>5) & (PAGE_OCTWORD_FLAG_SIZE-1) ] = old_valid | (1<<valid_shift);
    }

    new_value = old_value;
    if (bytes&1)
    {
        new_value &= 0xffffff00;
        new_value |= 0x000000ff & data;
    }
    if (bytes&2)
    {
        new_value &= 0xffff00ff;
        new_value |= 0x0000ff00 & data;
    }
    if (bytes&4)
    {
        new_value &= 0xff00ffff;
        new_value |= 0x00ff0000 & data;
    }
    if (bytes&8)
    {
        new_value &= 0x00ffffff;
        new_value |= 0xff000000 & data;
    }
//  printf ("c_ram_model::write: write %x %x %x %x->%x\n", address, data, bytes, old_value, new_value);
    data_page->data[ (address & (PAGE_BYTE_SIZE-1)) / sizeof(unsigned int) ] = new_value;
}

