
//a Sample DCM
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


//b Phase shift DCM 4
//old //
//old // Generate the internal input pin clock
//old // this uses the main clock, and does a fine phase shift by 1/3 of a clock (i.e. 4ns)
//old // this balances clock-to-out delay of 2ns, Tac of 0.75ns, and some trace delay
//old DCM dram_input_pin_gen(       .CLKIN (int_output_drm_clock_buffered),
//old                               .RST (system_reset_in),
//old                               .CLK0 (int_input_drm_clock_90),
//old                               .CLKFB (int_input_drm_clock_90_buffered),
//old                               .LOCKED (dcm_locked[3] )
//old                     );
//old //ssynthesis attribute LOC of dram_input_pin_gen is "DCM_X5Y0";
//old 
//old // ssynthesis attribute CLK_FEEDBACK of dram_input_pin_gen is "1X"; 
//old // ssynthesis attribute CLKIN_DIVIDE_BY_2 of dram_input_pin_gen is 0;
//old // ssynthesis attribute CLKOUT_PHASE_SHIFT of dram_input_pin_gen is "FIXED"; 
//old // ssynthesis attribute DESKEW_ADJUST of dram_input_pin_gen is "SYSTEM_SYNCHRONOUS";
//old // ssynthesis attribute DFS_FREQUENCY_MODE of dram_input_pin_gen is "LOW"; 
//old // ssynthesis attribute DLL_FREQUENCY_MODE of dram_input_pin_gen is "LOW"; 
//old // ssynthesis attribute DSS_MODE of dram_input_pin_gen is "NONE"; 
//old // ssynthesis attribute DUTY_CYCLE_CORRECTION of dram_input_pin_gen is "FALSE";
//old // ssynthesis attribute PHASE_SHIFT of dram_input_pin_gen is 96;
//old // ssynthesis attribute STARTUP_WAIT of dram_input_pin_gen is 1;
//old 
//old BUFG drm_input_clock_buffer( .I(int_input_drm_clock), .O(int_input_drm_clock_buffered) );
//old BUFG drm_input_90_clock_buffer( .I(int_input_drm_clock_90), .O(int_input_drm_clock_90_buffered) );
//old 
//old //ssynthesis attribute clock_signal of int_input_drm_clock_buffered is yes;
//old //ssynthesis attribute clock_signal of int_input_drm_clock_90_buffered is yes;
//old //ssynthesis attribute PERIOD of int_input_drm_clock_buffered is "12 ns";
//old //ssynthesis attribute PERIOD of int_input_drm_clock_90_buffered is "12 ns";

