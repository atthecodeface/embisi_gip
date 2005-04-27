/*a Includes
 */
#include "../common/wrapper.h"

/*a Defines
 */
#define NOP { __asm__ volatile(" movnv r0, r0"); }
#define NOP_WRINT { NOP; NOP; NOP; }
static void nop_many( void ) { NOP; NOP; NOP; NOP;    NOP; NOP; NOP; NOP;    NOP; NOP; NOP; NOP;    NOP; NOP; NOP; NOP;    NOP; NOP; NOP; NOP;    NOP; NOP; NOP; NOP;    NOP; NOP; NOP; NOP;   NOP; NOP; NOP; NOP; }
#define NOP_WREXT { nop_many(); }
#define FLASH_CONFIG_READ(v) { __asm__ volatile (" .word 0xec00ce04 \n mov %0, r8" : "=r" (v) ); }
#define FLASH_CONFIG_WRITE(v) { __asm__ volatile (" .word 0xec00c48e \n mov r8, %0" : : "r" (v) ); NOP_WRINT; }
#define FLASH_ADDRESS_READ(v) { __asm__ volatile (" .word 0xec00ce04 \n mov %0, r10" : "=r" (v) ); }
#define FLASH_ADDRESS_WRITE(v) { __asm__ volatile (" .word 0xec00c4ae \n mov r8, %0" : : "r" (v) ); NOP_WRINT; }
#define FLASH_DATA_READ(v) { __asm__ volatile (" .word 0xec00ce04 \n mov %0, r9" : "=r" (v) ); }
#define FLASH_DATA_WRITE( v ) { __asm__ volatile (" .word 0xec00c49e \n mov r8, %0" : : "r" (v) ); NOP_WREXT; }

 // data == watermark (10;21), size_m_one (10;11), base (11;0)
 // cmd == fifo(2;29)=0, status_fifo(28)=0, ingress(27)=0, length_m_one(5;10)=0, io_dest_type(2;8)=1, route(7)=0, last(1)=0 -- configure tx data fifo
#define POSTBUS_IO_FIFO_CFG( data, rx, fifo, base, size_m_one, wm ) { \
    unsigned int s; \
    s = ((wm)<<21) | ((size_m_one)<<11) | ((base)<<0); \
    __asm__ volatile ( " .word 0xec00c62e \n mov r0, %0 \n" : : "r" (s) ); \
    s = ((fifo)<<29) | ((!(data))<<28) | ((rx)<<27) | (0<<10) | (1<<8); \
    __asm__ volatile ( " .word 0xec00c61e \n mov r0, %0 \n" : : "r" (s) ); \
}

/*a Ethernet handling functions
 */
/*f Transmit an ethernet packet
 */
static void tx_ethernet_packet( int byte_length, unsigned int *data )
{
    int i, j;
    unsigned int s;
    for (i=j=0; i<(byte_length+3)/4; i++)
    {
        __asm__ volatile ( " .word 0xec00c62e \n mov r0, %0 \n" : : "r" (data[i]) ); // postbus data 0
        j++;
        if (j==8)
        {
            // wait for txpending[0] to be clear?
            s = 0x00000200; // fifo(2;29)=0, status_fifo(28)=0, ingress(27)=0, length_m_one(5;10)=0, io_dest_type(2;8)=2, route(7)=0, last(1)=0 -- configure tx data fifo
            s |= ((j-1)<<10);
            __asm__ volatile ( " .word 0xec00c61e \n mov r0, %0 \n" : : "r" (s) ); // postbus cmd 0
            j = 0;
        }
    }
    if (j>0)
    {
        // wait for txpending[0] to be clear?
        s = 0x00000200; // fifo(2;29)=0, status_fifo(28)=0, ingress(27)=0, length_m_one(5;10)=0, io_dest_type(2;8)=2, route(7)=0, last(1)=0 -- configure tx data fifo
        s |= ((j-1)<<10);
        __asm__ volatile ( " .word 0xec00c61e \n mov r0, %0 \n" : : "r" (s) ); // postbus cmd 0
    }

    // wait for txpending[0] to be clear?
    s = 0x01000000; // cmd for eth tx - immediate
    __asm__ volatile ( " .word 0xec00c62e \n mov r0, %0 \n" : : "r" (s) ); // postbus data 0
    s = 0x21800000; // cmd for eth tx tx_fcs(29), half_duplex (28), deferral_restart_value (8;20), retry_count (4;16), length (16;0)
    s |= byte_length;
    __asm__ volatile ( " .word 0xec00c62e \n mov r0, %0 \n" : : "r" (s) ); // postbus data 0
    s = 0x10000600; // fifo(2;29)=0, status_fifo(28)=1, ingress(27)=0, length_m_one(5;10)=1, io_dest_type(2;8)=2, route(7)=0, last(1)=0 -- 
    __asm__ volatile ( " .word 0xec00c61e \n mov r0, %0 \n" : : "r" (s) ); // postsbus cmd 0
}

