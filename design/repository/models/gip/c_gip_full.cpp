/*a To do
  Hold of writing ALU acc and flags and shf and A/B (for mults) if RF will not take its result - treat it as not condition passed?

  Add the register file write buffer (single reg number, single data) for peripheral accesses.
  Reading of peripherals in RFR stage
  Writing of peripherals in RFW stage

  Checking instruction flow is okay and flush never occurs after write of PC - adding 'd' flag to instructions will ensure this

  Zero overhead loops - either with special register as count, or immediate count (which should be faster to use)
  For ZOLs use the repeat count above, and also have internal registers for the start and lengths

  Mapping of registers with thread-based map

  Preemption
  Deschedule instruction

  Thunking library call instruction
  SWIs

  Native instruction access in ARM mode - with conditional access (break out that coproc space?)
  Add in saturated N/P and saturate instructions - how to clear them? No need for sticky V or N I think

  Note: need to resolve the mechanism for one-shot scheduling?

  Note: the way the 'D' flag is implemented for delay slots, the native decode requires a 'D' instruction to immediately succeed an 'F' instruction, with NO intervening cycles; this means that a conditional branch with delay slot must be placed so that the prefetch logic is GUARANTEED to provide the succeeding instruction on the succeeding cycle.
 */

/*a Includes
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "c_gip_full.h"
#include "c_memory_model.h"
#include "symbols.h"
#include "syscalls.h"
#include "arm_dis.h"
#include "gip_internals.h"
#include "gip_instructions.h"
#include "gip_native.h"

/*a Defines
 */
#define SET_ZN(pd, res) {pd->z=(res==0);pd->n=(res&0x80000000)?1:0;}
#define SET_ZNC(pd, res, car) {pd->z=(res==0);pd->n=(res&0x80000000)?1:0;pd->c=car;}
#define SET_ZNCV(pd, res, car, ovr) {pd->z=(res==0);pd->n=(res&0x80000000)?1:0;pd->c=car;pd->v=ovr;}
#define MAX_BREAKPOINTS (8)
#define MAX_INTERRUPTS (8)
#define SET_DEBUGGING_ENABLED(pd) {pd->debugging_enabled = (pd->breakpoints_enabled != 0) || (pd->halt);}
#define MAX_REGISTERS (32)
#define GIP_REGISTER_MASK (0x1f)
#define GIP_REGISTER_TYPE (0x20)
#define NUM_THREADS (8)

/*a Types
 */
/*t t_shf_type
 */
typedef enum
{
    shf_type_lsl=0,
    shf_type_lsr=1,
    shf_type_asr=2,
    shf_type_ror=3,
    shf_type_rrx=4,
} t_shf_type;

/*t t_gip_dec_op_state
 */
typedef enum t_gip_dec_op_state
{
    gip_dec_op_state_idle,
    gip_dec_op_state_native,
    gip_dec_op_state_emulate,
    gip_dec_op_state_preempt
} t_gip_dec_op_state;

/*t t_gip_dec_extcmd
 */
typedef struct t_gip_dec_extcmd
{
    int extended; // zero if rest of fields are not valid (no extcmd instruction)
    int cc; // 4 bits of condition
    int sign_or_stack; // 1 bit, set sign or 'is stack' for loads
    int acc; // 1 bit, set accumulator
    int op; // 2 bits of operation extension
    int burst; // 4 bits of burst size
} t_gip_dec_extcmd;

/*t t_gip_sched_thread - Per thread register file storage
 */
typedef struct t_gip_sched_thread
{
    unsigned int restart_pc;
    int flag_dependencies; // 4-bit field indicating which sempahores are important
    int register_map;
    int config; // 1 for ARM, 0 for GIP native
    int running; // 1 if the thread has been scheduled and is running (or is currently preempted)
} t_gip_sched_thread;

/*t t_gip_sched_state
 */
typedef struct t_gip_sched_state
{

    /*b State in the scheduler guaranteed by the design
     */
    t_gip_sched_thread thread_data[NUM_THREADS]; // 'Register file' of thread data for all eight threads
    int thread; // Actual thread number that is currently running
    int running; // 1 if a thread is running, 0 otherwise
    int preempted_medium_thread; // 0 if none, else number
    int preempted_low_thread; // 0 if not, else 1

    int thread_to_start_valid; // 1 if the bus contents are valid; bus contents may only change when this signal rises
    int thread_to_start; // Thread number to start
    int thread_to_start_pc; // Thread number to start
    int thread_to_start_config; // GIP pipeline configuration including operating mode
    int thread_to_start_level; // 0 for low, 1 for medium, 2 for high
} t_gip_sched_state;

/*t t_gip_sched_data
 */
typedef struct t_gip_sched_data
{
    /*b State of the scheduler
     */
    t_gip_sched_state state;
    t_gip_sched_state next_state;

    /*b Combinatorials in the scheduler
     */
    int next_thread_to_start; // Next thread, presented to decoder
    int next_thread_to_start_valid; // 1 if next_thread_to_start is valid and indicates a thread to change to
    int thread_data_to_read; // Thread number to read data for; if scheduling, then the next thread, else the selected_thread register
    unsigned int thread_data_pc; // PC from the thread data bank for thread_data_to_read
    int thread_data_config; // Config from the thread data bank for thread_data_to_read

} t_gip_sched_data;

/*t t_gip_special_state
 */
typedef struct t_gip_special_state
{

    /*b State in the special registers guaranteed by the design
     */
    unsigned int semaphores;
    int cooperative;
    int round_robin;
    int thread_0_privilege;
    int selected_thread; // Thread to read or write
    int write_thread; // Thread to actually use for writing thread_data_pc/config
    int thread_data_write_pc; // 1 if the thread_data_pc should be written to write_thread
    int thread_data_write_config; // 1 if the thread_data_config should be written to write_thread
    unsigned int thread_data_pc;
    int thread_data_config;
    int repeat_count; // repeat count

} t_gip_special_state;

/*t t_gip_special_data
 */
typedef struct t_gip_special_data
{
    /*b State of the special registers
     */
    t_gip_special_state state;
    t_gip_special_state next_state;

    /*b Combinatorials in the special registers
     */
} t_gip_special_data;

/*t t_gip_dec_idle_data
 */
typedef struct t_gip_dec_idle_data
{
    int next_pc_valid;
    int next_pc;
    int next_thread;
} t_gip_dec_idle_data;

/*t t_gip_dec_native_data
 */
typedef struct t_gip_dec_native_data
{
    unsigned int next_pc;
    int next_cycle_of_opcode;
    int inst_valid; // If 1 then instruction is valid, else not
    t_gip_instruction inst; // Instruction to be executed by the RF stage

    int next_instruction_extended;
    unsigned int next_extended_immediate;
    t_gip_ins_r next_extended_rd; // if extended instruction this specifies rd; if none, then do not extend rd
    t_gip_ins_r next_extended_rm; // if extended instruction this specifies rm; if none, then do not extend rm
    t_gip_ins_r next_extended_rn; // if extended instruction this specifies rn; if none, then do not extend rn
    t_gip_dec_extcmd next_extended_cmd; // 6-bit field of extension bits for a command - needs to cover burst size (k), and whether to increment register

    int extending; // 1 if the current native decode is an extension command; if this is not set and an instruction is being decoded then the next extension values should be cleared

    int next_in_delay_slot; // 1 if in will be in a delay slot; ignore if flushed
    int next_follow_delay_pc; // 1 if the next instruction will be the delay slot for a known taken branch
    unsigned int next_delay_pc; // if it will be in a delay slot next as part of a know taken branch, then this is the PC that should be fetched for after that delay slot; otherwise, ignore

    int next_in_immediate_shadow;

} t_gip_dec_native_data;

/*t t_gip_dec_arm_data
 */
typedef struct t_gip_dec_arm_data
{
    unsigned int next_pc;
    int next_cycle_of_opcode;
    int next_acc_valid;
    int next_reg_in_acc;

    int inst_valid; // If 1 then instruction is valid, else not
    t_gip_instruction inst; // Instruction to be executed by the RF stage

} t_gip_dec_arm_data;

/*t t_gip_dec_state
 */
typedef struct t_gip_dec_state
{
    /*b State in the decoder stage guaranteed by the design
     */
    t_gip_dec_op_state op_state;
    unsigned int opcode;
    unsigned int pc;
    int in_delay_slot; // 1 if in a delay slot
    int follow_delay_pc; // 1 if delay_pc should be taken rather than what the instruction decodes to
    unsigned int delay_pc; // If in decoding a branch delay slot, then this is the next pc after the slot independent of what that instruction says
    int pc_valid;
    int acknowledge_scheduler;
    int cycle_of_opcode;
    int acc_valid; // 1 if the register in the accumulator is valid, 0 if the accumulator should not be used
    int reg_in_acc; // register number (0 through 14) that is in the accumulator; 15 means none.
    int fetch_requested; // 1 if at end of previous cycle a new instruction was stored in the opcode - basically comb cycle_of_opcode==0
    unsigned int extended_immediate; // 28-bit extended immediate value for native instructions (ORred in with ARM decode immediate also); if zero, no extension yet
    t_gip_ins_r extended_rd; // if extended instruction this specifies rd; if none, then do not extend rd
    t_gip_ins_r extended_rm; // if extended instruction this specifies rm; if none, then do not extend rm - note for loads this is the whole rm if extended, for others just top half of rm
    t_gip_ins_r extended_rn; // if extended instruction this specifies rn; if none, then do not extend rn
    t_gip_dec_extcmd extended_cmd; // Command extension
    int in_conditional_shadow; // 1 if in immediate shadow or (if shadow length of 2) if last accepted instruction was in immediate shadow
    int in_immediate_shadow;
} t_gip_dec_state;

/*t t_gip_dec_data
 */
typedef struct t_gip_dec_data
{
    /*b State in the RF guaranteed by the design
     */
    t_gip_dec_state state;
    t_gip_dec_state next_state;

    /*b Combinatorials in the decoder
     */
    int inst_valid; // If 1 then instruction is valid, else not
    t_gip_instruction inst; // Instruction to be executed by the RF stage
    t_gip_dec_op_state next_op_state;
    int next_acknowledge_scheduler; // 1 to acknowledge scheduler on next cycle
    t_gip_ins_cc gip_ins_cc;

    /*b Subelement
     */
    t_gip_dec_native_data native;
    t_gip_dec_arm_data arm;
    t_gip_dec_idle_data idle;

} t_gip_dec_data;

/*t t_gip_rf_state
 */
typedef struct t_gip_rf_state
{
    /*b State in the RF itself
     */
    unsigned int regs[ MAX_REGISTERS ];

    /*b State in the RF read stage guaranteed by the design
     */
    int inst_valid; // If 1 then instruction is valid, else not
    t_gip_instruction inst; // Instruction being executed by the RF stage

    /*b State in the RF write stage guaranteed by the design
     */
    t_gip_ins_r alu_rd;   // Type of register file write requested for the result of ALU operation
    int use_shifter; // If 1 and alu_rd indicates a write then use the shifter result not the ALU result
    unsigned int alu_result; // Registered result of the ALU
    unsigned int shf_result; // Registered result of the shifter
    t_gip_ins_r mem_rd;   // Type of register file write requested for the result of memory operation
    unsigned int mem_result; // Register result of the memory stage
    int accepting_alu_rd; // 1 if the RFW stage can take an ALU register write; 0 only if the mem_rd is not-none and the alu_rd is not-none
} t_gip_rf_state;

/*t t_gip_rf_data
 */
typedef struct t_gip_rf_data
{
    /*b State in the RF guaranteed by the design
     */
    t_gip_rf_state state;
    t_gip_rf_state next_state;

    /*b Combinatorials in the RF
     */
    unsigned int read_port_0, read_port_1; // Two register file read port values
    int special_read, special_write, special_read_address;
    unsigned int special_read_data;
    int postbus_read, postbus_write, postbus_read_address;
    unsigned int postbus_read_data, periph_read_data; // Read data from other places
    unsigned int port_0_data, port_1_data; // Outputs to ALU stage
    t_gip_ins_r rd;
    unsigned int rfw_data;

    int accepting_dec_instruction; // 1 if the RF stage is taking a proffered instruction from the decode independent of the ALU; will be asserted if the RF stage has no valid instruction to decode currently
    int accepting_dec_instruction_if_alu_does; // 1 if the RF stage is taking a proffered instruction from the decode depending on the ALU taking the current instruction; 0 if the RF has no valid instruction, or if it has an instruction blocking on a pending register scoreboard; 1 if the RF has all the data and instruction ready for the ALU, and so depends on the ALU taking its instruction

} t_gip_rf_data;

/*t t_gip_alu_state
 */
typedef struct t_gip_alu_state
{

    /*b State in the ALU guaranteed by the design
     */
    int inst_valid; // If 1 then instruction is valid, else not
    t_gip_instruction inst; // Instruction being executed by the ALU stage
    unsigned int alu_a_in; // Registered input from the register file stage
    unsigned int alu_b_in; // Registered input from the register file stage
    unsigned int acc;      // Accumulator value
    unsigned int shf;      // Shifter value
    int z;                 // Zero flag result from previous ALU operation
    int c;                 // Carry flag result from previous ALU operation
    int v;                 // last V flag from the ALU, or sticky version thereof
    int n;                 // last N flag from the ALU
    int p;                 // last carry out of the shifter
    int cp;                // last condition passed indication
    int old_cp;            // previous-to-last condition passed indication; use if we have a 'trail' of 2
} t_gip_alu_state;

/*t t_gip_alu_data
 */
typedef struct t_gip_full_alu_data
{

    /*b State in the ALU guaranteed by the design
     */
    t_gip_alu_state state;
    t_gip_alu_state next_state;

    /*b Combinatorials in the ALU
     */
    int condition_passed; // Requested condition in the ALU stage passed
    t_gip_alu_op1_src op1_src;
    t_gip_alu_op2_src op2_src;
    unsigned int alu_constant; // Constant 0, 1, 2 or 4 for offsets for stores
    int set_acc;
    int set_zcvn;
    int set_p;
    t_gip_alu_op gip_alu_op; // Actual ALU operation to perform
    unsigned int alu_op1; // Operand 1 to the ALU logic itself
    unsigned int alu_op2; // Operand 2 to the ALU logic itself
    int alu_z, alu_n, alu_c, alu_v;
    int logic_z, logic_n;
    unsigned int shf_result; // Result of the shifter
    int shf_carry; // Carry out of the shifter
    unsigned int arith_result; // Result of the arithmetic operation
    unsigned int logic_result; // Result of the logical operation
    unsigned int alu_result; // Result of the ALU as a whole
    unsigned int use_shifter; // Use shf_result instead of alu_result

    unsigned int next_acc;
    int next_c;
    int next_z;
    int next_v;
    int next_n;
    int next_p;
    unsigned int next_shf;
    int next_cp; // equal to condition_passed unless destination is a condition, in which case use old value (if not condition_passed) or result of condition evaluation (if condition_passed)
    int next_old_cp; // equal to current cp state, unless destination is a condtion, in which case this is 1

    t_gip_ins_r alu_rd;   // Type of register file write requested for the result of ALU operation
    t_gip_ins_r mem_rd;   // Type of register file write requested for the result of ALU operation
    t_gip_mem_op gip_mem_op; // Type of memory operation the memory stage should perform in the next cycle
    unsigned int mem_data_in; // Data in for a write to the memory operation
    unsigned int mem_address; // Address for a memory operation

    int accepting_rf_instruction; // 1 if the ALU stage is taking a proffered instruction from the RF state; if not asserted then the ALU stage should not assert flush for the pipeline, and the instruction within the ALU will not complete. Should only be deasserted if the ALU instruction is valid and writes to the RF but the RF indicates a stall, or if the ALU instruction is valid and causes a memory operation and the memory stage indicates a stall
} t_gip_alu_data;

/*t t_gip_mem_state
 */
typedef struct t_gip_mem_state
{
    /*b State in the memory stage guaranteed by the design
     */
    t_gip_ins_r mem_rd;   // Type of register file write requested for the result of ALU operation
    t_gip_mem_op gip_mem_op; // Type of memory operation the memory stage should perform in the next cycle
    unsigned int mem_data_in; // Data in for a write to the memory operation
    unsigned int mem_address; // Address for a memory operation
} t_gip_mem_state;

/*t t_gip_mem_data
 */
typedef struct t_gip_mem_data
{
    /*b State in the memory stage guaranteed by the design
     */
    t_gip_mem_state state;
    t_gip_mem_state next_state;

    /*b Combinatorials in the memory stage
     */
    unsigned int mem_result; // Combinatorial, but effectively generated off the clock edge ending the previous cycle for a sync RAM

} t_gip_mem_data;

/*t t_private_data
 */
typedef struct t_private_data
{
    c_memory_model *memory;
    int cycle;
    int verbose;

    /*b Configuration of the pipeline
     */
    int sticky_v;
    int sticky_z;
    int sticky_c;
    int sticky_n;
    int cp_trail_2; // Assert to have a 'condition passed' trail of 2, else it is one.
    int arm_trap_semaphore; // Number of semaphore to assert when an undefined instruction or SWI is hit
    int alu_mode; // 2-bit mode; 01 => math mode; 10 => bit mode; 11 => general purpose; 00=>bit mode (for now)
// these instructions force a deschedule, mov pc to r15 (optional), mov instruction to r?, set semaphore bit (in some order)

    /*b State and combinatorials in the GIP internal pipeline stages
     */
    t_gip_dec_data dec;
    t_gip_rf_data rf;
    t_gip_alu_data alu;
    t_gip_sched_data sched;
    t_gip_special_data special;
    t_gip_mem_data mem;
    t_gip_pipeline_results gip_pipeline_results;

    /*b Unsorted
     */

    /*b Combinatorials in the pipeline
     */

    /*b Irrelevant stuff for now
     */
    unsigned int last_address;
    int seq_count;
    int debugging_enabled;
    int breakpoints_enabled;
    unsigned int breakpoint_addresses[ MAX_BREAKPOINTS ];
    int halt;
    
} t_private_data;

/*a Static variables
 */

/*a Support functions
 */
/*f is_condition_met
 */
static int is_condition_met( t_private_data *pd, t_gip_full_alu_data *alu, t_gip_ins_cc gip_ins_cc )
{
    switch (gip_ins_cc)
    {
    case gip_ins_cc_eq:
        return alu->state.z;
    case gip_ins_cc_ne:
        return !alu->state.z;
    case gip_ins_cc_cs:
        return alu->state.c;
    case gip_ins_cc_cc:
        return !alu->state.c;
    case gip_ins_cc_mi:
        return alu->state.n;
    case gip_ins_cc_pl:
        return !alu->state.n;
    case gip_ins_cc_vs:
        return alu->state.v;
    case gip_ins_cc_vc:
        return !alu->state.v;
    case gip_ins_cc_hi:
        return alu->state.c && !alu->state.z;
    case gip_ins_cc_ls:
        return !alu->state.c || alu->state.z;
    case gip_ins_cc_ge:
        return (!alu->state.n && !alu->state.v) || (alu->state.n && alu->state.v);
    case gip_ins_cc_lt:
        return (!alu->state.n && alu->state.v) || (alu->state.n && !alu->state.v);
    case gip_ins_cc_gt:
        return ((!alu->state.n && !alu->state.v) || (alu->state.n && alu->state.v)) && !alu->state.z;
    case gip_ins_cc_le:
        return (!alu->state.n && alu->state.v) || (alu->state.n && !alu->state.v) || alu->state.z;
    case gip_ins_cc_always:
        return 1;
    case gip_ins_cc_cp:
        if (pd->cp_trail_2)
        {
            return alu->state.cp && alu->state.old_cp;
        }
        return alu->state.cp;
    }
    return 0;
}

/*f rotate_right
 */
static unsigned int rotate_right( unsigned int value, int by )
{
    unsigned int result;
    result = value>>by;
    result |= value<<(32-by);
    return result;
}

/*f barrel_shift
 */
static unsigned int barrel_shift( int c_in, t_shf_type type, unsigned int val, int by, int *c_out )
{
    //printf("Barrel shift %08x.%d type %d by %02x\n", val, c_in, type, by );
    by &= 0xff;
    if (by==0)
    {
        *c_out = c_in;
        return val;
    }
    if (by>32)
    {
        switch (type)
        {
        case shf_type_lsl:
            *c_out = 0;
            return 0;
        case shf_type_lsr:
            *c_out = 0;
            return 0;
        case shf_type_asr:
            *c_out = (val>>31)&1;
            return ((val>>31)&1) ? 0xfffffff : 0;
        case shf_type_ror:
            by = by&31;
            if (by==0)
            {
                by=32;
            }
            break;
        default:
            break;
        }
    }
    if (by==32)
    {
        switch (type)
        {
        case shf_type_lsl:
            *c_out = val&1;
            return 0;
        case shf_type_lsr:
            *c_out = (val>>31)&1;
            return 0;
        case shf_type_asr:
            *c_out = (val>>31)&1;
            return ((val>>31)&1) ? 0xffffffff : 0;
        case shf_type_ror:
            *c_out = (val>>31)&1;
            return val;
        default:
            break;
        }
    }
    switch (type)
    {
    case shf_type_lsl:
        *c_out = (val>>(32-by))&1;
        return val<<by;
    case shf_type_lsr:
        *c_out = (val>>(by-1))&1;
        return val>>by;
    case shf_type_asr:
        *c_out = (((int)val)>>(by-1))&1;
        return (unsigned int)(((int)val)>>by);
    case shf_type_ror:
        *c_out = (val>>(by-1))&1;
        return rotate_right( val, by );
    case shf_type_rrx:
        *c_out = val&1;
        val = (val>>1)&0x7fffffff;
        if (c_in)
            val |= 0x80000000;
        return val;
    }
    return 0;
}

/*f find_bit_set
 */
static unsigned int find_bit_set( unsigned int value, int dirn )
{
    unsigned int chain_left, chain_right;
    unsigned int single_bit;
    unsigned int result;
    int i;
    chain_left = 0;
    chain_right = 0;
    for (i=0; i<32; i++)
    {
        chain_right = (chain_right|value)>>1;
        chain_left = (chain_left|value)<<1;
    }
    single_bit = ((dirn==1)?chain_left:chain_right) ^ value; // Will be zero or will have precisely one bit set
    result = 0;
    if (single_bit & 0xaaaaaaaa) // bits 1,3,5,7,9 etc
    {
        result |= 1;
    }
    if (single_bit & 0xcccccccc) // bits 2,3,6,7,10,11,14,15 etc
    {
        result |= 2;
    }
    if (single_bit & 0xf0f0f0f0) // bits 4,5,6,7,12,13,14,15 etc
    {
        result |= 4;
    }
    if (single_bit & 0xff00ff00) // bits 8-15, 24-31
    {
        result |= 8;
    }
    if (single_bit & 0xffff0000) // bits 16-31
    {
        result |= 16;
    }
    return (single_bit)?result:0xffffffff; // single_bit of zero implies no bits set
}

/*f bit_count
 */
static unsigned int bit_count( unsigned int value )
{
    int i, j;
    for (i=0; i<32; i++)
    {
        if (value&(1<<i))
        {
            j++;
        }
    }
    return j;
}

/*f add_op
  in 8-bit world
  0x7f + 0xc0 = 0x13f, (0x3f C v)
  0x00 + 0x00 = 0x000, (0x00 c v)
  0x40 + 0x3f = 0x07f, (0x7f c v)
  0x40 + 0x40 = 0x080, (0x80 c V)
  0x7f + 0x80 = 0x1ff, (0xff C v)
  0xff + 0xff = 0x1fe, (0xfe C v)
  0x80 + 0x00 = 0x080, (0x80 c v)
  0x80 + 0x80 = 0x100, (0x00 C V)
  0x7f + 0x7f = 0x0fe, (0xfe c V)
  Note: +ve + -ve cannot overflow (V never set): smallest is 0x00+0x80+0 = 0x80, largest is 0x7f+0xff+1 = 0x7f.
  i.e. opposite top bits => V clear
  +ve + +ve overflows if carry in to top bit (i.e. top bit is set) (V set if res is negative)
  i.e. top bits both clear, result tb set => V set
  -ve + -ve overflows if carry in to top bit is clear, (i.e. result is positive)
  i.e. top bits both set, result tb clear => V set
*/
static unsigned int add_op( unsigned int a, unsigned int b, int c_in, int *c_out, int *v_out )
{
    unsigned int result_lo, result_hi;
    unsigned int sign_a, sign_b, sign_r;
    result_lo = (a&0xffff) + (b&0xffff) + (unsigned int)c_in;
    result_hi = (a>>16) + (b>>16) + (result_lo>>16);
    result_lo &= 0xffff;
    if (result_hi>>16)
    {
        *c_out = 1;
    }
    else
    {
        *c_out = 0;
    }
    sign_a = a>>31;
    sign_b = b>>31;
    sign_r = (result_hi>>15)&1;
    if (sign_a && sign_b && !sign_r)
    {
        *v_out = 1;
    }
    else if (!sign_a && !sign_b && sign_r)
    {
        *v_out = 1;
    }
    else
    {
        *v_out = 0;
    }
    return (result_hi<<16) | result_lo;
}

/*f disassemble_native_instruction
 */
