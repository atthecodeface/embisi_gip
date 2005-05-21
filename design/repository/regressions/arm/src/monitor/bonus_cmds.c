#include <stdlib.h>
#include "monitor.h"
#include "memory.h"
#include "ethernet.h"

#include <stdio.h>
static t_command_chain monitor_memory_chain;
static t_command_chain monitor_ethernet_chain;
extern void chain_extra_cmds( t_command_chain *chain )
{

    chain->next = &monitor_ethernet_chain;
    monitor_ethernet_chain.next = NULL;
    monitor_ethernet_chain.cmds = &monitor_ethernet_cmds[0];

/*
    chain->next = &monitor_memory_chain;
    monitor_memory_chain.next = NULL;
    monitor_memory_chain.cmds = &monitor_memory_cmds[0];
    chain = &monitor_memory_cmds;
*/
}
extern void extra_init( void )
{
    ethernet_init();
}
