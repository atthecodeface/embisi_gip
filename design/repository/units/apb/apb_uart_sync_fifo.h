/*a Copyright Gavin J Stark, 2004
 */

/*a Module
 */
extern module apb_uart_sync_fifo( clock int_clock "Internal system clock",
                                  input bit int_reset "Internal reset",

                                  input bit write,
                                  input bit read,
                                  output bit empty_flag,
                                  output bit full_flag,
                                  output bit[32] read_data,
                                  input bit[32] write_data )
{
    timing to rising clock int_clock int_reset, read, write, write_data;
    timing from rising clock int_clock empty_flag, full_flag, read_data;
}
