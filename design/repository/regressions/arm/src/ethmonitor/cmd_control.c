/*a Includes
 */
#include "cmd.h"
#include "gip_support.h"

/*a Defines
 */

/*a Types
 */

/*a Static functions
 */
/*f command_control_pc
  .. <address>
 */
static int command_control_pc( void *handle, int argc, unsigned int *args )
{
    __asm__ volatile ("mov lr, pc ; mov pc, %0 ; mov r0, r0" : : "r"(args[0]) );
    return 1;
}

/*f command_control_regs
  .. <address>
 */
static int command_control_regs( void *handle, int argc, unsigned int *args )
{
    __asm__ volatile ("mov r0, %0 ; ldmia r0, {r0-pc}" : : "r"(args[0]) );
    return 1;
}

/*a External variables
 */
/*v monitor_control_chain
 */
extern const t_command monitor_cmds_control[];
const t_command monitor_cmds_control[] =
{
    {"cpc",   command_control_pc},
    {"cregs", command_control_regs},
};

