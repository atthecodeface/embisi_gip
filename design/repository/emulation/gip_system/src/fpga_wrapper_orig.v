//a Xilinx note
// to avoid the xilinx bug commented out ddr_dram_dqm, ddr_dram_dqs
// commented out DDR controller instance
// commented out drm_dq_out logic etc
// commented out dqs outputs
// commented out dq inputs
// commented out dram control outputs
// commented out dram clock outputs

//a FPGA wrapper module
module fpga_wrapper
(
    sys_drm_clock_in,
    sys_drm_clock_fb,
    sys_drm_clock_fb_out,

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

    leds
);

//b Inputs and outputs
    //b Clocks
    input sys_drm_clock_in;
    input sys_drm_clock_fb;
    output sys_drm_clock_fb_out;

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

    output ddr_dram_cke;
    output [1:0]ddr_dram_s_n;
    output ddr_dram_ras_n;
    output ddr_dram_cas_n;
    output ddr_dram_we_n;
    output [1:0]ddr_dram_ba;
    output [12:0]ddr_dram_a;

    inout [31:0]ddr_dram_dq;
    output [3:0]ddr_dram_dqm;
    inout [3:0]ddr_dram_dqs;

    //b Outputs
    output [7:0]leds;

//b Assign pins and I/O standards
    //IOSTANDARD is pg 431 of constraints guide

    //b Clocks in
    //synthesis attribute LOC of sys_drm_clock_in is "AK18";
    //synthesis attribute LOC of sys_drm_clock_fb is "AG18";
    //synthesis attribute IOSTANDARD of sys_drm_clock_fb is "LVCMOS33"; // This is bank 5, so has a VCCO of 3.3V; it should be okay at a 2.5V input drive for a 1, as the spec is 2.0V for Vih
    //synthesis attribute LOC of sys_drm_clock_fb_out is "AE28";
    //synthesis attribute IOSTANDARD of sys_drm_clock_fb_out is "LVCMOS25";
    //synthesis attribute LOC of clk_40MHz_in is "AK19";

    //b DRAM pins
    //synthesis attribute LOC of ddr_dram_clk_0 is "N27";
    //synthesis attribute LOC of ddr_dram_clk_0_n is "P27";
    //synthesis attribute IOSTANDARD of ddr_dram_clk_0 is "LVDS_25";
    //synthesis attribute LOC of ddr_dram_clk_1 is "AF29";
    //synthesis attribute LOC of ddr_dram_clk_1_n is "AE29";
    //synthesis attribute IOSTANDARD of ddr_dram_clk_1 is "LVDS_25";

    //synthesis attribute LOC of ddr_dram_cke is "";
    //synthesis attribute IOSTANDARD of ddr_dram_cke is "SSTL2_II";
    //synthesis attribute LOC of ddr_dram_s_n is "";
    //synthesis attribute IOSTANDARD of ddr_dram_s_n is "SSTL2_II";
    //synthesis attribute LOC of ddr_dram_ras_n is "";
    //synthesis attribute IOSTANDARD of ddr_dram_ras_n is "SSTL2_II";
    //synthesis attribute LOC of ddr_dram_cas_n is "";
    //synthesis attribute IOSTANDARD of ddr_dram_cas_n is "SSTL2_II";
    //synthesis attribute LOC of ddr_dram_ba_n is "";
    //synthesis attribute IOSTANDARD of ddr_dram_ba_n is "SSTL2_II";
    //synthesis attribute LOC of ddr_dram_a_n is "";
    //synthesis attribute IOSTANDARD of ddr_dram_a_n is "SSTL2_II";

    //synthesis attribute LOC of ddr_dram_dq is "N29 M32 M31 L32 R26 R32 P26 P31 L31 K31 J32 J31 P30 N31 N30 L30 H32 H31 G32 F32 K29 K30 J29 H29 F31 F32 E31 D32 H30 H28 G29 F30";
    //synthesis attribute IOSTANDARD of ddr_dram_dq is "SSTL2_II";
    //synthesis attribute LOC of ddr_dram_dqm is "";
    //synthesis attribute IOSTANDARD of ddr_dram_dqm is "SSTL2_II";
    //synthesis attribute LOC of ddr_dram_dqs is "";
    //synthesis attribute IOSTANDARD of ddr_dram_dqs is "SSTL2_II";

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

//b Reset and clock signals/pins
wire int_output_drm_clock_buffered;
wire int_input_drm_clock_90;

