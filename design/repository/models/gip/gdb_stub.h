/*a Copyright Gavin J Stark and John Croft, 2003
 */

/*a Wrapper
 */
#ifndef INC_GDB_STUB
#define INC_GDB_STUB

/*a External functions
 */
extern void gdb_stub_disable( void );
extern void gdb_stub_init( class c_gip_pipeline_single *gip, class c_memory_model *memory, class c_mmio_model *mmio );
extern void gdb_trap (int trapid);
extern int gdb_poll (int reason );
extern void gdb_set_breakpoint (unsigned int addr);
extern void gdb_clear_breakpoint (unsigned int addr);

/*a Wrapper
 */
#endif