static void disassemble_native_instruction( unsigned int pc, int opcode, char *buffer, int length )
{
    char *op;
    char *rm, *rd;
    char rm_buffer[16], rd_buffer[8];
    char text[128];

    t_gip_native_ins_class ins_class;
    t_gip_native_ins_subclass ins_subclass;
    int imm;
    int is_imm;

    /*b Generated 'op'
     */
    ins_class = (t_gip_native_ins_class) ((opcode>>12)&0xf);
    switch (ins_class)
    {
        /*b ALU
         */
    case gip_native_ins_class_alu_reg:
    case gip_native_ins_class_alu_imm:
        ins_subclass = (t_gip_native_ins_subclass) ((opcode>>8)&0xf);
        imm = (opcode&0xf);
        switch (ins_subclass)
        {
        case gip_native_ins_subclass_alu_and:
            op = "AND";
            break;
        case gip_native_ins_subclass_alu_or:
            op = "OR ";
            break;
        case gip_native_ins_subclass_alu_xor:
            op = "XOR";
            break;
        case gip_native_ins_subclass_alu_dis_orn_mla:
            op = "ORN|MLA";
            break;
        case gip_native_ins_subclass_alu_dis_bic_init:
            op = "BIC|INIT";
            break;
        case gip_native_ins_subclass_alu_mov:
            op = "MOV";
            break;
        case gip_native_ins_subclass_alu_mvn:
            op = "MVN";
            break;
        case gip_native_ins_subclass_alu_add:
            op = "ADD";
            break;
        case gip_native_ins_subclass_alu_sub:
            op = "SUB";
            break;
        case gip_native_ins_subclass_alu_rsb:
            op = "RSB";
            break;
        case gip_native_ins_subclass_alu_dis_andcnt_mlb:
            op = "ANDC|MLB";
            break;
        case gip_native_ins_subclass_alu_adc:
            op = "ADC";
            break;
        case gip_native_ins_subclass_alu_dis_xorlast_sbc:
            op = "XORL|SBC";
            break;
        case gip_native_ins_subclass_alu_rsc:
            op = "RSC";
            break;
        case gip_native_ins_subclass_alu_dis_bitreverse_dva:
            op = "BITR|DVA";
            break;
        case gip_native_ins_subclass_alu_dis_bytereverse_dvb:
            op = "BYTRE|DVB";
            break;
        case gip_native_ins_subclass_alu_xorfirst:
            op = "XORF";
            break;
        default:
            op = "<UNEXPECTED>";
            break;
        }
        break;
        /*b Conditional
         */
    case gip_native_ins_class_cond_reg:
    case gip_native_ins_class_cond_imm:
        ins_subclass = (t_gip_native_ins_subclass) ((opcode>>8)&0xf);
        imm = (opcode&0xf);
        switch (ins_subclass)
        {
        case gip_native_ins_subclass_cond_eq: op = "CMPEQ"; break;
        case gip_native_ins_subclass_cond_ne: op = "CMPNE"; break;
        case gip_native_ins_subclass_cond_gt: op = "CMPGT"; break;
        case gip_native_ins_subclass_cond_ge: op = "CMPGE"; break;
        case gip_native_ins_subclass_cond_lt: op = "CMPLT"; break;
        case gip_native_ins_subclass_cond_le: op = "CMPLE"; break;
        case gip_native_ins_subclass_cond_hi: op = "CMPHI"; break;
        case gip_native_ins_subclass_cond_hs: op = "CMPHS"; break;
        case gip_native_ins_subclass_cond_lo: op = "CMPLO"; break;
        case gip_native_ins_subclass_cond_ls: op = "CMPLS"; break;
        case gip_native_ins_subclass_cond_seq: op = "TSTSEQ"; break;
        case gip_native_ins_subclass_cond_sne: op = "TSTSNE"; break;
        case gip_native_ins_subclass_cond_sgt: op = "TSTSGT"; break;
        case gip_native_ins_subclass_cond_sge: op = "TSTSGE"; break;
        case gip_native_ins_subclass_cond_slt: op = "TSTSLT"; break;
        case gip_native_ins_subclass_cond_sle: op = "TSTSLE"; break;
        case gip_native_ins_subclass_cond_shi: op = "TSTSHI"; break;
        case gip_native_ins_subclass_cond_shs: op = "TSTSHS"; break;
        case gip_native_ins_subclass_cond_slo: op = "TSTSLO"; break;
        case gip_native_ins_subclass_cond_sls: op = "TSTSLS"; break;
        case gip_native_ins_subclass_cond_smi: op = "TSTSMI"; break;
        case gip_native_ins_subclass_cond_spl: op = "TSTSPL"; break;
        case gip_native_ins_subclass_cond_svs: op = "TSTSVS"; break;
        case gip_native_ins_subclass_cond_svc: op = "TSTSVC"; break;
        case gip_native_ins_subclass_cond_sps: op = "TSTSPS"; break;
        case gip_native_ins_subclass_cond_spc: op = "TSTSPC"; break;
        case gip_native_ins_subclass_cond_allset: op = "TSTALLSET"; break;
        case gip_native_ins_subclass_cond_anyclr: op = "TSTANYCLR"; break;
        case gip_native_ins_subclass_cond_allclr: op = "TSTALLCLR"; break;
        case gip_native_ins_subclass_cond_anyset: op = "TSTANYSET"; break;
        default: op = "<UNEXPECTED>"; break;
        }
        break;
        /*b Shift
         */
    case gip_native_ins_class_shift:
        ins_subclass = (t_gip_native_ins_subclass) ((opcode>>10)&0x3);
        imm = (opcode&0xf)|(((opcode>>8)&1)<<4);
        is_imm = (opcode>>9)&1;
        switch (ins_subclass)
        {
        case gip_native_ins_subclass_shift_lsl: op = "LSL"; break;
        case gip_native_ins_subclass_shift_lsr: op = "LSR"; break;
        case gip_native_ins_subclass_shift_asr: op = "ASR"; break;
        case gip_native_ins_subclass_shift_ror:
            op = "ROR";
            if (is_imm && imm==0)
            {
                op = "ROR33";
            }
            break;
        default:            op = "<UNEXPECTED>";            break;
        }
        break;
        /*b Memoty (load/store)
         */
    case gip_native_ins_class_memory:
        ins_subclass = (t_gip_native_ins_subclass) ((opcode>>9)&0x7);
        switch ((t_gip_native_ins_subclass) ((opcode>>8)&1))
        {
        case gip_native_ins_subclass_memory_load: op = "LDR"; break;
        case gip_native_ins_subclass_memory_store: op = "STR"; break;
        default:            op = "<UNEXPECTED>";            break;
        }
        break;
        /*b Branch
         */
    case gip_native_ins_class_branch:
        ins_subclass = (t_gip_native_ins_subclass) ((opcode>>11)&0x1);
        switch (ins_subclass)
        {
        case gip_native_ins_subclass_branch_no_delay: op = "B"; break;
        case gip_native_ins_subclass_branch_delay: op = "B #1, "; break;
        default:            op = "<UNEXPECTED>";            break;
        }
        break;
        /*b Done
         */
    default:
        break;
    }

    /*b Get text of Rd
     */
    switch ((opcode>>4)&0xf)
    {
    default:
        rd = rd_buffer;
        sprintf( rd_buffer, "R%d", ((opcode>>4)&0xf) );
        break;
    }

    /*b Get text of Rm
     */
    switch ((opcode>>0)&0xf)
    {
    default:
        rm = rm_buffer;
        sprintf( rm_buffer, "R%d", ((opcode>>0)&0xf) );
        break;
    }

    /*b Decode instruction
     */
    switch (ins_class)
    {
    case gip_native_ins_class_alu_reg:
    case gip_native_ins_class_cond_reg:
        sprintf( text, "%s %s, %s",
                 op,
                 rd,
                 rm );
        break;
    case gip_native_ins_class_alu_imm:
    case gip_native_ins_class_cond_imm:
        sprintf( text, "%s %s, #0x%01x",
                 op,
                 rd,
                 imm );
        break;
    case gip_native_ins_class_shift:
        if ((is_imm) && (imm==0))
        {
            sprintf( text, "%s %s",
                     op,
                     rd );
        }
        else if (is_imm)
        {
            sprintf( text, "%s %s, #%d",
                     op,
                     rd,
                     imm );
        }
        else
        {
            sprintf( text, "%s %s, %s",
                     op,
                     rd,
                     rm );
        }
        break;
    case gip_native_ins_class_memory:
        ins_subclass = (t_gip_native_ins_subclass) ((opcode>>9)&0x7);
        switch (ins_subclass)
        {
        case gip_native_ins_subclass_memory_word_noindex: sprintf( text, "%s %s, [%s]", op, rd, rm ); break;
        case gip_native_ins_subclass_memory_half_noindex: sprintf( text, "%sH %s, [%s]", op, rd, rm ); break;
        case gip_native_ins_subclass_memory_byte_noindex: sprintf( text, "%sB %s, [%s]", op, rd, rm ); break;
        case gip_native_ins_subclass_memory_word_preindex_up: sprintf( text, "%s %s, [%s, #4]%s", op, rd, rm, (opcode>>8)&1?"!":"" ); break;
        case gip_native_ins_subclass_memory_word_preindex_up_shf: sprintf( text, "%s %s, [%s, SHF]", op, rd, rm ); break;
        case gip_native_ins_subclass_memory_word_preindex_down_shf: sprintf( text, "%s %s, [%s, -SHF]", op, rd, rm ); break;
        case gip_native_ins_subclass_memory_word_postindex_up: sprintf( text, "%s %s, [%s], #4", op, rd, rm ); break;
        case gip_native_ins_subclass_memory_word_postindex_down: sprintf( text, "%s %s, [%s], #-4", op, rd, rm ); break;
        default: break;
        }
        break;
    case gip_native_ins_class_branch:
        sprintf( text, "%s 0x%08x",
                 op,
                 pc-8+((opcode<<20)>>20) );
        break;
    case gip_native_ins_class_extimm_0:
    case gip_native_ins_class_extimm_1:
    case gip_native_ins_class_extimm_2:
    case gip_native_ins_class_extimm_3:
        sprintf( text, "EXTIMM #%08x",
                 ((opcode<<18)>>18) );
        break;
    case gip_native_ins_class_extrdrm:
        sprintf( text, "EXTRDRM #%02x, #%02x", (opcode>>4)&0xff, ((opcode>>0)&0xf)<<4 );
        break;
    case gip_native_ins_class_extrnrm:
        sprintf( text, "EXTRNRM #%02x, #%02x", (opcode>>4)&0xf0, ((opcode>>0)&0xf)<<4 );
        break;
    default:
        sprintf( text, "NO DISASSEMBLY YET" );
        break;
    }

    /*b Display instruction
     */
    strncpy( buffer, text, length );
    buffer[length-1] = 0;
}

/*f c_gip_full::disassemble_int_instruction
 */
void c_gip_full::disassemble_int_instruction( int valid, t_gip_instruction *inst, char *buffer, int length )
{
    char *op;
    char *cc;
    char *rn, *rm, *rd;
    const char *offset;
    char rn_buffer[8], rm_buffer[16], rd_buffer[8];
    int no_rn;
    char text[128];
    char *tptr;

    text[0] = valid?'V':'I';
    text[1] = ':';
    tptr = text+2;

    /*b Get text of CC
     */
    switch (inst->gip_ins_cc)
    {
    case gip_ins_cc_eq:
        cc = "EQ";
        break;
    case gip_ins_cc_ne:
        cc = "NE";
        break;
    case gip_ins_cc_cs:
        cc = "CS";
        break;
    case gip_ins_cc_cc:
        cc = "CC";
        break;
    case gip_ins_cc_mi:
        cc = "MI";
        break;
    case gip_ins_cc_pl:
        cc = "PL";
        break;
    case gip_ins_cc_vs:
        cc = "VS";
        break;
    case gip_ins_cc_vc:
        cc = "VC";
        break;
    case gip_ins_cc_hi:
        cc = "HI";
        break;
    case gip_ins_cc_ls:
        cc = "LS";
        break;
    case gip_ins_cc_ge:
        cc = "GE";
        break;
    case gip_ins_cc_lt:
        cc = "LT";
        break;
    case gip_ins_cc_gt:
        cc = "GT";
        break;
    case gip_ins_cc_le:
        cc = "LE";
        break;
    case gip_ins_cc_always:
        cc = "";
        break;
    case gip_ins_cc_cp:
        cc = "CP";
        break;
    }

    /*b Get text of Rd
     */
    switch (inst->gip_ins_rd.type)
    {
    case gip_ins_r_type_none:
    case gip_ins_r_type_none_b:
            rd = "";
            break;
    case gip_ins_r_type_register:
        rd = rd_buffer;
        sprintf( rd_buffer, "-> R%d", inst->gip_ins_rd.data.r );
        break;
    case gip_ins_r_type_register_indirect:
        rd = rd_buffer;
        sprintf( rd_buffer, "-> [[R%d]]", inst->gip_ins_rd.data.r );
        break;
    case gip_ins_r_type_internal:
        switch (inst->gip_ins_rd.data.rd_internal)
        {
        case gip_ins_rd_int_eq:
            rd = "-> EQ";
            break;
        case gip_ins_rd_int_ne:
            rd = "-> NE";
            break;
        case gip_ins_rd_int_cs:
            rd = "-> CS";
            break;
        case gip_ins_rd_int_cc:
            rd = "-> CC";
            break;
        case gip_ins_rd_int_hi:
            rd = "-> HI";
            break;
        case gip_ins_rd_int_ls:
            rd = "-> LS";
            break;
        case gip_ins_rd_int_ge:
            rd = "-> GE";
            break;
        case gip_ins_rd_int_lt:
            rd = "-> LT";
            break;
        case gip_ins_rd_int_gt:
            rd = "-> GT";
            break;
        case gip_ins_rd_int_le:
            rd = "-> LE";
            break;
        case gip_ins_rd_int_pc:
            rd = "-> PC";
            break;
        }
        break;
    case gip_ins_r_type_periph:
        rd = rd_buffer;
        sprintf( rd_buffer, "-> P%d", inst->gip_ins_rd.data.r );
        break;
    case gip_ins_r_type_postbus:
        rd = rd_buffer;
        sprintf( rd_buffer, "-> C%d", inst->gip_ins_rd.data.r );
        break;
    case gip_ins_r_type_special:
        rd = rd_buffer;
        sprintf( rd_buffer, "-> S%d", inst->gip_ins_rd.data.r );
        break;
    default:
        rd = rd_buffer;
        sprintf( rd_buffer, "-> ???C%d", inst->gip_ins_rd.data.r );
        break;
    }

    /*b Get text of Rn
     */
    switch (inst->gip_ins_rn.type)
    {
    case gip_ins_r_type_none:
    case gip_ins_r_type_none_b:
        rn = "<NONE>";
        break;
    case gip_ins_r_type_register:
        rn = rn_buffer;
        sprintf( rn_buffer, "-> R%d", inst->gip_ins_rn.data.r );
        break;
    case gip_ins_r_type_register_indirect:
        rn = rn_buffer;
        sprintf( rn_buffer, "-> [[R%d]]", inst->gip_ins_rn.data.r );
        break;
    case gip_ins_r_type_internal:
        switch (inst->gip_ins_rn.data.rnm_internal)
        {
        case gip_ins_rnm_int_acc:
            rn = "ACC";
            break;
        case gip_ins_rnm_int_shf:
            rn = "SHF";
            break;
        case gip_ins_rnm_int_pc:
            rn = "PC";
            break;
        default:
            rn = "<UNKNOWN>";
            break;
        }
        break;
    case gip_ins_r_type_periph:
        rn = rn_buffer;
        sprintf( rn_buffer, "-> P%d", inst->gip_ins_rn.data.r );
        break;
    case gip_ins_r_type_postbus:
        rn = rn_buffer;
        sprintf( rn_buffer, "-> C%d", inst->gip_ins_rn.data.r );
        break;
    case gip_ins_r_type_special:
        rn = rn_buffer;
        sprintf( rn_buffer, "-> S%d", inst->gip_ins_rn.data.r );
        break;
    default:
        rn = rn_buffer;
        sprintf( rn_buffer, "-> ???C%d", inst->gip_ins_rn.data.r );
        break;
    }

    /*b Get text of Rm
     */
    if (inst->rm_is_imm)
    {
        sprintf( rm_buffer, "#%08x", inst->rm_data.immediate );
        rm = rm_buffer;
    }
    else
    {
        switch (inst->rm_data.gip_ins_rm.type)
        {
        case gip_ins_r_type_none:
        case gip_ins_r_type_none_b:
            rm = "<NONE>";
            break;
        case gip_ins_r_type_register:
            rm = rm_buffer;
            sprintf( rm_buffer, "-> R%d", inst->rm_data.gip_ins_rm.data.r );
            break;
        case gip_ins_r_type_register_indirect:
            rm = rm_buffer;
            sprintf( rm_buffer, "-> [[R%d]]", inst->rm_data.gip_ins_rm.data.r );
            break;
        case gip_ins_r_type_internal:
            switch (inst->rm_data.gip_ins_rm.data.rnm_internal)
            {
            case gip_ins_rnm_int_acc:
                rm = "ACC";
                break;
            case gip_ins_rnm_int_shf:
                rm = "SHF";
                break;
            case gip_ins_rnm_int_pc:
                rm = "PC";
                break;
            default:
                rm = "<UNKNOWN>";
                break;
            }
            break;
        case gip_ins_r_type_periph:
            rm = rm_buffer;
            sprintf( rm_buffer, "-> P%d", inst->rm_data.gip_ins_rm.data.r );
            break;
        case gip_ins_r_type_postbus:
            rm = rm_buffer;
            sprintf( rm_buffer, "-> C%d", inst->rm_data.gip_ins_rm.data.r );
            break;
        case gip_ins_r_type_special:
            rm = rm_buffer;
            sprintf( rm_buffer, "-> S%d", inst->rm_data.gip_ins_rm.data.r );
            break;
        default:
            rm = rm_buffer;
            sprintf( rm_buffer, "-> ???C%d", inst->rm_data.gip_ins_rm.data.r );
            break;
        }

    }

    /*b Decode instruction
     */
    switch (inst->gip_ins_class)
    {
    case gip_ins_class_logic:
        no_rn = 0;
        switch (inst->gip_ins_subclass)
        {
        case gip_ins_subclass_logic_and:
            op = "IAND";
            break;
        case gip_ins_subclass_logic_or:
            op = "IOR";
            break;
        case gip_ins_subclass_logic_xor:
            op = "IXOR";
            break;
        case gip_ins_subclass_logic_bic:
            op = "IBIC";
            break;
        case gip_ins_subclass_logic_orn:
            op = "IORN";
            break;
        case gip_ins_subclass_logic_mov:
            op = "IMOV";
            no_rn = 1;
            break;
        case gip_ins_subclass_logic_mvn:
            op = "IMVN";
            no_rn = 1;
            break;
        case gip_ins_subclass_logic_andcnt:
            op = "IANDC";
            break;
        case gip_ins_subclass_logic_andxor:
            op = "IANDX";
            break;
        case gip_ins_subclass_logic_xorfirst:
            op = "IXORF";
            break;
        case gip_ins_subclass_logic_xorlast:
            op = "IXORL";
            no_rn = 1;
            break;
        case gip_ins_subclass_logic_bitreverse:
            op = "IBITR";
            no_rn = 1;
            break;
        case gip_ins_subclass_logic_bytereverse:
            op = "IBYTR";
            no_rn = 1;
            break;
        default:
            break;
        }
        sprintf( tptr, "%s%s%s%s%s%s %s%s%s %s",
                 op,
                 cc,
                 inst->gip_ins_opts.alu.s?"S":"",
                 inst->gip_ins_opts.alu.p?"P":"",
                 inst->a?"A":"",
                 inst->f?"F":"",
                 no_rn?"":rn,
                 no_rn?"":", ",
                 rm,
                 rd );
        break;
    case gip_ins_class_arith:
        switch (inst->gip_ins_subclass)
        {
        case gip_ins_subclass_arith_add:
            op = "IADD";
            break;
        case gip_ins_subclass_arith_adc:
            op = "IADC";
            break;
        case gip_ins_subclass_arith_sub:
            op = "ISUB";
            break;
        case gip_ins_subclass_arith_sbc:
            op = "ISBC";
            break;
        case gip_ins_subclass_arith_rsb:
            op = "IRSB";
            break;
        case gip_ins_subclass_arith_rsc:
            op = "IRSC";
            break;
        case gip_ins_subclass_arith_init:
            op = "INIT";
            break;
        case gip_ins_subclass_arith_mla:
            op = "IMLA";
            break;
        case gip_ins_subclass_arith_mlb:
            op = "IMLB";
            break;
        default:
            break;
        }
        sprintf( tptr, "%s%s%s%s%s %s, %s %s",
                 op,
                 cc,
                 inst->gip_ins_opts.alu.s?"S":"",
                 inst->a?"A":"",
                 inst->f?"F":"",
                 rn,
                 rm,
                 rd );
        break;
    case gip_ins_class_shift:
        switch (inst->gip_ins_subclass)
        {
        case gip_ins_subclass_shift_lsl:
            op = "ILSL";
            break;
        case gip_ins_subclass_shift_lsr:
            op = "ILSR";
            break;
        case gip_ins_subclass_shift_asr:
            op = "IASR";
            break;
        case gip_ins_subclass_shift_ror:
            op = "IROR";
            break;
        case gip_ins_subclass_shift_ror33:
            op = "IROR33";
            break;
        default:
            break;
        }
        sprintf( tptr, "%s%s%s%s %s, %s %s",
                 op,
                 cc,
                 inst->gip_ins_opts.alu.s?"S":"",
                 inst->f?"F":"",
                 rn,
                 rm,
                 rd );
        break;
    case gip_ins_class_load:
        op = "<OP>";
        offset = "<OFS>";
        switch (inst->gip_ins_subclass & gip_ins_subclass_memory_size)
        {
        case gip_ins_subclass_memory_word: op=""; break;
        case gip_ins_subclass_memory_half: op="H"; break;
        case gip_ins_subclass_memory_byte: op="B"; break;
        default: break;
        }
        switch (inst->gip_ins_subclass & gip_ins_subclass_memory_dirn)
        {
        case gip_ins_subclass_memory_up: offset="+"; break;
        default:  offset="-"; break;
        }
        sprintf( tptr, "ILDR%s%s%s%s%s #%d (%s%s, %s%s%s %s",
                 cc,
                 op,
                 inst->a?"A":"",
                 inst->gip_ins_opts.load.stack?"S":"",
                 inst->f?"F":"",
                 inst->k,
                 rn,
                 ((inst->gip_ins_subclass&gip_ins_subclass_memory_index)==gip_ins_subclass_memory_postindex)?")":"",
                 offset,
                 rm,
                 ((inst->gip_ins_subclass&gip_ins_subclass_memory_index)==gip_ins_subclass_memory_postindex)?"":")",
                 rd );
        break;
    case gip_ins_class_store:
        op = "<OP>";
        offset = "<OFS>";
        switch (inst->gip_ins_subclass & gip_ins_subclass_memory_size)
        {
        case gip_ins_subclass_memory_word: op=""; break;
        case gip_ins_subclass_memory_half: op="H"; break;
        case gip_ins_subclass_memory_byte: op="B"; break;
        default: break;
        }
        if (inst->gip_ins_opts.store.offset_type)
        {
            if ((inst->gip_ins_subclass & gip_ins_subclass_memory_dirn) == gip_ins_subclass_memory_up)
            {
                offset="+SHF";
            }
            else
            {
                offset="-SHF";
            }
        }
        else
        {
            switch (inst->gip_ins_subclass & gip_ins_subclass_memory_size)
            {
            case gip_ins_subclass_memory_word:
                offset = ((inst->gip_ins_subclass & gip_ins_subclass_memory_dirn) == gip_ins_subclass_memory_up)?"+4":"-4";
                break;
            case gip_ins_subclass_memory_half:
                offset = ((inst->gip_ins_subclass & gip_ins_subclass_memory_dirn) == gip_ins_subclass_memory_up)?"+2":"-2";
                break;
            case gip_ins_subclass_memory_byte:
                offset = ((inst->gip_ins_subclass & gip_ins_subclass_memory_dirn) == gip_ins_subclass_memory_up)?"+1":"-1";
                break;
            default:
                break;
            }
        }
        sprintf( tptr, "ISTR%s%s%s%s #%d (%s%s%s%s <- %s %s",
                 cc,
                 op,
                 inst->a?"A":"",
                 inst->gip_ins_opts.store.stack?"S":"",
                 inst->k,
                 rn,
                 ((inst->gip_ins_subclass&gip_ins_subclass_memory_index)==gip_ins_subclass_memory_postindex)?")":"",
                 offset,
                 ((inst->gip_ins_subclass&gip_ins_subclass_memory_index)==gip_ins_subclass_memory_postindex)?"":")",
                 rm,
                 rd );
        break;
    default:
        sprintf( tptr, "NO DISASSEMBLY YET %d/%d", inst->gip_ins_class, inst->gip_ins_subclass );
        break;
    }

    /*b Display instruction
     */
    strncpy( buffer, text, length );
    buffer[length-1] = 0;
}

/*a Constructors/destructors
 */
/*f c_gip_full::c_gip_full
 */
c_gip_full::c_gip_full( c_memory_model *memory ) : c_execution_model_class( memory )
{
    int i;

    pd = (t_private_data *)malloc(sizeof(t_private_data));

    pd->cycle = 0;
    pd->memory = memory;
    pd->verbose = 0;

    /*b Initialize the special registers
     */
    pd->special.state.semaphores = 1;
    pd->special.state.cooperative = 1;
    pd->special.state.round_robin = 0;
    pd->special.state.thread_0_privilege = 1;
    pd->special.state.selected_thread = 0;
    pd->special.state.write_thread = 0;
    pd->special.state.thread_data_write_pc = 0;
    pd->special.state.thread_data_write_config = 0;
    pd->special.state.repeat_count = 0;

    /*b Initialize the scheduler
     */
    for (i=0; i<NUM_THREADS; i++)
    {
        pd->sched.state.thread_data[i].restart_pc = 0;
        pd->sched.state.thread_data[i].flag_dependencies = 0;
        pd->sched.state.thread_data[i].register_map = 0;
        pd->sched.state.thread_data[i].config = 0;
        pd->sched.state.thread_data[i].running = 0;
    }
    pd->sched.state.thread_data[0].flag_dependencies = 1;
    pd->sched.state.thread = 0;
    pd->sched.state.running = 0;
    pd->sched.state.preempted_medium_thread = 0;
    pd->sched.state.preempted_low_thread = 0;
    
    /*b Initialize the decoder
     */
    pd->dec.state.op_state = gip_dec_op_state_idle;
    pd->dec.state.pc = 0;
    pd->dec.state.pc_valid = 0;
    pd->dec.state.opcode = 0;
    pd->dec.state.acknowledge_scheduler = 0;
    pd->dec.state.cycle_of_opcode = 0;
    pd->dec.state.fetch_requested = 0;
    pd->dec.state.extended_immediate = 0;
    pd->dec.state.extended_rd.type= gip_ins_r_type_none;
    pd->dec.state.extended_rm.type = gip_ins_r_type_none;
    pd->dec.state.extended_rn.type = gip_ins_r_type_none;
    pd->dec.state.extended_cmd.extended = 0;
    pd->dec.state.reg_in_acc = 0;
    pd->dec.state.acc_valid = 0;
    pd->dec.state.in_delay_slot = 0;
    pd->dec.state.follow_delay_pc = 0;
    pd->dec.state.delay_pc = 0;
    pd->dec.state.in_conditional_shadow = 0;
    pd->dec.state.in_immediate_shadow = 0;

    /*b Initialize the register file
     */
    for (i=0; i<MAX_REGISTERS; i++)
    {
        pd->rf.state.regs[i] = 0;
    }
    build_gip_instruction_nop( &pd->rf.state.inst );
    pd->rf.state.inst_valid = 0;
    pd->rf.state.alu_rd.type = gip_ins_r_type_none;
    pd->rf.state.mem_rd.type = gip_ins_r_type_none;
    pd->rf.state.accepting_alu_rd = 1;
    pd->rf.state.use_shifter = 0;

    /*b Initialize the ALU
     */
    build_gip_instruction_nop( &pd->alu.state.inst );
    pd->alu.state.inst_valid = 0;
    pd->alu.state.acc = 0;
    pd->alu.state.alu_a_in = 0;
    pd->alu.state.alu_b_in = 0;
    pd->alu.state.shf = 0;
    pd->alu.state.z = 0;
    pd->alu.state.c = 0;
    pd->alu.state.v = 0;
    pd->alu.state.n = 0;
    pd->alu.state.p = 0;
    pd->alu.state.cp = 0;

    /*b Initialize the memory stage
     */
    pd->mem.state.mem_rd.type = gip_ins_r_type_none;
    pd->mem.state.gip_mem_op = gip_mem_op_none;
    pd->mem.state.mem_data_in = 0;
    pd->mem.state.mem_address = 0;

    /*b Initialize the other stuff for the model
     */
    pd->last_address = 0;
    pd->seq_count = 0;
    /*
      pd->debugging_enabled = 0;
      pd->breakpoints_enabled = 0;
      pd->tracing_enabled = 0;
      for (i=0; i<MAX_BREAKPOINTS; i++)
      {
      pd->breakpoint_addresses[i] = 0;
      }
      for (i=0; i<MAX_TRACING; i++)
      {
      pd->trace_region_starts[i] = 0;
      pd->trace_region_ends[i] = 0;
      }
      pd->halt = 0;
      for (i=0; i<8; i++)
      {
      pd->interrupt_handler[i] = 0;
      }
      pd->swi_return_addr = 0xdeadadd0;
      pd->swi_sp = 0xdeadadd1;
      pd->swi_code = 0xdeadadd2;
      pd->kernel_sp = 0xdeadadd3;
      pd->super = 0xdeadadd4;
    */
}

/*f c_gip_full::~c_gip_full
 */
c_gip_full::~c_gip_full()
{
}

/*a Debug information methods
 */
/*f c_gip_full:debug
 */
void c_gip_full::debug( int mask )
{
    char buffer[256];
    unsigned int address;
    unsigned int opcode;

    if (mask&8)
    {
        address = pd->dec.state.pc;
        if (address!=pd->last_address+4)
        {
            pd->last_address = address;
            pd->seq_count = 0;
        }
        else
        {
            pd->last_address = address;
            pd->seq_count++;
        }
    }
    if (mask&2)
    {
        printf( "\t r0: %08x   r1: %08x   r2: %08x   r3: %08x\n",
                pd->rf.state.regs[0],
                pd->rf.state.regs[1],
                pd->rf.state.regs[2],
                pd->rf.state.regs[3] );
        printf( "\t r4: %08x   r5: %08x   r6: %08x   r7: %08x\n",
                pd->rf.state.regs[4],
                pd->rf.state.regs[5],
                pd->rf.state.regs[6],
                pd->rf.state.regs[7] );
        printf( "\t r8: %08x   r9: %08x  r10: %08x  r11: %08x\n",
                pd->rf.state.regs[8],
                pd->rf.state.regs[9],
                pd->rf.state.regs[10],
                pd->rf.state.regs[11] );
        printf( "\tr12: %08x  r13: %08x  r14: %08x  r15: %08x\n",
                pd->rf.state.regs[12],
                pd->rf.state.regs[13],
                pd->rf.state.regs[14],
                pd->rf.state.regs[15] );
    }
    if (mask&4)
    {
        printf( "\t  acc:%08x shf:%08x c:%d  n:%d  z:%d  v:%d p:%d cp:%d ocp:%d\n",
                pd->alu.state.acc,
                pd->alu.state.shf,
                pd->alu.state.c,
                pd->alu.state.n,
                pd->alu.state.z,
                pd->alu.state.v,
                pd->alu.state.p,
                pd->alu.state.cp,
                pd->alu.state.old_cp );
        printf( "\t  alu a:%08x alu b:%08x\n",
                pd->alu.state.alu_a_in,
                pd->alu.state.alu_b_in );
    }
    if (mask&1)
    {
        address = pd->dec.state.pc;
        opcode = pd->memory->read_memory( address );
        arm_disassemble( address, opcode, buffer );

        const char * label = symbol_lookup (address);
        printf( "%32s %08x %08x: %s\n",
                label?label:"",
                address,
                opcode,
                buffer );
    }
    if (mask)
    {
        fflush(stdout);
    }
}

/*a Public level methods
 */
/*f c_gip_full:step
  Returns number of instructions executed
 */
int c_gip_full::step( int *reason, int requested_count )
{
    int i;

    *reason = 0;

    /*b Loop for requested count
     */
    for (i=0; i<requested_count; i++)
    {
        comb();
        preclock();
        clock();
    }
    return requested_count;
}

/*f c_gip_full::load_code
 */
void c_gip_full::load_code( FILE *f, unsigned int base_address )
{
    unsigned int opcode;
    unsigned int address;

    while (!feof(f))
    {
        if (fscanf(f,  "%08x: %08x %*s\n", &address, &opcode)==2)
        {
            pd->memory->write_memory( address, opcode, 0xf );
        }
        else
        {
            int c;
            c=fgetc(f);
            while ((c!=EOF) && (c!='\n'))
            {
                c=fgetc(f);
            }
            c=fgetc(f);
        }
    }
}

/*f c_gip_full::load_code_binary
 */
void c_gip_full::load_code_binary( FILE *f, unsigned int base_address )
{
    unsigned int opcode;
    unsigned int address;

    address = base_address;
    while (!feof(f))
    {
        if (fread( &opcode, 1, 4, f )!=4)
        {
            break;
        }
        pd->memory->write_memory( address, opcode, 0xf );
        address+=4;
    }
    printf ("Code loaded, end address was %x\n", address);
}

/*f c_gip_full::load_symbol_table
 */
void c_gip_full::load_symbol_table( char *filename )
{
    symbol_initialize( filename );
}

/*a Scheduler stage methods
  The scheduler presents (from a clock edge) its request to change to another thread
  The decode also has information from its clock edge based on the previous instruction
  decoded indicating whether it can be preempted; if it is idle, then it can be :-)
  The decode stage combinatorially combines this information to determine if the
  instruction it currently has should be decoded, or if a NOP should be decoded instead.
  This information is given to the scheduler as an acknowledge
  Note that if a deschedule instruction (even if conditional) has been inserted in to the pipeline then that must
  block any acknowledge - it implicitly ensures the 'atomic' indication is set.
  In addition to the scheduling above, the register file read stage can indicate to the scheduler that
  a thread execution has completed (i.e. the thread has descheduled). This may be accompanied by a restart address.
  The scheduler should also restart lower priority threads when high priority threads
  complete.
  So, if we fo for the discard of prefetched instructions, we have:
    decoder -> scheduler indicates that any presented thread preempt or start will be taken
    scheduler -> decoder indicates a thread to start and its PC and decode/pipeline configuration
    register file read -> scheduler indicates that a thread has completed (possibly with restart PC)
    register file write -> scheduler indicates restart PC for a thread and configuration
    decoder -> ALU indicates priority level that is being operated at (for CZVN and accumulator and shifter)
    register file read -> decoder indicates that a deschedule event actually took place (qualifier for flush)
 */
/*f c_gip_full::sched_comb
  The scheduler operates in three modes:
    cooperative round robin
    cooperative prioritized
    preemptive prioritized
 */