wire system_reset_in;
assign system_reset_in = switches[7];
assign sys_drm_clock_fb_out = int_output_drm_clock_buffered;

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
//assign ddr_dram_clk_0=int_output_drm_clock_buffered;
//assign ddr_dram_clk_0_n=!int_output_drm_clock_buffered;
//assign ddr_dram_clk_1=int_output_drm_clock_buffered;
//assign ddr_dram_clk_1_n=!int_output_drm_clock_buffered;

//b DRAM control outputs: cke, s_n, ras_n, cas_n, we_n, ba, a
//reg ddr_dram_cke;
//reg [1:0]ddr_dram_s_n;
//reg ddr_dram_ras_n;
//reg ddr_dram_cas_n;
//reg ddr_dram_we_n;
//reg [1:0]ddr_dram_ba;
//reg [12:0]ddr_dram_a;
//
//wire drm_next_cke;
//wire [1:0]drm_next_s_n;
//wire drm_next_ras_n;
//wire drm_next_cas_n;
//wire drm_next_we_n;
//wire [1:0]drm_next_ba;
//wire [12:0]drm_next_a;
//always @(posedge int_output_drm_clock_buffered or posedge system_reset_in)
//begin
//    if (system_reset_in)
//    begin
//        ddr_dram_cke <= 0;
//        ddr_dram_s_n <= 2'b11;
//        ddr_dram_ras_n <= 1;
//        ddr_dram_cas_n <= 1;
//        ddr_dram_we_n <= 1;
//        ddr_dram_ba <= 0;
//        ddr_dram_a <= 0;
//    end
//    else
//    begin
//        ddr_dram_cke <= drm_next_cke;
//        ddr_dram_s_n <= drm_next_s_n;
//        ddr_dram_ras_n <= drm_next_ras_n;
//        ddr_dram_cas_n <= drm_next_cas_n;
//        ddr_dram_we_n <= drm_next_we_n;
//        ddr_dram_ba <= drm_next_ba;
//        ddr_dram_a <= drm_next_a;
//    end
//end

//b DQ and DQM outputs
//reg drm_dqoe;
//reg [31:0]drm_dq_out;
//reg [3:0]drm_dqm_out;
//
//wire drm_next_dqoe;
//wire [31:0]drm_next_dq;
//wire [3:0]drm_next_dqm;
//always @(posedge int_output_drm_clock_buffered or posedge system_reset_in)
//begin
//    if (system_reset_in)
//    begin
//        drm_dqoe <= 0;
//        drm_dq_out <= 0;
//        drm_dqm_out <= 0;
//    end
//    else
//    begin
//        drm_dqoe <= drm_next_dqoe;
//        drm_dq_out <= drm_next_dq;
//        drm_dqm_out <= drm_next_dqm;
//    end
//end
// synthesis attribute IOB of drm_dqoe is "TRUE";
// synthesis attribute IOB of drm_dq_out is "TRUE";
// synthesis attribute IOB of drm_dqm_out "TRUE";
//assign ddr_dram_dq[31:0] = drm_dqoe ? drm_dq_out : 32'bz;
//assign ddr_dram_dqm[3:0] = drm_dqm_out; // these are not tristate

//b DQ inputs - registered on posedge and negedge 90 input clocks
//reg[31:0] drm_input_dq_low;
//reg[31:0] drm_input_dq_high;
//wire [3:0] drm_next_dqs_high;
//wire [3:0] drm_next_dqs_low;
//wire [3:0] drm_dqs_out;
//always @(posedge int_input_drm_clock_90 or posedge system_reset_in)
//begin
//    if (system_reset_in)
//    begin
//        drm_input_dq_high <= 0;
//    end
//    else
//    begin
//        drm_input_dq_high <= ddr_dram_dq[31:0];
//    end
//end
//
//always @(negedge int_input_drm_clock_90 or posedge system_reset_in)
//begin
//    if (system_reset_in)
//    begin
//        drm_input_dq_low <= 0;
//    end
//else
//    begin
//        drm_input_dq_low <= ddr_dram_dq[31:0];
//    end
//end
// synthesis attribute IOB of drm_input_dq_low is "TRUE";
// synthesis attribute IOB of drm_input_dq_high is "TRUE";

