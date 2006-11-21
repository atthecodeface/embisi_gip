module rf_2r_1w_32_32_fc
(
    rf_clock,
    rf_fast_clock,
    rf_fast_clock_phase, // high at rising rf_fast_clock 'simultaneously' with rising rf_clock

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
    input rf_fast_clock;
    input rf_fast_clock_phase;

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

    //b Phase delay shift register - phase_delay[0] is asserted on first rising rf_fast_clock after rf_clock, phase_delay[1] one cycle later
    reg [1:0]phase_delay;
    always @(posedge rf_fast_clock)
    begin
        phase_delay[0] <= rf_fast_clock_phase;
        phase_delay[1] <= phase_delay[0];
    end

    //b We use a dual port RAM with writes 'simultaneous' with rf_clock, and the two reads in successive cycles
    wire [4:0]addra;
    assign addra = rf_fast_clock_phase ? rf_wr_addr : rf_rd_addr_0;
    dp_32_32_core core( .clka(rf_fast_clock),
                         .ena(rf_fast_clock_phase | phase_delay[0]),
                         .wea(rf_fast_clock_phase & rf_wr_enable),
                         .addra(addra),
                         .dina(rf_wr_data),
                         .douta(rf_rd_data_0),
                        .clkb(rf_fast_clock),
                         .enb(phase_delay[0]),
                         .web(1'b0),
                         .addrb(rf_rd_addr_1),
                         .dinb(32'b0),
                         .doutb(rf_rd_data_1) );
    
endmodule
