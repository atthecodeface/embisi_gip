/*a Defines
 */

/*a Functions
 */
extern int uart_init( void );
extern int uart_rx_poll( void );
extern int uart_rx_byte( void );
extern void uart_tx( int a );
extern void uart_tx_hex8( unsigned int a );