/*f Poll the rx status fifo
  return 1 if not empty
 */
static int poll_rx_status_fifo( void )
{
    unsigned int s;
    // clear the semaphore
    s = 1<<5;
    __asm__ volatile ( " .word 0xec00c82e \n mov r0, %0 \n" : : "r" (s) ); // extrdrm (c): dnxm n/m is top 3.1, type 7 for no override - special is sems 0, semsset 1, semsclr 2

    // send cmd to give us its status
    s = 0x28800400; // header for I/O to send back: route(7;1)=0, dest GIP FIFO (2;8)=0, I/O length(4;10)=1, I/O cmd (2;22)=2 (read status), signal (5;27)=5
    __asm__ volatile ( " .word 0xec00c62e \n mov r0, %0 \n" : : "r" (s) ); // extrnrm (d): dnxm n/m is top 3.1, type 7 for no override - tx fifo config
    s = 0x18000000; // fifo(2;29)=0, status_fifo(28)=1, ingress(27)=1, length_m_one(5;10)=0, io_dest_type(2;8)=0, route(7)=0, last(1)=0 -- do 'event cmd' command
    __asm__ volatile ( " .word 0xec00c61e \n mov r0, %0 \n" : : "r" (s) ); // extrnrm (d): dnxm n/m is top 3.1, type 7 for no override - tx fifo config

    // wait for the data to come back
    s = 0;
    while (!(s&0x20))
    {
        __asm__ volatile (" .word 0xec00ce08 \n mov %0, r0" : "=r" (s) );
    }

    // Read the postbus rx fifo to get its data word
    __asm__ volatile (" .word 0xec00ce06 \n mov %0, r1" : "=r" (s) );

    // Check the empty bit of the status
    return (s&0x80000000)==0;
}

/*f Handle a non-empty ethernet rx status fifo
  Packet length is bottom 16 bits
  Block length is 8;16
  Status is 3;24 : FCS ok (0), FCS bad (1), odd nybbles (2), block complete (3), overrun (4), framing error (5)
 */
static void handle_rx_status_fifo( void )
{
    unsigned int s;
    unsigned int time;
    // clear the semaphore
    s = 1<<5;
    __asm__ volatile ( " .word 0xec00c82e \n mov r0, %0 \n" : : "r" (s) ); // extrdrm (c): dnxm n/m is top 3.1, type 7 for no override - special is sems 0, semsset 1, semsclr 2

    // read 2 words from the status fifo
    s = 0x28000800; // header for I/O to send back: route(7;1)=0, dest GIP FIFO (2;8)=0, I/O length(4;10)=2, I/O cmd (2;22)=0 (read data), signal (5;27)=5
    __asm__ volatile ( " .word 0xec00c62e \n mov r0, %0 \n" : : "r" (s) ); // extrnrm (d): dnxm n/m is top 3.1, type 7 for no override - tx fifo config
    s = 0x18000000; // fifo(2;29)=0, status_fifo(28)=1, ingress(27)=1, length_m_one(5;10)=0, io_dest_type(2;8)=0, route(7)=0, last(1)=0 -- do 'event cmd' command
    __asm__ volatile ( " .word 0xec00c61e \n mov r0, %0 \n" : : "r" (s) ); // extrnrm (d): dnxm n/m is top 3.1, type 7 for no override - tx fifo config

    // wait for the data to come back
    s = 0;
    while (!(s&0x20))
    {
        __asm__ volatile (" .word 0xec00ce08 \n mov %0, r0" : "=r" (s) );
    }
    
    // read the status and the time
    __asm__ volatile (" .word 0xec00ce06 \n mov %0, r1" : "=r" (time) );
    __asm__ volatile (" .word 0xec00ce06 \n mov %0, r1" : "=r" (s) );
    s = (s>>16)&3;
    if (s==0) {} // tx ok
    if (s==1) {} // late coll
    if (s==2) {} // retries exceeded

}

/*f Poll the tx status fifo
  return 1 if not empty
 */
