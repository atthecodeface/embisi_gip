/* gipeth.c: a GIP net driver

*/

//#define USE_CIRC_BUFFERS

#ifdef REGRESSION
#include <stdlib.h>
#include "../microkernel/microkernel.h"
#include "uart.h"
#include <gip_support.h>
#include <gip_system.h>
#include <postbus.h>
#include "sync_serial.h"
#else


#include <linux/config.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/linkage.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>
#include <linux/init.h>

#include <asm/microkernel.h>
#include <asm/gip_support.h>
#include <asm/gip_system.h>
#include <asm/postbus.h>
#include "sync_serial.h"

#endif

/*a Defines
 */
#define GIPETH_BUFFER_LENGTH (2048)
#define GIPETH_BUFFER_LENGTH_INC_TIME (GIPETH_BUFFER_LENGTH+4)
#define NUM_TX_BUFFERS (4)
#define NUM_RX_BUFFERS (4)
#define CIRC_BUFFER_BASE   (0x80200000)
#define CIRC_BUFFER_BASE_2 (0x80202000)

/*a Types
 */
/*t t_gipeth_buffer - this must match the assembler version
 */
typedef struct t_gipeth_buffer
{
    struct t_gipeth_buffer *next_in_list;
    unsigned int *data; // pointer to data start in the buffer - there needs to be one word before this pointer for the time the first block of data was received
    int length; // length in bytes - updated on receive by hardware thread, set on transmit by client
    int ready; // for rx indicates buffer is full and length set, client to read data and set done
    // for tx indicates buffer is full and length set, driver to transmit and set done
    int done; // for rx indicates buffer is available for driver to fill, client to clear upon handling of ready buffer
    // for tx indicates driver has transmitted
} t_gipeth_buffer;

/*t t_gipeth
 */
typedef struct t_gipeth
{
    t_gipeth_buffer *rx_next_buffer_to_reuse; // owned by hardware thread, indicates next buffer to receive into - not used by client
    unsigned int *rx_buffer_in_hand;    // owned by the hardware thread, buffer being received into (NULL if none)
    int rx_length_so_far;               // owned by the hardware thread, length received so far in buffer
    t_gipeth_buffer *tx_next_buffer_to_tx; // owned by the hardware thread, indicates next buffer to transmit from (if ready) - not used by client
    unsigned int *tx_buffer_in_hand;       // owned by the hardware thread, next data to be transmitted
    int tx_length;                         // owned by the hardware thread, indicates amount of data at tx_buffer_in_hand

    unsigned int regs[16]; // place to store registers when in hardware thread

    // rest is not accessible from hardware thread

    t_gipeth_buffer *rx_buffers; // circular linked chain of rx buffers
    t_gipeth_buffer *tx_buffers; // circular linked chain of tx buffers

    t_gipeth_buffer *rx_next_buffer_rxed; // owned by client, indicates next buffer to check for 'ready' in - not used by driver
    t_gipeth_buffer *tx_next_buffer_done; // owned by client, indicates next buffer to check for 'done' in - not used by driver

} t_gipeth;

/*a External functions - in asm version
 */
extern void gipeth_isr_asm( void );
extern void gipeth_hw_thread( void );

/*a Static variables
 */
static t_gipeth eth;
static struct net_device * dev1;

/*a Driver functions including hardware thread
 */
/*f postbus_config
  set postbus fifos up
  for the gip RF split with rx_0, tx_0, rx_1 and tx_1
  if tx_0<=0 then share it with rx_0
  ditto for tx_1
 */
