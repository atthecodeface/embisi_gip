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
    gip_ins_subclass_arith_init=8,
    gip_ins_subclass_arith_mla=10,
    gip_ins_subclass_arith_mlb=11,
    gip_ins_subclass_arith_dva=10,
    gip_ins_subclass_arith_dvb=11,

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

/*t t_gip_ins_rd_int
 */
typedef enum
{
    gip_ins_rd_int_none = 0,
    gip_ins_rd_int_eq = 0x10,
    gip_ins_rd_int_ne = 0x11,
    gip_ins_rd_int_cs = 0x12,
    gip_ins_rd_int_cc = 0x13,
    gip_ins_rd_int_mi = 0x14,
    gip_ins_rd_int_pl = 0x15,
    gip_ins_rd_int_vs = 0x16,
    gip_ins_rd_int_vc = 0x17,
    gip_ins_rd_int_hi = 0x18,
    gip_ins_rd_int_ls = 0x19,
    gip_ins_rd_int_ge = 0x1a,
    gip_ins_rd_int_lt = 0x1b,
    gip_ins_rd_int_gt = 0x1c,
    gip_ins_rd_int_le = 0x1d,
    gip_ins_rd_int_pc = 0x1f,
} t_gip_ins_rd_int;

/*t t_gip_ins_rd_type
 */
typedef enum
{
    gip_ins_rd_type_internal,
    gip_ins_rd_type_register,
    gip_ins_rd_type_coproc,
    gip_ins_rd_type_periph
} t_gip_ins_rd_type;

/*t t_gip_ins_rd
 */
typedef struct
{
    t_gip_ins_rd_type type;
    union
    {
        int r;
        t_gip_ins_rd_int internal;
        int coproc;
        int periph;
    } data;
} t_gip_ins_rd;

/*t t_gip_ins_rnm_int
 */
typedef enum
{
    gip_ins_rnm_int_none = 0,
    gip_ins_rnm_int_shf = 0x10,
    gip_ins_rnm_int_acc = 0x11,
    gip_ins_rnm_int_pc = 0x1f,
} t_gip_ins_rnm_int;

/*t t_gip_ins_rnm_type
 */
typedef enum
{
    gip_ins_rnm_type_internal,
    gip_ins_rnm_type_register,
    gip_ins_rnm_type_coproc,
    gip_ins_rnm_type_periph
} t_gip_ins_rnm_type;

/*t t_gip_ins_rnm
 */
typedef struct
{
    t_gip_ins_rnm_type type;
    union
    {
        int r;
        t_gip_ins_rnm_int internal;
        int coproc;
        int periph;
    } data;
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
