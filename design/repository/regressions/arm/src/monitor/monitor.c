/*a Includes
 */
#include "../common/wrapper.h"
#ifndef LINUX
#include "uart.h"
#endif

#ifdef LINUX
#include <stdlib.h>
#include <stdio.h>
#define uart_rx_poll(a) (1)
#define uart_rx_byte() (getchar())
#define uart_tx(a) {putc(a,stderr);}
#define uart_tx_hex8(a) {fprintf(stderr,"%08x", a);}
#define test_entry_point() main()
#endif

/*a Defines
 */
#define MAX_ARGS (8)

/*a Types
 */
typedef int t_command_fn( int argc, unsigned int *args );
typedef struct
{
    const char *name;
    t_command_fn *fn;
} t_command;

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
            uart_tx_hex8( ptr );
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

static const t_command cmds[] =
{
    {"r", command_read_location},
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
    int command;
    int argc;
    unsigned int args[MAX_ARGS];

    __asm__ volatile (" mov r8, %0, lsl #8 \n orr r8, r8, #0x96 \n orr r8, r8, #0x16000000 \n .word 0xec00c40e \n mov r8, r8" :  : "r" ('*') : "r8" ); // extrdrm (c): cddm rd is (3.5 type 2 reg 0) (m is top 3.1 - type of 7 for no override)
    __asm__ volatile (" mov r8, %0, lsl #8 \n orr r8, r8, #0x96 \n orr r8, r8, #0x16000000 \n .word 0xec00c40e \n mov r8, r8" :  : "r" (10) : "r8" ); // extrdrm (c): cddm rd is (3.5 type 2 reg 0) (m is top 3.1 - type of 7 for no override)

    uart_init();
    while (1)
    {
        /*b Display the prompt
         */
        uart_tx( 'a' );
//        uart_tx( 10 );
//        uart_tx( 'O' );
//        uart_tx( 'K' );
//        uart_tx( '>' );
//        uart_tx( ' ' );

        /*b Wait for incoming string, echoing each byte as it comes
         */
        length = 0;
        while (1)
        {
//        uart_tx( 'b' );
            if (uart_rx_poll())
            {
                char c;
                c = uart_rx_byte();
                if (c<32)
                {
                    uart_tx(10);
                    break;
                }
                if (c==127)
                {
                    if (length>0)
                    {
                        length--;
                        uart_tx(c);
                    }
                }
                if (length<sizeof(buffer)-1)
                {
                    buffer[length++] = c;
                    uart_tx(c);
                }
            }
        }
        buffer[length]=0;
        uart_tx( 'c' );

        /*b Parse the string
         */
        for (i=0; buffer[i]==' '; i++);
        for (j=0; j<(sizeof(cmds)/sizeof(t_command)); j++)
        {
//            fprintf(stderr,"Compare buffer %s with command %s\n", buffer, cmds[j].name );
            l = 1;
            for (k=0; cmds[j].name[k]; k++)
            {
                if (cmds[j].name[k] != buffer[i+k])
                {
                    l = 0;
                    break;
                }
            }
            if (l)
            {
                break;
            }
        }
        uart_tx( 'd' );
        command = -1;
        if (l)
        {
            command = j;
            argc=0;
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
        uart_tx( 'e' );
        error = 1;
        if (command>=0)
        {
//            fprintf(stderr,"Command %d\n", command);
            error = cmds[command].fn( argc, args );
        }
        if (error)
        {
            uart_tx( 10 );
            uart_tx( '?' );
        }
        uart_tx( 'f' );
        uart_tx( 10 );
        break;
    }
}
