/*a Includes
 */
include "postbus.h"

/*a Types
 */

/*t t_gip_shift_op
 */
typedef enum [3]
{
    gip_shift_op_lsl=0,
    gip_shift_op_lsr=1,
    gip_shift_op_asr=2,
    gip_shift_op_ror=3,
    gip_shift_op_rrx=4,
} t_gip_shift_op;

/*t t_gip_rf_result
 */
typedef struct
{
    bit write_pc;
    t_gip_word rfw_data;
} t_gip_rf_result;

/*t t_gip_ins_r_int
 */
typedef enum [5]
{
    gip_ins_rd_int_eq = 0,
    gip_ins_rd_int_ne = 1,
    gip_ins_rd_int_cs = 2,
    gip_ins_rd_int_cc = 3,
    gip_ins_rd_int_hi = 4,
    gip_ins_rd_int_ls = 5,
    gip_ins_rd_int_ge = 6,
    gip_ins_rd_int_lt = 7,
    gip_ins_rd_int_gt = 8,
    gip_ins_rd_int_le = 9,
    gip_ins_r_int_pc = 16,
    gip_ins_rnm_int_block_all = 20, // Value of acc, but blocks if any register write is pending at all (to make it safe for deschedules)
    gip_ins_rnm_int_shf = 24,
    gip_ins_rnm_int_acc = 25,
} t_gip_ins_r_int;

/*t t_gip_ins_r_type - 3 bit field, 5 bits of sub-field defined below
 */
typedef enum [3]
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
typedef struct
{
    t_gip_ins_r_type type;
    bit[5] r;
} t_gip_ins_r;

/*t t_gip_logic_op - these are hand coded to match the decode; do not change!
 */
typedef enum [4]
{
    gip_logic_op_and=0,
    gip_logic_op_or=1,
    gip_logic_op_xor=2,
    gip_logic_op_orn=3,
    gip_logic_op_bic=4,
    gip_logic_op_mov=5,
    gip_logic_op_mvn=6,
    gip_logic_op_and_cnt=7,
    gip_logic_op_and_xor=8,
    gip_logic_op_xor_first=9,
    gip_logic_op_bit_reverse=11,
    gip_logic_op_byte_reverse=12,
} t_gip_logic_op;

/*t t_gip_arith_op - these are hand coded to match the decode; do not change!
 */
typedef enum [4]
{
    gip_arith_op_add=0,
    gip_arith_op_adc=1,
    gip_arith_op_sub=2,
    gip_arith_op_sbc=3,
    gip_arith_op_rsub=4,
    gip_arith_op_rsbc=5,
    gip_arith_op_init=6,
    gip_arith_op_mla=7,
    gip_arith_op_mlb=8,
} t_gip_arith_op;

/*t t_gip_ins_class
 */
typedef enum [3]
{
    gip_ins_class_arith = 0,
    gip_ins_class_logic = 1,
    gip_ins_class_shift = 2,
    gip_ins_class_load = 4,
    gip_ins_class_store = 5,
} t_gip_ins_class;

/*t t_gip_ins_subclass
 */
typedef enum [4]
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

/*t t_gip_ins_cc
 */
typedef enum [4]
{
    gip_ins_cc_eq = 0,
    gip_ins_cc_ne = 1,
    gip_ins_cc_cs = 2,
    gip_ins_cc_cc = 3,
    gip_ins_cc_mi = 4,
    gip_ins_cc_pl = 5,
    gip_ins_cc_vs = 6,
    gip_ins_cc_vc = 7,
    gip_ins_cc_hi = 8,
    gip_ins_cc_ls = 9,
    gip_ins_cc_ge = 10,
    gip_ins_cc_lt = 11,
    gip_ins_cc_gt = 12,
    gip_ins_cc_le = 13,
    gip_ins_cc_always = 14,
    gip_ins_cc_cp = 15,
} t_gip_ins_cc;

/*t t_gip_instruction_rf
 */
