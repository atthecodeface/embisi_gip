/*a Constants
 */
// We break out the command FIFO data
// We need a packet length, a retry count, FCS required, full-duplex (ignore collisions), and possibly deferral time; deferral time is 9.6us at 100Mbps/25MHz/40ns, or 240 bits, so we need 8-bits for deferral.
constant integer io_eth_tx_max_packet_byte_length_bits = 16;
constant integer io_eth_tx_packet_retry_count_bits = 4;
constant integer io_eth_tx_deferral_value_bits = 8;
constant integer io_eth_tx_packet_status_bits = 2;
constant integer io_eth_tx_cfd_packet_length_start_bit    = 0;
constant integer io_eth_tx_cfd_packet_retry_count_start_bit = io_eth_tx_cfd_packet_length_start_bit    + io_eth_tx_max_packet_byte_length_bits;
constant integer io_eth_tx_cfd_deferral_restart_value_bit = io_eth_tx_cfd_packet_retry_count_start_bit         + io_eth_tx_packet_retry_count_bits; // For transmit
constant integer io_eth_tx_cfd_half_duplex_bit            = io_eth_tx_cfd_deferral_restart_value_bit + io_eth_tx_deferral_value_bits;
constant integer io_eth_tx_cfd_transmit_fcs_bit           = io_eth_tx_cfd_half_duplex_bit + 1;
constant integer io_eth_tx_cfd_packet_status_start_bit    = io_eth_tx_cfd_packet_retry_count_start_bit         + io_eth_tx_packet_retry_count_bits; // For receive

constant integer io_eth_tx_max_counter_for_permitted_collision = 64;

/*a Types
 */
/*t t_io_eth_tx_status
 */
typedef enum [io_eth_tx_packet_status_bits]
{
    io_eth_tx_status_ok = 0,
    io_eth_tx_status_late_col = 1,
    io_eth_tx_status_retries_exceeded = 2,
} t_io_eth_tx_status;

