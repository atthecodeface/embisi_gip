/*t postbus
    3 2        21        10        0
    10987654321098765432109876543210
    @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ - transaction header
    ................?.......RRRRRRRL - interpreted by routing fabric
    .....sssss......?LLLLL.......... - interpreted by GIP transmit
    SSSSS.....ffffFF?.....rr........ - interpreted by GIP receive
    DDDDD.....ffffFF?.....dd........ - interpreted by IO interface
 */
enum
{
    postbus_command_last_bit = 0, // bit 0 of first transaction word indicates its also the last
    postbus_command_route_start = 1, // bits 1-7 of first transaction word indicates the destination (routing information) for the transaction
    postbus_command_rx_fifo_start = 8, // bit 8-9 of first transaction word indicates the receive FIFO to receive into
    postbus_command_io_dest_type_start = 8, // bit 8-9 of first transaction word indicates the type of destination - 00=>channel, 01=>data FIFO, 10=>command FIFO, 11=>IO interface config
    postbus_command_tx_length_start = 10, // bits 10-14 of first transaction word indicates the length (to GIP)
    postbus_command_flow_type_start = 15, // bit 15-16 of first transaction word indicates the flow control action to do on receive (00=>write, 01=>OR, 10=>add)
    postbus_command_flow_value_start = 17, // bit 17-20 of first transaction word indicates the data for the flow control action
    postbus_command_spare = 21, // bit 21 of first transaction word spare
    postbus_command_tx_signal_start = 22, // bit 22-26 of command (first transcation word out) indicates (if nonzero) the semaphore to set on completion of push to postbus
    postbus_command_rx_signal_start = 27, // bit 27-31 of first transaction word indicates (if nonzero) the semaphore to set on completion of push postbus
    postbus_command_io_dest_start = 27, // bit 27-31 of first transaction word indicates the channel or data/command FIFO an IO transaction word is for
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


