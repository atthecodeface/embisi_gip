constant integer c_postbus_width=32;
typedef enum [2]
{
    postbus_word_type_start = 0,
    postbus_word_type_idle = 1,
    postbus_word_type_hold = 1,
    postbus_word_type_data = 2,
    postbus_word_type_last = 3,
} t_postbus_type;
typedef bit[c_postbus_width] t_postbus_data;
//typedef bit t_postbus_ack;
typedef enum [1]
{
    postbus_ack_hold = 0,
    postbus_ack_taken = 1,
} t_postbus_ack;

// bits 0 through 7 are consumed by routing
// bits 8 through 17 are consumed by source (e.g. 5 bits length, 5 bits semaphore; 5 bits length, 2 bits operation)
// bits 18 through 24 are consumed by flow control (future)
// bits 25 through 31 are consumed by target (e.g. 2 bits fifo, 5 bits semaphore; 5 bits fifo, 2 bits command)
constant integer postbus_command_route_size = 7;
constant integer postbus_command_source_size = 10;
constant integer postbus_command_flow_control_size = 7;
constant integer postbus_command_target_size = 7;

constant integer postbus_command_last_bit = 0; // bit 0 of first transaction word indicates its also the last
constant integer postbus_command_route_start = 1; // bits 1-7 of first transaction word indicates the destination (routing information) for the transaction
constant integer postbus_command_source_start = postbus_command_route_start + postbus_command_route_size;
constant integer postbus_command_flow_control_start = postbus_command_source_start + postbus_command_source_size;
constant integer postbus_command_target_start = postbus_command_flow_control_start + postbus_command_flow_control_size;

constant integer postbus_command_source_gip_tx_length_start = postbus_command_source_start;
constant integer postbus_command_source_gip_tx_signal_start = postbus_command_source_gip_tx_length_start+5;
constant integer postbus_command_source_io_length_start = postbus_command_source_start;
constant integer postbus_command_source_io_cmd_op_start = postbus_command_source_io_length_start+5;

constant integer postbus_command_target_gip_rx_fifo_start = postbus_command_target_start;                    //  the receive FIFO to receive into
constant integer postbus_command_target_gip_rx_semaphore_start = postbus_command_target_gip_rx_fifo_start+2; //  the semaphore to signal once completely received

constant integer postbus_command_target_io_dest_type_start = postbus_command_target_start;               // 2 bits of the destination type; general config, fifo config, data, command
constant integer postbus_command_target_io_dest_start = postbus_command_target_io_dest_type_start+2;     // 5 bits of write address OR ...
constant integer postbus_command_target_io_fifo_start = postbus_command_target_io_dest_type_start+2;     // 2 bits of fifo number
constant integer postbus_command_target_io_ingress = postbus_command_target_io_fifo_start+2;             // 1 bit for tx/rx (ingress)
constant integer postbus_command_target_io_cmd_status = postbus_command_target_io_ingress+1;             // 1 bit for cmd status / data
