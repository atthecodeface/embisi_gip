<?php

include "web_locals.php";
include "${toplevel}web_assist/web_globals.php";

site_set_location($location);

$page_title = "GIP Documentation";

include "${toplevel}web_assist/web_header.php";

page_header( "GIP Prefetch" );

page_sp();
?>

<?php page_section( "overview", "Overview" ); ?>

The GIP prefetch unit is basically responsible for supplying instructions to the GIP decode, with as low a latency as is reasonable.

The decode unit supplies an immediate request, which must be satisfied combinatorially; the prefetchs response is a 32-bit data word and a valid signal.

The types of fetch that the decode can request are:

1. pipeline restart; new PC is presented

2. pipeline continues with 16-bit instruction from last address presented

3. 

So the basic flow can be:

Decode idle

Decode schedule native

Decode; PC not requested yet
Fetch new pc ('a')
    Prefetch sequential 16
Prefetch combinatorially returns invalid data
Prefetch clocks in PC and reads SRAM in following cycle

Decode does not have valid instruction, but request has been placed for 'a'
    Fetch pending request
    Prefetch sequential 16
Prefetch combinatorially returns valid data 'a' from pending request, from SRAM path directly for SRAM read
Decode clocks valid instruction in its active register

Decode decodes inst 'a', which is not a taken branch; next pc op is sequential
    Fetch last prefetched
    Prefetch (from next pc op) is sequential
Prefetch logic has 16-bit data in register; it passes that 'a+1', and using next prefetch op of sequential starts SRAM read of next word

Decode decodes inst 'a+1' which is a taken branch; next pc op is branch to 'b', no delay
    Fetch new pc 'b'
    Prefetch sequential 16
Prefetch does not have the data, so it returns invalid
Prefetch clocks in PC and reads SRAM in following cycle

Decode does not have valid instruction, but request has been placed for 'b'
    Fetch pending request
    Prefetch sequential 16
Prefetch combinatorially returns valid data 'b' from pending request, from SRAM path directly for SRAM read
Decode clocks valid instruction in its active register

Decode decodes inst 'b', which is not a taken branch; next pc op is sequential
    Fetch last prefetched
    Prefetch (from next pc op) is sequential
Prefetch logic has 16-bit data in register; it passes that 'a+1', and using next prefetch op of sequential starts SRAM read of next word
BUT - 'b' is not taken by the RF for some reason (pipeline blocked); what happens?



Decode blocked

Decode delayed branch

Decode flush



<?php page_ep();

include "${toplevel}web_assist/web_footer.php"; ?>