typedef struct 
{
    bit valid;
    t_gip_ins_class gip_ins_class;
    t_gip_ins_subclass gip_ins_subclass;
    t_gip_ins_cc gip_ins_cc;
    t_gip_ins_r gip_ins_rd;
    t_gip_ins_r gip_ins_rn;
    t_gip_ins_r gip_ins_rm;
    bit rm_is_imm;
    t_gip_word immediate;
    bit[4] k; // Burst size remaining for load/store burst
    bit a; // set accumulator with ALU result
    bit f; // flush rest of pipeline if the condition succeeds
    bit s_or_stack; // 's' for ALU, COND, SHF; 'stack' hint for load/store
    bit p_or_offset_is_shift; // 'p' for ALU, COND, SHF; offset type (1 for SHF, 0 for access size) for stores
    bit d; // delay slot instruction - do not flush even if a flush is indicated
    bit[32] pc; // PC value for the instruction - for ARM emulation, PC+8 generally
    bit[2] tag; // decoder-custom tag (presented as the instruction is executed in the pipeline results)
} t_gip_instruction_rf;

/*t t_gip_ext_cmd
 */
typedef struct
{
    bit extended;
    t_gip_ins_cc cc;
    bit sign_or_stack;
    bit acc;
    bit[2] op;
    bit[4] burst;
} t_gip_ext_cmd;

/*t t_gip_alu_op1_src
 */
typedef enum [2]
{
    gip_alu_op1_src_a_in,
    gip_alu_op1_src_acc,
} t_gip_alu_op1_src;

/*t t_gip_alu_op2_src
 */
typedef enum [2]
{
    gip_alu_op2_src_b_in,
    gip_alu_op2_src_acc,
    gip_alu_op2_src_shf,
    gip_alu_op2_src_constant
} t_gip_alu_op2_src;

/*t t_gip_pc_op
 */
typedef enum [2]
{
    gip_pc_op_hold,
    gip_pc_op_sequential,
    gip_pc_op_delayed_branch,
    gip_pc_op_branch,
} t_gip_pc_op;

/*a Submodules
 */
/*m gip_rf
 */
extern module gip_rf( clock gip_clock,
                      input bit gip_reset,

                      input t_gip_instruction_rf dec_inst,
                      output t_gip_instruction_rf rfr_inst,

                      output bit rfr_accepting_dec_instruction_always,
                      output bit rfr_accepting_dec_instruction_if_alu_does,
                      input bit rfr_accepting_dec_instruction,

                      output bit rfr_postbus_read,
                      output bit[5] rfr_postbus_read_address,
                      input t_gip_word rfr_postbus_read_data,

                      output bit rfr_special_read,
                      output bit[5] rfr_special_read_address,
                      input t_gip_word rfr_special_read_data,

                      output bit rfr_periph_read,
                      output bit[5] rfr_periph_read_address,
                      input bit rfr_periph_read_data_valid,
                      input bit[32] rfr_periph_read_data,

                      input bit rfr_periph_busy,

                      output t_gip_word rfr_port_0,
                      output t_gip_word rfr_port_1,

                      input bit alu_inst_valid, // ALU state
                      input t_gip_ins_r alu_inst_gip_ins_rd,

                      input t_gip_ins_r alu_rd,
                      input bit alu_use_shifter,
                      input t_gip_word alu_arith_logic_result,
                      input t_gip_word alu_shifter_result,

                      input t_gip_ins_r mem_1_rd,
                      input t_gip_ins_r mem_2_rd,
                      input bit mem_read_data_valid,
                      input t_gip_word mem_read_data,

                      output bit rfw_postbus_write,
                      output bit[5] rfw_postbus_write_address,
                      output bit rfw_special_write,
                      output bit[5] rfw_special_write_address,

                      output bit rfw_periph_write,
                      output bit[5] rfw_periph_write_address,

                      input bit gip_pipeline_flush,

                      output bit rfw_accepting_alu_rd,
                      output bit gip_pipeline_rfw_write_pc,
                      output bit[32] gip_pipeline_rfw_data
    )
{
    timing to rising clock gip_clock gip_reset;

    timing to rising clock gip_clock dec_inst;
    timing from rising clock gip_clock rfr_inst;

    timing from rising clock gip_clock rfr_accepting_dec_instruction_always, rfr_accepting_dec_instruction_if_alu_does;

    timing from rising clock gip_clock rfr_postbus_read, rfr_postbus_read_address;
    timing to rising clock gip_clock rfr_postbus_read_data;
    timing from rising clock gip_clock rfr_special_read, rfr_special_read_address;
    timing to rising clock gip_clock rfr_special_read_data;

    timing from rising clock gip_clock rfr_port_0, rfr_port_1;

    timing to rising clock gip_clock alu_inst_valid, alu_inst_gip_ins_rd;

    timing to rising clock gip_clock rfr_accepting_dec_instruction;
    timing to rising clock gip_clock alu_rd, alu_use_shifter, alu_arith_logic_result, alu_shifter_result;

    timing to rising clock gip_clock mem_1_rd, mem_2_rd, mem_read_data, mem_read_data_valid;

    timing comb input alu_inst_valid, alu_inst_gip_ins_rd;
    timing comb output rfr_accepting_dec_instruction_if_alu_does;
    timing comb output rfr_inst; // .valid depends on blocking

    timing from rising clock gip_clock rfw_postbus_write, rfw_postbus_write_address;
    timing from rising clock gip_clock rfw_special_write, rfw_special_write_address;

    timing to rising clock gip_clock gip_pipeline_flush;

    timing from rising clock gip_clock rfw_accepting_alu_rd;
    timing from rising clock gip_clock gip_pipeline_rfw_write_pc, gip_pipeline_rfw_data;
    
}

