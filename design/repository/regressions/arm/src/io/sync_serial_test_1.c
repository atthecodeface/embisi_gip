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
    unsigned int time, status, read_data;
    unsigned int last_time;
    int second_time, third_time, fourth_time;

    postbus_config( 16, 16, 0, 0 ); // use 16 for tx and 16 for rx, no sharing
    GIP_CLEAR_SEMAPHORES( -1 );

    sync_serial_init( IO_A_SLOT_SYNC_SERIAL_0 );
    sync_serial_mdio_write( IO_A_SLOT_SYNC_SERIAL_0, 1, 0x12345678 ); 
    sync_serial_mdio_write( IO_A_SLOT_SYNC_SERIAL_0, 2, 0xfedcba98 ); //0x58920040 is auto-neg to 10-FD
    sync_serial_mdio_write( IO_A_SLOT_SYNC_SERIAL_0, 3, 0xbeeff00d ); //    GIP_POST_TXD_0( 0x58821340 ); is restart auto-neg
    sync_serial_mdio_write( IO_A_SLOT_SYNC_SERIAL_0, 4, 0xd10deade ); 

    sync_serial_wait_and_read_response( IO_A_SLOT_SYNC_SERIAL_0, &time, &status, &read_data );
    if ((read_data != 0x12345678) || (status!=(0x10700020|(1<<8)))) return 1;
    last_time = time;
    sync_serial_wait_and_read_response( IO_A_SLOT_SYNC_SERIAL_0, &time, &status, &read_data );
    if ((read_data != 0xfedcba98) || (status!=(0x10700020|(2<<8)))) return 2;
    second_time = time - last_time;
    last_time = time;
    sync_serial_wait_and_read_response( IO_A_SLOT_SYNC_SERIAL_0, &time, &status, &read_data );
    if ((read_data != 0xbeeff00d) || (status!=(0x10700020|(3<<8)))) return 3;
    third_time = time - last_time;
    last_time = time;
    sync_serial_wait_and_read_response( IO_A_SLOT_SYNC_SERIAL_0, &time, &status, &read_data );
    if ((read_data != 0xd10deade) || (status!=(0x10700020|(4<<8)))) return 4;
    fourth_time = time - last_time;
    last_time = time;

    // Now, second_time was a divide of (2+1), or 3, so it should have taken about 3n. Third time about 4n. Fourth time around 5n.
    // so second_time * 20 is about 60n, and so on
    // it turns out that 'n' is about 70 - 35 clocks? possibly 34 plus 
    // so we expect roughly 210 (actually 215), 280 and 350.
    // if we check each for n+-2, we need to look for +-4*60
    dprintf("Times ", second_time, third_time, fourth_time );
    second_time *= 20;
    third_time *= 15;
    fourth_time *= 12;
    third_time -= second_time;
    if (third_time<0) third_time=-third_time;
    if (third_time>240) return 5;
    fourth_time -= second_time;
    if (fourth_time<0) fourth_time=-fourth_time;
    if (fourth_time>240) return 6;
    return 0;
}
