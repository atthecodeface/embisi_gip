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
always @(posedge output_clock) begin d_keep <= next_d; oe_n <= !next_oe; end
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

    gbe_rst_n,
    gmii_rx_clk,
    gmii_tx_clk,
    gmii_crs,
    gmii_col,
    gmii_txd,
    gmii_tx_en,
    gmii_tx_er,
    gmii_rxd,
    gmii_rx_dv,
    gmii_rx_er,
    gmii_mdio,
    gmii_mdc,

    leds,

    analyzer_clock,
    analyzer_signals
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

    inout [31:0]ddr_dram_dq;
    output [3:0]ddr_dram_dqm;
    inout [7:0]ddr_dram_dqs;

    //b Ethernet
    output gbe_rst_n;
    input gmii_rx_clk;
    input gmii_tx_clk;
    input gmii_crs;
    input gmii_col;
    output [3:0]gmii_txd;
    output gmii_tx_en;
    output gmii_tx_er;
    input [3:0]gmii_rxd;
    input gmii_rx_dv;
    input gmii_rx_er;
    inout gmii_mdio;
    output gmii_mdc;

    //b Outputs
    output [7:0]leds;
    output analyzer_clock;
    output [31:0]analyzer_signals;

//b Assign pins and I/O standards
    //IOSTANDARD is pg 431 of constraints guide

    //b Clocks in
    // The DRAM output clock feedback is bank 6/7 with VCCO of 2.5V
    // The input clocks are bank 5, so has a VCCO of 3.3V; it should be okay at a 2.5V input drive for a 1, as the spec is 2.0V for Vih
    //synthesis attribute LOC of sys_drm_clock_in is "AK18";
    //synthesis attribute LOC of sys_drm_clock_fb is "AG18";
    //synthesis attribute IOSTANDARD of sys_drm_clock_fb is "LVCMOS33";
    //synthesis attribute LOC of sys_drm_clock_fb_out is "AE28";
    //synthesis attribute IOSTANDARD of sys_drm_clock_fb_out is "LVCMOS25";
    //synthesis attribute LOC of clk_40MHz_in is "AK19";

    //b DRAM pins
    //synthesis attribute LOC of ddr_dram_clk_0 is "N27";
    //synthesis attribute LOC of ddr_dram_clk_0_n is "P27";
    //synthesis attribute IOSTANDARD of ddr_dram_clk_0 is "SSTL2_II";
    //synthesis attribute IOSTANDARD of ddr_dram_clk_0_n is "SSTL2_II";
    //synthesis attribute LOC of ddr_dram_clk_1 is "AF29";
    //synthesis attribute LOC of ddr_dram_clk_1_n is "AE29";
    //synthesis attribute IOSTANDARD of ddr_dram_clk_1 is "SSTL2_II";
    //synthesis attribute IOSTANDARD of ddr_dram_clk_1_n is "SSTL2_II";

    //synthesis attribute LOC of ddr_dram_cke is "Y25 N25";
    //synthesis attribute IOSTANDARD of ddr_dram_cke is "SSTL2_II";
    //synthesis attribute LOC of ddr_dram_s_n is "V29 AC29";
    //synthesis attribute IOSTANDARD of ddr_dram_s_n is "SSTL2_II";
    //synthesis attribute LOC of ddr_dram_ras_n is "V25";
    //synthesis attribute IOSTANDARD of ddr_dram_ras_n is "SSTL2_II";
    //synthesis attribute LOC of ddr_dram_cas_n is "W29";
    //synthesis attribute IOSTANDARD of ddr_dram_cas_n is "SSTL2_II";
    //synthesis attribute LOC of ddr_dram_we_n is "AB26";
    //synthesis attribute IOSTANDARD of ddr_dram_we_n is "SSTL2_II";
    //synthesis attribute LOC of ddr_dram_ba is "U27 AC25";
    //synthesis attribute IOSTANDARD of ddr_dram_ba is "SSTL2_II";
    //synthesis attribute LOC of ddr_dram_a is "W25 N26 AB27 AA25 N28 Y26 M26 AA26 T25 AA27 U25 AB25 U26";
    //synthesis attribute IOSTANDARD of ddr_dram_a is "SSTL2_II";

    //synthesis attribute LOC of ddr_dram_dq is "N29 M32 M31 L32 R26 R32 P26 P31 L31 K31 J32 J31 P30 N31 N30 L30 H32 H31 G32 F32 K29 K30 J29 H29 F31 E32 E31 D32 H30 H28 G29 F30";
    //synthesis attribute IOSTANDARD of ddr_dram_dq is "SSTL2_II";
    //synthesis attribute LOC of ddr_dram_dqm is "V27 P25 M28 K28";
    //synthesis attribute IOSTANDARD of ddr_dram_dqm is "SSTL2_II";
    //synthesis attribute LOC of ddr_dram_dqs is "AH29 AG28 AD27 AC27 V26 R25 M25 L28";
    //synthesis attribute IOSTANDARD of ddr_dram_dqs is "SSTL2_II";

    //b Ethernet
    // gbe_rst# pin 33 av data60 pin 92 AF14
    // gmii_rx_clk pin 57 av data38 pin 6   data_av38  AF18
    // gmii_tx_clk pin 60 av data46 pin 12  data_av46  AJ19
    // gmii_crs pin 40 av data35 pin 73     data_av35  AM17
    // gmii_col pin 39 av data34 pin 3      data_av34  AF20
    // gmii_rx_er pin 41 av data37 pin 5    data_av37  AE18
    // gmii_rxd[3:0] pins 51, 52, 55, 56 av data42 data44 data45 data47 pins 9 80 11 82 data_av42/44/45/47  AE17/AF15/AM28/AE15
    // gmii_rx_dv pin 44 av data39 pin 76   data_av39  AM16
    // gmii_mdio pin 80 av data33 pin 2     data_av33  AE19
    // gmii_tx_er pin 61 av data48 pin 83   data_av48  AK14
    // gmii_txd[3:0] pin 71 72 75 76 av data55 data54 data58 data57 pins 88 18 21 20 data_av55/54/58/57   AM12/AM20/AM19/AL19
    // gmii_tx_en pin 62 av data49 pin 14    data_av49  AK22
    // gmii_mdio pin 80 av data33 pin 2     data_av33  AE19
    // gmii_mdc pin 81 av data32 pin 72     data_av32  AL17
    //synthesis attribute LOC of gbe_rst_n   is "AF14";
    //synthesis attribute LOC of gmii_rx_clk is "AF18";
    //synthesis attribute LOC of gmii_tx_clk is "AJ19";
    //synthesis attribute LOC of gmii_crs    is "AM17";
    //synthesis attribute LOC of gmii_col    is "AF20";
    //synthesis attribute LOC of gmii_rx_er  is "AE18";
    //synthesis attribute LOC of gmii_rxd    is "AE17 AF15 AM28 AE15";
    //synthesis attribute LOC of gmii_rx_dv  is "AM16";
    //synthesis attribute LOC of gmii_tx_er  is "AK14";
    //synthesis attribute LOC of gmii_txd    is "AM12 AM20 AM19 AL19";
    //synthesis attribute LOC of gmii_tx_en  is "AK22";
    //synthesis attribute LOC of gmii_mdio   is "AE19";
    //synthesis attribute LOC of gmii_mdc    is "AL17";

    //b UART
    //synthesis attribute LOC of rxd is "M9";
    //synthesis attribute LOC of txd is "K5";
    //synthesis attribute IOSTANDARD of rxd is "LVTTL";

    //b LEDs
    //synthesis attribute LOC of leds is "L3 L6 M3 M6 N7 N8 P8 N9";
    //synthesis attribute IOSTANDARD of leds is "LVTTL";

    //b Switches
    //synthesis attribute LOC of switches is "AE11 AF11 AF7 AG7 AH6 AJ5 AK3 AL3";
    //synthesis attribute IOSTANDARD of switches is "LVTTL";

    //b Expansion bus (addr_av, data_av, and various ce's: flash_ce<3..0> is N10 M10 K6 E4, from page 3/14; flash_we/ce/oe is J6 H7 K4 from pg 3/7/14; flash_rst is L10 from pg 3)
    //synthesis attribute LOC of eb_address is "AL23 AJ22 AH23 AL22 AG23 AF23 AM24 AJ23 AK24 AM23 AL25 AF24 AJ24 AL24 AK26 AK25 AL27 AJ25 AM27 AH25 AH24 AM26 AH26 AK27";
    //synthesis attribute LOC of eb_ce_n is "N10 E4 K6 H7";
    //synthesis attribute LOC of eb_data is "AM6 AL6 AM7 AM8 AJ8 AH9 AL8 AK9 AJ9 AM9 AL9 AJ10 AH10 AL10 AK11 AK10 AL11 AM11 AG12 AJ11 AH12 AG11 AL12 AL13 AJ12 AH13 AF13 AG13 AE13 AF12 AG14 AE12";
    //synthesis attribute LOC of eb_oe_n is "K4";
    //synthesis attribute LOC of eb_we_n is "J6";
    //synthesis attribute IOSTANDARD of eb_address is "LVTTL";
    //synthesis attribute IOSTANDARD of eb_ce_n is "LVTTL";
    //synthesis attribute IOSTANDARD of eb_data is "LVTTL";
    //synthesis attribute IOSTANDARD of eb_oe_n is "LVTTL";
    //synthesis attribute IOSTANDARD of eb_we_n is "LVTTL";

    //b Logic analyzer
    // on JP16, data on pins 1 thru 32, clock on 50 (GEN_IO1_0 thru 31 and 49)
    //synthesis attribute LOC of analyzer_signals is "J24 F24 G24 D24   K23 C24 H23 G23   C23 D23 F23 J23   D22 C22 F22 E22   E21 G22 H21 H22   J22 K22 K21 H20   H19 J21 C18 F19   F18 K20 H18 D18";
    //synthesis attribute LOC of analyzer_clock is "D29";