static int poll_tx_status_fifo( void )
{
    unsigned int s;
    // clear the semaphore
    s = 1<<5;
    __asm__ volatile ( " .word 0xec00c82e \n mov r0, %0 \n" : : "r" (s) ); // extrdrm (c): dnxm n/m is top 3.1, type 7 for no override - special is sems 0, semsset 1, semsclr 2

    // send cmd to give us its status
    s = 0x28800400; // header for I/O to send back: route(7;1)=0, dest GIP FIFO (2;8)=0, I/O length(4;10)=1, I/O cmd (2;22)=2 (read status), signal (5;27)=5
    __asm__ volatile ( " .word 0xec00c62e \n mov r0, %0 \n" : : "r" (s) ); // extrnrm (d): dnxm n/m is top 3.1, type 7 for no override - tx fifo config
    s = 0x38000000; // fifo(2;29)=1, status_fifo(28)=1, ingress(27)=1, length_m_one(5;10)=0, io_dest_type(2;8)=0, route(7)=0, last(1)=0 -- do 'event cmd' command
    __asm__ volatile ( " .word 0xec00c61e \n mov r0, %0 \n" : : "r" (s) ); // extrnrm (d): dnxm n/m is top 3.1, type 7 for no override - tx fifo config

    // wait for the data to come back
    s = 0;
    while (!(s&0x20))
    {
        __asm__ volatile (" .word 0xec00ce08 \n mov %0, r0" : "=r" (s) );
    }

    // Read the postbus rx fifo to get its data word
    __asm__ volatile (" .word 0xec00ce06 \n mov %0, r1" : "=r" (s) );

    // Check the empty bit of the status
    return (s&0x80000000)==0;
}

/*f Handle a non-empty ethernet tx fifo
  Length is bottom 16 bits
  Status is 2;16
 */
static void handle_tx_status_fifo( void )
{
    unsigned int s;
    unsigned int time;
    // clear the semaphore
    s = 1<<5;
    __asm__ volatile ( " .word 0xec00c82e \n mov r0, %0 \n" : : "r" (s) ); // extrdrm (c): dnxm n/m is top 3.1, type 7 for no override - special is sems 0, semsset 1, semsclr 2

    // read 2 words from the status fifo
    s = 0x28000800; // header for I/O to send back: route(7;1)=0, dest GIP FIFO (2;8)=0, I/O length(4;10)=2, I/O cmd (2;22)=0 (read data), signal (5;27)=5
    __asm__ volatile ( " .word 0xec00c62e \n mov r0, %0 \n" : : "r" (s) ); // extrnrm (d): dnxm n/m is top 3.1, type 7 for no override - tx fifo config
    s = 0x38000000; // fifo(2;29)=1, status_fifo(28)=1, ingress(27)=1, length_m_one(5;10)=0, io_dest_type(2;8)=0, route(7)=0, last(1)=0 -- do 'event cmd' command
    __asm__ volatile ( " .word 0xec00c61e \n mov r0, %0 \n" : : "r" (s) ); // extrnrm (d): dnxm n/m is top 3.1, type 7 for no override - tx fifo config

    // wait for the data to come back
    s = 0;
    while (!(s&0x20))
    {
        __asm__ volatile (" .word 0xec00ce08 \n mov %0, r0" : "=r" (s) );
    }
    
    // read the status and the time
    __asm__ volatile (" .word 0xec00ce06 \n mov %0, r1" : "=r" (time) );
    __asm__ volatile (" .word 0xec00ce06 \n mov %0, r1" : "=r" (s) );
    s = (s>>16)&3;
    if (s==0) {} // tx ok
    if (s==1) {} // late coll
    if (s==2) {} // retries exceeded

}

/*a Test entry point
 */
/*f test_entry_point
The postbus interface has 4 status, 4 cmd, 4 fifo, 4 rx config, 4 tx config; 0,1,2,3 is status, command, tx fifo, rx fifo; 16/17 is rx/tx config. Add 4 for the fifo 1, 8 for fifo 2, 12 for fifo 3

constant integer postbus_command_last_bit = 0; // bit 0 of first transaction word indicates its also the last
constant integer postbus_command_route_start = 1; // bits 1-7 of first transaction word indicates the destination (routing information) for the transaction
constant integer postbus_command_rx_fifo_start = 8; // bit 8-9 of first transaction word indicates the receive FIFO to receive into
constant integer postbus_command_io_dest_type_start = 8; // bit 8-9 of first transaction word indicates the type of destination - 00=>channel, 01=>data FIFO, 10=>command FIFO, 11=>IO interface config
constant integer postbus_command_tx_length_start = 10; // bits 10-14 of first transaction word indicates the length (to GIP)
constant integer postbus_command_flow_type_start = 15; // bit 15-16 of first transaction word indicates the flow control action to do on receive (00=>write, 01=>set, 10=>add)
constant integer postbus_command_flow_value_start = 17; // bit 17-20 of first transaction word indicates the data for the flow control action
constant integer postbus_command_spare = 21; // bit 21 of first transaction word spare
constant integer postbus_command_tx_signal_start = 22; // bit 22-26 of command (first transcation word out) indicates (if nonzero) the semaphore to set on completion of push to postbus
constant integer postbus_command_rx_signal_start = 27; // bit 27-31 of first transaction word indicates (if nonzero) the semaphore to set on completion of push postbus
constant integer postbus_command_io_dest_start = 27; // bit 27-31 of first transaction word indicates the channel or data/command FIFO an IO transaction word is for

 */
