/*a Copyright Gavin J Stark and John Croft, 2003
 */

/*a Includes
 */
#include <stdlib.h>
#include <stdio.h>
#include "c_mmio_model.h"
#include "c_memory_model.h"
#include "c_gip_pipeline_single.h"
#include "gdb_stub.h"
#include "ether.h"

extern int debug_level;

void tty_out (char c);
char tty_in (void);

/*a Defines
 */

/*a Types
 */
/*t t_mmio_model_data
 */
typedef struct t_mmio_model_data
{
//	unsigned int isr;
//	unsigned int return_addr;
    c_gip_pipeline_single *gip;
} t_mmio_model_data;

/*a Static function wrappers for class functions
 */
/*f write_mmio
 */
static void write_mmio( void *handle, unsigned int address, unsigned int data, int bytes )
{
    c_mmio_model *mmio = (c_mmio_model *)handle;
    mmio->write( address, data, bytes );
}

/*f read_mmio
 */
static unsigned int read_mmio( void *handle, unsigned int address )
{
    c_mmio_model *mmio = (c_mmio_model *)handle;
    return mmio->read( address );
}

/*a Constructors and destructors
 */
/*f c_mmio_model::c_mmio_model
 */
c_mmio_model::c_mmio_model( c_gip_pipeline_single *gip )
{
	private_data = (t_mmio_model_data *)malloc(sizeof(t_mmio_model_data));
//	private_data->isr = 0;
//	private_data->return_addr = 0;
    private_data->gip = gip;
}

/*f c_mmio_model::~c_mmio_model
 */
c_mmio_model::~c_mmio_model( void )
{
}

/*a Memory map registration
 */
int c_mmio_model::register_with_memory_map( class c_memory_model *memory, unsigned int base_address, unsigned int address_range_size )
{
    return memory->map_memory( (void *)this, base_address, address_range_size, write_mmio, read_mmio );
}


/*a Read/write mmio accesses
 */
/*f c_mmio_model::read
 */
unsigned int c_mmio_model::read( unsigned int address )
{
    unsigned int res = 0;
    
    switch (address & 0xff)
    {
    case 0x14:
//	printf ("Restoring Timer ISR address\n");
//        res = private_data->return_addr;
	printf ("*** HISTORY\n");
        gdb_trap(5);
        break;

    case 0x18:
        //res = private_data->gip->get_swi_code();
	break;
	
    case 0x1c:
        //res = private_data->gip->get_swi_return_addr();
//	printf ("reading pre-SWI pc (%x)\n", res);
	break;

    case 0x20:
    //	res = private_data->gip->get_swi_sp ();
//	printf ("reading pre-SWI sp (%x)\n", res);
	break;

    case 0x24:
        //res = private_data->gip->get_kernel_sp();
	break;

    case 0x28:
	res = tty_in ();
	break;

    case 0x30:
	return ether_rx ();

    case 0x34:
	return ether_size ();
	
    case 0x10:
        res = private_data->gip->get_flags();
        break;
    }

    return res;
}


/*f c_mmio_model::write
 */
void c_mmio_model::write( unsigned int address, unsigned int data, int bytes )
{
    FILE * fp;

    switch (address & 0xff)
    {
    case 0:
        data &= 0xff;
        fp = fopen ("out.log", "a");
        fprintf (fp, "%c", data);
        fclose (fp);

        if ((data >= 32 && data < 127) || data == '\n')
            printf ("%c", (data & 0xff));
        else
            printf ("%2.2x", data);
        return;

    case 4:
        debug_level = data;
        return;

    case 8:
        gdb_trap(5);
        return;

    case 0x0c:
        printf ("Setting Timer ISR address to %x\n", data);
        //private_data->gip->set_interrupt_vector (0, data);
        return;
	
    case 0x10:
        private_data->gip->set_flags( data, 0xf0000001);
        return;

    case 0x14:
	printf ("*** HISTORICAL\n");
        gdb_trap(5);
        return;

    case 0x20:
        //private_data->gip->set_interrupt_vector (1, data);
	return;

    case 0x24:
        //private_data->gip->set_kernel_sp (data);
	return;

    case 0x28:
	tty_out ((char)data);
	return;

    case 0x30:
	ether_byte ((unsigned char)data);
	return;

    case 0x34:
	ether_send ();
	return;
	
    case 0x2c:
        //private_data->gip->set_interrupt_vector (2, data);
	printf ("Setting up ISR for serial RX\n");
	return;
	
    case 0x100:
        private_data->gip->trace_restart();
        return;

    case 0x104:
        private_data->gip->trace_all_stop();
        return;
    }
}
/*
unsigned int c_mmio_model::get_timer_isr ()
{
	if (private_data->return_addr != 0)
		return 0;
	else
		return private_data->isr;
}

void c_mmio_model::set_return_address (unsigned int addr)
{
	private_data->return_addr = addr;
}
*/
