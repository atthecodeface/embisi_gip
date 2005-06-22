/*a Includes
 */
#include "gip_support.h"
#include "../drivers/ethernet.h"
#include "ethernet.h"
#include "uart.h"
#include "../drivers/uart.h"
#include "cmd.h"
#include <stdlib.h> // for NULL

/*a Defines
 */
#define DELAY(a) {int i;for (i=0; i<a; i++);}

/*a Types
 */
/*t t_data_fn, t_data_init_fn
 */
typedef void (t_data_init_fn)(unsigned int *ptr, unsigned int size );
typedef unsigned int (t_data_fn)(void);

/*t t_data_type
 */
typedef struct t_data_type
{
    t_data_init_fn *init_fn;
    t_data_fn *data_fn;
} t_data_type;

/*a Static functions
 */
/*f command_test_eth
 totaltx size_each max_in_queue
 */
static int outstanding_tx;
static void tst_rx_callback( void *handle, t_eth_buffer *buffer, int rxed_byte_length )
{
    buffer->buffer_size = 2048;
    ethernet_add_rx_buffer( buffer );
}
static void tst_tx_callback( void *handle, t_eth_buffer *buffer )
{
    uart_tx_string("V");
    outstanding_tx--;
}
static int command_test_eth( void *handle, int argc, unsigned int *args )
{
    t_eth_buffer buffers[16];
    int i;

    if (argc<3)
        return 1;
    if (args[2]>16)
        return 1;

    ethernet_set_tx_callback( tst_tx_callback, NULL );
    ethernet_set_rx_callback( tst_rx_callback, NULL );

    outstanding_tx = 0;
    for (i=0; i<args[0]; )
    {
        if (outstanding_tx<args[2])
        {
            uart_tx_string("^");
            buffers[i&0xf].buffer_size = args[1];
            buffers[i&0xf].data = (char *)command_test_eth;
            ethernet_tx_buffer( &buffers[i&0xf] );
            outstanding_tx++;
            i++;
        }
        mon_uart_poll();
        mon_ethernet_poll();
    }
    while (outstanding_tx>0)
    {
        mon_uart_poll();
        mon_ethernet_poll();
    }
    uart_tx_string_nl("Done");
    return 0;
}

/*f command_timer_read
 */
static int command_timer_read( void *handle, int argc, unsigned int *args )
{
    unsigned int s;

    GIP_TIMER_READ_0(s);
    uart_tx_string("cntr: ");
    uart_tx_hex8(s);
    uart_tx_nl();

    GIP_TIMER_READ_1(s);
    uart_tx_string("val1: ");
    uart_tx_hex8(s);
    uart_tx_nl();

    GIP_TIMER_READ_2(s);
    uart_tx_string("val2: ");
    uart_tx_hex8(s);
    uart_tx_nl();

    GIP_TIMER_READ_3(s);
    uart_tx_string("val3: ");
    uart_tx_hex8(s);
    uart_tx_nl();

    return 0;
}

/*f command_timer_enable
 */
static int command_timer_enable( void *handle, int argc, unsigned int *args )
{
    if (argc<1)
        return 1;

    if (args[0])
    {
        GIP_TIMER_ENABLE();
    }
    else
    {
        GIP_TIMER_DISABLE();
    }
    return 0;
}

/*f command_timer_leds
  The order of operations is important, as the timer passed bit gets cleared when its write completes, and that then flows through the local semaphores which can take (!) 8 cycles to 'uneffect' our semaphore
 */
static void led_flash( void )
{
    __asm__ volatile ( " stmfd sp!, {r0} " ); //push r0
    __asm__ volatile ( " .word 0xec00ce05 ; mov r0, r8 " );  // r0 <= periph[24] (timer value)
    __asm__ volatile ( " .word 0xec00c59e ; add r0, r0, #0x40000 " ); // periph[25] = timer value+0x40000, and clears passed
    __asm__ volatile ( " .word 0xec00c80e ; mov r0, #0x01<<28 " ); // write to spec0 (sems to clr)
    __asm__ volatile ( " .word 0xec00c18e ; .word 0xec00d18e ; eor r8, r8, #1 ; .word 0xec00c521 ; mov r0, r8 " ); // set leds,r24 to r24^1
    __asm__ volatile ( " .word 0xec00ce08 ; mov r0, r0 " ); // r0 <= spec0 (read and clear semaphores)
    __asm__ volatile ( " ldmfd sp!, {r0} " ); // pop r0
    __asm__ volatile ( " .word 0xec007305 " ); // deschedule
}
static int command_timer_leds( void *handle, int argc, unsigned int *args )
{
    unsigned int s;

    if (argc<1)
        return 1;

    if (args[0])
    {
        GIP_TIMER_DISABLE();
        GIP_BLOCK_ALL();
        GIP_READ_AND_CLEAR_SEMAPHORES(s,0xf<<28);
        GIP_SET_THREAD(7,led_flash,0x11); // set thread 4 startup to be ARM, on semaphore 16 set, and the entry point
        GIP_SET_LOCAL_EVENTS_CFG( 0xf ); // set config so that timer 1 (event 0) -> high priority thread 7
        __asm__ volatile ( " .word 0xec00c18e ; mov r8, #0x00aa" ); // set r24 to bottom leds all off and driven
        GIP_READ_AND_SET_SEMAPHORES(s,0x1<<28);
        GIP_TIMER_ENABLE();
    }
    else
    {
        GIP_TIMER_DISABLE();
        GIP_BLOCK_ALL();
        GIP_READ_AND_CLEAR_SEMAPHORES(s,0xf<<28);
    }
    __asm__ volatile ( " mov r0, #0xab ; eor %0, r0, #1 " : : "r"(s) : "r0" );
    uart_tx_hex8(s);
    uart_tx_nl();
    return 0;
}

/*f command_leds_read_write
 */
static int command_leds_read_write( void *handle, int argc, unsigned int *args )
{
    if (argc<1)
    {
        unsigned int s;
        GIP_LED_OUTPUT_CFG_READ(s);
        uart_tx_hex8(s);
        uart_tx_nl();
        return 0;
    }
    GIP_LED_OUTPUT_CFG_WRITE( args[0] );
    return 0;
}

/*a External variables
 */
/*v monitor_cmds_test
 */
extern const t_command monitor_cmds_test[];
const t_command monitor_cmds_test[] =
{
    {"testeth", command_test_eth},
    {"tmrd", command_timer_read},
    {"tmen", command_timer_enable},
    {"tmld", command_timer_leds},
    {"ldrw", command_leds_read_write},
    {(const char *)0, (t_command_fn *)0},
};

