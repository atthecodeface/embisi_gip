# Simple test,
printf "Running repeated collision test, one packet which should get through after 5 collisions"

fifo_cmd_add 0 0 0x300f0010
fifo_tx_data_add 0 4 0x00001234 0x01020304 0x05060708 0x090a0b0c

end
