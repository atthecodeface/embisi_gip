/*a Includes
 */
include "postbus.h"

/*a Types
 */
/*t t_gip_word
 */
typedef bit[32] t_gip_word;

/*t t_gip_special_reg
 */
typedef enum [5]
{
    gip_special_reg_semaphores_clear = 0, // read returns semaphores, and clears bits in mask, clears mask; write writes mask
    gip_special_reg_semaphores_set   = 1, // read returns semaphores, and sets bits in mask, clears mask; write writes mask
    gip_special_reg_local_events   = 2, // read/write, configures which semaphores are set when local events fire
    gip_special_reg_gip_config = 3, // scheduler mode, privelege state of thread 0
    gip_special_reg_thread = 4, // thread to write with 'selected' thread; may be read
    gip_special_reg_thread_pc = 5, // thread restart address, bottom bit indicates current thread or selected thread; on reads actually you get the selected thread, but only if no scheduling is going on
    gip_special_reg_thread_data = 6, // thread restart semaphore mask, decoder type, pipeline config (trail, sticky), ALU mode (for native) - restart data; on reads actually you get the selected thread, but only if no scheduling is going on
    gip_special_reg_thread_regmap = 7, // Need something here - may be part of data?
    gip_special_reg_repeat_count = 8, // Register repeat count; for nested, one can use immediate or this value, so not two variable nested! - not readable
    gip_special_reg_preempted_pc_l = 12, // Low priority PC value stacked, if thread 0 was preempted
    gip_special_reg_preempted_pc_m = 13, // Medium priority PC value stacked, if thread 1-3 was preempted
    gip_special_reg_preempted_flags_l = 14, // ZNCV in 0 thru 3 for low priority thread
    gip_special_reg_preempted_flags_m = 15, // ZNCV in 0 thru 3 for medium priority thread
    gip_special_reg_dag_value_0 = 16, // Data-address generator 0 value
    gip_special_reg_dag_base_0 = 17, // Data-address generator 0 config base
    gip_special_reg_dag_config_0 = 18, // Data-address generator 0 config size, current offset, bit reverse
    gip_special_reg_dag_value_1 = 20, // Data-address generator 1 value
    gip_special_reg_dag_base_1 = 21, // Data-address generator 1 config base
    gip_special_reg_dag_config_1 = 22, // Data-address generator 1 config size, current offset, bit reverse

} t_gip_special_reg;

/*t t_gip_postbus_reg
 */
constant integer    gip_postbus_reg_fifo_bit = 4; // bit number that indicates FIFO
typedef enum [5]
{
    gip_postbus_reg_fifo_mask = 16, // bit mask that indicates FIFO

    gip_postbus_reg_fifo_0 = 0, // bit values that indicates FIFO 0
    gip_postbus_reg_fifo_1 = 16, // bit values that indicates FIFO 1 rather than FIFO 0

    gip_postbus_reg_tx_fifo_0 = 0, // write only - in fact, anything below command register is tx fifo
    gip_postbus_reg_rx_fifo_0 = 0, // read only - in fact, anything below status register is rx fifo
    gip_postbus_reg_status_0         = 12 , // Status register 0 - read only - bit 0 is cmd 0 tx pending (cmd and data still in use), bit 1 is rx fifo ptrs differ, bits 8 thru 11 are all 4 bits of pending
    gip_postbus_reg_command_0        = 12, // Command register 0 - write only - see postbus.h for details of its contents
    gip_postbus_reg_rx_fifo_config_0 = 14, // Data Rx FIFO config 0
    gip_postbus_reg_tx_fifo_config_0 = 15, // Data Tx FIFO config 0

    gip_postbus_reg_tx_fifo_1 = 16, // write only - in fact, anything below command register is tx fifo
    gip_postbus_reg_rx_fifo_1 = 16, // read only - in fact, anything below status register is rx fifo
    gip_postbus_reg_status_1         = 28 , // Status register 0 - read only - bit 0 is cmd 0 tx pending (cmd and data still in use), bit 1 is rx fifo ptrs differ, bits 8 thru 11 are all 4 bits of pending
    gip_postbus_reg_command_1        = 28, // Command register 0 - write only - see postbus.h for details of its contents
    gip_postbus_reg_rx_fifo_config_1 = 30, // Data Rx FIFO config 0
    gip_postbus_reg_tx_fifo_config_1 = 31, // Data Tx FIFO config 0

} t_gip_postbus_reg;

