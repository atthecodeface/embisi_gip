module clock_generator(
    sys_drm_clock_in,

    int_output_drm_clock,
    int_output_drm_clock_90,
    int_output_drm_clock_180,
    int_output_drm_clock_270,

    int_logic_drm_clock_buffered,
    int_logic_slow_clock_buffered,
    cke_gl_to_drm,
    cke_drm_to_gl
);

    input sys_drm_clock_in;
    output int_output_drm_clock;
    output int_output_drm_clock_90;
    output int_output_drm_clock_180;
    output int_output_drm_clock_270;

    output int_logic_drm_clock_buffered;
    output int_logic_slow_clock_buffered;
    output cke_gl_to_drm;
    output cke_drm_to_gl;

// Generate the main clock frequency (83MHz)
DCM clk_freq_gen(       .CLKIN (sys_drm_clock_in),
                        .CLKFX (int_starting_drm_clock)
    );
// Multiply up from 125MHz/8ns to 500MHz/2ns
// Divide from 500MHz/ns to 83MHz/12ns
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

// Generate the output DRAM clocks with 0, 90, 180 and 270 phase
DCM clk_phases_gen(       .CLKIN (int_starting_drm_clock),
                          .CLKFB (int_output_drm_clock),
                          .CLK0 (int_output_drm_clock),
                          .CLK90 (int_output_drm_clock_90),
                          .CLK180 (int_output_drm_clock_180),
                          .CLK270 (int_output_drm_clock_270)
    );
// synthesis attribute CLK_FEEDBACK of clk_phases_gen is "1X";
// synthesis attribute CLKOUT_PHASE_SHIFT of clk_phases_gen is "NONE"; 
// synthesis attribute DESKEW_ADJUST of clk_phases_gen is "SYSTEM_SYNCHRONOUS";
// synthesis attribute DFS_FREQUENCY_MODE of clk_phases_gen is "LOW";
// synthesis attribute DLL_FREQUENCY_MODE of clk_phases_gen is "LOW";
// synthesis attribute DSS_MODE of clk_phases_gen is "NONE"; 
// synthesis attribute DUTY_CYCLE_CORRECTION of clk_phases_gen is "FALSE";
// synthesis attribute PHASE_SHIFT of clk_phases_gen is 0;
// synthesis attribute STARTUP_WAIT of clk_phases_gen is 1;
//synthesis attribute clock_signal of int_output_drm_clock is yes;
//synthesis attribute clock_signal of int_output_drm_clock_180 is yes;
//synthesis attribute clock_signal of int_output_drm_clock_90 is yes;
//synthesis attribute clock_signal of int_output_drm_clock_270 is yes;

// Generate the internal logic DRAM clock
DCM dram_logic_gen(       .CLKIN (int_output_drm_clock),
                          .CLK0 (int_logic_drm_clock),
                          .CLKFB (int_logic_drm_clock_buffered)
                    );

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

// Divide the internal logic DRAM clock, and generate two enables
reg [2:0]clock_divider;
reg divided_clock;
reg cke_gl_to_drm;
reg cke_drm_to_gl;
always @(posedge int_logic_drm_clock_buffered)
begin
    clock_divider <= clock_divider+1;
    cke_gl_to_drm = 0;
    cke_drm_to_gl = 0;
    if (clock_divider==7)
    begin
        clock_divider <= 0;
    end
    if (clock_divider==0)
    begin
        cke_drm_to_gl = 1;
    end
    if (clock_divider==1)
    begin
        divided_clock <= !divided_clock;
    end
    if (clock_divider==2)
    begin
        cke_gl_to_drm = 1;
    end
end

// Run a DCM on the output to get zero skew from this to the clock tree
DCM slow_logic_gen(       .CLKIN (divided_clock),
                        .CLK0 (int_logic_slow_clock),
                        .CLKFB (int_logic_slow_clock_buffered)
                );

// synthesis attribute CLK_FEEDBACK of slow_logic_gen is "1X"; 
// synthesis attribute CLKIN_DIVIDE_BY_2 of slow_logic_gen is 0;
// synthesis attribute CLKOUT_PHASE_SHIFT of slow_logic_gen is "NONE"; 
// synthesis attribute DESKEW_ADJUST of slow_logic_gen is "SYSTEM_SYNCHRONOUS";
// synthesis attribute DFS_FREQUENCY_MODE of slow_logic_gen is "LOW"; 
// synthesis attribute DLL_FREQUENCY_MODE of slow_logic_gen is "LOW"; 
// synthesis attribute DSS_MODE of slow_logic_gen is "NONE"; 
// synthesis attribute DUTY_CYCLE_CORRECTION of slow_logic_gen is "FALSE";// (TRUE, FALSE)
// synthesis attribute PHASE_SHIFT of slow_logic_gen is 0;
// synthesis attribute STARTUP_WAIT of slow_logic_gen is 1;

BUFG slow_logic_clock_buffer( .I(int_logic_slow_clock), .O(int_logic_slow_clock_buffered) );
//synthesis attribute clock_signal of int_logic_slow_clock_buffered is yes;


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


