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

    drm_ck_0,
    drm_ck_0_n,
    drm_ck_1,
    drm_ck_1_n,
    drm_cke,
    drm_cke_n,

    drm_ras_n,
    drm_cas_n,
    drm_we_n,
    drm_a,
    drm_ba,
    drm_s,

    drm_dq,
    drm_dm,
    drm_dqs,

    leds
);

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
    output drm_ck_0;
    output drm_ck_0_n;
    output drm_ck_1;
    output drm_ck_1_n;
    output drm_cke;
    output drm_cke_n;

    output drm_ras_n;
    output drm_cas_n;
    output drm_we_n;
    output [12:0]drm_a;
    output [1:0]drm_ba;
    output [1:0]drm_s;

    inout [63:0]drm_dq;
    output [7:0]drm_dm;
    inout [7:0]drm_dqs;

    //b Outputs
    output [7:0]leds;

wire [31:0]ext_bus_read_data;
wire [3:0]ext_bus_ce;
wire [31:0]ext_bus_write_data;
wire ext_bus_write_data_enable;
wire ext_bus_oe;
wire ext_bus_we;
wire [23:0]ext_bus_address;

    clock_generator ckgen( .sys_drm_clock_in(sys_drm_clock_in),
                           .int_output_drm_clock(int_output_drm_clock),
                           .int_output_drm_clock_90(int_output_drm_clock_90),
                           .int_output_drm_clock_180(int_output_drm_clock_180),
                           .int_output_drm_clock_270(int_output_drm_clock_270),
                           .int_logic_drm_clock_buffered(int_logic_drm_clock_buffered),
                           .int_logic_slow_clock_buffered(int_logic_slow_clock_buffered),
                           .cke_gl_to_drm(cke_gl_to_drm),
                           .cke_drm_to_gl(cke_drm_to_gl) );

ddr_dram_as_sram ddrs( .drm_clock(int_logic_drm_clock_buffered),
                       .slow_clock(int_logic_slow_clock_buffered),
                       .drm_ctl_reset( switches[7] ),
                       .sram_priority( sram_priority ),
                       .sram_read( sram_read ),
                       .sram_write( sram_write ),
                       .sram_write_byte_enables( sram_write_byte_enables ),
                       .sram_address( sram_address ),
                       .sram_write_data( sram_write_data ),
                       .sram_read_data( sram_read_data ),
                       .sram_low_priority_wait( sram_low_priority_wait ),

                       .cke_last_of_logic( cke_last_of_logic ),
                       .next_cke( next_cke ),
                       .next_s(drm_next_s),
                       .next_ras_n(drm_next_ras_n),
                       .next_cas_n(drm_next_cas_n),
                       .next_we_n(drm_next_we_n),
                       .next_a(drm_next_a),
                       .next_ba(drm_next_ba),
                       .next_dq(drm_next_dq),     // we run these at clock rate, not ddr
                       .next_dqm(drm_next_dqm),   // we run these at clock rate, not ddr
                       .next_dqoe(drm_next_dqoe), // we run these at clock rate, not ddr; it drives dq and dqs
                       .next_dqs_low(drm_next_dqs_low), // dqs strobes for low period of drm_clock
                       .next_dqs_low(drm_next_dqs_high), // dqs strobes for low period of drm_clock
                       .input_dq_low(drm_input_dq_low), // last dq data during low period of drm_clock
                       .input_dq_high(drm_input_dq_high) // last dq data during high period of drm_clock
    );

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

    assign eb_ce_n[0] = !ext_bus_ce[0];
    assign eb_ce_n[1] = !ext_bus_ce[1];
    assign eb_ce_n[2] = !ext_bus_ce[2];
    assign eb_ce_n[3] = !ext_bus_ce[3];
    assign eb_we_n = !ext_bus_we;
    assign eb_oe_n = !ext_bus_oe;
    assign eb_address = ext_bus_address;
    assign eb_data = ext_bus_write_data_enable ? ext_bus_write_data : 32'bz; //'
    assign ext_bus_read_data = eb_data;
    
endmodule
