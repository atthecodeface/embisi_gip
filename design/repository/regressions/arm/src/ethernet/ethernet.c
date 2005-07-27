/*a Includes
 */
#include <stdlib.h> // for NULL
#include "gip_support.h"
#include "postbus.h"
#include "../common/wrapper.h"
#include "../drivers/ethernet.h"

/*a Defines
 */

/*a Ethernet handling functions
 */

/*a Test entry point
 */
static t_eth_buffer buffers[16];

/*f test_entry_point
 */
#define DRAM_START ((void *)(0x80010000))
#define BUFFER_SIZE (2048)
static void tx_callback( void *handle, t_eth_buffer *buffer )
{
    buffer->buffer_size = BUFFER_SIZE;
    ethernet_add_rx_buffer( buffer );
}
static void rx_callback( void *handle, t_eth_buffer *buffer, int rxed_byte_length )
{
    FLASH_CONFIG_WRITE(0xffff);
    FLASH_ADDRESS_WRITE(buffer);
    FLASH_DATA_WRITE(rxed_byte_length);
    buffer->buffer_size = rxed_byte_length;
    ethernet_tx_buffer( buffer );
}
#define write_timer(t,s) {__asm__ volatile ( " .word 0xec00c58e+" #t "<<4 \n mov r0, %0 \n" : : "r" (s) ); NOP; NOP; }
#define read_timer0(s) {__asm__ volatile ( " .word 0xec00ce05 \n mov %0, r8 \n " : "=r" (s) ); }
#define read_timer1(s) {__asm__ volatile ( " .word 0xec00ce05 \n mov %0, r9 \n " : "=r" (s) ); }
#define read_timer2(s) {__asm__ volatile ( " .word 0xec00ce05 \n mov %0, r10 \n " : "=r" (s) ); }
#define read_timer3(s) {__asm__ volatile ( " .word 0xec00ce05 \n mov %0, r11 \n " : "=r" (s) ); }
#define enable_timer() {__asm__ volatile ( " .word 0xec00c58e \n mov r0, #0<<31 \n" ); NOP; NOP; }
#define SET_LOCAL_EVENTS_CFG(s) {__asm__ volatile ( " .word 0xec00c82e \n mov r0, %0 \n" : : "r" (s) ); NOP; NOP; }

#define TIMER_1_THREAD (4)
static int timer_1_delay;
static int timer_1_reg_store[12];
static void timer_1_entry( void )
{
    // preserve r0 in r20
    __asm__ volatile( " .word 0xec00c14e \n mov r0, r0 \n " );
    // preserve r1-r12 in memory
    __asm__ volatile( " ldr r0, =timer_1_reg_store \n stmia r0, {r1-r12} \n" );

    {
        unsigned int t, s;
        // read timer value
        GIP_TIMER_READ_0( t );
        // set timer to new value (old+x)
        t += timer_1_delay;
        GIP_TIMER_WRITE( 1, t );
        // get led 0 ; this section should be atomic, but its only an LED!
        GIP_LED_OUTPUT_CFG_READ( s );
        // clear semaphore - cannot do it before, else the timer will still be driving the semaphore; of course if we are interrupted, we could be too late here.
        GIP_READ_AND_CLEAR_SEMAPHORES( s, 1<<(TIMER_1_THREAD*4+0) );
        // toggle bit 0 value
        s ^= 1;
        // write ld 0
        GIP_LED_OUTPUT_CFG_WRITE( s );
    }
    // recover r1-r12 in memory
    __asm__ volatile( " ldr r0, =timer_1_reg_store \n ldmia r0, {r1-r12} \n" );
    // recover r0, from r20
    __asm__ volatile( " .word 0xec00ce01 \n mov r0, r4 \n " );
    // deschedule
    GIP_DESCHEDULE();
    NOP;NOP;NOP;
}
static void freeze( void )
{
    while (1);
}
extern int test_entry_point()
{
    int i;

    if (0)
    {
        unsigned int s;
        FLASH_CONFIG_WRITE(0x000);
        FLASH_ADDRESS_WRITE(0);
        read_timer0(s);
        FLASH_DATA_WRITE(s);
        read_timer1(s);
        FLASH_DATA_WRITE(s);
        enable_timer();
        read_timer0(s);
        FLASH_DATA_WRITE(s);
        read_timer1(s);
        FLASH_DATA_WRITE(s);
        read_timer2(s);
        FLASH_DATA_WRITE(s);
        read_timer3(s);
        FLASH_DATA_WRITE(s);
        write_timer(1,0x12345678)
        read_timer1(s);
        FLASH_DATA_WRITE(s);
        read_timer0(s);
        FLASH_DATA_WRITE(s);
        write_timer(2,s+64);
        read_timer2(s);
        FLASH_DATA_WRITE(s);
        //timer2 is local event 1
        SET_LOCAL_EVENTS_CFG(0xf<<(1*4));
    }

    ethernet_init();
    ethernet_set_tx_callback( tx_callback, NULL );
    ethernet_set_rx_callback( rx_callback, NULL );
    for (i=0; i<8; i++)
    {
        buffers[i].data = DRAM_START+BUFFER_SIZE*i;
        buffers[i].buffer_size = BUFFER_SIZE;
        ethernet_add_rx_buffer( &buffers[i] );
    }
    while (1)
    {
        ethernet_poll();
    }

}
