module memory_s_sp_4096_x_4b8(
    sram_clock, 
    sram_read,
    sram_write,
    sram_byte_enables,
    sram_address,
    sram_write_data,
    sram_read_data );

    input sram_clock;
    input sram_read;
    input sram_write;
    input [3:0] sram_byte_enables;
    input [11:0] sram_address;
    input [31:0] sram_write_data;
    output [31:0] sram_read_data;


    memory_s_sp_4096_x_8 byte_0( .sram_clock(sram_clock),
                                 .sram_read(sram_read),
                                 .sram_write(sram_write && sram_byte_enables[0]),
                                 .sram_address(sram_address),
                                 .sram_write_data(sram_write_data[7:0]),
                                 .sram_read_data(sram_read_data[7:0]) );
    memory_s_sp_4096_x_8 byte_1( .sram_clock(sram_clock),
                                 .sram_read(sram_read),
                                 .sram_write(sram_write && sram_byte_enables[1]),
                                 .sram_address(sram_address),
                                 .sram_write_data(sram_write_data[15:8]),
                                 .sram_read_data(sram_read_data[15:8]) );
    memory_s_sp_4096_x_8 byte_2( .sram_clock(sram_clock),
                                 .sram_read(sram_read),
                                 .sram_write(sram_write && sram_byte_enables[2]),
                                 .sram_address(sram_address),
                                 .sram_write_data(sram_write_data[23:16]),
                                 .sram_read_data(sram_read_data[23:16]) );
    memory_s_sp_4096_x_8 byte_3( .sram_clock(sram_clock),
                                 .sram_read(sram_read),
                                 .sram_write(sram_write && sram_byte_enables[3]),
                                 .sram_address(sram_address),
                                 .sram_write_data(sram_write_data[31:24]),
                                 .sram_read_data(sram_read_data[31:24]) );
endmodule
