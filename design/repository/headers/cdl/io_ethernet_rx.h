/*a Constants
 */
// We break out the status FIFO data
// We need a packet length in bytes (zero for non-end-of-packet blocks), block length in bytes (0 thru 64 probably), and a status
constant integer io_eth_rx_packet_byte_length_bits = 16;
constant integer io_eth_rx_block_byte_length_bits = 8;
constant integer io_eth_rx_block_status_bits = 3;
constant integer io_eth_rx_sfd_packet_length_start_bit     = 0;
constant integer io_eth_rx_sfd_block_byte_length_start_bit = io_eth_rx_sfd_packet_length_start_bit     + io_eth_rx_packet_byte_length_bits;
constant integer io_eth_rx_sfd_block_status_start_bit      = io_eth_rx_sfd_block_byte_length_start_bit + io_eth_rx_block_byte_length_bits;

/*a Types
 */
/*t t_io_eth_rx_status
 */
typedef enum [io_eth_rx_block_status_bits]
{
    io_eth_rx_status_packet_complete_fcs_ok   = 0,
    io_eth_rx_status_packet_complete_fcs_bad  = 1,
    io_eth_rx_status_packet_complete_odd_nybbles = 2,
    io_eth_rx_status_block_complete = 3,
    io_eth_rx_status_fifo_overrun = 4,
    io_eth_rx_status_framing_error = 5,
} t_io_eth_tx_status;

/*a Modules
 */
extern module io_ethernet_rx( clock io_clock,
                    input bit io_reset,

                    output bit[32] data_fifo_data,
                    output bit data_fifo_toggle,
                    input bit data_fifo_full,

                    output bit[32] status_fifo_data,
                    output bit status_fifo_toggle,

                    input bit mii_dv,
                    input bit mii_err,
                    input bit[4] mii_data
    )
{
    timing to rising clock io_clock io_reset;
    timing to rising clock io_clock data_fifo_full;
    timing to rising clock io_clock mii_dv, mii_err, mii_data;
    timing from rising clock io_clock data_fifo_data, data_fifo_toggle;
    timing from rising clock io_clock status_fifo_data, status_fifo_toggle;

    timing comb input io_reset;
}