/*t t_gip_native_mode
 */
typedef enum [2]
{
    gip_native_mode_default = 0, // If 0, then the default mode for the thread is used (i.e. 1 thru 3)
    gip_native_mode_bit = 1,
    gip_native_mode_math = 2,
    gip_native_mode_gp = 3,
} t_gip_native_mode;

/*t t_gip_native_ins_class
 */
typedef enum [4]
{
    gip_native_ins_class_alu_reg = 0,
    gip_native_ins_class_alu_imm = 1,
    gip_native_ins_class_cond_reg = 2,
    gip_native_ins_class_cond_imm = 3,
    gip_native_ins_class_shift = 4,
    gip_native_ins_class_memory = 5,
    gip_native_ins_class_branch = 6,
    gip_native_ins_class_special = 7,
    gip_native_ins_class_extimm_0 = 8, // 8 thru 11
    gip_native_ins_class_extimm_1 = 9, // 8 thru 11
    gip_native_ins_class_extimm_2 = 10, // 8 thru 11
    gip_native_ins_class_extimm_3 = 11, // 8 thru 11
    gip_native_ins_class_extrdrm = 12, // extend rd, and 'top' half of rm; rn is as in instruction, bottom half of rm is as in instruction
    gip_native_ins_class_extrnrm = 13, // extend rn, and 'top' half of rm; rd is as in instruction, bottom half of rm is as in instruction
    gip_native_ins_class_extcmd = 14, // extend command by 6 bits
    gip_native_ins_class_debug = 15, // Undefined as yet

} t_gip_native_ins_class;

/*t t_gip_native_ins_subclass
 */
