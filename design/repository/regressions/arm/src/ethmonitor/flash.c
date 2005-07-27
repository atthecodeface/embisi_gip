/*a Includes
 */
#include <stdlib.h> // for NULL
#include "gip_support.h"
#include "../drivers/uart.h"
#include "../drivers/flash.h"
#include "flash.h"

/*a Defines
 */
#define set_byte_inc( d, c ) {*(d++) = c;}
#define FLASH_EXEC( v ) { __asm__ volatile (" mov r0, %0 \n mov lr, pc \n ldmia r0, {r0-r10,pc} \n movnv r0, r0" : : "r" (v) ); }

/*a Download
 */
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
                if (length<(int)(sizeof(buffer))-1)
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

        // commands coming down can be erase or write
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
/*f mon_flash_read_object
  Read an object from the flash address.
  If offset is 0, then csum should be cleared, and the object length read
  If offset is >0, then csum ought to be valid, and data read from address+*offset, up to total_length
  Return 0 for bad object, -1 for incomplete object, else size read for valid object
 */
extern int mon_flash_read_object( unsigned int address, unsigned int *csum, char *buffer, int *offset, int max_length, int verbose )
{
    int total_length;
    int i;
    unsigned int cs;
    int ending_block;
    unsigned char header[4];

    /*b Read total length and expected csum - header of object
     */
    if (!flash_read_buffer( address, header, 4 )) return 0;
    total_length = header[0] | (header[1]<<8);
    if (total_length&0xc000)
    {
        if (verbose)
        {
            uart_tx_string( "bad len " );
            uart_tx_hex8(total_length);
            uart_tx_nl();
        }
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
    if (!flash_read_buffer( address+4+*offset, buffer, total_length )) return 0;

    /*b Calculate checksum and update offset
     */
    if (*offset==0)
    {
        cs = 0;
    }
    else
    {
        cs = *csum;
    }
    for (i=0; i<total_length; i+=2)
    {
        cs += buffer[0] + (buffer[1]<<8);
        buffer+=2;
    }
    cs = (cs&0xffff)+(cs>>16);
    cs = ((cs >> 16) + cs)&0xffff;
    *csum = cs;
    *offset += total_length;

    /*b If last block of the object then check the checksum: return error(0) or total length of block
     */
    if (ending_block)
    {
        cs = header[2] | (header[3]<<8);
        if (*csum != cs)
        {
            if (verbose)
            {
                uart_tx_string( "csum exp " );
                uart_tx_hex8(*csum);
                uart_tx_nl();
            }
            return 0;
        }
        return total_length;
    }

    /*b The object continues; return -1, with updated checksum and offset
     */
    return -1;
}

/*f mon_flash_boot
  Read the flash from address, and perform a boot of that if possible
  Return 0 on error, 1 if booting done (returned from image or no execution in objects at address)
 */
#define DELAY(a) {int i;for (i=0; i<a; i++) __asm__ volatile("mov r0, r0");}
extern int mon_flash_boot( unsigned int address, int verbose, unsigned char *config, int config_size )
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
        obj_status = mon_flash_read_object( address, &csum, buffer, &offset, sizeof( buffer ), verbose );
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
                if (verbose)
                {
                    uart_tx_string_nl("Data blk ");
                }
                unsigned char *dest;
                dest = (unsigned char *)(buffer[1] | (buffer[2]<<8) | (buffer[3]<<16) | (buffer[4]<<24));
                for (i=5; i<obj_status-1; i++) // don't do the last byte - it will always be padding
                {
                    set_byte_inc( dest, buffer[i] );
                }
                break;
            }
            case obj_type_regs:
                if (verbose)
                {
                    uart_tx_string_nl("Regs ");
                }
                for (i=0; (i<obj_status)&&(i<(int)sizeof(regs)); i++)
                {
                    ((unsigned char *)regs)[i] = buffer[i+1];
                }
                have_regs = 1;
                break;
            case obj_type_cfg:
                if (verbose)
                {
                    uart_tx_string_nl("Cfg ");
                }
                for (i=0; (i<obj_status)&&(i<config_size); i++)
                {
                    ((unsigned char *)config)[i] = buffer[i+1];
                }
                break;
            case obj_type_dram_phase:
                if (verbose)
                {
                    uart_tx_string_nl("dram phase ");
                }
                GIP_LED_OUTPUT_CFG_WRITE(0);
                for (i=0; i<256; i++)
                {
                    GIP_LED_OUTPUT_CFG_WRITE(0x3000); // set bit 7 to direction (1 is inc), bit 6 to make it go; then clear bit 6
                    GIP_BLOCK_ALL();
                    GIP_LED_OUTPUT_CFG_WRITE(0); // Clear psen
                    GIP_BLOCK_ALL();
                    DELAY(1000);
                }
                for (i=0; i<buffer[1]; i++)
                {
                    GIP_LED_OUTPUT_CFG_WRITE(0xf000); // set bit 7 to direction (1 is inc), bit 6 to make it go; then clear bit 6
                    GIP_BLOCK_ALL();
                    GIP_LED_OUTPUT_CFG_WRITE(0xc000); // Clear psen
                    GIP_BLOCK_ALL();
                    DELAY(1000);
                }
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
                    obj_status = mon_flash_read_object( address, &csum, buffer, &offset, sizeof( buffer ), verbose );
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