void c_gip_full::sched_comb( void )
{
    int i;
    int schedulable[NUM_THREADS];
    int priority_thread, priority_schedulable;
    int round_robin_thread, round_robin_schedulable;
    int chosen_thread, chosen_schedulable;

    /*b Determine the schedulability of each thread, and highest priority
     */
    priority_schedulable = 0;
    priority_thread = 0;
    for (i=0; i<NUM_THREADS; i++)
    {
        schedulable[i] = ( ( pd->sched.state.thread_data[i].flag_dependencies &
                             (pd->special.state.semaphores>>(i*4)) &
                             0xf ) &&
                           !pd->sched.state.thread_data[i].running
            );
        if (schedulable[i])
        {
            priority_thread = i;
            priority_schedulable = 1;
            printf("Schedule %d\n", i );
        }
    }

    /*b Determine round-robin thread
     */
    round_robin_thread = (pd->sched.state.thread+1)%NUM_THREADS;
    round_robin_schedulable = 0;
    if (!pd->sched.state.running) // If running, hold the round robin thread at the next thread, else...
    {
        if (schedulable[pd->sched.state.thread]) // If the next thread is schedulable, then use it
        {
            round_robin_thread = pd->sched.state.thread;
            round_robin_schedulable = 1;
        }
        else if (schedulable[pd->sched.state.thread|1]) // Else try (possibly) the one after that
        {
            round_robin_thread = pd->sched.state.thread|1;
            round_robin_schedulable = 1;
        }
        else // Else just move on the thread
        {
            round_robin_thread = ((pd->sched.state.thread|1)+1)%NUM_THREADS;
            round_robin_schedulable = 0;
        }
    }

    /*b Choose high priority or round-robin
     */
    chosen_thread = priority_thread;
    chosen_schedulable = priority_schedulable;
    if (pd->special.state.round_robin)
    {
        chosen_thread = round_robin_thread;
        chosen_schedulable = round_robin_schedulable;
    }

    /*b Determine requester - if not running we will pick the chosen thread; if running we only do so if preempting and the levels allow it
    */
    pd->sched.next_thread_to_start = chosen_thread;
    pd->sched.next_thread_to_start_valid = 0;
    switch (chosen_thread)
    {
    case 0:
        if (!pd->sched.state.running)
        {
            pd->sched.next_thread_to_start_valid = chosen_schedulable;
        }
        break;
    case 1:
    case 2:
    case 3:
        if ( (!pd->sched.state.running) ||
             ((pd->sched.state.thread==0) && !pd->special.state.cooperative) )
        {
            pd->sched.next_thread_to_start_valid = chosen_schedulable;
        }
        break;
    case 4:
    case 5:
    case 6:
    case 7:
        if ( (!pd->sched.state.running) ||
             (!(pd->sched.state.thread&4) && !pd->special.state.cooperative) )
        {
            pd->sched.next_thread_to_start_valid = chosen_schedulable;
        }
        break;
    }

    pd->sched.thread_data_to_read = pd->special.state.selected_thread;
    if (pd->sched.next_thread_to_start_valid && !(pd->sched.state.thread_to_start_valid))
    {
        pd->sched.thread_data_to_read = pd->sched.next_thread_to_start;
    }
    pd->sched.thread_data_pc = pd->sched.state.thread_data[ pd->sched.thread_data_to_read ].restart_pc;
    pd->sched.thread_data_config = pd->sched.state.thread_data[ pd->sched.thread_data_to_read ].config;
}

/*f c_gip_full::sched_preclock
 */
void c_gip_full::sched_preclock( void )
{
    /*b Copy current to next
     */
    memcpy( &pd->sched.next_state, &pd->sched.state, sizeof(pd->sched.state) );

    /*b Store thread and running
     */
    pd->sched.next_state.thread_to_start = pd->sched.state.thread_to_start; // Ensure the bus is held constant unless we are raising request
    if (pd->dec.state.acknowledge_scheduler)
    {
        pd->sched.next_state.thread = pd->sched.next_thread_to_start;
        pd->sched.next_state.thread_data[ pd->sched.next_state.thread_to_start ].running = 1;
        pd->sched.next_state.running = 1;
    }
    pd->sched.next_state.thread_to_start_valid = pd->sched.next_thread_to_start_valid;
    if (pd->sched.next_state.thread_to_start_valid && !(pd->sched.state.thread_to_start_valid))
    {
        pd->sched.next_state.thread_to_start = pd->sched.next_thread_to_start;
        pd->sched.next_state.thread_to_start_pc = pd->sched.thread_data_pc;
        pd->sched.next_state.thread_to_start_config = pd->sched.thread_data_config;
        pd->sched.next_state.thread_to_start_level = 0;
    }

    /*b Write thread data register file
     */
    if (pd->special.state.thread_data_write_pc)
    {
        pd->sched.next_state.thread_data[ pd->special.state.write_thread ].restart_pc = pd->special.state.thread_data_pc;
        pd->sched.next_state.thread_data[ pd->special.state.write_thread ].running = 0;
    }
    if (pd->special.state.thread_data_write_config)
    {
        pd->sched.next_state.thread_data[ pd->special.state.write_thread ].config = pd->special.state.thread_data_config;
    }
}

/*f c_gip_full::sched_clock
 */
void c_gip_full::sched_clock( void )
{
    /*b Debug
     */
    if (pd->verbose)
    {
        printf( "\t**:SCH %d/%d @ %08x/%02x/%d (%08x)\n",
                pd->sched.state.thread_to_start_valid,
                pd->sched.state.thread_to_start,
                pd->sched.state.thread_to_start_pc,
                pd->sched.state.thread_to_start_config,
                pd->sched.state.thread_to_start_level,
                pd->special.state.semaphores
            );
    }

    /*b Copy next to current
     */
    memcpy( &pd->sched.state, &pd->sched.next_state, sizeof(pd->sched.state) );

}

/*a Decode stage methods
 */
/*f c_gip_full::dec_comb
 */
void c_gip_full::dec_comb( void )
{
    char buffer[256];
    unsigned int native_opcode;

    /*b Fake the prefetch of the instruction
     */
    if ( pd->dec.state.pc_valid && pd->dec.state.fetch_requested)
    {
        pd->dec.state.opcode = pd->memory->read_memory( pd->dec.state.pc );
    }

    /*b Native decode - use top or bottom half of instruction (pc bit 1 is set AND native AND valid)
     */
    build_gip_instruction_nop( &pd->dec.native.inst );
    pd->dec.gip_ins_cc = gip_ins_cc_always;
    if (pd->dec.state.in_conditional_shadow)
    {
        pd->dec.gip_ins_cc = gip_ins_cc_cp;
    }
    pd->dec.native.inst_valid = 0;
    native_opcode = pd->dec.state.opcode;
    if ( (!pd->dec.state.pc_valid) ||
         (pd->dec.state.op_state!=gip_dec_op_state_native) ||
         ((pd->dec.state.pc&2)==0) )
    {
        native_opcode = native_opcode & 0xffff;
    }
    else
    {
        native_opcode = native_opcode>>16;
    }
    pd->dec.native.next_pc = pd->dec.state.pc;
    pd->dec.native.next_cycle_of_opcode = 0;
    pd->dec.native.next_in_delay_slot = 0;
    pd->dec.native.next_follow_delay_pc = 0;
    pd->dec.native.next_in_immediate_shadow = 0;
    pd->dec.native.next_extended_immediate = pd->dec.state.extended_immediate;
    pd->dec.native.extending = 0;
    if ( decode_native_debug( native_opcode ) ||
         decode_native_extend( native_opcode ) ||
         decode_native_alu( native_opcode ) ||
         decode_native_cond( native_opcode ) ||
         decode_native_shift( native_opcode ) ||
         decode_native_ldr( native_opcode ) ||
         decode_native_str( native_opcode ) ||
         decode_native_branch( native_opcode ) ||
         0 )
    {
        pd->dec.native.inst_valid = !pd->dec.native.extending;
    }
    if (pd->dec.state.follow_delay_pc)
    {
        pd->dec.native.next_pc = pd->dec.state.delay_pc;
    }
    if ((!pd->dec.native.extending) || (pd->gip_pipeline_results.flush) )
    {
        pd->dec.native.next_extended_immediate = 0;
        pd->dec.native.next_extended_cmd.extended = 0;
        pd->dec.native.next_extended_rd.type = gip_ins_r_type_no_override;
        pd->dec.native.next_extended_rn.type = gip_ins_r_type_no_override;
        pd->dec.native.next_extended_rm.type = gip_ins_r_type_no_override;
    }
    if (!pd->dec.state.pc_valid)
    {
        pd->dec.native.next_pc = pd->dec.state.pc;
        pd->dec.native.next_cycle_of_opcode = 0;
        pd->dec.native.inst_valid = 0;
    }
    if (pd->gip_pipeline_results.write_pc)
    {
        pd->dec.native.next_pc = pd->gip_pipeline_results.rfw_data;
    }
    if (pd->gip_pipeline_results.flush)
    {
        pd->dec.native.inst_valid = 0;
        pd->dec.native.next_in_delay_slot = 0;
        pd->dec.native.next_follow_delay_pc = 0;
        pd->dec.native.next_in_immediate_shadow = 0;
    }

    /*b ARM decode - attempt to decode opcode, trying each instruction coding one at a time till we succeed
     */
    build_gip_instruction_nop( &pd->dec.arm.inst );
    pd->dec.arm.inst_valid = 0;
    pd->dec.arm.next_acc_valid = 0;
    pd->dec.arm.next_reg_in_acc = pd->dec.state.reg_in_acc;
    pd->dec.arm.next_pc = pd->dec.state.pc;
    pd->dec.arm.next_cycle_of_opcode = pd->dec.state.cycle_of_opcode+1;
    if ( decode_arm_debug() ||
         decode_arm_mul() ||
         decode_arm_alu() ||
         decode_arm_ld_st() ||
         decode_arm_ldm_stm() ||
         decode_arm_branch() ||
         decode_arm_trace() ||
         0 )
    {
        pd->dec.arm.inst_valid = 1;
    }
    if ( (pd->dec.arm.inst.gip_ins_rn.type == gip_ins_r_type_register) &&
         (pd->dec.arm.inst.gip_ins_rn.data.r == pd->dec.state.reg_in_acc) &&
         (pd->dec.state.acc_valid) )
    {
        pd->dec.arm.inst.gip_ins_rn.type = gip_ins_r_type_internal;
        pd->dec.arm.inst.gip_ins_rn.data.rnm_internal = gip_ins_rnm_int_acc;
    }
    if ( (!pd->dec.arm.inst.rm_is_imm) &&
         (pd->dec.arm.inst.rm_data.gip_ins_rm.type == gip_ins_r_type_register) &&
         (pd->dec.arm.inst.rm_data.gip_ins_rm.data.r == pd->dec.state.reg_in_acc) &&
         (pd->dec.state.acc_valid) )
    {
        pd->dec.arm.inst.rm_data.gip_ins_rm.type = gip_ins_r_type_internal;
        pd->dec.arm.inst.rm_data.gip_ins_rm.data.rnm_internal = gip_ins_rnm_int_acc;
    }
    if (!pd->dec.state.pc_valid)
    {
        pd->dec.arm.next_pc = pd->dec.state.pc;
        pd->dec.arm.next_cycle_of_opcode = 0;
        pd->dec.arm.inst_valid = 0;
    }
    if (pd->gip_pipeline_results.write_pc)
    {
        pd->dec.arm.next_pc = pd->gip_pipeline_results.rfw_data;
    }
    if (pd->gip_pipeline_results.flush)
    {
        pd->dec.arm.inst_valid = 0;
    }

    /*b Handle according to operating state
     */
    pd->dec.next_op_state = pd->dec.state.op_state;
    switch (pd->dec.state.op_state)
    {
        /*b Idle - wait for schedule request
         */
    case gip_dec_op_state_idle:
        pd->dec.idle.next_pc_valid = 0;
        pd->dec.next_acknowledge_scheduler = 0;
        if (pd->sched.state.thread_to_start_valid)
        {
            pd->dec.next_op_state = gip_dec_op_state_emulate;
            pd->dec.next_op_state = gip_dec_op_state_native;
            pd->dec.next_acknowledge_scheduler = 1;
            pd->dec.idle.next_thread = pd->sched.state.thread_to_start;
            pd->dec.idle.next_pc = pd->sched.state.thread_to_start_pc;
            pd->dec.idle.next_pc_valid = 1;
            pd->dec.arm.next_cycle_of_opcode=0;
            pd->dec.arm.next_acc_valid = 0;
        }
        break;
        /*b ARM mode
         */
    case gip_dec_op_state_emulate:
        /*b Disassemble instruction if verbose
         */
        if ( pd->dec.state.pc_valid && (pd->dec.state.cycle_of_opcode==0))
        {
            if (pd->verbose)
            {
                arm_disassemble( pd->dec.state.pc, pd->dec.state.opcode, buffer );
                printf( "%08x %08x: %s\n", pd->dec.state.pc, pd->dec.state.opcode, buffer );
            }
        }
        /*b Select ARM decode as our decode (NEED TO MUX IN NATIVE IF ENCODED NATIVE - BUT THEN STILL USE OUR CC)
         */
        pd->dec.inst = pd->dec.arm.inst;
        pd->dec.inst_valid = pd->dec.arm.inst_valid;
        break;
        /*b Native - decode top or bottom half of instruction (pc bit 1 is set AND native AND valid)
         */
    case gip_dec_op_state_native:
        /*b Disassemble instruction if verbose
         */
        if ( pd->dec.state.pc_valid && (pd->dec.state.cycle_of_opcode==0))
        {
            if (pd->verbose)
            {
                disassemble_native_instruction( pd->dec.state.pc, native_opcode, buffer, sizeof(buffer) );
                printf( "%08x %04x: %s (native)\n", pd->dec.state.pc, native_opcode, buffer );
            }
        }
        /*b Select native instruction as our decode
         */
        pd->dec.inst = pd->dec.native.inst;
        pd->dec.inst_valid = pd->dec.native.inst_valid;
        break;
        /*b Preempt - NOT WRITTEN YET
         */
    case gip_dec_op_state_preempt:
        break;
        /*b All done
         */
    }

}

/*f c_gip_full::dec_preclock
 */
void c_gip_full::dec_preclock( void )
{
    /*b Copy current to next
     */
    memcpy( &pd->dec.next_state, &pd->dec.state, sizeof(pd->dec.state) );

    /*b Handle according to operating state
     */
    pd->dec.next_state.acknowledge_scheduler = 0;
    switch (pd->dec.state.op_state)
    {
        /*b Idle - wait for schedule request
         */
    case gip_dec_op_state_idle:
        pd->dec.next_state.pc = pd->dec.idle.next_pc;
        pd->dec.next_state.pc_valid = pd->dec.idle.next_pc_valid;
        pd->dec.next_state.cycle_of_opcode = 0;
        pd->dec.next_state.acc_valid = pd->dec.arm.next_acc_valid;
        pd->dec.next_state.op_state = pd->dec.next_op_state;
        pd->dec.next_state.acknowledge_scheduler = pd->dec.next_acknowledge_scheduler;
        pd->dec.next_state.in_conditional_shadow = 0;
        pd->dec.next_state.in_immediate_shadow = 0;
            pd->dec.next_state.extended_immediate = 0;
            pd->dec.next_state.extended_cmd.extended = 0;
            pd->dec.next_state.extended_rd.type = gip_ins_r_type_no_override;
            pd->dec.next_state.extended_rn.type = gip_ins_r_type_no_override;
            pd->dec.next_state.extended_rm.type = gip_ins_r_type_no_override;
        printf("Idle %d %d %d\n", pd->dec.next_op_state, pd->dec.next_acknowledge_scheduler, pd->sched.state.thread_to_start_valid );
        break;
    case gip_dec_op_state_emulate:
        /*b ARM - Move on PC
         */
        if ( (pd->rf.accepting_dec_instruction) ||
             (pd->rf.accepting_dec_instruction_if_alu_does && pd->alu.accepting_rf_instruction) )
        {
            pd->dec.next_state.pc = pd->dec.arm.next_pc;
            pd->dec.next_state.cycle_of_opcode = pd->dec.arm.next_cycle_of_opcode;
            pd->dec.next_state.acc_valid = pd->dec.arm.next_acc_valid;
            pd->dec.next_state.reg_in_acc = pd->dec.arm.next_reg_in_acc;
        }
        if (pd->gip_pipeline_results.write_pc)
        {
            pd->dec.next_state.pc = pd->gip_pipeline_results.rfw_data;
            pd->dec.next_state.pc_valid = 1;
        }
        if (pd->gip_pipeline_results.flush)
        {
            pd->dec.next_state.cycle_of_opcode=0;
            pd->dec.next_state.pc_valid = pd->gip_pipeline_results.write_pc;
            pd->dec.next_state.acc_valid = 0;
        }
        pd->dec.next_state.in_delay_slot = 0;
        pd->dec.next_state.follow_delay_pc = 0;
        pd->dec.next_state.delay_pc = 0;
        pd->dec.next_state.in_conditional_shadow = 0;
        pd->dec.next_state.in_immediate_shadow = 0;
            pd->dec.next_state.extended_immediate = 0;
            pd->dec.next_state.extended_cmd.extended = 0;
            pd->dec.next_state.extended_rd.type = gip_ins_r_type_no_override;
            pd->dec.next_state.extended_rn.type = gip_ins_r_type_no_override;
            pd->dec.next_state.extended_rm.type = gip_ins_r_type_no_override;
        break;
        /*b Native
         */
    case gip_dec_op_state_native:
        if ( (pd->rf.accepting_dec_instruction) ||
             (pd->rf.accepting_dec_instruction_if_alu_does && pd->alu.accepting_rf_instruction) )
        {
            pd->dec.next_state.pc = pd->dec.native.next_pc;
            pd->dec.next_state.cycle_of_opcode = pd->dec.native.next_cycle_of_opcode;
            pd->dec.next_state.acc_valid = pd->dec.arm.next_acc_valid;
            pd->dec.next_state.reg_in_acc = pd->dec.arm.next_reg_in_acc;
            pd->dec.next_state.in_delay_slot = pd->dec.native.next_in_delay_slot;
            pd->dec.next_state.follow_delay_pc = pd->dec.native.next_follow_delay_pc;
            pd->dec.next_state.delay_pc = pd->dec.native.next_delay_pc;
            pd->dec.next_state.extended_immediate = pd->dec.native.next_extended_immediate;
            pd->dec.next_state.extended_cmd.extended = pd->dec.native.next_extended_cmd.extended;
            pd->dec.next_state.extended_rd = pd->dec.native.next_extended_rd;
            pd->dec.next_state.extended_rn = pd->dec.native.next_extended_rn;
            pd->dec.next_state.extended_rm = pd->dec.native.next_extended_rm;
            pd->dec.next_state.in_conditional_shadow = 0;
            pd->dec.next_state.in_immediate_shadow = 0;
            if (pd->dec.native.next_in_immediate_shadow)
            {
                pd->dec.next_state.in_conditional_shadow = 1;
                pd->dec.next_state.in_immediate_shadow = 1;
            }
            if ( (pd->dec.state.in_immediate_shadow) && (pd->cp_trail_2) )
            {
                pd->dec.next_state.in_conditional_shadow = 1;
            }
        }
        if (pd->gip_pipeline_results.write_pc)
        {
            pd->dec.next_state.pc = pd->gip_pipeline_results.rfw_data;
            pd->dec.next_state.pc_valid = 1;
        }
        if (pd->gip_pipeline_results.flush)
        {
            pd->dec.next_state.cycle_of_opcode=0;
            pd->dec.next_state.pc_valid = pd->gip_pipeline_results.write_pc;
            pd->dec.next_state.acc_valid = 0;
            pd->dec.next_state.in_conditional_shadow = 0;
            pd->dec.next_state.extended_immediate = 0;
            pd->dec.next_state.extended_cmd.extended = 0;
            pd->dec.next_state.extended_rd.type = gip_ins_r_type_no_override;
            pd->dec.next_state.extended_rn.type = gip_ins_r_type_no_override;
            pd->dec.next_state.extended_rm.type = gip_ins_r_type_no_override;
        }
        break;
        /*b Preempt - NOT WRITTEN YET
         */
    case gip_dec_op_state_preempt:
        break;
        /*b All done
         */
    }
    pd->dec.next_state.fetch_requested=0;
    if (pd->dec.next_state.cycle_of_opcode==0)
    {
        pd->dec.next_state.fetch_requested=1;
    }
}

/*f c_gip_full::dec_clock
 */
void c_gip_full::dec_clock( void )
{
    /*b Debug
     */
    if (pd->verbose)
    {
        char buffer[256];
        disassemble_int_instruction( pd->dec.inst_valid, &pd->dec.inst, buffer, sizeof(buffer) );
        printf( "\t**:DEC %08x(%d)/%08x (ARM c %d ACC %d/%02d)\t:\t... %s\n",
                pd->dec.state.pc,
                pd->dec.state.pc_valid,
                pd->dec.state.opcode,
                pd->dec.state.cycle_of_opcode,
                pd->dec.state.acc_valid,
                pd->dec.state.reg_in_acc,
                buffer
            );
    }

    /*b Handle simulation debug instructions
     */
    if ( (pd->rf.accepting_dec_instruction) ||
         (pd->rf.accepting_dec_instruction_if_alu_does && pd->alu.accepting_rf_instruction) )
    {
        if ( (pd->dec.state.pc_valid) &&
             ((pd->dec.state.opcode&0xff000000)==0xf0000000) )
        {
            switch (pd->dec.state.opcode&0xff)
            {
            case 0x90:
                printf( "********************************************************************************\nTest passed\n********************************************************************************\n\n");
                break;
            case 0x91:
                printf( "********************************************************************************\n--------------------------------------------------------------------------------\nTest failed\n--------------------------------------------------------------------------------\n\n");
                break;
            case 0xa0:
                char buffer[256];
                pd->memory->copy_string( buffer, pd->rf.state.regs[0], sizeof(buffer) );
                printf( buffer, pd->rf.state.regs[1], pd->rf.state.regs[2], pd->rf.state.regs[3] );
                break;
            case 0xa1:
                printf( "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\nDump regs\n");
                debug(-1);
                break;
            case 0xa2:
                printf( "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\nVerbose on\n");
                pd->verbose = 1;
                break;
            case 0xa3:
                printf( "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\nVerbose off\n");
                pd->verbose = 0;
                break;
            }
        }
    }

    /*b Copy current to next
     */
    memcpy( &pd->dec.state, &pd->dec.next_state, sizeof(pd->dec.state) );

}

/*a Register file operation
 */
/*f rf_read_int_register
 */
static unsigned int rf_read_int_register( t_gip_rf_data *rf, unsigned int pc, t_gip_ins_r r )
{
    /*b Simple forwarding path through register file, or read register file itself
     */
    if (r.type==gip_ins_r_type_register)
    {
        if ( (rf->state.mem_rd.type==gip_ins_r_type_register) &&
             (r.data.r==rf->state.mem_rd.data.r) )
        {
            return rf->state.mem_result;
        }
        if ( (rf->state.alu_rd.type==gip_ins_r_type_register) &&
             (r.data.r==rf->state.alu_rd.data.r) )
        {
            return rf->state.use_shifter ? rf->state.shf_result : rf->state.alu_result;
        }
        return rf->state.regs[r.data.r&0x1f];
    }

    /*b For internal results give the PC - other possibles are ACC and SHF, which will be replaced in ALU stage anyway
     */
    return pc;
}

/*f c_gip_full::rf_comb
 */
void c_gip_full::rf_comb( t_gip_pipeline_results *results )
{

    /*b Determine whether to accept another instruction from decode
     */
    pd->rf.accepting_dec_instruction = 0;
    pd->rf.accepting_dec_instruction_if_alu_does = 0;
    if (pd->rf.state.inst_valid)
    {
        pd->rf.accepting_dec_instruction_if_alu_does = 1;
        if (pd->rf.state.inst.gip_ins_rn.type==gip_ins_r_type_register)
        {
            if ( pd->alu.state.inst_valid &&
                 (pd->alu.state.inst.gip_ins_rd.type==gip_ins_r_type_register) &&
                 (pd->rf.state.inst.gip_ins_rn.data.r==pd->alu.state.inst.gip_ins_rd.data.r) )
            {
                pd->rf.accepting_dec_instruction_if_alu_does = 0;
            }
            if ( (pd->mem.state.mem_rd.type==gip_ins_r_type_register) &&
                 (pd->rf.state.inst.gip_ins_rn.data.r==pd->mem.state.mem_rd.data.r) )
            {
                pd->rf.accepting_dec_instruction_if_alu_does = 0;
            }
            if ( (pd->rf.state.mem_rd.type!=gip_ins_r_type_none) &&
                 (pd->rf.state.alu_rd.type==gip_ins_r_type_register) &&
                 (pd->rf.state.inst.gip_ins_rn.data.r==pd->rf.state.alu_rd.data.r) )
            {
                pd->rf.accepting_dec_instruction_if_alu_does = 0;
            }
        }
        if (pd->rf.state.inst.rm_data.gip_ins_rm.type==gip_ins_r_type_register)
        {
            if ( pd->alu.state.inst_valid &&
                 (pd->alu.state.inst.gip_ins_rd.type==gip_ins_r_type_register) &&
                 (pd->rf.state.inst.rm_data.gip_ins_rm.data.r==pd->alu.state.inst.gip_ins_rd.data.r) )
            {
                pd->rf.accepting_dec_instruction_if_alu_does = 0;
            }
            if ( (pd->mem.state.mem_rd.type==gip_ins_r_type_register) &&
                 (pd->rf.state.inst.rm_data.gip_ins_rm.data.r==pd->mem.state.mem_rd.data.r) )
            {
                pd->rf.accepting_dec_instruction_if_alu_does = 0;
            }
            if ( (pd->rf.state.mem_rd.type!=gip_ins_r_type_none) &&
                 (pd->rf.state.alu_rd.type==gip_ins_r_type_register) &&
                 (pd->rf.state.inst.rm_data.gip_ins_rm.data.r==pd->rf.state.alu_rd.data.r) )
            {
                pd->rf.accepting_dec_instruction_if_alu_does = 0;
            }
        }
    }
    else
    {
        pd->rf.accepting_dec_instruction = 1;
    }

    /*b If a deschedule then execute IF condition passed UNLESS flushing in ALU stage OR ALU is not taking instruction
      the instruction should not be in the pipeline at this stage if there is any other earlier instruction writing
      the PC. This will be true in general as instructions that write the PC will cause a flush. However, a
      deschedule in the shadow of a branch (in a delay slot particularly) would not be flushed.
    */
    printf("decode handles deschedule - if condition passed unless flushing in ALU stage or ALU is not taking instructoin; must not be here if any previous instruction is writing PC\n");

    /*b Read the register file
     */
    pd->rf.read_port_0 = rf_read_int_register( &pd->rf, pd->rf.state.inst.pc, pd->rf.state.inst.gip_ins_rn ); // Read register, with forwarding
    pd->rf.read_port_1 = rf_read_int_register( &pd->rf, pd->rf.state.inst.pc, pd->rf.state.inst.rm_data.gip_ins_rm ); // Read register, with forwarding

    /*b Read the special and postbus data
     */
    pd->rf.postbus_read = 0;
    pd->rf.postbus_read_address = pd->rf.state.inst.gip_ins_rn.data.r;
    pd->rf.special_read = 0;
    pd->rf.special_read_address = pd->rf.state.inst.gip_ins_rn.data.r;
    switch (pd->rf.state.inst.gip_ins_rn.type)
    {
    case gip_ins_r_type_postbus:
        pd->rf.postbus_read = 1;
        pd->rf.postbus_read_address = pd->rf.state.inst.gip_ins_rn.data.r;
        break;
    case gip_ins_r_type_special:
        pd->rf.special_read = 1;
        pd->rf.special_read_address = pd->rf.state.inst.gip_ins_rn.data.r;
        break;
    default:
        break;
    }
    switch (pd->rf.state.inst.rm_data.gip_ins_rm.type)
    {
    case gip_ins_r_type_postbus:
        pd->rf.postbus_read = 1;
        pd->rf.postbus_read_address |= pd->rf.state.inst.rm_data.gip_ins_rm.data.r;
        break;
    case gip_ins_r_type_special:
        pd->rf.special_read = 1;
        pd->rf.special_read_address |= pd->rf.state.inst.rm_data.gip_ins_rm.data.r;
        break;
    default:
        break;
    }

    /*b Call the postbus and special register combinatorial functions!
     */
    postbus_comb( pd->rf.postbus_read, pd->rf.postbus_read_address, &pd->rf.postbus_read_data );
    special_comb( pd->rf.special_read, pd->rf.special_read_address, &pd->rf.special_read_data );

    /*b Multiplex the read data - can only be done once special, postbus and periph comb functions are called, so we have called them!
     */
    switch (pd->rf.state.inst.gip_ins_rn.type)
    {
    case gip_ins_r_type_special:
        pd->rf.port_0_data = pd->rf.postbus_read_data;
        break;
    case gip_ins_r_type_postbus:
        pd->rf.port_0_data = pd->rf.special_read_data;
        break;
    default:
        pd->rf.port_0_data = pd->rf.read_port_0;
        break;
    }
    switch (pd->rf.state.inst.rm_data.gip_ins_rm.type)
    {
    case gip_ins_r_type_special:
        pd->rf.port_1_data = pd->rf.postbus_read_data;
        break;
    case gip_ins_r_type_postbus:
        pd->rf.port_1_data = pd->rf.special_read_data;
        break;
    default:
        pd->rf.port_1_data = pd->rf.read_port_1;
        break;
    }

    /*b Register file writeback
     */
    pd->rf.rfw_data =  pd->rf.state.use_shifter ? pd->rf.state.shf_result : pd->rf.state.alu_result;
    results->write_pc = 0;
    pd->rf.rd = pd->rf.state.alu_rd;
    if (pd->rf.state.mem_rd.type!=gip_ins_r_type_none)
    {
        pd->rf.rd = pd->rf.state.mem_rd;
        pd->rf.rfw_data = pd->rf.state.mem_result;
    }
    if ( (pd->rf.rd.type==gip_ins_r_type_internal) &&
         (pd->rf.rd.data.rd_internal==gip_ins_rd_int_pc) )
    {
        results->write_pc = 1;
    }
    results->rfw_data = pd->rf.rfw_data;
}

/*f c_gip_full::rf_preclock
 */