typedef enum [5]
{
    gip_native_ins_subclass_alu_and=0, // bits 8-11 of 0-11
    gip_native_ins_subclass_alu_or=1,
    gip_native_ins_subclass_alu_xor=2,
    gip_native_ins_subclass_alu_mov=3,
    gip_native_ins_subclass_alu_mvn=4,
    gip_native_ins_subclass_alu_add=5,
    gip_native_ins_subclass_alu_sub=6,
    gip_native_ins_subclass_alu_adc=7,

    gip_native_ins_subclass_alu_xorfirst=8, // common to all modes
    gip_native_ins_subclass_alu_rsb=9,

    gip_native_ins_subclass_alu_bic=10, // bit mode
    gip_native_ins_subclass_alu_orn=11,
    gip_native_ins_subclass_alu_andcnt=12,
    gip_native_ins_subclass_alu_xorlast=13,
    gip_native_ins_subclass_alu_bitreverse=14,
    gip_native_ins_subclass_alu_bytereverse=15,

    gip_native_ins_subclass_alu_init=10, // math mode
    gip_native_ins_subclass_alu_mla=11,
    gip_native_ins_subclass_alu_mlb=12,
    gip_native_ins_subclass_alu_sbc=13,
    gip_native_ins_subclass_alu_dva=14,
    gip_native_ins_subclass_alu_dvb=15,

    gip_native_ins_subclass_alu_andxor=11, // gp mode uses xorfirst (8), rsb (9), bic (10), andxor (11), rsc (12), readflags (14), writeflags (15)
    gip_native_ins_subclass_alu_rsc=12,
    gip_native_ins_subclass_alu_readflags = 14,
    gip_native_ins_subclass_alu_writeflags = 15,

    gip_native_ins_subclass_cond_eq=0, // bits 8-11 of 0-11, maps to ALU op sub, cond eq/z set
    gip_native_ins_subclass_cond_ne=1, // bits 8-11 of 0-11, maps to ALU op sub, cond ne/z clr
    gip_native_ins_subclass_cond_gt=2, // bits 8-11 of 0-11, maps to ALU op sub, cond gt
    gip_native_ins_subclass_cond_ge=3, // bits 8-11 of 0-11, maps to ALU op sub, cond ge
    gip_native_ins_subclass_cond_lt=4, // bits 8-11 of 0-11, maps to ALU op sub, cond lt
    gip_native_ins_subclass_cond_le=5, // bits 8-11 of 0-11, maps to ALU op sub, cond le
    gip_native_ins_subclass_cond_hi=6, // bits 8-11 of 0-11, maps to ALU op sub, cond hi/c set
    gip_native_ins_subclass_cond_hs=7, // bits 8-11 of 0-11, maps to ALU op sub, cond hs/c clr
    gip_native_ins_subclass_cond_lo=8, // bits 8-11 of 0-11, maps to ALU op sub, cond lo
    gip_native_ins_subclass_cond_ls=9, // bits 8-11 of 0-11, maps to ALU op sub, cond ls
    gip_native_ins_subclass_cond_sps=10, // bits 8-11 of 0-11, maps to no op, cond P set
    gip_native_ins_subclass_cond_spc=11, // bits 8-11 of 0-11, maps to no op, cond P set
    gip_native_ins_subclass_cond_allset=12, // bits 8-11 of 0-11, maps to ALU op ANDXOR, cond result==0
    gip_native_ins_subclass_cond_allclr=13, // bits 8-11 of 0-11, maps to ALU op AND, cond result==0
    gip_native_ins_subclass_cond_anyset=14, // bits 8-11 of 0-11, maps to ALU op AND, cond result!=0
    gip_native_ins_subclass_cond_anyclr=15, // bits 8-11 of 0-11, maps to ALU op ANDXOR, cond result!=0
    gip_native_ins_subclass_cond_seq=16, // bits 8-11 of 0-11, maps to ALU op sub, cond eq/z set
    gip_native_ins_subclass_cond_sne=17, // bits 8-11 of 0-11, maps to ALU op sub, cond ne/z clr
    gip_native_ins_subclass_cond_sgt=18, // bits 8-11 of 0-11, maps to ALU op sub, cond gt
    gip_native_ins_subclass_cond_sge=19, // bits 8-11 of 0-11, maps to ALU op sub, cond ge
    gip_native_ins_subclass_cond_slt=20, // bits 8-11 of 0-11, maps to ALU op sub, cond lt
    gip_native_ins_subclass_cond_sle=21, // bits 8-11 of 0-11, maps to ALU op sub, cond le
    gip_native_ins_subclass_cond_shi=22, // bits 8-11 of 0-11, maps to ALU op sub, cond hi/c set
    gip_native_ins_subclass_cond_shs=23, // bits 8-11 of 0-11, maps to ALU op sub, cond hs/c clr
    gip_native_ins_subclass_cond_slo=24, // bits 8-11 of 0-11, maps to ALU op sub, cond lo
    gip_native_ins_subclass_cond_sls=25, // bits 8-11 of 0-11, maps to ALU op sub, cond ls
    gip_native_ins_subclass_cond_smi=26, // bits 8-11 of 0-11, maps to ALU op sub, cond mi/n set
    gip_native_ins_subclass_cond_spl=27, // bits 8-11 of 0-11, maps to ALU op sub, cond pl/n clr
    gip_native_ins_subclass_cond_svs=28, // bits 8-11 of 0-11, maps to ALU op sub, cond vs/v set
    gip_native_ins_subclass_cond_svc=29, // bits 8-11 of 0-11, maps to ALU op sub, cond vc/v clr

    gip_native_ins_subclass_shift_lsl=0, // bits 10-11 of 0-11
    gip_native_ins_subclass_shift_lsr=1,
    gip_native_ins_subclass_shift_asr=2,
    gip_native_ins_subclass_shift_ror=3,
    gip_native_ins_subclass_shift_ror33=4, // possible only through imm ror of 0

    gip_native_ins_subclass_memory_load = 0, // bits 8 of 0-11
    gip_native_ins_subclass_memory_store = 1, // bits 8 of 0-11
    gip_native_ins_subclass_memory_word_noindex = 0, // bits 9-11 of 0-11; extcmd may provide burst and pre/postindex byte/word
    gip_native_ins_subclass_memory_half_noindex = 1,
    gip_native_ins_subclass_memory_byte_noindex = 2,
    gip_native_ins_subclass_memory_word_preindex_up = 3, // store does wb, load does not
    gip_native_ins_subclass_memory_word_preindex_up_shf = 4, // ldr uses extreg rm to use preindex by rm, or extimm for full imm
    gip_native_ins_subclass_memory_word_preindex_down_shf = 5, // ldr uses extreg rm to use preindex by rm, or extimm for full imm
    gip_native_ins_subclass_memory_word_postindex_up = 6, // store only writebacks rn as rd by default; ldr uses extreg rm to use preindex by rm, or extimm for full imm
    gip_native_ins_subclass_memory_word_postindex_down = 7, // store only writebacks rn as rd by default; ldr uses extreg rm to use preindex by rm, or extimm for full imm

    gip_native_ins_subclass_branch_no_delay = 0, // bit 11 of 0-11
    gip_native_ins_subclass_branch_delay = 1,

} t_gip_native_ins_subclass;

