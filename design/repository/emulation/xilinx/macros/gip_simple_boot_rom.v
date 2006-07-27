//a Module boot_rom_wrapper

module gip_simple_boot_rom
(
    rom_clock,

    rom_address,
    rom_read,
    rom_reset,

    rom_read_data
);

    //b Clocks
    input rom_clock;

    //b Inputs
    input [11:0]rom_address;
    input rom_read;
    input rom_reset;

    //b Outputs
    output [31:0]rom_read_data;

//    brw boot_rom_wrapper_inst( .rom_clock(rom_clock), .rom_address(rom_address), .rom_read_data(rom_read_data), .rom_read(rom_read));
    boot_rom boot_rom_wrapper_inst( .clk(rom_clock), .addr(rom_address), .dout(rom_read_data), .en(rom_read) );
endmodule
