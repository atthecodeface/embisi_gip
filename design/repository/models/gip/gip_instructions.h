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
/*t t_gip_ins_class
 */
typedef enum
{
    gip_ins_class_arith = 0,
    gip_ins_class_logic = 1,
    gip_ins_class_shift = 2,
    gip_ins_class_coproc = 3,
    gip_ins_class_load = 4,
    gip_ins_class_store = 5,
} t_gip_ins_class;

/*t t_gip_ins_subclass
 */
typedef enum
{
    gip_ins_subclass_arith_add=0,
    gip_ins_subclass_arith_adc=1,
    gip_ins_subclass_arith_sub=2,
    gip_ins_subclass_arith_sbc=3,
    gip_ins_subclass_arith_rsb=4,
    gip_ins_subclass_arith_rsc=5,

    gip_ins_subclass_logic_and=0,
    gip_ins_subclass_logic_or=1,
    gip_ins_subclass_logic_xor=2,
    gip_ins_subclass_logic_orn=3,
    gip_ins_subclass_logic_bic=4,
    gip_ins_subclass_logic_mov=5,
    gip_ins_subclass_logic_mvn=6,

    gip_ins_subclass_shift_lsl=0,
    gip_ins_subclass_shift_lsr=1,
    gip_ins_subclass_shift_asr=2,
    gip_ins_subclass_shift_ror=3,
    gip_ins_subclass_shift_ror33=4,

    gip_ins_subclass_memory_word = 0,
    gip_ins_subclass_memory_half = 1,
    gip_ins_subclass_memory_byte = 2,
    gip_ins_subclass_memory_size = 3,
    gip_ins_subclass_memory_down = 0,
    gip_ins_subclass_memory_up = 4,
    gip_ins_subclass_memory_dirn = 4,
    gip_ins_subclass_memory_postindex = 0,
    gip_ins_subclass_memory_preindex = 8,
    gip_ins_subclass_memory_index = 8,
} t_gip_ins_subclass;

/*t t_gip_ins_opts
 */
typedef union
{
    struct
    {
        int s;
        int p;
    } alu;
    struct
    {
        int stack;
    } load;
    struct
    {
        int stack;
        int offset_type; // 1 for SHF, 0 for access size
    } store;
} t_gip_ins_opts;

/*t t_gip_ins_cc
 */
typedef enum
{
    gip_ins_cc_eq = 0x0,
    gip_ins_cc_ne = 0x1,
    gip_ins_cc_cs = 0x2,
    gip_ins_cc_cc = 0x3,
    gip_ins_cc_mi = 0x4,
    gip_ins_cc_pl = 0x5,
    gip_ins_cc_vs = 0x6,
    gip_ins_cc_vc = 0x7,
    gip_ins_cc_hi = 0x8,
    gip_ins_cc_ls = 0x9,
    gip_ins_cc_ge = 0xa,
    gip_ins_cc_lt = 0xb,
    gip_ins_cc_gt = 0xc,
    gip_ins_cc_le = 0xd,
    gip_ins_cc_always = 0xe,
    gip_ins_cc_cp = 0xf,
} t_gip_ins_cc;

/*t t_gip_ins_rd
 */
typedef enum
{
    gip_ins_rd_none = 0,
    gip_ins_rd_eq = 0x10,
    gip_ins_rd_ne = 0x11,
    gip_ins_rd_cs = 0x12,
    gip_ins_rd_cc = 0x13,
    gip_ins_rd_mi = 0x14,
    gip_ins_rd_pl = 0x15,
    gip_ins_rd_vs = 0x16,
    gip_ins_rd_vc = 0x17,
    gip_ins_rd_hi = 0x18,
    gip_ins_rd_ls = 0x19,
    gip_ins_rd_ge = 0x1a,
    gip_ins_rd_lt = 0x1b,
    gip_ins_rd_gt = 0x1c,
    gip_ins_rd_le = 0x1d,
    gip_ins_rd_pc = 0x1f,
    gip_ins_rd_r00 = 0x20,
    gip_ins_rd_r01 = 0x21,
    gip_ins_rd_r02 = 0x22,
    gip_ins_rd_r03 = 0x23,
    gip_ins_rd_r04 = 0x24,
    gip_ins_rd_r05 = 0x25,
    gip_ins_rd_r06 = 0x26,
    gip_ins_rd_r07 = 0x27,
    gip_ins_rd_r08 = 0x28,
    gip_ins_rd_r09 = 0x29,
    gip_ins_rd_r10 = 0x2a,
    gip_ins_rd_r11 = 0x2b,
    gip_ins_rd_r12 = 0x2c,
    gip_ins_rd_r13 = 0x2d,
    gip_ins_rd_r14 = 0x2e,
    gip_ins_rd_r15 = 0x2f,
    gip_ins_rd_r16 = 0x30,
    gip_ins_rd_r17 = 0x31,
    gip_ins_rd_r18 = 0x32,
    gip_ins_rd_r19 = 0x33,
    gip_ins_rd_r20 = 0x34,
    gip_ins_rd_r21 = 0x35,
    gip_ins_rd_r22 = 0x36,
    gip_ins_rd_r23 = 0x37,
    gip_ins_rd_r24 = 0x38,
    gip_ins_rd_r25 = 0x39,
    gip_ins_rd_r26 = 0x3a,
    gip_ins_rd_r27 = 0x3b,
    gip_ins_rd_r28 = 0x3c,
    gip_ins_rd_r29 = 0x3d,
    gip_ins_rd_r30 = 0x3e,
    gip_ins_rd_r31 = 0x3f,
} t_gip_ins_rd;

