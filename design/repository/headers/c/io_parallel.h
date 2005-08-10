/*t t_io_parallel_action
 */
typedef enum
{
    io_parallel_action_idle = 0,
    io_parallel_action_idlecnt_capture = 1,
    io_parallel_action_setcnt_nocapture = 2,
    io_parallel_action_setcnt_capture = 3,
    io_parallel_action_deccnt_nocapture = 4,
    io_parallel_action_deccnt_capture = 5,
    io_parallel_action_spare = 6,
    io_parallel_action_end = 7
} t_io_parallel_action;

/*t t_io_parallel_cmd_type
 */
typedef enum
{
    io_parallel_cmd_type_reset = 0x00,
    io_parallel_cmd_type_rf_0 = 0x10,
    io_parallel_cmd_type_rf_1 = 0x20,
    io_parallel_cmd_type_config = 0x30,
    io_parallel_cmd_type_go = 0x40,
    io_parallel_cmd_type_enable = 0x65
} t_io_parallel_cmd_type;

/*t t_io_parallel_cfg_capture_size
 */
typedef enum
{
    io_parallel_cfg_capture_size_1  = 0,
    io_parallel_cfg_capture_size_2  = 1,
    io_parallel_cfg_capture_size_4  = 2,
    io_parallel_cfg_capture_size_8  = 3,
    io_parallel_cfg_capture_size_16 = 4,
} t_io_parallel_cfg_capture_size;

enum
{
    io_parallel_cfd_data_holdoff_start_bit      = 0,
    io_parallel_cfd_status_holdoff_start_bit    = 4,
    io_parallel_cfd_auto_restart_fsm_start_bit  = 8,
    io_parallel_cfd_capture_size_start_bit      = 9,
    io_parallel_cfd_data_capture_enabled_start_bit = 12,
    io_parallel_cfd_use_registered_control_inputs_start_bit = 13,
    io_parallel_cfd_use_registered_data_inputs_start_bit = 14
};