//b Reset and clock signals/pins
wire int_double_drm_clock_buffered;
wire int_drm_clock_phase;
wire int_drm_clock_buffered;
wire int_drm_clock_90_buffered;

wire system_reset_in;
wire system_reset; // from the clock generator, once the DLLs have settled
assign system_reset_in = switches[7];

//synthesis attribute shreg_extract of fpga_wrapper is no;
reg int_drm_clock_phase_4;
reg int_drm_clock_phase_3;
reg int_drm_clock_phase_2;
reg int_drm_clock_phase_1;
always @(posedge int_double_drm_clock_buffered)
begin
    int_drm_clock_phase_1 <= int_drm_clock_phase;
    int_drm_clock_phase_2 <= int_drm_clock_phase_1;
    int_drm_clock_phase_3 <= int_drm_clock_phase_2;
    int_drm_clock_phase_4 <= int_drm_clock_phase_3;
end

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

//b DRAM clock inputs/outputs
reg ddr_dram_clk_0;
reg ddr_dram_clk_0_n;
reg ddr_dram_clk_1;
reg ddr_dram_clk_1_n;
always @(posedge int_double_drm_clock_buffered)
begin
    ddr_dram_clk_0 <= int_drm_clock_phase_4;
    ddr_dram_clk_0_n <= ddr_dram_clk_0;
    ddr_dram_clk_1 <= int_drm_clock_phase_4;
    ddr_dram_clk_1_n <= ddr_dram_clk_1;
