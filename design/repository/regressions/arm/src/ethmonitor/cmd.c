/*a Includes
 */
#include <stdlib.h> // for NULL
#include "../drivers/uart.h"
#include "ethernet.h"
#include "cmd.h"

/*a Defines
 */
#define HEX(a) (((a)>=10) ? (((a)-10)+'a') : ((a)+'0'))

/*a Types
 */
typedef struct t_cmd_result
{
    void *handle;
    char *data;
    int space_remaining;
} t_cmd_result;

/*a Static variables
 */
/*v cmds
 */
static const t_command base_cmds[] =
{
    {(const char *)0, (t_command_fn *)0},
};

static t_command_chain cmd_chain;

/*a Result functions
 */
extern void cmd_result_string( void *handle, const char *text )
{
    if (handle)
    {
        t_cmd_result *result = (t_cmd_result *)handle;
        int i;
        for (i=0; text[i] && (result->space_remaining>0); i++)
        {
            result->data[0] = text[i];
            result->data++;
            result->space_remaining--;
        }
    }
    else
    {
        uart_tx_string( text );
    }
}

extern void cmd_result_hex2( void *handle, unsigned int a )
{
    if (handle)
    {
        t_cmd_result *result = (t_cmd_result *)handle;
        int c;
        int i;
        for (i=0; (i<2) && (result->space_remaining>0); i++)
        {
            c = (a>>4)&0xf;
            result->data[0] = HEX(c);
            result->data++;
            result->space_remaining--;
            a <<= 4;
        }
    }
    else
    {
        uart_tx(HEX((a>>4)&0xf));
        uart_tx(HEX((a>>0)&0xf));
    }
}

extern void cmd_result_hex8( void *handle, unsigned int a )
{
    if (handle)
    {
        t_cmd_result *result = (t_cmd_result *)handle;
        int c;
        int i;
        for (i=0; (i<8) && (result->space_remaining>0); i++)
        {
            c = (a>>28)&0xf;
            result->data[0] = HEX(c);
            result->data++;
            result->space_remaining--;
            a <<= 4;
        }
    }
    else
    {
        uart_tx_hex8( a );
    }
}

extern void cmd_result_string_nl( void *handle, const char *string )
{
    cmd_result_string( handle, string );
    cmd_result_nl(handle);
}

extern void cmd_result_nl( void *handle )
{
    if (handle)
    {
        t_cmd_result *result = (t_cmd_result *)handle;
        if (result->space_remaining>0)
        {
            result->data[0] = '\n';
            result->data++;
            result->space_remaining--;
        }
    }
    else
    {
        uart_tx_nl();
    }
}

static void cmd_result_done( void *handle )
{
    if (handle)
    {
        t_cmd_result *result = (t_cmd_result *)handle;
        if (result->space_remaining==0)
            result->data--;
        result->data[0] = 0;
        mon_ethernet_cmd_done(result->handle, result->space_remaining);
    }
}

/*a Init and obey functions
 */
extern void cmd_init( void )
{
    cmd_chain.cmds = base_cmds;
    cmd_chain.next = (t_command_chain *)0;
    chain_extra_cmds( &cmd_chain );
}

extern void cmd_obey( void *handle, char *buffer, int length, int max_result_length )
{
#define MAX_ARGS (8)
    int i, j, k, l;
    t_command *cmd;
    int argc;
    unsigned int args[MAX_ARGS];
    t_command_chain *chain;
    int error;
    t_cmd_result result_buffer;

    result_buffer.handle = handle;
    result_buffer.space_remaining = max_result_length;
    result_buffer.data = buffer;

    if (handle)
    {
        handle = (void *)&result_buffer;
    }

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
        while ((buffer[i]>=' ') && (argc<MAX_ARGS))
        {
            for (; (buffer[i]>' '); i++); // skip to white space
            for (; (buffer[i]==' '); i++); // skip past white space
            if (buffer[i]>' ')
            {
                args[argc] = 0;
                while (buffer[i]>' ')
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
        error = cmd->fn( handle, argc, args );
    }
    if (error)
    {
        cmd_result_string( handle, "Error" );
    }
    cmd_result_done( handle );

}
