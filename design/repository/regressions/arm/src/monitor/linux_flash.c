#include <stdio.h>
#include <stdlib.h>
#include "flash.h"
#include "c_sl_error.h"
#include "sl_mif.h"

#define ADDR_MASK (0x7fffff)

typedef enum
{
    flash_mode_read_array,
    flash_mode_erase,
    flash_mode_erase_confirmed,
    flash_mode_write_buffer_wait,
    flash_mode_write_buffer_ready,
    flash_mode_write_buffer_data,
    flash_mode_write_buffer_confirm,
    flash_mode_write_buffer_confirmed,
    flash_mode_read_status,
} t_flash_mode;

typedef struct t_flash
{
    unsigned int config;
    unsigned int address;
    unsigned int wb_address;
    t_flash_mode mode;
    unsigned int *data;
    int counter;
    int status;
    int next_status;
    unsigned char *memory;
} t_flash;

static t_flash flash;
static int verbose=0;
static int inited=0;
static c_sl_error *error;

#define VERBOSE(f,a...) {if(verbose){fprintf(stderr,"%s::" f "\n", __func__, ## a);}}
#define COUNTER_ERASE (8)
#define COUNTER_WRITE_BUFFER (8)

static void flash_model_init( void )
{
    inited = 1;
    error = new c_sl_error();
    sl_mif_allocate_and_read_mif_file( error, "linux_flash.mif", "linux_flash", 8*1024*1024, 8, 0, (int **)&flash.memory, NULL, NULL );
    flash.status = 0x80;
}

static void flash_model_address_inc( void )
{
    switch ((flash.config>>10)&3)
    {
    case 0: flash.address+=0; break;
    case 1: flash.address+=1; break;
    case 2: flash.address+=2; break;
    case 3: flash.address+=4; break;
    }
}
static void flash_model_exit( void )
{
    error->check_errors_and_reset( stderr, error_level_info, error_level_info );
}

extern void flash_model_config_write( unsigned int v )
{
    VERBOSE( "%08x", v );
    if (!inited)
    {
        flash_model_init();
    }
    flash.config = v;
    flash_model_exit();
}

extern unsigned int flash_model_config_read( void )
{
    VERBOSE( "%08x", flash.config );
    flash_model_exit();
    return flash.config;
}

extern void flash_model_address_write( unsigned int v )
{
    VERBOSE( "%08x", v );
    flash.address = v;
    flash_model_exit();
}

extern unsigned int flash_model_address_read( void )
{
    VERBOSE( "%08x", flash.address );
    flash_model_exit();
    return flash.address;
}

extern void flash_model_data_write( unsigned int v )
{
    VERBOSE( "%08x", v );
    switch (flash.mode)
    {
    case flash_mode_erase:
        if ((v&0xff)==0xd0)
        {
            flash.mode = flash_mode_read_status;
            flash.counter = COUNTER_ERASE;
            flash.next_status = 0x80;
            flash.status = 0;
        }
        else
        {
            flash.mode = flash_mode_read_status;
        }
        break;
    case flash_mode_write_buffer_ready: // now we should take the amount of data!
        VERBOSE( "Write buffer being given %d 16-bit values", (v&0x0f)+1 );
        flash.counter = (v&0x0f)+1;
        flash.mode = flash_mode_write_buffer_data;
        flash.wb_address = flash.address;
        break;
    case flash_mode_write_buffer_data:
        VERBOSE( "Write buffer writing data %04x to address %08x", v&0xffff, flash.wb_address );
        flash.wb_address = flash.wb_address+1;
        if (flash.counter>1)
        {
            flash.counter = flash.counter-1;
        }
        else
        {
            flash.counter = 0;
            flash.mode = flash_mode_write_buffer_confirm;
        }
        break;
    case flash_mode_write_buffer_confirm:
        if ((v&0xff)==0xd0)
        {
            flash.mode = flash_mode_read_status;
            flash.counter = COUNTER_WRITE_BUFFER;
            flash.next_status = 0x80;
            flash.status = 0;
        }
        else
        {
            flash.mode = flash_mode_read_status;
        }
        break;
    default:
        switch (v&0xff)
        {
        case 0x70:
            flash.mode = flash_mode_read_status;
            break;
        case 0x20:
            flash.mode = flash_mode_erase;
            break;
        case 0xe8:
            flash.mode = flash_mode_write_buffer_wait;
            flash.counter = COUNTER_WRITE_BUFFER;
            flash.next_status = 0x8000;
            break;
        case 0xff:
            flash.mode = flash_mode_read_array;
            break;
        }
    }
    flash_model_exit();
}

extern unsigned int flash_model_data_read( void )
{
    unsigned int result;
    result = 0;

    VERBOSE( "address %08x", flash.address );

    if (flash.counter>0)
    {
        if (flash.counter==1)
        {
            flash.status = flash.next_status;
        }
        flash.counter--;
    }

    switch (flash.mode)
    {
    case flash_mode_read_array:
        if (flash.memory)
        {
            result = 0xcafe0000;
            result |= flash.memory[(flash.address&ADDR_MASK)*2];
            result |= flash.memory[(flash.address&ADDR_MASK)*2+1]<<8;
        }
        else
        {
            result = 0xdeadbeef;
        }
        break;
    case flash_mode_erase:
        break;
    case flash_mode_read_status:
        result = flash.status;
        break;
    case flash_mode_write_buffer_wait:
        result = flash.status>>8;
        if (flash.counter==0)
        {
            flash.mode = flash_mode_write_buffer_ready;
        }
        break;
    }

    VERBOSE( "data %08x", result );
    flash_model_address_inc();
    flash_model_exit();
    return result;
}