void c_gip_full::rf_preclock( void )
{
    /*b Copy current to next
     */
    memcpy( &pd->rf.next_state, &pd->rf.state, sizeof(pd->rf.state) );

    /*b Pipeline instruction for RF read stage
     */
    if ( (pd->rf.accepting_dec_instruction) ||
         (pd->rf.accepting_dec_instruction_if_alu_does && pd->alu.accepting_rf_instruction) )
    {
        pd->rf.next_state.inst = pd->dec.inst;
        pd->rf.next_state.inst_valid = pd->dec.inst_valid;
        if (pd->gip_pipeline_results.flush)
        {
            pd->rf.next_state.inst_valid = 0;
        }
    }
    else // Not willing to take another instruction; we must have one on hold - flush if asked AND if it is not 'D'
    {
        if ( pd->gip_pipeline_results.flush && !pd->rf.state.inst.d )
        {
            pd->rf.next_state.inst_valid = 0;
        }
    }

    /*b Record RFW stage results for the write stage
     */
    if (pd->rf.state.accepting_alu_rd)
    {
        pd->rf.next_state.alu_rd = pd->alu.alu_rd;
        pd->rf.next_state.alu_result = pd->alu.alu_result;
        pd->rf.next_state.use_shifter = pd->alu.use_shifter;
        pd->rf.next_state.shf_result = pd->alu.shf_result;
    }
    pd->rf.next_state.mem_rd = pd->mem.state.mem_rd;
    pd->rf.next_state.mem_result = pd->mem.mem_result;
    pd->rf.next_state.accepting_alu_rd = 0;
    if (pd->rf.next_state.alu_rd.type==gip_ins_r_type_none)
    {
        pd->rf.next_state.accepting_alu_rd = 1;
    }
    if (pd->rf.next_state.mem_rd.type==gip_ins_r_type_none)
    {
        pd->rf.next_state.accepting_alu_rd = 1;
    }

    /*b Register file writeback operation
     */
    pd->rf.postbus_write = 0;
    pd->rf.special_write = 0;
    switch (pd->rf.rd.type)
    {
    case gip_ins_r_type_register:
        pd->rf.next_state.regs[ pd->rf.rd.data.r&0x1f ] = pd->rf.rfw_data;
        break;
    case gip_ins_r_type_special:
        pd->rf.special_write = 1;
        break;
    case gip_ins_r_type_postbus:
        pd->rf.postbus_write = 1;
        break;
    default:
        break;
    }

    /*b Call the postbus and special register preclocks
     */
    postbus_preclock( pd->gip_pipeline_results.flush, pd->rf.postbus_read, pd->rf.postbus_read_address, pd->rf.postbus_write, pd->rf.rd.data.r, pd->rf.rfw_data );
    special_preclock( pd->gip_pipeline_results.flush, pd->rf.special_read, pd->rf.special_read_address, pd->rf.special_write, pd->rf.rd.data.r, pd->rf.rfw_data );

}

/*f c_gip_full::rf_clock
 */
void c_gip_full::rf_clock( void )
{
    /*b Debug
     */
    if (pd->verbose)
    {
        char buffer[256];
        disassemble_int_instruction( pd->rf.state.inst_valid, &pd->rf.state.inst, buffer, sizeof(buffer) );
        printf( "\t**:RFR IV %d P0 (%d/%02x) %08x P1 (%d/%02x) %08x Rd %d/%02x\t:\t...%s\n",
                pd->rf.state.inst_valid,
                pd->rf.state.inst.gip_ins_rn.type,
                pd->rf.state.inst.gip_ins_rn.data.r,
                pd->rf.read_port_0,
                pd->rf.state.inst.rm_data.gip_ins_rm.type,
                pd->rf.state.inst.rm_data.gip_ins_rm.data.r,
                pd->rf.read_port_1,
                pd->rf.state.inst.gip_ins_rd.type,
                pd->rf.state.inst.gip_ins_rd.data.r,
                buffer
            );
        printf("\t**:RFW ALU %d/%02x %08x SHF %d/%08x MEM %d/%02x %08x\n",
               pd->rf.state.alu_rd.type,
               pd->rf.state.alu_rd.data.r,
               pd->rf.state.alu_result,
               pd->rf.state.use_shifter,
               pd->rf.state.shf_result,
               pd->rf.state.mem_rd.type,
               pd->rf.state.mem_rd.data.r,
               pd->rf.state.mem_result );
    }

    /*b Copy next to current
     */
    memcpy( &pd->rf.state, &pd->rf.next_state, sizeof(pd->rf.state) );

    /*b Clock postbs and special
     */
    postbus_clock( );
    special_clock( );

}

/*a ALU methods
 */
/*f c_gip_full::alu_comb
 */
void c_gip_full::alu_comb( t_gip_pipeline_results *results )
{
    int writes_conditional;
    int conditional_result;

    /*b Evaluate condition associated with the instruction - simultaneous with ALU stage, blocks all results from instruction if it fails
     */
    pd->alu.condition_passed = 0;
    if (pd->alu.state.inst_valid)
    {
        pd->alu.condition_passed = is_condition_met( pd, &pd->alu, pd->alu.state.inst.gip_ins_cc );
    }

    /*b Get values for ALU operands
     */
    pd->alu.op1_src = gip_alu_op1_src_a_in;
    if ( (pd->alu.state.inst.gip_ins_rn.type==gip_ins_r_type_internal) &&
         (pd->alu.state.inst.gip_ins_rn.data.rnm_internal==gip_ins_rnm_int_acc) )
    {
        pd->alu.op1_src = gip_alu_op1_src_acc;
    }
    if (pd->alu.state.inst.rm_is_imm)
    {
        pd->alu.op2_src = gip_alu_op2_src_b_in;
    }
    else
    {
        pd->alu.op2_src = gip_alu_op2_src_b_in;
        if ( (pd->alu.state.inst.rm_data.gip_ins_rm.type==gip_ins_r_type_internal) &&
             (pd->alu.state.inst.rm_data.gip_ins_rm.data.rnm_internal==gip_ins_rnm_int_acc) )
        {
            pd->alu.op2_src = gip_alu_op2_src_acc;
        }
        if ( (pd->alu.state.inst.rm_data.gip_ins_rm.type==gip_ins_r_type_internal) &&
             (pd->alu.state.inst.rm_data.gip_ins_rm.data.rnm_internal==gip_ins_rnm_int_shf) )
        {
            pd->alu.op2_src = gip_alu_op2_src_shf;
        }
    }

    /*b Determine which flags and accumulator to set, and the ALU operation
     */
    pd->alu.set_zcvn = 0;
    pd->alu.set_p = 0;
    pd->alu.set_acc = pd->alu.state.inst.a;
    pd->alu.use_shifter = 0;
    pd->alu.gip_alu_op = gip_alu_op_add;
    switch (pd->alu.state.inst.gip_ins_class)
    {
    case gip_ins_class_arith:
        pd->alu.set_zcvn = pd->alu.state.inst.gip_ins_opts.alu.s;
        pd->alu.set_p = pd->alu.state.inst.gip_ins_opts.alu.p;
        switch (pd->alu.state.inst.gip_ins_subclass)
        {
        case gip_ins_subclass_arith_add:
            pd->alu.gip_alu_op = gip_alu_op_add;
            break;
        case gip_ins_subclass_arith_adc:
            pd->alu.gip_alu_op = gip_alu_op_adc;
            break;
        case gip_ins_subclass_arith_sub:
            pd->alu.gip_alu_op = gip_alu_op_sub;
            break;
        case gip_ins_subclass_arith_sbc:
            pd->alu.gip_alu_op = gip_alu_op_sbc;
            break;
        case gip_ins_subclass_arith_rsb:
            pd->alu.gip_alu_op = gip_alu_op_rsub;
            break;
        case gip_ins_subclass_arith_rsc:
            pd->alu.gip_alu_op = gip_alu_op_rsbc;
            break;
        case gip_ins_subclass_arith_init:
            pd->alu.gip_alu_op = gip_alu_op_init;
            break;
        case gip_ins_subclass_arith_mla:
            pd->alu.gip_alu_op = gip_alu_op_mla;
            break;
        case gip_ins_subclass_arith_mlb:
            pd->alu.gip_alu_op = gip_alu_op_mlb;
            break;
        default:
            break;
        }
        break;
    case gip_ins_class_logic:
        pd->alu.set_zcvn = pd->alu.state.inst.gip_ins_opts.alu.s;
        pd->alu.set_p = pd->alu.state.inst.gip_ins_opts.alu.p;
        switch (pd->alu.state.inst.gip_ins_subclass)
        {
        case gip_ins_subclass_logic_and:
            pd->alu.gip_alu_op = gip_alu_op_and;
            break;
        case gip_ins_subclass_logic_or:
            pd->alu.gip_alu_op = gip_alu_op_or;
            break;
        case gip_ins_subclass_logic_xor:
            pd->alu.gip_alu_op = gip_alu_op_xor;
            break;
        case gip_ins_subclass_logic_bic:
            pd->alu.gip_alu_op = gip_alu_op_bic;
            break;
        case gip_ins_subclass_logic_orn:
            pd->alu.gip_alu_op = gip_alu_op_orn;
            break;
        case gip_ins_subclass_logic_mov:
            pd->alu.gip_alu_op = gip_alu_op_mov;
            break;
        case gip_ins_subclass_logic_mvn:
            pd->alu.gip_alu_op = gip_alu_op_mvn;
            break;
        case gip_ins_subclass_logic_andcnt:
            pd->alu.gip_alu_op = gip_alu_op_and_cnt;
            break;
        case gip_ins_subclass_logic_andxor:
            pd->alu.gip_alu_op = gip_alu_op_and_xor;
            break;
        case gip_ins_subclass_logic_xorfirst:
            pd->alu.gip_alu_op = gip_alu_op_xor_first;
            break;
        case gip_ins_subclass_logic_xorlast:
            pd->alu.gip_alu_op = gip_alu_op_xor_last;
            break;
        case gip_ins_subclass_logic_bitreverse:
            pd->alu.gip_alu_op = gip_alu_op_bit_reverse;
            break;
        case gip_ins_subclass_logic_bytereverse:
            pd->alu.gip_alu_op = gip_alu_op_byte_reverse;
            break;
        default:
            break;
        }
        break;
    case gip_ins_class_shift:
        pd->alu.set_zcvn = pd->alu.state.inst.gip_ins_opts.alu.s;
        pd->alu.set_p = 0;
        pd->alu.use_shifter = 1;
        switch (pd->alu.state.inst.gip_ins_subclass)
        {
        case gip_ins_subclass_shift_lsl:
            pd->alu.gip_alu_op = gip_alu_op_lsl;
            break;
        case gip_ins_subclass_shift_lsr:
            pd->alu.gip_alu_op = gip_alu_op_lsr;
            break;
        case gip_ins_subclass_shift_asr:
            pd->alu.gip_alu_op = gip_alu_op_asr;
            break;
        case gip_ins_subclass_shift_ror:
            pd->alu.gip_alu_op = gip_alu_op_ror;
            break;
        case gip_ins_subclass_shift_ror33:
            pd->alu.gip_alu_op = gip_alu_op_ror33;
            break;
        default:
            break;
        }
        break;
    case gip_ins_class_load:
        if ((pd->alu.state.inst.gip_ins_subclass & gip_ins_subclass_memory_dirn)==gip_ins_subclass_memory_up)
        {
            pd->alu.gip_alu_op = gip_alu_op_add;
        }
        else
        {
            pd->alu.gip_alu_op = gip_alu_op_sub;
        }
        break;
    case gip_ins_class_store:
        if ((pd->alu.state.inst.gip_ins_subclass & gip_ins_subclass_memory_dirn)==gip_ins_subclass_memory_up)
        {
            pd->alu.gip_alu_op = gip_alu_op_add;
        }
        else
        {
            pd->alu.gip_alu_op = gip_alu_op_sub;
        }
        switch (pd->alu.state.inst.gip_ins_subclass & gip_ins_subclass_memory_size)
        {
        case gip_ins_subclass_memory_word:
            pd->alu.alu_constant = 4;
            break;
        case gip_ins_subclass_memory_half:
            pd->alu.alu_constant = 2;
            break;
        case gip_ins_subclass_memory_byte:
            pd->alu.alu_constant = 1;
            break;
        default:
            pd->alu.alu_constant = 0;
            break;
        }
        if (pd->alu.state.inst.gip_ins_opts.store.offset_type==1)
        {
            pd->alu.op2_src = gip_alu_op2_src_shf;
        }
        else
        {
            pd->alu.op2_src = gip_alu_op2_src_constant;
        }
        break;
    }
    if (!pd->alu.condition_passed)
    {
        pd->alu.set_acc = 0;
        pd->alu.set_zcvn = 0;
        pd->alu.set_p = 0;
    }

    /*b Determine inputs to the shifter and ALU
     */
    switch (pd->alu.op1_src)
    {
    case gip_alu_op1_src_a_in:
        pd->alu.alu_op1 = pd->alu.state.alu_a_in;
        break;
    case gip_alu_op1_src_acc:
        pd->alu.alu_op1 = pd->alu.state.acc;
        break;
    }
    switch (pd->alu.op2_src)
    {
    case gip_alu_op2_src_b_in:
        pd->alu.alu_op2 = pd->alu.state.alu_b_in;
        break;
    case gip_alu_op2_src_acc:
        pd->alu.alu_op2 = pd->alu.state.acc;
        break;
    case gip_alu_op2_src_shf:
        pd->alu.alu_op2 = pd->alu.state.shf;
        break;
    case gip_alu_op2_src_constant:
        pd->alu.alu_op2 = pd->alu.alu_constant;
        break;
    }

    /*b Perform shifter operation - operates on C, ALU A in, ALU B in: what about accumulator?
     */
    switch (pd->alu.gip_alu_op)
    {
    case gip_alu_op_lsl:
        pd->alu.shf_result = barrel_shift( pd->alu.state.c, shf_type_lsl, pd->alu.alu_op1, pd->alu.alu_op2, &pd->alu.shf_carry );
        break;
    case gip_alu_op_lsr:
        pd->alu.shf_result = barrel_shift( pd->alu.state.c, shf_type_lsr, pd->alu.alu_op1, pd->alu.alu_op2, &pd->alu.shf_carry );
        break;
    case gip_alu_op_asr:
        pd->alu.shf_result = barrel_shift( pd->alu.state.c, shf_type_asr, pd->alu.alu_op1, pd->alu.alu_op2, &pd->alu.shf_carry );
        break;
    case gip_alu_op_ror:
        pd->alu.shf_result = barrel_shift( pd->alu.state.c, shf_type_ror, pd->alu.alu_op1, pd->alu.alu_op2, &pd->alu.shf_carry );
        break;
    case gip_alu_op_ror33:
        pd->alu.shf_result = barrel_shift( pd->alu.state.c, shf_type_rrx, pd->alu.alu_op1, pd->alu.alu_op2, &pd->alu.shf_carry );
        break;
    case gip_alu_op_init:
        pd->alu.shf_result = barrel_shift( 0&pd->alu.state.c, shf_type_lsr, pd->alu.alu_op1, 0, &pd->alu.shf_carry );
        break;
    case gip_alu_op_mla:
    case gip_alu_op_mlb:
        pd->alu.shf_result = barrel_shift( pd->alu.state.c, shf_type_lsr, pd->alu.state.shf, 2, &pd->alu.shf_carry );
        break;
    case gip_alu_op_divst:
        break;
    default:
        break;
    }

    /*b Perform logical operation - operates on ALU op 1 and ALU op 2
     */
    switch (pd->alu.gip_alu_op)
    {
    case gip_alu_op_mov:
        pd->alu.logic_result = pd->alu.alu_op2;
        break;
    case gip_alu_op_mvn:
        pd->alu.logic_result = ~pd->alu.alu_op2;
        break;
    case gip_alu_op_and:
        pd->alu.logic_result = pd->alu.alu_op1 & pd->alu.alu_op2;
        break;
    case gip_alu_op_or:
        pd->alu.logic_result = pd->alu.alu_op1 | pd->alu.alu_op2;
        break;
    case gip_alu_op_xor:
        pd->alu.logic_result = pd->alu.alu_op1 ^ pd->alu.alu_op2;
        break;
    case gip_alu_op_bic:
        pd->alu.logic_result = pd->alu.alu_op1 &~ pd->alu.alu_op2;
        break;
    case gip_alu_op_orn:
        pd->alu.logic_result = pd->alu.alu_op1 |~ pd->alu.alu_op2;
        break;
    case gip_alu_op_and_cnt:
        pd->alu.logic_result = bit_count(pd->alu.alu_op1 & pd->alu.alu_op2);
        break;
    case gip_alu_op_and_xor:
        pd->alu.logic_result = (pd->alu.alu_op1 & pd->alu.alu_op2) ^ pd->alu.alu_op2;
        break;
    case gip_alu_op_xor_first:
        pd->alu.logic_result = find_bit_set(pd->alu.alu_op1 ^ pd->alu.alu_op2, -1);
        break;
    case gip_alu_op_xor_last:
        pd->alu.logic_result = find_bit_set(pd->alu.alu_op1 ^ pd->alu.alu_op2, 1);
        break;
    case gip_alu_op_bit_reverse:
        pd->alu.logic_result = ( (pd->alu.alu_op2&0xffffff00) |
                                 ((pd->alu.alu_op2&0x00000080)>>7) |
                                 ((pd->alu.alu_op2&0x00000040)>>5) |
                                 ((pd->alu.alu_op2&0x00000020)>>3) |
                                 ((pd->alu.alu_op2&0x00000010)>>1) |
                                 ((pd->alu.alu_op2&0x00000008)<<1) |
                                 ((pd->alu.alu_op2&0x00000004)<<3) |
                                 ((pd->alu.alu_op2&0x00000002)<<5) |
                                 ((pd->alu.alu_op2&0x00000001)<<7) );
        break;
    case gip_alu_op_byte_reverse:
        pd->alu.logic_result = ( ((pd->alu.alu_op2&0x000000ff)<<24) |
                                 ((pd->alu.alu_op2&0x0000ff00)<<8) |
                                 ((pd->alu.alu_op2&0x00ff0000)>>8) |
                                 ((pd->alu.alu_op2&0xff000000)>>24) );
        break;
    case gip_alu_op_init:
    case gip_alu_op_mla:
    case gip_alu_op_mlb:
    case gip_alu_op_divst:
        break;
    default:
        break;
    }
    pd->alu.logic_z = (pd->alu.logic_result==0);
    pd->alu.logic_n = ((pd->alu.logic_result&0x80000000)!=0);

    /*b Perform arithmetic operation - operates on C, ALU op 1 and ALU op 2
     */
    switch (pd->alu.gip_alu_op)
    {
    case gip_alu_op_add:
        pd->alu.arith_result = add_op( pd->alu.alu_op1, pd->alu.alu_op2, 0, &pd->alu.alu_c, &pd->alu.alu_v );
        break;
    case gip_alu_op_adc:
        pd->alu.arith_result = add_op( pd->alu.alu_op1, pd->alu.alu_op2, pd->alu.state.c, &pd->alu.alu_c, &pd->alu.alu_v );
        break;
    case gip_alu_op_sub:
        pd->alu.arith_result = add_op( pd->alu.alu_op1, ~pd->alu.alu_op2, 1, &pd->alu.alu_c, &pd->alu.alu_v );
        break;
    case gip_alu_op_sbc:
        pd->alu.arith_result = add_op( pd->alu.alu_op1, ~pd->alu.alu_op2, pd->alu.state.c, &pd->alu.alu_c, &pd->alu.alu_v );
        break;
    case gip_alu_op_rsub:
        pd->alu.arith_result = add_op( ~pd->alu.alu_op1, pd->alu.alu_op2, 1, &pd->alu.alu_c, &pd->alu.alu_v );
        break;
    case gip_alu_op_rsbc:
        pd->alu.arith_result = add_op( ~pd->alu.alu_op1, pd->alu.alu_op2, pd->alu.state.c, &pd->alu.alu_c, &pd->alu.alu_v );
        break;
    case gip_alu_op_init:
        pd->alu.arith_result = add_op( 0&pd->alu.alu_op1, pd->alu.alu_op2, 0, &pd->alu.alu_c, &pd->alu.alu_v );
        break;
    case gip_alu_op_mla:
    case gip_alu_op_mlb:
        switch ((pd->alu.state.shf&3)+pd->alu.state.p)
        {
        case 0:
            pd->alu.arith_result = add_op( pd->alu.alu_op1, 0&pd->alu.alu_op2, 0, &pd->alu.alu_c, &pd->alu.alu_v );
            pd->alu.shf_carry = 0;
            break;
        case 1:
            pd->alu.arith_result = add_op( pd->alu.alu_op1, pd->alu.alu_op2, 0, &pd->alu.alu_c, &pd->alu.alu_v );
            pd->alu.shf_carry = 0;
            break;
        case 2:
            pd->alu.arith_result = add_op( pd->alu.alu_op1, pd->alu.alu_op2<<1, 0, &pd->alu.alu_c, &pd->alu.alu_v );
            pd->alu.shf_carry = 0;
            break;
        case 3:
            pd->alu.arith_result = add_op( pd->alu.alu_op1, ~pd->alu.alu_op2, 1, &pd->alu.alu_c, &pd->alu.alu_v );
            pd->alu.shf_carry = 1;
            break;
        case 4:
            pd->alu.arith_result = add_op( pd->alu.alu_op1, 0&pd->alu.alu_op2, 0, &pd->alu.alu_c, &pd->alu.alu_v );
            pd->alu.shf_carry = 1;
            break;
        }
        break;
    case gip_alu_op_divst:
        break;
    default:
        break;
    }
    pd->alu.alu_z = (pd->alu.arith_result==0);
    pd->alu.alu_n = ((pd->alu.arith_result&0x80000000)!=0);

    /*b Determine ALU result, next accumulator, next shifter and next flags
     */
    pd->alu.next_acc = pd->alu.state.acc;
    pd->alu.next_c = pd->alu.state.c;
    pd->alu.next_z = pd->alu.state.z;
    pd->alu.next_v = pd->alu.state.v;
    pd->alu.next_n = pd->alu.state.n;
    pd->alu.next_p = pd->alu.state.p;
    pd->alu.next_shf = pd->alu.state.shf;
    switch (pd->alu.gip_alu_op)
    {
    case gip_alu_op_mov:
    case gip_alu_op_mvn:
    case gip_alu_op_and:
    case gip_alu_op_or:
    case gip_alu_op_xor:
    case gip_alu_op_bic:
    case gip_alu_op_orn:
    case gip_alu_op_and_cnt:
    case gip_alu_op_and_xor:
    case gip_alu_op_xor_first:
    case gip_alu_op_xor_last:
    case gip_alu_op_bit_reverse:
    case gip_alu_op_byte_reverse:
        pd->alu.alu_result = pd->alu.logic_result;
        if (pd->alu.set_p)
        {
            pd->alu.next_c = pd->alu.state.p;
        }
        if (pd->alu.set_zcvn)
        {
            pd->alu.next_z = pd->alu.logic_z;
            pd->alu.next_n = pd->alu.logic_n;
        }
        if (pd->alu.set_acc)
        {
            pd->alu.next_acc = pd->alu.logic_result;
        }
        break;
    case gip_alu_op_add:
    case gip_alu_op_adc:
    case gip_alu_op_sub:
    case gip_alu_op_sbc:
    case gip_alu_op_rsub:
    case gip_alu_op_rsbc:
        pd->alu.alu_result = pd->alu.arith_result;
        if (pd->alu.set_zcvn)
        {
            pd->alu.next_z = pd->alu.alu_z;
            pd->alu.next_n = pd->alu.alu_n;
            pd->alu.next_c = pd->alu.alu_c;
            pd->alu.next_v = pd->alu.alu_v;
        }
        if (pd->alu.set_acc)
        {
            pd->alu.next_acc = pd->alu.arith_result;
        }
        pd->alu.next_shf = 0;// should only be with the 'C' flag, but have not defined that yet
        break;
    case gip_alu_op_init:
    case gip_alu_op_mla:
    case gip_alu_op_mlb:
        pd->alu.alu_result = pd->alu.arith_result;
        if (pd->alu.set_zcvn)
        {
            pd->alu.next_z = (pd->alu.arith_result==0);
            pd->alu.next_n = ((pd->alu.arith_result&0x80000000)!=0);
            pd->alu.next_c = pd->alu.alu_c;
            pd->alu.next_v = pd->alu.alu_v;
        }
        if (pd->alu.set_acc)
        {
            pd->alu.next_acc = pd->alu.arith_result;
        }
        pd->alu.next_shf = pd->alu.shf_result;
        pd->alu.next_p = pd->alu.shf_carry;
        break;
    case gip_alu_op_lsl:
    case gip_alu_op_lsr:
    case gip_alu_op_asr:
    case gip_alu_op_ror:
    case gip_alu_op_ror33:
        if (pd->alu.set_zcvn)
        {
            pd->alu.next_z = (pd->alu.shf_result==0);
            pd->alu.next_n = ((pd->alu.shf_result&0x80000000)!=0);
            pd->alu.next_c = pd->alu.shf_carry;
        }
        pd->alu.next_shf = pd->alu.shf_result;
        pd->alu.next_p = pd->alu.shf_carry;
        break;
    case gip_alu_op_divst:
        break;
    }

    /*b Determine 'cp' and 'old_cp' state
     */
    writes_conditional = 0;
    if (pd->alu.state.inst.gip_ins_rd.type == gip_ins_r_type_internal) 
    {
        switch (pd->alu.state.inst.gip_ins_rd.data.rd_internal)
        {
        case gip_ins_rd_int_eq:
            conditional_result = pd->alu.alu_z;
            writes_conditional = 1;
            break;
        case gip_ins_rd_int_ne:
            conditional_result = !pd->alu.alu_z;
            writes_conditional = 1;
            break;
        case gip_ins_rd_int_cs:
            conditional_result = pd->alu.alu_c;
            writes_conditional = 1;
            break;
        case gip_ins_rd_int_cc:
            conditional_result = !pd->alu.alu_c;
            writes_conditional = 1;
            break;
        case gip_ins_rd_int_hi:
            conditional_result = pd->alu.alu_c && !pd->alu.alu_z;
            writes_conditional = 1;
            break;
        case gip_ins_rd_int_ls:
            conditional_result = !pd->alu.alu_c || pd->alu.alu_z;
            writes_conditional = 1;
            break;
        case gip_ins_rd_int_ge:
            conditional_result = (!pd->alu.alu_n && !pd->alu.alu_v) || (pd->alu.alu_n && pd->alu.alu_v);
            writes_conditional = 1;
            break;
        case gip_ins_rd_int_lt:
            conditional_result = (!pd->alu.alu_n && pd->alu.alu_v) || (pd->alu.alu_n && !pd->alu.alu_v);
            writes_conditional = 1;
            break;
        case gip_ins_rd_int_gt:
            conditional_result = ((!pd->alu.alu_n && !pd->alu.alu_v) || (pd->alu.alu_n && pd->alu.alu_v)) && !pd->alu.alu_z;
            writes_conditional = 1;
            break;
        case gip_ins_rd_int_le:
            conditional_result = (!pd->alu.alu_n && pd->alu.alu_v) || (pd->alu.alu_n && !pd->alu.alu_v) || pd->alu.alu_z;
            writes_conditional = 1;
            break;
        default:
            break;
        }
    }
    if (writes_conditional)
    {
        if (pd->alu.condition_passed) // could be first of a sequence, or later on; if first of a sequence, we must set it to our result; if later then condition should be CP for ANDing condition passed
            // if ANDing (CP is therefore set) and our result is 1, then the next result is 1; if our result is 0, then the next result is zero; so this is the same as the first condition
        {
            pd->alu.next_cp = conditional_result;
        }
        else // must be later in a sequence; condition ought to have been 'CP', so this means 'state.cp' should be zero already. No reason to make it one.
        {
            pd->alu.next_cp = 0;
        }
        pd->alu.next_old_cp = 1;
    }
    else
    {
        pd->alu.next_cp = pd->alu.condition_passed;
        pd->alu.next_old_cp = pd->alu.state.cp;
    }

    /*b Get inputs to memory stage
     */
    pd->alu.mem_address = ((pd->alu.state.inst.gip_ins_subclass & gip_ins_subclass_memory_index)==gip_ins_subclass_memory_preindex)?pd->alu.alu_result:pd->alu.alu_op1;
    pd->alu.mem_data_in = pd->alu.state.alu_b_in;

    /*b Determine next Rd for the ALU operation, and memory operation, and if the instruction is blocked
     */
    pd->alu.alu_rd.type = gip_ins_r_type_none;
    pd->alu.mem_rd.type = gip_ins_r_type_none;
    pd->alu.gip_mem_op = gip_mem_op_none;
    pd->alu.accepting_rf_instruction = 1;
    if (pd->alu.condition_passed) // This is zero if instruction is not valid, so no writeback from invalid instructions!
    {
        switch (pd->alu.state.inst.gip_ins_class)
        {
        case gip_ins_class_arith:
        case gip_ins_class_logic:
        case gip_ins_class_shift:
            pd->alu.alu_rd = pd->alu.state.inst.gip_ins_rd;
            pd->alu.accepting_rf_instruction = pd->rf.state.accepting_alu_rd;
            break;
        case gip_ins_class_store:
            pd->alu.alu_rd = pd->alu.state.inst.gip_ins_rd;
            switch (pd->alu.state.inst.gip_ins_subclass & gip_ins_subclass_memory_size)
            {
            case gip_ins_subclass_memory_word:
                pd->alu.gip_mem_op = gip_mem_op_store_word;
                break;
            case gip_ins_subclass_memory_half:
                pd->alu.gip_mem_op = gip_mem_op_store_half;
                break;
            case gip_ins_subclass_memory_byte:
                pd->alu.gip_mem_op = gip_mem_op_store_byte;
                break;
            default:
                break;
            }
            pd->alu.accepting_rf_instruction = pd->rf.state.accepting_alu_rd && 1; // GJS - ADD MEMORY BLOCK HERE
            break;
        case gip_ins_class_load:
            pd->alu.mem_rd = pd->alu.state.inst.gip_ins_rd;
            switch (pd->alu.state.inst.gip_ins_subclass & gip_ins_subclass_memory_size)
            {
            case gip_ins_subclass_memory_word:
                pd->alu.gip_mem_op = gip_mem_op_load_word;
                break;
            case gip_ins_subclass_memory_half:
                pd->alu.gip_mem_op = gip_mem_op_load_half;
                break;
            case gip_ins_subclass_memory_byte:
                pd->alu.gip_mem_op = gip_mem_op_load_byte;
                break;
            default:
                break;
            }
            pd->alu.accepting_rf_instruction = 1; // GJS - ADD MEMORY BLOCK HERE
            break;
        }
    }

    /*b Determine flush output
     */
    results->flush = pd->alu.state.inst.f;
    if (!pd->alu.condition_passed) // Also kills flush if the instruction is invalid
    {
        results->flush = 0;
    }
    if (!pd->alu.accepting_rf_instruction) // Also kill flush if we are blocked for actually completing the instruction
    {
        results->flush = 0;
    }
}

