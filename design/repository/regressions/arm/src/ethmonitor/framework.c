/*a Includes
 */
#include <stdlib.h> // for NULL
#include "gip_support.h"
#include "../common/wrapper.h"
#include "ethernet.h"
#include "uart.h"
#include "memory.h"

/*a Defines
 */

/*a Test entry point
 */
/*f test_entry_point
 */
extern int test_entry_point()
{
    int i;

    mon_ethernet_init();
    mon_uart_init();
    while (1)
    {
        mon_ethernet_poll();
        mon_uart_poll();
    }

}
