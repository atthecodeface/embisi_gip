/*a Copyright Gavin J Stark and John Croft, 2003
 */

/*a Wrapper
 */
#ifndef GIP_NATIVE
#define GIP_NATIVE

/*a Includes
 */

/*a Types
 */
/*t t_gip_native_ins_class
 */
typedef enum
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

} t_gip_native_ins_class;

/*t t_gip_native_ins_subclass
 */
typedef enum
{
    gip_native_ins_subclass_alu_and=0, // bits 8-11 of 0-11
    gip_native_ins_subclass_alu_or=1,
    gip_native_ins_subclass_alu_xor=2,
    gip_native_ins_subclass_alu_orn=3,
    gip_native_ins_subclass_alu_bic=4,
    gip_native_ins_subclass_alu_mov=5,
    gip_native_ins_subclass_alu_mvn=6,
    gip_native_ins_subclass_alu_add=7,
    gip_native_ins_subclass_alu_sub=8,
    gip_native_ins_subclass_alu_rsb=9,
    gip_native_ins_subclass_alu_init=10,
    gip_native_ins_subclass_alu_mla=11,
    gip_native_ins_subclass_alu_mlb=12,
    gip_native_ins_subclass_alu_adc=13,
    gip_native_ins_subclass_alu_sbc=14,
    gip_native_ins_subclass_alu_rsc=15,
    gip_native_ins_subclass_alu_dva=16, // here on down not possible without extension
    gip_native_ins_subclass_alu_dvb=17,
    gip_native_ins_subclass_alu_andcnt=18,
    gip_native_ins_subclass_alu_xorfirst=19,
    gip_native_ins_subclass_alu_xorlast=20,
    gip_native_ins_subclass_alu_bitreverse=21,
    gip_native_ins_subclass_alu_bytereverse=22,

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
    gip_native_ins_subclass_cond_mi=10, // bits 8-11 of 0-11, maps to ALU op sub, cond mi/n set
    gip_native_ins_subclass_cond_pl=11, // bits 8-11 of 0-11, maps to ALU op sub, cond pl/n clr
    gip_native_ins_subclass_cond_vs=12, // bits 8-11 of 0-11, maps to ALU op sub, cond vs/v set
    gip_native_ins_subclass_cond_vc=13, // bits 8-11 of 0-11, maps to ALU op sub, cond vc/v clr
    gip_native_ins_subclass_cond_anyclr=14, // bits 8-11 of 0-11, maps to ALU op XOR, cond result!=op2
    gip_native_ins_subclass_cond_allset=15, // bits 8-11 of 0-11, maps to ALU op XOR, cond result==op2
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
    gip_native_ins_subclass_cond_sps=30, // bits 8-11 of 0-11, maps to no op, cond P set
    gip_native_ins_subclass_cond_spc=31, // bits 8-11 of 0-11, maps to no op, cond P set

    gip_native_ins_subclass_shift_lsl=0, // bits 10-11 of 0-11
    gip_native_ins_subclass_shift_lsr=1,
    gip_native_ins_subclass_shift_asr=2,
    gip_native_ins_subclass_shift_ror=3,
    gip_native_ins_subclass_shift_ror33=4, // possible only through imm ror of 0

    gip_native_ins_subclass_memory_word_noindex = 0, // bits 9-11 of 0-11; extcmd may provide burst and pre/postindex byte/word
    gip_native_ins_subclass_memory_half_noindex = 1,
    gip_native_ins_subclass_memory_byte_noindex = 2,
    gip_native_ins_subclass_memory_word_preindex_up_wb = 3, // store only
    gip_native_ins_subclass_memory_word_preindex_up = 4, // ldr uses extreg rm to use preindex by rm, or extimm for full imm
    gip_native_ins_subclass_memory_word_preindex_down = 5, // ldr uses extreg rm to use preindex by rm, or extimm for full imm
    gip_native_ins_subclass_memory_word_postindex_up = 6, // store only writebacks rn as rd by default; ldr uses extreg rm to use preindex by rm, or extimm for full imm
    gip_native_ins_subclass_memory_word_postindex_down = 7, // store only writebacks rn as rd by default; ldr uses extreg rm to use preindex by rm, or extimm for full imm

    gip_native_ins_subclass_branch_no_delay = 0, // bit 11 of 0-11
    gip_native_ins_subclass_branch_delay = 1,

} t_gip_native_ins_subclass;

/*a Wrapper
 */
#endif
