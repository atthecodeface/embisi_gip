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
// The structure of the DCMs is...
// DCM 1: take 125MHz, generate 83MHz
// DCM 2: take 83MHz, generate int_output_drm_clock and int_output_drm_clock_90
// DCM 3: mirror DCM 2, generate int_logic_drm_clock_buffered using a GBUF with the feedback
// DCM 4: take input pin CLK and generate int_input_drm_clock_90
// We also divide by 12 to get our internal clock frequency
// DCM 5: take divided clock and generate zero-skew to internal clock tree using a GBUF with the feedback
module clock_generator(
    sys_drm_clock_in,
    system_reset_in, // this resets the pin-feedback clock
    system_reset_out, // this is asserted when all the DLLs are locked

    int_output_drm_clock_buffered, // use for clock out only

    feedback_ddr_clock, // feed back from the LVDS input pads for the clock out
    int_input_drm_clock_90_buffered, // use for dq input clocking only

    int_logic_drm_clock_buffered, // use for all pads and internal DRAM logic, tight skew to output_drm_clock
    int_logic_slow_clock_buffered,
    cke_last_of_logic,

    info
);

    input sys_drm_clock_in;
    input system_reset_in;
    output system_reset_out;

    output int_output_drm_clock_buffered;

    input feedback_ddr_clock;
    output int_input_drm_clock_90_buffered; // buffered version of the fedback DDR clock, out of phase by 90

    output int_logic_drm_clock_buffered;
    output int_logic_slow_clock_buffered;
    output cke_last_of_logic; // asserted if the clock edge happening is the last of the logic clock low

    output [3:0] info;

    wire [4:0]dcm_locked;
    assign info = dcm_locked[4:1];

// ----------------------------------------------------------------------------
// DCM 1
// ----------------------------------------------------------------------------
// Generate the main clock frequency (83MHz)
// This can sit at either DCM_X0Y0 (bottom left) or DCM_X5Y0 (bottom right)
// It only generates a single signal that has a single target
// input SDR clock is in bank 5 (IO_L96N_5); this is middle of bank 5 (which is bottom at left)
// We can use the 5th on the bottom, DCM_X4Y0
// We don't want to use DCM_X3Y0, as that is right next to the GBUFs
DCM clk_freq_gen(       .CLKIN (sys_drm_clock_in),
                        .CLKFX (int_starting_drm_clock),
                        .LOCKED (dcm_locked[0] )
    );
//synthesis attribute LOC of clk_freq_gen is "DCM_X4Y0";
//synthesis attribute PERIOD of sys_drm_clock_in is "8 ns";

// Multiply up from 125MHz/8ns to 500MHz/2ns
// Divide from 500MHz/ns to 83MHz/12ns
// Gavin added the next line at 11pm Sunday
// synthesis attribute CLKIN_PERIOD of clk_freq_gen is "8 ns";
// synthesis attribute CLKFX_MULTIPLY of clk_freq_gen is 4;
// synthesis attribute CLKFX_DIVIDE of clk_freq_gen is 6;
// synthesis attribute CLKIN_DIVIDE_BY_2 of clk_freq_gen is 0;
// synthesis attribute CLKOUT_PHASE_SHIFT of clk_freq_gen is "NONE"; 
// synthesis attribute DESKEW_ADJUST of clk_freq_gen is "SYSTEM_SYNCHRONOUS";
// synthesis attribute DFS_FREQUENCY_MODE of clk_freq_gen is "LOW";
// synthesis attribute DLL_FREQUENCY_MODE of clk_freq_gen is "LOW";
// synthesis attribute DSS_MODE of clk_freq_gen is "NONE"; 
// synthesis attribute DUTY_CYCLE_CORRECTION of clk_freq_gen is "FALSE";
// synthesis attribute PHASE_SHIFT of clk_freq_gen is 0;
// synthesis attribute STARTUP_WAIT of clk_freq_gen is 1;

//synthesis attribute clock_signal of int_starting_drm_clock is yes;
//synthesis attribute PERIOD of int_starting_drm_clock is "12 ns";

BUFG drm_starting_clock_buffer( .I(int_starting_drm_clock), .O(int_starting_drm_clock_buffered) );
//synthesis attribute clock_signal of int_starting_drm_clock_buffered is yes;
//synthesis attribute PERIOD of int_starting_drm_clock_buffered is "12 ns";


