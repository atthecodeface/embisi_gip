module rf_2r_1w_32_32
(
    rf_clock,

    rf_reset,

    rf_rd_addr_0,
    rf_rd_data_0,

    rf_rd_addr_1,
    rf_rd_data_1,

    rf_wr_enable,
    rf_wr_addr,
    rf_wr_data
);

    //b Clocks
    input rf_clock;

    //b Inputs
    input rf_reset;
    input [4:0]rf_rd_addr_0;
    input [4:0]rf_rd_addr_1;
    input rf_wr_enable;
    input [4:0]rf_wr_addr;
    input [31:0]rf_wr_data;

    //b Outputs
    output [31:0]rf_rd_data_0;
    output [31:0]rf_rd_data_1;

    reg [31:0]rf[31:0];

    reg [31:0]rf_rd_data_0;
    reg [31:0]rf_rd_data_1;

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
    
    always @(rf_rd_addr_1)
    begin
        rf_rd_data_1 = rf[rf_rd_addr_1];
    end
    
endmodule