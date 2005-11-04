//a Documentation
// The structure of the DCMs is...
// DCM 1: take 125MHz, generate 250MHz and 125MHz in sync, with gbufs on both, feeding back the 125MHz so it is locked - this is our internal DRAM logic clock (with the 2x so we can regenerate the internal clock on pins out)
// DCM 2: take 125MHz, generate a phase shifted version for clocking the input data from the DRAM - we have a big read data window (a whole clock) so hitting this should be easy
//
// Shifting to using both edges of the clock for data etc means
// data out must be clocked 1/4 clock after dqs, so we use the 90 clock for that
// data in can still be clocked with a phase shift. The equivalent of this, though, should be a clock locked to the clock_FB pin; we will use adjustable phase shift to start with
//
// we have taken out double_clock from the pads, as we use a DDR pad clocked from int_drm_clock_buffered instead; this means double_clock is not actually used
//
// Global clocks
//  sys_drm_clock_in - frequency important, phase irrelevant
//  int_drm_clock_buffered - frequency locked to input clock, phase fixed - this is our reference clock
//  int_double_drm_clock_buffered - double frequency of int_drm_clock_buffered, closely locked in phase to int_drm_clock_buffered
//  int_drm_clock_90_buffered - 90 degree out of phase to int_drm_clock_buffered, subject to subtle clock tree variances; when int_drm_clock_buffered rises, this is low and this rises 90 degrees (1.6-2.5ns, dependent on freq) later
//  int_input_drm_clock_buffered - adjustable phase shifted version of int_drm_clock_buffered, used for clocking in input data
//                                 this is expected to rise clk->out + 2*trace + input pad delay after int_drm_clock_buffered, but as close to that as possible - certainly less than 3/4 of a clock period (2.5-3.7ns)
//
// Local clocks
//  int_drm_clock - slightly leads int_drm_clock_buffered and int_double_drm_clock_buffered, so it can be used to regenerate int_drm_clock_buffered by using it as sn input to a flop clocked from int_double_drm_clock_buffered
//
// clock out is 2.15-2.17ns int_double_drm_clock_buffered to out (ddr_clocks.twr)
// control signals 1.8-1.95ns after int_drm_clock_90_buffered, or 3.8-3.95ns after int_drm_clock_buffered
// data signals are 1.85ns after int_drm_clock_buffered rising/falling
// strobe signals are ?
//
// DRAM pumps out data and strobes 
//
// We also divide by 12 to get our internal clock frequency

//a clock_generator module
module clock_generator(
    sys_drm_clock_in,
    system_reset_in, // this resets the pin-feedback clock
    system_reset_out, // this is asserted when all the DLLs are locked

    int_double_drm_clock_buffered, // use for clock out generation only
    int_drm_clock_phase,       // use for clock out generation only; register this value to get the clock out
    int_drm_clock_buffered,    // use for clocking outputs only
    int_drm_clock_90_buffered, // use for clocking control outputs after their 'next values' are registered on drm_clock_buffered

    int_input_drm_clock_buffered, // use for dq input clocking only

    int_logic_slow_clock_buffered,
    cke_next_will_be_last_of_logic,

    ps,
    ps_done,

    info
);

//b Inputs and outputs
    input sys_drm_clock_in;
    input system_reset_in;
    output system_reset_out;

    output int_double_drm_clock_buffered; // divide by 2 to generate the output clock - register int_output_dram_clock_phase
    output int_drm_clock_phase; // value that can be registered on int_double_drm_clock_buffered to regenerate int_drm_clock_buffered
    output int_drm_clock_buffered;
    output int_drm_clock_90_buffered;

    output int_input_drm_clock_buffered; // buffered version of the fedback DDR clock, out of phase by 90

    output int_logic_slow_clock_buffered;
    output cke_next_will_be_last_of_logic; // asserted if the clock edge happening is the next-to-last of the logic clock low

    output [3:0] info;

    input [1:0] ps;
    output ps_done;

