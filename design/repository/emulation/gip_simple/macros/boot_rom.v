module boot_rom ( clk, address, data, enable ); // synthesis black_box

input clk;
input enable;
input [11:0] address;
output [31:0] data;

// synopsys translate_off
    // Parameters are:
    //  address width
    //   default data value
    //   depth
    //  enable_rlocs
    //   has_default_data
    //   has_din (writable)
    //  has_en (has enable)
    //  has_limit_data_pitch
    //   has_nd
    //   has_rdy
    //   has_rfd
    //   has_sinit
    //   has_we
    //   limit_data_pitch
    //  mem init file (binary MIF)
    //  pipe stages
    //   reg inputs
    //  sinit value
    //  width
    //  write mode
    //  bottom_addr
    //  clk_is_rising
    //  hierarchy
    //  make_bmm
    //  primitive type
    //  sinit_is_high
    //  top_addr
    //  use_single_primitive
    //  we_is_high
    //  disable_warnings
	BLKMEMSP_V6_1 #( 12, "0", 4096,
                     0, 0, 0,
                     1,
                     0,0, 0, 0, 0, 0, 18,
                     "boot_rom.mif",
                     0, 1,
                     "0",
                     32,
                    0, "0", 1, 1, "hierarchy1", 0, "16kx1", 1, "4096", 0, 1, 1 )
            boot_rom( .CLK(clk),
                      .EN(enable),
                      .ADDR(address),
                      .DOUT(data) );
// synopsys translate_on

// box_type "black_box"
// synthesis attribute box_type of boot_rom is "black_box"

endmodule

