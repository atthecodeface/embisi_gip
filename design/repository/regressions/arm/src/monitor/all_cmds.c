#include <stdlib.h>
#include "monitor.h"
#include "memory.h"

#include <stdio.h>
static t_command_chain monitor_memory_chain;
extern void chain_extra_cmds( t_command_chain *chain )
{
    chain->next = &monitor_memory_chain;
    monitor_memory_chain.next = NULL;
    monitor_memory_chain.cmds = &monitor_memory_cmds[0];
}
