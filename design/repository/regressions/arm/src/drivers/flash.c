/*a Includes
 */
#include "gip_support.h"
#include "flash.h"

/*a Defines
 */
#define POLL_TIMEOUT (1024*1024)
#define MASK_128kB (0x1ffff)

/*a Types
 */

/*a Erase, write and read functions
 */
/*f flash_poll_status
 */
static unsigned int flash_poll_status( void )
{
    unsigned int v;
    int i;

    v = 0;
    for (i=0; i<POLL_TIMEOUT; i++)
    {
        NOP; NOP; NOP; NOP; NOP;
        FLASH_DATA_READ( v );
        if (v&0x80) // Wait for v&0x80 (ready) to be high
            break;
    }
    return v;
}

/*f flash_erase_block
  Erase a bank of flash.
  address should be aligned to a 128kbyte boundary
  address is byte-based
  return 1 for success, 0 for failure
 */
extern int flash_erase_block( unsigned int address )
{
    unsigned int v;

    /*b Check address is aligned
     */
    if (address & MASK_128kB)
        return 0;

    /*b Set config
     */
    FLASH_CONFIG_WRITE( 0x3ff );

    /*b Clear status
     */
    FLASH_DATA_WRITE( 0x50 ); // write 0x50 - clear status

    /*b Send erase command
     */
    FLASH_ADDRESS_WRITE( address/2 ); // set address to our bank address, chip select 0
    FLASH_DATA_WRITE( 0x20 ); // write 0x20 to our address - bank erase
    FLASH_DATA_WRITE( 0xd0 ); // write 0xd0 to our address - bank erase confirm

    /*b Poll status (with timeout)
     */
    FLASH_DATA_WRITE( 0x70 ); // write 0x70 - status
    v = flash_poll_status();
    return !(v&0x20);
}

/*f flash_write_buffer
  address is byte-based
  return 1 for success, 0 for failure
 */
extern int flash_write_buffer( unsigned int address, unsigned char *buffer, int length  )
{
    int i, j;
    unsigned int v;

    /*b Set config
     */
    FLASH_CONFIG_WRITE( 0x3ff );

    /*b Clear status
     */
    FLASH_DATA_WRITE( 0x50 ); // write 0x50 - clear status

    /*b Write data to buffer in bursts
     */
    while (length>0)
    {
        /*b Calculate length for this burst
         */
        i = 32;
        if (i>length); i = length;
        if (i>32-(int)((address&0x1f))) i=32-((int)(address&0x1f));
        i = (i+1)/2;
        if (i==0) break;

        /*b Now write to the flash 'i' 16-bit values from buffer to address/2
         */
        FLASH_CONFIG_WRITE( 0x3ff );
        FLASH_ADDRESS_WRITE( address/2 ); // Set address of write buffer program
        FLASH_DATA_WRITE( 0xe8 ); // write 0xe8 - write to buffer

        /*b Poll status (write buffer available) with timeout
         */
        v = flash_poll_status();
        if (!(v&0x80))
            return 0;

        /*b Write word count -1
         */
        FLASH_DATA_WRITE( i-1 ); // write word count

        /*b Fill the write buffer
         */
        FLASH_CONFIG_WRITE( 0x7ff );
        for (j=0; j<i; j++)
        {
            v = buffer[0] | (buffer[1]<<8);
            FLASH_DATA_WRITE( v ); // write word count
            buffer+=2;
        }

        /*b Flash write buffer confirm, and poll status until done
         */
        FLASH_CONFIG_WRITE( 0x3ff );
        FLASH_ADDRESS_WRITE( (address/2) ); // Set address of write buffer program
        FLASH_DATA_WRITE( 0xd0 ); // write 0xd0 - confirm write
        v = flash_poll_status();
        if (!(v&0x80))
            return 0;

        /*b Move on pointers etc
         */
        length -= i*2;
        address += i*2;
    }

    /*b Done
     */
    return 1;
}

/*f flash_read_buffer
  address is byte-based
  return 1 for success, 0 for failure
 */
extern int flash_read_buffer( unsigned int address, unsigned char *buffer, int length  )
{
    unsigned int v;

    /*b Set config
     */
    FLASH_CONFIG_WRITE( 0x7ff );

    /*b Put in read array mode
     */
    FLASH_DATA_WRITE( 0xff ); // write 0xff - read array

    /*b Set address
     */
    FLASH_ADDRESS_WRITE( address/2 );

    /*b Read odd first byte if required
     */
    if (address&1)
    {
        FLASH_DATA_READ( v );
        buffer[0] = (v>>8);
        length--;
        buffer++;
    }

    /*b Read remaining data except last byte, if odd
     */
    while (length>=2)
    {
        FLASH_DATA_READ( v );
        buffer[0] = v&0xff;
        buffer[1] = (v>>8)&0xff;
        buffer+=2;
        length-=2;
    }

    /*b Read last byte
     */
    if (length)
    {
        FLASH_DATA_READ( v );
        buffer[0] = v&0xff;
    }

    /*b Done
     */
    return 1;
}

