//a Documentation
// One approach (once we have an 83MHz clock) is to...
// 1...
// Drive this 83MHz clock out to the DRAM
// Feed this back in as our clock; note that this means the clock will not be running until the xilinx is configured
// The DRAM generates data locked to this clock; its read data and strobes are valid 0.75ns after the edge it gets until 0.75 before the next edge
// From this fed back clock we can generate an internal clock
// This internal clock can be used at 90 and !90 to latch the incoming data.
// Note that inputs have a sampling error of 0.55ns, we have clock tree skew of 0.15ns, and package skew of 0.15ns. This gives us an input uncertainty of 0.85ns.
// So this works great for inputs
// For outputs, though, we have the clock-to-q delay; we may not meet this with the fedback clock
// 2...
// Drive this 83MHz clock out to the DRAM
// Use this 83MHz clock rising edge to clock out commands
// register data out on this clock to (we are SDR for dq and dqm out)
// register data strobes on this edge
// fire the actual data strobes out delayed by 1/4 clock and 3/4 clock (90 and !90 clocks)
// But for inputs...
// our clock-to-out is 1.74ns-0.36ns (for LVDS) max; this means that the DRAM should be driving data valid after...
// max clk_skew + 1.38ns + pkg_skew + Tac after clock edge
// max clk_skew + 0.00ns + pkg_skew + Tac before next clock edge
// we have a setup of 1.06ns and a hold requirement of -0.45ns for inputs, with 0.48ns max delay for SSTL-II
// so worst case time from our clock to data being registered would be (with 0 margin) 0.15+1.38+0.15+0.75+1.06 = 3.49ns
// This would imply a 14ns clock period...
// This does not work.
// 3...
// Drive this 83MHz clock out to the DRAM
// Use this 83MHz clock rising edge to clock out commands
// register data out on this clock to (we are SDR for dq and dqm out)
// register falling data strobe on this edge
// use this edge for data strobe high out, and the registered for data strobe low out
// this then is int_output_drm_clock, and int_logic_drm_clock_buffered
// But for inputs...
// feed back the 83MHz, and use a DCM to generate 90 degree phase clocks
// use the 90 and !90 to clock the input data
// Now we have a timing of...
// worst case setup:
//   quarter clock period
//      MINUS
//   LVDS input pad delay min
//   DCM clock error (internal clock could be slightly early; jitter plus phase offset = 150ps+50ps)
//   max clk skew (skew to input flops)
//   max pkg skew (skew of data pins)
//   Tac (valid)
// which is TCK/4 -  ( 0ns + 0.2ns + 0.15ns + 0.15ns + 0.75ns ) ) > 1.06ns, our required setup
// which is Tck/4 > 1.06ns + 1.25ns, or 2.31ns
// worst case hold
//   quarter clock period (the data changes after a half clock period, and we clock it on the previous quater clock)
//      MINUS
//   LVDS input pad delay max
//   DCM clock error (internal clock could be slightly early; jitter plus phase offset = 150ps+50ps)
//   max clk skew (skew to input flops)
//   max pkg skew (skew of data pins)
//   Tac (valid)
// which is TCK/4 -  ( (0.88ns+0.69ns) + 0.2ns + 0.15ns + 0.15ns + 0.75ns ) ) > -0.45ns, our required hold
// which is Tck/4 > -0.45ns + 1.57ns + 1.25ns, or 2.37ns
// This works, and lets us run the interface up to 100MHz without special phase aligning to account for clock-to-out issues
// this clock is then int_input_drm_clock_90
//
// Well, Xilinx messed that one up
// Driving the clock out invokes extra wiring so that the clock out has +3ns of extra delay over clocks to pads. Weird.
//
// 4...
// Get the 83MHz starting clock
// Double this clock and buffer it for registers for the outputs
// Also generate a 90 degree phase shifted, so that at the clock out pins we can regenerate the buffered version
// Note that a 90 degree phase shifted clock is LOW when the nonshifted clock rises, so use !90_buffered to regenerate
// register commands out on the buffered clock
// register data out on the buffered clock to (we are SDR for dq and dqm out)
// register falling data strobe on this edge
// use this edge for data strobe high out, and the registered for data strobe low out
// this then is int_output_drm_clock, and int_logic_drm_clock_buffered
// But for inputs...
// feed back the 83MHz, and use a DCM to generate 90 degree phase clocks
// use the 90 and !90 to clock the input data
// Now we have a timing of...
// worst case setup:
//   quarter clock period
//      MINUS
//   LVDS input pad delay min
//   DCM clock error (internal clock could be slightly early; jitter plus phase offset = 150ps+50ps)
//   max clk skew (skew to input flops)
//   max pkg skew (skew of data pins)
//   Tac (valid)
// which is TCK/4 -  ( 0ns + 0.2ns + 0.15ns + 0.15ns + 0.75ns ) ) > 1.06ns, our required setup
// which is Tck/4 > 1.06ns + 1.25ns, or 2.31ns
// worst case hold
//   quarter clock period (the data changes after a half clock period, and we clock it on the previous quater clock)
//      MINUS
//   LVDS input pad delay max
//   DCM clock error (internal clock could be slightly early; jitter plus phase offset = 150ps+50ps)
//   max clk skew (skew to input flops)
//   max pkg skew (skew of data pins)
//   Tac (valid)
// which is TCK/4 -  ( (0.88ns+0.69ns) + 0.2ns + 0.15ns + 0.15ns + 0.75ns ) ) > -0.45ns, our required hold
// which is Tck/4 > -0.45ns + 1.57ns + 1.25ns, or 2.37ns
// This works, and lets us run the interface up to 100MHz without special phase aligning to account for clock-to-out issues
// this clock is then int_input_drm_clock_90
// provided the clock to output does get worse than 90 degrees of phase...
// The actual timings turn out to be...
//  from clock out of DCM through GBUF to flop clock for 166MHz, 1.4ns + 1.9ns (1.4 for to GBUF and intrinsic delay, 1.9 for global clock tree)
//  from clock out of DCM through GBUF to flop clock for 83MHz, 1.4ns + 1.9ns (1.4 for to GBUF and intrinsic delay, 1.9 for global clock tree)
//  clock to pad for LVDS 1.7ns
//  so we have an issue with hold times for command; we can trust for a while...
//  we need 0.5ns setup and hold of dq/dqm to dqs; we meet that by design
//  dqs has no particular setup to the clock
//  control signals, though, have a 0.9ns hold requirement; we are giving them about 0.5ns at the largest.
//  now we have an 8-part DIMM module, so clock goes to 4 chips, but control signals to 8; this is an extra 8-12pF for signals, should give us 0.5ns with 50ohms
// our feedback clock needs a high slew rate driver to get the timing down from 11ns to 7ns (main clock in to pad); in this case it is about 0.5ns slower than the LVDS clock.
// hope board integrity is okay for that signal


