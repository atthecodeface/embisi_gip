/*a Types
 */
/*t t_command_fn
 */
typedef int t_command_fn( void *handle, int argc, unsigned int *args );

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

/*a Functions in our source
 */
extern void cmd_result_string( void *handle, const char *text );
extern void cmd_result_hex8( void *handle, unsigned int v );
extern void cmd_result_hex2( void *handle, unsigned int v );
extern void cmd_result_string_nl( void *handle, const char *string );
extern void cmd_result_nl( void *handle );
extern void cmd_obey( void *handle, char *buffer, int length, int max_result_length );
extern void cmd_init( void );

/*a Functions defined in other source
 */
extern void chain_extra_cmds( t_command_chain *chain );
extern void extra_init( void );
