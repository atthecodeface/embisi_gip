/*a Includes
 */

/*a Types
 */
/*t t_arm_shf
 */
typedef enum [2]
{
    arm_shf_lsl = 0,
    arm_shf_lsr = 1,
    arm_shf_asr = 2,
    arm_shf_ror = 3
} t_arm_shf;

/*t t_arm_ins_class
 */
typedef enum [4]
{
    arm_ins_class_none,
    arm_ins_class_alu,
    arm_ins_class_branch,
    arm_ins_class_load,
    arm_ins_class_store,
    arm_ins_class_ldm,
    arm_ins_class_stm,
    arm_ins_class_mul,
    arm_ins_class_native,
} t_arm_ins_class;

