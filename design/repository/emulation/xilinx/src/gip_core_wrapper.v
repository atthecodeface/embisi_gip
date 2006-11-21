
//a Core wrapper module
module gip_core_wrapper
(
    system_clock_in,

    switches,
    rxd,
    txd,
    leds,

    fetch_16,
    fetch_op,
    prefetch_op,
    prefetch_address_32,
    fetch_flush,
    fetch_data_32,
    fetch_data_valid,
    fetch_pc_32,

    rfr_periph_read,
    rfr_periph_read_global,
    rfr_periph_read_address_5,
    rfw_periph_write,
    rfw_periph_write_global,
    rfw_periph_write_address_5,
    rfw_periph_write_data_32,
    gip_pipeline_flush,

    postbus_tx_type_2,
    postbus_tx_data_32,
    postbus_rx_ack,

    alu_mem_op_3,
    alu_mem_options_3,
    alu_mem_address_32,
    alu_mem_write_data_32,
    alu_mem_burst_4

);

//b Inputs and outputs
    //b Clocks
    input system_clock_in;

    //b Switches and LEDs
    input [7:0]switches;
    output [7:0]leds;

    //b UART
    input rxd;
    output txd;

    //b Fetch interface
    output fetch_16;
    output[1:0] fetch_op;
    output[1:0] prefetch_op;
    output[31:0] prefetch_address_32;
    output fetch_flush;
    input[31:0] fetch_data_32;
    input fetch_data_valid;
    input[31:0] fetch_pc_32;

    //b RF interface
    output rfr_periph_read;
    output rfr_periph_read_global;
    output[4:0] rfr_periph_read_address_5;
    output rfw_periph_write;
    output rfw_periph_write_global;
    output[4:0] rfw_periph_write_address_5;
    output[31:0] rfw_periph_write_data_32;
    output gip_pipeline_flush;

    //b Postbus interface
    output[1:0] postbus_tx_type_2;
    output[31:0] postbus_tx_data_32;
    output postbus_rx_ack;

    //b Memory interface
    output[2:0] alu_mem_op_3;
    output[2:0] alu_mem_options_3;
    output[31:0] alu_mem_address_32;
    output[31:0] alu_mem_write_data_32;
    output[3:0] alu_mem_burst_4;

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


//b Assignments to outputs
    assign txd=rxd ^ switches[0] ^ switches[7];
    assign leds = alu_mem_write_data_32[7:0];

//b Instantiate the core
    gip_core core( .gip_clock(gip_clock),
                   .gip_fast_clock(system_clock_in),
                   .gip_clock_phase(clock_phase),
                   .gip_reset(switches[0]),

                 .fetch_16(fetch_16),
                 .fetch_op(fetch_op), // Early in the cycle, so data may be returned combinatorially
                 .fetch_data(fetch_data_32),
                 .fetch_data_valid(fetch_data_valid),
                 .fetch_pc(fetch_pc_32),
                 .prefetch_op(prefetch_op), // Late in the cycle; can be used to start an SRAM cycle in the clock edge for fetch data in next cycle (if next cycle fetch requests it)
                 .prefetch_address(prefetch_address_32),
                 .fetch_flush(fetch_flush),

                 .rfr_periph_read(rfr_periph_read),
                 .rfr_periph_read_global(rfr_periph_read_global),
                 .rfr_periph_read_address(rfr_periph_read_address_5),
                 .rfr_periph_read_data_valid(switches[4]), //'
                 .rfr_periph_read_data(32'b0), //'
                 .rfr_periph_busy(switches[3]), //'
                 .rfw_periph_write(rfw_periph_write),
                 .rfw_periph_write_global(rfw_periph_write_global),
                 .rfw_periph_write_address(rfw_periph_write_address_5),
                 .rfw_periph_write_data(rfw_periph_write_data_32),
                 .gip_pipeline_flush(gip_pipeline_flush),

                 .alu_mem_op(alu_mem_op_3),
                 .alu_mem_options(alu_mem_options_3),
                 .alu_mem_address(alu_mem_address_32),
                 .alu_mem_write_data(alu_mem_write_data_32),
                 .alu_mem_burst(alu_mem_burst_4),

                 .mem_alu_busy(switches[1]), //'
                 .mem_read_data_valid(switches[2]), //'
                 .mem_read_data(32'b0), //'

                 .local_events_in(switches),

                 .postbus_tx_type(postbus_tx_type_2),
                 .postbus_tx_data(postbus_tx_data_32),
                 .postbus_tx_ack(switches[3]), //'

                 .postbus_rx_type(switches[6:5]), // ' 1 is idle
                 .postbus_rx_data(32'b0), // '
                 .postbus_rx_ack(postbus_rx_ack)

                 );

//b Memory interface
// The theory is that a single memory can, in 5 cycles, be the RF read ports (2), RF write port, instruction fetch and data read/write
// Actually at most 4 will in general be required, and a stall could be possible.
// The thought is RFW in first cycle, RFReads in second and third, data read/write fourth, and instruction read fifth.
// Now, the RFW controls are set up in cycle 1, and the write can occur in cycle 2, while the signals for read are set, and RF port 1 read data is in cycle 3
// In reality the writes should come after the reads: their results are not important. This requires little time for the read controls to settle prior
// to the first (fast) clock tick following the GIP clock.
// Also, we have dual port memories available - we could do two RF reads, , data read/write, instruction read / RF write
// The instruction fetch should be pushed as late as possible as the instruction decode is our long pole.
// Plausibly we could reach a 3-cycle-per-instruction setup, so 60-80MHz.
// We might also want to investigate going to separate instruction/data and RF RAMS, with the postbus instantiated with the RF in the second RAM.

//b End module
endmodule
