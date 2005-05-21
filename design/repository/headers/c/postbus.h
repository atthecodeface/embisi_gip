/*t postbus
 */
enum
{
    postbus_command_route_size = 7,
    postbus_command_source_size = 10,
    postbus_command_flow_control_size = 7,
    postbus_command_target_size = 7,

    postbus_command_last_bit = 0, // bit 0 of first transaction word indicates its also the last
    postbus_command_route_start = 1, // bits 1-7 of first transaction word indicates the destination (routing information) for the transaction
    postbus_command_source_start = postbus_command_route_start + postbus_command_route_size,
    postbus_command_flow_control_start = postbus_command_source_start + postbus_command_source_size,
    postbus_command_target_start = postbus_command_flow_control_start + postbus_command_flow_control_size,

    postbus_command_source_gip_tx_length_start = postbus_command_source_start,
    postbus_command_source_gip_tx_signal_start = postbus_command_source_gip_tx_length_start+5,
    postbus_command_source_io_length_start = postbus_command_source_start,
    postbus_command_source_io_cmd_op_start = postbus_command_source_io_length_start+5,

    postbus_command_target_gip_rx_fifo_start = postbus_command_target_start,                    //  the receive FIFO to receive into
    postbus_command_target_gip_rx_semaphore_start = postbus_command_target_gip_rx_fifo_start+2, //  the semaphore to signal once completely received

    postbus_command_target_io_dest_type_start = postbus_command_target_start,               // 2 bits of the destination type, general config, fifo config, data, command
    postbus_command_target_io_dest_start = postbus_command_target_io_dest_type_start+2,     // 5 bits of write address OR ...
    postbus_command_target_io_fifo_start = postbus_command_target_io_dest_type_start+2,     // 2 bits of fifo number
    postbus_command_target_io_ingress = postbus_command_target_io_fifo_start+2,             // 1 bit for tx/rx (ingress)
    postbus_command_target_io_cmd_status = postbus_command_target_io_ingress+1             // 1 bit for cmd status / data
};

typedef enum
{
    postbus_ack_hold = 0,
    postbus_ack_taken = 1,
} t_postbus_ack;

typedef enum
{
    postbus_word_type_start = 0,
    postbus_word_type_idle = 1,
    postbus_word_type_hold = 1,
    postbus_word_type_data = 2,
    postbus_word_type_last = 3,
} t_postbus_type;