/*m gip_alu
 */
extern module gip_alu( clock gip_clock,
                       input bit gip_reset,

                       input t_gip_instruction_rf rfr_inst,
                       input t_gip_word rf_read_port_0,
                       input t_gip_word rf_read_port_1,

                       output t_gip_instruction_rf alu_inst,

                       input bit rfw_accepting_alu_rd,
                       input bit mem_alu_busy,
                       input bit alu_accepting_rfr_instruction,
                       output bit alu_accepting_rfr_instruction_always,
                       output bit alu_accepting_rfr_instruction_if_mem_does,
                       output bit alu_accepting_rfr_instruction_if_rfw_does,

                       output t_gip_ins_r alu_rd,
                       output bit alu_use_shifter,
                       output t_gip_word alu_arith_logic_result,
                       output t_gip_word alu_shifter_result,

                       output t_gip_mem_op alu_mem_op,
                       output t_gip_ins_r alu_mem_rd,
                       output t_gip_word alu_mem_address,
                       output t_gip_word alu_mem_write_data,
                       output bit[4] alu_mem_burst,

                       input bit special_cp_trail_2,

                       output bit gip_pipeline_flush,
                       output bit[2] gip_pipeline_tag,
                       output bit gip_pipeline_executing
    )
{
    timing to rising clock gip_clock gip_reset;

    timing to rising clock gip_clock rfr_inst, rf_read_port_0, rf_read_port_1;
    timing from rising clock gip_clock alu_inst;

    timing to rising clock gip_clock rfw_accepting_alu_rd, mem_alu_busy;
    timing to rising clock gip_clock alu_accepting_rfr_instruction;
    timing from rising clock gip_clock alu_accepting_rfr_instruction_always, alu_accepting_rfr_instruction_if_mem_does, alu_accepting_rfr_instruction_if_rfw_does;

    timing from rising clock gip_clock alu_rd, alu_use_shifter, alu_arith_logic_result, alu_shifter_result;

    timing from rising clock gip_clock alu_mem_op, alu_mem_rd, alu_mem_address, alu_mem_write_data, alu_mem_burst;

    timing to rising clock gip_clock special_cp_trail_2;

    timing from rising clock gip_clock gip_pipeline_flush, gip_pipeline_tag, gip_pipeline_executing;
    
}

/*m gip_memory
 */
extern module gip_memory( clock gip_clock,
                   input bit gip_reset,

                   input t_gip_ins_r alu_mem_rd,
                   input bit mem_alu_busy,

                   output t_gip_ins_r mem_1_rd,
                   output t_gip_ins_r mem_2_rd,
                   input bit mem_read_data_valid
    )
{
    timing to rising clock gip_clock gip_reset;

    timing to rising clock gip_clock alu_mem_rd, mem_alu_busy;
    timing from rising clock gip_clock mem_1_rd, mem_2_rd;
    timing to rising clock gip_clock mem_read_data_valid;
}

/*m gip_decode
 */
