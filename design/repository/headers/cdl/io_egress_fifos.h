/*a Copyright Gavin J Stark, 2004
 */

/*a To do
 */

/*a Constants
 */

/*a Includes
 */
include "io.h"

/*a Types
 */
/*a Modules
 */
extern module io_egress_fifos( clock int_clock "main system clck",
                                input bit int_reset "Internal system reset",

                                input t_io_fifo_op fifo_op "Operation to perform",
                                input bit fifo_op_to_cmd "Asserted for status FIFO operations, deasserted for Rx Data FIFO operations",
                                input bit fifo_address_from_read_ptr "Asserted if the FIFO address output should be for the read ptr of the specified FIFO, deasserted for write",
                                input bit[2] fifo_to_access "Number of FIFO to access for operations",
                                output bit[io_sram_log_size] fifo_address "FIFO address out",

                                output bit[4] cmd_fifo_empty       "Per-cmd FIFO, asserted if more than zero entries are present",
                                output bit[4] cmd_fifo_full        "Per-cmd FIFO, asserted if read_ptr==write_ptr and not empty",
                                output bit[4] cmd_fifo_overflowed  "Per-cmd FIFO, asserted if FIFO has overflowed since last reset or configuration write",
                                output bit[4] cmd_fifo_underflowed "Per-cmd FIFO, asserted if FIFO has underflowed since last reset or configuration write",

                                output bit[4] tx_data_fifo_empty       "Per-tx_data FIFO, asserted if more than zero entries are present",
                                output bit[4] tx_data_fifo_watermark   "Per-tx_data FIFO, asserted if more than watermak entries are present in the FIFO",
                                output bit[4] tx_data_fifo_full        "Per-tx_data FIFO, asserted if read_ptr==write_ptr and not empty",
                                output bit[4] tx_data_fifo_overflowed  "Per-tx_data FIFO, asserted if FIFO has overflowed since last reset or configuration write",
                                output bit[4] tx_data_fifo_underflowed "Per-tx_data FIFO, asserted if FIFO has underflowed since last reset or configuration write",

                                input bit[io_sram_log_size] cfg_base_address,
                                input bit[io_sram_log_size] cfg_size_m_one,
                                input bit[io_sram_log_size] cfg_watermark )
{
    timing to rising clock int_clock int_reset;
    timing to rising clock int_clock fifo_op, fifo_op_to_cmd, fifo_address_from_read_ptr, fifo_to_access;
    timing from rising clock int_clock fifo_address;

    timing from rising clock int_clock cmd_fifo_empty, cmd_fifo_full, cmd_fifo_overflowed, cmd_fifo_underflowed;
    timing from rising clock int_clock tx_data_fifo_empty, tx_data_fifo_full, tx_data_fifo_watermark, tx_data_fifo_overflowed, tx_data_fifo_underflowed;

    timing to rising clock int_clock cfg_base_address, cfg_size_m_one, cfg_watermark;

    timing comb input fifo_address_from_read_ptr, fifo_op_to_cmd, fifo_to_access;
    timing comb output fifo_address;

    timing comb input int_reset;
}

