/*a Copyright Gavin J Stark and John Croft, 2003
 */

/*a Wrapper
 */
#ifndef GIP_INSTRUCIONS
#define GIP_INSTRUCIONS

/*a Includes
 */

/*a Types
 */
/*t t_gip_ins_cc
 */
typedef enum
{
    gip_ins_cc_al,
    gip_ins_cc_eq,
    gip_ins_cc_ne,
    gip_ins_cc_cc,
    gip_ins_cc_cs,
    gip_ins_cc_gt,
    gip_ins_cc_ge,
    gip_ins_cc_lt,
    gip_ins_cc_le,
    gip_ins_cc_pl,
    gip_ins_cc_mi,
    gip_ins_cc_vs,
    gip_ins_cc_vc,
    gip_ins_cc_ls,
    gip_ins_cc_hi
} t_gip_ins_cc;

/*t t_gip_ins_op
 */
typedef enum
{
    gip_ins_op_iadd,
    gip_ins_op_iadc,
    gip_ins_op_isub,
    gip_ins_op_isbc,
    gip_ins_op_irsb,
    gip_ins_op_irsc,
    gip_ins_op_iand,
    gip_ins_op_iorr,
    gip_ins_op_ixor,
    gip_ins_op_ibic,
    gip_ins_op_iorn,
    gip_ins_op_imov,
    gip_ins_op_imvn,
    gip_ins_op_ilsl,
    gip_ins_op_ilsr,
    gip_ins_op_iasr,
    gip_ins_op_iror,
    gip_ins_op_iror33,
    gip_ins_op_icprd,
    gip_ins_op_icpwr,
    gip_ins_op_icpcmd,
    gip_ins_op_ildrpre,
    gip_ins_op_ildrpost,
    gip_ins_op_istr,
} t_gip_ins_op;

/*t t_gip_ins_opts
 */
typedef union
{
    struct
    {
        int p;
        int s;
    } alu;
    struct
    {
        int add; // 1 for index add offset, 0 for index subtract offset
        int burst_remaining; // number of reads to occur after this in the burst; 0 to 15
    } memory;
} t_gip_ins_opts;

/*t t_gip_instruction
 */
typedef struct t_gip_instruction
{
    t_gip_ins_op op;
    t_gip_ins_opts opts;
    int a;
    int flush;
    int rn;
    int rm_type;
    unsigned int rm_data;
    int rfw;
} t_gip_instruction;

/*t t_gip_pipeline_results
 */
typedef struct t_gip_pipeline_results
{
    int flush; // If the instruction at the ALU stage issued a flush
    int rfw; // From the end of the pipeline, so that branches and jump tables may be handled
    unsigned int rfw_data; // To go with rfw, the data being written; writes to the PC should always follow (or be simultaneous with) a flush
} t_gip_pipeline_results;

/*a Wrapper
 */
#endif
