
/*a Types
 */
/*t t_command_fn
 */
typedef int t_command_fn( int argc, unsigned int *args );

/*t t_command
 */
typedef struct
{
    const char *name;
    t_command_fn *fn;
} t_command;

/*t t_command_chain
 */
typedef struct t_command_chain
{
    struct t_command_chain *next;
    const t_command *cmds;
} t_command_chain;

/*a Functions
 */
extern void chain_extra_cmds( t_command_chain *chain );
