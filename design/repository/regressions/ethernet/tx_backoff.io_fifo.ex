# Simple test,
printf "Running a collision timing test; max retries is 15, set collisions in sinks externally; calculates average transmit time for 20 packets"

int pkt_time
int value

int total_time
int packet_number
int packet_data
int max_packets

sub max_packets env_int("total_packets") 1
set packet_data 0x01020304
for packet_number 0 max_packets 1

    fifo_cmd_add 0 0 0x300f0010
    fifo_tx_data_add 0 1 0x00001234

    fifo_tx_data_add 0 1 packet_data
    add packet_data packet_data 0x04040404
    bne packet_data 0xfdff0000 skip_1
    set packet_data 0xfdfeff00
skip_1:    
    bne packet_data 0x02030304 skip_1b
    set packet_data 0x01020304
skip_1b:    
    fifo_tx_data_add 0 1 packet_data
    add packet_data packet_data 0x04040404
    bne packet_data 0xfdff0000 skip_2
    set packet_data 0xfdfeff00
skip_2:    
    bne packet_data 0x02030304 skip_2b
    set packet_data 0x01020304
skip_2b:    
    fifo_tx_data_add 0 1 packet_data
    add packet_data packet_data 0x04040404
    bne packet_data 0xfdff0000 skip_3
    set packet_data 0xfdfeff00
skip_3:    
    bne packet_data 0x02030304 skip_3b
    set packet_data 0x01020304
skip_3b:    


    wait_until_status_ne 0
    fifo_status_read 0 pkt_time value

next packet_number

set total_time pkt_time

printf "Total time %d0%" total_time
printf "Total time is made up of nc*np*(failed_tx_time + expected_backoff) + np*success_tx_time"
printf "Success time for 20 4 word packets is 1320 cycles, so 66 cycles each (with 0 deferral counter)"
printf "Total time for 1 collision for 20 4 word packets is 5118 cycles, so about 190 cycles for failed_tx+expected_backoff(1); expected_backoff should be 256 on average, but we have 14 short, 6 long"
printf "This makes the failed_tx*20+6*expected_backoff = 3800 cycles, approx, with expected_backoff of 512 cycles, so 6*exp=3072, failed_tx = 36 (approx)"
printf "So, run for more packets for 1 collision"
printf "Expected backoff for 2 collisions is 0.5*512 + 0.5*1024+0.5*512, or 0.5*2048 = 1024"
printf "Expected backoff for 3 collisions is 0.5*512 + 0.5*1024+0.5*512 + 0.5*2048+0.5*1024+0.5*512, or 0.5*5632 = 2816"
printf "Measured 0 backoff 1 collision is 105 cycles, 1 backoff 1 collision is 608 cycles; difference of 503 cycles."
printf "Note 14*105+6*608 is 5118."
printf "Successful tx is 66, so 105 is 66 success plus 39 failure"
printf "NP    NC   Total time     Total time/packet Calculated ave backoff/packet (=total_time/np-nc*failed_tx(39)-success_time(66))    Expected ave backoff/packet"
printf "20    0        1320                66           0                                                                                     0"
printf "1000  0       66000                66           0                                                                                     0"
printf "20    1        5118               256          151                                                                                  256"
printf "80    1       30532               381          276                                                                                  256"
printf "200   1       77839               389          284                                                                                  256"
printf "1000  1      398249               398          293                                                                                  256"
printf "10000 1     4010155               401          296                                                                                  256"
printf "100   2      104007              1040          896                                                                                 1024"
printf "100   3      330760              3307         3124                                                                                 2816"
printf "100   4      638014              6380         6158                                                                                 6656"
printf "100   5     1604023             16040        15779                                                                                14592"
pass global_cycle() "All packets received correctly (unless failures were reported)"

end
