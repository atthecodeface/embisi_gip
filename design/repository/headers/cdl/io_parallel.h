/*a Constants
 */
constant integer io_parallel_type_bits = 8;
constant integer io_parallel_rf_0_bits = 24;
constant integer io_parallel_rf_1_bits = 20;

constant integer io_parallel_cfg_data_holdoff_bits = 4; // system related
constant integer io_parallel_cfg_status_holdoff_bits = 4; // system related
constant integer io_parallel_cfg_auto_restart_fsm_bits = 1; // both related
constant integer io_parallel_cfg_capture_size_bits = 3; // both related?
constant integer io_parallel_cfg_data_capture_enabled_bits = 1; // capture related
constant integer io_parallel_cfg_use_registered_control_inputs_bits = 1; // capture related
constant integer io_parallel_cfg_use_registered_data_inputs_bits = 1; // capture related
constant integer io_parallel_cfg_interim_status_bits = 2; // both related

constant integer io_parallel_cfd_type_start_bit = 24;

constant integer io_parallel_data_start_bit = 0;

constant integer io_parallel_cfd_data_holdoff_start_bit   = 0;
constant integer io_parallel_cfd_status_holdoff_start_bit = io_parallel_cfd_data_holdoff_start_bit + io_parallel_cfg_data_holdoff_bits; // 4

constant integer io_parallel_cfd_auto_restart_fsm_start_bit              = io_parallel_cfd_status_holdoff_start_bit + io_parallel_cfg_status_holdoff_bits;
constant integer io_parallel_cfd_capture_size_start_bit                  = io_parallel_cfd_auto_restart_fsm_start_bit + io_parallel_cfg_auto_restart_fsm_bits;
constant integer io_parallel_cfd_data_capture_enabled_start_bit          = io_parallel_cfd_capture_size_start_bit + io_parallel_cfg_capture_size_bits;
constant integer io_parallel_cfd_use_registered_control_inputs_start_bit = io_parallel_cfd_data_capture_enabled_start_bit + io_parallel_cfg_data_capture_enabled_bits;
constant integer io_parallel_cfd_use_registered_data_inputs_start_bit    = io_parallel_cfd_use_registered_control_inputs_start_bit + io_parallel_cfg_use_registered_control_inputs_bits;
constant integer io_parallel_cfd_interim_status_start_bit                = io_parallel_cfd_use_registered_data_inputs_start_bit + io_parallel_cfg_use_registered_data_inputs_bits;

/*a Types
 */

/*a Modules
 */
extern module io_parallel( clock par_clock,
                           input bit par_reset,

                           input bit[32] tx_data_fifo_data,
                           output t_io_tx_data_fifo_cmd tx_data_fifo_cmd,
                           output bit tx_data_fifo_toggle,

                           output bit[32] rx_data_fifo_data,
                           output bit rx_data_fifo_toggle,

                           input bit cmd_fifo_empty,
                           input bit[32] cmd_fifo_data,
                           output bit cmd_fifo_toggle,

                           output bit status_fifo_toggle,
                           output bit[32] status_fifo_data,

                           input bit[3] control_inputs,
                           input bit[16] data_inputs,

                           output bit[4] control_outputs,
                           output bit[4] control_oes,
                           output bit[16] data_outputs,
                           output bit[3] data_output_width,
                           output bit data_oe
    )

{
    timing to rising clock par_clock par_reset;

    timing to rising clock par_clock tx_data_fifo_data;
    timing from rising clock par_clock tx_data_fifo_cmd, tx_data_fifo_toggle;

    timing to rising clock par_clock cmd_fifo_empty, cmd_fifo_data;
    timing from rising clock par_clock cmd_fifo_toggle;

    timing from rising clock par_clock status_fifo_toggle, status_fifo_data;
    timing from rising clock par_clock rx_data_fifo_data, rx_data_fifo_toggle;

    timing to rising clock par_clock control_inputs, data_inputs;
    timing from rising clock par_clock control_outputs, control_oes, data_outputs, data_output_width, data_oe;

    timing comb input par_reset;
}