//
// The structure of the DCMs is...
// DCM 1: take 125MHz, generate 250MHz and 125MHz in sync, with gbufs on both, feeding back the 125MHz so it is locked
// DCM 2: take 125MHz, generate a phase shifted version for clocking the input data from the DRAM
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

//b Wires
    wire [1:0]dcm_locked;
    assign info = {2'b0, dcm_locked[1:0]}; //'

//b DCM 1
//
// Generate the logic and output DRAM clocks with 0
// This should go relatively near the DRAM pads
// DQ is split between banks 6 and 7; then again all the outputs are...
// Bank 7 is right balls at the top, bank 6 is right balls at the bottom (top view)
// I think this is bank 7 on left at top, bank 6 on left at bottom
// The best place would then be DCM_X0Y0, the bottom left DCM; although all the gbufs are central, so X2Y0 is better
DCM clk_phases_gen(       .CLKIN (sys_drm_clock_in),
                          .CLKFB (int_drm_clock_buffered), // our reference signal, 4ns high, 4ns low
                          .CLK0 (int_drm_clock), // -4 to -2ns before int_drm_clock_buffered (dependent on gbuf delay; likely around 2.8ns)
                          .CLK180 (int_drm_clock_90), // -2ns to +0ns around int_drm_clock_buffered (+2ns from clk0)
                          .CLK2X (int_double_drm_clock), // -4 to -2ns before int_drm_clock_buffered, (dependent on gbuf delay; likely around 2.8ns)
                          .LOCKED (dcm_locked[0] )
    );
//synthesis attribute LOC of clk_phases_gen is "DCM_X2Y0";

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

BUFG drm_clock_buffer( .I(int_drm_clock), .O(int_drm_clock_buffered) ); // reference clock, 4ns high, 4ns low
BUFG drm_clock_90_buffer( .I(int_drm_clock_90), .O(int_drm_clock_90_buffered) ); // +1.5ns to +2.5ns after int_drm_clock_buffered
BUFG drm_double_clock_buffer( .I(int_double_drm_clock), .O(int_double_drm_clock_buffered) ); // 2ns high, 2ns low, starting -0.5 to +0.5 around int_drm_clock_buffered (skew in gbufs)
//synthesis attribute clock_signal of int_double_drm_clock_buffered is yes;
//synthesis attribute clock_signal of int_drm_clock_buffered is yes;
//synthesis attribute clock_signal of int_drm_clock_90_buffered is yes;
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
                              .CLK0 (int_input_drm_clock),
                              .CLKFB (int_input_drm_clock_buffered),
                              .LOCKED (dcm_locked[1] )
                    );
// synthesis attribute LOC of dram_input_pin_gen is "DCM_X3Y0";

// synthesis attribute CLK_FEEDBACK of dram_input_pin_gen is "1X"; 
// synthesis attribute CLKIN_PERIOD of dram_input_pin_gen is "8 ns";
// synthesis attribute CLKFX_MULTIPLY of dram_input_pin_gen is 2;
// synthesis attribute CLKFX_DIVIDE of dram_input_pin_gen is 1;
// synthesis attribute CLKIN_DIVIDE_BY_2 of dram_input_pin_gen is 0;
// synthesis attribute CLKOUT_PHASE_SHIFT of dram_input_pin_gen is "FIXED"; 
// synthesis attribute DESKEW_ADJUST of dram_input_pin_gen is "SYSTEM_SYNCHRONOUS";
// synthesis attribute DFS_FREQUENCY_MODE of dram_input_pin_gen is "LOW"; 
// synthesis attribute DLL_FREQUENCY_MODE of dram_input_pin_gen is "LOW"; 
// synthesis attribute DSS_MODE of dram_input_pin_gen is "NONE"; 
// synthesis attribute DUTY_CYCLE_CORRECTION of dram_input_pin_gen is "FALSE";
// synthesis attribute PHASE_SHIFT of dram_input_pin_gen is 80;
// synthesis attribute STARTUP_WAIT of dram_input_pin_gen is 1;

BUFG drm_input_clock_buffer( .I(int_input_drm_clock), .O(int_input_drm_clock_buffered) );

//synthesis attribute clock_signal of int_input_drm_clock_buffered is yes;

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
