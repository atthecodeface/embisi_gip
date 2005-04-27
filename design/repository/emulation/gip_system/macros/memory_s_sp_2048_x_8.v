module memory_s_sp_2048_x_8(
     sram_clock, 
    sram_read,
    sram_write,
    sram_address,
    sram_write_data,
    sram_read_data );

    input sram_clock;
    input sram_read;
    input sram_write;
    input [10:0] sram_address;
    input [7:0] sram_write_data;
    output [7:0] sram_read_data;

reg[7:0] memory[2047:0];
reg read;
reg[10:0] address;
reg[7:0] sram_read_data;
always @(posedge sram_clock)
begin
    read <= sram_read;
    address <= sram_address;

    if (sram_write)
    begin
        memory[sram_address] <= sram_write_data;
    end
end

always @(read or address ) // or memory)
begin
    sram_read_data = 0;
    if (read)
    begin
        sram_read_data = memory[address];
    end
end

endmodule
