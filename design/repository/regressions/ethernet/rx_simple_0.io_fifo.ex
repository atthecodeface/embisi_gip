# Simple test,
printf "Running first simple test, check first 4 packets of length header+CRC, fixed header, incrementing data contents from 1, no errors"

int dummy

wait_until_status_ne 0
fifo_status_check 0 0 0x000c000c
fifo_rx_data_check 0 1 0x00001234
fifo_rx_data_check 0 1 0x01020304
fifo_rx_data_read 0 dummy

wait_until_status_ne 0
fifo_status_check 0 0 0x000c000c
fifo_rx_data_check 0 1 0x00001234
fifo_rx_data_check 0 1 0x05060708
fifo_rx_data_read 0 dummy

wait_until_status_ne 0
fifo_status_check 0 0 0x000c000c
fifo_rx_data_check 0 1 0x00001234
fifo_rx_data_check 0 1 0x090a0b0c
fifo_rx_data_read 0 dummy

wait_until_status_ne 0
fifo_status_check 0 0 0x000c000c
fifo_rx_data_check 0 1 0x00001234
fifo_rx_data_check 0 1 0x0d0e0f10
fifo_rx_data_read 0 dummy

pass global_cycle() "All packets received correctly (unless failures were reported)"

end
