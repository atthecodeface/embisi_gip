/*a Module declarations
 */
/*m gip_alu_barrel_shift
 */
extern module gip_alu_barrel_shift( input bit carry_in,
                                    input t_gip_shift_op gip_shift_op,
                                    input t_gip_word value,
                                    input bit[8] amount,
                                    output t_gip_word result,
                                    output bit carry_out )
{
    timing comb input carry_in, gip_shift_op, value, amount;
    timing comb output result, carry_out;
}

/*m gip_alu_logical_op
 */
extern module gip_alu_logical_op( input t_gip_word op_a,
                                  input t_gip_word op_b,
                                  input t_gip_logic_op logic_op,
                                  output t_gip_word result,
                                  output bit z,
                                  output bit n )
{
    timing comb input op_a, op_b, logic_op;
    timing comb output result, z, n;
}

/*m gip_alu_arith_op
 */
extern module gip_alu_arith_op( input t_gip_word op_a,
                         input t_gip_word op_b,
                         input bit c_in,
                         input bit p_in,
                         input bit[2] shf,
                         input t_gip_arith_op arith_op,
                         output t_gip_word result,
                         output bit z,
                         output bit n,
                         output bit v,
                         output bit c,
                         output bit shf_carry_override,
                         output bit shf_carry_out )
{
    timing comb input op_a, op_b, c_in, p_in, shf, arith_op;
    timing comb output result, z, n, v, c;
    timing comb output shf_carry_override, shf_carry_out;
}