static void postbus_config( int rx_0, int tx_0, int rx_1, int tx_1 )
{
    int n;

    // GIP postbus RF split as 16 for rx/tx fifo 0, 16 for rx/tx fifo 1 (which does not yet exist due to Xilinx tool issues)
    n = 0;
    GIP_POSTIF_CFG( 0, (n<<0) | ((n+rx_0)<<8)); // Rx fifo 0 config base 0, end 8, read 0, write 0 - can overlap with tx as we use one at a time
    if (tx_0>0)
    {
        n += rx_0;
    }
    else
    {
        tx_0 = rx_0;
    }
    GIP_POSTIF_CFG( 1, (n<<0) | ((n+tx_0)<<8) ); // Tx fifo 0 config base 8, end 16, read 0, write 0
    n += tx_0;

    // egress fifo sram (2kx32) split as etxcmd 32, sscmd 32, ssdata 32, gap, etx 1920 (rest)
    GIP_POSTBUS_IO_FIFO_CFG( 0, 0, 0,  0/2,   15, 3 );  // eth tx cmd - unused - note status size and base are in entries, not words
    GIP_POSTBUS_IO_FIFO_CFG( 0, 0, 1, 32/2,   15, 3 );  // ss cmd
    GIP_POSTBUS_IO_FIFO_CFG( 0, 0, 2,  0/2,   15, 3 );  // unused
    GIP_POSTBUS_IO_FIFO_CFG( 0, 0, 3,  0/2,   15, 3 );  // unused
    GIP_POSTBUS_IO_FIFO_CFG( 1, 0, 0,  128, 1919, 511 );  // eth tx data
    GIP_POSTBUS_IO_FIFO_CFG( 1, 0, 1,   64,   31, 3 );  // ss txd
    GIP_POSTBUS_IO_FIFO_CFG( 1, 0, 2,    0,   31, 3 );  // unused
    GIP_POSTBUS_IO_FIFO_CFG( 1, 0, 3,    0,   31, 3 );  // unused

    // ingress fifo sram (2kx32) split as erx status 32, etx status 32, ss status 32, ss rxd 32, erx 1920 (rest)
    GIP_POSTBUS_IO_FIFO_CFG( 0, 1, 0,  0/2,   15, 3 ); // eth status - note status size and base are in entries, not words
    GIP_POSTBUS_IO_FIFO_CFG( 0, 1, 1, 32/2,   15, 3 ); // ss status
    GIP_POSTBUS_IO_FIFO_CFG( 0, 1, 2,  0/2,   15, 3 ); // unused
    GIP_POSTBUS_IO_FIFO_CFG( 0, 1, 3,  0/2,   15, 3 ); // unused
    GIP_POSTBUS_IO_FIFO_CFG( 1, 1, 0,  128, 1919, 3 ); // erx data
    GIP_POSTBUS_IO_FIFO_CFG( 1, 1, 1,   96,   31, 3 ); // ss rx data
    GIP_POSTBUS_IO_FIFO_CFG( 1, 1, 2,    0,   31, 3 ); // etx data - unused
    GIP_POSTBUS_IO_FIFO_CFG( 1, 1, 3,    0,   31, 3 ); // unused

}
/*f gipdrv_ethernet_init
  we use semaphore 24 as virtual rx status ne
  we use semaphore 25 as virtual tx status ne
 */
