/*a Copyright Gavin J Stark, 2004
 */

/*a To do
 */

/*a Constants
 */
// In a command we specify tx config, rx config and an optional tx data byte (which may be 9 bits for parity)
// On transmit success we return a status; on receive we return a status too
constant integer io_uart_cmd_bit_tx_start_bits=0; // 1 bit
constant integer io_uart_cmd_bit_tx_data_bits=1; // 3 bits, for 5, 6, 7, 8 or 9 (for parity)
constant integer io_uart_cmd_bit_tx_stop_bits=4; // 2 bits, for 0.5 to 2
constant integer io_uart_cmd_bit_tx_fc=6; // 1 bit, indicates flow control should be used
constant integer io_uart_cmd_bit_tx_req=7; // 1 bit, indicates byte should be txed
constant integer io_uart_cmd_bit_tx_byte_start=8;    

constant integer io_uart_cmd_bit_rx_start_bits=24; // 1 bit
constant integer io_uart_cmd_bit_rx_data_bits=25; // 3 bits, for 5, 6, 7, 8 or 9 
constant integer io_uart_cmd_bit_rx_stop_bits=28; // 2 bits, for 0.5 to 2

/*a Types
 */

/*a Modules
 */
extern module io_uart( clock int_clock "Internal system clock",
                       input bit int_reset "Internal reset",

                       input bit tx_baud_enable "Baud enable for transmit, 16 x bit time",
                       output bit txd "Transmit data out",
                       input bit txd_fc "Transmit flow control; assert to pause transmit",

                       input bit rx_baud_enable "Baud enable for receive, 16 x bit time",
                       input bit rxd "Receive data in",
                       output bit rxd_fc "Receive flow control; asserted to pause transmit",

                       input bit cmd_fifo_empty,
                       input bit[32] cmd_fifo_data,
                       output bit cmd_fifo_toggle,

                       input bit status_fifo_full,
                       output bit status_fifo_toggle,
                       output bit[32] status_fifo_data )
{
    timing to rising clock int_clock int_reset;

    timing to rising clock int_clock tx_baud_enable, txd_fc;
    timing from rising clock int_clock txd;

    timing to rising clock int_clock rx_baud_enable, rxd;
    timing from rising clock int_clock rxd_fc;

    timing to rising clock int_clock cmd_fifo_empty, cmd_fifo_data;
    timing from rising clock int_clock cmd_fifo_toggle;

    timing to rising clock int_clock status_fifo_full;
    timing from rising clock int_clock status_fifo_toggle, status_fifo_data;
}
