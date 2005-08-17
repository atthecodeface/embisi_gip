/*a Includes
 */
#include "../common/wrapper.h"
#include "uart.h"
#include "flash.h"
#include "monitor.h"

/*a Defines
 */
#define MAX_ARGS (8)

/*a Static functions
 */
/*f command_read_location
 */
static int command_read_location( int argc, unsigned int *args )
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
                uart_tx_nl();
            }
            uart_tx_hex8( (unsigned int)(ptr+i*sizeof(int)) );
            uart_tx(':');
        }
        else
        {
            uart_tx(' ');
        }
        v = ptr[i];
        uart_tx_hex8(v);
        j++;
        if (j==8) j=0;
    }
    return 0;
}

/*f command_write_location
 */
static int command_write_location( int argc, unsigned int *args )
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

/*f command_ext_bus_config
 */
static int command_ext_bus_config( int argc, unsigned int *args )
{
    unsigned int v;

    if (argc<1)
    {
//        __asm__ volatile (" .word 0xec00ce04 \n mov %0, r8" :  "=r" (v) ); // extrdrm none, periph.8 (8 is ext bus config, 9 is ext bus data, 10 is ext bus address)
        FLASH_CONFIG_READ(v);
        uart_tx_hex8(v);
        uart_tx_nl();
    }
    else
    {
        v = args[0];
        FLASH_CONFIG_WRITE(v);
//        __asm__ volatile (" .word 0xec00c48e \n mov r8, %0" :  : "r" (v) ); // extrdrm periph.8, none (8 is ext bus config, 9 is ext bus data, 10 is ext bus address)
    }
    return 0;
}

/*f command_ext_bus_address
 */
static int command_ext_bus_address( int argc, unsigned int *args )
{
    unsigned int v;

    if (argc<1)
    {
        FLASH_ADDRESS_READ(v);
        uart_tx_hex8(v);
        uart_tx_nl();
    }
    else
    {
        v = args[0];
        FLASH_ADDRESS_WRITE(v);
    }
    return 0;
}

/*f command_ext_bus_data
 */
static int command_ext_bus_data( int argc, unsigned int *args )
{
    unsigned int v;

    if (argc<1)
    {
        FLASH_DATA_READ(v);
        uart_tx_hex8(v);
        uart_tx_nl();
    }
    else
    {
        v = args[0];
        FLASH_DATA_WRITE(v);
    }
    return 0;
}

/*f command_flash_erase
 */
static int command_flash_erase( int argc, unsigned int *args )
{
    if (argc<1) return 1;
    return !flash_erase_block( args[0]<<17 );
}

/*f command_flash_boot
 */
static int command_flash_boot( int argc, unsigned int *args )
{
    if (argc<1)
    {
        int i, offset;
        unsigned int csum;
        char buffer[256];
        for (i=0; i<16; i++)
        {
            offset = 0;
            if ( (flash_read_object( i<<17, &csum, buffer, &offset, sizeof(buffer) )>0) &&
                 (buffer[0]==obj_type_description) )
            {
                uart_tx_hex8( i );
                uart_tx_string(" : ");
                uart_tx_string_nl(buffer+1);
            }
        }
        return 0;
    }
    return !flash_boot( args[0]<<17 );
}

/*f command_flash_download
 */
static int command_flash_download( int argc, unsigned int *args )
{
    if (argc!=0)
    {
        return 1;
    }
    flash_download();
    return 0;
}

/*a Static variables
 */
/*v cmds
 */
static const t_command monitor_cmds[] =
{
    {"mr", command_read_location},
    {"mw", command_write_location},
    {"ebc", command_ext_bus_config},
    {"eba", command_ext_bus_address},
    {"ebd", command_ext_bus_data},
    {"fe", command_flash_erase},
    {"fb", command_flash_boot},
    {"fd", command_flash_download},
    {(const char *)0, (t_command_fn *)0},
};

/*a Test entry point
 */
/*f test_entry_point
 */
extern int test_entry_point()
{
    int i, j, k, l;
    int error;
    int length;
    char buffer[256];
    const t_command *cmd;
    t_command_chain cmd_chain, *chain;
    int argc;
    unsigned int args[MAX_ARGS];
 
#ifdef LINUX
    static int heap[65536];
    fprintf(stderr,"256kB heap at %p\n",heap);
#endif

    cmd_chain.cmds = monitor_cmds;
    cmd_chain.next = (t_command_chain *)0;
    chain_extra_cmds( &cmd_chain );
    uart_init();
    extra_init();
    while (1)
    {
        /*b Display the prompt
         */
        uart_tx_string( "\r\nOK > ");

        /*b Wait for incoming string, echoing each byte as it comes
         */
        length = 0;
        while (1)
        {
            if (uart_rx_poll())
            {
                char c;
                c = uart_rx_byte();
                if (c<32)
                {
                    uart_tx_nl();
                    break;
                }
                if (c==8)
                {
                    if (length>0)
                    {
                        length--;
                        uart_tx(c);
                    }
                }
                if (length<(int)(sizeof(buffer))-1)
                {
                    buffer[length++] = c;
                    uart_tx(c);
                }
            }
        }
        buffer[length]=0;

        /*b Parse the string
         */
        for (i=0; buffer[i]==' '; i++);
        cmd = (t_command *)0;
        for (chain = &cmd_chain; (!cmd) && chain; chain=chain->next)
        {
            for (j=0; (!cmd) && (chain->cmds[j].name); j++)
            {
                //fprintf(stderr,"Compare buffer %s with command %s\n", buffer, chain->cmds[j].name );
                l = 1;
                for (k=0; chain->cmds[j].name[k]; k++)
                {
                    if (chain->cmds[j].name[k] != buffer[i+k])
                    {
                        l = 0;
                        break;
                    }
                }
                if (l)
                {
                    cmd = &(chain->cmds[j]);
                    break;
                }
            }
        }
        argc=0;
        if (cmd)
        {
            while (buffer[i] && (argc<MAX_ARGS))
            {
                for (; (buffer[i]!=' ') && (buffer[i]); i++); // skip to white space
                for (; (buffer[i]==' ') && (buffer[i]); i++); // skip past white space
                if (buffer[i])
                {
                    args[argc] = 0;
                    while ((buffer[i]!=' ') && (buffer[i]))
                    {
                        args[argc] = (args[argc]<<4) | ((buffer[i]>'9') ? ((buffer[i]&0xf)+9) : (buffer[i]&0xf));
                        i++;
                    }
                    argc++;
                }
            }
        }

        /*b Obey the string
         */
        error = 1;
        if (cmd)
        {
//            fprintf(stderr,"Command %d\n", command);
            error = cmd->fn( argc, args );
        }
        if (error)
        {
            uart_tx_string_nl( "\n\rError" );
        }
    }
}
