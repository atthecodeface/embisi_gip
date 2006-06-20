
/*a Types
 */
/*t t_eth_buffer
 */
typedef struct t_eth_buffer
{
    struct t_eth_buffer *next; // when owned by the driver, this links buffers
    unsigned char *data;  // pointer to data
    int buffer_size;      // on transmit is bytes to transmit; on receive, size of buffer space available
    void *buffer_handle;  // not used by driver; can be used by client
} t_eth_buffer;

/*t t_eth_tx_callback_fn
 */
typedef void t_eth_tx_callback_fn( void *handle, t_eth_buffer *buffer );

/*t t_eth_rx_callback_fn
 */
typedef void t_eth_rx_callback_fn( void *handle, t_eth_buffer *buffer, int rxed_byte_length );

/*a External functions
 */
extern void ethernet_set_tx_callback( t_eth_tx_callback_fn callback, void *handle );
extern void ethernet_tx_buffer( t_eth_buffer *buffer );
extern void ethernet_set_rx_callback( t_eth_rx_callback_fn callback, void *handle );
extern void ethernet_add_rx_buffer( t_eth_buffer *buffer );
extern void ethernet_poll( void );
extern void ethernet_init( int gip_postbus_route, int eth_postbus_route, int slot, int endian_swap, int padding );