//a Old DCM 4
//b DCM 4
//old //
//old // Generate the internal input pin clock
//old // this uses the clk LVDS input pins to generate a clock to buffer the input data
//old // the LVDS input pins are AF29/AE28(IO_L24NP_6) ck1 and P27/N27(IO_L68NP_7)
//old // Actually we use a dedicated feedback pin which is in bank 5...
//old // This should be placed near its input
//old // The pads are IO_(L|R)([0-9][0-9])(N|P)_(B)
//old // $1 = L|R (must also be top or bottom) I think
//old // $2 = pad number (0 at bottom/top of left, 96 in middle)
//old // $3 = N or P for which of LVDS pair
//old // $4 = bank number
//old // Lets put it at the top on the left, DCM_X0Y1. Hm, there is a warning about this, since we are using bank 5 for the input clock pin
//old // Try instead at DCM_X5Y0
//old DCM dram_input_pin_gen(       .CLKIN (feedback_ddr_clock),
//old                               .RST (system_reset_in),
//old                               .CLK0 (int_input_drm_clock),
//old                               .CLK90 (int_input_drm_clock_90),
//old                               .CLKFB (int_input_drm_clock_buffered),
//old                               .LOCKED (dcm_locked[3] )
//old                     );
//old // ssynthesis attribute LOC of dram_input_pin_gen is "DCM_X5Y0";
//old 
//old // ssynthesis attribute CLK_FEEDBACK of dram_input_pin_gen is "1X"; 
//old // ssynthesis attribute CLKIN_DIVIDE_BY_2 of dram_input_pin_gen is 0;
//old // ssynthesis attribute CLKOUT_PHASE_SHIFT of dram_input_pin_gen is "NONE"; 
//old // ssynthesis attribute DESKEW_ADJUST of dram_input_pin_gen is "SYSTEM_SYNCHRONOUS";
//old // ssynthesis attribute DFS_FREQUENCY_MODE of dram_input_pin_gen is "LOW"; 
//old // ssynthesis attribute DLL_FREQUENCY_MODE of dram_input_pin_gen is "LOW"; 
//old // ssynthesis attribute DSS_MODE of dram_input_pin_gen is "NONE"; 
//old // ssynthesis attribute DUTY_CYCLE_CORRECTION of dram_input_pin_gen is "FALSE";
//old // ssynthesis attribute PHASE_SHIFT of dram_input_pin_gen is 0;
//old // ssynthesis attribute STARTUP_WAIT of dram_input_pin_gen is 1;
//old 
//old BUFG drm_input_clock_buffer( .I(int_input_drm_clock), .O(int_input_drm_clock_buffered) );
//old BUFG drm_input_90_clock_buffer( .I(int_input_drm_clock_90), .O(int_input_drm_clock_90_buffered) );
//old 
//old //ssynthesis attribute clock_signal of int_input_drm_clock_buffered is yes;
//old //ssynthesis attribute clock_signal of int_input_drm_clock_90_buffered is yes;
//old //ssynthesis attribute PERIOD of int_input_drm_clock_buffered is "12 ns";
//old //ssynthesis attribute PERIOD of int_input_drm_clock_90_buffered is "12 ns";
//old 
//old 

//a Old DCM 3
//old //b DCM 3
//old //
//old // Generate the internal logic DRAM clock
//old // This does not need to be close to any pads, but the related logic will be on the left side near the pads
//old // It connects to DCM2 at DCM_X0Y0, and a GBUF; so use DCM_X2Y0
//old DCM dram_logic_gen(       .CLKIN (int_output_drm_clock_buffered),
//old                           .CLK0 (int_logic_drm_clock),
//old                           .CLKFB (int_logic_drm_clock_buffered),
//old                           .LOCKED (dcm_locked[2] )
//old                     );
//old //ssynthesis attribute LOC of dram_logic_gen is "DCM_X2Y0";
//old 
//old // ssynthesis attribute CLK_FEEDBACK of dram_logic_gen is "1X"; 
//old // ssynthesis attribute CLKIN_DIVIDE_BY_2 of dram_logic_gen is 0;
//old // ssynthesis attribute CLKOUT_PHASE_SHIFT of dram_logic_gen is "NONE"; 
//old // ssynthesis attribute DESKEW_ADJUST of dram_logic_gen is "SYSTEM_SYNCHRONOUS";
//old // ssynthesis attribute DFS_FREQUENCY_MODE of dram_logic_gen is "LOW"; 
//old // ssynthesis attribute DLL_FREQUENCY_MODE of dram_logic_gen is "LOW"; 
//old // ssynthesis attribute DSS_MODE of dram_logic_gen is "NONE"; 
//old // ssynthesis attribute DUTY_CYCLE_CORRECTION of dram_logic_gen is "FALSE";
//old // ssynthesis attribute PHASE_SHIFT of dram_logic_gen is 0;
//old // ssynthesis attribute STARTUP_WAIT of dram_logic_gen is 1;
//old 
//old BUFG drm_logic_clock_buffer( .I(int_logic_drm_clock), .O(int_logic_drm_clock_buffered) );
//old 
//old //ssynthesis attribute clock_signal of int_logic_drm_clock_buffered is yes;
//old //ssynthesis attribute PERIOD of int_logic_drm_clock_buffered is "12 ns";
