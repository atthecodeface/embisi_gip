/*a Includes
 */
#include <stdlib.h> // for NULL
#include "gip_support.h"
#include "../common/wrapper.h"
#include "ethernet.h"
#include "uart.h"
#include "memory.h"
#include "cmd.h"

/*a Defines
 */

/*a Test entry point
 */
typedef struct t_config
{
    unsigned int eth_hi;
    unsigned int eth_lo;
    unsigned int ip_address;
} t_config;

/*f test_entry_point
 */
extern int test_entry_point()
{
    int i;
    t_config config = {0x1234, 0x56789abc, 0x0a016405 };
    {
        unsigned int s;
        GIP_GPIO_INPUT_STATUS(s);
        if (s&1)
        {
            mon_flash_boot( 0, 1, &config, sizeof(config) );
        }
    }
    mon_ethernet_init( config.eth_hi, config.eth_lo, config.ip_address );
    mon_uart_init();
    extra_init();
    cmd_init(mon_ethernet_cmd_done); // after extra_init()
    while (1)
    {
        mon_ethernet_poll();
        mon_uart_poll();
    }

}
