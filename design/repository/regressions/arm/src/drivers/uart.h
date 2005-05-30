/*a Defines
 */

/*a Functions
 */
#ifndef LINUX
extern int uart_init( void );
extern int uart_rx_poll( void );
extern int uart_rx_byte( void );
extern void uart_tx( int a );
extern void uart_tx_nl( void );
extern void uart_tx_string( const char *string );
extern void uart_tx_string_nl( const char *string );
extern void uart_tx_hex8( unsigned int a );
extern void uart_tx_hex2( unsigned int a );
#else
#include <stdlib.h>
#include <stdio.h>
#define uart_init()
#define uart_rx_poll(a) (1)
#define uart_rx_byte() (getchar())
#define uart_tx(a) {putc(a,stderr);}
#define uart_tx_nl() {putc('\n',stderr);}
#define uart_tx_string(a) {fprintf(stderr,"%s",a);}
#define uart_tx_string_nl(a) {fprintf(stderr,"%s\n",a);}
#define uart_tx_hex8(a) {fprintf(stderr,"%08x", (unsigned int)a);}
#define test_entry_point() main()
#endif
