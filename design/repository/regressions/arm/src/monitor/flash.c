/*a Includes
 */
#include "../common/wrapper.h"
#include "flash.h"
#include "uart.h"

/*a Defines
 */
#define POLL_TIMEOUT (1024*1024)
#define MASK_128kB (0x1ffff)
#ifdef LINUX
#define set_byte_inc( d, c ) {fprintf(stderr, "%08x <= %02x\n", d, c&0xff ); d++; }
#else
#define set_byte_inc( d, c ) {*(d++) = c;}
#endif

/*a Types
 */

/*a Erase and write functions
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
    int i;

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
    uart_tx_hex8( v );
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
        if (i>32-(address&0x1f)) i=32-(address&0x1f);
        i = (i+1)/2;
        if (i==0) break;

        /*b Now write to the flash 'i' 16-bit values from buffer to address/2
         */
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

/*f flash_download
 */
extern int flash_download( void )
{
    int i, j;
    int length;
    int result;
    unsigned int address;
    unsigned char buffer[256+12];
    unsigned int csum;
    int c;

    while (!uart_rx_poll());

    uart_tx_string_nl("db");
    while (1)
    {
        length = 0;
        while (1)
        {
            if (uart_rx_poll())
            {
                char c;
                c = uart_rx_byte();
                if (c<32)
                    break;
                if (length<sizeof(buffer)-1)
                {
                    buffer[length++] = c;
                }
            }
        }
        csum = 0;
        for (i=j=0; i<length; i++, j++)
        {
            if (buffer[i]>=0xf0)
            {
                c = ((buffer[i]&0xf)<<4)|(buffer[i+1]&0xf);
                i++;
            }
            else
            {
                c = buffer[i] ^ 0x80;
            }
            csum += c;
            buffer[j] = c;
        }
        length = j;
        csum = csum & 0xff;

        // commands coming down can be erase, write, read
        address = buffer[2] | (buffer[3]<<8) | (buffer[4]<<16) | (buffer[5]<<24);
//        fprintf(stderr, "Address %08x length %d csum %02x buffer %02x %02x\n", address, length, csum, buffer[0], buffer[1] );
        result = 0;
        if ((length>=6) && (csum==0))
        {
            switch (buffer[1])
            {
            case 'e':
                result = flash_erase_block( address );
                break;
            case 'w':
                result = flash_write_buffer( address, buffer+6, length-6 );
                break;
//            case 'r':
//                result = flash_read_buffer( address, buffer, buffer[5] | (buffer[6]<<8) );
//                break;
            default:
                break;
            }
        }
        if (!result)
            break;
        uart_tx_string_nl("dc");
    }
    uart_tx_string_nl("de");
    return 1;
}

/*a Read functions
 */
/*f flash_read_object
  Read an object from the flash address.
  If offset is 0, then csum should be cleared, and the object length read
  If offset is >0, then csum ought to be valid, and data read from address+*offset, up to total_length
  Return 0 for bad object, -1 for incomplete object, else size read for valid object
 */
extern int flash_read_object( unsigned int address, unsigned int *csum, char *buffer, int *offset, int max_length  )
{
    int total_length;
    int i;
    unsigned int v;
    unsigned int cs;
    int ending_block;

//    fprintf(stderr, "fro:%08x:%d:%d\n", address, *offset, max_length );

    /*b Set config - autoincrement
     */
    FLASH_CONFIG_WRITE( 0x7ff );

    /*b Put in read array mode
     */
    FLASH_DATA_WRITE( 0xff ); // write 0xff - read array

    /*b If not a continuation read then prepare the lengths and csum
     */
    if (*offset==0)
    {
        *csum = 0;
    }
    FLASH_ADDRESS_WRITE( address/2 );
    FLASH_DATA_READ( total_length );
    total_length = total_length&0xffff;
    if (total_length&0xc000)
    {
        uart_tx_string( "bad len " );
        uart_tx_hex8(total_length);
        uart_tx_nl();
        return 0;
    }

    /*b Read up to min(total_length-*offset, max_length) bytes to buffer from address+4+*offset, adding to checksum
     */
    total_length = total_length - *offset;
    ending_block = 1;
    if (max_length<total_length)
    {
        total_length = max_length;
        ending_block = 0;
    }
    FLASH_ADDRESS_WRITE( (address+4+*offset)/2 );

    cs = *csum;
    for (i=0; i<total_length; i+=2)
    {
        FLASH_DATA_READ( v );
        buffer[0] = v&0xff;
        buffer[1] = (v>>8)&0xff;
        cs += (v&0xffff);
        buffer+=2;
    }
    cs = (cs+(cs<<16));
    cs = cs >> 16;
    *csum = cs;
    *offset += i;

    if (ending_block)
    {
        FLASH_ADDRESS_WRITE( (address+2)/2 );
        FLASH_DATA_READ( cs );
        cs = cs & 0xffff;
        if (*csum != cs)
        {
            uart_tx_string( "csum exp " );
            uart_tx_hex8(*csum);
            uart_tx_nl();
            return 0;
        }
//        fprintf(stderr, "fro:exit:%d\n", total_length );
        return total_length;
    }
//    fprintf(stderr, "fro:exit:%d\n", -1 );
    return -1;
}

