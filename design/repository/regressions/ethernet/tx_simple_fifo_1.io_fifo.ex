# Simple test,
printf "Running simple collision test, with 6 packets of fixed length, fixed header, incrementing data contents from 1, second and fourth attemtps collision in preamble, sixth collision in data, seventh attempt ok"

fifo_cmd_add 0 0 0x300f0010
fifo_tx_data_add 0 4 0x00001234 0x01020304 0x05060708 0x090a0b0c

fifo_cmd_add 0 0 0x300f0010
fifo_tx_data_add 0 4 0x00001234 0x0d0e0f10 0x11121314 0x15161718

fifo_cmd_add 0 0 0x300f0010
fifo_tx_data_add 0 4 0x00001234 0x191a1b1c 0x1d1e1f20 0x21222324

int packet_number
set packet_number 0
wait_until_status_ne 0
printf "Packet %d1% %d0%" global_cycle() packet_number
add packet_number packet_number 1
fifo_status_check 0 66 0x30000010

wait_until_status_ne 0
printf "Packet %d1% %d0%" global_cycle() packet_number
add packet_number packet_number 1
fifo_status_check 0 0 0x30010010

wait_until_status_ne 0
printf "Packet %d1% %d0%" global_cycle() packet_number
add packet_number packet_number 1
fifo_status_check 0 0 0x30010010

pass global_cycle() "All packets received correctly (unless failures were reported)"
end
