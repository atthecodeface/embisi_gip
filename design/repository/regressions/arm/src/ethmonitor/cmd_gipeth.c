/*a Includes
 */
#include "gip_support.h"
#include "cmd.h"
#include <stdlib.h> // for NULL

/*a Defines
 */

/*a Types
 */

/*a Static functions
 */

extern void gipeth_setup(void);
extern int gipeth_xmit( unsigned int *data, int length );
/*f command_gipeth_eth
 */
static int command_gipeth_driver( void *handle, int argc, unsigned int *args )
{
    unsigned int s;
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
    return gipeth_xmit( (unsigned int)"This is the string we want to send", args[0] );
}

/*a External variables
 */
/*v monitor_cmds_gipeth
 */
extern const t_command monitor_cmds_gipeth[];
const t_command monitor_cmds_gipeth[] =
{
    {"gedrv", command_gipeth_driver},
    {"getx", command_gipeth_tx},
    {(const char *)0, (t_command_fn *)0},
};

