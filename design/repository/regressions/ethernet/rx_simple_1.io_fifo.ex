# Simple test,
printf "Running first simple test, check first 4 packets of length header+CRC, fixed header, incrementing data contents from 1, no errors"

int dummy

wait_until_status_ne 0
fifo_status_check 0 0 0x000d000d
fifo_rx_data_check 0 1 0x00001234
fifo_rx_data_check 0 1 0x01020304
fifo_rx_data_check_x 0 1 0x0ff000000 0x05000000
fifo_rx_data_read 0 dummy

wait_until_status_ne 0
fifo_status_check 0 0 0x000d000d
fifo_rx_data_check 0 1 0x00001234
fifo_rx_data_check 0 1 0x06070809
fifo_rx_data_check_x 0 1 0x0ff000000 0x0a000000
fifo_rx_data_read 0 dummy

wait_until_status_ne 0
fifo_status_check 0 0 0x000d000d
fifo_rx_data_check 0 1 0x00001234
fifo_rx_data_check 0 1 0x0b0c0d0e
fifo_rx_data_check_x 0 1 0x0ff000000 0x0f000000
fifo_rx_data_read 0 dummy

wait_until_status_ne 0
fifo_status_check 0 0 0x000d000d
fifo_rx_data_check 0 1 0x00001234
fifo_rx_data_check 0 1 0x10111213
fifo_rx_data_check_x 0 1 0x0ff000000 0x14000000
fifo_rx_data_read 0 dummy

pass global_cycle() "All packets received correctly (unless failures were reported)"

end
