/*a Copyright Gavin J Stark and John Croft, 2003
 */

/*a Wrapper
 */
#ifndef C_GIP_PIPELINE_SINGLE
#define C_GIP_PIPELINE_SINGLE

/*a Includes
 */
#include "c_execution_model_class.h"
#include "gip_instructions.h"
#include "gip_internals.h"

/*a Types
 */
/*t	c_gip_pipeline_single
*/
class c_gip_pipeline_single : public c_execution_model_class
{
public:

    /*b Public access methods
     */
    c_gip_pipeline_single::c_gip_pipeline_single( class c_memory_model *memory_model );
    c_gip_pipeline_single::~c_gip_pipeline_single();

    /*b Execution methods
     */
    virtual int c_gip_pipeline_single::step( int *reason, int requested_count );

    /*b Code loading methods
     */
    virtual void c_gip_pipeline_single::load_code( FILE *f, unsigned int base_address );
    virtual void c_gip_pipeline_single::load_code_binary( FILE *f, unsigned int base_address );
    virtual void c_gip_pipeline_single::load_symbol_table( char *filename );

    /*b Debug methods
     */
    virtual void c_gip_pipeline_single::set_register( int r, unsigned int value );
    virtual unsigned int c_gip_pipeline_single::get_register( int r );
    virtual void c_gip_pipeline_single::set_flags( int value, int mask );
    virtual int c_gip_pipeline_single::get_flags( void );
    virtual int c_gip_pipeline_single::set_breakpoint( unsigned int address );
    virtual int c_gip_pipeline_single::unset_breakpoint( unsigned int address );
    virtual void c_gip_pipeline_single::halt_cpu( void );
    virtual void c_gip_pipeline_single::debug( int mask );

private:
    /*b Internal instruction execution methods
     */
    unsigned int c_gip_pipeline_single::read_int_register( t_gip_instruction *instr, t_gip_ins_rnm r );
    void c_gip_pipeline_single::disassemble_int_instruction( t_gip_instruction *inst );
    void c_gip_pipeline_single::execute_int_alu_instruction( t_gip_alu_op alu_op, t_gip_alu_op1_src op1_src, t_gip_alu_op2_src op2_src, int a, int s, int p );
    void c_gip_pipeline_single::execute_int_memory_instruction( t_gip_mem_op gip_mem_op, unsigned int address, unsigned int data_in );
    void c_gip_pipeline_single::execute_int_instruction( t_gip_instruction *inst, struct t_gip_pipeline_results *results );

    /*b Internal instruction building methods
     */
    void c_gip_pipeline_single::build_gip_instruction_alu( t_gip_instruction *gip_instr, t_gip_ins_class gip_ins_class, t_gip_ins_subclass gip_ins_subclass, int a, int s, int p, int f );
    void c_gip_pipeline_single::build_gip_instruction_shift( t_gip_instruction *gip_instr, t_gip_ins_subclass gip_ins_subclass, int s, int f );
    void c_gip_pipeline_single::build_gip_instruction_load( t_gip_instruction *gip_instr, t_gip_ins_subclass gip_ins_subclass, int preindex, int up, int stack, int burst_left, int a, int f );
    void c_gip_pipeline_single::build_gip_instruction_store( t_gip_instruction *gip_instr, t_gip_ins_subclass gip_ins_subclass, int preindex, int up, int use_shift, int stack, int burst_left, int a, int f );
    void c_gip_pipeline_single::build_gip_instruction_cc( t_gip_instruction *gip_instr, t_gip_ins_cc gip_ins_cc);
    void c_gip_pipeline_single::build_gip_instruction_rn( t_gip_instruction *gip_instr, t_gip_ins_rnm gip_ins_rn );
    void c_gip_pipeline_single::build_gip_instruction_rm( t_gip_instruction *gip_instr, t_gip_ins_rnm gip_ins_rm );
    void c_gip_pipeline_single::build_gip_instruction_rd( t_gip_instruction *gip_instr, t_gip_ins_rd gip_ins_rd );
    void c_gip_pipeline_single::build_gip_instruction_immediate( t_gip_instruction *gip_instr, unsigned int imm_val );
    t_gip_ins_cc c_gip_pipeline_single::map_condition_code( int arm_cc );
    t_gip_ins_rnm c_gip_pipeline_single::map_source_register( int arm_r );
    t_gip_ins_rd c_gip_pipeline_single::map_destination_register( int arm_rd );
    t_gip_ins_subclass c_gip_pipeline_single::map_shift( int shf_how, int imm, int amount );

    /*b ARM Execution methods
     */
    int c_gip_pipeline_single::execute_arm_alu( unsigned int opcode );
    int c_gip_pipeline_single::execute_arm_branch( unsigned int opcode );
    int c_gip_pipeline_single::execute_arm_ld_st( unsigned int opcode );
    int c_gip_pipeline_single::execute_arm_ldm_stm( unsigned int opcode );
    int c_gip_pipeline_single::execute_arm_mul( unsigned int opcode );
    int c_gip_pipeline_single::execute_arm_trace( unsigned int opcode );


};

/*a External functions
 */

/*a Wrapper
 */
#endif
