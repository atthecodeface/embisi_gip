/*a Copyright Gavin J Stark and John Croft, 2003
 */

/*a Wrapper
 */
#ifndef C_MMIO_MODEL
#define C_MMIO_MODEL

/*a Mmio model class
 */
class c_mmio_model
{
 private:
	struct t_mmio_model_data *private_data;

 public:
	int page_size;
    c_mmio_model::c_mmio_model( class c_execution_model_class *model );
	c_mmio_model::~c_mmio_model( void );
    int c_mmio_model::register_with_memory_map( class c_memory_model *memory, unsigned int base_address, unsigned int address_range_size );
    unsigned int c_mmio_model::read( unsigned int address );
    void c_mmio_model::write( unsigned int address, unsigned int data, int bytes );

//    unsigned int c_mmio_model::get_timer_isr ();
//    void c_mmio_model::set_return_address (unsigned int addr);
};

/*a Wrapper
 */
#endif