/*f c_gip_full::alu_preclock
 */
void c_gip_full::alu_preclock( void )
{
    /*b Copy current to next
     */
    memcpy( &pd->alu.next_state, &pd->alu.state, sizeof(pd->alu.state) );

    /*b Copy the instruction across
     */
    if (pd->alu.accepting_rf_instruction )
    {
        pd->alu.next_state.inst = pd->rf.state.inst;
        pd->alu.next_state.inst_valid = pd->rf.state.inst_valid; // Zero this if it does not mean to proffer the instruction...
        if (pd->rf.accepting_dec_instruction_if_alu_does==0)
        {
            pd->alu.next_state.inst_valid = 0;
        }
    }
    if (pd->gip_pipeline_results.flush)
    {
        pd->alu.next_state.inst_valid = 0;
    }

    /*b Select next values for ALU inputs based on execution blocked, or particular ALU operation (multiplies particularly)
     */
    if ( (pd->rf.state.inst_valid) &&
         !(pd->gip_pipeline_results.flush) &&
         pd->alu.accepting_rf_instruction )
    {
        if ( 0 && (pd->rf.state.inst.gip_ins_class==gip_ins_class_arith) &&
             (pd->rf.state.inst.gip_ins_subclass==gip_ins_subclass_arith_mlb) )
        {
            pd->alu.next_state.alu_a_in = pd->alu.state.alu_a_in;
        }
        else
        {
            pd->alu.next_state.alu_a_in = pd->rf.read_port_0;
        }
        if ( (pd->rf.state.inst.gip_ins_class==gip_ins_class_arith) &&
             (pd->rf.state.inst.gip_ins_subclass==gip_ins_subclass_arith_mlb) )
        {
            pd->alu.next_state.alu_b_in = pd->alu.state.alu_b_in<<2;// An MLB instruction in RF read stage implies shift left by 2; but only if it moves to the ALU stage, which it does here
        }
        else if (pd->rf.state.inst.rm_is_imm)
        {
            pd->alu.next_state.alu_b_in = pd->rf.state.inst.rm_data.immediate; // If immediate, pass immediate data in
        }
        else
        {
            pd->alu.next_state.alu_b_in = pd->rf.read_port_1; // Else read register/special
        }
    }

    /*b Update ALU result, accumulator, shifter and flags
     */
    if ( (pd->alu.accepting_rf_instruction) &&
         (pd->alu.state.inst_valid) )
    {
        pd->alu.next_state.c = pd->alu.next_c;
        pd->alu.next_state.z = pd->alu.next_z;
        pd->alu.next_state.v = pd->alu.next_v;
        pd->alu.next_state.n = pd->alu.next_n;
        pd->alu.next_state.p = pd->alu.next_p;
        pd->alu.next_state.acc = pd->alu.next_acc;
        pd->alu.next_state.shf = pd->alu.next_shf;
    }

    /*b Record condition passed indications
     */
    if ( (pd->alu.accepting_rf_instruction) &&
         (pd->alu.state.inst_valid) )
    {
        pd->alu.next_state.old_cp = pd->alu.next_old_cp;
        pd->alu.next_state.cp = pd->alu.next_cp;
    }

}

/*f c_gip_full::alu_clock
 */
void c_gip_full::alu_clock( void )
{
    /*b Debug
     */
    if (pd->verbose)
    {
        char buffer[256];
        disassemble_int_instruction( pd->alu.state.inst_valid, &pd->alu.state.inst, buffer, sizeof(buffer) );
        printf( "\t**:ALU OP %d Op1 %08x Op2 %08x CP %d A %08x B %08x R %08x ACC %08x ARd %d/%02x MRd %d/%02x\t:\t...%s\n",
                pd->alu.gip_alu_op,
                pd->alu.alu_op1,
                pd->alu.alu_op2,
                pd->alu.condition_passed,
                pd->alu.state.alu_a_in,
                pd->alu.state.alu_b_in,
                pd->alu.alu_result,
                pd->alu.state.acc,
                pd->alu.alu_rd.type,
                pd->alu.alu_rd.data.r,
                pd->alu.mem_rd.type,
                pd->alu.mem_rd.data.r,
                buffer );
    }

    /*b Copy next to current
     */
    memcpy( &pd->alu.state, &pd->alu.next_state, sizeof(pd->alu.state) );

}

/*a Memory stage methods
 */
/*f c_gip_full::mem_comb
 */
void c_gip_full::mem_comb( t_gip_pipeline_results *results )
{
    /*b No need to do anything here as the actual combinatorials derive from the previous clock edge - see the clock funcion for that operation
     */
}

/*f c_gip_full::mem_preclock
 */
void c_gip_full::mem_preclock( void )
{

    /*b Copy current to next
     */
    memcpy( &pd->mem.next_state, &pd->mem.state, sizeof(pd->mem.state) );

    /*b Record input state
     */
    pd->mem.next_state.mem_rd = pd->alu.mem_rd;
    pd->mem.next_state.gip_mem_op = pd->alu.gip_mem_op;
    pd->mem.next_state.mem_data_in = pd->alu.mem_data_in;
    pd->mem.next_state.mem_address = pd->alu.mem_address;

}

/*f c_gip_full::mem_clock
 */
void c_gip_full::mem_clock( void )
{
    int offset_in_word;
    unsigned int data;
    unsigned int address;

    /*b Debug
     */
    if (pd->verbose)
    {
        printf( "\t**:MEM OP %d at %08x with %08x Rd %d/%02x\n",
                pd->mem.state.gip_mem_op,
                pd->mem.state.mem_address,
                pd->mem.state.mem_data_in,
                pd->mem.state.mem_rd.type,
                pd->mem.state.mem_rd.data.r );
    }

    /*b Perform a memory write if required
     */
    address = pd->mem.state.mem_address;
    switch (pd->mem.state.gip_mem_op)
    {
    case gip_mem_op_store_word:
        pd->memory->write_memory( address, pd->mem.state.mem_data_in, 0xf );
        break;
    case gip_mem_op_store_half:
        offset_in_word = address&0x2;
        address &= ~2;
        pd->memory->write_memory( address, pd->mem.state.mem_data_in<<(8*offset_in_word), 3<<offset_in_word );
        break;
    case gip_mem_op_store_byte:
        offset_in_word = address&0x3;
        address &= ~3;
        pd->memory->write_memory( address, pd->mem.state.mem_data_in<<(8*offset_in_word), 1<<offset_in_word );
        break;
    default:
        break;
    }

    /*b Copy next to current
     */
    memcpy( &pd->mem.state, &pd->mem.next_state, sizeof(pd->mem.state) );

    /*b Perform a memory read if required
     */
    address = pd->mem.state.mem_address;
    switch (pd->mem.state.gip_mem_op)
    {
    case gip_mem_op_load_word:
        pd->mem.mem_result = pd->memory->read_memory( address );
        break;
    case gip_mem_op_load_half:
        data = pd->memory->read_memory( address );
        offset_in_word = address&0x2;
        data >>= 8*offset_in_word;
        pd->mem.mem_result = data;
        break;
    case gip_mem_op_load_byte:
        data = pd->memory->read_memory( address );
        offset_in_word = address&0x3;
        data >>= 8*offset_in_word;
        pd->mem.mem_result = data;
        break;
    default:
        break;
    }
}

/*a Full pipeline methods
 */
/*f c_gip_full::comb
 */
void c_gip_full::comb( void )
{
    /*b Combinatorial functions
     */
    rf_comb( &pd->gip_pipeline_results );
    alu_comb( &pd->gip_pipeline_results );
    mem_comb( &pd->gip_pipeline_results );
    sched_comb( );
    dec_comb( );
}

/*f c_gip_full::preclock
 */
void c_gip_full::preclock( void )
{
    /*b Preclock the stages
     */
    sched_preclock( );
    dec_preclock( );
    rf_preclock( );
    alu_preclock( );
    mem_preclock( );

}

/*f c_gip_full::clock
 */
void c_gip_full::clock( void )
{
    if (pd->verbose)
    {
        debug(6);
    }

    /*b Clock the stages
     */
    sched_clock( );
    dec_clock( );
    rf_clock( );
    alu_clock( );
    mem_clock( );

    /*b Done
     */
}

/*a Internal instruction building
 */
/*f c_gip_full::build_gip_instruction_nop
 */
void c_gip_full::build_gip_instruction_nop( t_gip_instruction *gip_instr )
{
    gip_instr->gip_ins_class = gip_ins_class_logic;
    gip_instr->gip_ins_subclass = gip_ins_subclass_logic_mov;
    gip_instr->gip_ins_opts.alu.s = 0;
    gip_instr->gip_ins_opts.alu.p = 0;
    gip_instr->gip_ins_cc = gip_ins_cc_always;
    gip_instr->a = 0;
    gip_instr->f = 0;
    gip_instr->d = 0;
    gip_instr->gip_ins_rn.type = gip_ins_r_type_internal;
    gip_instr->gip_ins_rn.data.rnm_internal = gip_ins_rnm_int_acc;
    gip_instr->rm_is_imm = 1;
    gip_instr->rm_data.immediate = 0;
    gip_instr->gip_ins_rd.type = gip_ins_r_type_none;
    gip_instr->pc = pd->dec.state.pc+8;
}

/*f c_gip_full::build_gip_instruction_alu
 */
void c_gip_full::build_gip_instruction_alu( t_gip_instruction *gip_instr, t_gip_ins_class gip_ins_class, t_gip_ins_subclass gip_ins_subclass, int a, int s, int p, int f )
{
    gip_instr->gip_ins_class = gip_ins_class;
    gip_instr->gip_ins_subclass = gip_ins_subclass;
    gip_instr->gip_ins_opts.alu.s = s;
    gip_instr->gip_ins_opts.alu.p = p;
    gip_instr->gip_ins_cc = gip_ins_cc_always;
    gip_instr->a = a;
    gip_instr->f = f;
    gip_instr->d = 0;
    gip_instr->gip_ins_rn.type = gip_ins_r_type_internal;
    gip_instr->gip_ins_rn.data.rnm_internal = gip_ins_rnm_int_acc;
    gip_instr->rm_is_imm = 1;
    gip_instr->rm_data.immediate = 0;
    gip_instr->gip_ins_rd.type = gip_ins_r_type_none;
    gip_instr->pc = pd->dec.state.pc+8;
}

/*f c_gip_full::build_gip_instruction_shift
 */
void c_gip_full::build_gip_instruction_shift( t_gip_instruction *gip_instr, t_gip_ins_subclass gip_ins_subclass, int s, int f )
{
    gip_instr->gip_ins_class = gip_ins_class_shift;
    gip_instr->gip_ins_subclass = gip_ins_subclass;
    gip_instr->gip_ins_opts.alu.s = s;
    gip_instr->gip_ins_opts.alu.p = 0;
    gip_instr->gip_ins_cc = gip_ins_cc_always;
    gip_instr->a = 0;
    gip_instr->f = f;
    gip_instr->d = 0;
    gip_instr->gip_ins_rn.type = gip_ins_r_type_internal;
    gip_instr->gip_ins_rn.data.rnm_internal = gip_ins_rnm_int_acc;
    gip_instr->rm_is_imm = 1;
    gip_instr->rm_data.immediate = 0;
    gip_instr->gip_ins_rd.type = gip_ins_r_type_none;
    gip_instr->pc = pd->dec.state.pc+8;
}

/*f c_gip_full::build_gip_instruction_load
 */
void c_gip_full::build_gip_instruction_load( t_gip_instruction *gip_instr, t_gip_ins_subclass gip_ins_subclass, int preindex, int up, int stack, int burst_left, int a, int f )
{
    gip_instr->gip_ins_class = gip_ins_class_load;
    if (preindex)
    {
        gip_ins_subclass = (t_gip_ins_subclass) (gip_ins_subclass | gip_ins_subclass_memory_preindex);
    }
    else
    {
        gip_ins_subclass = (t_gip_ins_subclass) (gip_ins_subclass | gip_ins_subclass_memory_postindex);
    }
    if (up)
    {
        gip_ins_subclass = (t_gip_ins_subclass) (gip_ins_subclass | gip_ins_subclass_memory_up);
    }
    else
    {
        gip_ins_subclass = (t_gip_ins_subclass) (gip_ins_subclass | gip_ins_subclass_memory_down);
    }
    gip_instr->gip_ins_subclass = gip_ins_subclass;
    gip_instr->gip_ins_opts.load.stack = stack;
    gip_instr->gip_ins_cc = gip_ins_cc_always;
    gip_instr->a = a;
    gip_instr->f = f;
    gip_instr->d = 0;
    gip_instr->k = burst_left;
    gip_instr->gip_ins_rn.type = gip_ins_r_type_internal;
    gip_instr->gip_ins_rn.data.rnm_internal = gip_ins_rnm_int_acc;
    gip_instr->rm_is_imm = 1;
    gip_instr->rm_data.immediate = 0;
    gip_instr->gip_ins_rd.type = gip_ins_r_type_none;
    gip_instr->pc = pd->dec.state.pc+8;
}

/*f c_gip_full::build_gip_instruction_store
 */
void c_gip_full::build_gip_instruction_store( t_gip_instruction *gip_instr, t_gip_ins_subclass gip_ins_subclass, int preindex, int up, int use_shift, int stack, int burst_left, int a, int f )
{
    gip_instr->gip_ins_class = gip_ins_class_store;
    if (preindex)
    {
        gip_ins_subclass = (t_gip_ins_subclass) (gip_ins_subclass | gip_ins_subclass_memory_preindex);
    }
    else
    {
        gip_ins_subclass = (t_gip_ins_subclass) (gip_ins_subclass | gip_ins_subclass_memory_postindex);
    }
    if (up)
    {
        gip_ins_subclass = (t_gip_ins_subclass) (gip_ins_subclass | gip_ins_subclass_memory_up);
    }
    else
    {
        gip_ins_subclass = (t_gip_ins_subclass) (gip_ins_subclass | gip_ins_subclass_memory_down);
    }
    gip_instr->gip_ins_subclass = gip_ins_subclass;
    gip_instr->gip_ins_opts.store.stack = stack;
    gip_instr->gip_ins_opts.store.offset_type = use_shift;
    gip_instr->gip_ins_cc = gip_ins_cc_always;
    gip_instr->a = a;
    gip_instr->f = f;
    gip_instr->d = 0;
    gip_instr->k = burst_left;
    gip_instr->gip_ins_rn.type = gip_ins_r_type_internal;
    gip_instr->gip_ins_rn.data.rnm_internal = gip_ins_rnm_int_acc;
    gip_instr->rm_is_imm = 1;
    gip_instr->rm_data.immediate = 0;
    gip_instr->gip_ins_rd.type = gip_ins_r_type_none;
    gip_instr->pc = pd->dec.state.pc+8;
}

/*f c_gip_full::build_gip_instruction_immediate
 */
void c_gip_full::build_gip_instruction_immediate( t_gip_instruction *gip_instr, unsigned int imm_val )
{
    gip_instr->rm_is_imm = 1;
    gip_instr->rm_data.immediate = imm_val;
}

/*f c_gip_full::build_gip_instruction_cc
 */
void c_gip_full::build_gip_instruction_cc( t_gip_instruction *gip_instr, t_gip_ins_cc gip_ins_cc)
{
    gip_instr->gip_ins_cc = gip_ins_cc;
}

/*f c_gip_full::build_gip_instruction_rn
 */
void c_gip_full::build_gip_instruction_rn( t_gip_instruction *gip_instr, t_gip_ins_r gip_ins_rn )
{
    gip_instr->gip_ins_rn = gip_ins_rn;
}

/*f c_gip_full::build_gip_instruction_rn_int
 */
void c_gip_full::build_gip_instruction_rn_int( t_gip_instruction *gip_instr, t_gip_ins_rnm_int gip_ins_rn_int )
{
    gip_instr->gip_ins_rn.type = gip_ins_r_type_internal;
    gip_instr->gip_ins_rn.data.rnm_internal = gip_ins_rn_int;
}

/*f c_gip_full::build_gip_instruction_rm
 */
void c_gip_full::build_gip_instruction_rm( t_gip_instruction *gip_instr, t_gip_ins_r gip_ins_rm )
{
    gip_instr->rm_is_imm = 0;
    gip_instr->rm_data.gip_ins_rm = gip_ins_rm;
}

/*f c_gip_full::build_gip_instruction_rm_int
 */
void c_gip_full::build_gip_instruction_rm_int( t_gip_instruction *gip_instr, t_gip_ins_rnm_int gip_ins_rm_int )
{
    gip_instr->rm_is_imm = 0;
    gip_instr->rm_data.gip_ins_rm.type = gip_ins_r_type_internal;
    gip_instr->rm_data.gip_ins_rm.data.rnm_internal = gip_ins_rm_int;
}

/*f c_gip_full::build_gip_instruction_rd
 */
void c_gip_full::build_gip_instruction_rd( t_gip_instruction *gip_instr, t_gip_ins_r gip_ins_rd )
{
    gip_instr->gip_ins_rd = gip_ins_rd;
}

/*f c_gip_full::build_gip_instruction_rd_int
 */
void c_gip_full::build_gip_instruction_rd_int( t_gip_instruction *gip_instr, t_gip_ins_rd_int gip_ins_rd_int )
{
    gip_instr->gip_ins_rd.type = gip_ins_r_type_internal;
    gip_instr->gip_ins_rd.data.rd_internal = gip_ins_rd_int;
}

/*f c_gip_full::build_gip_instruction_rd_reg
 */
void c_gip_full::build_gip_instruction_rd_reg( t_gip_instruction *gip_instr, int gip_ins_rd_r )
{
    gip_instr->gip_ins_rd.type = gip_ins_r_type_register;
    gip_instr->gip_ins_rd.data.r = gip_ins_rd_r;
}

/*f c_gip_full::map_condition_code
 */
t_gip_ins_cc c_gip_full::map_condition_code( int arm_cc )
{
    switch (arm_cc&0xf)
    {
    case 0x0: return gip_ins_cc_eq;
    case 0x1: return gip_ins_cc_ne;
    case 0x2: return gip_ins_cc_cs;
    case 0x3: return gip_ins_cc_cc;
    case 0x4: return gip_ins_cc_mi;
    case 0x5: return gip_ins_cc_pl;
    case 0x6: return gip_ins_cc_vs;
    case 0x7: return gip_ins_cc_vc;
    case 0x8: return gip_ins_cc_hi;
    case 0x9: return gip_ins_cc_ls;
    case 0xa: return gip_ins_cc_ge;
    case 0xb: return gip_ins_cc_lt;
    case 0xc: return gip_ins_cc_gt;
    case 0xd: return gip_ins_cc_le;
    case 0xe: return gip_ins_cc_always;
    case 0xf: return gip_ins_cc_always;
    }
    return gip_ins_cc_always;
}

/*f c_gip_full::map_source_register
 */
t_gip_ins_r c_gip_full::map_source_register( int arm_r )
{
    t_gip_ins_r result;
    result.type = gip_ins_r_type_register;
    result.data.r = arm_r;
    if (arm_r==15)
    {
        result.type = gip_ins_r_type_internal;
        result.data.rnm_internal = gip_ins_rnm_int_pc;
    }
    return result;
}

/*f c_gip_full::map_destination_register
 */
t_gip_ins_r c_gip_full::map_destination_register( int arm_rd )
{
    t_gip_ins_r result;
    result.type = gip_ins_r_type_register;
    result.data.r = arm_rd;
    if (arm_rd==15)
    {
        result.type = gip_ins_r_type_internal;
        result.data.rd_internal = gip_ins_rd_int_pc;
    }
    return result;
}

/*f c_gip_full::map_shift
 */
t_gip_ins_subclass c_gip_full::map_shift( int shf_how, int imm, int amount )
{
    t_gip_ins_subclass shift;
    switch ((t_shf_type)(shf_how))
    {
    case shf_type_lsl:
        shift = gip_ins_subclass_shift_lsl;
        break;
    case shf_type_lsr:
        shift = gip_ins_subclass_shift_lsr;
        break;
    case shf_type_asr:
        shift = gip_ins_subclass_shift_asr;
        break;
    case shf_type_ror:
        shift = gip_ins_subclass_shift_ror;
        if ((imm) && (amount==0))
        {
            shift = gip_ins_subclass_shift_ror33;
        }
        break;
    default:
        break;
    }
    return shift;
}


/*f c_gip_full::map_native_rm
 */
t_gip_ins_r c_gip_full::map_native_rm( int rm, int use_full_ext_rm )
{
    t_gip_ins_r result;
    if ( pd->dec.state.extended_rm.type!=gip_ins_r_type_no_override )
    {
        result.type = pd->dec.state.extended_rm.type;
        if (use_full_ext_rm)
        {
            result.data.r = (pd->dec.state.extended_rm.data.r&0x3f);
        }
        else
        {
            result.data.r = (pd->dec.state.extended_rm.data.r&0x30) | (rm&0xf);
        }
    }
    else
    {
        result.type = gip_ins_r_type_register;
        result.data.r = rm;
        if (rm==15)
        {
            result.type = gip_ins_r_type_internal;
            result.data.rnm_internal = gip_ins_rnm_int_acc;
        }
    }
    return result;
}

/*f c_gip_full::map_native_rn
 */
t_gip_ins_r c_gip_full::map_native_rn( int rn )
{
    t_gip_ins_r result;
    if ( pd->dec.state.extended_rn.type!=gip_ins_r_type_no_override )
    {
        result.type = pd->dec.state.extended_rn.type;
        result.data.r = pd->dec.state.extended_rn.data.r;
    }
    else
    {
        result.type = gip_ins_r_type_register;
        result.data.r = rn;
        if (rn==15)
        {
            result.type = gip_ins_r_type_internal;
            result.data.rnm_internal = gip_ins_rnm_int_acc;
        }
    }
    return result;
}

/*f c_gip_full::map_native_rd
 */
t_gip_ins_r c_gip_full::map_native_rd( int rd )
{
    t_gip_ins_r result;
    if ( pd->dec.state.extended_rd.type!=gip_ins_r_type_no_override )
    {
        result.type = pd->dec.state.extended_rd.type;
        result.data.r = pd->dec.state.extended_rd.data.r;
    }
    else
    {
        result.type = gip_ins_r_type_register;
        result.data.r = rd;
        if (rd==15)
        {
            result.type = gip_ins_r_type_internal;
            result.data.rd_internal = gip_ins_rd_int_pc;
        }
    }
    return result;
}

/*f c_gip_full::map_native_immediate
 */
unsigned int c_gip_full::map_native_immediate( int imm )
{
    unsigned int result;
    if ( pd->dec.state.extended_immediate )
    {
        result = (imm&0xf) | (pd->dec.state.extended_immediate<<4);
    }
    else
    {
        result = imm;
    }
    return result;
}

/*a Native Decode functions
 */
/*f c_gip_full::decode_native_debug
 */
int c_gip_full::decode_native_debug( unsigned int opcode )
{
    int ins_class;
    int type;

    ins_class = ((opcode>>12)&0xf);
    type = (opcode>>8)&0xf;

    if ((ins_class==0xf) && (type==0x0))
    {
        build_gip_instruction_nop( &pd->dec.native.inst );
        pd->dec.native.next_pc = pd->dec.state.pc+2;
        pd->dec.native.next_cycle_of_opcode = 0;
        return 1;
    }
    return 0;
}

/*f c_gip_full::decode_native_extend
 */
int c_gip_full::decode_native_extend( unsigned int opcode )
{
    t_gip_native_ins_class ins_class;
    int rd;
    int rm;
    int rn;
    int imm;
    int cond, sign, acc, op, burst;

    ins_class = (t_gip_native_ins_class) ((opcode>>12)&0xf);
    rd = (opcode>>4)&0xff; // full extension of rd
    rm = (opcode>>0)&0xf; // top 4 bits of rm
    rn = (opcode>>8)&0xf; // top 4 bits of rn
    imm = (opcode>>0)&0x3fff; // 14 bit immediate extension
    cond = (opcode>>8)&0x0f; // 4 bit cond
    sign = (opcode>>7)&0x01; // 1 bit sign
    acc = (opcode>>6)&0x01; // 1 bit acc
    op = (opcode>>4)&0x03; // 2 bit options
    burst = (opcode>>0)&0x0f; // 4 bit burst

    switch (ins_class)
    {
    case gip_native_ins_class_extimm_0:
    case gip_native_ins_class_extimm_1:
    case gip_native_ins_class_extimm_2:
    case gip_native_ins_class_extimm_3:
        if (pd->dec.state.extended_immediate==0)
        {
            pd->dec.native.next_extended_immediate = (imm<<18)>>18;
        }
        else
        {
            pd->dec.native.next_extended_immediate = (pd->dec.state.extended_immediate<<18) | imm;
        }
        pd->dec.native.extending = 1;
        pd->dec.native.next_pc = pd->dec.state.pc+2;
        pd->dec.native.next_cycle_of_opcode = 0;
        return 1;
    case gip_native_ins_class_extrdrm:
        pd->dec.native.next_extended_rd.type = (t_gip_ins_r_type) ((rd>>5)&7);
        pd->dec.native.next_extended_rd.data.r = (rd&0x1f);
        pd->dec.native.next_extended_rm.type = (t_gip_ins_r_type) ((rm>>1)&7);
        pd->dec.native.next_extended_rm.data.r = (rm&1)<<4;
        pd->dec.native.extending = 1;
        pd->dec.native.next_pc = pd->dec.state.pc+2;
        pd->dec.native.next_cycle_of_opcode = 0;
        return 1;
    case gip_native_ins_class_extrnrm:
        pd->dec.native.next_extended_rn.type = (t_gip_ins_r_type) ((rn>>1)&7);
        pd->dec.native.next_extended_rn.data.r = (rn&1)<<4;
        pd->dec.native.next_extended_rm.type = (t_gip_ins_r_type) ((rm>>1)&7);
        pd->dec.native.next_extended_rm.data.r = (rm&1)<<4;
        pd->dec.native.extending = 1;
        pd->dec.native.next_pc = pd->dec.state.pc+2;
        pd->dec.native.next_cycle_of_opcode = 0;
        return 1;
    case gip_native_ins_class_extcmd:
        pd->dec.native.next_extended_cmd.extended = 1;
        pd->dec.native.next_extended_cmd.cc = cond;
        pd->dec.native.next_extended_cmd.sign_or_stack = sign;
        pd->dec.native.next_extended_cmd.acc = acc;
        pd->dec.native.next_extended_cmd.op = op;
        pd->dec.native.next_extended_cmd.burst = burst;
        pd->dec.native.next_pc = pd->dec.state.pc+2;
        pd->dec.native.next_cycle_of_opcode = 0;
        return 1;
    default:
        break;
    }
    return 0;
}

/*f c_gip_full::decode_native_alu
 */
