//a Modules needed
//m ddr_in_sdr_out_pin
module ddr_in_sdr_out_pin
(
    reset,
    output_clock,
    next_oe, next_d,
    input_clock,
    d_in_low, d_in_high,
    pad
);
input reset;
input output_clock;
input next_oe, next_d;
input input_clock;
output d_in_low, d_in_high;
inout pad;

IFDDRCPE  dqi( .CLR(reset),
               .PRE(1'b0),
               .CE(1'b1),
               .D(pad),
               .C0(input_clock),
               .C1(!input_clock),
               .Q0( d_in_high ),
               .Q1( d_in_low ) );

reg d_keep;
reg oe_n;
always @(posedge output_clock) begin oe_n <= !next_oe; end
always @(posedge output_clock or posedge reset ) begin if (reset) d_keep<=0; else d_keep <= next_d; end
// synthesis attribute equivalent_register_removal of oe_n is "no" ;
OFDDRTCPE dqo( .CLR(reset),
               .PRE(1'b0),
               .CE(1'b1),
               .D0(next_d),
               .D1(d_keep),
               .C0(output_clock),
               .C1(!output_clock),
               .T( oe_n ),
               .O( pad ) );
endmodule

//m ddr_dram_clk_pad
module ddr_dram_clk_pad
(
    output_clock,
    pad_p,
    pad_n
);
input output_clock;
output pad_p;
output pad_n;

OFDDRTCPE pos_pad( .CLR(1'b0),
                    .PRE(1'b0),
                    .CE(1'b1),
                    .D0(1'b1),
                    .D1(1'b0),
                    .C0(output_clock),
                    .C1(!output_clock),
                    .T( 1'b0 ),
                    .O( pad_p ) );

OFDDRTCPE neg_pad( .CLR(reset),
                    .PRE(1'b0),
                    .CE(1'b1),
                    .D0(1'b0),
                    .D1(1'b1),
                    .C0(output_clock),
                    .C1(!output_clock),
                    .T( 1'b0 ),
                    .O( pad_n ) );
endmodule

//a FPGA wrapper module
module fpga_wrapper
(
    sys_drm_clock_in,

    switches,
    rxd,

    txd,

    eb_ce_n,
    eb_oe_n,
    eb_we_n,
    eb_address,
    eb_data,

    ddr_dram_clk_0,
    ddr_dram_clk_0_n,
    ddr_dram_clk_1,
    ddr_dram_clk_1_n,

    ddr_dram_cke,
    ddr_dram_s_n,
    ddr_dram_ras_n,
    ddr_dram_cas_n,
    ddr_dram_we_n,
    ddr_dram_ba,
    ddr_dram_a,

    ddr_dram_dq,
    ddr_dram_dqm,
    ddr_dram_dqs,

    eth_rst_n,
    mii_rx_clk,
    mii_tx_clk,
    mii_crs,
    mii_col,
    mii_txd,
    mii_tx_en,
    mii_tx_er,
    mii_rxd,
    mii_rx_dv,
    mii_rx_er,
    mii_mdio,
    mii_mdc,

    leds,

    leds_dual

//    analyzer_clock,
//    analyzer_signals
);

//b Inputs and outputs
    //b Clocks
    input sys_drm_clock_in;

    //b Switches
    input [7:0]switches;

    //b UART
    input rxd;
    output txd;

    //b External bus
    output [3:0]eb_ce_n;
    output eb_oe_n;
    output eb_we_n;
    output [23:0]eb_address;
    inout [31:0]eb_data;

    //b Dram
    output ddr_dram_clk_0;
    output ddr_dram_clk_0_n;
    output ddr_dram_clk_1;
    output ddr_dram_clk_1_n;

    output [1:0]ddr_dram_cke;
    output [1:0]ddr_dram_s_n;
    output ddr_dram_ras_n;
    output ddr_dram_cas_n;
    output ddr_dram_we_n;
    output [1:0]ddr_dram_ba;
    output [12:0]ddr_dram_a;

    inout [63:0]ddr_dram_dq;
    output [3:0]ddr_dram_dqm;
    inout [7:0]ddr_dram_dqs;

    //b Ethernet
    output eth_rst_n;
    input mii_rx_clk;
    input mii_tx_clk;
    input mii_crs;
    input mii_col;
    output [3:0]mii_txd;
    output mii_tx_en;
    output mii_tx_er;
    input [3:0]mii_rxd;
    input mii_rx_dv;
    input mii_rx_er;
    inout mii_mdio;
    output mii_mdc;

    //b Outputs
    output [7:0]leds;
    output [7:0]leds_dual;
//    output analyzer_clock;
//    output [31:0]analyzer_signals;
wire analyzer_clock;

//b Reset and clock signals/pins
wire int_double_drm_clock_buffered;
wire int_drm_clock_phase;
wire int_drm_clock_buffered;
wire int_drm_clock_90_buffered;

wire system_reset_in;
wire system_reset; // from the clock generator, once the DLLs have settled
wire system_reset_out; // from the CPU, once the DRAM has initialized (so cannot be used for DRAM address or control lines, but it can be used for DRAM data)
assign system_reset_in = switches[7];
wire [7:0]gip_leds_out;

//synthesis attribute shreg_extract of fpga_wrapper is no;

//b Expansion bus mappings to/from internal signals
wire [31:0]ext_bus_read_data;
wire [3:0]ext_bus_ce;
wire [31:0]ext_bus_write_data;
wire ext_bus_write_data_enable;
wire ext_bus_oe;
wire ext_bus_we;
wire [23:0]ext_bus_address;

assign eb_ce_n[0] = !ext_bus_ce[0];
assign eb_ce_n[1] = !ext_bus_ce[1];
assign eb_ce_n[2] = !ext_bus_ce[2];
assign eb_ce_n[3] = !ext_bus_ce[3];
assign eb_we_n = !ext_bus_we;
assign eb_oe_n = !ext_bus_oe;
assign eb_address = ext_bus_address;
assign eb_data = ext_bus_write_data_enable ? ext_bus_write_data : 32'bz; //'
assign ext_bus_read_data = eb_data;

//b DRAM clock outputs
ddr_dram_clk_pad ddr_clk0( .output_clock(int_drm_clock_buffered), .pad_p( ddr_dram_clk_0 ), .pad_n( ddr_dram_clk_0_n ) );
ddr_dram_clk_pad ddr_clk1( .output_clock(int_drm_clock_buffered), .pad_p( ddr_dram_clk_1 ), .pad_n( ddr_dram_clk_1_n ) );

//b DRAM control outputs: cke, s_n, ras_n, cas_n, we_n, ba, a
wire [1:0]ddr_dram_cke;
reg [1:0]ddr_dram_s_n;
reg ddr_dram_ras_n;
reg ddr_dram_cas_n;
reg ddr_dram_we_n;
reg [1:0]ddr_dram_ba;
reg [12:0]ddr_dram_a;

wire drm_next_cke;
wire [1:0]drm_next_s_n;
wire drm_next_ras_n;
wire drm_next_cas_n;
wire drm_next_we_n;
wire [1:0]drm_next_ba;
wire [12:0]drm_next_a;

reg drm_hold_cke;
reg [1:0]drm_hold_s_n;
reg drm_hold_ras_n;
reg drm_hold_cas_n;
reg drm_hold_we_n;
reg [1:0]drm_hold_ba;
reg [12:0]drm_hold_a;
reg drm_cke_0;
reg drm_cke_1;
always @(posedge int_drm_clock_buffered or posedge system_reset_in)
begin
    if (system_reset_in)
    begin
        drm_hold_cke <= 0;
        drm_cke_0 <= 0;
        drm_cke_1 <= 0;
        drm_hold_s_n <= 0;
        drm_hold_ras_n <= 0;
        drm_hold_cas_n <= 0;
        drm_hold_we_n <= 0;
        drm_hold_ba <= 0;
        drm_hold_a <= 0;
    end
    else
    begin
        drm_hold_cke <= drm_next_cke;
        drm_cke_0 <= drm_next_cke; // put these back on the unphase shifted clock - its okay, this is a slow output; it needs the same oclk as dqs signals
        drm_cke_1 <= drm_next_cke;
        drm_hold_s_n <= drm_next_s_n;
        drm_hold_ras_n <= drm_next_ras_n;
        drm_hold_cas_n <= drm_next_cas_n;
        drm_hold_we_n <= drm_next_we_n;
        drm_hold_ba <= drm_next_ba;
        drm_hold_a <= drm_next_a;
    end
end

always @(posedge int_drm_clock_90_buffered or posedge system_reset_in)
begin
    if (system_reset_in)
    begin
        ddr_dram_s_n <= 2'b11; //'
        ddr_dram_ras_n <= 1;
        ddr_dram_cas_n <= 1;
        ddr_dram_we_n <= 1;
        ddr_dram_ba <= 0;
        ddr_dram_a <= 0;
    end
    else
    begin
        ddr_dram_s_n <= drm_hold_s_n;
        ddr_dram_ras_n <= drm_hold_ras_n;
        ddr_dram_cas_n <= drm_hold_cas_n;
        ddr_dram_we_n <= drm_hold_we_n;
        ddr_dram_ba <= drm_hold_ba;
        ddr_dram_a <= drm_hold_a;
    end
end

assign ddr_dram_cke[0] = drm_cke_0;
assign ddr_dram_cke[1] = drm_cke_1;
// synthesis attribute equivalent_register_removal of drm_cke_0 is "no" ;
// synthesis attribute equivalent_register_removal of drm_cke_1 is "no" ;

//b DQM outputs - drive out on drm_clock_buffered
reg [3:0]ddr_dram_dqm;
wire drm_next_dqoe;
wire [63:0]drm_next_dq;
wire [7:0]drm_next_dqm;
always @(posedge int_drm_clock_buffered or posedge system_reset_in)
begin
    if (system_reset_in)
    begin
        ddr_dram_dqm <= 0;
    end
    else
    begin
        ddr_dram_dqm <= drm_next_dqm; // these are not tristate; we own them
    end
end

//b DQ pads - input registered on posedge and negedge 90 input clocks, outputs on drm_clock_buffered as we have a long setup and hold on these (we hold them steady as outputs)
// we don't care about reset too much, but it must match the internals to get the flops colocated - reset only effects the signal values, not the oe
wire [63:0] drm_input_dq_low;
wire [63:0] drm_input_dq_high;

ddr_in_sdr_out_pin dqp_00( .reset(system_reset_out), .output_clock(int_drm_clock_buffered), .next_oe(drm_next_dqoe), .next_d(drm_next_dq[ 0]), .input_clock(int_input_drm_clock_buffered), .d_in_low(drm_input_dq_low[ 0]), .d_in_high(drm_input_dq_high[ 0]), .pad(ddr_dram_dq[ 0]) );
ddr_in_sdr_out_pin dqp_01( .reset(system_reset_out), .output_clock(int_drm_clock_buffered), .next_oe(drm_next_dqoe), .next_d(drm_next_dq[ 1]), .input_clock(int_input_drm_clock_buffered), .d_in_low(drm_input_dq_low[ 1]), .d_in_high(drm_input_dq_high[ 1]), .pad(ddr_dram_dq[ 1]) );
ddr_in_sdr_out_pin dqp_02( .reset(system_reset_out), .output_clock(int_drm_clock_buffered), .next_oe(drm_next_dqoe), .next_d(drm_next_dq[ 2]), .input_clock(int_input_drm_clock_buffered), .d_in_low(drm_input_dq_low[ 2]), .d_in_high(drm_input_dq_high[ 2]), .pad(ddr_dram_dq[ 2]) );
ddr_in_sdr_out_pin dqp_03( .reset(system_reset_out), .output_clock(int_drm_clock_buffered), .next_oe(drm_next_dqoe), .next_d(drm_next_dq[ 3]), .input_clock(int_input_drm_clock_buffered), .d_in_low(drm_input_dq_low[ 3]), .d_in_high(drm_input_dq_high[ 3]), .pad(ddr_dram_dq[ 3]) );
ddr_in_sdr_out_pin dqp_04( .reset(system_reset_out), .output_clock(int_drm_clock_buffered), .next_oe(drm_next_dqoe), .next_d(drm_next_dq[ 4]), .input_clock(int_input_drm_clock_buffered), .d_in_low(drm_input_dq_low[ 4]), .d_in_high(drm_input_dq_high[ 4]), .pad(ddr_dram_dq[ 4]) );
ddr_in_sdr_out_pin dqp_05( .reset(system_reset_out), .output_clock(int_drm_clock_buffered), .next_oe(drm_next_dqoe), .next_d(drm_next_dq[ 5]), .input_clock(int_input_drm_clock_buffered), .d_in_low(drm_input_dq_low[ 5]), .d_in_high(drm_input_dq_high[ 5]), .pad(ddr_dram_dq[ 5]) );
ddr_in_sdr_out_pin dqp_06( .reset(system_reset_out), .output_clock(int_drm_clock_buffered), .next_oe(drm_next_dqoe), .next_d(drm_next_dq[ 6]), .input_clock(int_input_drm_clock_buffered), .d_in_low(drm_input_dq_low[ 6]), .d_in_high(drm_input_dq_high[ 6]), .pad(ddr_dram_dq[ 6]) );
ddr_in_sdr_out_pin dqp_07( .reset(system_reset_out), .output_clock(int_drm_clock_buffered), .next_oe(drm_next_dqoe), .next_d(drm_next_dq[ 7]), .input_clock(int_input_drm_clock_buffered), .d_in_low(drm_input_dq_low[ 7]), .d_in_high(drm_input_dq_high[ 7]), .pad(ddr_dram_dq[ 7]) );
ddr_in_sdr_out_pin dqp_08( .reset(system_reset_out), .output_clock(int_drm_clock_buffered), .next_oe(drm_next_dqoe), .next_d(drm_next_dq[ 8]), .input_clock(int_input_drm_clock_buffered), .d_in_low(drm_input_dq_low[ 8]), .d_in_high(drm_input_dq_high[ 8]), .pad(ddr_dram_dq[ 8]) );
ddr_in_sdr_out_pin dqp_09( .reset(system_reset_out), .output_clock(int_drm_clock_buffered), .next_oe(drm_next_dqoe), .next_d(drm_next_dq[ 9]), .input_clock(int_input_drm_clock_buffered), .d_in_low(drm_input_dq_low[ 9]), .d_in_high(drm_input_dq_high[ 9]), .pad(ddr_dram_dq[ 9]) );
ddr_in_sdr_out_pin dqp_10( .reset(system_reset_out), .output_clock(int_drm_clock_buffered), .next_oe(drm_next_dqoe), .next_d(drm_next_dq[10]), .input_clock(int_input_drm_clock_buffered), .d_in_low(drm_input_dq_low[10]), .d_in_high(drm_input_dq_high[10]), .pad(ddr_dram_dq[10]) );
ddr_in_sdr_out_pin dqp_11( .reset(system_reset_out), .output_clock(int_drm_clock_buffered), .next_oe(drm_next_dqoe), .next_d(drm_next_dq[11]), .input_clock(int_input_drm_clock_buffered), .d_in_low(drm_input_dq_low[11]), .d_in_high(drm_input_dq_high[11]), .pad(ddr_dram_dq[11]) );
ddr_in_sdr_out_pin dqp_12( .reset(system_reset_out), .output_clock(int_drm_clock_buffered), .next_oe(drm_next_dqoe), .next_d(drm_next_dq[12]), .input_clock(int_input_drm_clock_buffered), .d_in_low(drm_input_dq_low[12]), .d_in_high(drm_input_dq_high[12]), .pad(ddr_dram_dq[12]) );
ddr_in_sdr_out_pin dqp_13( .reset(system_reset_out), .output_clock(int_drm_clock_buffered), .next_oe(drm_next_dqoe), .next_d(drm_next_dq[13]), .input_clock(int_input_drm_clock_buffered), .d_in_low(drm_input_dq_low[13]), .d_in_high(drm_input_dq_high[13]), .pad(ddr_dram_dq[13]) );
ddr_in_sdr_out_pin dqp_14( .reset(system_reset_out), .output_clock(int_drm_clock_buffered), .next_oe(drm_next_dqoe), .next_d(drm_next_dq[14]), .input_clock(int_input_drm_clock_buffered), .d_in_low(drm_input_dq_low[14]), .d_in_high(drm_input_dq_high[14]), .pad(ddr_dram_dq[14]) );
ddr_in_sdr_out_pin dqp_15( .reset(system_reset_out), .output_clock(int_drm_clock_buffered), .next_oe(drm_next_dqoe), .next_d(drm_next_dq[15]), .input_clock(int_input_drm_clock_buffered), .d_in_low(drm_input_dq_low[15]), .d_in_high(drm_input_dq_high[15]), .pad(ddr_dram_dq[15]) );
ddr_in_sdr_out_pin dqp_16( .reset(system_reset_out), .output_clock(int_drm_clock_buffered), .next_oe(drm_next_dqoe), .next_d(drm_next_dq[16]), .input_clock(int_input_drm_clock_buffered), .d_in_low(drm_input_dq_low[16]), .d_in_high(drm_input_dq_high[16]), .pad(ddr_dram_dq[16]) );
ddr_in_sdr_out_pin dqp_17( .reset(system_reset_out), .output_clock(int_drm_clock_buffered), .next_oe(drm_next_dqoe), .next_d(drm_next_dq[17]), .input_clock(int_input_drm_clock_buffered), .d_in_low(drm_input_dq_low[17]), .d_in_high(drm_input_dq_high[17]), .pad(ddr_dram_dq[17]) );
ddr_in_sdr_out_pin dqp_18( .reset(system_reset_out), .output_clock(int_drm_clock_buffered), .next_oe(drm_next_dqoe), .next_d(drm_next_dq[18]), .input_clock(int_input_drm_clock_buffered), .d_in_low(drm_input_dq_low[18]), .d_in_high(drm_input_dq_high[18]), .pad(ddr_dram_dq[18]) );
ddr_in_sdr_out_pin dqp_19( .reset(system_reset_out), .output_clock(int_drm_clock_buffered), .next_oe(drm_next_dqoe), .next_d(drm_next_dq[19]), .input_clock(int_input_drm_clock_buffered), .d_in_low(drm_input_dq_low[19]), .d_in_high(drm_input_dq_high[19]), .pad(ddr_dram_dq[19]) );
ddr_in_sdr_out_pin dqp_20( .reset(system_reset_out), .output_clock(int_drm_clock_buffered), .next_oe(drm_next_dqoe), .next_d(drm_next_dq[20]), .input_clock(int_input_drm_clock_buffered), .d_in_low(drm_input_dq_low[20]), .d_in_high(drm_input_dq_high[20]), .pad(ddr_dram_dq[20]) );
ddr_in_sdr_out_pin dqp_21( .reset(system_reset_out), .output_clock(int_drm_clock_buffered), .next_oe(drm_next_dqoe), .next_d(drm_next_dq[21]), .input_clock(int_input_drm_clock_buffered), .d_in_low(drm_input_dq_low[21]), .d_in_high(drm_input_dq_high[21]), .pad(ddr_dram_dq[21]) );
ddr_in_sdr_out_pin dqp_22( .reset(system_reset_out), .output_clock(int_drm_clock_buffered), .next_oe(drm_next_dqoe), .next_d(drm_next_dq[22]), .input_clock(int_input_drm_clock_buffered), .d_in_low(drm_input_dq_low[22]), .d_in_high(drm_input_dq_high[22]), .pad(ddr_dram_dq[22]) );
ddr_in_sdr_out_pin dqp_23( .reset(system_reset_out), .output_clock(int_drm_clock_buffered), .next_oe(drm_next_dqoe), .next_d(drm_next_dq[23]), .input_clock(int_input_drm_clock_buffered), .d_in_low(drm_input_dq_low[23]), .d_in_high(drm_input_dq_high[23]), .pad(ddr_dram_dq[23]) );
ddr_in_sdr_out_pin dqp_24( .reset(system_reset_out), .output_clock(int_drm_clock_buffered), .next_oe(drm_next_dqoe), .next_d(drm_next_dq[24]), .input_clock(int_input_drm_clock_buffered), .d_in_low(drm_input_dq_low[24]), .d_in_high(drm_input_dq_high[24]), .pad(ddr_dram_dq[24]) );
ddr_in_sdr_out_pin dqp_25( .reset(system_reset_out), .output_clock(int_drm_clock_buffered), .next_oe(drm_next_dqoe), .next_d(drm_next_dq[25]), .input_clock(int_input_drm_clock_buffered), .d_in_low(drm_input_dq_low[25]), .d_in_high(drm_input_dq_high[25]), .pad(ddr_dram_dq[25]) );
ddr_in_sdr_out_pin dqp_26( .reset(system_reset_out), .output_clock(int_drm_clock_buffered), .next_oe(drm_next_dqoe), .next_d(drm_next_dq[26]), .input_clock(int_input_drm_clock_buffered), .d_in_low(drm_input_dq_low[26]), .d_in_high(drm_input_dq_high[26]), .pad(ddr_dram_dq[26]) );
ddr_in_sdr_out_pin dqp_27( .reset(system_reset_out), .output_clock(int_drm_clock_buffered), .next_oe(drm_next_dqoe), .next_d(drm_next_dq[27]), .input_clock(int_input_drm_clock_buffered), .d_in_low(drm_input_dq_low[27]), .d_in_high(drm_input_dq_high[27]), .pad(ddr_dram_dq[27]) );
ddr_in_sdr_out_pin dqp_28( .reset(system_reset_out), .output_clock(int_drm_clock_buffered), .next_oe(drm_next_dqoe), .next_d(drm_next_dq[28]), .input_clock(int_input_drm_clock_buffered), .d_in_low(drm_input_dq_low[28]), .d_in_high(drm_input_dq_high[28]), .pad(ddr_dram_dq[28]) );
ddr_in_sdr_out_pin dqp_29( .reset(system_reset_out), .output_clock(int_drm_clock_buffered), .next_oe(drm_next_dqoe), .next_d(drm_next_dq[29]), .input_clock(int_input_drm_clock_buffered), .d_in_low(drm_input_dq_low[29]), .d_in_high(drm_input_dq_high[29]), .pad(ddr_dram_dq[29]) );
ddr_in_sdr_out_pin dqp_30( .reset(system_reset_out), .output_clock(int_drm_clock_buffered), .next_oe(drm_next_dqoe), .next_d(drm_next_dq[30]), .input_clock(int_input_drm_clock_buffered), .d_in_low(drm_input_dq_low[30]), .d_in_high(drm_input_dq_high[30]), .pad(ddr_dram_dq[30]) );
ddr_in_sdr_out_pin dqp_31( .reset(system_reset_out), .output_clock(int_drm_clock_buffered), .next_oe(drm_next_dqoe), .next_d(drm_next_dq[31]), .input_clock(int_input_drm_clock_buffered), .d_in_low(drm_input_dq_low[31]), .d_in_high(drm_input_dq_high[31]), .pad(ddr_dram_dq[31]) );
ddr_in_sdr_out_pin dqp_32( .reset(system_reset_out), .output_clock(int_drm_clock_buffered), .next_oe(drm_next_dqoe), .next_d(drm_next_dq[32]), .input_clock(int_input_drm_clock_buffered), .d_in_low(drm_input_dq_low[32]), .d_in_high(drm_input_dq_high[32]), .pad(ddr_dram_dq[32]) );
ddr_in_sdr_out_pin dqp_33( .reset(system_reset_out), .output_clock(int_drm_clock_buffered), .next_oe(drm_next_dqoe), .next_d(drm_next_dq[33]), .input_clock(int_input_drm_clock_buffered), .d_in_low(drm_input_dq_low[33]), .d_in_high(drm_input_dq_high[33]), .pad(ddr_dram_dq[33]) );
ddr_in_sdr_out_pin dqp_34( .reset(system_reset_out), .output_clock(int_drm_clock_buffered), .next_oe(drm_next_dqoe), .next_d(drm_next_dq[34]), .input_clock(int_input_drm_clock_buffered), .d_in_low(drm_input_dq_low[34]), .d_in_high(drm_input_dq_high[34]), .pad(ddr_dram_dq[34]) );
ddr_in_sdr_out_pin dqp_35( .reset(system_reset_out), .output_clock(int_drm_clock_buffered), .next_oe(drm_next_dqoe), .next_d(drm_next_dq[35]), .input_clock(int_input_drm_clock_buffered), .d_in_low(drm_input_dq_low[35]), .d_in_high(drm_input_dq_high[35]), .pad(ddr_dram_dq[35]) );
ddr_in_sdr_out_pin dqp_36( .reset(system_reset_out), .output_clock(int_drm_clock_buffered), .next_oe(drm_next_dqoe), .next_d(drm_next_dq[36]), .input_clock(int_input_drm_clock_buffered), .d_in_low(drm_input_dq_low[36]), .d_in_high(drm_input_dq_high[36]), .pad(ddr_dram_dq[36]) );
ddr_in_sdr_out_pin dqp_37( .reset(system_reset_out), .output_clock(int_drm_clock_buffered), .next_oe(drm_next_dqoe), .next_d(drm_next_dq[37]), .input_clock(int_input_drm_clock_buffered), .d_in_low(drm_input_dq_low[37]), .d_in_high(drm_input_dq_high[37]), .pad(ddr_dram_dq[37]) );
ddr_in_sdr_out_pin dqp_38( .reset(system_reset_out), .output_clock(int_drm_clock_buffered), .next_oe(drm_next_dqoe), .next_d(drm_next_dq[38]), .input_clock(int_input_drm_clock_buffered), .d_in_low(drm_input_dq_low[38]), .d_in_high(drm_input_dq_high[38]), .pad(ddr_dram_dq[38]) );
ddr_in_sdr_out_pin dqp_39( .reset(system_reset_out), .output_clock(int_drm_clock_buffered), .next_oe(drm_next_dqoe), .next_d(drm_next_dq[39]), .input_clock(int_input_drm_clock_buffered), .d_in_low(drm_input_dq_low[39]), .d_in_high(drm_input_dq_high[39]), .pad(ddr_dram_dq[39]) );
ddr_in_sdr_out_pin dqp_40( .reset(system_reset_out), .output_clock(int_drm_clock_buffered), .next_oe(drm_next_dqoe), .next_d(drm_next_dq[40]), .input_clock(int_input_drm_clock_buffered), .d_in_low(drm_input_dq_low[40]), .d_in_high(drm_input_dq_high[40]), .pad(ddr_dram_dq[40]) );
ddr_in_sdr_out_pin dqp_41( .reset(system_reset_out), .output_clock(int_drm_clock_buffered), .next_oe(drm_next_dqoe), .next_d(drm_next_dq[41]), .input_clock(int_input_drm_clock_buffered), .d_in_low(drm_input_dq_low[41]), .d_in_high(drm_input_dq_high[41]), .pad(ddr_dram_dq[41]) );
ddr_in_sdr_out_pin dqp_42( .reset(system_reset_out), .output_clock(int_drm_clock_buffered), .next_oe(drm_next_dqoe), .next_d(drm_next_dq[42]), .input_clock(int_input_drm_clock_buffered), .d_in_low(drm_input_dq_low[42]), .d_in_high(drm_input_dq_high[42]), .pad(ddr_dram_dq[42]) );
ddr_in_sdr_out_pin dqp_43( .reset(system_reset_out), .output_clock(int_drm_clock_buffered), .next_oe(drm_next_dqoe), .next_d(drm_next_dq[43]), .input_clock(int_input_drm_clock_buffered), .d_in_low(drm_input_dq_low[43]), .d_in_high(drm_input_dq_high[43]), .pad(ddr_dram_dq[43]) );
ddr_in_sdr_out_pin dqp_44( .reset(system_reset_out), .output_clock(int_drm_clock_buffered), .next_oe(drm_next_dqoe), .next_d(drm_next_dq[44]), .input_clock(int_input_drm_clock_buffered), .d_in_low(drm_input_dq_low[44]), .d_in_high(drm_input_dq_high[44]), .pad(ddr_dram_dq[44]) );
ddr_in_sdr_out_pin dqp_45( .reset(system_reset_out), .output_clock(int_drm_clock_buffered), .next_oe(drm_next_dqoe), .next_d(drm_next_dq[45]), .input_clock(int_input_drm_clock_buffered), .d_in_low(drm_input_dq_low[45]), .d_in_high(drm_input_dq_high[45]), .pad(ddr_dram_dq[45]) );
ddr_in_sdr_out_pin dqp_46( .reset(system_reset_out), .output_clock(int_drm_clock_buffered), .next_oe(drm_next_dqoe), .next_d(drm_next_dq[46]), .input_clock(int_input_drm_clock_buffered), .d_in_low(drm_input_dq_low[46]), .d_in_high(drm_input_dq_high[46]), .pad(ddr_dram_dq[46]) );
ddr_in_sdr_out_pin dqp_47( .reset(system_reset_out), .output_clock(int_drm_clock_buffered), .next_oe(drm_next_dqoe), .next_d(drm_next_dq[47]), .input_clock(int_input_drm_clock_buffered), .d_in_low(drm_input_dq_low[47]), .d_in_high(drm_input_dq_high[47]), .pad(ddr_dram_dq[47]) );
ddr_in_sdr_out_pin dqp_48( .reset(system_reset_out), .output_clock(int_drm_clock_buffered), .next_oe(drm_next_dqoe), .next_d(drm_next_dq[48]), .input_clock(int_input_drm_clock_buffered), .d_in_low(drm_input_dq_low[48]), .d_in_high(drm_input_dq_high[48]), .pad(ddr_dram_dq[48]) );
ddr_in_sdr_out_pin dqp_49( .reset(system_reset_out), .output_clock(int_drm_clock_buffered), .next_oe(drm_next_dqoe), .next_d(drm_next_dq[49]), .input_clock(int_input_drm_clock_buffered), .d_in_low(drm_input_dq_low[49]), .d_in_high(drm_input_dq_high[49]), .pad(ddr_dram_dq[49]) );
ddr_in_sdr_out_pin dqp_50( .reset(system_reset_out), .output_clock(int_drm_clock_buffered), .next_oe(drm_next_dqoe), .next_d(drm_next_dq[50]), .input_clock(int_input_drm_clock_buffered), .d_in_low(drm_input_dq_low[50]), .d_in_high(drm_input_dq_high[50]), .pad(ddr_dram_dq[50]) );
ddr_in_sdr_out_pin dqp_51( .reset(system_reset_out), .output_clock(int_drm_clock_buffered), .next_oe(drm_next_dqoe), .next_d(drm_next_dq[51]), .input_clock(int_input_drm_clock_buffered), .d_in_low(drm_input_dq_low[51]), .d_in_high(drm_input_dq_high[51]), .pad(ddr_dram_dq[51]) );
ddr_in_sdr_out_pin dqp_52( .reset(system_reset_out), .output_clock(int_drm_clock_buffered), .next_oe(drm_next_dqoe), .next_d(drm_next_dq[52]), .input_clock(int_input_drm_clock_buffered), .d_in_low(drm_input_dq_low[52]), .d_in_high(drm_input_dq_high[52]), .pad(ddr_dram_dq[52]) );
ddr_in_sdr_out_pin dqp_53( .reset(system_reset_out), .output_clock(int_drm_clock_buffered), .next_oe(drm_next_dqoe), .next_d(drm_next_dq[53]), .input_clock(int_input_drm_clock_buffered), .d_in_low(drm_input_dq_low[53]), .d_in_high(drm_input_dq_high[53]), .pad(ddr_dram_dq[53]) );
ddr_in_sdr_out_pin dqp_54( .reset(system_reset_out), .output_clock(int_drm_clock_buffered), .next_oe(drm_next_dqoe), .next_d(drm_next_dq[54]), .input_clock(int_input_drm_clock_buffered), .d_in_low(drm_input_dq_low[54]), .d_in_high(drm_input_dq_high[54]), .pad(ddr_dram_dq[54]) );
ddr_in_sdr_out_pin dqp_55( .reset(system_reset_out), .output_clock(int_drm_clock_buffered), .next_oe(drm_next_dqoe), .next_d(drm_next_dq[55]), .input_clock(int_input_drm_clock_buffered), .d_in_low(drm_input_dq_low[55]), .d_in_high(drm_input_dq_high[55]), .pad(ddr_dram_dq[55]) );
ddr_in_sdr_out_pin dqp_56( .reset(system_reset_out), .output_clock(int_drm_clock_buffered), .next_oe(drm_next_dqoe), .next_d(drm_next_dq[56]), .input_clock(int_input_drm_clock_buffered), .d_in_low(drm_input_dq_low[56]), .d_in_high(drm_input_dq_high[56]), .pad(ddr_dram_dq[56]) );
ddr_in_sdr_out_pin dqp_57( .reset(system_reset_out), .output_clock(int_drm_clock_buffered), .next_oe(drm_next_dqoe), .next_d(drm_next_dq[57]), .input_clock(int_input_drm_clock_buffered), .d_in_low(drm_input_dq_low[57]), .d_in_high(drm_input_dq_high[57]), .pad(ddr_dram_dq[57]) );
ddr_in_sdr_out_pin dqp_58( .reset(system_reset_out), .output_clock(int_drm_clock_buffered), .next_oe(drm_next_dqoe), .next_d(drm_next_dq[58]), .input_clock(int_input_drm_clock_buffered), .d_in_low(drm_input_dq_low[58]), .d_in_high(drm_input_dq_high[58]), .pad(ddr_dram_dq[58]) );
ddr_in_sdr_out_pin dqp_59( .reset(system_reset_out), .output_clock(int_drm_clock_buffered), .next_oe(drm_next_dqoe), .next_d(drm_next_dq[59]), .input_clock(int_input_drm_clock_buffered), .d_in_low(drm_input_dq_low[59]), .d_in_high(drm_input_dq_high[59]), .pad(ddr_dram_dq[59]) );
ddr_in_sdr_out_pin dqp_60( .reset(system_reset_out), .output_clock(int_drm_clock_buffered), .next_oe(drm_next_dqoe), .next_d(drm_next_dq[60]), .input_clock(int_input_drm_clock_buffered), .d_in_low(drm_input_dq_low[60]), .d_in_high(drm_input_dq_high[60]), .pad(ddr_dram_dq[60]) );
ddr_in_sdr_out_pin dqp_61( .reset(system_reset_out), .output_clock(int_drm_clock_buffered), .next_oe(drm_next_dqoe), .next_d(drm_next_dq[61]), .input_clock(int_input_drm_clock_buffered), .d_in_low(drm_input_dq_low[61]), .d_in_high(drm_input_dq_high[61]), .pad(ddr_dram_dq[61]) );
ddr_in_sdr_out_pin dqp_62( .reset(system_reset_out), .output_clock(int_drm_clock_buffered), .next_oe(drm_next_dqoe), .next_d(drm_next_dq[62]), .input_clock(int_input_drm_clock_buffered), .d_in_low(drm_input_dq_low[62]), .d_in_high(drm_input_dq_high[62]), .pad(ddr_dram_dq[62]) );
ddr_in_sdr_out_pin dqp_63( .reset(system_reset_out), .output_clock(int_drm_clock_buffered), .next_oe(drm_next_dqoe), .next_d(drm_next_dq[63]), .input_clock(int_input_drm_clock_buffered), .d_in_low(drm_input_dq_low[63]), .d_in_high(drm_input_dq_high[63]), .pad(ddr_dram_dq[63]) );

//b DQS outputs - individual tristate registers, DDR register in the IOBs; can use drm_clock_buffered to drive them out as skew to the clock should be around 0ns
wire [3:0] drm_next_dqs_high;
wire [3:0] drm_next_dqs_low;
reg drm_dqs_oe_n_0;
reg drm_dqs_oe_n_1;
reg drm_dqs_oe_n_2;
reg drm_dqs_oe_n_3;
reg drm_dqs_oe_n_4;
reg drm_dqs_oe_n_5;
reg drm_dqs_oe_n_6;
reg drm_dqs_oe_n_7;
reg [3:0] drm_dqs_low;
always @(posedge int_drm_clock_buffered)
begin
    drm_dqs_low <= drm_next_dqs_low;
    drm_dqs_oe_n_0 <= !drm_next_dqoe;
    drm_dqs_oe_n_1 <= !drm_next_dqoe;
    drm_dqs_oe_n_2 <= !drm_next_dqoe;
    drm_dqs_oe_n_3 <= !drm_next_dqoe;
    drm_dqs_oe_n_4 <= !drm_next_dqoe;
    drm_dqs_oe_n_5 <= !drm_next_dqoe;
    drm_dqs_oe_n_6 <= !drm_next_dqoe;
    drm_dqs_oe_n_7 <= !drm_next_dqoe;
end
// synthesis attribute equivalent_register_removal of drm_dqs_oe_n_0 is "no" ;
// synthesis attribute equivalent_register_removal of drm_dqs_oe_n_1 is "no" ;
// synthesis attribute equivalent_register_removal of drm_dqs_oe_n_2 is "no" ;
// synthesis attribute equivalent_register_removal of drm_dqs_oe_n_3 is "no" ;
// synthesis attribute equivalent_register_removal of drm_dqs_oe_n_4 is "no" ;
// synthesis attribute equivalent_register_removal of drm_dqs_oe_n_5 is "no" ;
// synthesis attribute equivalent_register_removal of drm_dqs_oe_n_6 is "no" ;
// synthesis attribute equivalent_register_removal of drm_dqs_oe_n_7 is "no" ;

OFDDRTCPE dqs_0( .PRE(1'b0), .CLR(system_reset_in), .D0(drm_next_dqs_high[0]), .D1(drm_dqs_low[0]), .C0(int_drm_clock_buffered), .C1(!int_drm_clock_buffered), .CE(1'b1), .O(ddr_dram_dqs[0]), .T(drm_dqs_oe_n_0) );
OFDDRTCPE dqs_1( .PRE(1'b0), .CLR(system_reset_in), .D0(drm_next_dqs_high[1]), .D1(drm_dqs_low[1]), .C0(int_drm_clock_buffered), .C1(!int_drm_clock_buffered), .CE(1'b1), .O(ddr_dram_dqs[1]), .T(drm_dqs_oe_n_1) );
OFDDRTCPE dqs_2( .PRE(1'b0), .CLR(system_reset_in), .D0(drm_next_dqs_high[2]), .D1(drm_dqs_low[2]), .C0(int_drm_clock_buffered), .C1(!int_drm_clock_buffered), .CE(1'b1), .O(ddr_dram_dqs[2]), .T(drm_dqs_oe_n_2) );
OFDDRTCPE dqs_3( .PRE(1'b0), .CLR(system_reset_in), .D0(drm_next_dqs_high[3]), .D1(drm_dqs_low[3]), .C0(int_drm_clock_buffered), .C1(!int_drm_clock_buffered), .CE(1'b1), .O(ddr_dram_dqs[3]), .T(drm_dqs_oe_n_3) );
OFDDRTCPE dqs_4( .PRE(1'b0), .CLR(system_reset_in), .D0(drm_next_dqs_high[0]), .D1(drm_dqs_low[0]), .C0(int_drm_clock_buffered), .C1(!int_drm_clock_buffered), .CE(1'b1), .O(ddr_dram_dqs[4]), .T(drm_dqs_oe_n_4) );
OFDDRTCPE dqs_5( .PRE(1'b0), .CLR(system_reset_in), .D0(drm_next_dqs_high[1]), .D1(drm_dqs_low[1]), .C0(int_drm_clock_buffered), .C1(!int_drm_clock_buffered), .CE(1'b1), .O(ddr_dram_dqs[5]), .T(drm_dqs_oe_n_5) );
OFDDRTCPE dqs_6( .PRE(1'b0), .CLR(system_reset_in), .D0(drm_next_dqs_high[2]), .D1(drm_dqs_low[2]), .C0(int_drm_clock_buffered), .C1(!int_drm_clock_buffered), .CE(1'b1), .O(ddr_dram_dqs[6]), .T(drm_dqs_oe_n_6) );
OFDDRTCPE dqs_7( .PRE(1'b0), .CLR(system_reset_in), .D0(drm_next_dqs_high[3]), .D1(drm_dqs_low[3]), .C0(int_drm_clock_buffered), .C1(!int_drm_clock_buffered), .CE(1'b1), .O(ddr_dram_dqs[7]), .T(drm_dqs_oe_n_7) );


//b Clock generator instance
wire [3:0]info;
// sys_drm_clock_in is at 125MHz, 8ns (4 high, 4 low nominally)
// int_drm_clock_buffered is sdc-0.5ns 
// int_input_drm_clock_buffered is sdc+2.5ns
// the input data is valid close after the clock falling for a whole clock (coz we put the same data out for two words!)
// so we capture it on the clock rising
// int_drm_clock_phase can be registered off double_drm_clock_buffered to recover int_drm_clock
// int_drm_clock_phase_1 is 1 int_double_drm_clock_buffered after int_drm_clock_phase (ie -ve)
// int_drm_clock_phase_2 is 2 int_double_drm_clock_buffered after int_drm_clock_phase
// int_drm_clock_phase_3 is 3 int_double_drm_clock_buffered after int_drm_clock_phase (ie -ve)
// int_drm_clock_phase_4 is 4 int_double_drm_clock_buffered after int_drm_clock_phase
clock_generator ckgen( .sys_drm_clock_in(sys_drm_clock_in),
                           .system_reset_in(system_reset_in),
                           .system_reset_out(system_reset),
                           .info(info),
                           .int_double_drm_clock_buffered(int_double_drm_clock_buffered),
                           .int_drm_clock_phase(int_drm_clock_phase),
                           .int_drm_clock_buffered(int_drm_clock_buffered),
                           .int_drm_clock_90_buffered(int_drm_clock_90_buffered),
                           .int_input_drm_clock_buffered(int_input_drm_clock_buffered),
                           .int_logic_slow_clock_buffered(int_logic_slow_clock_buffered),
                           .cke_next_will_be_last_of_logic(cke_next_will_be_last_of_logic),
                           .ps(gip_leds_out[7:6]),
                           .ps_done(ps_done) );

reg cke_last_of_logic;
always @(posedge int_drm_clock_buffered) begin cke_last_of_logic <= cke_next_will_be_last_of_logic; end

//b DDR DRAM controller instance
wire [1:0]sscl;
wire [1:0]sscl_oe;
wire ssdo, ssdo_oe;
wire [1:0]ssdi;
wire [7:0]sscs;
wire [7:0]internal_switches;
wire [31:0]analyzer_async_trace_out;

assign mii_mdc = sscl[0];
assign mii_mdio = ssdo_oe ? ssdo : 1'bz; //'
assign ssdi[0] = mii_mdio;
assign ssdi[1] = 0;
assign eth_rst_n = !system_reset_out;
assign analyzer_clock = int_logic_slow_clock_buffered;
assign leds_dual[6:0] = leds[6:0];
assign leds_dual[7] = drm_input_dq_high[63];
assign internal_switches = {switches[7:4], ps_done, switches[2:0]};
assign leds[7:0] = gip_leds_out[7:0];
gip_system body( .drm_clock(int_drm_clock_buffered),
                 .int_clock(int_logic_slow_clock_buffered),

                 .system_reset( system_reset ),
                 .reset_out( system_reset_out ),

                 .eth_mii_rx_clock(mii_rx_clk),
                 .eth_mii_tx_clock(mii_tx_clk),

                 .eth_mii_crs(mii_crs), // mii_crs
                 .eth_mii_col(mii_col), // mii_col
                 .eth_mii_rx_er(mii_rx_er), // mii_rx_er
                 .eth_mii_rx_d(mii_rxd), // mii_rxd[3:0]
                 .eth_mii_rx_dv(mii_rx_dv), // mii_rx_dv

                 .eth_mii_tx_er(mii_tx_er), // mii_tx_er
                 .eth_mii_tx_d(mii_txd), //mii_txd[3:0]
                 .eth_mii_tx_en(mii_tx_en), // mii_tx_en

                 .sscl(sscl), //  2 bits - bit 0 to mii_mdc
                 .sscl_oe(sscl_oe), // 2 bits - bit 0 oe not used as mii_mdc is driven
                 .ssdo(ssdo), // 1 pin - to mii_mdio
                 .ssdo_oe(ssdo_oe), // 1 pin - to mii_mdio oe
                 .ssdi(ssdi), // 2 pins - pin 0 from mii_mdio
                 .sscs(sscs), // 8 pins - unused

                 .switches(internal_switches),
                 .rxd(rxd),
                 .leds(gip_leds_out),
                 .txd(txd),
                 .ext_bus_read_data( ext_bus_read_data),
                 .ext_bus_write_data( ext_bus_write_data ),
                 .ext_bus_write_data_enable( ext_bus_write_data_enable ),
                 .ext_bus_address( ext_bus_address ),
                 .ext_bus_we( ext_bus_we ),
                 .ext_bus_oe( ext_bus_oe ),
                 .ext_bus_ce( ext_bus_ce ),


                 .cke_last_of_logic( cke_last_of_logic ),

                 .next_cke( drm_next_cke ), // these signals all to be clocked on int_drm_clock_buffered, and driven out on int_drm_clock_90_buffered
                 .next_s_n(drm_next_s_n),
                 .next_ras_n(drm_next_ras_n),
                 .next_cas_n(drm_next_cas_n),
                 .next_we_n(drm_next_we_n),
                 .next_a(drm_next_a),
                 .next_ba(drm_next_ba),
                 .next_dq(drm_next_dq[31:0]),     // we run these at clock rate, not ddr; output on int_drm_clock_buffered
                 .next_dqm(drm_next_dqm[3:0]),   // we run these at clock rate, not ddr; output on int_drm_clock_buffered
                 .next_dqoe(drm_next_dqoe), // we run these at clock rate, not ddr; it drives dq and dqs; use on int_drm_clock_buffered
                 .next_dqs_low(drm_next_dqs_low), // dqs strobes for low period of drm_clock (register on int_drm_clock_buffered and drive when that is low)
                 .next_dqs_high(drm_next_dqs_high), // dqs strobes for high period of drm_clock

                 .input_dq_low(drm_input_dq_low[31:0]), // last dq data during low period of drm_clock - GJS swapped these July 3 2005, but that does not work
                 .input_dq_high(drm_input_dq_high[31:0]), // last dq data during high period of drm_clock - GJS swapped these July 3 2005

                 .analyzer_clock(analyzer_clock),
                 .analyzer_async_trace_valid(analyzer_signals_valid),
                 .analyzer_async_trace_out(analyzer_signals)
    );
assign drm_next_dq[63:32] = 0;
assign drm_next_dqm[7:4] = 0;

//b End module
endmodule
