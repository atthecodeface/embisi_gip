module rf_1r_1w_16_4
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
    input [3:0]rf_rd_addr_0;
    input rf_wr_enable;
    input [3:0]rf_wr_addr;
    input [3:0]rf_wr_data;

    //b Outputs
    output [3:0]rf_rd_data_0;

    reg [3:0]rf[15:0];

    reg [3:0]rf_rd_data_0;

    always @(posedge rf_clock)
    begin
        if (rf_wr_enable)
        begin
            rf[rf_wr_addr] <= rf_wr_data;
        end
    end

    always @(rf_rd_addr_0)
    begin
        rf_rd_data_0 = rf[rf_rd_addr_0];
    end
    
endmodule
