/*a Copyright Gavin J Stark and John Croft, 2003
 */

/*a Wrapper
 */
#ifndef C_GIP_PIPELINE_SINGLE
#define C_GIP_PIPELINE_SINGLE

/*a Includes
 */
#include "gip_instructions.h"
#include "gip_internals.h"

/*a Types
 */
/*t	c_gip_pipeline_single
*/
class c_gip_pipeline_single
{
public:
    c_gip_pipeline_single::c_gip_pipeline_single( class c_memory_model *memory_model );
    c_gip_pipeline_single::~c_gip_pipeline_single();
    int c_gip_pipeline_single::arm_step( int *reason );
private:
    /*b Code loading methods
     */
    void c_gip_pipeline_single::load_code( FILE *f, unsigned int base_address );
    void c_gip_pipeline_single::load_code_binary( FILE *f, unsigned int base_address );
    void c_gip_pipeline_single::load_symbol_table( char *filename );

    /*b Internal instruction execution methods
     */
    unsigned int c_gip_pipeline_single::read_int_register( int r );
    void c_gip_pipeline_single::execute_int_alu_instruction( t_gip_alu_op alu_op, t_gip_alu_op1_src op1_src, t_gip_alu_op2_src op2_src, int s, int p );
    void c_gip_pipeline_single::execute_int_instruction( struct t_gip_instruction *inst, struct t_gip_pipeline_results *results );

    /*b ARM Execution methods
     */
    int c_gip_pipeline_single::execute_arm_alu( unsigned int opcode );
    int c_gip_pipeline_single::execute_arm_branch( unsigned int opcode );
    int c_gip_pipeline_single::execute_arm_ld_st( unsigned int opcode );
    int c_gip_pipeline_single::execute_arm_ldm_stm( unsigned int opcode );
    int c_gip_pipeline_single::execute_arm_mul( unsigned int opcode );
    int c_gip_pipeline_single::execute_arm_trace( unsigned int opcode );

    /*b Log methods
     */
    void c_gip_pipeline_single::log( char *reason, unsigned int arg );
    void c_gip_pipeline_single::log_display( FILE *f );
    void c_gip_pipeline_single::log_reset( void );
    void c_gip_pipeline_single::debug( int mask );

    struct t_gip_pipeline_single_data *pd;
};

/*a External functions
 */

/*a Wrapper
 */
#endif
