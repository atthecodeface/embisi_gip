/*a Includes
 */
#include "../common/wrapper.h"
#include "uart.h"

/*a Defines
 */
#define hex(a) (((a)>=10) ? (((a)-10)+'a') : ((a)+'0'))

/*a Static variables
 */
static int rx_rdy;
static char rx_byte;
static int tx_av;

/*a Rx functions
 */
extern int uart_init( void )
{
    rx_rdy = 0;
    tx_av = 1000;
}

extern int uart_rx_poll( void )
{
    int s;

//    uart_tx_hex8(rx_rdy);
//    uart_tx_hex8(&rx_rdy);
    if (rx_rdy)
        return 1;

//    uart_tx_hex8(tx_av);
    while (1)
    {
        // poll for status
        // read periph reg 1; test bit 0 for status available; if clear there is some; so would like to do an 'and rd, status, #1', but peripheral MUST be Rm...
        __asm__ volatile ( " .word 0xec00de04 \n mov %0, r1 \n and %0, %0, #1\n" : "=r" (s) ); // extrnrm (d): dnxm n/m is top 3.1, type 7 for no override
        if (s)
            return 0;

        // read status fifo, which is periph reg 0
        __asm__ volatile ( " .word 0xec00de04 \n mov %0, r0\n " : "=r" (s) ); // extrnrm (d): dnxm n/m is top 3.1, type 7 for no override

        // if it is an rx byte, data is [17] framing error [16] 1 [9;0] data
        if (s&0x10000)
        {
            rx_rdy = 1;
            rx_byte = s&0xff;
            uart_tx_hex8( s );
            return 1;
        }
        else
        {
        // if is is a tx byte ok, data is [16] 0
            tx_av++;
        }
    }
}

extern int uart_rx_byte( void )
{
    rx_rdy = 0;
    return rx_byte;
}

/*a Tx functions
 */
// Whenever we write to the UART we must set the configuration also
// We require 1 start bit, 8 data bits, 1 stop bit for tx and rx
// (also to send data on tx we need to set bit 7)
// This is 01_011_0 for tx at bits 0 thru 6 (0x00000096)
// And  01_011_0 for rx at bits 24 thru 30 (0x16000000)
extern void uart_tx(a)
{
//    if (tx_av==0)
//        return;
    __asm__ volatile (" mov r8, %0, lsl #8 \n orr r8, r8, #0x96 \n orr r8, r8, #0x16000000 \n .word 0xec00c40e \n mov r8, r8" :  : "r" (a) : "r8" ); // extrdrm (c): cddm rd is (3.5 type 2 reg 0) (m is top 3.1 - type of 7 for no override)
    delay_char();
    tx_av--;
}

extern void uart_tx_hex8( unsigned int a )
{
    int c;
    int i;
    for (i=0; i<8; i++)
    {
        c = (a>>28)&0xf;
        c = hex(c);
        uart_tx(c);
        a <<= 4;
    }
}