extern int test_entry_point()
{
    unsigned int s;
    int empty;
    FLASH_CONFIG_WRITE( 0x355 );
    FLASH_ADDRESS_WRITE( 0x80000000 ); // CE 2
//    __asm__ volatile ( " .word 0xec00de06 \n mov %0, r0 \n" : "=r" (s) ); // extrnrm (d): dnxm n/m is top 3.1, type 7 for no override
    s = 0x08081008; // base 8, end 16, read 8, write 8
    __asm__ volatile ( " .word 0xec00c70e \n mov r0, %0 \n" : : "r" (s) ); // extrnrm (d): dnxm n/m is top 3.1, type 7 for no override - tx fifo config

    s = 0x00000800; // base 0, end 8, read 0, write 0
    __asm__ volatile ( " .word 0xec00c71e \n mov r0, %0 \n" : : "r" (s) ); // extrnrm (d): dnxm n/m is top 3.1, type 7 for no override - tx fifo config

    NOP;
    NOP;
    NOP;
    NOP;

    __asm__ volatile ( " .word 0xec00de07 \n mov %0, r1 \n" : "=r" (s) ); // extrnrm (d): dnxm n/m is top 3.1, type 7 for no override - tx fifo config
    FLASH_DATA_WRITE( s );

    __asm__ volatile ( " .word 0xec00de07 \n mov %0, r0 \n" : "=r" (s) ); // extrnrm (d): dnxm n/m is top 3.1, type 7 for no override - rx fifo config
    FLASH_DATA_WRITE( s );

    NOP;
    NOP;
    NOP;
    NOP;

    POSTBUS_IO_FIFO_CFG( 1, 0, 0,     0, 511, 3 ); // etx d data, rx, fifo #,  base, size-1, wm
    POSTBUS_IO_FIFO_CFG( 0, 0, 0,  512,  15, 3 ); // etx c data, rx, fifo #,  base, size-1, wm
    POSTBUS_IO_FIFO_CFG( 1, 1, 0,     0, 511, 3 ); // erx d data, rx, fifo #,  base, size-1, wm
    POSTBUS_IO_FIFO_CFG( 0, 1, 0,  512,  15, 3 ); // erx s data, rx, fifo #,  base, size-1, wm
    POSTBUS_IO_FIFO_CFG( 0, 1, 1,  528,  15, 3 ); // etx s data, rx, fifo #,  base, size-1, wm

// invoke the postbus target to send us a header back - test the path, really
//    s = 0x18000000; // header for I/O to send back: route(7;1)=0, dest GIP FIFO (2;8)=0, I/O length(4;10)=0, I/O cmd (2;22)=0 (send hdr), signal (5;27)=3
//    __asm__ volatile ( " .word 0xec00c62e \n mov r0, %0 \n" : : "r" (s) ); // extrnrm (d): dnxm n/m is top 3.1, type 7 for no override - tx fifo config
//    s = 0x38000000; // fifo(2;29)=1, status_fifo(28)=1, ingress(27)=1, length_m_one(5;10)=0, io_dest_type(2;8)=0, route(7)=0, last(1)=0 -- do 'send header' command
//    __asm__ volatile ( " .word 0xec00c61e \n mov r0, %0 \n" : : "r" (s) ); // extrnrm (d): dnxm n/m is top 3.1, type 7 for no override - tx fifo config

//    for (s=0; s<30; s++) NOP; // 650ns per loop


    tx_ethernet_packet( 16, (unsigned int *)"this is the day that the Lord has made" );

    while (1)
    {
        // Check tx status fifo empty
        if (poll_tx_status_fifo())
        {
            handle_tx_status_fifo();
        }
        if (poll_rx_status_fifo())
        {
            handle_rx_status_fifo();
        }
    }

}

// status 0 => erx
// status 1 => etx
// status 2 => uart 0
// cmd 0 => etx
// cmd 1 => uart 0
// txdata 0 => etx
// rxdata 0 => erx
