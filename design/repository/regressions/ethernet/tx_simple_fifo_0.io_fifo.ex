# Simple test,
printf "Running first simple test, with 4 packets of fixed length, fixed header, incrementing data contents from 1"

fifo_cmd_add 0 0 0x20000010
fifo_tx_data_add 0 4 0x00001234 0x01020304 0x05060708 0x090a0b0c

fifo_cmd_add 0 0 0x20000010
fifo_tx_data_add 0 4 0x00001234 0x0d0e0f10 0x11121314 0x15161718

fifo_cmd_add 0 0 0x20000010
fifo_tx_data_add 0 4 0x00001234 0x191a1b1c 0x1d1e1f20 0x21222324

fifo_cmd_add 0 0 0x20000010
fifo_tx_data_add 0 4 0x00001234 0x25262728 0x292a2b2c 0x2d2e2f30

wait_until_status_ne 0
fifo_status_check 0 0 0x20000010
wait_until_status_ne 0
fifo_status_check 0 0 0x20000010
wait_until_status_ne 0
fifo_status_check 0 0 0x20000010
wait_until_status_ne 0
fifo_status_check 0 0 0x20000011

pass global_cycle() "All packets received correctly (unless failures were reported)"

end
