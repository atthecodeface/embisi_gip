extern module gip_decode_native( input bit[16] opcode,

                                 input bit[8] special_repeat_count,
                                 input bit[2] special_alu_mode,

                                 output t_gip_instruction_rf inst,
                                 output t_gip_pc_op pc_op,
                                 output bit[32] native_mapped_branch_offset,

                                 input bit extended,
                                 input bit[28] extended_immediate,
                                 input t_gip_ins_r extended_rd,
                                 input t_gip_ins_r extended_rn,
                                 input t_gip_ins_r extended_rm,
                                 input t_gip_ext_cmd extended_cmd,

                                 output bit extending,
                                 output bit[28] next_extended_immediate,
                                 output t_gip_ins_r next_extended_rd,
                                 output t_gip_ins_r next_extended_rn,
                                 output t_gip_ins_r next_extended_rm,
                                 output t_gip_ext_cmd next_extended_cmd,

                                 input bit in_conditional_shadow,
                                 input bit in_immediate_conditional_shadow,
                                 output bit next_in_immediate_conditional_shadow

    )
{
    timing comb input opcode;
    timing comb input special_repeat_count, special_alu_mode;
    timing comb output inst, pc_op, native_mapped_branch_offset;

    timing comb input extended, extended_immediate, extended_rd, extended_rn, extended_rm, extended_cmd;
    timing comb output next_extended_immediate, next_extended_rd, next_extended_rn, next_extended_rm, next_extended_cmd;

    timing comb input in_conditional_shadow, in_immediate_conditional_shadow;
    timing comb output next_in_immediate_conditional_shadow;
}

extern module gip_decode_arm( input bit[32] opcode,
                       input bit[5] cycle_of_opcode,

                       output t_gip_instruction_rf inst,

                       output bit[5] next_cycle_of_opcode,
                       output t_gip_pc_op pc_op,
                       output bit[32] arm_branch_offset,

                       input bit extended,
                       input bit[28] extended_immediate,
                       input t_gip_ins_r extended_rd,
                       input t_gip_ins_r extended_rn,
                       input t_gip_ins_r extended_rm,
                       input t_gip_ext_cmd extended_cmd

                       )
{
    timing comb input opcode;

    timing comb output inst, next_cycle_of_opcode, pc_op, arm_branch_offset;

    timing comb input extended, extended_immediate, extended_rd, extended_rn, extended_rm, extended_cmd;

}