extern void gipdrv_ethernet_init( void )
{
    int i;

    postbus_config( 16, 16, 0, 0 );

    GIP_POST_TXD_0(9); // set ethernet config, holding in reset
    GIP_POST_TXC_0_IO_CFG(0,0,IO_A_SLOT_ETHERNET_0);   // do I/O configuration
    sync_serial_init( IO_A_SLOT_SYNC_SERIAL_0 );

    // set auto-neg to 10FD only
    // clock MDC 32 times with MDIO high, then send command
    sync_serial_mdio_write( IO_A_SLOT_SYNC_SERIAL_0, 10, 0xffffffff ); 
    sync_serial_mdio_write( IO_A_SLOT_SYNC_SERIAL_0, 10, 0x58920040 ); // 0101 10001 0010010 0000 0000 0100 0000 - address is 0x11
    sync_serial_mdio_write( IO_A_SLOT_SYNC_SERIAL_0, 10, 0xffffffff ); 
    sync_serial_mdio_write( IO_A_SLOT_SYNC_SERIAL_0, 10, 0x51920040 ); // 0101 00011 0010010 0000 0000 0100 0000 - address is 0x03
    // restart auto-neg
    // clock MDC 32 times with MDIO high, then send command
    sync_serial_mdio_write( IO_A_SLOT_SYNC_SERIAL_0, 10, 0xffffffff ); 
    sync_serial_mdio_write( IO_A_SLOT_SYNC_SERIAL_0, 10, 0x58821340 ); // 0101 10001 0000010 0001 0011 0100 0000 - address is 0x11
    sync_serial_mdio_write( IO_A_SLOT_SYNC_SERIAL_0, 10, 0xffffffff ); 
    sync_serial_mdio_write( IO_A_SLOT_SYNC_SERIAL_0, 10, 0x51821340 ); // 0101 00011 0010010 0000 0000 0100 0000 - address is 0x03

    GIP_POST_TXD_0(9); // set ethernet config, releasing reset
    GIP_POST_TXC_0_IO_CFG(0,0,IO_A_SLOT_ETHERNET_0);   // do I/O configuration

// header_details[0] is all we have at the moment (in event occurrence)
// set event[1] to be erxs (empty==0) send header_details[0] as command
// set event[2] to be etxs (empty==0) send header_details[0] as command
// set event[3] to be etxd (watermark==0) send header_details[0] as command
// set header_details[0] to point to us
    GIP_CLEAR_SEMAPHORES(0xf<<24);
    GIP_SET_SEMAPHORES(SEM_TX_DATA_VWM); // watermark is low to start with, so set our semaphore
    GIP_POST_TXD_0( GIP_POSTBUS_COMMAND( 0, 0, 0 ) ); // header details: route=0
    GIP_POST_TXC_0_IO_CMD( 0, 0, 0, 0x11 );
    NOP;NOP;NOP;NOP;NOP;NOP;
    GIP_POST_TXD_0( (IO_A_SLOT_ETHERNET_0<<0) | (1<<2) | (1<<3) | (1<<4) | (0<<5) | (0<<postbus_command_source_io_cmd_op_start) | (0<<postbus_command_source_io_length_start) | (SEM_NUM_STATUS_VNE<<postbus_command_target_gip_rx_semaphore_start) ); // ingress=1, cmd_status=1, fifo=0, empty_not_watermark=1, level=0, length=0, cmd_op=0 (read data length 0)
    GIP_POST_TXC_0_IO_CMD( 0, 0, 0, 0x14 );
    NOP;NOP;NOP;NOP;NOP;NOP;
    GIP_POST_TXD_0( (IO_A_SLOT_ETHERNET_0<<0) | (0<<2) | (0<<3) | (0<<4) | (0<<5) | (0<<postbus_command_source_io_cmd_op_start) | (0<<postbus_command_source_io_length_start) | (SEM_NUM_TX_DATA_VWM<<postbus_command_target_gip_rx_semaphore_start) ); // ingress=0, cmd_status=0, fifo=0, empty_not_watermark=0, level=0, length=0, cmd_op=0 (read data length 0)
    GIP_POST_TXC_0_IO_CMD( 0, 0, 0, 0x18 );
    NOP;NOP;NOP;NOP;NOP;NOP;

    char *block;
    t_gipeth_buffer *buf, *last_buf;
#ifdef REGRESSION
    block = (char *)0x80100000;
#else
    block = kmalloc((sizeof(t_gipeth_buffer)+GIPETH_BUFFER_LENGTH_INC_TIME)*(NUM_TX_BUFFERS + NUM_RX_BUFFERS), GFP_KERNEL);
#endif

    last_buf = (t_gipeth_buffer *)(block + (sizeof(t_gipeth_buffer)+GIPETH_BUFFER_LENGTH_INC_TIME)*(NUM_TX_BUFFERS-1));
    for (i=0; i<NUM_TX_BUFFERS; i++)
    {
        buf = (t_gipeth_buffer *)block;
        buf->next_in_list = last_buf;
        buf->data = (unsigned int *)(block+sizeof(t_gipeth_buffer));
        buf->ready = 0; // initialized here, set by client, cleared on tx done
        buf->done = 1; // initialized here, cleared by us when ready, set on tx done, read by client
        last_buf = buf;
        block += (sizeof(t_gipeth_buffer)+GIPETH_BUFFER_LENGTH_INC_TIME);
    }
    
    eth.tx_next_buffer_to_tx = last_buf; // initialized here, owned by the hardware thread
    eth.tx_next_buffer_done = last_buf; // initialized here, owned by the client
    eth.tx_buffer_in_hand = NULL; // initialized here, owned by the hardware thread

    last_buf = (t_gipeth_buffer *)(block + (sizeof(t_gipeth_buffer)+GIPETH_BUFFER_LENGTH_INC_TIME)*(NUM_RX_BUFFERS-1));
    for (i=0; i<NUM_RX_BUFFERS; i++)
    {
        buf = (t_gipeth_buffer *)block;
        buf->next_in_list = last_buf;
        buf->data = ((unsigned int *)(block+sizeof(t_gipeth_buffer)))+1; // allow 1 word for time
        buf->ready = 0; // initialized here, set by hardware thread on rx done, read by client and cleared
        buf->done = 1; // initialized here, set by client on handling complete, cleared by driver when buffer taken by hardware thread
        last_buf = buf;
        block += (sizeof(t_gipeth_buffer)+GIPETH_BUFFER_LENGTH_INC_TIME);
    }
    eth.rx_buffer_in_hand = NULL; // initialized here, owned by hardware thread - takes rx_next_buffer_to_reuse if NULL and data comes in
    eth.rx_next_buffer_to_reuse = last_buf; // initialized here, owned by us - we reuse this buffer if it is done
    eth.rx_next_buffer_rxed = last_buf; // initialized here, owned by the client
    __asm__ volatile ( " .word 0xec00c1ce ; mov r0, %0 " : : "r" (&eth) );

#ifdef REGRESSION
    uart_tx_string_nl("here");
#else
    printk("GIP ethernet driver initialized - last buffer %p\n", last_buf );
#endif
    MK_INT_DIS();
	MK_TRAP2( MK_TRAP_SET_IRQ_HANDLER, 1, (unsigned int)gipeth_isr_asm);
    GIP_ATOMIC_MAX_BLOCK();
    GIP_SET_THREAD(6,gipeth_hw_thread,(((SEM_STATUS_VNE|SEM_SOFT_IRQ)>>24)<<4)|1); // set thread 6 startup to be ARM, on semaphore 24, 25 or 26 set, and the entry point
    MK_INT_EN();
}