// ----------------------------------------------------------------------------
// DCM 2
// ----------------------------------------------------------------------------
// Generate the output DRAM clocks with 0, 90, 180 and 270 phase
// This should go relatively near the DRAM pads; we don't want too much net between int_output_drm_clock and DCM3
// So this connects to DCM3, and should be relatively near the DRAM DQ and DQS pins
// DQ is split between banks 6 and 7; then again all the outputs are...
// Bank 7 is right balls at the top, bank 6 is right balls at the bottom (top view)
// I think this is bank 7 on left at top, bank 6 on left at bottom
// The best place would then be DCM_X0Y0, the bottom left DCM
DCM clk_phases_gen(       .CLKIN (int_starting_drm_clock_buffered),
                          .CLKFB (int_output_drm_clock_buffered),
                          .CLK0 (int_output_drm_clock),
                          .LOCKED (dcm_locked[1] )
    );
//synthesis attribute LOC of clk_phases_gen is "DCM_X0Y0";

// synthesis attribute CLK_FEEDBACK of clk_phases_gen is "1X";
// synthesis attribute CLKOUT_PHASE_SHIFT of clk_phases_gen is "NONE"; 
// synthesis attribute DESKEW_ADJUST of clk_phases_gen is "SYSTEM_SYNCHRONOUS";
// synthesis attribute DFS_FREQUENCY_MODE of clk_phases_gen is "LOW";
// synthesis attribute DLL_FREQUENCY_MODE of clk_phases_gen is "LOW";
// synthesis attribute DSS_MODE of clk_phases_gen is "NONE"; 
// synthesis attribute DUTY_CYCLE_CORRECTION of clk_phases_gen is "FALSE";
// synthesis attribute PHASE_SHIFT of clk_phases_gen is 0;
// synthesis attribute STARTUP_WAIT of clk_phases_gen is 1;

BUFG drm_output_clock_buffer( .I(int_output_drm_clock), .O(int_output_drm_clock_buffered) );
//synthesis attribute clock_signal of int_output_drm_clock_buffered is yes;
//synthesis attribute PERIOD of int_output_drm_clock_buffered is "12 ns";

// ----------------------------------------------------------------------------
// DCM 3
// ----------------------------------------------------------------------------
// Generate the internal logic DRAM clock
// This does not need to be close to any pads, but the related logic will be on the left side near the pads
// It connects to DCM2 at DCM_X0Y0, and a GBUF; so use DCM_X2Y0
DCM dram_logic_gen(       .CLKIN (int_output_drm_clock_buffered),
                          .CLK0 (int_logic_drm_clock),
                          .CLKFB (int_logic_drm_clock_buffered),
                          .LOCKED (dcm_locked[2] )
                    );
//synthesis attribute LOC of dram_logic_gen is "DCM_X2Y0";

// synthesis attribute CLK_FEEDBACK of dram_logic_gen is "1X"; 
// synthesis attribute CLKIN_DIVIDE_BY_2 of dram_logic_gen is 0;
// synthesis attribute CLKOUT_PHASE_SHIFT of dram_logic_gen is "NONE"; 
// synthesis attribute DESKEW_ADJUST of dram_logic_gen is "SYSTEM_SYNCHRONOUS";
// synthesis attribute DFS_FREQUENCY_MODE of dram_logic_gen is "LOW"; 
// synthesis attribute DLL_FREQUENCY_MODE of dram_logic_gen is "LOW"; 
// synthesis attribute DSS_MODE of dram_logic_gen is "NONE"; 
// synthesis attribute DUTY_CYCLE_CORRECTION of dram_logic_gen is "FALSE";// (TRUE, FALSE)
// synthesis attribute PHASE_SHIFT of dram_logic_gen is 0;
// synthesis attribute STARTUP_WAIT of dram_logic_gen is 1;

BUFG drm_logic_clock_buffer( .I(int_logic_drm_clock), .O(int_logic_drm_clock_buffered) );

//synthesis attribute clock_signal of int_logic_drm_clock_buffered is yes;
//synthesis attribute PERIOD of int_logic_drm_clock_buffered is "12 ns";

