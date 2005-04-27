module fpga_wrapper_leds
(
    sys_drm_clock_in,

    drm_dimm_clk_0,
    drm_dimm_clk_0_n,

    sys_drm_clock_fb_out,
    sys_drm_clock_fb,

    switches,
    rxd,

    txd,

    leds
);

    //b Clocks
    input sys_drm_clock_in;

    //b DRAM DIMM pins
    output drm_dimm_clk_0;
    output drm_dimm_clk_0_n;
    output sys_drm_clock_fb_out;
    input sys_drm_clock_fb;

    //b Switches
    input [7:0]switches;

    //b UART
    input rxd;
    output txd;

    //b Outputs
    output [7:0]leds;

wire int_output_drm_clock_buffered, feedback_ddr_clock, int_input_drm_clock_90_buffered, int_logic_drm_clock_buffered, int_logic_slow_clock_buffered, cke_last_of_logic;
wire system_reset_in;
wire system_reset_out;

//b Attach I/Os to particular pads and assign standards
//IOSTANDARD is pg 431 of constraints guide

// Clocks in
//synthesis attribute LOC of sys_drm_clock_in is "AK18";
//synthesis attribute LOC of sys_drm_clock_fb is "AG18";
//synthesis attribute IOSTANDARD of sys_drm_clock_fb is "LVCMOS33"; // This is bank 5, so has a VCCO of 3.3V; it should be okay at a 2.5V input drive for a 1, as the spec is 2.0V for Vih
//synthesis attribute LOC of sys_drm_clock_fb_out is "AE28";
//synthesis attribute IOSTANDARD of sys_drm_clock_fb_out is "LVCMOS25";
//synthesis attribute LOC of clk_40MHz_in is "AK19";

// DRAM pins
//synthesis attribute LOC of ddr_dram_clk_0 is "N27";
//synthesis attribute LOC of ddr_dram_clk_0_n is "P27";
//synthesis attribute IOSTANDARD of ddr_dram_clk_0 is "LVDS_25";
//synthesis attribute LOC of ddr_dram_clk_1 is "AF29";
//synthesis attribute LOC of ddr_dram_clk_1_n is "AE29";
//synthesis attribute IOSTANDARD of ddr_dram_clk_1 is "LVDS_25";
//synthesis attribute LOC of ddr_dram_dq is "F30 G29 H28 H30 D32 E31 F32 F31 H29 J29 K30 K29 F32 G32 H31 H32 L30 N30 N31 P30 J31 J32 K31 L31 P31 P26 R32 R26 L32 M31 M32 N29"; // this is 31..0!
//synthesis attribute IOSTANDARD of ddr_dram_dq is "SSTL2_II";

// UART
//synthesis attribute LOC of rxd is "M9";
//synthesis attribute LOC of txd is "K5";
//synthesis attribute IOSTANDARD of rxd is "?";

// LEDs
//synthesis attribute LOC of leds is "L3 L6 M3 M6 N7 N8 P8 N9";

// Switches
//synthesis attribute LOC of switches is "AE11 AF11 AF7 AG7 AH6 AJ5 AK3 AL3";


//b Instantiate pads
wire int_double_drm_clock_buffered;
wire int_output_drm_clock_phase;
reg sys_drm_clock_fb_out_r;
always @(posedge int_double_drm_clock_buffered) begin sys_drm_clock_fb_out_r <= !int_output_drm_clock_phase; end
// synthesis attribute equivalent_register_removal of sys_drm_clock_fb_out_r is "no" ;
assign sys_drm_clock_fb_out = sys_drm_clock_fb_out_r;

//b Make dividers for LEDs
assign system_reset_in = switches[7];


reg [26:0] sys_drm_clock_divider; reg sys_drm_clock_led;
always @(posedge sys_drm_clock_in) begin
if (system_reset_in) begin sys_drm_clock_divider<=0;sys_drm_clock_led<=0; end
else if (sys_drm_clock_divider==1245*100*1000) begin sys_drm_clock_led <= !sys_drm_clock_led; sys_drm_clock_divider<=0; end
else begin sys_drm_clock_divider <= sys_drm_clock_divider+1; end
end    

