#define    postbus_command_last_bit (0)
#define    postbus_command_route_start (1)
#define    postbus_command_source_start (postbus_command_route_start + 7)
#define    postbus_command_flow_control_start (postbus_command_source_start + 10)
#define    postbus_command_target_start (postbus_command_flow_control_start + 7)

#define    postbus_command_source_gip_tx_length_start (postbus_command_source_start)
#define    postbus_command_source_gip_tx_signal_start (postbus_command_source_gip_tx_length_start+5)
#define    postbus_command_source_io_length_start     (postbus_command_source_start)
#define    postbus_command_source_io_cmd_op_start     (postbus_command_source_io_length_start+5)

#define    postbus_command_target_gip_rx_fifo_start      (postbus_command_target_start)
#define    postbus_command_target_gip_rx_semaphore_start (postbus_command_target_gip_rx_fifo_start+2)

#define    postbus_command_target_io_dest_type_start (postbus_command_target_start)
#define    postbus_command_target_io_dest_start      (postbus_command_target_io_dest_type_start+2)
#define    postbus_command_target_io_fifo_start      (postbus_command_target_io_dest_type_start+2)
#define    postbus_command_target_io_ingress         (postbus_command_target_io_fifo_start+2)
#define    postbus_command_target_io_cmd_status      (postbus_command_target_io_ingress+1)
