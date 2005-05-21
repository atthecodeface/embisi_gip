/*a Constants
 */
constant integer io_sync_serial_transfer_length_bits = 8;
constant integer io_sync_serial_clock_divide_bits = 8;
constant integer io_sync_serial_chip_select_bits = 3;
constant integer io_sync_serial_tristate_delay_bits = 4;
constant integer io_sync_serial_type_bits = 2;


constant integer io_sync_serial_cfd_transfer_length_start_bit = 0;
constant integer io_sync_serial_cfd_clock_divide_start_bit    = io_sync_serial_cfd_transfer_length_start_bit + io_sync_serial_transfer_length_bits; // 8
constant integer io_sync_serial_cfd_tristate_delay_start_bit  = io_sync_serial_cfd_clock_divide_start_bit + io_sync_serial_clock_divide_bits; // 16
constant integer io_sync_serial_cfd_chip_select_start_bit     = io_sync_serial_cfd_tristate_delay_start_bit + io_sync_serial_tristate_delay_bits; // 20
constant integer io_sync_serial_cfd_input_pin_bit             = io_sync_serial_cfd_chip_select_start_bit + io_sync_serial_chip_select_bits; //23
constant integer io_sync_serial_cfd_clock_pin_bit             = io_sync_serial_cfd_input_pin_bit + 1; // 24
constant integer io_sync_serial_cfd_cpol_bit                  = io_sync_serial_cfd_clock_pin_bit + 1; // 25
constant integer io_sync_serial_cfd_cpha_bit                  = io_sync_serial_cfd_cpol_bit + 1; // 26
constant integer io_sync_serial_cfd_type_start_bit            = io_sync_serial_cfd_cpha_bit + 1; // 27
constant integer io_sync_serial_cfd_continuation_requested_bit = io_sync_serial_cfd_type_start_bit + io_sync_serial_type_bits; // 29

constant integer io_sync_serial_cmd_type_spi_output = 0;
constant integer io_sync_serial_cmd_type_spi_no_output = 1;
constant integer io_sync_serial_cmd_type_mdio = 2;

/*a Types
 */

/*a Modules
 */
extern module io_sync_serial( clock int_clock,
                              input bit int_reset,

                              input bit[32] tx_data_fifo_data,
                              output t_io_tx_data_fifo_cmd tx_data_fifo_cmd,
                              output bit tx_data_fifo_toggle,

                              output bit[32] rx_data_fifo_data,
                              output bit rx_data_fifo_toggle,

                              input bit cmd_fifo_empty,
                              input bit[32] cmd_fifo_data,
                              output bit cmd_fifo_toggle,

                              output bit status_fifo_toggle,
                              output bit[32] status_fifo_data,

                              output bit[2] sscl,
                              output bit[2] sscl_oe,
                              output bit ssdo,
                              output bit ssdo_oe,
                              input bit[2] ssdi,
                              output bit[8] sscs
    )
{
    timing to rising clock int_clock int_reset;

    timing to rising clock int_clock tx_data_fifo_data, cmd_fifo_empty, cmd_fifo_data;
    timing from rising clock int_clock tx_data_fifo_cmd, tx_data_fifo_toggle, cmd_fifo_toggle;

    timing from rising clock int_clock status_fifo_toggle, status_fifo_data, rx_data_fifo_data, rx_data_fifo_toggle;

    timing to rising clock int_clock ssdi;
    timing from rising clock int_clock sscl, sscl_oe, ssdo, ssdo_oe, sscs;

    timing comb input int_reset;
}
