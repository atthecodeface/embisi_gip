typedef enum [4]
{
    io_tx_data_fifo_cmd_read_fifo,
    io_tx_data_fifo_cmd_commit_fifo,
    io_tx_data_fifo_cmd_read_and_commit_fifo,
    io_tx_data_fifo_cmd_revert_fifo,
} t_io_tx_data_fifo_cmd;

typedef enum [4]
{
    io_rx_data_fifo_cmd_write_fifo,
} t_io_rx_data_fifo_cmd;

typedef enum [4]
{
    io_cmd_fifo_cmd_read_fifo,
} t_io_cmd_fifo_cmd;

typedef enum [4]
{
    io_status_fifo_cmd_write_fifo,
} t_io_status_fifo_cmd;
