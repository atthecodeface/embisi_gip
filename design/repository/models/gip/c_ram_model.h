/*a Copyright Gavin J Stark and John Croft, 2003
 */

/*a Wrapper
 */
#ifndef C_RAM_MODEL
#define C_RAM_MODEL

/*a Ram model class
 */
class c_ram_model
{
 private:
	struct t_ram_model_data *private_data;
	struct t_ram_page *c_ram_model::find_page( unsigned int address );
	struct t_ram_page *c_ram_model::allocate_page( unsigned int address );

 public:
    class c_memory_model *memory;
	int page_size;
	c_ram_model::c_ram_model( unsigned int size );
	c_ram_model::~c_ram_model( void );
    int c_ram_model::register_with_memory_map( class c_memory_model *memory, unsigned int base_address, unsigned int address_range_size );
    unsigned int c_ram_model::read( unsigned int address );
    void c_ram_model::write( unsigned int address, unsigned int data, int bytes );
};

/*a Wrapper
 */
#endif