end
// synthesis attribute equivalent_register_removal of ddr_dram_clk_0 is "no" ;
// synthesis attribute equivalent_register_removal of ddr_dram_clk_0_n is "no" ;
// synthesis attribute equivalent_register_removal of ddr_dram_clk_1 is "no" ;
// synthesis attribute equivalent_register_removal of ddr_dram_clk_1_n is "no" ;
// synthesis attribute iob of ddr_dram_clk_0 is "true" ;
// synthesis attribute iob of ddr_dram_clk_0_n is "true" ;
// synthesis attribute iob of ddr_dram_clk_1 is "true" ;
// synthesis attribute iob of ddr_dram_clk_1_n is "true" ;

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
wire [31:0]drm_next_dq;
wire [3:0]drm_next_dqm;
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
wire [31:0] drm_input_dq_low;
wire [31:0] drm_input_dq_high;

ddr_in_sdr_out_pin dqp_00( .reset(system_reset_in), .output_clock(int_drm_clock_buffered), .next_oe(drm_next_dqoe), .next_d(drm_next_dq[ 0]), .input_clock(int_input_drm_clock_buffered), .d_in_low(drm_input_dq_low[ 0]), .d_in_high(drm_input_dq_high[ 0]), .pad(ddr_dram_dq[ 0]) );
ddr_in_sdr_out_pin dqp_01( .reset(system_reset_in), .output_clock(int_drm_clock_buffered), .next_oe(drm_next_dqoe), .next_d(drm_next_dq[ 1]), .input_clock(int_input_drm_clock_buffered), .d_in_low(drm_input_dq_low[ 1]), .d_in_high(drm_input_dq_high[ 1]), .pad(ddr_dram_dq[ 1]) );
ddr_in_sdr_out_pin dqp_02( .reset(system_reset_in), .output_clock(int_drm_clock_buffered), .next_oe(drm_next_dqoe), .next_d(drm_next_dq[ 2]), .input_clock(int_input_drm_clock_buffered), .d_in_low(drm_input_dq_low[ 2]), .d_in_high(drm_input_dq_high[ 2]), .pad(ddr_dram_dq[ 2]) );
ddr_in_sdr_out_pin dqp_03( .reset(system_reset_in), .output_clock(int_drm_clock_buffered), .next_oe(drm_next_dqoe), .next_d(drm_next_dq[ 3]), .input_clock(int_input_drm_clock_buffered), .d_in_low(drm_input_dq_low[ 3]), .d_in_high(drm_input_dq_high[ 3]), .pad(ddr_dram_dq[ 3]) );
ddr_in_sdr_out_pin dqp_04( .reset(system_reset_in), .output_clock(int_drm_clock_buffered), .next_oe(drm_next_dqoe), .next_d(drm_next_dq[ 4]), .input_clock(int_input_drm_clock_buffered), .d_in_low(drm_input_dq_low[ 4]), .d_in_high(drm_input_dq_high[ 4]), .pad(ddr_dram_dq[ 4]) );
ddr_in_sdr_out_pin dqp_05( .reset(system_reset_in), .output_clock(int_drm_clock_buffered), .next_oe(drm_next_dqoe), .next_d(drm_next_dq[ 5]), .input_clock(int_input_drm_clock_buffered), .d_in_low(drm_input_dq_low[ 5]), .d_in_high(drm_input_dq_high[ 5]), .pad(ddr_dram_dq[ 5]) );
ddr_in_sdr_out_pin dqp_06( .reset(system_reset_in), .output_clock(int_drm_clock_buffered), .next_oe(drm_next_dqoe), .next_d(drm_next_dq[ 6]), .input_clock(int_input_drm_clock_buffered), .d_in_low(drm_input_dq_low[ 6]), .d_in_high(drm_input_dq_high[ 6]), .pad(ddr_dram_dq[ 6]) );
ddr_in_sdr_out_pin dqp_07( .reset(system_reset_in), .output_clock(int_drm_clock_buffered), .next_oe(drm_next_dqoe), .next_d(drm_next_dq[ 7]), .input_clock(int_input_drm_clock_buffered), .d_in_low(drm_input_dq_low[ 7]), .d_in_high(drm_input_dq_high[ 7]), .pad(ddr_dram_dq[ 7]) );
ddr_in_sdr_out_pin dqp_08( .reset(system_reset_in), .output_clock(int_drm_clock_buffered), .next_oe(drm_next_dqoe), .next_d(drm_next_dq[ 8]), .input_clock(int_input_drm_clock_buffered), .d_in_low(drm_input_dq_low[ 8]), .d_in_high(drm_input_dq_high[ 8]), .pad(ddr_dram_dq[ 8]) );
ddr_in_sdr_out_pin dqp_09( .reset(system_reset_in), .output_clock(int_drm_clock_buffered), .next_oe(drm_next_dqoe), .next_d(drm_next_dq[ 9]), .input_clock(int_input_drm_clock_buffered), .d_in_low(drm_input_dq_low[ 9]), .d_in_high(drm_input_dq_high[ 9]), .pad(ddr_dram_dq[ 9]) );
ddr_in_sdr_out_pin dqp_10( .reset(system_reset_in), .output_clock(int_drm_clock_buffered), .next_oe(drm_next_dqoe), .next_d(drm_next_dq[10]), .input_clock(int_input_drm_clock_buffered), .d_in_low(drm_input_dq_low[10]), .d_in_high(drm_input_dq_high[10]), .pad(ddr_dram_dq[10]) );
ddr_in_sdr_out_pin dqp_11( .reset(system_reset_in), .output_clock(int_drm_clock_buffered), .next_oe(drm_next_dqoe), .next_d(drm_next_dq[11]), .input_clock(int_input_drm_clock_buffered), .d_in_low(drm_input_dq_low[11]), .d_in_high(drm_input_dq_high[11]), .pad(ddr_dram_dq[11]) );
ddr_in_sdr_out_pin dqp_12( .reset(system_reset_in), .output_clock(int_drm_clock_buffered), .next_oe(drm_next_dqoe), .next_d(drm_next_dq[12]), .input_clock(int_input_drm_clock_buffered), .d_in_low(drm_input_dq_low[12]), .d_in_high(drm_input_dq_high[12]), .pad(ddr_dram_dq[12]) );
ddr_in_sdr_out_pin dqp_13( .reset(system_reset_in), .output_clock(int_drm_clock_buffered), .next_oe(drm_next_dqoe), .next_d(drm_next_dq[13]), .input_clock(int_input_drm_clock_buffered), .d_in_low(drm_input_dq_low[13]), .d_in_high(drm_input_dq_high[13]), .pad(ddr_dram_dq[13]) );
ddr_in_sdr_out_pin dqp_14( .reset(system_reset_in), .output_clock(int_drm_clock_buffered), .next_oe(drm_next_dqoe), .next_d(drm_next_dq[14]), .input_clock(int_input_drm_clock_buffered), .d_in_low(drm_input_dq_low[14]), .d_in_high(drm_input_dq_high[14]), .pad(ddr_dram_dq[14]) );
ddr_in_sdr_out_pin dqp_15( .reset(system_reset_in), .output_clock(int_drm_clock_buffered), .next_oe(drm_next_dqoe), .next_d(drm_next_dq[15]), .input_clock(int_input_drm_clock_buffered), .d_in_low(drm_input_dq_low[15]), .d_in_high(drm_input_dq_high[15]), .pad(ddr_dram_dq[15]) );
ddr_in_sdr_out_pin dqp_16( .reset(system_reset_in), .output_clock(int_drm_clock_buffered), .next_oe(drm_next_dqoe), .next_d(drm_next_dq[16]), .input_clock(int_input_drm_clock_buffered), .d_in_low(drm_input_dq_low[16]), .d_in_high(drm_input_dq_high[16]), .pad(ddr_dram_dq[16]) );
ddr_in_sdr_out_pin dqp_17( .reset(system_reset_in), .output_clock(int_drm_clock_buffered), .next_oe(drm_next_dqoe), .next_d(drm_next_dq[17]), .input_clock(int_input_drm_clock_buffered), .d_in_low(drm_input_dq_low[17]), .d_in_high(drm_input_dq_high[17]), .pad(ddr_dram_dq[17]) );
ddr_in_sdr_out_pin dqp_18( .reset(system_reset_in), .output_clock(int_drm_clock_buffered), .next_oe(drm_next_dqoe), .next_d(drm_next_dq[18]), .input_clock(int_input_drm_clock_buffered), .d_in_low(drm_input_dq_low[18]), .d_in_high(drm_input_dq_high[18]), .pad(ddr_dram_dq[18]) );
ddr_in_sdr_out_pin dqp_19( .reset(system_reset_in), .output_clock(int_drm_clock_buffered), .next_oe(drm_next_dqoe), .next_d(drm_next_dq[19]), .input_clock(int_input_drm_clock_buffered), .d_in_low(drm_input_dq_low[19]), .d_in_high(drm_input_dq_high[19]), .pad(ddr_dram_dq[19]) );
ddr_in_sdr_out_pin dqp_20( .reset(system_reset_in), .output_clock(int_drm_clock_buffered), .next_oe(drm_next_dqoe), .next_d(drm_next_dq[20]), .input_clock(int_input_drm_clock_buffered), .d_in_low(drm_input_dq_low[20]), .d_in_high(drm_input_dq_high[20]), .pad(ddr_dram_dq[20]) );
ddr_in_sdr_out_pin dqp_21( .reset(system_reset_in), .output_clock(int_drm_clock_buffered), .next_oe(drm_next_dqoe), .next_d(drm_next_dq[21]), .input_clock(int_input_drm_clock_buffered), .d_in_low(drm_input_dq_low[21]), .d_in_high(drm_input_dq_high[21]), .pad(ddr_dram_dq[21]) );
ddr_in_sdr_out_pin dqp_22( .reset(system_reset_in), .output_clock(int_drm_clock_buffered), .next_oe(drm_next_dqoe), .next_d(drm_next_dq[22]), .input_clock(int_input_drm_clock_buffered), .d_in_low(drm_input_dq_low[22]), .d_in_high(drm_input_dq_high[22]), .pad(ddr_dram_dq[22]) );
ddr_in_sdr_out_pin dqp_23( .reset(system_reset_in), .output_clock(int_drm_clock_buffered), .next_oe(drm_next_dqoe), .next_d(drm_next_dq[23]), .input_clock(int_input_drm_clock_buffered), .d_in_low(drm_input_dq_low[23]), .d_in_high(drm_input_dq_high[23]), .pad(ddr_dram_dq[23]) );
ddr_in_sdr_out_pin dqp_24( .reset(system_reset_in), .output_clock(int_drm_clock_buffered), .next_oe(drm_next_dqoe), .next_d(drm_next_dq[24]), .input_clock(int_input_drm_clock_buffered), .d_in_low(drm_input_dq_low[24]), .d_in_high(drm_input_dq_high[24]), .pad(ddr_dram_dq[24]) );
ddr_in_sdr_out_pin dqp_25( .reset(system_reset_in), .output_clock(int_drm_clock_buffered), .next_oe(drm_next_dqoe), .next_d(drm_next_dq[25]), .input_clock(int_input_drm_clock_buffered), .d_in_low(drm_input_dq_low[25]), .d_in_high(drm_input_dq_high[25]), .pad(ddr_dram_dq[25]) );
ddr_in_sdr_out_pin dqp_26( .reset(system_reset_in), .output_clock(int_drm_clock_buffered), .next_oe(drm_next_dqoe), .next_d(drm_next_dq[26]), .input_clock(int_input_drm_clock_buffered), .d_in_low(drm_input_dq_low[26]), .d_in_high(drm_input_dq_high[26]), .pad(ddr_dram_dq[26]) );
ddr_in_sdr_out_pin dqp_27( .reset(system_reset_in), .output_clock(int_drm_clock_buffered), .next_oe(drm_next_dqoe), .next_d(drm_next_dq[27]), .input_clock(int_input_drm_clock_buffered), .d_in_low(drm_input_dq_low[27]), .d_in_high(drm_input_dq_high[27]), .pad(ddr_dram_dq[27]) );
ddr_in_sdr_out_pin dqp_28( .reset(system_reset_in), .output_clock(int_drm_clock_buffered), .next_oe(drm_next_dqoe), .next_d(drm_next_dq[28]), .input_clock(int_input_drm_clock_buffered), .d_in_low(drm_input_dq_low[28]), .d_in_high(drm_input_dq_high[28]), .pad(ddr_dram_dq[28]) );
ddr_in_sdr_out_pin dqp_29( .reset(system_reset_in), .output_clock(int_drm_clock_buffered), .next_oe(drm_next_dqoe), .next_d(drm_next_dq[29]), .input_clock(int_input_drm_clock_buffered), .d_in_low(drm_input_dq_low[29]), .d_in_high(drm_input_dq_high[29]), .pad(ddr_dram_dq[29]) );
ddr_in_sdr_out_pin dqp_30( .reset(system_reset_in), .output_clock(int_drm_clock_buffered), .next_oe(drm_next_dqoe), .next_d(drm_next_dq[30]), .input_clock(int_input_drm_clock_buffered), .d_in_low(drm_input_dq_low[30]), .d_in_high(drm_input_dq_high[30]), .pad(ddr_dram_dq[30]) );
ddr_in_sdr_out_pin dqp_31( .reset(system_reset_in), .output_clock(int_drm_clock_buffered), .next_oe(drm_next_dqoe), .next_d(drm_next_dq[31]), .input_clock(int_input_drm_clock_buffered), .d_in_low(drm_input_dq_low[31]), .d_in_high(drm_input_dq_high[31]), .pad(ddr_dram_dq[31]) );

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
//assign feedback_ddr_clock = sys_drm_clock_fb;
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
                           .cke_next_will_be_last_of_logic(cke_next_will_be_last_of_logic) );

