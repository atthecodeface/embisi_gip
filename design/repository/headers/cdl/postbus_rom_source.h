/*a Copyright Gavin J Stark, 2004
 */

/*a Includes
 */
include "postbus.h"

/*a Constants
 */
constant integer postbus_rom_address_log_size=8;
constant integer postbus_rom_delay_size = 20;

constant integer postbus_rom_size_start_bit=0;
constant integer postbus_rom_jump_bit = 8;
constant integer postbus_rom_delay_start_bit = 12;

/*a Types
 */

/*a Modules
 */
extern module postbus_rom_source( clock int_clock,
                              input bit int_reset,

                              output bit rom_read "Synchronous ROM read",
                              output bit[postbus_rom_address_log_size] rom_address "Synchronous ROM address",
                              input bit[32] rom_data "Data from sync ROM, expected the clock after read/address",

                              output t_postbus_type postbus_type,
                              output t_postbus_data postbus_data,
                              input t_postbus_ack postbus_ack
    )
{
    timing to rising clock int_clock int_reset;
    timing comb input int_reset;

    timing from rising clock int_clock rom_read, rom_address;
    timing to rising clock int_clock rom_data;

    timing from rising clock int_clock postbus_type, postbus_data;
    timing to rising clock int_clock postbus_ack;

}
