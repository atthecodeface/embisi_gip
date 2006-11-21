//a ALU wrapper module
module gip_alu_wrapper
(
    system_clock_in,

    switches,
    leds,

    bus_a, bus_b, bus_c, bus_d, bus_e, bus_f,

    io_0, io_1, io_2, io_3, io_4, io_5, io_6, io_7, io_8, io_9,
    io_10, io_11, io_12, io_13, io_14, io_15, io_16, io_17, io_18, io_19,
    io_20, io_21, io_22, io_23, io_24, io_25, io_26, io_27, io_28, io_29,
    io_30, io_31, io_32, io_33, io_34, io_35, io_36, io_37, io_38, io_39,
    io_40, io_41, io_42, io_43, io_44, io_45, io_46, io_47, io_48, io_49,
    io_50, io_51, io_52, io_53, io_54, io_55, io_56, io_57, io_58, io_59,
    io_60, io_61, io_62, io_63, io_64, io_65, io_66, io_67, io_68

);

//b Inputs and outputs
    //b Clocks
    input system_clock_in;

    //b Switches and LEDs
    input [7:0]switches;
    output [7:0]leds;

    //b Clock divider
    // divider:    5 4 3 2 1 0 5 4 3 2 1 0 5 4 3 2 1 0
    // gip_clock:  x x l l l H H H l l l H H H l l l H
    // phase:      l l l l H l l l l l H l l l l l H l
    // i.e. phase is high on clock_in 'simultaneously with' rising gip_clock
    reg[2:0] divider;
    reg clock_phase;
    reg gip_clock;
    always @(posedge system_clock_in)
    begin
        divider <= divider-1;
        clock_phase <= 0;
        case (divider)
        0: divider <= 5;
        1: gip_clock <= 1;
        2: clock_phase <= 1;
        4: gip_clock <= 0;
        endcase
    end

    //b Other inputs
    input [31:0]bus_a;
    input [31:0]bus_b;
    input [31:0]bus_c;

    input io_0;
    input io_1;
    input io_2;
    input io_3;
    input io_4;
    input io_5;
    input io_6;
    input io_7;
    input io_8;
    input io_9;
    input io_10;
    input io_11;
    input io_12;
    input io_13;
    input io_14;
    input io_15;
    input io_16;
    input io_17;
    input io_18;
    input io_19;

    //b Other outputs
    output [31:0]bus_d;
    output [31:0]bus_e;
    output [31:0]bus_f;
    output io_20;
    output io_21;
    output io_22;
    output io_23;
    output io_24;
    output io_25;
    output io_26;
    output io_27;
    output io_28;
    output io_29;
    output io_30;
    output io_31;
    output io_32;
    output io_33;
    output io_34;
    output io_35;
    output io_36;
    output io_37;
    output io_38;
    output io_39;
    output io_40;
    output io_41;
    output io_42;
    output io_43;
    output io_44;
    output io_45;
    output io_46;
    output io_47;
    output io_48;
    output io_49;
    output io_50;
    output io_51;
    output io_52;
    output io_53;
    output io_54;
    output io_55;
    output io_56;
    output io_57;
    output io_58;
    output io_59;
    output io_60;
    output io_61;
    output io_62;
    output io_63;
    output io_64;
    output io_65;
    output io_66;
    output io_67;
    output io_68;

    reg [31:0]bus_d;
    reg [31:0]bus_e;
    reg [31:0]bus_f;
    reg io_20;
    reg io_21;
    reg io_22;
    reg io_23;
    reg io_24;
    reg io_25;
    reg io_26;
    reg io_27;
    reg io_28;
    reg io_29;
    reg io_30;
    reg io_31;
    reg io_32;
    reg io_33;
    reg io_34;
    reg io_35;
    reg io_36;
    reg io_37;
    reg io_38;
    reg io_39;
    reg io_40;
    reg io_41;
    reg io_42;
    reg io_43;
    reg io_44;
    reg io_45;
    reg io_46;
    reg io_47;
    reg io_48;
    reg io_49;
    reg io_50;
    reg io_51;
    reg io_52;
    reg io_53;
    reg io_54;
    reg io_55;
    reg io_56;
    reg io_57;
    reg io_58;
    reg io_59;
    reg io_60;
    reg io_61;
    reg io_62;
    reg io_63;
    reg io_64;
    reg io_65;
    reg io_66;
    reg io_67;
    reg io_68;

    //b Random data generator for inputs
    reg [31:0] random_data;
    always @(posedge gip_clock)
    begin
        random_data[31:8] <= random_data[23:0];
        random_data[7:0] <= random_data[31:24]^switches[7:0];
    end

    //b Input wires
    wire special_cp_trail_2;
    wire alu_accepting_rfr_instruction;
    wire mem_alu_busy;
    wire rfw_accepting_alu_rd;
    wire [31:0]rf_read_port_1;
    wire [31:0]rf_read_port_0;
    wire rfr_inst__valid;
    wire [2:0]rfr_inst__gip_ins_class;
    wire [3:0]rfr_inst__gip_ins_subclass;
    wire [3:0]rfr_inst__gip_ins_cc;
    wire [2:0]rfr_inst__gip_ins_rd__type;
    wire [4:0]rfr_inst__gip_ins_rd__r;
    wire [2:0]rfr_inst__gip_ins_rn__type;
    wire [4:0]rfr_inst__gip_ins_rn__r;
    wire [2:0]rfr_inst__gip_ins_rm__type;
    wire [4:0]rfr_inst__gip_ins_rm__r;
    wire rfr_inst__rm_is_imm;
    wire [31:0]rfr_inst__immediate;
    wire [3:0]rfr_inst__k;
    wire rfr_inst__a;
    wire rfr_inst__f;
    wire rfr_inst__s_or_stack;
    wire rfr_inst__p_or_offset_is_shift;
    wire rfr_inst__d;
    wire [31:0]rfr_inst__pc;
    wire [1:0]rfr_inst__tag;

    //b Input assignments - buses a-c, ios 0-10
    assign special_cp_trail_2 = io_10;
    assign alu_accepting_rfr_instruction = io_9;
    assign mem_alu_busy = io_8;
    assign rfw_accepting_alu_rd = io_7;
    assign rf_read_port_1 = bus_a;
    assign rf_read_port_0 = bus_b;
    assign rfr_inst__valid = io_6;
    assign rfr_inst__gip_ins_class = random_data[2:0];
    assign rfr_inst__gip_ins_subclass = random_data[6:3];
    assign rfr_inst__gip_ins_cc = random_data[10:7];
    assign rfr_inst__gip_ins_rd__type = random_data[10:8];
    assign rfr_inst__gip_ins_rd__r = random_data[15:11];
    assign rfr_inst__gip_ins_rn__type = random_data[18:16];
    assign rfr_inst__gip_ins_rn__r = random_data[23:19];
    assign rfr_inst__gip_ins_rm__type = random_data[26:24];
    assign rfr_inst__gip_ins_rm__r = random_data[31:27];
    assign rfr_inst__rm_is_imm = io_6;
    assign rfr_inst__immediate = bus_c;
    assign rfr_inst__k = io_0;
    assign rfr_inst__a = io_1;
    assign rfr_inst__f = io_2;
    assign rfr_inst__s_or_stack = io_3;
    assign rfr_inst__p_or_offset_is_shift = io_4;
    assign rfr_inst__d = io_5;
    assign rfr_inst__pc = 0;
    assign rfr_inst__tag = random_data[1:0];

    //b Output wires
    wire gip_pipeline_executing;
    wire [1:0]gip_pipeline_tag;
    wire gip_pipeline_flush;
    wire [3:0]alu_mem_burst;
    wire [31:0]alu_mem_write_data;
    wire [31:0]alu_mem_address;
    wire [2:0]alu_mem_rd__type;
    wire [4:0]alu_mem_rd__r;
    wire [2:0]alu_mem_options;
    wire [2:0]alu_mem_op;
    wire [31:0]alu_shifter_result;
    wire [31:0]alu_arith_logic_result;
    wire alu_use_shifter;
    wire [2:0]alu_rd__type;
    wire [4:0]alu_rd__r;
    wire alu_accepting_rfr_instruction_if_rfw_does;
    wire alu_accepting_rfr_instruction_if_mem_does;
    wire alu_accepting_rfr_instruction_always;
    wire alu_inst__valid;
    wire [2:0]alu_inst__gip_ins_class;
    wire [3:0]alu_inst__gip_ins_subclass;
    wire [3:0]alu_inst__gip_ins_cc;
    wire [2:0]alu_inst__gip_ins_rd__type;
    wire [4:0]alu_inst__gip_ins_rd__r;
    wire [2:0]alu_inst__gip_ins_rn__type;
    wire [4:0]alu_inst__gip_ins_rn__r;
    wire [2:0]alu_inst__gip_ins_rm__type;
    wire [4:0]alu_inst__gip_ins_rm__r;
    wire alu_inst__rm_is_imm;
    wire [31:0]alu_inst__immediate;
    wire [3:0]alu_inst__k;
    wire alu_inst__a;
    wire alu_inst__f;
    wire alu_inst__s_or_stack;
    wire alu_inst__p_or_offset_is_shift;
    wire alu_inst__d;
    wire [31:0]alu_inst__pc;
    wire [1:0]alu_inst__tag;

    //b Output assignments - buses d-f, ios 30-60
    reg [127:0]isr;
    reg [127:0]ixr;
    always @(posedge gip_clock)
        begin
            bus_d <= alu_mem_write_data ^ alu_mem_address;
            bus_e <= alu_shifter_result ^ alu_arith_logic_result;
            bus_f <= alu_inst__immediate ^ alu_inst__pc;
            io_30 <= gip_pipeline_executing;
            io_31 <= gip_pipeline_flush;
            io_32 <= alu_use_shifter;
            io_33 <= alu_accepting_rfr_instruction_if_rfw_does;
            io_34 <= alu_accepting_rfr_instruction_if_mem_does;
            io_35 <= alu_accepting_rfr_instruction_always;
            io_36 <= alu_inst__valid;
            io_37 <= alu_inst__rm_is_imm;
            io_38 <= alu_inst__a;
            io_39 <= alu_inst__f;
            io_40 <= alu_inst__s_or_stack;
            io_42 <= alu_inst__p_or_offset_is_shift;
            io_41 <= alu_inst__d;
            ixr <= 128'b0; //'
            ixr[1:0] <= gip_pipeline_tag;
            ixr[5:2] <= alu_mem_burst;
            ixr[8:6] <= alu_mem_rd__type;
            ixr[13:9] <= alu_mem_rd__r;
            ixr[16:14] <= alu_mem_options;
            ixr[19:17] <= alu_mem_op;
            ixr[22:20] <= alu_rd__type;
            ixr[27:23] <= alu_rd__r;
            ixr[30:28] <= alu_inst__gip_ins_class;
            ixr[34:31] <= alu_inst__gip_ins_subclass;
            ixr[38:35] <= alu_inst__gip_ins_cc;
            ixr[50:48] <= alu_inst__gip_ins_rd__type;
            ixr[55:51] <= alu_inst__gip_ins_rd__r;
            ixr[58:56] <= alu_inst__gip_ins_rn__type;
            ixr[63:59] <= alu_inst__gip_ins_rn__r;
            ixr[66:64] <= alu_inst__gip_ins_rm__type;
            ixr[71:67] <= alu_inst__gip_ins_rm__r;
            ixr[75:72] <= alu_inst__k;
            ixr[77:76] <= alu_inst__tag;
            isr[127:1] <= isr[126:0] ^ ixr;
            io_42 <= isr[127];
       end

