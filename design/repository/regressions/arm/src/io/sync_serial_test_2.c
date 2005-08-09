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
    sync_serial_mdio_read(  IO_A_SLOT_SYNC_SERIAL_0, 1, 0xfedcba98 ); // we read back...

    sync_serial_wait_and_read_response( IO_A_SLOT_SYNC_SERIAL_0, &time, &status, &read_data );
    if ((read_data != 0x12345678) || (status!=(0x10700020|(1<<8)))) return 1;
    sync_serial_wait_and_read_response( IO_A_SLOT_SYNC_SERIAL_0, &time, &status, &read_data );
    if ((read_data != (0xfedc0000|(0x45678>>1))) || (status!=(0x107e0020|(1<<8)))) return 2;
    sync_serial_wait_and_read_response( IO_A_SLOT_SYNC_SERIAL_0, &time, &status, &read_data );

    return 0;
}