/*a ISR and driver init
 */
/*f gipeth_isr
  called when a transmit or a receive completes
  the transmit block ptrs and receive block ptrs should be checked to see if there is anything in the FIFOs
 */
#ifdef USE_CIRC_BUFFERS
static unsigned int *circ_buffer = (unsigned int *)CIRC_BUFFER_BASE;
#endif
extern void gipeth_isr (void)
{
//	printk ("Eth ISR\n");

#ifdef REGRESSION
    uart_init();
#endif
    while (1)
	{
        t_gipeth_buffer *buf;
        int len;

        // hardware thread takes buffer marked 'done', clears 'done', fills buffer, sets 'ready' and won't touch the buffer until 'done' is set
        // if ready is set then we handle the buffer clear ready, then set done and watch the next buffer
        buf = eth.rx_next_buffer_rxed;
        if (buf->ready)
        {
            len = buf->length;
#ifdef REGRESSION
            uart_tx_string("Pkt ");
            uart_tx_hex8(len);
            uart_tx_nl();
#else
            unsigned char * ptr;
//            printk("L %d\n",len); // Note len includes padding of 2 bytes at the front
            if (len>0)
            {
                struct sk_buff * skb;
                struct net_device * dev = dev1;
                skb = dev_alloc_skb(len); // allocate room for data and padding
#ifdef USE_CIRC_BUFFERS
                circ_buffer[0] = (unsigned int)len;
                circ_buffer[1] = (unsigned int)skb;
                {unsigned int s; GIP_TIMER_READ_0(s); circ_buffer[2]=s;}
                circ_buffer[3] = (unsigned int)buf;
                circ_buffer = (unsigned int *) ( (((unsigned int)circ_buffer)+16)&~0x1000 );
#endif // USE_CIRC_BUFFERS
                if (!skb)
                {
                    printk ("Failed to allocate skb\n");
                }
                else
                {
                    skb->dev = dev;
                    skb->pkt_type = PACKET_HOST; 
                
                    skb_reserve(skb, 2); // align IP header
                
                    ptr = skb_put(skb, len-2);
                    //		printk ("ptr = %x\n", ptr);
                    memcpy (ptr, ((unsigned char *)buf->data)+2, len-2);// skip padding
 	
                    skb->protocol = eth_type_trans(skb, dev);
                    //		printk ("Protocol is %x\n", skb->protocol);
                    netif_rx(skb);
                }
            }
#endif // REGRESSION
            eth.rx_next_buffer_rxed = buf->next_in_list;
            buf->ready = 0;
            buf->done = 1;
        }
        else
        {
            break;
        }
	}
    while (0) // data queued for transmit
    {
//        int i;
//        i = tx.next_buffer_done;
//        if (tx.buffers[i].done)
//        {
//            tx.next_buffer_done = (i+1)%NUM_TX_BUFFERS;
//        }
        // transmit to buffer i, mark it as ready
    }
}

/*f gipeth_xmit
  If there is a tx block available then copy it to the tx buffer and free
  Else add it to our tx queue
 */