//b gip_alu instantiation
    gip_alu galu ( .gip_clock(gip_clock),
                   .gip_fast_clock(system_clock_in),
                   .gip_clock_phase(clock_phase),

                   .gip_reset(switches[0]),

                   .special_cp_trail_2(special_cp_trail_2),
                   .alu_accepting_rfr_instruction(alu_accepting_rfr_instruction),
                   .mem_alu_busy(mem_alu_busy),
                   .rfw_accepting_alu_rd(rfw_accepting_alu_rd),
                   .rf_read_port_1(rf_read_port_1),
                   .rf_read_port_0(rf_read_port_0),
                   .rfr_inst__valid(rfr_inst__valid),
                   .rfr_inst__gip_ins_class(rfr_inst__gip_ins_class),
                   .rfr_inst__gip_ins_subclass(rfr_inst__gip_ins_subclass),
                   .rfr_inst__gip_ins_cc(rfr_inst__gip_ins_cc),
                   .rfr_inst__gip_ins_rd__type(rfr_inst__gip_ins_rd__type),
                   .rfr_inst__gip_ins_rd__r(rfr_inst__gip_ins_rd__r),
                   .rfr_inst__gip_ins_rn__type(rfr_inst__gip_ins_rn__type),
                   .rfr_inst__gip_ins_rn__r(rfr_inst__gip_ins_rn__r),
                   .rfr_inst__gip_ins_rm__type(rfr_inst__gip_ins_rm__type),
                   .rfr_inst__gip_ins_rm__r(rfr_inst__gip_ins_rm__r),
                   .rfr_inst__rm_is_imm(rfr_inst__rm_is_imm),
                   .rfr_inst__immediate(rfr_inst__immediate),
                   .rfr_inst__k(rfr_inst__k),
                   .rfr_inst__a(rfr_inst__a),
                   .rfr_inst__f(rfr_inst__f),
                   .rfr_inst__s_or_stack(rfr_inst__s_or_stack),
                   .rfr_inst__p_or_offset_is_shift(rfr_inst__p_or_offset_is_shift),
                   .rfr_inst__d(rfr_inst__d),
                   .rfr_inst__pc(rfr_inst__pc),
                   .rfr_inst__tag(rfr_inst__tag),

                   .gip_pipeline_executing(gip_pipeline_executing),
                   .gip_pipeline_tag(gip_pipeline_tag),
                   .gip_pipeline_flush(gip_pipeline_flush),
                   .alu_mem_burst(alu_mem_burst),
                   .alu_mem_write_data(alu_mem_write_data),
                   .alu_mem_address(alu_mem_address),
                   .alu_mem_rd__type(alu_mem_rd__type),
                   .alu_mem_rd__r(alu_mem_rd__r),
                   .alu_mem_options(alu_mem_options),
                   .alu_mem_op(alu_mem_op),
                   .alu_shifter_result(alu_shifter_result),
                   .alu_arith_logic_result(alu_arith_logic_result),
                   .alu_use_shifter(alu_use_shifter),
                   .alu_rd__type(alu_rd__type),
                   .alu_rd__r(alu_rd__r),
                   .alu_accepting_rfr_instruction_if_rfw_does(alu_accepting_rfr_instruction_if_rfw_does),
                   .alu_accepting_rfr_instruction_if_mem_does(alu_accepting_rfr_instruction_if_mem_does),
                   .alu_accepting_rfr_instruction_always(alu_accepting_rfr_instruction_always),
                   .alu_inst__valid(alu_inst__valid),
                   .alu_inst__gip_ins_class(alu_inst__gip_ins_class),
                   .alu_inst__gip_ins_subclass(alu_inst__gip_ins_subclass),
                   .alu_inst__gip_ins_cc(alu_inst__gip_ins_cc),
                   .alu_inst__gip_ins_rd__type(alu_inst__gip_ins_rd__type),
                   .alu_inst__gip_ins_rd__r(alu_inst__gip_ins_rd__r),
                   .alu_inst__gip_ins_rn__type(alu_inst__gip_ins_rn__type),
                   .alu_inst__gip_ins_rn__r(alu_inst__gip_ins_rn__r),
                   .alu_inst__gip_ins_rm__type(alu_inst__gip_ins_rm__type),
                   .alu_inst__gip_ins_rm__r(alu_inst__gip_ins_rm__r),
                   .alu_inst__rm_is_imm(alu_inst__rm_is_imm),
                   .alu_inst__immediate(alu_inst__immediate),
                   .alu_inst__k(alu_inst__k),
                   .alu_inst__a(alu_inst__a),
                   .alu_inst__f(alu_inst__f),
                   .alu_inst__s_or_stack(alu_inst__s_or_stack),
                   .alu_inst__p_or_offset_is_shift(alu_inst__p_or_offset_is_shift),
                   .alu_inst__d(alu_inst__d),
                   .alu_inst__pc(alu_inst__pc),
                   .alu_inst__tag(alu_inst__tag) );

//b End module
endmodule
