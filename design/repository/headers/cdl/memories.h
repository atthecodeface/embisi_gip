/*a Copyright Gavin J Stark, 2004
 */

/*a To do
 */

/*a Constants
 */

/*a Types
 */

/*a Modules
 */
extern module memory_s_sp_2048_x_32( clock sram_clock "SRAM clock",
                                     input bit sram_read "SRAM read",
                                     input bit sram_write "SRAM write",
                                     input bit[9] sram_address "SRAM address",
                                     input bit[32] sram_write_data "SRAM write data",
                                     output bit[32] sram_read_data "SRAM read data" )
{
    timing to rising clock sram_clock sram_read, sram_write, sram_address, sram_write_data;
    timing from rising clock sram_clock sram_read_data;
}

