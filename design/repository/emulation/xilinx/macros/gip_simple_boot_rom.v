//a Module boot_rom_wrapper
// We fake out xst with this declaration of boot_rom - hopefully the real one gets put in
module boot_rom ( addr, clk, dout, en );
    input [11:0]addr;
    input clk, en;
    output [31:0]dout;
endmodule

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