reg cke_last_of_logic;
always @(posedge int_drm_clock_buffered) begin cke_last_of_logic <= cke_next_will_be_last_of_logic; end

//b DDR DRAM controller instance
wire [1:0]sscl;
wire [1:0]sscl_oe;
wire ssdo, ssdo_oe;
wire [1:0]ssdi;
wire [7:0]sscs;
assign gmii_mdc = sscl[0];
assign gmii_mdio = ssdo_oe ? ssdo : 1'bz; //'
assign ssdi[0] = gmii_mdio;
assign ssdi[1] = 0;
assign gbe_rst_n = !system_reset;
assign analyzer_clock = int_logic_slow_clock_buffered;
gip_system body( .drm_clock(int_drm_clock_buffered),
                 .int_clock(int_logic_slow_clock_buffered),

                 .eth_mii_rx_clock(gmii_rx_clk), // gmii_rx_clk pin 57 av data38 pin 6   data_av38
                 .eth_mii_tx_clock(gmii_tx_clk), // gmii_tx_clk pin 60 av data46 pin 12  data_av46

                 .eth_mii_crs(gmii_crs), // gmii_crs pin 40 av data35 pin 73       data_av35
                 .eth_mii_col(gmii_col), // gmii_col pin 39 av data34 pin 3        data_av34
                 .eth_mii_rx_er(gmii_rx_er), // gmii_rx_er pin 41 av data37 pin 5  data_av37
                 .eth_mii_rx_d(gmii_rxd), // gmii_rxd[3:0] pins 51, 52, 55, 56 av data42 data44 data45 data47 pins 9 80 11 82 data_av22/44/45/47
                 .eth_mii_rx_dv(gmii_rx_dv), // gmii_rx_dv pin 44 av data39 pin 76 data_av39

                 .eth_mii_tx_er(gmii_tx_er), // gmii_tx_er pin 61 av data48 pin 83 data_av48
                 .eth_mii_tx_d(gmii_txd), //gmii_txd[3:0] pin 71 72 75 76 av data55 data54 data58 data57 pins 88 18 21 20 data_av55/54/58/57
                 .eth_mii_tx_en(gmii_tx_en), // gmii_tx_en pin 62 av data49 pin 14  data_av49

                 .sscl(sscl), //  2 bits - bit 0 to gmii_mdc pin 81 av data32 pin 72           data_av32
                 .sscl_oe(sscl_oe), // 2 bits - bit 0 oe not used as gmii_mdc is driven
                 .ssdo(ssdo), // 1 pin - to gmii_mdio pin 80 av data33 pin 2      data_av33
                 .ssdo_oe(ssdo_oe), // 1 pin - to gmii_mdio oe pin 80 av data33 pin 2      data_av33
                 .ssdi(ssdi), // 2 pins - pin 0 from gmii_mdio pin 80 av data33 pin 2      data_av33
                 .sscs(sscs), // 8 pins - unused

                 .system_reset( system_reset ),

                 .switches(switches),
                 .rxd(rxd),
                 .leds(leds),
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
                 .next_dq(drm_next_dq),     // we run these at clock rate, not ddr; output on int_drm_clock_buffered
                 .next_dqm(drm_next_dqm),   // we run these at clock rate, not ddr; output on int_drm_clock_buffered
                 .next_dqoe(drm_next_dqoe), // we run these at clock rate, not ddr; it drives dq and dqs; use on int_drm_clock_buffered
                 .next_dqs_low(drm_next_dqs_low), // dqs strobes for low period of drm_clock (register on int_drm_clock_buffered and drive when that is low)
                 .next_dqs_high(drm_next_dqs_high), // dqs strobes for high period of drm_clock

                 .input_dq_low(drm_input_dq_low), // last dq data during low period of drm_clock
                 .input_dq_high(drm_input_dq_high), // last dq data during high period of drm_clock

                 .analyzer_clock(analyzer_clock),
                 .analyzer_signals(analyzer_signals)
    );
//assign drm_next_a = drm_input_dq_low[12:0];
//assign drm_next_ba = drm_input_dq_low[14:13];
//assign drm_next_ras_n = drm_input_dq_low[15];
//assign drm_next_cas_n = drm_input_dq_low[16];
//assign drm_next_dqm = drm_input_dq_low[20:17];
//assign drm_next_dqs_low  = drm_input_dq_low[24:21];
//assign drm_next_dqs_high = drm_input_dq_high[28:25];
//assign drm_next_we_n = drm_input_dq_low[29];
//assign drm_next_s_n = drm_input_dq_low[31:30];
//assign drm_next_dq = drm_input_dq_high[31:0];
//assign drm_next_dqoe = drm_input_dq_low[31];
//assign drm_next_cke = drm_input_dq_low[31];

//b End module
endmodule
