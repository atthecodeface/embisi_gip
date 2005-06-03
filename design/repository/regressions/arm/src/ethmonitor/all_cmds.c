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
static t_command_chain monitor_cmds_postbus_chain;
static t_command_chain monitor_cmds_extbus_chain;
static t_command_chain monitor_cmds_flash_chain;
static t_command_chain monitor_cmds_leds_chain;
static t_command_chain monitor_cmds_test_chain;
extern void chain_extra_cmds( t_command_chain *chain )
{
    chain->next = &monitor_cmds_memory_chain;
    monitor_cmds_memory_chain.next = NULL;
    monitor_cmds_memory_chain.cmds = &monitor_cmds_memory[0];
    chain = &monitor_cmds_memory_chain;

    chain->next = &monitor_cmds_postbus_chain;
    monitor_cmds_postbus_chain.next = NULL;
    monitor_cmds_postbus_chain.cmds = &monitor_cmds_postbus[0];
    chain = &monitor_cmds_postbus_chain;

    chain->next = &monitor_cmds_extbus_chain;
    monitor_cmds_extbus_chain.next = NULL;
    monitor_cmds_extbus_chain.cmds = &monitor_cmds_extbus[0];
    chain = &monitor_cmds_extbus_chain;

    chain->next = &monitor_cmds_flash_chain;
    monitor_cmds_flash_chain.next = NULL;
    monitor_cmds_flash_chain.cmds = &monitor_cmds_flash[0];
    chain = &monitor_cmds_flash_chain;

    chain->next = &monitor_cmds_leds_chain;
    monitor_cmds_leds_chain.next = NULL;
    monitor_cmds_leds_chain.cmds = &monitor_cmds_leds[0];
    chain = &monitor_cmds_leds_chain;

    chain->next = &monitor_cmds_test_chain;
    monitor_cmds_test_chain.next = NULL;
    monitor_cmds_test_chain.cmds = &monitor_cmds_test[0];
    chain = &monitor_cmds_test_chain;

}
extern void extra_init( void )
{
}
