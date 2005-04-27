/*a Copyright Gavin J Stark, 2004
 */

/*a To do
 */

/*a Constants
 */

/*a Includes
 */

/*a Types
 */
/*t t_ingress_fifo_op
 */
typedef enum [3]
{
    ingress_fifo_op_none, // does nothing
    ingress_fifo_op_reset, // zeros ptrs, empties FIFO
    ingress_fifo_op_write_cfg, // write base address (8/9 bits, added to ptrs), size (8/9 bits), watermark (8/9 bits); empties FIFO, zeros ptrs also
    ingress_fifo_op_inc_write_ptr, // If full, does nothing, excepts sets overflow
    ingress_fifo_op_inc_read_ptr, // If empty, does nothing, excepts sets underflow
} t_ingress_fifo_op;

/*a Modules
 */
extern module io_ingress_fifos( clock int_clock "main system clck",
                                input bit int_reset "Internal system reset",
                                input t_ingress_fifo_op fifo_op "Operation to perform",
                                input bit fifo_op_to_status "Asserted for status FIFO operations, deasserted for Rx Data FIFO operations",
                                input bit fifo_address_from_read_ptr "Asserted if the FIFO address output should be for the read ptr of the specified FIFO, deasserted for write",
                                input bit[2] fifo_to_access "Number of FIFO to access for operations",
                                input t_io_fifo_event_type fifo_event_type "Describes what sort of FIFO event to watch for",
                                output bit[io_sram_log_size] fifo_address "FIFO address out",

                                output bit[4] status_fifo_empty       "Per-status FIFO, asserted if more than zero entries are present",
                                output bit[4] status_fifo_full        "Per-status FIFO, asserted if read_ptr==write_ptr and not empty",
                                output bit[4] status_fifo_overflowed  "Per-status FIFO, asserted if FIFO has overflowed since last reset or configuration write",
                                output bit[4] status_fifo_underflowed "Per-status FIFO, asserted if FIFO has underflowed since last reset or configuration write",

                                output bit[4] rx_data_fifo_empty       "Per-rx_data FIFO, asserted if more than zero entries are present",
                                output bit[4] rx_data_fifo_watermark   "Per-rx_data FIFO, asserted if more than watermak entries are present in the FIFO",
                                output bit[4] rx_data_fifo_full        "Per-rx_data FIFO, asserted if read_ptr==write_ptr and not empty",
                                output bit[4] rx_data_fifo_overflowed  "Per-rx_data FIFO, asserted if FIFO has overflowed since last reset or configuration write",
                                output bit[4] rx_data_fifo_underflowed "Per-rx_data FIFO, asserted if FIFO has underflowed since last reset or configuration write",

                                output bit event_from_status "Asserted if event comes from a status FIFO",
                                output bit[2] event_fifo "Fifo number last written to",
                                output t_io_fifo_event event_empty "Indicates value of empty and if empty changed (and edge event)",
                                output t_io_fifo_event event_watermark "Indicates value of watermark and if watermark changed (and edge event)",

                                input bit[io_sram_log_size] cfg_base_address,
                                input bit[io_sram_log_size] cfg_size_m_one,
                                input bit[io_sram_log_size] cfg_watermark,
                                output bit[32] read_cfg_status )
{
    timing to rising clock int_clock int_reset;
    timing to rising clock int_clock fifo_op, fifo_op_to_status, fifo_address_from_read_ptr, fifo_to_access, fifo_event_type;
    timing from rising clock int_clock fifo_address;

    timing from rising clock int_clock status_fifo_empty, status_fifo_full, status_fifo_overflowed, status_fifo_underflowed;
    timing from rising clock int_clock rx_data_fifo_empty, rx_data_fifo_full, rx_data_fifo_watermark, rx_data_fifo_overflowed, rx_data_fifo_underflowed;
    timing from rising clock int_clock event_from_status, event_fifo, event_empty, event_watermark;

    timing to rising clock int_clock cfg_base_address, cfg_size_m_one, cfg_watermark;
    timing from rising clock int_clock read_cfg_status;

    timing comb input fifo_address_from_read_ptr, fifo_op_to_status, fifo_to_access;
    timing comb output fifo_address, read_cfg_status;

}

