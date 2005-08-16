/*a Includes
 */
#include "gip_support.h"
#include "gip_system.h"
#include "cmd.h"
#include <stdlib.h> // for NULL
#include <stdlib.h> // for NULL
#include "../drivers/uart.h"
#include "../drivers/parallel.h"

/*a Defines
 */

/*a Types
 */

/*a Static functions
 */
/*f command_video_capture
  <buffer> <lines> <pixels_per_line> [<line skip>] [<init_gap=2>] [<pixel_gap=1>]
vdcp 80280000 10 10
vdcp 80280000 10 100 0 40 3
For 800x600 we need to capture 800 pixels at every 4th pixel, which means 200 pixels per line, gap 3...
vdcp 80280000 x c8 x x 3
Each line is 200 pixels at 2bpp, i.e. 4 pixels per byte, so 50 bytes
In a 4000 byte buffer, we can capture 80 lines? Seems to be more than we can handle.
vdcp 80280000 50 c8 x 0 3
vdcp 80280000 10 c8 x 0 3

our svga input has clock of 40MHz = 25ns/ck
hsync period of 26317ns (38kHz) = 1252 clocks
hsync high of 800ns = 32 clocks
vsync period of 16540us
vsync low of 109.2us or 4,368 clocks
first pixel at 4700ns after hsync low = 188 clocks

So, first pixel is 188+31 after hsync goes high, or 219 clocks
vdcp 80280000 10 c8 x bb 3
vdcp 80280000 10 c8 40 bb 3

vdcp 80280000 10 c8 40 bb 3
vdcp 80280000 10 c8 40 bb 3
vdcp 80280000 10 10 40 bb 3
vdcp 80280000 10 10 40 bb 3
vdcp 80280000 10 c0 40 bb 3

 */
static int command_video_capture( void *handle, int argc, unsigned int *args )
{
    unsigned int line_skip, nlines, init_gap, pixel_gap, pixels_per_line;
    unsigned int capture_time, frame_size;
    unsigned char *buffer;

    if (argc<3) return 1;
    buffer = (unsigned char *)args[0];
    nlines = args[1];
    pixels_per_line = args[2];
    line_skip = 0;
    init_gap = 2;
    pixel_gap = 1;
    if (argc>3) line_skip=args[3];
    if (argc>4) init_gap=args[4];
    if (argc>5) pixel_gap=args[5];

    uart_tx_string_nl("A");
    parallel_frame_capture_init( IO_A_SLOT_PARALLEL_0, line_skip, nlines, init_gap, pixel_gap, pixels_per_line ); // capture frames of 8 lines, 6 pixels front porch, three gaps between each pixel, and 16 pixels per line
    uart_tx_string_nl("B");
    parallel_frame_capture_start( IO_A_SLOT_PARALLEL_0, 0x01000000 );
    uart_tx_string_nl("C");
    frame_size = parallel_frame_capture_buffer( IO_A_SLOT_PARALLEL_0, (unsigned int *)buffer, &capture_time );
    uart_tx_string_nl("D");
 
    cmd_result_string( handle, "Frame captured time " );
    cmd_result_hex8( handle, capture_time );
    cmd_result_string( handle, " data size " );
    cmd_result_hex8( handle, frame_size );
    cmd_result_string( handle, " " );
    return 0;
}

/*a External variables
 */
/*v monitor_cmds_video
 */
extern const t_command monitor_cmds_video[];
const t_command monitor_cmds_video[] =
{
    {"vdcp", command_video_capture},
    {(const char *)0, (t_command_fn *)0},
};