extern void analyzer_enable( void );
#ifdef USE_CIRC_BUFFERS
static unsigned int *tx_circ_buffer = (unsigned int *)CIRC_BUFFER_BASE_2;
#endif // USE_CIRC_BUFFERS
#ifdef REGRESSION
extern int gipeth_xmit( unsigned int *data, int length )
#else
static int gipeth_xmit(struct sk_buff *skb, struct net_device *dev)
#endif // REGRESSION
{
    t_gipeth_buffer *buf;
	
// 	struct net_device_stats *stats = dev->priv;
// 	stats->tx_packets++;
// 	stats->tx_bytes+=skb->len;

    buf = eth.tx_next_buffer_done;
#ifdef USE_CIRC_BUFFERS
    tx_circ_buffer[0] = ((unsigned int)skb->len) | (buf->done?0x80000000:0);
    tx_circ_buffer[1] = (unsigned int)skb;
    {unsigned int s; GIP_TIMER_READ_0(s); tx_circ_buffer[2]=s;}
    tx_circ_buffer[3] = (unsigned int)buf;
    tx_circ_buffer = (unsigned int *) ( (((unsigned int)tx_circ_buffer)+16)&~0x1000 );
#endif // USE_CIRC_BUFFERS
    if (buf->done)
    {
#ifdef REGRESSION
        int i;
        length+=2; // account for padding - we ignore first two bytes of data
        if (length>GIPETH_BUFFER_LENGTH) length=GIPETH_BUFFER_LENGTH;
        for (i=0; (i<length/4); i++ )
        {
            buf->data[i] = data[i];
        }
        for (;i<17;i++)
        {
            buf->data[i] = 0;
        }
        buf->length = length<62?62:length; // 62-2 for padding+4 for crc is the minimum packet size of 64 bytes
#else
        int i;
        skb_copy_datagram(skb, 0, ((unsigned char *)buf->data)+2, GIPETH_BUFFER_LENGTH-2); // don't put data in the padding
        i = skb->len+2;
        dev_kfree_skb(skb);
        for (;i<60;i+=4)
        {
            buf->data[i/4+1] = 0;
        }
        buf->length = i;

// 	struct net_device_stats *stats = dev->priv;
// 	stats->tx_packets++;
// 	stats->tx_bytes+=skb->len;
    analyzer_enable();
#endif // REGRESSION
        buf->done = 0;
        buf->ready = 1;
        eth.tx_next_buffer_done = buf->next_in_list;

        GIP_SET_SEMAPHORES( SEM_SOFT_IRQ );        // hit hardware thread with softirq
    }
    else
    {
#ifdef REGRESSION
#else
//		dev_kfree_skb(skb);
#endif // REGRESSION
        // enqueue buffer
        return 1;
    }
	
	return 0;
}

/*f gipeth_accept_fastpath
 */
#ifdef CONFIG_NET_FASTROUTE
static int gipeth_accept_fastpath(struct net_device *dev, struct dst_entry *dst)
{
 	return -1;
 }
#endif

#ifdef REGRESSION
#else
static struct net_device_stats *gipeth_get_stats(struct net_device *dev)
{
 	return dev->priv;
}
/* fake multicast ability */
static void set_multicast_list(struct net_device *dev)
{
}
 
#endif // REGRESSION

/*f gipeth_setup
 */
#ifdef REGRESSION
extern void gipeth_setup(void)
#else
static void __init gipeth_setup(struct net_device *dev)
#endif // REGRESSION
{
#ifdef REGRESSION
#else
    printk("gipeth.c:gipeth_setup\n");
#endif
    gipdrv_ethernet_init();

#ifdef REGRESSION
#else
 	/* Initialize the device structure. */
 	dev->get_stats = gipeth_get_stats;
 	dev->hard_start_xmit = gipeth_xmit;
 	dev->set_multicast_list = set_multicast_list;
#ifdef CONFIG_NET_FASTROUTE
 	dev->accept_fastpath = gipeth_accept_fastpath;
#endif
 
 	/* Fill in device structure with ethernet-generic values. */
 	ether_setup(dev);
 	dev->tx_queue_len = 0;
// 	dev->flags |= IFF_NOARP;
 	dev->flags &= ~IFF_MULTICAST;
 	SET_MODULE_OWNER(dev);
 
 	dev1 = dev;
#endif // REGRESSION
}

#ifdef REGRESSION
#else
static struct net_device *dev_gipeth;
static int __init gipeth_init_module(void)
{
	int err;

	dev_gipeth = alloc_netdev(sizeof(struct net_device_stats),
				 "eth%d", gipeth_setup);

	if (!dev_gipeth)
		return -ENOMEM;

	dev_gipeth->dev_addr[0] = 0x00;
	dev_gipeth->dev_addr[1] = 0x12;
	dev_gipeth->dev_addr[2] = 0x34;
	dev_gipeth->dev_addr[3] = 0x56;
	dev_gipeth->dev_addr[4] = 0x78;
	dev_gipeth->dev_addr[5] = 0x90;
	
	if ((err = register_netdev(dev_gipeth))) {
		kfree(dev_gipeth);
		dev_gipeth = NULL;
	}
	return err;
}

static void __exit gipeth_cleanup_module(void)
{
	unregister_netdev(dev_gipeth);
	free_netdev(dev_gipeth);
	dev_gipeth = NULL;
}
  

module_init(gipeth_init_module);
module_exit(gipeth_cleanup_module);
MODULE_LICENSE("GPL");

#endif // REGRESSION