/*t t_gip_ins_rnm
 */
typedef enum
{
    gip_ins_rnm_none = 0,
    gip_ins_rnm_shf = 0x10,
    gip_ins_rnm_acc = 0x11,
    gip_ins_rnm_pc = 0x1f,
    gip_ins_rnm_r00 = 0x20,
    gip_ins_rnm_r01 = 0x21,
    gip_ins_rnm_r02 = 0x22,
    gip_ins_rnm_r03 = 0x23,
    gip_ins_rnm_r04 = 0x24,
    gip_ins_rnm_r05 = 0x25,
    gip_ins_rnm_r06 = 0x26,
    gip_ins_rnm_r07 = 0x27,
    gip_ins_rnm_r08 = 0x28,
    gip_ins_rnm_r09 = 0x29,
    gip_ins_rnm_r10 = 0x2a,
    gip_ins_rnm_r11 = 0x2b,
    gip_ins_rnm_r12 = 0x2c,
    gip_ins_rnm_r13 = 0x2d,
    gip_ins_rnm_r14 = 0x2e,
    gip_ins_rnm_r15 = 0x2f,
    gip_ins_rnm_r16 = 0x30,
    gip_ins_rnm_r17 = 0x31,
    gip_ins_rnm_r18 = 0x32,
    gip_ins_rnm_r19 = 0x33,
    gip_ins_rnm_r20 = 0x34,
    gip_ins_rnm_r21 = 0x35,
    gip_ins_rnm_r22 = 0x36,
    gip_ins_rnm_r23 = 0x37,
    gip_ins_rnm_r24 = 0x38,
    gip_ins_rnm_r25 = 0x39,
    gip_ins_rnm_r26 = 0x3a,
    gip_ins_rnm_r27 = 0x3b,
    gip_ins_rnm_r28 = 0x3c,
    gip_ins_rnm_r29 = 0x3d,
    gip_ins_rnm_r30 = 0x3e,
    gip_ins_rnm_r31 = 0x3f,
} t_gip_ins_rnm;

/*t t_gip_instruction
 */
typedef struct t_gip_instruction
{
    t_gip_ins_class gip_ins_class;
    t_gip_ins_subclass gip_ins_subclass;
    t_gip_ins_opts gip_ins_opts;
    t_gip_ins_cc gip_ins_cc;
    t_gip_ins_rnm gip_ins_rn;
    int rm_is_imm;
    union {
        t_gip_ins_rnm gip_ins_rm;
        unsigned int immediate;
    } rm_data;
    t_gip_ins_rd gip_ins_rd;
    int k; // Burst size remaining for load/store burst
    int a; // set accumulator with ALU result
    int f; // flush rest of pipeline if the condition succeeds
    unsigned int pc; // PC value for the instruction - for ARM emulation, PC+8 generally
} t_gip_instruction;

/*t t_gip_pipeline_results
 */
typedef struct t_gip_pipeline_results
{
    int flush; // If the instruction at the ALU stage issued a flush
    int write_pc; // From the end of the pipeline, so that branches and jump tables may be handled
    unsigned int rfw_data; // To go with write_pc, the data being written; writes to the PC should always follow (or be simultaneous with) a flush
} t_gip_pipeline_results;

/*a Wrapper
 */
#endif
