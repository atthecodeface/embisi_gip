
//a Counter module
module counter
(
    system_clock_in,
    system_reset,
    counter,
    led

);

//b Inputs and outputs
    //b Clocks and reset
    input system_clock_in;
    input system_reset;

    //b Counter out, with led
    output [7:0]counter;
    output led;

    //b Clock counter
    reg[7:0] counter;
    reg counter_is_zero;
    always @(posedge system_clock_in)
    begin
        counter <= counter+1;
        counter_is_zero <= (counter==255);
    end

    //b LEDs
    assign led=counter_is_zero;

//b End module
endmodule
