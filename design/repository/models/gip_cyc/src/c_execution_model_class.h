/*a Copyright Gavin J Stark and John Croft, 2003
 */

/*a Wrapper
 */
#ifndef C_EXECUTION_MODEL_CLASS
#define C_EXECUTION_MODEL_CLASS

/*a Includes
 */

/*a Defines
 */
#define MAX_TRACING (8)

/*a Types
 */
/*t gip_flag_mask_*
 */
enum
{
    gip_flag_mask_z = 1,
    gip_flag_mask_n = 2,
    gip_flag_mask_c = 4,
    gip_flag_mask_v = 8,
    gip_flag_mask_cp= 16,
};

/*t t_log_data
 */
typedef struct t_log_data
{
    unsigned int address;
    unsigned int opcode;
    int conditional;
    int condition_passed;
    int sequential;
    int branch;
    int rfr;
    int rfw;
    int sign;
} t_log_data;

/*t	c_execution_model_class
*/
class c_execution_model_class
{
public:

    /*b Constructors/destructors methods
     */
    c_execution_model_class::c_execution_model_class( class c_memory_model *memory_model );
    virtual c_execution_model_class::~c_execution_model_class() = 0;

    /*b Execution methods
     */
    virtual int c_execution_model_class::step( int *reason, int requested_count ) = 0;
    virtual void c_execution_model_class::reset( void) = 0;
    virtual void c_execution_model_class::preclock( void) = 0;
    virtual void c_execution_model_class::clock( void) = 0;
    virtual void c_execution_model_class::comb( void *data ) = 0;

    /*b Code loading methods
     */
    virtual void c_execution_model_class::load_code( FILE *f, unsigned int base_address ) = 0;
    virtual void c_execution_model_class::load_code_binary( FILE *f, unsigned int base_address ) = 0;
    virtual void c_execution_model_class::load_symbol_table( char *filename ) = 0;

    /*b Debug methods
     */
    virtual void c_execution_model_class::set_register( int r, unsigned int value ) = 0;
    virtual unsigned int c_execution_model_class::get_register( int r ) = 0;
    virtual void c_execution_model_class::set_flags( int value, int mask ) = 0;
    virtual int c_execution_model_class::get_flags( void ) = 0;
    virtual int c_execution_model_class::set_breakpoint( unsigned int address ) = 0;
    virtual int c_execution_model_class::unset_breakpoint( unsigned int address ) = 0;
    virtual void c_execution_model_class::halt_cpu( void ) = 0;
    virtual void c_execution_model_class::debug( int mask ) = 0;

    /*a Trace methods
     */
    void c_execution_model_class::trace_output( char *format, ... );
    int c_execution_model_class::trace_set_file( char *filename );
    int c_execution_model_class::trace_region( int region, unsigned int start_address, unsigned int end_address );
    int c_execution_model_class::trace_region_stop( int region );
    int c_execution_model_class::trace_all_stop( void );
    int c_execution_model_class::trace_restart( void );

    /*b Log methods
     */
    void c_execution_model_class::log( char *reason, unsigned int arg );
    void c_execution_model_class::log_display( FILE *f );
    void c_execution_model_class::log_reset( void );

    struct t_private_data *pd;
private:
    int tracing_enabled; // If 0, no tracing, else tracing is performed
    FILE *trace_file;
    unsigned int trace_region_starts[ MAX_TRACING ]; // inclusive start address of area to trace - up to MAX_TRACING areas
    unsigned int trace_region_ends[ MAX_TRACING ]; // exclusive end address of area to trace - up to MAX_TRACING areas
    t_log_data log_data;
    /*b Private data
     */
};

/*a External functions
 */

/*a Wrapper
 */
#endif