// ----------------------------------------------------------------------------
// DCM 4
// ----------------------------------------------------------------------------
// Generate the internal input pin clock
// this uses the clk LVDS input pins to generate a clock to buffer the input data
// the LVDS input pins are AF29/AE28(IO_L24NP_6) ck1 and P27/N27(IO_L68NP_7)
// Actually we use a dedicated feedback pin which is in bank 5...
// This should be placed near its input
// The pads are IO_(L|R)([0-9][0-9])(N|P)_(B)
// $1 = L|R (must also be top or bottom) I think
// $2 = pad number (0 at bottom/top of left, 96 in middle)
// $3 = N or P for which of LVDS pair
// $4 = bank number
// Lets put it at the top on the left, DCM_X0Y1. Hm, there is a warning about this, since we are using bank 5 for the input clock pin
// Try instead at DCM_X5Y0
DCM dram_input_pin_gen(       .CLKIN (feedback_ddr_clock),
                              .RST (system_reset_in),
                              .CLK0 (int_input_drm_clock),
                              .CLK90 (int_input_drm_clock_90),
                              .CLKFB (int_input_drm_clock_buffered),
                              .LOCKED (dcm_locked[3] )
                    );
//synthesis attribute LOC of dram_input_pin_gen is "DCM_X5Y0";

// synthesis attribute CLK_FEEDBACK of dram_input_pin_ is "1X"; 
// synthesis attribute CLKIN_DIVIDE_BY_2 of dram_input_pin_ is 0;
// synthesis attribute CLKOUT_PHASE_SHIFT of dram_input_pin_ is "NONE"; 
// synthesis attribute DESKEW_ADJUST of dram_input_pin_ is "SYSTEM_SYNCHRONOUS";
// synthesis attribute DFS_FREQUENCY_MODE of dram_input_pin_ is "LOW"; 
// synthesis attribute DLL_FREQUENCY_MODE of dram_input_pin_ is "LOW"; 
// synthesis attribute DSS_MODE of dram_input_pin_ is "NONE"; 
// synthesis attribute DUTY_CYCLE_CORRECTION of dram_input_pin_ is "FALSE";// (TRUE, FALSE)
// synthesis attribute PHASE_SHIFT of dram_input_pin_ is 0;
// synthesis attribute STARTUP_WAIT of dram_input_pin_ is 1;

BUFG drm_input_clock_buffer( .I(int_input_drm_clock), .O(int_input_drm_clock_buffered) );
BUFG drm_input_90_clock_buffer( .I(int_input_drm_clock_90), .O(int_input_drm_clock_90_buffered) );

//synthesis attribute clock_signal of int_input_drm_clock_buffered is yes;
//synthesis attribute clock_signal of int_input_drm_clock_90_buffered is yes;
//synthesis attribute PERIOD of int_input_drm_clock_buffered is "12 ns";
//synthesis attribute PERIOD of int_input_drm_clock_90_buffered is "12 ns";

// ----------------------------------------------------------------------------
// Divider and DCM 5 ; actually no DCM as we are below ClkMin for the DCM!
// ----------------------------------------------------------------------------
// Divide the internal logic DRAM clock, and generate two enables
// Put the DCM in the middle at the bottom (DCM_X3Y0), so that it drives the BUFG well
// The logic gates should be right next to it
reg [3:0]clock_divider;
reg divided_clock;
reg cke_last_of_logic;
always @(posedge int_logic_drm_clock_buffered)
begin
    clock_divider <= clock_divider+1;
    cke_last_of_logic <= 0;
    if (clock_divider==10)
    begin
        cke_last_of_logic <= 1;
    end
    if (clock_divider==5)
    begin
        divided_clock <= 0;
    end
    if (cke_last_of_logic)
    begin
        clock_divider <= 0;
        divided_clock <= 1;
    end
end

// Run a DCM on the output to get zero skew from this to the clock tree
//DCM slow_logic_gen(     .CLKIN (divided_clock),
//                        .RST (system_reset_in),
//                        .CLK0 (int_logic_slow_clock),
//                        .CLKFB (int_logic_slow_clock_buffered),
//                        .LOCKED (dcm_locked[4] )
//                );
assign dcm_locked[4] = 1;
assign int_logic_slow_clock = divided_clock;
//no synthesis attribute LOC of slow_logic_gen is "DCM_X3Y0";

