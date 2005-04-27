//a Xilinx note
// to avoid the xilinx bug commented out ddr_dram_dqm, ddr_dram_dqs
// commented out DDR controller instance
// commented out drm_dq_out logic etc
// commented out dqs outputs
// commented out dq inputs
// commented out dram control outputs
// commented out dram clock outputs
// deleted commented code
// deleted pin section (synthesis attributes)
// This works.

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

//b End module
endmodule
