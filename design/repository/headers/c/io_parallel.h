/*t rf_state_*
 */
enum
{
    rf_state_counter_value_start  = 0,
    rf_state_counter_number_start = 12,
    rf_state_arc0_condition       = 14,
    rf_state_arc0_action          = 17,
    rf_state_arc0_rel_state       = 21,
    rf_state_arc1_condition       = 24,
    rf_state_arc1_action          = 27,
    rf_state_arc1_rel_state       = 31,
};

/*t t_io_parallel_ctl_oe
 */
typedef enum
{
    io_parallel_ctl_oe_both_hiz      = 0,
    io_parallel_ctl_oe_drive_one     = 1,
    io_parallel_ctl_oe_drive_both    = 2,
    io_parallel_ctl_oe_controlled    = 3,
} t_io_parallel_ctl_oe;

/*t t_io_parallel_action
 */
typedef enum
{
    io_parallel_action_idle            = 0,
    io_parallel_action_capture         = 1,
    io_parallel_action_transmit        = 2,
    io_parallel_action_both            = 3,

    io_parallel_action_deccnt          = 4,
    io_parallel_action_deccnt_capture  = 5,
    io_parallel_action_deccnt_transmit = 6,
    io_parallel_action_deccnt_both     = 7,

    io_parallel_action_setcnt          = 8,
    io_parallel_action_setcnt_capture  = 9,
    io_parallel_action_setcnt_transmit = 10,
    io_parallel_action_setcnt_both     = 11,

    io_parallel_action_end             = 12
} t_io_parallel_action;

/*t t_io_parallel_cmd_type
 */
typedef enum
{
    io_parallel_cmd_type_reset  = 0x0,
    io_parallel_cmd_type_rf_0   = 0x1,
    io_parallel_cmd_type_rf_1   = 0x2,
    io_parallel_cmd_type_config = 0x4,
    io_parallel_cmd_type_go     = 0x5,
    io_parallel_cmd_type_enable = 0xf
} t_io_parallel_cmd_type;

/*t t_io_parallel_cfg_data_size
 */
typedef enum
{
    io_parallel_cfg_data_size_1  = 0,
    io_parallel_cfg_data_size_2  = 1,
    io_parallel_cfg_data_size_4  = 2,
    io_parallel_cfg_data_size_8  = 3,
    io_parallel_cfg_data_size_16 = 4,
} t_io_parallel_cfg_data_size;

enum
{
    io_parallel_cfd_holdoff_start_bit                        = 0,
    io_parallel_cfd_auto_restart_fsm_start_bit               = 4,
    io_parallel_cfd_data_size_start_bit                      = 5,
    io_parallel_cfd_data_capture_enabled_start_bit           = 8,
    io_parallel_cfd_use_registered_control_inputs_start_bit  = 9,
    io_parallel_cfd_use_registered_data_inputs_start_bit     = 10,
    io_parallel_cfd_interim_status_start_bit                 = 11,
    io_parallel_cfd_reset_state_start_bit                    = 13,
    io_parallel_cfd_ctl_out_state_override_start_bit         = 17,
    io_parallel_cfd_data_out_enable_start_bit                = 21,
    io_parallel_cfd_data_out_use_ctl3_start_bit              = 22,
    io_parallel_cfd_ctl_oe01_start_bit                       = 23,
    io_parallel_cfd_ctl_oe23_start_bit                       = 25,
};
