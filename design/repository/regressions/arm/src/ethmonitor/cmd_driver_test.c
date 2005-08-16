/*a Includes
 */
#define GIP_INCLUDE_FROM_C
#include "gip_support.h"
#include "gip_system.h"
#include "cmd.h"
#include "postbus_config.h"
#include "../drivers/gipeth.h"
#include "../drivers/gip_video_capture.h"
#include "../drivers/uart.h"
#include <stdlib.h> // for NULL

/*a Defines
 */

/*a Types
 */

/*a Static functions
 */
/*f command_driver_init
 */
static int command_driver_init( void *handle, int argc, unsigned int *args )
{
    postbus_config( 16, 16, 0, 0 );
    return 0;
}

/*f command_gipeth_driver
 */
static int command_gipeth_driver( void *handle, int argc, unsigned int *args )
{
    uart_tx_string_nl("A");
    gipeth_setup();
    uart_tx_string_nl("B");
    return 0;
}

/*f command_gipeth_tx
 */
static int command_gipeth_tx( void *handle, int argc, unsigned int *args )
{
    if (argc<1)
        return 1;
    return gipeth_xmit( (unsigned int *)"This is the string we want to send", args[0] );
}

/*f command_gip_video_capture_driver
 */
static int command_gip_video_capture_driver( void *handle, int argc, unsigned int *args )
{
    uart_tx_string_nl("A");
    gip_video_capture_hw_thread_init( IO_A_SLOT_PARALLEL_0 );
    uart_tx_string_nl("B");
    return 0;
}

/*f command_gip_video_capture_configure
  <lines> <pixels_per_line> [<line skip>] [<init_gap=2>] [<pixel_gap=1>]
For 800x600 we need to capture 800 pixels at every 4th pixel, which means 200 pixels per line, gap 3...
vfccfg x c8 x x 3
Each line is 200 pixels at 2bpp, i.e. 4 pixels per byte, so 50 bytes

our svga input has clock of 40MHz = 25ns/ck
hsync period of 26317ns (38kHz) = 1252 clocks
hsync high of 800ns = 32 clocks
vsync period of 16540us
vsync low of 109.2us or 4,368 clocks
first pixel at 4700ns after hsync low = 188 clocks
first line after 23 hsyncs

So, first pixel is 188+31 after hsync goes high, or 219 clocks
vfccfg 10 c8 x bb 3
vfccfg 10 c8 17 bb 3

To capture an overview of the top 48 lines...
vfccfg 30 40 x bb b
vfccfg 30 40 17 bb b
 */
static int command_gip_video_capture_configure( void *handle, int argc, unsigned int *args )
{
    unsigned int line_skip, nlines, init_gap, pixel_gap, pixels_per_line;

    if (argc<2) return 1;
    nlines = args[0];
    pixels_per_line = args[1];
    line_skip = 0;
    init_gap = 2;
    pixel_gap = 1;
    if (argc>2) line_skip=args[2];
    if (argc>3) init_gap=args[3];
    if (argc>4) pixel_gap=args[4];

    uart_tx_string_nl("A");
    gip_video_capture_configure( line_skip, nlines, init_gap, pixel_gap, pixels_per_line );
    uart_tx_string_nl("B");

    return 0;
}

/*f command_gip_video_capture_start
vfcst <buffer> [<buffer length=64k>]
 */
static int command_gip_video_capture_start( void *handle, int argc, unsigned int *args )
{
    unsigned char *buffer;
    unsigned int buffer_length;

    if (argc<1) return 1;
    buffer = (unsigned char *)args[0];
    buffer_length=65536;
    if (argc>1) buffer_length=args[1];
    
    uart_tx_string_nl("A");
    gip_video_capture_start( buffer, buffer_length );
    uart_tx_string_nl("B");
    return 0;
}

/*f command_gip_video_capture_poll
 */
static int command_gip_video_capture_poll( void *handle, int argc, unsigned int *args )
{
 
    cmd_result_string( handle, "Poll returns " );
    cmd_result_hex8( handle, gip_video_capture_poll() );
    cmd_result_nl( handle );
    return 0;
}

/*a External variables
 */
/*v monitor_cmds_driver_test
 */
extern const t_command monitor_cmds_driver_test[];
const t_command monitor_cmds_driver_test[] =
{
    {"drvinit", command_driver_init},
    {"gedrv", command_gipeth_driver},
    {"getx", command_gipeth_tx},
    {"vfcdrv", command_gip_video_capture_driver },
    {"vfccfg", command_gip_video_capture_configure },
    {"vfcst", command_gip_video_capture_start },
    {"vfcpl", command_gip_video_capture_poll },
    {(const char *)0, (t_command_fn *)0},
};

