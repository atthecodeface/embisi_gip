/*a Includes
 */
#include <stdlib.h> // for NULL
#include "gip_support.h"
#include "gip_system.h"
#include "postbus.h"
#include "postbus_config.h"
#include "../common/wrapper.h"
#include "sync_serial.h"

/*a Defines
 */

/*a Types
 */

/*a Static variables
 */

/*a External functions
 */
extern int test_entry_point( void )
{
    int capture_time;
    unsigned char buffer[256];
    int frame_size;
    int failure;

    postbus_config( 16, 16, 0, 0 ); // use 16 for tx and 16 for rx, no sharing
    GIP_CLEAR_SEMAPHORES( -1 );

    parallel_frame_capture_init( IO_A_SLOT_PARALLEL_0, 8, 12, 3, 16 ); // capture frames of 8 lines, 6 pixels front porch, three gaps between each pixel, and 16 pixels per line
    parallel_frame_capture_start( IO_A_SLOT_PARALLEL_0, 0x01000000 );
    frame_size = parallel_frame_capture_buffer( IO_A_SLOT_PARALLEL_0, (unsigned int *)buffer, &capture_time );

    failure = 1;
    do
    {
        int i;
        if (frame_size!=32) break; // 8*16 pixels = 128 pixels = 32 bytes at 2bpp
        failure++;
        for (i=0; i<32; i++)
        {
            if (buffer[i]!=0xaa) break; // check all pixels match the expected value
            failure++;
        }
        if (i<32) break;
        failure = 0;
    } while (0);
    return failure;
}