extern module gip_decode( clock gip_clock,
                          input bit gip_reset,

                          output bit fetch_16,
                          output t_gip_fetch_op fetch_op, // Early in the cycle, so data may be returned combinatorially
                          input t_gip_word fetch_data,
                          input bit fetch_data_valid,
                          input bit[32] fetch_pc,
                          output t_gip_prefetch_op prefetch_op, // Late in the cycle; can be used to start an SRAM cycle in the clock edge for fetch data in next cycle (if next cycle fetch requests it)
                          output bit[32] prefetch_address,

                          output t_gip_instruction_rf dec_inst,
                          input bit rfr_accepting_dec_instruction,

                          input bit gip_pipeline_flush,
                          input bit gip_pipeline_rfw_write_pc,
                          input bit gip_pipeline_executing,
                          input bit[2] gip_pipeline_tag,
                          input t_gip_word gip_pipeline_rfw_data,

                          input bit sched_thread_to_start_valid,
                          input bit[3] sched_thread_to_start,
                          input bit[32] sched_thread_to_start_pc,
                          output bit acknowledge_scheduler,

                          input bit[8] special_repeat_count,
                          input bit[2] special_alu_mode,
                          input bit special_cp_trail_2

    )
{
    timing to rising clock gip_clock gip_reset;
    timing comb input gip_reset; // Use to take out prefetch_op

    timing from rising clock gip_clock fetch_op;
    timing to rising clock gip_clock fetch_data, fetch_data_valid;
    timing from rising clock gip_clock prefetch_op, prefetch_address;

    timing to rising clock gip_clock rfr_accepting_dec_instruction;
    timing comb input rfr_accepting_dec_instruction, gip_pipeline_flush;
    timing comb output fetch_op, prefetch_op;

    timing to rising clock gip_clock gip_pipeline_flush, gip_pipeline_rfw_write_pc, gip_pipeline_executing, gip_pipeline_tag, gip_pipeline_rfw_data;

    timing to rising clock gip_clock sched_thread_to_start_valid, sched_thread_to_start, sched_thread_to_start_pc;
    timing from rising clock gip_clock acknowledge_scheduler;

    timing to rising clock gip_clock special_repeat_count, special_alu_mode, special_cp_trail_2;
}

/*m gip_special
 */
extern module gip_special( clock gip_clock,
                           input bit gip_reset,

                           input bit read,
                           input bit[5] read_address,
                           output bit[32] read_data,
                           input bit write,
                           input bit[5] write_address,
                           input bit[32] write_data,

                           input bit[3] sched_state_thread,
                           input bit[4] sched_thread_data_config,
                           input bit[32] sched_thread_data_pc,

                           input bit[5] postbus_semaphore_to_set "Semaphore to set due to postbus receive/transmit - none if zero",

                           output bit[8] special_repeat_count,
                           output bit[2] special_alu_mode,
                           output bit special_cp_trail_2,
                           output bit[32] special_semaphores,
                           output bit special_cooperative,
                           output bit special_round_robin,
                           output bit special_thread_data_write_pc,
                           output bit special_thread_data_write_config,
                           output bit[3] special_write_thread,
                           output bit[32] special_thread_data_pc,
                           output bit[4] special_thread_data_config,
                           output bit[3] special_selected_thread

    )
{
    timing to rising clock gip_clock gip_reset;

    timing to rising clock gip_clock read, read_address;
    timing comb input read, read_address;
    timing from rising clock gip_clock read_data;
    timing comb output read_data;

    timing to rising clock gip_clock write, write_address, write_data;

    timing to rising clock gip_clock sched_state_thread, sched_thread_data_config, sched_thread_data_pc;

    timing to rising clock gip_clock postbus_semaphore_to_set;

    timing from rising clock gip_clock special_repeat_count, special_alu_mode, special_cp_trail_2;
    timing from rising clock gip_clock special_semaphores, special_cooperative, special_round_robin;
    timing from rising clock gip_clock special_thread_data_write_pc, special_thread_data_write_config, special_write_thread, special_thread_data_pc, special_thread_data_config, special_selected_thread;
    
}

/*m gip_postbus
 */