/*f flash_boot
  Read the flash from address, and perform a boot of that if possible
  Return 0 on error, 1 if booting done (returned from image or no execution in objects at address)
 */
extern int flash_boot( unsigned int address )
{
    int i;
    unsigned int csum;
    int offset;
    char buffer[56];
    int have_regs;
    unsigned int regs[12];
    int obj_status;
    int done, error;

    done = 0;
    error = 0;
    have_regs = 0;
    while (!done && !error)
    {
        offset = 0;
        obj_status = flash_read_object( address, &csum, buffer, &offset, sizeof( buffer ) );
        if (obj_status==0) // not an object!
        {
            return 0;
        }
        if (obj_status>0) // not a data block
        {
            address += obj_status+4;
            switch (buffer[0])
            {
            case obj_type_description:
                uart_tx_string("Loading ");
                uart_tx_string_nl(buffer+1);
                break;
            case obj_type_data:
            {
                unsigned char *dest;
                dest = (unsigned char *)(buffer[1] | (buffer[2]<<8) | (buffer[3]<<16) | (buffer[4]<<24));
                for (i=5; i<obj_status-1; i++) // don't do the last byte - it will always be padding
                {
                    set_byte_inc( dest, buffer[i] );
                }
                break;
            }
            case obj_type_regs:
                for (i=0; (i<obj_status)&&(i<sizeof(regs)); i++)
                {
                    ((unsigned char *)regs)[i] = buffer[i+1];
                }
                have_regs = 1;
                break;
            case obj_type_end:
                done = 1;
                break;
            default:
                error = 1;
                break;
            }
        }
        else // check its a data block, set up, and start copying until we are done!
        {
            if ((obj_status==-1) && (buffer[0]==obj_type_data))
            {
                unsigned char *dest;
                dest = (unsigned char *)(buffer[1] | (buffer[2]<<8) | (buffer[3]<<16) | (buffer[4]<<24));
                i = 5;
                while (1) // copy block in hand and get next block
                {
                    int size;
                    size = obj_status-1; // get size of source to copy from (inc start lump which we are skipping); this is the size of data read from flash (but don't do the last padding byte)
                    if (obj_status<0) size=sizeof(buffer);
                    for (; i<size; i++)
                    {
                        set_byte_inc( dest, buffer[i] );
                    }
                    i = 0; // start at beginning of next block for source data (if there is one)
                    if (obj_status>0) // if we have the end of it
                    {
                        break;
                    }
                    obj_status = flash_read_object( address, &csum, buffer, &offset, sizeof( buffer ));
                    if (obj_status==0)
                    {
                        error=1;
                        break;
                    }
                }
                address += offset+4;
            }
            else
            {
                error = 1;
                have_regs = 0;
            }
        }
    }
    if (error)
    {
        uart_tx_string("Error ");
        uart_tx_hex8( address );
        uart_tx_nl();
        return 0;
    }
    if (have_regs)
    {
        uart_tx_string("Execing ");
        uart_tx_hex8( regs[11] );
        uart_tx_nl();
        FLASH_EXEC( regs );
    }
    return 1;
}
