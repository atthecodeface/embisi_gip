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
    gip_ins_subclass_logic_andcnt=7,
    gip_ins_subclass_logic_andxor=8,
    gip_ins_subclass_logic_xorfirst=9,
    gip_ins_subclass_logic_xorlast=10,
    gip_ins_subclass_logic_bitreverse=11,
    gip_ins_subclass_logic_bytereverse=12,

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
    gip_ins_rd_int_eq = 0x00,
    gip_ins_rd_int_ne = 0x01,
    gip_ins_rd_int_cs = 0x02,
    gip_ins_rd_int_cc = 0x03,
    gip_ins_rd_int_hi = 0x04,
    gip_ins_rd_int_ls = 0x05,
    gip_ins_rd_int_ge = 0x06,
    gip_ins_rd_int_lt = 0x07,
    gip_ins_rd_int_gt = 0x08,
    gip_ins_rd_int_le = 0x09,
    gip_ins_rd_int_pc = 0x10,
} t_gip_ins_rd_int;

/*t t_gip_ins_rnm_int
 */
typedef enum
{
    gip_ins_rnm_int_shf = 0x18,
    gip_ins_rnm_int_acc = 0x19,
    gip_ins_rnm_int_pc = 0x10,
} t_gip_ins_rnm_int;

/*t t_gip_ins_r_type - 3 bit field, 5 bits of sub-field defined below
 */
typedef enum
{
    gip_ins_r_type_register = 0,
    gip_ins_r_type_register_indirect = 1, // DEPRECATED
    gip_ins_r_type_periph = 2, // access to 2-cycle peripheral space (timer, ?uart?)
    gip_ins_r_type_postbus = 3, // direct addressing of postbus register file, FIFOs and command/status
    gip_ins_r_type_special = 4, // access to DAGs, scheduler semaphores, repeat counts, ZOL data
    gip_ins_r_type_internal = 5, // conditions, accumulator, shifter result, pc
    gip_ins_r_type_none = 6,
    gip_ins_r_type_none_b = 7,
    gip_ins_r_type_no_override = 7, // only used by native decode
} t_gip_ins_r_type;

/*t t_gip_ins_r
 */
typedef struct t_gip_ins_r
{
    t_gip_ins_r_type type;
    union
    {
        int r;
        t_gip_ins_rd_int rd_internal;
        t_gip_ins_rnm_int rnm_internal;
    } data;
} t_gip_ins_r;

/*t t_gip_instruction
 */
typedef struct t_gip_instruction
{
    t_gip_ins_class gip_ins_class;
    t_gip_ins_subclass gip_ins_subclass;
    t_gip_ins_opts gip_ins_opts;
    t_gip_ins_cc gip_ins_cc;
    t_gip_ins_r gip_ins_rn;
    int rm_is_imm;
    union {
        t_gip_ins_r gip_ins_rm;
        unsigned int immediate;
    } rm_data;
    t_gip_ins_r gip_ins_rd;
    int k; // Burst size remaining for load/store burst
    int a; // set accumulator with ALU result
    int f; // flush rest of pipeline if the condition succeeds
    int d; // delay slot instruction - do not flush even if a flush is indicated
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

/*t t_gip_special_reg
 */
typedef enum
{
    gip_special_reg_semaphores = 0, // read only
    gip_special_reg_semaphores_set = 1, // write only
    gip_special_reg_semaphores_clear = 2, // write only
    gip_special_reg_gip_config = 3, // scheduler mode, ARM trap semaphore, privelege state of thread 0
    gip_special_reg_thread = 4, // thread to write with 'selected' thread; may be read
    gip_special_reg_thread_pc = 5, // thread restart address, bottom bit indicates current thread or selected thread; on reads actually you get the selected thread, but only if no scheduling is going on
    gip_special_reg_thread_data = 6, // thread restart semaphore mask, decoder type, pipeline config (trail, sticky), ALU mode (for native) - restart data; on reads actually you get the selected thread, but only if no scheduling is going on
    gip_special_reg_thread_regmap = 7, // Need something here - may be part of data?
    gip_special_reg_repeat_count = 8, // Register repeat count; for nested, one can use immediate or this value, so not two variable nested! - not readable
    gip_special_reg_preempted_pc_l = 12, // Low priority PC value stacked, if thread 0 was preempted; bit 0 indicates 1 for invalid (not preempted)
    gip_special_reg_preempted_pc_m = 13, // Medium priority PC value stacked, if thread 1-3 was preempted; bit 0 indicaes 1 for invalid (not preempted)
    gip_special_reg_preempted_flags = 14, // Low priority flags (0-7), medium priority flags (8-15), if preempted
    gip_special_reg_dag_value_0 = 16, // Data-address generator 0 value
    gip_special_reg_dag_base_0 = 17, // Data-address generator 0 config base
    gip_special_reg_dag_config_0 = 18, // Data-address generator 0 config size, current offset, bit reverse
    gip_special_reg_dag_value_1 = 20, // Data-address generator 1 value
    gip_special_reg_dag_base_1 = 21, // Data-address generator 1 config base
    gip_special_reg_dag_config_1 = 22, // Data-address generator 1 config size, current offset, bit reverse

} t_gip_special_reg;

/*t t_gip_postbus_reg
 */
typedef enum
{
    gip_postbus_reg_fifo = 4, // bit that indicates FIFO 1 rather than FIFO 0

    gip_postbus_reg_command_0 = 0, // Command register 0
    gip_postbus_reg_tx_fifo_0 = 1, // Data FIFO write 0
    gip_postbus_reg_rx_fifo_0 = 3, // Data FIFO read 0

    gip_postbus_reg_command_1 = 4, // Command register 1
    gip_postbus_reg_tx_fifo_1 = 5, // Data FIFO write 1
    gip_postbus_reg_rx_fifo_1 = 7, // Data FIFO read 1

    gip_postbus_reg_rx_fifo_config_0 = 8, // Data Rx FIFO config 0
    gip_postbus_reg_tx_fifo_config_0 = 9, // Data Tx FIFO config 0
    gip_postbus_reg_rx_fifo_config_1 = 12, // Data Rx FIFO config 1
    gip_postbus_reg_tx_fifo_config_1 = 13, // Data Tx FIFO config 1

} t_gip_postbus_reg;

/*a Wrapper
 */
#endif
