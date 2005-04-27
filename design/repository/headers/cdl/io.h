/*a Copyright Gavin J Stark, 2004
 */

/*a To do
 */

/*a Constants
 */
constant integer io_cmd_timestamp_length=24; // But in the FIFO the next bit up is the 'immediate' bit, ignoring the timestamp entirely
constant integer io_cmd_timestamp_sublength=io_cmd_timestamp_length/4;
constant integer io_sram_log_size=11;

/*a Types
 */
/*t t_io_fifo_event_type
 */
typedef enum [2]
{
    io_fifo_event_type_none,
    io_fifo_event_type_level,
    io_fifo_event_type_edge,
} t_io_fifo_event_type;

/*t t_io_fifo_event
 */
typedef struct
{
    bit event;
    bit value;
} t_io_fifo_event;

/*t t_io_fifo_op
 */
typedef enum [3]
{
    io_fifo_op_none, // does nothing
    io_fifo_op_reset, // zeros ptrs, empties FIFO
    io_fifo_op_write_cfg, // write base address (10/11 bits, added to ptrs), size (9/10 bits), watermark (9/10 bits); empties FIFO, zeros ptrs also
    io_fifo_op_inc_write_ptr, // If full, does nothing, excepts sets overflow
    io_fifo_op_inc_read_ptr_without_commit, // If empty, does nothing, excepts sets underflow
    io_fifo_op_inc_read_ptr,                // If empty, does nothing, excepts sets underflow
    io_fifo_op_commit_read_ptr,
    io_fifo_op_revert_read_ptr
} t_io_fifo_op;

/*t t_io_sram_data_op
 */
typedef enum [3]
{
    io_sram_data_op_none,
    io_sram_data_op_read,
    io_sram_data_op_read_fifo_status,
    io_sram_data_op_write_time,
    io_sram_data_op_write_data,
    io_sram_data_op_write_data_reg,
    io_sram_data_op_write_postbus
} t_io_sram_data_op;

/*t t_io_sram_data_reg_op
 */
typedef enum [2]
{
    io_sram_data_reg_op_hold,
    io_sram_data_reg_op_status,
    io_sram_data_reg_op_postbus,
} t_io_sram_data_reg_op;

/*t t_io_sram_address_op
 */
typedef enum [2]
{
    io_sram_address_op_fifo_ptr,
    io_sram_address_op_fifo_ptr_set_bit_0,
    io_sram_address_op_egress_addressed,
    io_sram_address_op_postbus_addressed,
} t_io_sram_address_op;

