#include <stdlib.h> // for NULL
#include "cmd.h"
#include "cmd_memory.h"
#include "cmd_postbus.h"
#include "cmd_extbus.h"
#include "cmd_flash.h"
#include "cmd_leds.h"
#include "cmd_test.h"

#include <stdio.h>
static t_command_chain monitor_cmds_memory_chain;
extern void chain_extra_cmds( t_command_chain *chain )
{
    chain->next = &monitor_cmds_memory_chain;
    monitor_cmds_memory_chain.next = NULL;
    monitor_cmds_memory_chain.cmds = &monitor_cmds_memory[0];
    chain = &monitor_cmds_memory_chain;

}
extern void extra_init( void )
{
}
