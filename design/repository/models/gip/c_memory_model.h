/*a Copyright Gavin J Stark and John Croft, 2003
 */

/*a Wrapper
 */
#ifndef C_MEMORY_MODEL
#define C_MEMORY_MODEL

/*a Types
 */
/*t t_memory_model_debug_action
 */
typedef enum
{
     memory_model_debug_action_unmapped_memory,
     memory_model_debug_action_watchpoint_location_written,
     memory_model_debug_action_read_of_undefined_memory,
} t_memory_model_debug_action;

/*t t_memory_model_debug_fn
  Called when a memory trap occurs
 */
typedef void t_memory_model_debug_fn( void *handle, t_memory_model_debug_action action, int write_not_read, unsigned int address, unsigned int data, int bytes );

/*t t_memory_model_write_fn
  Called when a write to a suitably mapped area of memory occurs
 */
typedef void t_memory_model_write_fn( void *handle, unsigned int address, unsigned int data, int bytes );

/*t t_memory_model_read_fn
  Called when a write to a suitably mapped area of memory occurs
 */
typedef unsigned int t_memory_model_read_fn( void *handle, unsigned int address );

/*a Memory model class
 */
class c_memory_model
{
 private:
	struct t_memory_model_data *private_data;
	unsigned int *c_memory_model::find_page( unsigned int address );
	unsigned int *c_memory_model::allocate_page( unsigned int address );

 public:
	int page_size;
	c_memory_model::c_memory_model( void );
	c_memory_model::~c_memory_model( void );
    int c_memory_model::set_log_file( const char *filename );
    void c_memory_model::set_log_level( int log_level );
    void c_memory_model::set_log_level( unsigned int base_address, unsigned int size, int log_level );
    void c_memory_model::log( const char *string, unsigned int address, unsigned int data, int bytes );
    int c_memory_model::map_memory( void *handle, unsigned int base_address, unsigned int address_range_size, t_memory_model_write_fn write_fn, t_memory_model_read_fn read_fn );
    int c_memory_model::register_debug_handler( void *handle, t_memory_model_debug_fn debug_fn );
	unsigned int c_memory_model::read_memory( unsigned int address );
	void c_memory_model::write_memory( unsigned int address, unsigned int data, int bytes );
    void c_memory_model::raise_memory_exception( t_memory_model_debug_action action, int write_not_read, unsigned int address, unsigned int data, int bytes );
};

/*a Wrapper
 */
#endif
