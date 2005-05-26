/*a Includes
 */
#include "cmd.h"
#include "gip_support.h"

/*a Defines
 */

/*f Command functions
 */
/*f command_ext_bus_config
 */
static int command_ext_bus_config( void *handle, int argc, unsigned int *args )
{
    unsigned int v;

    if (argc<1)
    {
        FLASH_CONFIG_READ(v);
        cmd_result_hex8( handle, v );
        cmd_result_nl( handle );
    }
    else
    {
        v = args[0];
        FLASH_CONFIG_WRITE(v);
    }
    return 0;
}

/*f command_ext_bus_address
 */
static int command_ext_bus_address( void *handle, int argc, unsigned int *args )
{
    unsigned int v;

    if (argc<1)
    {
        FLASH_ADDRESS_READ(v);
        cmd_result_hex8( handle, v );
        cmd_result_nl( handle );
    }
    else
    {
        v = args[0];
        FLASH_ADDRESS_WRITE(v);
    }
    return 0;
}

/*f command_ext_bus_data
 */
static int command_ext_bus_data( void *handle, int argc, unsigned int *args )
{
    unsigned int v;

    if (argc<1)
    {
        FLASH_DATA_READ(v);
        cmd_result_hex8( handle, v );
        cmd_result_nl( handle );
    }
    else
    {
        v = args[0];
        FLASH_DATA_WRITE(v);
    }
    return 0;
}

/*a External variables
 */
/*v monitor_cmds_extbus
 */
extern const t_command monitor_cmds_extbus[];
const t_command monitor_cmds_extbus[] =
{
    {"ebc", command_ext_bus_config},
    {"eba", command_ext_bus_address},
    {"ebd", command_ext_bus_data},
    {(const char *)0, (t_command_fn *)0},
};