//b DQS outputs
//reg [3:0] drm_dqs_low;
//always @(posedge int_output_drm_clock_buffered)
//begin
//    drm_dqs_low <= drm_next_dqs_low;
//end
//
//FDDRCPE dqs_0( .PRE(1'b0), .CLR(system_reset_in), .D0(drm_next_dqs_high[0]), .D1(drm_dqs_low[0]), .C0(int_output_drm_clock_buffered), .C1(!int_output_drm_clock_buffered), .CE(1'b1), .Q(drm_dqs_out[0]) );
//FDDRCPE dqs_1( .PRE(1'b0), .CLR(system_reset_in), .D0(drm_next_dqs_high[1]), .D1(drm_dqs_low[1]), .C0(int_output_drm_clock_buffered), .C1(!int_output_drm_clock_buffered), .CE(1'b1), .Q(drm_dqs_out[1]) );
//FDDRCPE dqs_2( .PRE(1'b0), .CLR(system_reset_in), .D0(drm_next_dqs_high[2]), .D1(drm_dqs_low[2]), .C0(int_output_drm_clock_buffered), .C1(!int_output_drm_clock_buffered), .CE(1'b1), .Q(drm_dqs_out[2]) );
//FDDRCPE dqs_3( .PRE(1'b0), .CLR(system_reset_in), .D0(drm_next_dqs_high[3]), .D1(drm_dqs_low[3]), .C0(int_output_drm_clock_buffered), .C1(!int_output_drm_clock_buffered), .CE(1'b1), .Q(drm_dqs_out[3]) );

//assign ddr_dram_dqs = drm_dqoe ? drm_dqs_out : 4'bz; //'

//b Clock generator instance
wire [3:0]info;
assign feedback_ddr_clock = sys_drm_clock_fb;
clock_generator ckgen( .sys_drm_clock_in(sys_drm_clock_in),
                           .system_reset_in(system_reset_in),
                           .system_reset_out(system_reset),
                           .info(info),
                           .int_output_drm_clock_buffered(int_output_drm_clock_buffered),
                           .feedback_ddr_clock(feedback_ddr_clock),
                           .int_input_drm_clock_90_buffered(int_input_drm_clock_90_buffered),
                           .int_logic_drm_clock_buffered(int_logic_drm_clock_buffered),
                           .int_logic_slow_clock_buffered(int_logic_slow_clock_buffered),
                           .cke_last_of_logic(cke_last_of_logic) );

//b DDR DRAM controller instance
//wire sram_priority;
//wire sram_read;
//wire sram_write;
//wire [3:0]sram_write_byte_enables;
//wire [23:0]sram_address;
//wire [31:0]sram_write_data;
//wire [31:0]sram_read_data;
//wire sram_low_priority_wait;
//ddr_dram_as_sram ddrs( .drm_clock(int_logic_drm_clock_buffered),
//                       .slow_clock(int_logic_slow_clock_buffered),
//
//                       .drm_ctl_reset( system_reset ),
//
//                       .sram_priority( sram_priority ),
//                       .sram_read( sram_read ),
//                       .sram_write( sram_write ),
//                       .sram_write_byte_enables( sram_write_byte_enables ),
//                       .sram_address( sram_address ),
//                       .sram_write_data( sram_write_data ),
//                       .sram_read_data( sram_read_data ),
//                       .sram_low_priority_wait( sram_low_priority_wait ),
//
//                       .cke_last_of_logic( cke_last_of_logic ),
//
//                       .next_cke( drm_next_cke ), // these signals all to be clocked on int_output_drm_clock_buffered
//                       .next_s_n(drm_next_s_n),
//                       .next_ras_n(drm_next_ras_n),
//                       .next_cas_n(drm_next_cas_n),
//                       .next_we_n(drm_next_we_n),
//                       .next_a(drm_next_a),
//                       .next_ba(drm_next_ba),
//                       .next_dq(drm_next_dq),     // we run these at clock rate, not ddr
//                       .next_dqm(drm_next_dqm),   // we run these at clock rate, not ddr
//                       .next_dqoe(drm_next_dqoe), // we run these at clock rate, not ddr; it drives dq and dqs
//                       .next_dqs_low(drm_next_dqs_low), // dqs strobes for low period of drm_clock (register on int_output_drm_clock_buffered and drive when that is low)
//                       .next_dqs_high(drm_next_dqs_high), // dqs strobes for high period of drm_clock
//
//                       .input_dq_low(drm_input_dq_low), // last dq data during low period of drm_clock
//                       .input_dq_high(drm_input_dq_high) // last dq data during high period of drm_clock
//
//    );

//b GIP instance
//    gip_simple body( .int_clock(global_clock),
//                     .int_reset(switches[7]),
//                     .switches(switches),
//                     .rxd(rxd),
//                     .leds(leds),
//                     .txd(txd),
//                     .ext_bus_read_data( ext_bus_read_data),
//                     .ext_bus_write_data( ext_bus_write_data ),
//                     .ext_bus_write_data_enable( ext_bus_write_data_enable ),
//                     .ext_bus_address( ext_bus_address ),
//                     .ext_bus_we( ext_bus_we ),
//                     .ext_bus_oe( ext_bus_oe ),
//                     .ext_bus_ce( ext_bus_ce ) );

//b End module
endmodule
