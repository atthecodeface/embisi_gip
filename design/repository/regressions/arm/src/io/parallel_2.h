/*a Types
 */
/*t t_par_action
 */
typedef struct t_par_action
{
    char condition;
    char next_state;
    t_io_parallel_action action;
} t_par_action;

/*t t_par_state
 */
typedef struct t_par_state
{
    int state;
    int *counter_value;
    int counter_number;
    t_par_action arc0;
    t_par_action arc1;
} t_par_state;

/*a Functions
 */
extern void parallel_config( unsigned int *action_array, int *action_number, t_io_parallel_cmd_type type, int data );
extern void parallel_config_fsm( unsigned int *action_array, int *action_number, int nstates, const t_par_state *states, const char *conds, const char *ctl_out, int do_all );
extern void parallel_init_interface( int slot, unsigned int *action_array, int action_number );
extern void parallel_wait_for_status( int slot );
extern void parallel_get_status( int slot, unsigned int *time, unsigned int *status );
