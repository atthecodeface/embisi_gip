/*a Copyright Gavin J Stark, 2004
 */

/*a To do
 */

/*a Constants
 */
constant integer io_ingress_fifo_address_size=8; // Use 8 for status, 9 for data

/*a Types
 */
/*t t_ingress_fifo_op
 */
typedef enum [3]
{
    ingress_fifo_op_reset, // zeros ptrs, empties FIFO
    ingress_fifo_op_write_cfg, // write base address (8/9 bits, added to ptrs), size (8/9 bits), watermark (8/9 bits); empties FIFO, zeros ptrs also
    ingress_fifo_op_inc_write_ptr, // If full, does nothing, excepts sets overflow
    ingress_fifo_op_inc_read_ptr, // If empty, does nothing, excepts sets underflow
} t_ingress_fifo_op;

/*a Modules
 */
extern module io_ingress_fifo( clock int_clock "main system clck",
                              input bit int_reset "main system reset",
                              input t_ingress_fifo_op fifo_op "Operation to perform",
                              output bit[io_ingress_fifo_address_size] fifo_write_address "Write address out",
                              output bit[io_ingress_fifo_address_size] fifo_read_address "Read address out",

                              output bit fifo_empty "Asserted if more than zero entries are present",
                              output bit fifo_watermark "Asserted if more than 'watermark' entries are present",
                              output bit fifo_full "Asserted if read_ptr==write_ptr and not empty",
                              output bit fifo_overflowed "Asserted if the FIFO has overflowed since last reset or configuration write",
                              output bit fifo_underflowed  "Asserted if the FIFO has underflowed since last reset or configuration write",

                              input bit[io_ingress_fifo_address_size] cfg_base_address,
                              input bit[io_ingress_fifo_address_size] cfg_size_m_one,
                              input bit[io_ingress_fifo_address_size] cfg_watermark )
{
    timing to rising clock int_clock int_reset;
    timing to rising clock int_clock fifo_op;
    timing from rising clock int_clock fifo_write_address, fifo_read_address;
    timing from rising clock int_clock fifo_empty, fifo_full, fifo_watermark, fifo_overflowed, fifo_underflowed;

    timing to rising clock int_clock cfg_base_address, cfg_size_m_one, cfg_watermark;
}