extern module gip_postbus( clock gip_clock,
                           input bit gip_reset,

                           input bit read,
                           input bit flush,
                           input bit[5] read_address,
                           output bit[32] read_data,

                           input bit write,
                           input bit[5] write_address,
                           input bit[32] write_data,

                           output t_postbus_type postbus_tx_type,
                           output t_postbus_data postbus_tx_data,
                           input t_postbus_ack postbus_tx_ack,

                           input t_postbus_type postbus_rx_type,
                           input t_postbus_data postbus_rx_data,
                           output t_postbus_ack postbus_rx_ack,

                           output bit[5] semaphore_to_set "Semaphore to set due to postbus receive/transmit - none if zero"

    )
{
    timing to rising clock gip_clock gip_reset;

    timing to rising clock gip_clock read, flush, read_address;
    timing comb input read, read_address;
    timing from rising clock gip_clock read_data;
    timing comb output read_data;

    timing to rising clock gip_clock write, write_address, write_data;

    timing from rising clock gip_clock postbus_tx_type, postbus_tx_data;
    timing to rising clock gip_clock postbus_tx_ack;

    timing from rising clock gip_clock postbus_rx_ack;
    timing to rising clock gip_clock postbus_rx_type, postbus_rx_data;

    timing from rising clock gip_clock semaphore_to_set;
}

/*m gip_scheduler
 */
extern module gip_scheduler( clock gip_clock,
                             input bit gip_reset,

                             input bit dec_acknowledge_scheduler,
                             input bit[32] special_semaphores,
                             input bit special_cooperative,
                             input bit special_round_robin,
                             input bit special_thread_data_write_pc,
                             input bit special_thread_data_write_config,
                             input bit[3] special_write_thread,
                             input bit[32] special_thread_data_pc,
                             input bit[4] special_thread_data_config,
                             input bit[3] special_selected_thread,

                             output bit thread_to_start_valid,
                             output bit[3] thread_to_start,
                             output bit[32] thread_to_start_pc,
                             output bit[4] thread_to_start_config,
                             output bit[2] thread_to_start_level,

                             output bit[3] thread,
                             output bit[32] thread_data_pc "For reading in the special registers",
                             output bit[4] thread_data_config "For reading in the special registers"
    )
{
    timing to rising clock gip_clock gip_reset;

    timing to rising clock gip_clock dec_acknowledge_scheduler;
    timing to rising clock gip_clock special_semaphores, special_cooperative, special_round_robin;
    timing to rising clock gip_clock special_thread_data_write_pc, special_thread_data_write_config, special_write_thread, special_thread_data_pc, special_thread_data_config, special_selected_thread;

    timing from rising clock gip_clock thread_to_start_valid, thread_to_start, thread_to_start_pc, thread_to_start_config, thread_to_start_level;

    timing from rising clock gip_clock thread, thread_data_pc, thread_data_config;
    
}

/*a External modules
 */
extern module rf_2r_1w_32_32( clock rf_clock,
                             input bit rf_reset,
                             input bit[5] rf_rd_addr_0,
                             output bit[32] rf_rd_data_0,
                             input bit[5] rf_rd_addr_1,
                             output bit[32] rf_rd_data_1,
                             input bit rf_wr_enable,
                             input bit[5] rf_wr_addr,
                             input bit[32] rf_wr_data )
{
    timing comb input rf_rd_addr_0, rf_rd_addr_1;
    timing comb output rf_rd_data_0, rf_rd_data_1;
    timing to rising clock rf_clock rf_reset;
    timing to rising clock rf_clock rf_wr_enable, rf_wr_addr, rf_wr_data;
    timing from rising clock rf_clock rf_rd_data_0, rf_rd_data_1;
}

extern module rf_1r_1w_32_32( clock rf_clock,
                             input bit rf_reset,
                             input bit[5] rf_rd_addr_0,
                             output bit[32] rf_rd_data_0,
                             input bit rf_wr_enable,
                             input bit[5] rf_wr_addr,
                             input bit[32] rf_wr_data )
{
    timing comb input rf_rd_addr_0;
    timing comb output rf_rd_data_0;
    timing to rising clock rf_clock rf_reset;
    timing to rising clock rf_clock rf_wr_enable, rf_wr_addr, rf_wr_data;
    timing from rising clock rf_clock rf_rd_data_0;
}

