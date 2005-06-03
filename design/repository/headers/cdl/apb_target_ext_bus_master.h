/*m apb_target_ext_bus_master
 */
extern module apb_target_ext_bus_master( clock apb_clock "Internal system clock",
                                  input bit int_reset "Internal reset",

                                  input bit[3] apb_paddr,
                                  input bit apb_penable,
                                  input bit apb_pselect,
                                  input bit[32] apb_pwdata,
                                  input bit apb_prnw,
                                  output bit[32] apb_prdata,
                                  output bit apb_pwait,

                                  output bit[4]ext_bus_ce,
                                  output bit ext_bus_oe,
                                  output bit ext_bus_we,
                                  output bit[24]ext_bus_address,
                                  output bit ext_bus_write_data_enable,
                                  output bit[32]ext_bus_write_data,
                                  input bit[32]ext_bus_read_data
                                  )
{
    timing to rising clock apb_clock int_reset;

    timing to rising clock apb_clock apb_pselect, apb_penable, apb_paddr, apb_pwdata, apb_prnw;
    timing comb input apb_pselect, apb_penable, apb_paddr, apb_prnw;
    timing from rising clock apb_clock apb_prdata;
    timing comb output apb_prdata;

    timing to rising clock apb_clock ext_bus_read_data;
    timing from rising clock apb_clock ext_bus_ce, ext_bus_oe, ext_bus_we, ext_bus_address, ext_bus_write_data_enable, ext_bus_write_data;
}

