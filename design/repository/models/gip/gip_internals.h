/*a Copyright Gavin J Stark and John Croft, 2003
 */

/*a Wrapper
 */
#ifndef GIP_INTERNALS
#define GIP_INTERNALS

/*a Includes
 */

/*a Types
 */
/*t t_gip_alu_op
 */
typedef enum
{
    gip_alu_op_mov,
    gip_alu_op_mvn,
    gip_alu_op_and,
    gip_alu_op_or,
    gip_alu_op_xor,
    gip_alu_op_bic,
    gip_alu_op_orn,
    gip_alu_op_and_cnt,
    gip_alu_op_and_xor,
    gip_alu_op_xor_first,
    gip_alu_op_xor_last,
    gip_alu_op_bit_reverse,
    gip_alu_op_byte_reverse,
    gip_alu_op_lsl,
    gip_alu_op_lsr,
    gip_alu_op_asr,
    gip_alu_op_ror,
    gip_alu_op_ror33,
    gip_alu_op_add,
    gip_alu_op_adc,
    gip_alu_op_sub,
    gip_alu_op_sbc,
    gip_alu_op_rsub,
    gip_alu_op_rsbc,
    gip_alu_op_init,
    gip_alu_op_mla,
    gip_alu_op_mlb,
    gip_alu_op_divst,
} t_gip_alu_op;

/*t t_gip_alu_op1_src
 */
typedef enum
{
    gip_alu_op1_src_a_in,
    gip_alu_op1_src_acc,
} t_gip_alu_op1_src;

/*t t_gip_alu_op2_src
 */
typedef enum
{
    gip_alu_op2_src_b_in,
    gip_alu_op2_src_acc,
    gip_alu_op2_src_shf,
    gip_alu_op2_src_constant
} t_gip_alu_op2_src;

/*t t_gip_mem_op
 */
typedef enum
{
    gip_mem_op_none,
    gip_mem_op_store_word,
    gip_mem_op_store_half,
    gip_mem_op_store_byte,
    gip_mem_op_load_word,
    gip_mem_op_load_half,
    gip_mem_op_load_byte
} t_gip_mem_op;

/*t t_gip_register_encode
 */
typedef enum
{
    gip_register_encode_acc,
    gip_register_encode_shf
} t_gip_register_encode;

/*a Wrapper
 */
#endif