/*t t_gip_fetch_op
 */
typedef enum [2]
{
    gip_fetch_op_this_prefetch,
    gip_fetch_op_last_prefetch,
    gip_fetch_op_sequential,
    gip_fetch_op_hold
} t_gip_fetch_op;

/*t t_gip_prefetch_op
 */
typedef enum [2]
{
    gip_prefetch_op_none, // If nothing scheduled, for example
    gip_prefetch_op_new_address,
    gip_prefetch_op_hold, // Last address was okay
    gip_prefetch_op_sequential
} t_gip_prefetch_op;

/*t t_gip_mem_op
 */
typedef enum [3]
{
    gip_mem_op_none,
    gip_mem_op_store_word,
    gip_mem_op_store_half,
    gip_mem_op_store_byte,
    gip_mem_op_load_word,
    gip_mem_op_load_half,
    gip_mem_op_load_byte
} t_gip_mem_op;


/*t t_gip_mem_options
 */
typedef enum[3]
{
    gip_mem_options_none = 0,
    gip_mem_options_signed = 1,
    gip_mem_options_stack = 2,
    gip_mem_options_bigendian = 4
} t_gip_mem_options;

/*a Core module
 */
/*m Module
 */
extern module gip_core( clock gip_clock,
                        input bit gip_reset,

                        output bit fetch_16,
                        output t_gip_fetch_op fetch_op, // Early in the cycle, so data may be returned combinatorially
                        input t_gip_word fetch_data,
                        input bit fetch_data_valid,
                        input bit[32] fetch_pc,
                        output t_gip_prefetch_op prefetch_op, // Late in the cycle; can be used to start an SRAM cycle in the clock edge for fetch data in next cycle (if next cycle fetch requests it)
                        output bit[32] prefetch_address,
                        output bit fetch_flush,

                        output bit rfr_periph_read,
                        output bit[5] rfr_periph_read_address,
                        input bit rfr_periph_read_data_valid,
                        input bit[32] rfr_periph_read_data,
                        input bit rfr_periph_busy,
                        output bit rfw_periph_write,
                        output bit[5] rfw_periph_write_address,
                        output bit[32] rfw_periph_write_data,
                        output bit gip_pipeline_flush,

                        output t_gip_mem_op alu_mem_op,
                        output t_gip_mem_options alu_mem_options,
                        output t_gip_word alu_mem_address,
                        output t_gip_word alu_mem_write_data,
                        output bit[4] alu_mem_burst,

                        input bit mem_alu_busy,
                        input bit mem_read_data_valid,
                        input bit[32] mem_read_data,

                        input bit[8] local_events_in,

                        output t_postbus_type postbus_tx_type,
                        output t_postbus_data postbus_tx_data,
                        input t_postbus_ack postbus_tx_ack,

                        input t_postbus_type postbus_rx_type,
                        input t_postbus_data postbus_rx_data,
                        output t_postbus_ack postbus_rx_ack

    )
{
    timing from rising clock gip_clock fetch_16, fetch_op;
    timing to rising clock gip_clock fetch_data, fetch_data_valid, fetch_pc;
    timing from rising clock gip_clock prefetch_op, prefetch_address;
    timing from rising clock gip_clock fetch_flush;

    timing from rising clock gip_clock alu_mem_op, alu_mem_address, alu_mem_write_data, alu_mem_burst;
    timing to rising clock gip_clock mem_alu_busy, mem_read_data_valid, mem_read_data;

    timing comb input mem_alu_busy;
    timing comb output fetch_op, prefetch_op;

    timing to rising clock gip_clock local_events_in;

    timing from rising clock gip_clock postbus_tx_type, postbus_tx_data;
    timing to rising clock gip_clock postbus_tx_ack;

    timing from rising clock gip_clock postbus_rx_ack;
    timing to rising clock gip_clock postbus_rx_type, postbus_rx_data;
}