int c_gip_full::decode_native_alu( unsigned int opcode )
{
    t_gip_native_ins_class ins_class;
    t_gip_native_ins_subclass ins_subclass;
    int rd;
    int rm;
    int imm;
    int mode;
    int gip_ins_a, gip_ins_s;
    t_gip_ins_cc gip_ins_cc;
    t_gip_ins_class gip_ins_class;
    t_gip_ins_subclass gip_ins_subclass;
    t_gip_ins_r gip_ins_rd;
    t_gip_ins_r gip_ins_rn;
    t_gip_ins_r gip_ins_rm;
    unsigned int imm_val;

    ins_class = (t_gip_native_ins_class) ((opcode>>12)&0xf);
    ins_subclass = (t_gip_native_ins_subclass) ((opcode>>8)&0xf);
    rd = (opcode>>4)&0xf;
    rm = (opcode>>0)&0xf;
    imm = (opcode>>0)&0xf;

    if ( (ins_class==gip_native_ins_class_alu_reg) ||
         (ins_class==gip_native_ins_class_alu_imm) )
    {
        /*b Decode mode and ALU operations
         */
        mode = pd->alu_mode;
        gip_ins_a = 1;
        gip_ins_s = 1;
        gip_ins_cc = pd->dec.gip_ins_cc;
        if (pd->dec.state.extended_cmd.extended)
        {
            if (pd->dec.state.extended_cmd.op!=0) mode = pd->dec.state.extended_cmd.op;
            gip_ins_a = pd->dec.state.extended_cmd.acc;
            gip_ins_s = pd->dec.state.extended_cmd.sign_or_stack;
            if (pd->dec.state.extended_cmd.cc!=14) gip_ins_cc = (t_gip_ins_cc) pd->dec.state.extended_cmd.cc;
        }
        if (((int)ins_subclass)<8) // basic instructions
        {
            /*b Basic instructions
             */
            switch (ins_subclass)
            {
            case gip_native_ins_subclass_alu_and:
                gip_ins_class = gip_ins_class_logic;
                gip_ins_subclass = gip_ins_subclass_logic_and;
                break;
            case gip_native_ins_subclass_alu_or:
                gip_ins_class = gip_ins_class_logic;
                gip_ins_subclass = gip_ins_subclass_logic_or;
                break;
            case gip_native_ins_subclass_alu_xor:
                gip_ins_class = gip_ins_class_logic;
                gip_ins_subclass = gip_ins_subclass_logic_xor;
                break;
            case gip_native_ins_subclass_alu_mov:
                gip_ins_class = gip_ins_class_logic;
                gip_ins_subclass = gip_ins_subclass_logic_mov;
                break;
            case gip_native_ins_subclass_alu_mvn:
                gip_ins_class = gip_ins_class_logic;
                gip_ins_subclass = gip_ins_subclass_logic_mvn;
                break;
            case gip_native_ins_subclass_alu_add:
                gip_ins_class = gip_ins_class_arith;
                gip_ins_subclass = gip_ins_subclass_arith_add;
                break;
            case gip_native_ins_subclass_alu_sub:
                gip_ins_class = gip_ins_class_arith;
                gip_ins_subclass = gip_ins_subclass_arith_sub;
                break;
            case gip_native_ins_subclass_alu_adc:
                gip_ins_class = gip_ins_class_arith;
                gip_ins_subclass = gip_ins_subclass_arith_adc;
                break;
            default:
                gip_ins_class = gip_ins_class_logic;
                gip_ins_subclass = gip_ins_subclass_logic_and;
                break;
            }
        }
        else
        {
            /*b Modal instructions
             */
            switch (mode)
            {
                /*b Bit mode
                 */
            case gip_native_mode_bit:
                switch (ins_subclass)
                {
                case gip_native_ins_subclass_alu_xorfirst:
                    gip_ins_class = gip_ins_class_logic;
                    gip_ins_subclass = gip_ins_subclass_logic_xorfirst;
                    break;
                case gip_native_ins_subclass_alu_rsb:
                    gip_ins_class = gip_ins_class_arith;
                    gip_ins_subclass = gip_ins_subclass_arith_rsb;
                    break;
                case gip_native_ins_subclass_alu_bic:
                    gip_ins_class = gip_ins_class_logic;
                    gip_ins_subclass = gip_ins_subclass_logic_bic;
                    break;
                case gip_native_ins_subclass_alu_orn:
                    gip_ins_class = gip_ins_class_logic;
                    gip_ins_subclass = gip_ins_subclass_logic_orn;
                    break;
                case gip_native_ins_subclass_alu_andcnt:
                    gip_ins_class = gip_ins_class_logic;
                    gip_ins_subclass = gip_ins_subclass_logic_andcnt;
                    break;
                case gip_native_ins_subclass_alu_xorlast:
                    gip_ins_class = gip_ins_class_logic;
                    gip_ins_subclass = gip_ins_subclass_logic_xorlast;
                    break;
                case gip_native_ins_subclass_alu_bitreverse:
                    gip_ins_class = gip_ins_class_logic;
                    gip_ins_subclass = gip_ins_subclass_logic_bitreverse;
                    break;
                case gip_native_ins_subclass_alu_bytereverse:
                    gip_ins_class = gip_ins_class_logic;
                    gip_ins_subclass = gip_ins_subclass_logic_bytereverse;
                    break;
                default:
                    gip_ins_class = gip_ins_class_logic;
                    gip_ins_subclass = gip_ins_subclass_logic_and;
                    break;
                }
                break;
                /*b Math mode
                 */
            case gip_native_mode_math:
                switch (ins_subclass)
                {
                case gip_native_ins_subclass_alu_xorfirst:
                    gip_ins_class = gip_ins_class_logic;
                    gip_ins_subclass = gip_ins_subclass_logic_xorfirst;
                    break;
                case gip_native_ins_subclass_alu_rsb:
                    gip_ins_class = gip_ins_class_arith;
                    gip_ins_subclass = gip_ins_subclass_arith_rsb;
                    break;
                case gip_native_ins_subclass_alu_init:
                    gip_ins_class = gip_ins_class_arith;
                    gip_ins_subclass = gip_ins_subclass_arith_init;
                    break;
                case gip_native_ins_subclass_alu_mla:
                    gip_ins_class = gip_ins_class_arith;
                    gip_ins_subclass = gip_ins_subclass_arith_mla;
                    break;
                case gip_native_ins_subclass_alu_mlb:
                    gip_ins_class = gip_ins_class_arith;
                    gip_ins_subclass = gip_ins_subclass_arith_mlb;
                    break;
                case gip_native_ins_subclass_alu_sbc:
                    gip_ins_class = gip_ins_class_arith;
                    gip_ins_subclass = gip_ins_subclass_arith_sbc;
                    break;
                case gip_native_ins_subclass_alu_dva:
                    gip_ins_class = gip_ins_class_arith;
                    gip_ins_subclass = gip_ins_subclass_arith_dva;
                    break;
                case gip_native_ins_subclass_alu_dvb:
                    gip_ins_class = gip_ins_class_arith;
                    gip_ins_subclass = gip_ins_subclass_arith_dvb;
                    break;
                default:
                    gip_ins_class = gip_ins_class_logic;
                    gip_ins_subclass = gip_ins_subclass_logic_and;
                    break;
                }
                break;
                /*b GP mode
                 */
            case gip_native_mode_gp:
                switch (ins_subclass)
                {
                case gip_native_ins_subclass_alu_xorfirst:
                    gip_ins_class = gip_ins_class_logic;
                    gip_ins_subclass = gip_ins_subclass_logic_xorfirst;
                    break;
                case gip_native_ins_subclass_alu_rsb:
                    gip_ins_class = gip_ins_class_arith;
                    gip_ins_subclass = gip_ins_subclass_arith_rsb;
                    break;
                case gip_native_ins_subclass_alu_bic:
                    gip_ins_class = gip_ins_class_logic;
                    gip_ins_subclass = gip_ins_subclass_logic_bic;
                    break;
                case gip_native_ins_subclass_alu_andxor:
                    gip_ins_class = gip_ins_class_arith;
                    gip_ins_subclass = gip_ins_subclass_logic_andxor;
                    break;
                case gip_native_ins_subclass_alu_rsc:
                    gip_ins_class = gip_ins_class_arith;
                    gip_ins_subclass = gip_ins_subclass_arith_rsc;
                    break;
                default:
                    gip_ins_class = gip_ins_class_logic;
                    gip_ins_subclass = gip_ins_subclass_logic_and;
                    break;
                }
                break;
            }
        }
        gip_ins_rd = map_native_rd( rd );
        gip_ins_rm = map_native_rm( rm, 0);
        gip_ins_rn = map_native_rn( rd );
        imm_val = map_native_immediate( imm );

        build_gip_instruction_alu( &pd->dec.native.inst, gip_ins_class, gip_ins_subclass, gip_ins_a, gip_ins_s, 0, (gip_ins_rd.type==gip_ins_r_type_internal)&&(gip_ins_rd.data.rd_internal==gip_ins_rd_int_pc) ); // a s p f
        build_gip_instruction_cc( &pd->dec.native.inst, gip_ins_cc );
        build_gip_instruction_rn( &pd->dec.native.inst, gip_ins_rn );
        build_gip_instruction_rd( &pd->dec.native.inst, gip_ins_rd );
        if (ins_class==gip_native_ins_class_alu_imm)
        {
            build_gip_instruction_immediate( &pd->dec.native.inst, imm_val );
        }
        else
        {
            build_gip_instruction_rm( &pd->dec.native.inst, gip_ins_rm );
        }
        pd->dec.native.next_pc = pd->dec.state.pc+2;
        pd->dec.native.next_cycle_of_opcode = 0;
        return 1;
    }
    return 0;
}

/*f c_gip_full::decode_native_cond
 */
int c_gip_full::decode_native_cond( unsigned int opcode )
{
    t_gip_native_ins_class ins_class;
    t_gip_native_ins_subclass ins_subclass;
    int rd;
    int rm;
    int imm;
    int gip_ins_a, gip_ins_s, gip_ins_p;
    t_gip_ins_cc gip_ins_cc;
    t_gip_ins_class gip_ins_class;
    t_gip_ins_subclass gip_ins_subclass;
    t_gip_ins_r gip_ins_rd;
    t_gip_ins_r gip_ins_rn;
    t_gip_ins_r gip_ins_rm;
    unsigned int imm_val;

    ins_class = (t_gip_native_ins_class) ((opcode>>12)&0xf);
    ins_subclass = (t_gip_native_ins_subclass) ((opcode>>8)&0xf);
    rd = (opcode>>4)&0xf;
    rm = (opcode>>0)&0xf;
    imm = (opcode>>0)&0xf;

    if ( (ins_class==gip_native_ins_class_cond_reg) ||
         (ins_class==gip_native_ins_class_cond_imm) )
    {
        /*b Decode mode and operation, rd
         */
        gip_ins_a = 1;
        gip_ins_s = 0;
        gip_ins_p = 0;
        gip_ins_cc = gip_ins_cc_always; // Unless in AND mode and in direct shadow of a conditional, when it should be CP
        if (pd->dec.state.in_immediate_shadow)
        {
            gip_ins_cc = gip_ins_cc_cp;
            printf("c_gip_full:decode_native_cond:Hope we are in AND mode\n");
        }
        if (pd->dec.state.extended_cmd.extended)
        {
            gip_ins_a = pd->dec.state.extended_cmd.acc;
            gip_ins_s = pd->dec.state.extended_cmd.sign_or_stack;
            if (pd->dec.state.extended_cmd.cc!=14) gip_ins_cc = (t_gip_ins_cc) pd->dec.state.extended_cmd.cc;
            ins_subclass = (t_gip_native_ins_subclass) ( ((int)(ins_subclass)) | ((pd->dec.state.extended_cmd.op&1)<<4) ); // use op bit 0 as top bit of conditional
        }
        /*b Get arithmetic operation
         */
        gip_ins_rd.type = gip_ins_r_type_internal;
        switch (ins_subclass)
        {
        case gip_native_ins_subclass_cond_eq:
        case gip_native_ins_subclass_cond_ne:
        case gip_native_ins_subclass_cond_gt:
        case gip_native_ins_subclass_cond_ge:
        case gip_native_ins_subclass_cond_lt:
        case gip_native_ins_subclass_cond_le:
        case gip_native_ins_subclass_cond_hi:
        case gip_native_ins_subclass_cond_hs:
        case gip_native_ins_subclass_cond_lo:
        case gip_native_ins_subclass_cond_ls:
        case gip_native_ins_subclass_cond_seq:
        case gip_native_ins_subclass_cond_sne:
        case gip_native_ins_subclass_cond_sgt:
        case gip_native_ins_subclass_cond_sge:
        case gip_native_ins_subclass_cond_slt:
        case gip_native_ins_subclass_cond_sle:
        case gip_native_ins_subclass_cond_shi:
        case gip_native_ins_subclass_cond_shs:
        case gip_native_ins_subclass_cond_slo:
        case gip_native_ins_subclass_cond_sls:
        case gip_native_ins_subclass_cond_smi:
        case gip_native_ins_subclass_cond_spl:
        case gip_native_ins_subclass_cond_svs:
        case gip_native_ins_subclass_cond_svc:
            gip_ins_class = gip_ins_class_arith;
            gip_ins_subclass = gip_ins_subclass_arith_sub;
            break;
        case gip_native_ins_subclass_cond_sps:
        case gip_native_ins_subclass_cond_spc:
            printf("Will not work in GIP pipeline without using logic Z for eq\n");
            gip_ins_class = gip_ins_class_logic;
            gip_ins_subclass = gip_ins_subclass_logic_mov;
            gip_ins_p = 1;
            break;
        case gip_native_ins_subclass_cond_allset:
        case gip_native_ins_subclass_cond_anyclr:
            printf("Will not work in GIP pipeline without using logic Z for eq\n");
            gip_ins_class = gip_ins_class_logic;
            gip_ins_subclass = gip_ins_subclass_logic_andxor;
            break;
        case gip_native_ins_subclass_cond_allclr:
        case gip_native_ins_subclass_cond_anyset:
            printf("Will not work in GIP pipeline without using logic Z for eq\n");
            gip_ins_class = gip_ins_class_logic;
            gip_ins_subclass = gip_ins_subclass_logic_and;
            break;
        }
        /*b Get target condition
         */
        gip_ins_rd.type = gip_ins_r_type_internal;
        switch (ins_subclass)
        {
        case gip_native_ins_subclass_cond_eq:
        case gip_native_ins_subclass_cond_allset:
        case gip_native_ins_subclass_cond_allclr:
        case gip_native_ins_subclass_cond_seq:
        case gip_native_ins_subclass_cond_sne:
        case gip_native_ins_subclass_cond_sgt:
        case gip_native_ins_subclass_cond_sge:
        case gip_native_ins_subclass_cond_slt:
        case gip_native_ins_subclass_cond_sle:
        case gip_native_ins_subclass_cond_shi:
        case gip_native_ins_subclass_cond_shs:
        case gip_native_ins_subclass_cond_slo:
        case gip_native_ins_subclass_cond_sls:
        case gip_native_ins_subclass_cond_smi:
        case gip_native_ins_subclass_cond_spl:
        case gip_native_ins_subclass_cond_svs:
        case gip_native_ins_subclass_cond_svc:
            gip_ins_rd.data.rd_internal = gip_ins_rd_int_eq;
            break;
        case gip_native_ins_subclass_cond_anyclr:
        case gip_native_ins_subclass_cond_anyset:
        case gip_native_ins_subclass_cond_ne:
            gip_ins_rd.data.rd_internal = gip_ins_rd_int_ne;
            break;
        case gip_native_ins_subclass_cond_gt:
            gip_ins_rd.data.rd_internal = gip_ins_rd_int_gt;
            break;
        case gip_native_ins_subclass_cond_ge:
            gip_ins_rd.data.rd_internal = gip_ins_rd_int_ge;
            break;
        case gip_native_ins_subclass_cond_lt:
            gip_ins_rd.data.rd_internal = gip_ins_rd_int_lt;
            break;
        case gip_native_ins_subclass_cond_le:
            gip_ins_rd.data.rd_internal = gip_ins_rd_int_le;
            break;
        case gip_native_ins_subclass_cond_hi:
            gip_ins_rd.data.rd_internal = gip_ins_rd_int_hi;
            break;
        case gip_native_ins_subclass_cond_hs:
        case gip_native_ins_subclass_cond_sps:
            gip_ins_rd.data.rd_internal = gip_ins_rd_int_cs;
            break;
        case gip_native_ins_subclass_cond_spc:
        case gip_native_ins_subclass_cond_lo:
            gip_ins_rd.data.rd_internal = gip_ins_rd_int_cc;
            break;
        case gip_native_ins_subclass_cond_ls:
            gip_ins_rd.data.rd_internal = gip_ins_rd_int_ls;
            break;
        }
        /*b Get conditional if override required
         */
        gip_ins_rd.type = gip_ins_r_type_internal;
        switch (ins_subclass)
        {
        case gip_native_ins_subclass_cond_eq:
        case gip_native_ins_subclass_cond_ne:
        case gip_native_ins_subclass_cond_gt:
        case gip_native_ins_subclass_cond_ge:
        case gip_native_ins_subclass_cond_lt:
        case gip_native_ins_subclass_cond_le:
        case gip_native_ins_subclass_cond_hi:
        case gip_native_ins_subclass_cond_hs:
        case gip_native_ins_subclass_cond_sps:
        case gip_native_ins_subclass_cond_spc:
        case gip_native_ins_subclass_cond_lo:
        case gip_native_ins_subclass_cond_ls:
        case gip_native_ins_subclass_cond_anyclr:
        case gip_native_ins_subclass_cond_anyset:
        case gip_native_ins_subclass_cond_allset:
        case gip_native_ins_subclass_cond_allclr:
            break;
        case gip_native_ins_subclass_cond_seq:
            gip_ins_cc = gip_ins_cc_eq;
            break;
        case gip_native_ins_subclass_cond_sne:
            gip_ins_cc = gip_ins_cc_ne;
            break;
        case gip_native_ins_subclass_cond_sgt:
            gip_ins_cc = gip_ins_cc_gt;
            break;
        case gip_native_ins_subclass_cond_sge:
            gip_ins_cc = gip_ins_cc_ge;
            break;
        case gip_native_ins_subclass_cond_slt:
            gip_ins_cc = gip_ins_cc_lt;
            break;
        case gip_native_ins_subclass_cond_sle:
            gip_ins_cc = gip_ins_cc_le;
            break;
        case gip_native_ins_subclass_cond_shi:
            gip_ins_cc = gip_ins_cc_hi;
            break;
        case gip_native_ins_subclass_cond_shs:
            gip_ins_cc = gip_ins_cc_cs;
            break;
        case gip_native_ins_subclass_cond_slo:
            gip_ins_cc = gip_ins_cc_cc;
            break;
        case gip_native_ins_subclass_cond_sls:
            gip_ins_cc = gip_ins_cc_ls;
            break;
        case gip_native_ins_subclass_cond_smi:
            gip_ins_cc = gip_ins_cc_mi;
            break;
        case gip_native_ins_subclass_cond_spl:
            gip_ins_cc = gip_ins_cc_pl;
            break;
        case gip_native_ins_subclass_cond_svs:
            gip_ins_cc = gip_ins_cc_vs;
            break;
        case gip_native_ins_subclass_cond_svc:
            gip_ins_cc = gip_ins_cc_vc;
            break;
        }
        gip_ins_rm = map_native_rm( rm, 0 );
        gip_ins_rn = map_native_rn( rd );
        imm_val = map_native_immediate( imm );

        build_gip_instruction_alu( &pd->dec.native.inst, gip_ins_class, gip_ins_subclass, gip_ins_a, gip_ins_s, gip_ins_p, 0 ); // a s p f
        build_gip_instruction_cc( &pd->dec.native.inst, gip_ins_cc );
        build_gip_instruction_rn( &pd->dec.native.inst, gip_ins_rn );
        build_gip_instruction_rd( &pd->dec.native.inst, gip_ins_rd );
        if (ins_class==gip_native_ins_class_cond_imm)
        {
            build_gip_instruction_immediate( &pd->dec.native.inst, imm_val );
        }
        else
        {
            build_gip_instruction_rm( &pd->dec.native.inst, gip_ins_rm );
        }
        pd->dec.native.next_pc = pd->dec.state.pc+2;
        pd->dec.native.next_cycle_of_opcode = 0;
        pd->dec.native.next_in_immediate_shadow = 1;
        return 1;
    }
    return 0;
}

/*f c_gip_full::decode_native_shift
 */
int c_gip_full::decode_native_shift( unsigned int opcode )
{
    t_gip_native_ins_class ins_class;
    t_gip_native_ins_subclass ins_subclass;
    int rd;
    int rm;
    int imm;
    int is_imm;
    int gip_ins_s;
    t_gip_ins_cc gip_ins_cc;
    t_gip_ins_subclass gip_ins_subclass;
    t_gip_ins_r gip_ins_rd;
    t_gip_ins_r gip_ins_rn;
    t_gip_ins_r gip_ins_rm;
    unsigned int imm_val;

    ins_class = (t_gip_native_ins_class) ((opcode>>12)&0xf);
    ins_subclass = (t_gip_native_ins_subclass) ((opcode>>10)&0x3);
    is_imm = (opcode>>9)&1;
    rd = (opcode>>4)&0xf;
    rm = (opcode>>0)&0xf;
    imm = ((opcode>>0)&0xf) | ((opcode&0x100)>>4);

    if (ins_class==gip_native_ins_class_shift)
    {
        /*b Decode shift operation
         */
        gip_ins_s = 1;
        gip_ins_cc = pd->dec.gip_ins_cc;
        if (pd->dec.state.extended_cmd.extended)
        {
            gip_ins_s = pd->dec.state.extended_cmd.sign_or_stack;
            if (pd->dec.state.extended_cmd.cc!=14) gip_ins_cc = (t_gip_ins_cc) pd->dec.state.extended_cmd.cc;
        }
        switch (ins_subclass)
        {
        case gip_native_ins_subclass_shift_lsl:
            gip_ins_subclass = gip_ins_subclass_shift_lsl;
            break;
        case gip_native_ins_subclass_shift_lsr:
            gip_ins_subclass = gip_ins_subclass_shift_lsr;
            break;
        case gip_native_ins_subclass_shift_asr:
            gip_ins_subclass = gip_ins_subclass_shift_asr;
            break;
        case gip_native_ins_subclass_shift_ror:
        default:
            gip_ins_subclass = gip_ins_subclass_shift_ror;
            if (is_imm && imm==0)
            {
                gip_ins_subclass = gip_ins_subclass_shift_ror33;
            }
            break;
        }
        gip_ins_rd = map_native_rd( rd );
        gip_ins_rm = map_native_rm( rm, 0);
        gip_ins_rn = map_native_rn( rd );
        imm_val = map_native_immediate( imm );

        build_gip_instruction_shift( &pd->dec.native.inst, gip_ins_subclass, gip_ins_s, (gip_ins_rd.type==gip_ins_r_type_internal)&&(gip_ins_rd.data.rd_internal==gip_ins_rd_int_pc) ); // s f
        build_gip_instruction_cc( &pd->dec.native.inst, gip_ins_cc );
        build_gip_instruction_rn( &pd->dec.native.inst, gip_ins_rn );
        build_gip_instruction_rd( &pd->dec.native.inst, gip_ins_rd );
        if (is_imm)
        {
            build_gip_instruction_immediate( &pd->dec.native.inst, imm_val );
        }
        else
        {
            build_gip_instruction_rm( &pd->dec.native.inst, gip_ins_rm );
        }
        pd->dec.native.next_pc = pd->dec.state.pc+2;
        pd->dec.native.next_cycle_of_opcode = 0;
        return 1;
    }
    return 0;
}

/*f c_gip_full::decode_native_ldr
 */
int c_gip_full::decode_native_ldr( unsigned int opcode )
{
    t_gip_native_ins_class ins_class;
    int store_not_load;
    t_gip_native_ins_subclass ins_subclass;
    int rd, rn;
    int imm;
    int gip_ins_a, gip_ins_s;
    t_gip_ins_cc gip_ins_cc;
    t_gip_ins_subclass gip_ins_subclass;
    t_gip_ins_r gip_ins_rd;
    t_gip_ins_r gip_ins_rn;
    t_gip_ins_r gip_ins_rm;
    int gip_ins_burst;
    int gip_ins_up;
    int gip_ins_preindex;
    int use_imm;
    unsigned int imm_val;

    ins_class = (t_gip_native_ins_class) ((opcode>>12)&0xf);
    store_not_load = ((opcode>>8)&1);
    ins_subclass = (t_gip_native_ins_subclass) ((opcode>>9)&0x7);
    rn = (opcode>>4)&0xf;
    rd = (opcode>>0)&0xf;

    if ( (ins_class==gip_native_ins_class_memory) && (!store_not_load) )
    {
        /*b Decode mode and operation, rd
         */
        gip_ins_a = 1;
        gip_ins_s = 0;
        gip_ins_burst = 0;
        gip_ins_cc = pd->dec.gip_ins_cc;
        gip_ins_rm = map_native_rm( rd, 1 ); // first argument is irrelevant
        use_imm = (pd->dec.state.extended_rm.type==gip_ins_r_type_no_override);
        switch (ins_subclass)
        {
        case gip_native_ins_subclass_memory_word_noindex: // Preindex Up Word, immediate of zero (unless extended - then preindex up by immediate value)
            gip_ins_preindex = 1;
            gip_ins_up = 1;
            gip_ins_subclass = gip_ins_subclass_memory_word;
            imm = 0;
            break;
        case gip_native_ins_subclass_memory_half_noindex: // Preindex Up Half, immediate of zero (unless extended - then preindex up by immediate value)
            gip_ins_preindex = 1;
            gip_ins_up = 1;
            gip_ins_subclass = gip_ins_subclass_memory_half;
            imm = 0;
            break;
        case gip_native_ins_subclass_memory_byte_noindex: // Preindex Up Byte, immediate of zero (unless extended - then preindex up by immediate value)
            gip_ins_preindex = 1;
            gip_ins_up = 1;
            gip_ins_subclass = gip_ins_subclass_memory_byte;
            imm = 0;
            break;
        case gip_native_ins_subclass_memory_word_preindex_up: // Preindex Up Word, immediate of four (unless extended - then preindex up by immediate value)
            gip_ins_preindex = 1;
            gip_ins_up = 1;
            gip_ins_subclass = gip_ins_subclass_memory_word;
            imm = 4;
            use_imm = 1;
            break;
        case gip_native_ins_subclass_memory_word_preindex_up_shf: // Preindex Up Word by SHF
            gip_ins_preindex = 1;
            gip_ins_up = 1;
            gip_ins_subclass = gip_ins_subclass_memory_word;
            gip_ins_rm.type = gip_ins_r_type_internal;
            gip_ins_rm.data.rnm_internal = gip_ins_rnm_int_shf;
            break;
        case gip_native_ins_subclass_memory_word_preindex_down_shf: // Preindex Up Word by SHF
            gip_ins_preindex = 1;
            gip_ins_up = 0;
            gip_ins_subclass = gip_ins_subclass_memory_word;
            gip_ins_rm.type = gip_ins_r_type_internal;
            gip_ins_rm.data.rnm_internal = gip_ins_rnm_int_shf;
            break;
        case gip_native_ins_subclass_memory_word_postindex_up: // Postindex Up Word, immediate of four (unless extended - then preindex up by immediate value)
            gip_ins_preindex = 0;
            gip_ins_up = 1;
            gip_ins_subclass = gip_ins_subclass_memory_word;
            imm = 4;
            use_imm = 1;
            break;
        case gip_native_ins_subclass_memory_word_postindex_down: // Postindex down Word, immediate of four (unless extended - then preindex up by immediate value)
            gip_ins_preindex = 0;
            gip_ins_up = 1;
            gip_ins_subclass = gip_ins_subclass_memory_word;
            imm = 4;
            use_imm = 1;
            break;
        default:
            break;
        }
        if (pd->dec.state.extended_cmd.extended)
        {
            gip_ins_a = pd->dec.state.extended_cmd.acc;
            gip_ins_s = pd->dec.state.extended_cmd.sign_or_stack;
            if (pd->dec.state.extended_cmd.cc!=14) gip_ins_cc = (t_gip_ins_cc) pd->dec.state.extended_cmd.cc;
            if (pd->dec.state.extended_cmd.op&1)
            {
                gip_ins_burst = pd->special.state.repeat_count;
            }
            else
            {
                gip_ins_burst = pd->dec.state.extended_cmd.burst;
            }
            gip_ins_preindex = !pd->dec.state.extended_cmd.op&1;
        }
        gip_ins_rd = map_native_rd( rd );
        gip_ins_rn = map_native_rn( rn );
        imm_val = map_native_immediate( imm );

        build_gip_instruction_load( &pd->dec.native.inst, gip_ins_subclass, gip_ins_preindex, gip_ins_up, gip_ins_s, gip_ins_burst, gip_ins_a, (gip_ins_rd.type==gip_ins_r_type_internal)&&(gip_ins_rd.data.rd_internal==gip_ins_rd_int_pc) ); // preindex up stack burst_left a f
        build_gip_instruction_cc( &pd->dec.native.inst, gip_ins_cc );
        build_gip_instruction_rn( &pd->dec.native.inst, gip_ins_rn );
        build_gip_instruction_rd( &pd->dec.native.inst, gip_ins_rd );
        if (use_imm)
        {
            build_gip_instruction_immediate( &pd->dec.native.inst, imm_val );
        }
        else
        {
            build_gip_instruction_rm( &pd->dec.native.inst, gip_ins_rm );
        }
        pd->dec.native.next_extended_cmd.burst = (gip_ins_burst==0)?0:gip_ins_burst-1;
        pd->dec.native.next_pc = pd->dec.state.pc+2;
        pd->dec.native.next_cycle_of_opcode = 0;
        return 1;
    }
    return 0;
}

/*f c_gip_full::decode_native_str
 */
int c_gip_full::decode_native_str( unsigned int opcode )
{
    t_gip_native_ins_class ins_class;
    int store_not_load;
    t_gip_native_ins_subclass ins_subclass;
    int rm, rn;
    int gip_ins_a, gip_ins_s;
    t_gip_ins_cc gip_ins_cc;
    t_gip_ins_subclass gip_ins_subclass;
    t_gip_ins_r gip_ins_rd;
    t_gip_ins_r gip_ins_rn;
    t_gip_ins_r gip_ins_rm;
    int gip_ins_burst;
    int gip_ins_up;
    int gip_ins_preindex;
    int gip_ins_use_shift;

    ins_class = (t_gip_native_ins_class) ((opcode>>12)&0xf);
    store_not_load = ((opcode>>8)&1);
    ins_subclass = (t_gip_native_ins_subclass) ((opcode>>9)&0x7);
    rn = (opcode>>4)&0xf;
    rm = (opcode>>0)&0xf;

    if ( (ins_class==gip_native_ins_class_memory) && (store_not_load) )
    {
        /*b Decode mode and operation, rd
         */
        gip_ins_a = 1;
        gip_ins_s = 0;
        gip_ins_burst = 0;
        gip_ins_cc = pd->dec.gip_ins_cc;
        gip_ins_rm = map_native_rm( rm, 0 );
        gip_ins_rd = map_native_rd( rn );
        gip_ins_rn = map_native_rn( rn );
        gip_ins_use_shift = 0;
        switch (ins_subclass)
        {
        case gip_native_ins_subclass_memory_word_noindex: // Postindex Up Word, no setting accumulator ; should not extend with rd set to something!
            gip_ins_a = 0;
            gip_ins_preindex = 0;
            gip_ins_up = 1;
            gip_ins_subclass = gip_ins_subclass_memory_word;
            break;
        case gip_native_ins_subclass_memory_half_noindex: // Postindex Up Half, no setting accumulator ; should not extend with rd set to something!
            gip_ins_a = 0;
            gip_ins_preindex = 0;
            gip_ins_up = 1;
            gip_ins_subclass = gip_ins_subclass_memory_half;
            break;
        case gip_native_ins_subclass_memory_byte_noindex: // Postindex Up Byte, no setting accumulator ; should not extend with rd set to something!
            gip_ins_a = 0;
            gip_ins_preindex = 0;
            gip_ins_up = 1;
            gip_ins_subclass = gip_ins_subclass_memory_byte;
            break;
        case gip_native_ins_subclass_memory_word_preindex_up: // Preindex Up Word with writeback
            gip_ins_preindex = 1;
            gip_ins_up = 1;
            gip_ins_subclass = gip_ins_subclass_memory_word;
            if (pd->dec.state.extended_rd.type==gip_ins_r_type_no_override)
            {
                gip_ins_rd = gip_ins_rn;
            }
            break;
        case gip_native_ins_subclass_memory_word_preindex_up_shf: // Preindex Up Word by SHF
            gip_ins_preindex = 1;
            gip_ins_up = 1;
            gip_ins_subclass = gip_ins_subclass_memory_word;
            gip_ins_use_shift = 1;
            break;
        case gip_native_ins_subclass_memory_word_preindex_down_shf: // Preindex Up Word by SHF
            gip_ins_preindex = 1;
            gip_ins_up = 0;
            gip_ins_subclass = gip_ins_subclass_memory_word;
            gip_ins_use_shift = 1;
            break;
        case gip_native_ins_subclass_memory_word_postindex_up: // Postindex Up Word
            gip_ins_preindex = 0;
            gip_ins_up = 1;
            gip_ins_subclass = gip_ins_subclass_memory_word;
            if (pd->dec.state.extended_rd.type==gip_ins_r_type_no_override)
            {
                gip_ins_rd = gip_ins_rn;
            }
            break;
        case gip_native_ins_subclass_memory_word_postindex_down: // Postindex down Word
            gip_ins_preindex = 0;
            gip_ins_up = 1;
            gip_ins_subclass = gip_ins_subclass_memory_word;
            if (pd->dec.state.extended_rd.type==gip_ins_r_type_no_override)
            {
                gip_ins_rd = gip_ins_rn;
            }
            break;
        default:
            break;
        }
        if (pd->dec.state.extended_cmd.extended)
        {
            gip_ins_a = pd->dec.state.extended_cmd.acc;
            gip_ins_s = pd->dec.state.extended_cmd.sign_or_stack;
            if (pd->dec.state.extended_cmd.cc!=14) gip_ins_cc = (t_gip_ins_cc) pd->dec.state.extended_cmd.cc;
            if (pd->dec.state.extended_cmd.op&1)
            {
                gip_ins_burst = pd->special.state.repeat_count;
            }
            else
            {
                gip_ins_burst = pd->dec.state.extended_cmd.burst;
            }
            gip_ins_preindex = !pd->dec.state.extended_cmd.op&1;
        }
        build_gip_instruction_store( &pd->dec.native.inst, gip_ins_subclass, gip_ins_preindex, gip_ins_up, gip_ins_use_shift, gip_ins_s, gip_ins_burst, gip_ins_a, (gip_ins_rd.type==gip_ins_r_type_internal)&&(gip_ins_rd.data.rd_internal==gip_ins_rd_int_pc) ); // preindex up stack burst_left a f
        build_gip_instruction_cc( &pd->dec.native.inst, gip_ins_cc );
        build_gip_instruction_rn( &pd->dec.native.inst, gip_ins_rn );
        build_gip_instruction_rd( &pd->dec.native.inst, gip_ins_rd );
        build_gip_instruction_rm( &pd->dec.native.inst, gip_ins_rm );
        pd->dec.native.next_extended_cmd.burst = (gip_ins_burst==0)?0:gip_ins_burst-1;
        pd->dec.native.next_pc = pd->dec.state.pc+2;
        pd->dec.native.next_cycle_of_opcode = 0;
        return 1;
    }
    return 0;
}

