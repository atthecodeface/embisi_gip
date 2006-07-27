module rf_1r_1w_32_32
(
    rf_clock,

    rf_reset,

    rf_rd_addr_0,
    rf_rd_data_0,

    rf_wr_enable,
    rf_wr_addr,
    rf_wr_data
);

    //b Clocks
    input rf_clock;

    //b Inputs
    input rf_reset;
    input [4:0]rf_rd_addr_0;
    input rf_wr_enable;
    input [4:0]rf_wr_addr;
    input [31:0]rf_wr_data;

    //b Outputs
    output [31:0]rf_rd_data_0;

rf_1r_1w_32_32_core core( .clka(rf_clock), .addra(rf_rd_addr_0), .douta(rf_rd_data_0),
                                .clkb(rf_clock), .addrb(rf_wr_addr), .dinb(rf_wr_data), .web(rf_wr_enable) );

endmodule