// no synthesis attribute CLK_FEEDBACK of slow_logic_gen is "1X"; 
// no synthesis attribute CLKIN_DIVIDE_BY_2 of slow_logic_gen is 0;
// no synthesis attribute CLKOUT_PHASE_SHIFT of slow_logic_gen is "NONE"; 
// no synthesis attribute DESKEW_ADJUST of slow_logic_gen is "SYSTEM_SYNCHRONOUS";
// no synthesis attribute DFS_FREQUENCY_MODE of slow_logic_gen is "LOW"; 
// no synthesis attribute DLL_FREQUENCY_MODE of slow_logic_gen is "LOW"; 
// no synthesis attribute DSS_MODE of slow_logic_gen is "NONE"; 
// no synthesis attribute DUTY_CYCLE_CORRECTION of slow_logic_gen is "FALSE";// (TRUE, FALSE)
// no synthesis attribute PHASE_SHIFT of slow_logic_gen is 0;
// no synthesis attribute STARTUP_WAIT of slow_logic_gen is 1;

BUFG slow_logic_clock_buffer( .I(int_logic_slow_clock), .O(int_logic_slow_clock_buffered) );

//synthesis attribute clock_signal of int_logic_slow_clock_buffered is yes;
//synthesis attribute PERIOD of int_logic_slow_clock_buffered is "100 ns";

// ----------------------------------------------------------------------------
// Reset output
// ----------------------------------------------------------------------------
reg system_reset_out;
always @(posedge int_logic_slow_clock_buffered or posedge system_reset_in)
begin
    if (system_reset_in)
    begin
        system_reset_out <= 1;
    end
    else
    begin
        if (dcm_locked==5'b11111) //'
        begin
            system_reset_out <= 0;
        end
    end
end

endmodule

// Generate the main clock frequency (83MHz) and its inverse
//DCM clk_freq_gen(       .CLKIN (sys_drm_clock_in),
//                        //.CLK0 (int_drm_clock),
//                        //.CLK180 (int_drm_clock_n), 
//                        //.CLK270 (user_CLK270),
//                        //.CLK2X (user_CLK2X),
//                        //.CLK2X180 (user_CLK2X180),
//                        //.CLK90 (user_CLK90), 
//                        //.CLKDV (user_CLKDV),
//                        .CLKFX (int_output_drm_clock),
//                        .CLKFX180 (int_output_drm_clock_n),
//                        //.LOCKED (user_LOCKED),
//                        //.PSDONE (user_PSDONE),
//                        //.STATUS (user_STATUS),
//                        //.CLKFB (user_CLKFB), 
//                        //.DSSEN (user_DSSEN),
//                        //.PSCLK (user_PSCLK),
//                        //.PSEN (user_PSEN),
//                        //.PSINCDEC (user_PSINCDEC),
//                        //.RST (user_RST)
//    );
////defparam clk_freq_gen.CLK_FEEDBACK => "string_value"; None needed, as we are running FX
////defparam clk_freq_gen.CLKDV_DIVIDE = integer_value; //(1.5,2,2.5,3,4,5,8,16)
//defparam clk_freq_gen.CLKFX_MULTIPLY => 4; // Multiply up from 125MHz/8ns to 500MHz/2ns
//defparam clk_freq_gen.CLKFX_DIVIDE => 6; // Divide from 500MHz/ns to 83MHz/12ns
//defparam clk_freq_gen.CLKIN_DIVIDE_BY_2 => FALSE;
//defparam clk_freq_gen.CLKOUT_PHASE_SHIFT => "NONE"; 
//defparam clk_freq_gen.DESKEW_ADJUST => "SYSTEM_SYNCHRONOUS";
//defparam clk_freq_gen.DFS_FREQUENCY_MODE => "LOW";  // Low frequency mode
//defparam clk_freq_gen.DLL_FREQUENCY_MODE => "LOW";   // Low frequency mode
//defparam clk_freq_gen.DSS_MODE => "NONE"; 
//defparam clk_freq_gen.DUTY_CYCLE_CORRECTION => "FALSE";// (TRUE, FALSE)
//defparam clk_freq_gen.PHASE_SHIFT => 0;
//
//defparam clk_freq_gen.STARTUP_WAIT => TRUE; // (TRUE, FALSE)


