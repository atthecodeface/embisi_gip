/*a Copyright Gavin J Stark and John Croft, 2003
 */

/*a Wrapper
 */
#ifndef C_GIP_FULL
#define C_GIP_FULL

/*a Includes
 */
#include "c_execution_model_class.h"
#include "gip_instructions.h"
#include "gip_internals.h"
#include "postbus.h"

/*a Types
 */
/*t t_gip_comb_data
 */
typedef struct t_gip_comb_data
{
    unsigned int postbus_tx_data;
    t_postbus_type postbus_tx_type;
    t_postbus_ack postbus_rx_ack;
} t_gip_comb_data;

/*t t_gip_inputs
 */
typedef struct t_gip_inputs
{
    unsigned int postbus_rx_data;
    t_postbus_type postbus_rx_type;
    t_postbus_ack postbus_tx_ack;
} t_gip_inputs;

/*t	c_gip_full
*/
class c_gip_full : public c_execution_model_class
{
public:

    /*b Public access methods
     */
    c_gip_full::c_gip_full( class c_memory_model *memory_model );
    c_gip_full::~c_gip_full();

    /*b Execution methods
     */
    virtual int c_gip_full::step( int *reason, int requested_count );

    /*b Code loading methods
     */
    virtual void c_gip_full::load_code( FILE *f, unsigned int base_address );
    virtual void c_gip_full::load_code_binary( FILE *f, unsigned int base_address );
    virtual void c_gip_full::load_symbol_table( char *filename );

    /*b Debug methods
     */
    virtual void c_gip_full::set_register( int r, unsigned int value );
    virtual unsigned int c_gip_full::get_register( int r );
    virtual void c_gip_full::set_flags( int value, int mask );
    virtual int c_gip_full::get_flags( void );
    virtual int c_gip_full::set_breakpoint( unsigned int address );
    virtual int c_gip_full::unset_breakpoint( unsigned int address );
    virtual void c_gip_full::halt_cpu( void );
    virtual void c_gip_full::debug( int mask );

    /*b Inputs
     */
    t_gip_inputs inputs;

private:
    /*b Internal instruction execution methods
     */
    void c_gip_full::disassemble_int_instruction( int valid, t_gip_instruction *inst, char *buffer, int length );
    void c_gip_full::execute_int_memory_instruction( t_gip_mem_op gip_mem_op, unsigned int address, unsigned int data_in );
    void c_gip_full::execute_int_instruction( t_gip_instruction *inst, struct t_gip_pipeline_results *results );

    /*b Internal instruction building methods
     */
    void c_gip_full::build_gip_instruction_nop( t_gip_instruction *gip_instr );
    void c_gip_full::build_gip_instruction_alu( t_gip_instruction *gip_instr, t_gip_ins_class gip_ins_class, t_gip_ins_subclass gip_ins_subclass, int a, int s, int p, int f );
    void c_gip_full::build_gip_instruction_shift( t_gip_instruction *gip_instr, t_gip_ins_subclass gip_ins_subclass, int s, int f );
    void c_gip_full::build_gip_instruction_load( t_gip_instruction *gip_instr, t_gip_ins_subclass gip_ins_subclass, int preindex, int up, int stack, int burst_left, int a, int f );
    void c_gip_full::build_gip_instruction_store( t_gip_instruction *gip_instr, t_gip_ins_subclass gip_ins_subclass, int preindex, int up, int use_shift, int stack, int burst_left, int a, int f );
    void c_gip_full::build_gip_instruction_cc( t_gip_instruction *gip_instr, t_gip_ins_cc gip_ins_cc);
    void c_gip_full::build_gip_instruction_rn( t_gip_instruction *gip_instr, t_gip_ins_r gip_ins_rn );
    void c_gip_full::build_gip_instruction_rn_int( t_gip_instruction *gip_instr, t_gip_ins_rnm_int gip_ins_rn_int );
    void c_gip_full::build_gip_instruction_rm( t_gip_instruction *gip_instr, t_gip_ins_r gip_ins_rm );
    void c_gip_full::build_gip_instruction_rm_int( t_gip_instruction *gip_instr, t_gip_ins_rnm_int gip_ins_rm_int );
    void c_gip_full::build_gip_instruction_rd( t_gip_instruction *gip_instr, t_gip_ins_r gip_ins_rd );
    void c_gip_full::build_gip_instruction_rd_int( t_gip_instruction *gip_instr, t_gip_ins_rd_int gip_ins_rd_int );
    void c_gip_full::build_gip_instruction_rd_reg( t_gip_instruction *gip_instr, int gip_ins_rd_r );
    void c_gip_full::build_gip_instruction_immediate( t_gip_instruction *gip_instr, unsigned int imm_val );
    t_gip_ins_cc c_gip_full::map_condition_code( int arm_cc );
    t_gip_ins_r c_gip_full::map_source_register( int arm_r );
    t_gip_ins_r c_gip_full::map_destination_register( int arm_rd );
    t_gip_ins_subclass c_gip_full::map_shift( int shf_how, int imm, int amount );
    t_gip_ins_r c_gip_full::map_native_rm( int rm, int use_full_ext_rm );
    t_gip_ins_r c_gip_full::map_native_rn( int rn );
    t_gip_ins_r c_gip_full::map_native_rd( int rd );
    unsigned int c_gip_full::map_native_immediate( int imm );