/*f c_gip_full::decode_native_branch
 */
int c_gip_full::decode_native_branch( unsigned int opcode )
{
    t_gip_native_ins_class ins_class;
    int delay_slot;
    int offset;
    t_gip_ins_cc gip_ins_cc;

    ins_class = (t_gip_native_ins_class) ((opcode>>12)&0xf);
    delay_slot = ((opcode>>0)&1);
    offset = (opcode>>0)&0xffe;
    offset = (offset<<20)>>20;
    if (ins_class==gip_native_ins_class_branch)
    {
        if (!pd->dec.state.in_conditional_shadow)
        {
            printf("********************************************************************************\nShould insert instruction to reset CPs to true - taken branch resets these!\n");
            offset = map_native_immediate( offset );
            if (delay_slot)
            {
                pd->dec.native.next_delay_pc = pd->dec.state.pc+8+offset;
                pd->dec.native.next_follow_delay_pc = 1;
                pd->dec.native.next_in_delay_slot = 1;
                pd->dec.native.next_pc = pd->dec.state.pc+2;
                pd->dec.native.next_cycle_of_opcode = 0;
            }
            else
            {
                pd->dec.native.next_pc = pd->dec.state.pc+8+offset;
                pd->dec.native.next_cycle_of_opcode = 0;
            }
            return 1;
        }
        /*b Build a conditional flushing instruction (ADD<cc>F pc, #offset), mark next instruction as delayed if required
         */
        gip_ins_cc = pd->dec.gip_ins_cc;
        if (pd->dec.state.extended_cmd.extended)
        {
            if (pd->dec.state.extended_cmd.cc!=14) gip_ins_cc = (t_gip_ins_cc) pd->dec.state.extended_cmd.cc;
        }
        build_gip_instruction_alu( &pd->dec.native.inst, gip_ins_class_arith, gip_ins_subclass_arith_add, 0, 0, 0, 1 ); // a s p f
        build_gip_instruction_cc( &pd->dec.native.inst,  gip_ins_cc ); // !cc is CC with bottom bit inverted, in ARM
        build_gip_instruction_rn_int( &pd->dec.native.inst, gip_ins_rnm_int_pc );
        build_gip_instruction_immediate( &pd->dec.native.inst, offset );
        build_gip_instruction_rd_int( &pd->dec.native.inst, gip_ins_rd_int_pc );
        pd->dec.native.next_in_delay_slot = 1;
        pd->dec.native.next_pc = pd->dec.state.pc+2;
        pd->dec.native.next_cycle_of_opcode = 0;
        return 1;
    }
    return 0;
}

/*a ARM Decode functions
 */
/*f c_gip_full::decode_arm_debug
 */
int c_gip_full::decode_arm_debug( void )
{
    /*b Check if instruction is 0xf00000..
     */
    if ((pd->dec.state.opcode&0xffffff00)==0xf0000000)
    {
        pd->dec.arm.next_cycle_of_opcode = 0;
        pd->dec.arm.next_pc = pd->dec.state.pc+4;
        return 1;
    }
    return 0;
}

/*f c_gip_full::decode_arm_alu
  ALU instructions map to one or more internal instructions
    Those that do not set the PC and are immediate or have a shift of LSL #0 map to one internal ALU instruction
    Those that do not set the PC and have a shift other than LSL #0 map to one internal SHIFT instruction and one internal ALU instruction
    Those that set the PC and have a shift of LSL #0 map to one internal ALU instruction with 'flush' set
    Those that set the PC and have a shift other than LSL #0 map to one internal SHIFT instruction and one internal ALU instruction with 'flush' set
 */
int c_gip_full::decode_arm_alu( void )
{
    int cc, zeros, imm, alu_op, sign, rn, rd, op2;
    int imm_val, imm_rotate;
    int shf_rs, shf_reg_zero, shf_imm_amt, shf_how, shf_by_reg, shf_rm;
    int conditional;
    t_gip_ins_class gip_ins_class;
    t_gip_ins_subclass gip_ins_subclass;
    t_gip_ins_cc gip_ins_cc;
    t_gip_ins_r gip_ins_rn;
    t_gip_ins_r gip_ins_rm;
    t_gip_ins_r gip_ins_rs;
    t_gip_ins_r gip_ins_rd;
    int gip_set_flags, gip_set_acc, gip_pass_p;

    /*b Decode ARM instruction
     */
    cc      = (pd->dec.state.opcode>>28) & 0x0f;
    zeros   = (pd->dec.state.opcode>>26) & 0x03;
    imm     = (pd->dec.state.opcode>>25) & 0x01;
    alu_op  = (pd->dec.state.opcode>>21) & 0x0f;
    sign    = (pd->dec.state.opcode>>20) & 0x01;
    rn      = (pd->dec.state.opcode>>16) & 0x0f;
    rd      = (pd->dec.state.opcode>>12) & 0x0f;
    op2     = (pd->dec.state.opcode>> 0) & 0xfff;

    /*b If not an ALU instruction, then exit
     */
    if (zeros)
    {
        return 0;
    }

    /*b Determine the shift
     */
    imm_rotate = (op2>>8) & 0x0f; // RRRRiiiiiiii
    imm_val    = (op2>>0) & 0xff;

    shf_rs       = (op2>> 8) & 0x0f; // ssss0tt1mmmm
    shf_reg_zero = (op2>> 7) & 0x01;
    shf_imm_amt  = (op2>> 7) & 0x1f; // iiiiitt0mmmm
    shf_how      = (op2>> 5) & 0x03;
    shf_by_reg   = (op2>> 4) & 0x01;
    shf_rm       = (op2>> 0) & 0x0f;

    if (!imm && shf_by_reg && shf_reg_zero)
    {
        fprintf( stderr, "Slow:ALU:Abort - imm zero, shf by reg 1, shf_reg_zero not zero\n");
        return 0;
    }

    /*b Map condition code
     */
    conditional = (cc!=14);
    gip_ins_cc = map_condition_code( cc );

    /*b Map operation and setting flags
     */
    gip_ins_class = gip_ins_class_arith;
    gip_ins_subclass = gip_ins_subclass_arith_add;
    gip_set_acc = 0;
    gip_set_flags = 0;
    gip_pass_p = 0;
    gip_ins_rn = map_source_register( rn );
    gip_ins_rm = map_source_register( shf_rm );
    gip_ins_rs = map_source_register( shf_rs );
    gip_ins_rd = map_destination_register( rd );
    switch (alu_op)
    {
    case  0: // and
        gip_ins_class = gip_ins_class_logic;
        gip_ins_subclass = gip_ins_subclass_logic_and;
        gip_set_flags = sign;
        gip_pass_p = sign;
        gip_set_acc = !conditional;
        break;
    case  1: // eor
        gip_ins_class = gip_ins_class_logic;
        gip_ins_subclass = gip_ins_subclass_logic_xor;
        gip_set_flags = sign;
        gip_pass_p = sign;
        gip_set_acc = !conditional;
        break;
    case  2: // sub
        gip_ins_class = gip_ins_class_arith;
        gip_ins_subclass = gip_ins_subclass_arith_sub;
        gip_set_flags = sign;
        gip_set_acc = !conditional;
        break;
    case  3: // rsb
        gip_ins_class = gip_ins_class_arith;
        gip_ins_subclass = gip_ins_subclass_arith_rsb;
        gip_set_flags = sign;
        gip_set_acc = !conditional;
        break;
    case  4: // add
        gip_ins_class = gip_ins_class_arith;
        gip_ins_subclass = gip_ins_subclass_arith_add;
        gip_set_flags = sign;
        gip_set_acc = !conditional;
        break;
    case  5: // adc
        gip_ins_class = gip_ins_class_arith;
        gip_ins_subclass = gip_ins_subclass_arith_adc;
        gip_set_flags = sign;
        gip_set_acc = !conditional;
        break;
    case  6: // sbc
        gip_ins_class = gip_ins_class_arith;
        gip_ins_subclass = gip_ins_subclass_arith_sbc;
        gip_set_flags = sign;
        gip_set_acc = !conditional;
        break;
    case  7: // rsc
        gip_ins_class = gip_ins_class_arith;
        gip_ins_subclass = gip_ins_subclass_arith_rsc;
        gip_set_flags = sign;
        gip_set_acc = !conditional;
        break;
    case  8: // tst
        gip_ins_class = gip_ins_class_logic;
        gip_ins_subclass = gip_ins_subclass_logic_and;
        gip_set_flags = 1;
        gip_pass_p = sign;
        gip_set_acc = 0;
        gip_ins_rd.type = gip_ins_r_type_none;
        break;
    case  9: // teq
        gip_ins_class = gip_ins_class_logic;
        gip_ins_subclass = gip_ins_subclass_logic_xor;
        gip_set_flags = 1;
        gip_pass_p = sign;
        gip_set_acc = 0;
        gip_ins_rd.type = gip_ins_r_type_none;
        break;
    case 10: // cmp
        gip_ins_class = gip_ins_class_arith;
        gip_ins_subclass = gip_ins_subclass_arith_sub;
        gip_set_flags = 1;
        gip_set_acc = 0;
        gip_ins_rd.type = gip_ins_r_type_none;
        break;
    case 11: // cmn
        gip_ins_class = gip_ins_class_arith;
        gip_ins_subclass = gip_ins_subclass_arith_add;
        gip_set_flags = 1;
        gip_set_acc = 0;
        gip_ins_rd.type = gip_ins_r_type_none;
        break;
    case 12: // orr
        gip_ins_class = gip_ins_class_logic;
        gip_ins_subclass = gip_ins_subclass_logic_or;
        gip_set_flags = sign;
        gip_pass_p = sign;
        gip_set_acc = !conditional;
        break;
    case 13: // mov
        gip_ins_class = gip_ins_class_logic;
        gip_ins_subclass = gip_ins_subclass_logic_mov;
        gip_set_flags = sign;
        gip_pass_p = sign;
        gip_set_acc = !conditional;
        break;
    case 14: // bic
        gip_ins_class = gip_ins_class_logic;
        gip_ins_subclass = gip_ins_subclass_logic_bic;
        gip_set_flags = sign;
        gip_pass_p = sign;
        gip_set_acc = !conditional;
        break;
    case 15: // mvn
        gip_ins_class = gip_ins_class_logic;
        gip_ins_subclass = gip_ins_subclass_logic_mvn;
        gip_set_flags = sign;
        gip_pass_p = sign;
        gip_set_acc = !conditional;
        break;
    default:
        break;
    }
    pd->dec.arm.next_acc_valid = pd->dec.state.acc_valid;
    pd->dec.arm.next_reg_in_acc = pd->dec.state.reg_in_acc;
    if (gip_set_acc)
    {
        if (gip_ins_rd.type==gip_ins_r_type_register) 
        {
            pd->dec.arm.next_acc_valid = !conditional;
            pd->dec.arm.next_reg_in_acc = gip_ins_rd.data.r;
        }
        else
        {
            pd->dec.arm.next_acc_valid = 0;
        }
    }
    else if ((gip_ins_rd.type==gip_ins_r_type_register)  && (gip_ins_rd.data.r==pd->dec.state.reg_in_acc))
    {
        pd->dec.arm.next_acc_valid = 0;
    }

    /*b Test for shift of 'LSL #0' or plain immediate
     */
    if (imm)
    {
        gip_pass_p = 0;
        build_gip_instruction_alu( &pd->dec.arm.inst, gip_ins_class, gip_ins_subclass, gip_set_acc, gip_set_flags, gip_pass_p, (rd==15) );
        build_gip_instruction_cc( &pd->dec.arm.inst, gip_ins_cc );
        build_gip_instruction_rn( &pd->dec.arm.inst, gip_ins_rn );
        build_gip_instruction_immediate( &pd->dec.arm.inst, rotate_right(imm_val,imm_rotate*2) );
        build_gip_instruction_rd( &pd->dec.arm.inst, gip_ins_rd );
        pd->dec.arm.next_cycle_of_opcode = 0;
        pd->dec.arm.next_pc = pd->dec.state.pc+4;
    }
    else if ( (((t_shf_type)shf_how)==shf_type_lsl) &&
              (!shf_by_reg) &&
              (shf_imm_amt==0) )
    {
        gip_pass_p = 0;
        build_gip_instruction_alu( &pd->dec.arm.inst, gip_ins_class, gip_ins_subclass, gip_set_acc, gip_set_flags, gip_pass_p, (rd==15) );
        build_gip_instruction_cc( &pd->dec.arm.inst, gip_ins_cc );
        build_gip_instruction_rn( &pd->dec.arm.inst, gip_ins_rn );
        build_gip_instruction_rm( &pd->dec.arm.inst, gip_ins_rm );
        build_gip_instruction_rd( &pd->dec.arm.inst, gip_ins_rd );
        pd->dec.arm.next_cycle_of_opcode = 0;
        pd->dec.arm.next_pc = pd->dec.state.pc+4;
    }
    else if (!shf_by_reg) // Immediate shift, non-zero or non-LSL: ISHF Rm, #imm; IALU{CC}(SP|)[A][F] Rn, SHF -> Rd
    {
        switch (pd->dec.state.cycle_of_opcode)
        {
        case 0:
            build_gip_instruction_shift( &pd->dec.arm.inst, map_shift( shf_how, 1, shf_imm_amt), 0, 0 );
            build_gip_instruction_rn( &pd->dec.arm.inst, gip_ins_rm );
            build_gip_instruction_immediate( &pd->dec.arm.inst, (shf_imm_amt==0)?32:shf_imm_amt );
            pd->dec.arm.next_acc_valid = pd->dec.state.acc_valid; // First internal does not set acc
            pd->dec.arm.next_reg_in_acc = pd->dec.state.reg_in_acc;
            break;
        default:
            build_gip_instruction_alu( &pd->dec.arm.inst, gip_ins_class, gip_ins_subclass, gip_set_acc, gip_set_flags, gip_pass_p, (rd==15) );
            build_gip_instruction_cc( &pd->dec.arm.inst, gip_ins_cc );
            build_gip_instruction_rn( &pd->dec.arm.inst, gip_ins_rn );
            build_gip_instruction_rm_int( &pd->dec.arm.inst, gip_ins_rnm_int_shf );
            build_gip_instruction_rd( &pd->dec.arm.inst, gip_ins_rd );
            pd->dec.arm.next_cycle_of_opcode = 0;
            pd->dec.arm.next_pc = pd->dec.state.pc+4;
            break;
        }
    }
    else // if (shf_by_reg) - must be! ISHF Rm, Rs; IALU{CC}(SP|)[A][F] Rn, SHF -> Rd
    {
        switch (pd->dec.state.cycle_of_opcode)
        {
        case 0:
            build_gip_instruction_shift( &pd->dec.arm.inst, map_shift(shf_how, 0, 0 ), 0, 0 );
            build_gip_instruction_rn( &pd->dec.arm.inst, gip_ins_rm );
            build_gip_instruction_rm( &pd->dec.arm.inst, gip_ins_rs );
            pd->dec.arm.next_acc_valid = pd->dec.state.acc_valid; // First internal does not set acc
            pd->dec.arm.next_reg_in_acc = pd->dec.state.reg_in_acc;
            break;
        default:
            build_gip_instruction_alu( &pd->dec.arm.inst, gip_ins_class, gip_ins_subclass, gip_set_acc, gip_set_flags, gip_pass_p, (rd==15) );
            build_gip_instruction_cc( &pd->dec.arm.inst, gip_ins_cc );
            build_gip_instruction_rn( &pd->dec.arm.inst, gip_ins_rn );
            build_gip_instruction_rm_int( &pd->dec.arm.inst, gip_ins_rnm_int_shf );
            build_gip_instruction_rd( &pd->dec.arm.inst, gip_ins_rd );
            pd->dec.arm.next_cycle_of_opcode = 0;
            pd->dec.arm.next_pc = pd->dec.state.pc+4;
            break;
        }
    }

    return 1;
}

/*f c_gip_full::decode_arm_branch
 */
int c_gip_full::decode_arm_branch( void )
{
    int cc, five, link, offset;

    /*b Decode ARM instruction
     */
    cc      = (pd->dec.state.opcode>>28) & 0x0f;
    five    = (pd->dec.state.opcode>>25) & 0x07;
    link    = (pd->dec.state.opcode>>24) & 0x01;
    offset  = (pd->dec.state.opcode>> 0) & 0x00ffffff;

    /*b Return if not a branch
     */
    if (five != 5)
        return 0;

    /*b Handle 5 cases; conditional or not, link or not; split conditional branches to predicted or not, also
     */
    if (!link)
    {
        if (cc==14) // guaranteed branch
        {
            pd->dec.arm.next_pc = pd->dec.state.pc+8+((offset<<8)>>6);
            pd->dec.arm.next_cycle_of_opcode = 0;
        }
        else
        {
            if (offset&0x800000) // backward conditional branch; sub(!cc)f pc, #4 -> pc
            {
                build_gip_instruction_alu( &pd->dec.arm.inst, gip_ins_class_arith, gip_ins_subclass_arith_sub, 0, 0, 0, 1 );
                build_gip_instruction_cc( &pd->dec.arm.inst, map_condition_code( cc^0x1 ) ); // !cc is CC with bottom bit inverted, in ARM
                build_gip_instruction_rn_int( &pd->dec.arm.inst, gip_ins_rnm_int_pc );
                build_gip_instruction_immediate( &pd->dec.arm.inst, 4 );
                build_gip_instruction_rd_int( &pd->dec.arm.inst, gip_ins_rd_int_pc );
                pd->dec.arm.next_pc = pd->dec.state.pc+8+((offset<<8)>>6);
                pd->dec.arm.next_cycle_of_opcode = 0;
            }
            else // forward conditional branch; mov[cc]f #target -> pc
            {
                build_gip_instruction_alu( &pd->dec.arm.inst, gip_ins_class_logic, gip_ins_subclass_logic_mov, 0, 0, 0, 1 );
                build_gip_instruction_cc( &pd->dec.arm.inst, map_condition_code( cc ) );
                build_gip_instruction_immediate( &pd->dec.arm.inst, pd->dec.state.pc+8+((offset<<8)>>6) );
                build_gip_instruction_rd_int( &pd->dec.arm.inst, gip_ins_rd_int_pc );
                pd->dec.arm.next_pc = pd->dec.state.pc+4;
                pd->dec.arm.next_cycle_of_opcode = 0;
            }
        }
    }
    else
    {
        if (cc==14) // guaranteed branch with link; sub pc, #4 -> r14
        {
            build_gip_instruction_alu( &pd->dec.arm.inst, gip_ins_class_arith, gip_ins_subclass_arith_sub, 0, 0, 0, 0 );
            build_gip_instruction_cc( &pd->dec.arm.inst, gip_ins_cc_always );
            build_gip_instruction_rn_int( &pd->dec.arm.inst, gip_ins_rnm_int_pc );
            build_gip_instruction_immediate( &pd->dec.arm.inst, 4 );
            build_gip_instruction_rd_reg( &pd->dec.arm.inst, 14 );
            pd->dec.arm.next_pc = pd->dec.state.pc+8+((offset<<8)>>6);
            pd->dec.arm.next_cycle_of_opcode = 0;
        }
        else // conditional branch with link; sub{!cc}f pc, #4 -> pc; sub pc, #4 -> r14
        {
            switch (pd->dec.state.cycle_of_opcode)
            {
            case 0:
                build_gip_instruction_alu( &pd->dec.arm.inst, gip_ins_class_arith, gip_ins_subclass_arith_sub, 0, 0, 0, 1 );
                build_gip_instruction_cc( &pd->dec.arm.inst, map_condition_code( cc^0x1 ) ); // !cc is CC with bottom bit inverted, in ARM
                build_gip_instruction_rn_int( &pd->dec.arm.inst, gip_ins_rnm_int_pc );
                build_gip_instruction_immediate( &pd->dec.arm.inst, 4 );
                build_gip_instruction_rd_int( &pd->dec.arm.inst, gip_ins_rd_int_pc );
                break;
            default:
                build_gip_instruction_alu( &pd->dec.arm.inst, gip_ins_class_arith, gip_ins_subclass_arith_sub, 0, 0, 0, 0 );
                build_gip_instruction_cc( &pd->dec.arm.inst, gip_ins_cc_always );
                build_gip_instruction_rn_int( &pd->dec.arm.inst, gip_ins_rnm_int_pc );
                build_gip_instruction_immediate( &pd->dec.arm.inst, 4 );
                build_gip_instruction_rd_reg( &pd->dec.arm.inst, 14 );
                pd->dec.arm.next_pc = pd->dec.state.pc+8+((offset<<8)>>6);
                pd->dec.arm.next_cycle_of_opcode = 0;
            }
        }
    }
    return 1;
}

/*f c_gip_full::decode_arm_ld_st
 */
