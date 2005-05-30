/*a Includes
 */
#include "../common/wrapper.h"
#include "../drivers/uart.h"
#include "cmd.h"
#include <stdlib.h> // for NULL

/*a Defines
 */

/*a Types
 */
typedef struct t_uart_data
{
    int state;
    int length;
    char buffer[256];
} t_uart_data;

enum
{
    fsm_pre_prompt,
    fsm_get_characters,
    fsm_handle_command
};

/*a Static variables
 */
static t_uart_data uart;

/*a External init and polling functions
 */
/*f mon_uart_init
 */
extern void mon_uart_init( void )
{
    uart_init();
    uart.state = fsm_pre_prompt;
}

/*f mon_uart_poll
 */
extern void mon_uart_poll( void )
{
    /*b Display the prompt if necessary
     */
    if (uart.state == fsm_pre_prompt)
    {
        uart_tx_string( "\r\nOK > ");
        uart.state = fsm_get_characters;
        uart.length = 0;
    }

    /*b Check for incoming character, echoing bytes and adding to our string
     */
    if (uart.state==fsm_get_characters)
    {
        if (uart_rx_poll())
        {
            char c;
            c = uart_rx_byte();
            if (c<32)
            {
                uart_tx_nl();
                uart.buffer[uart.length]=0;
                uart.state = fsm_handle_command;
            }
            if (c==8)
            {
                if (uart.length>0)
                {
                    uart.length--;
                    uart_tx(c);
                }
            }
            if (uart.length<(int)(sizeof(uart.buffer))-1)
            {
                uart.buffer[uart.length++] = c;
                uart_tx(c);
            }
        }
    }

    /*b Parse the string if we have one
     */
    if (uart.state==fsm_handle_command)
    {
        uart.state = fsm_pre_prompt;
        cmd_obey( NULL, uart.buffer, uart.length, -1 );
    }
}