    /*b Native Decode methods
     */
    int c_gip_full::decode_native_debug( unsigned int opcode );
    int c_gip_full::decode_native_alu( unsigned int opcode );
    int c_gip_full::decode_native_cond( unsigned int opcode );
    int c_gip_full::decode_native_shift( unsigned int opcode );
    int c_gip_full::decode_native_ldr( unsigned int opcode );
    int c_gip_full::decode_native_str( unsigned int opcode );
    int c_gip_full::decode_native_branch( unsigned int opcode );
    int c_gip_full::decode_native_extend( unsigned int opcode );

    /*b ARM Decode methods
     */
    int c_gip_full::decode_arm_debug( void );
    int c_gip_full::decode_arm_alu( void );
    int c_gip_full::decode_arm_branch( void );
    int c_gip_full::decode_arm_ld_st( void );
    int c_gip_full::decode_arm_ldm_stm( void );
    int c_gip_full::decode_arm_mul( void );
    int c_gip_full::decode_arm_trace( void );

    /*b Scheduler methods
     */
    void c_gip_full::sched_preclock( void );
    void c_gip_full::sched_clock( void );
    void c_gip_full::sched_comb( void );

    /*b Decoder methods
     */
    void c_gip_full::dec_preclock( void );
    void c_gip_full::dec_clock( void );
    void c_gip_full::dec_comb( void );

    /*b RF methods
     */
    void c_gip_full::rf_preclock( void  );
    void c_gip_full::rf_clock( void );
    void c_gip_full::rf_comb( t_gip_pipeline_results *results );

    /*b ALU methods
     */
    void c_gip_full::alu_clock( void );
    void c_gip_full::alu_preclock( void );
    void c_gip_full::alu_comb( t_gip_pipeline_results *results );

    /*b Memory stage methods
     */
    void c_gip_full::mem_clock( void );
    void c_gip_full::mem_preclock( void );
    void c_gip_full::mem_comb( t_gip_pipeline_results *results );

    /*b Postbus and special register methods
      comb is called after the pipeline register file comb, as that determines the register address to fetch
      the data is valid for the register file preclock function
      postbus covers the post bus register file, command registers and FIFO registers
      special covers the semaphores, pipeline configuration, repeat count, ZOL data - not all written through the RF path
      peripheral will include a peripheral indirect register, and all the peripherals themselves (a three-cycle access)
     */
    void c_gip_full::postbus_comb( int read_select, int read_address, unsigned int *read_data );
    void c_gip_full::postbus_preclock( int flush, int read_select, int read_address, int write_select, int write_address, unsigned int write_data );
    void c_gip_full::postbus_clock( void );
    void c_gip_full::special_comb( int read_select, int read_address, unsigned int *read_data );
    void c_gip_full::special_preclock( int flush, int read_select, int read_address, int write_select, int write_address, unsigned int write_data );
    void c_gip_full::special_clock( void );

    /*b Complete structure methods
     */
    void c_gip_full::reset( void );
    void c_gip_full::clock( void );
    void c_gip_full::preclock( void );
    void c_gip_full::comb( void *data );

};

/*a External functions
 */

/*a Wrapper
 */
#endif
