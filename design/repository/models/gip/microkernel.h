/*a Copyright Gavin J Stark and John Croft, 2003
 */

/*a Wrapper
 */
#ifndef MICROKERNEL_CLASS
#define MICROKERNEL_CLASS

/*a Class
 */
class microkernel
{
 public:
	microkernel::microkernel(class c_gip_pipeline_single * arm);
	void microkernel::handle_swi (int n);
	struct microkernel_data * private_data;
	void microkernel::handle_support (void);
	void microkernel::handle_syscall (int n);
	void microkernel::handle_syscall_ret (void);
	void microkernel::push (int n);

	void microkernel::handle_interrupt (int vector);

	int microkernel::do_interrupt (void);
	void microkernel::return_from_interrupt (void);
};

/*a Wrapper
 */
#endif