int c_gip_full::decode_arm_ld_st( void )
{
    int cc, one, not_imm, pre, up, byte, wb, load, rn, rd, offset;
    int imm_val;
    int shf_imm_amt, shf_how, shf_zero, shf_rm;

    /*b Decode ARM instruction
     */
    cc      = (pd->dec.state.opcode>>28) & 0x0f;
    one     = (pd->dec.state.opcode>>26) & 0x03;
    not_imm = (pd->dec.state.opcode>>25) & 0x01;
    pre     = (pd->dec.state.opcode>>24) & 0x01;
    up      = (pd->dec.state.opcode>>23) & 0x01;
    byte    = (pd->dec.state.opcode>>22) & 0x01;
    wb      = (pd->dec.state.opcode>>21) & 0x01;
    load    = (pd->dec.state.opcode>>20) & 0x01;
    rn      = (pd->dec.state.opcode>>16) & 0x0f;
    rd      = (pd->dec.state.opcode>>12) & 0x0f;
    offset  = (pd->dec.state.opcode>> 0) & 0xfff;

    /*b Validate this is a load/store
     */
    if (one != 1)
        return 0;

    /*b Break out offset
     */
    imm_val    = (offset>>0) & 0xfff;

    shf_imm_amt  = (offset>> 7) & 0x1f;
    shf_how      = (offset>> 5) & 0x03;
    shf_zero     = (offset>> 4) & 0x01;
    shf_rm       = (offset>> 0) & 0x0f;

    /*b Validate this is a load/store again
     */
    if (not_imm && shf_zero)
        return 0;

    /*b Handle loads - preindexed immediate/reg, preindexed reg with shift, preindexed immediate/reg with wb, preindexed reg with shift with wb, postindexed immediate/reg, postindexed reg with shift
     */
    if (load)
    {
        /*b Handle immediate or reg without shift
         */
        if (!not_imm ||
            (((t_shf_type)(shf_how)==shf_type_lsl) && (shf_imm_amt==0)) ) // immediate or reg without shift
        {
            /*b Preindexed, no writeback
             */
            if (pre && !wb) // preindexed immediate/reg: ILDR[CC]A[F] #0 (Rn, #+/-imm or +/-Rm) -> Rd
            {
                build_gip_instruction_load( &pd->dec.arm.inst, gip_ins_subclass_memory_word, 1, up, (rn==13), 0, 1, (rd==15) );
                build_gip_instruction_cc( &pd->dec.arm.inst, map_condition_code( cc ) );
                build_gip_instruction_rn( &pd->dec.arm.inst, map_source_register(rn) );
                if (not_imm)
                {
                    build_gip_instruction_rm( &pd->dec.arm.inst, map_source_register(shf_rm) );
                }
                else
                {
                    build_gip_instruction_immediate( &pd->dec.arm.inst, imm_val );
                }
                build_gip_instruction_rd( &pd->dec.arm.inst, map_destination_register(rd) );
                pd->dec.arm.next_pc = pd->dec.state.pc+4;
                pd->dec.arm.next_cycle_of_opcode = 0;
                return 1;
            }
            /*b Preindexed, writeback
             */
            else if (pre) // preindexed immediate/reg with writeback: IADD[CC]A/ISUB[CC]A Rn, #imm/Rm -> Rn; ILDRCP[F] #0, (Acc) -> Rd
            {
                switch (pd->dec.state.cycle_of_opcode)
                {
                case 0:
                    build_gip_instruction_alu( &pd->dec.arm.inst, gip_ins_class_arith, up?gip_ins_subclass_arith_add:gip_ins_subclass_arith_sub, 1, 0, 0, 0 );
                    build_gip_instruction_cc( &pd->dec.arm.inst, map_condition_code( cc ) );
                    build_gip_instruction_rn( &pd->dec.arm.inst, map_source_register(rn) );
                    if (not_imm)
                    {
                        build_gip_instruction_rm( &pd->dec.arm.inst, map_source_register(shf_rm) );
                    }
                    else
                    {
                        build_gip_instruction_immediate( &pd->dec.arm.inst, imm_val );
                    }
                    build_gip_instruction_rd( &pd->dec.arm.inst, map_destination_register(rn) );
                    break;
                default:
                    build_gip_instruction_load( &pd->dec.arm.inst, gip_ins_subclass_memory_word, 0, up, (rn==13), 0, 0, 0 );
                    build_gip_instruction_cc( &pd->dec.arm.inst, gip_ins_cc_cp );
                    build_gip_instruction_rn_int( &pd->dec.arm.inst, gip_ins_rnm_int_acc );
                    build_gip_instruction_rd( &pd->dec.arm.inst, map_destination_register( rd ) );
                    pd->dec.arm.next_pc = pd->dec.state.pc+4;
                    pd->dec.arm.next_cycle_of_opcode = 0;
                    break;
                }
                return 1;
            }
            /*b Postindexed
             */
            else // postindexed immediate/reg: ILDR[CC]A #0, (Rn), +/-Rm/Imm -> Rd; MOVCP[F] Acc -> Rn
            {
                switch (pd->dec.state.cycle_of_opcode)
                {
                case 0:
                    build_gip_instruction_load( &pd->dec.arm.inst,gip_ins_subclass_memory_word, 0, up, (rn==13), 0, 1, 0 );
                    build_gip_instruction_cc( &pd->dec.arm.inst, map_condition_code( cc ) );
                    build_gip_instruction_rn( &pd->dec.arm.inst, map_source_register(rn) );
                    if (not_imm)
                    {
                        build_gip_instruction_rm( &pd->dec.arm.inst, map_source_register(shf_rm) );
                    }
                    else
                    {
                        build_gip_instruction_immediate( &pd->dec.arm.inst, imm_val );
                    }
                    build_gip_instruction_rd( &pd->dec.arm.inst, map_destination_register(rd) );
                    break;
                default:
                    build_gip_instruction_alu( &pd->dec.arm.inst, gip_ins_class_logic, gip_ins_subclass_logic_mov, 0, 0, 0, (rd==15) );
                    build_gip_instruction_cc( &pd->dec.arm.inst, gip_ins_cc_cp );
                    build_gip_instruction_rm_int( &pd->dec.arm.inst, gip_ins_rnm_int_acc );
                    build_gip_instruction_rd( &pd->dec.arm.inst, map_destination_register(rn) );
                    pd->dec.arm.next_pc = pd->dec.state.pc+4;
                    pd->dec.arm.next_cycle_of_opcode = 0;
                    break;
                }
                return 1;
            }
        }
        /*b Register with shift
         */
        else // reg with shift
        {
            /*b Preindexed without writeback
             */
            if (pre && !wb) // preindexed reg with shift: ISHF Rm, #imm; ILDR[CC]A[F] (Rn, +/-SHF) -> Rd
            {
                switch (pd->dec.state.cycle_of_opcode)
                {
                case 0:
                    build_gip_instruction_shift( &pd->dec.arm.inst, map_shift(shf_how, 1, shf_imm_amt ), 0, 0 );
                    build_gip_instruction_rn( &pd->dec.arm.inst, map_source_register(shf_rm) );
                    build_gip_instruction_immediate( &pd->dec.arm.inst, shf_imm_amt );
                    break;
                default:
                    build_gip_instruction_load( &pd->dec.arm.inst, gip_ins_subclass_memory_word, 1, up, (rn==13), 0, 1, (rd==15) );
                    build_gip_instruction_cc( &pd->dec.arm.inst, map_condition_code( cc ) );
                    build_gip_instruction_rn( &pd->dec.arm.inst, map_source_register(rn) );
                    build_gip_instruction_rm_int( &pd->dec.arm.inst, gip_ins_rnm_int_shf );
                    build_gip_instruction_rd( &pd->dec.arm.inst, map_destination_register(rd) );
                    pd->dec.arm.next_pc = pd->dec.state.pc+4;
                    pd->dec.arm.next_cycle_of_opcode = 0;
                    break;
                }
                return 1;
            }
            /*b Preindexed with writeback
             */
            else if (pre) // preindexed reg with shift with writeback: ILSL Rm, #imm; IADD[CC]A/ISUB[CC]A Rn, SHF -> Rn; ILDRCP[F] #0 (Acc) -> Rd
            {
                switch (pd->dec.state.cycle_of_opcode)
                {
                case 0:
                    build_gip_instruction_shift( &pd->dec.arm.inst, map_shift(shf_how, 1, shf_imm_amt ), 0, 0 );
                    build_gip_instruction_rn( &pd->dec.arm.inst, map_source_register(shf_rm) );
                    build_gip_instruction_immediate( &pd->dec.arm.inst, shf_imm_amt );
                    break;
                case 1:
                    build_gip_instruction_alu( &pd->dec.arm.inst, gip_ins_class_arith, up?gip_ins_subclass_arith_add:gip_ins_subclass_arith_sub, 1, 0, 0, 0 );
                    build_gip_instruction_cc( &pd->dec.arm.inst, map_condition_code( cc ) );
                    build_gip_instruction_rn( &pd->dec.arm.inst, map_source_register(rn) );
                    build_gip_instruction_rm_int( &pd->dec.arm.inst, gip_ins_rnm_int_shf );
                    build_gip_instruction_rd( &pd->dec.arm.inst, map_destination_register(rn) );
                    break;
                default:
                    build_gip_instruction_load( &pd->dec.arm.inst, gip_ins_subclass_memory_word, 0, 1, (rn==13), 0, 0, (rd==15) );
                    build_gip_instruction_cc( &pd->dec.arm.inst, gip_ins_cc_cp );
                    build_gip_instruction_rn_int( &pd->dec.arm.inst, gip_ins_rnm_int_acc );
                    build_gip_instruction_rd( &pd->dec.arm.inst, map_destination_register(rd) );
                    pd->dec.arm.next_pc = pd->dec.state.pc+4;
                    pd->dec.arm.next_cycle_of_opcode = 0;
                    break;
                }
                return 1;
            }
            /*b Postindexed
             */
            else // postindexed reg with shift with writeback: ILSL Rm, #imm; ILDR[CC]A #0 (Rn), +/-SHF -> Rd; MOVCP[F] Acc -> Rn
            {
                switch (pd->dec.state.cycle_of_opcode)
                {
                case 0:
                    build_gip_instruction_shift( &pd->dec.arm.inst, map_shift(shf_how, 1, shf_imm_amt ), 0, 0 );
                    build_gip_instruction_rn( &pd->dec.arm.inst, map_source_register(shf_rm) );
                    build_gip_instruction_immediate( &pd->dec.arm.inst, shf_imm_amt );
                    break;
                case 1:
                    build_gip_instruction_load( &pd->dec.arm.inst, gip_ins_subclass_memory_word, 0, up, (rn==13), 0, 1, 0 );
                    build_gip_instruction_cc( &pd->dec.arm.inst,map_condition_code(cc) );
                    build_gip_instruction_rn( &pd->dec.arm.inst, map_source_register(rn) );
                    build_gip_instruction_rm_int( &pd->dec.arm.inst, gip_ins_rnm_int_shf );
                    build_gip_instruction_rd( &pd->dec.arm.inst, map_destination_register(rd) );
                    break;
                default:
                    build_gip_instruction_alu( &pd->dec.arm.inst, gip_ins_class_logic, gip_ins_subclass_logic_mov, 0, 0, 0, (rd==15) );
                    build_gip_instruction_cc( &pd->dec.arm.inst, gip_ins_cc_cp );
                    build_gip_instruction_rm_int( &pd->dec.arm.inst, gip_ins_rnm_int_acc );
                    build_gip_instruction_rd( &pd->dec.arm.inst, map_destination_register(rn) );
                    pd->dec.arm.next_pc = pd->dec.state.pc+4;
                    pd->dec.arm.next_cycle_of_opcode = 0;
                    break;
                }
                return 1;
            }
        }
        return 0;
    }

    /*b Handle stores - no offset, preindexed immediate, preindexed reg, preindexed reg with shift, postindexed immediate, postindexed reg, postindexed reg with shift
     */
    /*b Immediate offset of zero
     */
    if ( !not_imm && (imm_val==0) ) // no offset (hence no writeback, none needed); ISTR[CC] #0 (Rn) <- Rd
    {
        build_gip_instruction_store( &pd->dec.arm.inst, gip_ins_subclass_memory_word, 0, 1, 0, (rn==13), 0, 0, 0 );
        build_gip_instruction_cc( &pd->dec.arm.inst, map_condition_code( cc ) );
        build_gip_instruction_rn( &pd->dec.arm.inst, map_source_register(rn) );
        build_gip_instruction_rm( &pd->dec.arm.inst, map_source_register(rd) );
        pd->dec.arm.next_pc = pd->dec.state.pc+4;
        pd->dec.arm.next_cycle_of_opcode = 0;
        return 1;
    }
    /*b Preindexed store
     */
    else if (pre)
    {
        /*b preindexed immediate or rm no shift: IADD[CC]AC/ISUB[CC]AC Rn, #imm/Rm; ISTRCPA[S] #0, (ACC, +/-SHF) <- Rd [-> Rn]
         */
        if ( !not_imm ||
             (((t_shf_type)(shf_how)==shf_type_lsl) && (shf_imm_amt==0)) )
        {
            switch (pd->dec.state.cycle_of_opcode)
            {
            case 0:
                build_gip_instruction_alu( &pd->dec.arm.inst, gip_ins_class_arith, up?gip_ins_subclass_arith_add:gip_ins_subclass_arith_sub, 1, 0, 0, 0 );
                build_gip_instruction_cc( &pd->dec.arm.inst, map_condition_code(cc) );
                build_gip_instruction_rn( &pd->dec.arm.inst, map_source_register(rn) );
                if (!not_imm)
                {
                    build_gip_instruction_immediate( &pd->dec.arm.inst, imm_val );
                }
                else
                {
                    build_gip_instruction_rm( &pd->dec.arm.inst, map_source_register(shf_rm) );
                }
                break;
            default:
                build_gip_instruction_store( &pd->dec.arm.inst, gip_ins_subclass_memory_word, 1, up, 1, (rn==13), 0, 1, 0 );
                build_gip_instruction_cc( &pd->dec.arm.inst, gip_ins_cc_cp );
                build_gip_instruction_rn_int( &pd->dec.arm.inst, gip_ins_rnm_int_acc );
                build_gip_instruction_rm( &pd->dec.arm.inst, map_source_register(rd) );
                if (wb)
                {
                    build_gip_instruction_rd( &pd->dec.arm.inst, map_destination_register(rn) );
                }
                pd->dec.arm.next_pc = pd->dec.state.pc+4;
                pd->dec.arm.next_cycle_of_opcode = 0;
            }
            return 1;
        }
        /*b Preindexed Rm with shift: ISHF[CC] Rm, #imm; ISTRCPA[S] #0, (Rn, +/-SHF) <- Rd [-> Rn]
         */
        else
        {
            switch (pd->dec.state.cycle_of_opcode)
            {
            case 0:
                build_gip_instruction_shift( &pd->dec.arm.inst, map_shift(shf_how, 1, shf_imm_amt ), 0, 0 );
                build_gip_instruction_cc( &pd->dec.arm.inst, map_condition_code( cc ) );
                build_gip_instruction_rn( &pd->dec.arm.inst, map_source_register(shf_rm) );
                build_gip_instruction_immediate( &pd->dec.arm.inst, shf_imm_amt );
                break;
            default:
                build_gip_instruction_store( &pd->dec.arm.inst, gip_ins_subclass_memory_word, 1, up, 1, (rn==13), 0, 1, 0 );
                build_gip_instruction_cc( &pd->dec.arm.inst, gip_ins_cc_cp );
                build_gip_instruction_rn( &pd->dec.arm.inst, map_source_register(rn) );
                build_gip_instruction_rm( &pd->dec.arm.inst, map_source_register(rd) );
                if (wb)
                {
                    build_gip_instruction_rd( &pd->dec.arm.inst, map_destination_register(rn) );
                }
                pd->dec.arm.next_pc = pd->dec.state.pc+4;
                pd->dec.arm.next_cycle_of_opcode = 0;
                break;
            }
            return 1;
        }
    }
    /*b Postindexed
     */
    else // if (!pre) - must be postindexed:
    {
        /*b Postindexed immediate or reg without shift;  ISTR[CC][S] #0 (Rn) <-Rd; IADDCPA/ISUBCPA Rn, #Imm/Rm -> Rn
         */
        if ( !not_imm ||
             (((t_shf_type)(shf_how)==shf_type_lsl) && (shf_imm_amt==0)) )
        {
            switch (pd->dec.state.cycle_of_opcode)
            {
            case 0:
                build_gip_instruction_store( &pd->dec.arm.inst, gip_ins_subclass_memory_word, 0, 0, 0, (rn==13), 0, 0, 0 );
                build_gip_instruction_cc( &pd->dec.arm.inst, map_condition_code(cc) );
                build_gip_instruction_rn( &pd->dec.arm.inst, map_source_register(rn) );
                build_gip_instruction_rm( &pd->dec.arm.inst, map_source_register(rd) );
                break;
            default:
                build_gip_instruction_alu( &pd->dec.arm.inst, gip_ins_class_arith, up?gip_ins_subclass_arith_add:gip_ins_subclass_arith_sub, 1, 0, 0, 0 );
                build_gip_instruction_cc( &pd->dec.arm.inst, gip_ins_cc_cp );
                build_gip_instruction_rn( &pd->dec.arm.inst, map_source_register(rn) );
                if (!not_imm)
                {
                    build_gip_instruction_immediate( &pd->dec.arm.inst, imm_val );
                }
                else
                {
                    build_gip_instruction_rm( &pd->dec.arm.inst, map_source_register(shf_rm) );
                }
                build_gip_instruction_rd( &pd->dec.arm.inst, map_destination_register(rn) );
                pd->dec.arm.next_pc = pd->dec.state.pc+4;
                pd->dec.arm.next_cycle_of_opcode = 0;
                break;
            }
            return 1;
        }
        /*b Postindexed reg with shift
         */
        else //  ISHF[CC] Rm, #imm; ISTRCPA[S] #0 (Rn), +/-SHF) <-Rd -> Rn
        {
            switch (pd->dec.state.cycle_of_opcode)
            {
            case 0:
                build_gip_instruction_shift( &pd->dec.arm.inst, map_shift(shf_how, 1, shf_imm_amt ), 0, 0 );
                build_gip_instruction_cc( &pd->dec.arm.inst, map_condition_code( cc ) );
                build_gip_instruction_rn( &pd->dec.arm.inst, map_source_register(shf_rm) );
                build_gip_instruction_immediate( &pd->dec.arm.inst, shf_imm_amt );
                break;
            default:
                build_gip_instruction_store( &pd->dec.arm.inst, gip_ins_subclass_memory_word, 0, up, 1, (rn==13), 0, 1, 0 );
                build_gip_instruction_cc( &pd->dec.arm.inst, gip_ins_cc_cp );
                build_gip_instruction_rn( &pd->dec.arm.inst, map_source_register(rn) );
                build_gip_instruction_rm( &pd->dec.arm.inst, map_source_register(rd) );
                build_gip_instruction_rd( &pd->dec.arm.inst, map_destination_register(rn) );
                pd->dec.arm.next_pc = pd->dec.state.pc+4;
                pd->dec.arm.next_cycle_of_opcode = 0;
                break;
            }
            return 1;
        }
    }

    /*b Done
     */
    return 0;
}

/*f c_gip_full::decode_arm_ldm_stm
 */
int c_gip_full::decode_arm_ldm_stm( void )
{
    int cc, four, pre, up, psr, wb, load, rn, regs;
    int i, j, num_regs;

    /*b Decode ARM instruction
     */
    cc      = (pd->dec.state.opcode>>28) & 0x0f;
    four    = (pd->dec.state.opcode>>25) & 0x07;
    pre     = (pd->dec.state.opcode>>24) & 0x01;
    up      = (pd->dec.state.opcode>>23) & 0x01;
    psr     = (pd->dec.state.opcode>>22) & 0x01;
    wb      = (pd->dec.state.opcode>>21) & 0x01;
    load    = (pd->dec.state.opcode>>20) & 0x01;
    rn      = (pd->dec.state.opcode>>16) & 0x0f;
    regs    = (pd->dec.state.opcode>> 0) & 0xffff;

    /*b If not an LDM/STM, then return
     */
    if (four != 4)
        return 0;

    /*b Calculate memory address and writeback value
     */
    for (i=regs, num_regs=0; i; i>>=1)
    {
         if (i&1)
         {
              num_regs++;
         }
    }

    /*b Handle load
     */
    if (load)
    {
        /*b If DB/DA, do first instruction to generate base address: ISUB[CC]A Rn, #num_regs*4 [-> Rn]
         */
        if ((!up) && (pd->dec.state.cycle_of_opcode==0))
        {
            build_gip_instruction_alu( &pd->dec.arm.inst, gip_ins_class_arith, gip_ins_subclass_arith_sub, 1, 0, 0, 0 );
            build_gip_instruction_cc( &pd->dec.arm.inst, map_condition_code(cc) );
            build_gip_instruction_rn( &pd->dec.arm.inst, map_source_register(rn) );
            build_gip_instruction_immediate( &pd->dec.arm.inst, num_regs*4 );
            if (wb)
            {
                build_gip_instruction_rd( &pd->dec.arm.inst, map_destination_register(rn) );
            }
            else
            {
            }
            return 1;
        }

        /*b Generate num_regs 'ILDR[CC|CP]A[S][F] #i (Rn|Acc), #+4 or preindexed version
         */
        for (i=0; i<num_regs; i++)
        {
            for (j=0; (j<16) && ((regs&(1<<j))==0); j++);
            regs &= ~(1<<j);
            if ( (!up && (i==pd->dec.state.cycle_of_opcode+1)) ||
                 (up && (i==pd->dec.state.cycle_of_opcode)) )
            {
                build_gip_instruction_load( &pd->dec.arm.inst, gip_ins_subclass_memory_word, pre^!up, 1, (rn==13), (num_regs-1-i), 1, (j==15)&&!(up&&wb) );
                if (pd->dec.state.cycle_of_opcode==0)
                {
                    build_gip_instruction_cc( &pd->dec.arm.inst, map_condition_code(cc) );
                    build_gip_instruction_rn( &pd->dec.arm.inst, map_source_register(rn) );
                }
                else
                {
                    build_gip_instruction_cc( &pd->dec.arm.inst, gip_ins_cc_cp );
                    build_gip_instruction_rn_int( &pd->dec.arm.inst, gip_ins_rnm_int_acc );
                }
                build_gip_instruction_immediate( &pd->dec.arm.inst, 4 );
                build_gip_instruction_rd( &pd->dec.arm.inst, map_destination_register(j) );
            }
            if ( (!up && (pd->dec.state.cycle_of_opcode==num_regs)) ||
                 ((up&&!wb) && (pd->dec.state.cycle_of_opcode==num_regs-1)) )
            {
                pd->dec.arm.next_pc = pd->dec.state.pc+4;
                pd->dec.arm.next_cycle_of_opcode = 0;
            }
        }

        /*b If IB/IA with writeback then do final MOVCP[F] Acc -> Rn; F if PC was read in the list
         */
        if ((up&&wb) && (pd->dec.state.cycle_of_opcode==num_regs))
        {
            build_gip_instruction_alu( &pd->dec.arm.inst, gip_ins_class_logic, gip_ins_subclass_logic_mov, 0, 0, 0, ((pd->dec.state.opcode&0x8000)!=0) );
            build_gip_instruction_cc( &pd->dec.arm.inst, gip_ins_cc_cp );
            build_gip_instruction_rm_int( &pd->dec.arm.inst, gip_ins_rnm_int_acc );
            build_gip_instruction_rd( &pd->dec.arm.inst, map_destination_register(rn) );
            pd->dec.arm.next_pc = pd->dec.state.pc+4;
            pd->dec.arm.next_cycle_of_opcode = 0;
        }
        return 1;
    }

    /*b Handle store
     */
    if (!load)
    {
        /*b If DB/DA, do first instruction to generate base address: ISUB[CC]A Rn, #num_regs*4 [-> Rn]
         */
        if ((!up) && (pd->dec.state.cycle_of_opcode==0))
        {
            build_gip_instruction_alu( &pd->dec.arm.inst, gip_ins_class_arith, gip_ins_subclass_arith_sub, 1, 0, 0, 0 );
            build_gip_instruction_cc( &pd->dec.arm.inst, map_condition_code(cc) );
            build_gip_instruction_rn( &pd->dec.arm.inst, map_source_register(rn) );
            build_gip_instruction_immediate( &pd->dec.arm.inst, num_regs*4 );
            if (wb)
            {
                build_gip_instruction_rd( &pd->dec.arm.inst, map_destination_register(rn) );
            }
            else
            {
            }
            return 1;
        }

        /*b Generate num_regs 'ISTR[CC|CP]A[S][F] #i (Rn|Acc), #+4 [->Rn] or preindexed version
         */
        for (i=0; i<num_regs; i++)
        {
            for (j=0; (j<16) && ((regs&(1<<j))==0); j++);
            regs &= ~(1<<j);
            if ( (!up && (pd->dec.state.cycle_of_opcode==(i+1))) ||
                 (up && (pd->dec.state.cycle_of_opcode==i)) )
            {
                build_gip_instruction_store( &pd->dec.arm.inst, gip_ins_subclass_memory_word, pre^!up, 1, 0, (rn==13), (num_regs-1-i), 1, 0 );
                if (pd->dec.state.cycle_of_opcode==0)
                {
                    build_gip_instruction_cc( &pd->dec.arm.inst, map_condition_code(cc) );
                    build_gip_instruction_rn( &pd->dec.arm.inst, map_source_register(rn) );
                }
                else
                {
                    build_gip_instruction_cc( &pd->dec.arm.inst, gip_ins_cc_cp );
                    build_gip_instruction_rn_int( &pd->dec.arm.inst, gip_ins_rnm_int_acc );
                }
                build_gip_instruction_rm( &pd->dec.arm.inst, map_source_register(j) );
                if ((pd->dec.state.cycle_of_opcode==num_regs-1) && (up) && (wb))
                {
                    build_gip_instruction_rd( &pd->dec.arm.inst, map_destination_register(rn) );
                }
                else
                {
                }
                if ( (!up && (pd->dec.state.cycle_of_opcode==num_regs)) ||
                     (up && (pd->dec.state.cycle_of_opcode==num_regs-1)) )
                {
                    pd->dec.arm.next_pc = pd->dec.state.pc+4;
                    pd->dec.arm.next_cycle_of_opcode = 0;
                }
            }
        }

        return 1;
    }

    /*b Done
     */
    return 0; 
}

/*f c_gip_full::decode_arm_mul
 */
int c_gip_full::decode_arm_mul( void )
{
    int cc, zero, nine, accum, sign, rd, rn, rs, rm;

    /*b Decode ARM instruction
     */
    cc      = (pd->dec.state.opcode>>28) & 0x0f;
    zero    = (pd->dec.state.opcode>>22) & 0x3f;
    accum   = (pd->dec.state.opcode>>21) & 0x01;
    sign    = (pd->dec.state.opcode>>20) & 0x01;
    rd      = (pd->dec.state.opcode>>16) & 0x0f;
    rn      = (pd->dec.state.opcode>>12) & 0x0f;
    rs      = (pd->dec.state.opcode>>8) & 0x0f;
    nine    = (pd->dec.state.opcode>>4) & 0x0f;
    rm      = (pd->dec.state.opcode>>0) & 0x0f;

    /*b Validate MUL/MLA instruction
     */
    if ((zero!=0) || (nine!=9) || (cc==15))
        return 0;

    /*b Decode according to stage
     */
    switch (pd->dec.state.cycle_of_opcode)
    {
    case 0:
        /*b First the INIT instruction
         */
        build_gip_instruction_alu( &pd->dec.arm.inst, gip_ins_class_arith, gip_ins_subclass_arith_init, 1, 0, 0, 0 );
        build_gip_instruction_cc( &pd->dec.arm.inst, map_condition_code(cc) );
        build_gip_instruction_rn( &pd->dec.arm.inst, map_source_register(rm) ); // Note these are
        if (accum)
        {
            build_gip_instruction_rm( &pd->dec.arm.inst, map_source_register(rn) ); //  correctly reversed
        }
        else
        {
            build_gip_instruction_immediate( &pd->dec.arm.inst, 0 );
        }
        break;
    case 1:
        /*b Then the MLA instruction to get the ALU inputs ready, and do the first step
         */
        build_gip_instruction_alu( &pd->dec.arm.inst, gip_ins_class_arith, gip_ins_subclass_arith_mla, 1, 0, 0, 0 );
        build_gip_instruction_cc( &pd->dec.arm.inst, gip_ins_cc_cp );
        build_gip_instruction_rn_int( &pd->dec.arm.inst, gip_ins_rnm_int_acc );
        build_gip_instruction_rm( &pd->dec.arm.inst, map_source_register(rs) );
        break;
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
    case 15:
        /*b Then 14 MLB instructions to churn
         */
        build_gip_instruction_alu( &pd->dec.arm.inst, gip_ins_class_arith, gip_ins_subclass_arith_mlb, 1, 0, 0, 0 );
        build_gip_instruction_cc( &pd->dec.arm.inst, gip_ins_cc_cp );
        build_gip_instruction_rn_int( &pd->dec.arm.inst, gip_ins_rnm_int_acc );
        build_gip_instruction_immediate( &pd->dec.arm.inst, 0 ); // Not used
        break;
    default:
        /*b Then the last MLB instructions to produce the final results
         */
        build_gip_instruction_alu( &pd->dec.arm.inst, gip_ins_class_arith, gip_ins_subclass_arith_mlb, 1, sign, 0, 0 );
        build_gip_instruction_cc( &pd->dec.arm.inst, gip_ins_cc_cp );
        build_gip_instruction_rn_int( &pd->dec.arm.inst, gip_ins_rnm_int_acc );
        build_gip_instruction_immediate( &pd->dec.arm.inst, 0 ); // Not used
        build_gip_instruction_rd( &pd->dec.arm.inst, map_destination_register(rd) );
        pd->dec.arm.next_pc = pd->dec.state.pc+4;
        pd->dec.arm.next_cycle_of_opcode = 0;
        break;
    }

    /*b Done
     */
    return 1; 
}

/*f c_gip_full::decode_arm_trace
  Steal top half of SWI space
*/
int c_gip_full::decode_arm_trace( void )
{
    int cc, fifteen, code;

    /*b Decode ARM instruction
     */
    cc      = (pd->dec.state.opcode>>28) & 0xf;
    fifteen = (pd->dec.state.opcode>>24) & 0xf;
    code    = (pd->dec.state.opcode>> 0) & 0xffffff;

    if (fifteen!=15)
        return 0;

    if (1 || (code & 0x800000))
    {
        return 0;
    }
    return 0;
}

/*a Debug interface
 */
/*f c_gip_full::set_register
 */
void c_gip_full::set_register( int r, unsigned int value )
{
    pd->rf.state.regs[r&0x1f] = value;
}

/*f c_gip_full::get_register
 */
unsigned int c_gip_full::get_register( int r )
{
    return pd->rf.state.regs[r&0x1f];
}

/*f c_gip_full::set_flags
 */
void c_gip_full::set_flags( int value, int mask )
{
    if (mask & gip_flag_mask_z)
        pd->alu.state.z = ((value & gip_flag_mask_z)!=0);
    if (mask & gip_flag_mask_c)
        pd->alu.state.c = ((value & gip_flag_mask_c)!=0);
    if (mask & gip_flag_mask_n)
        pd->alu.state.n = ((value & gip_flag_mask_n)!=0);
    if (mask & gip_flag_mask_v)
        pd->alu.state.v = ((value & gip_flag_mask_v)!=0);
}

/*f c_gip_full::get_flags
 */
int c_gip_full::get_flags( void )
{
    int result;
    result = 0;
    if (pd->alu.state.z)
        result |= gip_flag_mask_z;
    if (pd->alu.state.c)
        result |= gip_flag_mask_c;
    if (pd->alu.state.n)
        result |= gip_flag_mask_n;
    if (pd->alu.state.v)
        result |= gip_flag_mask_v;
    return result;
}

/*f c_gip_full::set_breakpoint
 */
int c_gip_full::set_breakpoint( unsigned int address )
{
    int bkpt;
    for (bkpt = 0; bkpt < MAX_BREAKPOINTS && (pd->breakpoints_enabled & (1<<bkpt)); bkpt++)
	    ;
    if ((bkpt<0) || (bkpt>=MAX_BREAKPOINTS))
        return 0;
    pd->breakpoint_addresses[bkpt] = address;
    pd->breakpoints_enabled = pd->breakpoints_enabled | (1<<bkpt);
    pd->debugging_enabled = 1;
//    printf("c_gip_full::set_breakpoint:%d:%08x\n", bkpt, address );
    return 1;
}

/*f c_gip_full::unset_breakpoint
 */
int c_gip_full::unset_breakpoint( unsigned int address )
{
    int bkpt;
    for (bkpt = 0; bkpt < MAX_BREAKPOINTS && pd->breakpoint_addresses[bkpt] != address; bkpt++)
	    ;
    if ((bkpt<0) || (bkpt>=MAX_BREAKPOINTS))
        return 0;
    pd->breakpoints_enabled = pd->breakpoints_enabled &~ (1<<bkpt);
    SET_DEBUGGING_ENABLED(pd);
//    printf("c_gip_full::unset_breakpoint:%d\n", bkpt );
    return 1;
}

/*f c_gip_full::halt_cpu
  If we are currently executing, forces an immediate halt
  This is used by a memory model to abort execution after an unmapped access, for example
*/
void c_gip_full::halt_cpu( void )
{
    pd->debugging_enabled = 1;
    pd->halt = 1;
    printf("c_gip_full::halt_cpu\n");
}

/*a Special register handling
 */
/*f c_gip_full::special_comb
 */
void c_gip_full::special_comb( int read_select, int read_address, unsigned int *read_data )
{
    unsigned int result;
    result = 0;
    switch ((t_gip_special_reg)read_address)
    {
    case gip_special_reg_semaphores:
        result = pd->special.state.semaphores;
        break;
    case gip_special_reg_thread:
        result = pd->special.state.selected_thread;
        break;
    case gip_special_reg_thread_pc:
        result = pd->special.state.thread_data_pc;
        break;
    case gip_special_reg_thread_data:
        result = pd->special.state.thread_data_config;
        break;
    case gip_special_reg_preempted_pc_l:
        printf("special_comb:Do not have mechanism for preemption yet\n");
        result = 0;
        break;
    case gip_special_reg_preempted_pc_m:
        printf("special_comb:Do not have mechanism for preemption yet\n");
        result = 0;
        break;
    case gip_special_reg_preempted_flags:
        printf("special_comb:Do not have mechanism for preemption yet\n");
        result = 0;
        break;
    default:
        break;
    }
    *read_data = result;
}

/*f c_gip_full::special_preclock
 */
void c_gip_full::special_preclock( int flush, int read_select, int read_address, int write_select, int write_address, unsigned int write_data )
{
    /*b Copy current to next
     */
    memcpy( &pd->special.next_state, &pd->special.state, sizeof(pd->special.state) );

    /*b Default values for interacting with scheduler
     */
    pd->special.next_state.thread_data_write_pc = 0;
    pd->special.next_state.thread_data_write_config = 0;
    pd->special.next_state.thread_data_pc = pd->sched.thread_data_pc;
    pd->special.next_state.thread_data_config = pd->sched.thread_data_config;
    pd->special.next_state.write_thread = pd->special.state.selected_thread;

    /*b Handle writes
     */
    switch ((t_gip_special_reg)write_address)
    {
    case gip_special_reg_gip_config:
        if (write_select)
        {
            pd->special.next_state.cooperative = (write_data>>0)&1;
            pd->special.next_state.round_robin = (write_data>>1)&1;
            pd->special.next_state.thread_0_privilege = (write_data>>2)&1;
            pd->arm_trap_semaphore = (write_data>>16)&0x1f;
        }
        break;
    case gip_special_reg_semaphores_set:
        if (write_select)
        {
            pd->special.next_state.semaphores = pd->special.state.semaphores | write_data;
        }
        break;
    case gip_special_reg_semaphores_clear:
        if (write_select)
        {
            pd->special.next_state.semaphores = pd->special.state.semaphores &~ write_data;
        }
        break;
    case gip_special_reg_thread:
        if (write_select)
        {
            pd->special.next_state.selected_thread = write_data & (NUM_THREADS-1);
        }
        break;
    case gip_special_reg_thread_pc:
        if (write_select)
        {
            pd->special.next_state.thread_data_write_pc = 1;
            pd->special.next_state.thread_data_pc = write_data&~1;
            if (write_data&1)
            {
                pd->special.next_state.write_thread = pd->sched.state.thread;
            }
            else
            {
                pd->special.next_state.write_thread = pd->special.state.selected_thread;
            }
            printf("special_preclock:Do not have mechanism for current thread yet\n");
        }
        break;
    case gip_special_reg_thread_data:
        if (write_select)
        {
            pd->special.next_state.thread_data_write_config = 1;
            pd->special.next_state.thread_data_config = write_data;
            if (write_data&1)
            {
                pd->special.next_state.write_thread = pd->sched.state.thread;
            }
            else
            {
                pd->special.next_state.write_thread = pd->special.state.selected_thread;
            }
        }
        break;
    case gip_special_reg_repeat_count:
        if (write_select)
        {
            pd->special.next_state.repeat_count = write_data&0xff;
        }
        break;
    default:
        break;
    }
}

/*f c_gip_full::special_clock
 */
void c_gip_full::special_clock( void )
{
    memcpy( &pd->special.state, &pd->special.next_state, sizeof(pd->special.state) );
}

/*a Postbus register handling
 */
/*f c_gip_full::postbus_comb
 */
void c_gip_full::postbus_comb( int read_select, int read_address, unsigned int *read_data )
{
}

/*f c_gip_full::postbus_preclock
 */
void c_gip_full::postbus_preclock( int flush, int read_select, int read_address, int write_select, int write_address, unsigned int write_data )
{
}

/*f c_gip_full::postbus_clock
 */
void c_gip_full::postbus_clock( void )
{
}

