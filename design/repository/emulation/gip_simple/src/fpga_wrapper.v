//Note: $SYNPLICITY is the Synplicity install directory.

//  `include "/home/xilinx//lib/xilinx/virtex.v" 
module fpga_wrapper
(
    int_clock,

    switches,
    rxd,

    txd,

    eb_ce_n,
    eb_oe_n,
    eb_we_n,
    eb_address,
    eb_data,

    leds
);

    //b Clocks
    input int_clock;

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

    //b Outputs
    output [7:0]leds;

wire [31:0]ext_bus_read_data;
wire [3:0]ext_bus_ce;
wire [31:0]ext_bus_write_data;
wire ext_bus_write_data_enable;
wire ext_bus_oe;
wire ext_bus_we;
wire [23:0]ext_bus_address;

reg [2:0]clock_divider;
reg divided_clock;
wire global_clock;
always @(posedge int_clock)
begin
    clock_divider = clock_divider+1;
    divided_clock = (clock_divider==0);
end
BUFG clock_buffer( .I(divided_clock), .O(global_clock) );
// Not this?synthesis attribute buffer_type of global_clock is bufgp
//synthesis attribute clock_signal of global_clock is yes;
    gip_simple body( .int_clock(global_clock),
                     .int_reset(switches[7]),
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
                     .ext_bus_ce( ext_bus_ce ) );

    assign eb_ce_n[0] = !ext_bus_ce[0];
    assign eb_ce_n[1] = !ext_bus_ce[1];
    assign eb_ce_n[2] = !ext_bus_ce[2];
    assign eb_ce_n[3] = !ext_bus_ce[3];
    assign eb_we_n = !ext_bus_we;
    assign eb_oe_n = !ext_bus_oe;
    assign eb_address = ext_bus_address;
    assign eb_data = ext_bus_write_data_enable ? ext_bus_write_data : 32'bz;
    assign ext_bus_read_data = eb_data;
    
endmodule