reg [26:0] int_output_drm_clock_buffered_divider; reg int_output_drm_clock_buffered_led;
always @(posedge int_output_drm_clock_buffered) begin
if (system_reset_in) begin int_output_drm_clock_buffered_divider<=0;int_output_drm_clock_buffered_led<=0; end
else if (int_output_drm_clock_buffered_divider==83*1000*1000) begin int_output_drm_clock_buffered_led <= !int_output_drm_clock_buffered_led; int_output_drm_clock_buffered_divider<=0; end
else begin int_output_drm_clock_buffered_divider <= int_output_drm_clock_buffered_divider+1; end
end    

reg [26:0] int_logic_drm_clock_buffered_divider; reg int_logic_drm_clock_buffered_led;
always @(posedge int_logic_drm_clock_buffered) begin
if (system_reset_in) begin int_logic_drm_clock_buffered_divider<=0;int_logic_drm_clock_buffered_led<=0; end
else if (int_logic_drm_clock_buffered_divider==83*1000*1000) begin int_logic_drm_clock_buffered_led <= !int_logic_drm_clock_buffered_led; int_logic_drm_clock_buffered_divider<=0; end
else begin int_logic_drm_clock_buffered_divider <= int_logic_drm_clock_buffered_divider+1; end
end    

reg [26:0] int_input_drm_clock_90_buffered_divider; reg int_input_drm_clock_90_buffered_led;
always @(posedge int_input_drm_clock_90_buffered) begin
if (system_reset_in) begin int_input_drm_clock_90_buffered_divider<=0;int_input_drm_clock_90_buffered_led<=0; end
else if (int_input_drm_clock_90_buffered_divider==83*1000*1000) begin int_input_drm_clock_90_buffered_led <= !int_input_drm_clock_90_buffered_led; int_input_drm_clock_90_buffered_divider<=0; end
else begin int_input_drm_clock_90_buffered_divider <= int_input_drm_clock_90_buffered_divider+1; end
end    

reg [26:0] int_logic_slow_clock_buffered_divider; reg int_logic_slow_clock_buffered_led;
always @(posedge int_logic_slow_clock_buffered) begin
if (system_reset_in) begin int_logic_slow_clock_buffered_divider<=0;int_logic_slow_clock_buffered_led<=0; end
else if (int_logic_slow_clock_buffered_divider==83*1000*1000/12) begin int_logic_slow_clock_buffered_led <= !int_logic_slow_clock_buffered_led; int_logic_slow_clock_buffered_divider<=0; end
else begin int_logic_slow_clock_buffered_divider <= int_logic_slow_clock_buffered_divider+1; end
end    

//b Assign LEDS
assign leds[7] = system_reset_in;
assign leds[6] = system_reset_out;
assign leds[0] = sys_drm_clock_led;
assign leds[1] = int_output_drm_clock_buffered_led;
assign leds[2] = int_input_drm_clock_90_buffered_led;
assign leds[3] = int_logic_drm_clock_buffered_led;
assign leds[4] = int_logic_slow_clock_buffered_led;

wire [3:0]info;
assign leds[5] = info[switches[1:0]];

//b Instantiate clock generator
assign feedback_ddr_clock = sys_drm_clock_fb;
//for test only assign feedback_ddr_clock = int_output_drm_clock_buffered;
clock_generator ckgen( .sys_drm_clock_in(sys_drm_clock_in),
                           .system_reset_in(system_reset_in),
                           .system_reset_out(system_reset_out),
                           .info(info),
                           .int_double_drm_clock_buffered(int_double_drm_clock_buffered),
                           .int_output_drm_clock_phase(int_output_drm_clock_phase),
                           .int_output_drm_clock_buffered(int_output_drm_clock_buffered),
                           .feedback_ddr_clock(feedback_ddr_clock),
                           .int_input_drm_clock_90_buffered(int_input_drm_clock_90_buffered),
                           .int_logic_drm_clock_buffered(int_logic_drm_clock_buffered),
                           .int_logic_slow_clock_buffered(int_logic_slow_clock_buffered),
                           .cke_last_of_logic(cke_last_of_logic) );


//b Done    
endmodule
