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
/*m memory_s_sp_2048_x_32
 */
extern module memory_s_sp_2048_x_32( clock sram_clock "SRAM clock",
                                     input bit sram_read "SRAM read",
                                     input bit sram_write "SRAM write",
                                     input bit[11] sram_address "SRAM address",
                                     input bit[32] sram_write_data "SRAM write data",
                                     output bit[32] sram_read_data "SRAM read data" )
{
    timing to rising clock sram_clock sram_read, sram_write, sram_address, sram_write_data;
    timing from rising clock sram_clock sram_read_data;
}

/*m memory_s_sp_2048_x_4b8
 */
extern module memory_s_sp_2048_x_4b8( clock sram_clock "SRAM clock",
                                      input bit sram_read_not_write "SRAM read not write",
                                      input bit[4] sram_write_enable "SRAM write byte enables; only used if sram_read_not_write is low",
                                      input bit[11] sram_address "SRAM address",
                                      input bit[32] sram_write_data "SRAM write data",
                                      output bit[32] sram_read_data "SRAM read data" )
{
    timing to rising clock sram_clock sram_read_not_write, sram_write_enable, sram_address, sram_write_data;
    timing from rising clock sram_clock sram_read_data;
}

/*m memory_s_dp_2048_x_32
 */
extern module memory_s_dp_2048_x_32( clock sram_clock "SRAM clock",
                                     input bit sram_read "SRAM read",
                                     input bit[11] sram_read_address "SRAM address",
                                     input bit sram_write "SRAM write",
                                     input bit[11] sram_write_address "SRAM address",
                                     input bit[32] sram_write_data "SRAM write data",
                                     output bit[32] sram_read_data "SRAM read data" )
{
    timing to rising clock sram_clock sram_read, sram_read_address, sram_write, sram_write_address, sram_write_data;
    timing from rising clock sram_clock sram_read_data;
}