//b Wires
    wire [1:0]dcm_locked;
    assign info = {2'b0, dcm_locked[1:0]}; //'

wire derived_ps_inc_dec;
reg derived_ps_en;
reg sync_ps;
reg last_ps;
reg result_psdone;

//b DCM 1
//
// Generate the logic and output DRAM clocks with 0
// This should go relatively near the DRAM pads
// DQ is split between banks 6 and 7; then again all the outputs are...
// Bank 7 is right balls at the top, bank 6 is right balls at the bottom (top view)
// I think this is bank 7 on left at top, bank 6 on left at bottom
// The best place would then be DCM_X0Y0, the bottom left DCM; although all the gbufs are central, so X2Y0 is better
DCM clk_phases_gen(       .CLKIN (sys_drm_clock_in),
                          .RST(system_reset_in),
                          .CLKFB (int_drm_clock_buffered), // our reference signal, 4ns high, 4ns low
                          .CLK0 (int_drm_clock),           // -4 to -2ns before int_drm_clock_buffered (dependent on gbuf delay; likely around 2.8ns)
                          .CLK90 (int_drm_clock_90),       // -2ns to +0ns around int_drm_clock_buffered (+2ns from clk0)
                          .CLK2X (int_double_drm_clock),   // -4 to -2ns before int_drm_clock_buffered, (dependent on gbuf delay; likely around 2.8ns)
                          .LOCKED (dcm_locked[0] )
    );

// synthesis attribute CLK_FEEDBACK of clk_phases_gen is "1X";
// synthesis attribute CLKIN_PERIOD of clk_phases_gen is 8;
// synthesis attribute CLKFX_MULTIPLY of clk_phases_gen is 2;
// synthesis attribute CLKFX_DIVIDE of clk_phases_gen is 1;
// synthesis attribute CLKIN_DIVIDE_BY_2 of clk_phases_gen is 0;
// synthesis attribute CLKDV_DIVIDE of clk_phases_gen is 2;
// synthesis attribute CLKOUT_PHASE_SHIFT of clk_phases_gen is "NONE"; 
// synthesis attribute DESKEW_ADJUST of clk_phases_gen is "SYSTEM_SYNCHRONOUS";
// synthesis attribute DFS_FREQUENCY_MODE of clk_phases_gen is "LOW";
// synthesis attribute DLL_FREQUENCY_MODE of clk_phases_gen is "LOW";
// synthesis attribute DSS_MODE of clk_phases_gen is "NONE"; 
// synthesis attribute DUTY_CYCLE_CORRECTION of clk_phases_gen is "FALSE";
// synthesis attribute PHASE_SHIFT of clk_phases_gen is 0;
// synthesis attribute STARTUP_WAIT of clk_phases_gen is 1;

BUFG drm_clock_buffer( .I(int_drm_clock), .O(int_drm_clock_buffered) );                        // reference clock, 4ns high, 4ns low
BUFG drm_clock_90_buffer( .I(int_drm_clock_90), .O(int_drm_clock_90_buffered) );               // +1.5ns to +2.5ns after int_drm_clock_buffered
BUFG drm_double_clock_buffer( .I(int_double_drm_clock), .O(int_double_drm_clock_buffered) );   // 2ns high, 2ns low, starting -0.5 to +0.5 around int_drm_clock_buffered (skew in gbufs)
//synthesis attribute CLOCK_SIGNAL of int_double_drm_clock_buffered is "YES";
//synthesis attribute CLOCK_SIGNAL of int_drm_clock_buffered is "YES";
//synthesis attribute CLOCK_SIGNAL of int_drm_clock_90_buffered is "YES";
// on rising int_double_drm_clock_buffered, int_drm_clock_buffered toggles, and int_drm_clock contains the value it will toggle to as it leads the buffered version
// on rising int_double_drm_clock_buffered, if we register int_drm_clock, we will get a copy of int_drm_clock_buffered
// if we call this int_drm_clock_phase and somebody registers this, then they register it on int_double_drm_clock_buffered, they will get the inverse, as they are a double clock period away (one half of the drm clock)
// so we want to do an inversion first, so clients only have to register the phase to regenerate the clock.
reg int_drm_clock_phase;
always @(posedge int_double_drm_clock_buffered) begin int_drm_clock_phase <= !int_drm_clock; end // int_drm_clock is guaranteed to be -0.5<>+0.5ns - (1.5<>2.5)ns, or -3ns to -1ns before int_double_drm_clock_buffered

//b DCM 2
//
// Generate the internal input pin clock
// This should be placed near its input
// we skew by 1/3 of a clock for now; not too important why yet till we get to reads
DCM dram_input_pin_gen(       .CLKIN (sys_drm_clock_in),
                              .RST(system_reset_in),
                              .CLK0 (int_input_drm_clock),
                              .CLKFB (int_input_drm_clock_buffered),
                              .PSCLK(sys_drm_clock_in),
                              .PSEN(derived_ps_en),
                              .PSINCDEC(derived_ps_inc_dec),
                              .PSDONE(psdone),
                              .LOCKED (dcm_locked[1] )
                    );
// synthesis attribute CLK_FEEDBACK of dram_input_pin_gen is "1X"; 
// synthesis attribute CLKIN_PERIOD of dram_input_pin_gen is 8;
// synthesis attribute CLKFX_MULTIPLY of dram_input_pin_gen is 2;
// synthesis attribute CLKFX_DIVIDE of dram_input_pin_gen is 1;
// synthesis attribute CLKIN_DIVIDE_BY_2 of dram_input_pin_gen is 0;
// synthesis attribute CLKOUT_PHASE_SHIFT of dram_input_pin_gen is "VARIABLE"; 
// synthesis attribute DESKEW_ADJUST of dram_input_pin_gen is "SYSTEM_SYNCHRONOUS";
// synthesis attribute DFS_FREQUENCY_MODE of dram_input_pin_gen is "LOW"; 
// synthesis attribute DLL_FREQUENCY_MODE of dram_input_pin_gen is "LOW"; 
// synthesis attribute DSS_MODE of dram_input_pin_gen is "NONE"; 
// synthesis attribute DUTY_CYCLE_CORRECTION of dram_input_pin_gen is "TRUE";
// was 80 and working, but with no PSEN input; 120 seems to work as well, and so does 190.
// but all work as badly with the second DRAM module. Waaah.
// synthesis attribute PHASE_SHIFT of dram_input_pin_gen is 0;
// synthesis attribute STARTUP_WAIT of dram_input_pin_gen is 1;

BUFG drm_input_clock_buffer( .I(int_input_drm_clock), .O(int_input_drm_clock_buffered) );

//synthesis attribute CLOCK_SIGNAL of int_input_drm_clock_buffered is "YES";

always @(posedge sys_drm_clock_in)
begin
    derived_ps_en <= 0;
    if (sync_ps && !last_ps)
    begin
        derived_ps_en <= 1;
        result_psdone <= 0;
    end
    if (psdone)
    begin
        result_psdone <= 1;
    end
    sync_ps <= ps[0];
    last_ps <= sync_ps;
end
assign derived_ps_inc_dec = ps[1];
assign ps_done = result_psdone;

//b Clock divider
//
// Divide the internal logic DRAM clock, and generate two enables
// The logic gates should be right next to the DCMs and GBUFs
reg [3:0]clock_divider;
reg divided_clock;
reg cke_next_will_be_last_of_logic;
reg cke_last_of_logic_int; // duplicate so that there is a local copy for the divider, and a copy to be placed for the dram logic; this will take a good load off the signal
always @(posedge int_drm_clock_buffered)
begin
    clock_divider <= clock_divider+1;
    cke_next_will_be_last_of_logic <= 0;
    cke_last_of_logic_int <= 0;
    if (clock_divider==9)
    begin
        cke_next_will_be_last_of_logic <= 1;
    end
    if (cke_next_will_be_last_of_logic)
    begin
        cke_last_of_logic_int <= 1;
    end
    if (clock_divider==5)
    begin
        divided_clock <= 0;
    end
    if (cke_last_of_logic_int)
    begin
        clock_divider <= 0;
        divided_clock <= 1;
    end
end
// synthesis attribute equivalent_register_removal of cke_last_of_logic_int is "no" ;

assign int_logic_slow_clock = divided_clock;
BUFG slow_logic_clock_buffer( .I(int_logic_slow_clock), .O(int_logic_slow_clock_buffered) );
//synthesis attribute clock_signal of int_logic_slow_clock_buffered is yes;

//b Reset output
//
reg system_reset_out;
always @(posedge int_logic_slow_clock_buffered or posedge system_reset_in)
begin
    if (system_reset_in)
    begin
        system_reset_out <= 1;
    end
    else
    begin
        if (dcm_locked==2'b11) //'
        begin
            system_reset_out <= 0;
        end
    end
end

//b End module
endmodule
