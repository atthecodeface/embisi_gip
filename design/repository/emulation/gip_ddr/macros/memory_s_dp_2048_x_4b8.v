module memory_s_sp_2048_x_4b8(
     sram_clock, 
    sram_read_not_write,
    sram_write_enable,
    sram_address,
    sram_write_data,
    sram_read_data );

    input sram_clock;
    input sram_read_not_write;
    input [3:0] sram_write_enable;
    input [10:0] sram_address;
    input [31:0] sram_write_data;
    output [31:0] sram_read_data;

reg[31:0] memory[2047:0];
reg read;
reg[10:0] address;
reg[31:0] sram_read_data;
always @(posedge sram_clock)
begin
    read <= sram_read_not_write;
    address <= sram_address;

    if (!sram_read_not_write)
    begin
        if (sram_write_enable[0])
        begin
            memory[sram_write_address][ 7: 0] <= sram_write_data[ 7: 0];
        end
        if (sram_write_enable[1])
        begin
            memory[sram_write_address][15: 8] <= sram_write_data[15: 8];
        end
        if (sram_write_enable[2])
        begin
            memory[sram_write_address][23:16] <= sram_write_data[23:16];
        end
        if (sram_write_enable[3])
        begin
            memory[sram_write_address][31:24] <= sram_write_data[31:24];
        end
    end
end

always @(read or address ) // or memory)
begin
    sram_read_data = 0;
    if (read)
    begin
        sram_read_data = memory[rd_addr];
    end
end

endmodule
