/*a Includes
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "c_gip_pipeline_single.h"
#include "c_memory_model.h"
#include "symbols.h"
#include "syscalls.h"
#include "arm_dis.h"
#include "gip_internals.h"
#include "gip_instructions.h"

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

/*t t_gip_sch_thread - Per thread register file storage
 */
typedef struct t_gip_sch_thread
{
    unsigned int restart_pc;
    int flag_dependencies; // 4-bit field indicating which sempahores are important
    int register_map;
    int processor_mode; // 1 for ARM, 0 for GIP native
    int running; // 1 if the thread has been scheduled and is running (or is currently preempted)
} t_gip_sch_thread;

/*t t_private_data
 */
typedef struct t_private_data
{
    c_memory_model *memory;
    int cycle;

    /*b Configuration of the pipeline
     */
    int sticky_v;
    int sticky_z;
    int sticky_c;
    int sticky_n;
    int cp_trail_2; // Assert to have a 'condition passed' trail of 2, else it is one.

    /*b State of the scheduler
     */
    t_gip_sch_thread sch_thread_data[8]; // 'Register file' of thread data for all eight threads
    unsigned int sch_semaphores;
    int sch_mode; // 0 is round-robin cooperative, 1 is prioritized cooperative, 2 is full-blown prioritized preemption
    int sch_thread; // Actual thread number that is currently running
    int sch_running; // 1 if a thread is running, 0 otherwise

    /*b Combinatorials in the scheduler
     */

    /*b State in the pipeline guaranteed by the design
     */
    int reg_in_acc; // register number (0 through 14) that is in the accumulator; 15 means none.
    unsigned int pc;
    unsigned int regs[ MAX_REGISTERS ]; // register file
    unsigned int alu_a_in;
    unsigned int alu_b_in;
    unsigned int acc; // Accumulator value
    unsigned int shf; // Shifter value
    int z;
    int c;
    int v; // last V flag from the ALU, or sticky version thereof
    int n; // last N flag from the ALU
    int p; // last carry out of the shifter
    int cp; // last condition passed indication
    int old_cp; // previous-to-last condition passed indication; use if we have a 'trail' of 2
    unsigned int alu_result; // Result of the ALU for writing to the register file
    unsigned int mem_result; // Result of a memory read
    t_gip_ins_rd alu_rd; // Type of register file write from the ALU path
    t_gip_ins_rd mem_rd; // Type of register file write from the memory path

    /*b Combinatorials in the pipeline
     */
    int condition_passed; // Requested condition in the ALU stage passed
    t_gip_alu_op1_src op1_src;
    t_gip_alu_op2_src op2_src;
    unsigned int alu_constant; // Constant 0, 1, 2 or 4 for offsets for stores
    int set_acc;
    int set_zcvn;
    int set_p;
    unsigned int alu_op1; // Operand 1 to the ALU logic itself
    unsigned int alu_op2; // Operand 2 to the ALU logic itself
    int alu_c;
    int alu_v;
    unsigned int shf_result; // Result of the shifter
    int shf_carry; // Carry out of the shifter
    unsigned int arith_result; // Result of the arithmetic operation
    unsigned int logic_result; // Result of the logical operation

    /*b Irrelevant stuff for now
     */
    unsigned int last_address;
    int seq_count;
    int debugging_enabled;
    int breakpoints_enabled;
    unsigned int breakpoint_addresses[ MAX_BREAKPOINTS ];
    int halt;
    
    int interrupt_handler[MAX_INTERRUPTS];
    int swi_return_addr;
    int swi_sp;
    int swi_code;
    int kernel_sp;
    int super;

    struct microkernel * ukernel;
} t_private_data;

/*a Static variables
 */
static t_gip_ins_rd gip_ins_rd_none = { gip_ins_rd_type_internal, {gip_ins_rd_int_none}};
static t_gip_ins_rd gip_ins_rd_pc = { gip_ins_rd_type_internal, {gip_ins_rd_int_pc}};
static t_gip_ins_rd gip_ins_rd_r14 = { gip_ins_rd_type_register, {14}};
static t_gip_ins_rnm gip_ins_rnm_acc = { gip_ins_rnm_type_internal, {gip_ins_rnm_int_acc}};
static t_gip_ins_rnm gip_ins_rnm_shf = { gip_ins_rnm_type_internal, {gip_ins_rnm_int_shf}};
static t_gip_ins_rnm gip_ins_rnm_pc = { gip_ins_rnm_type_internal, {gip_ins_rnm_int_pc}};

/*a Support functions
 */
/*f is_condition_met
 */
static int is_condition_met( t_gip_ins_cc gip_ins_cc, t_private_data *pd )
{
    switch (gip_ins_cc)
    {
    case gip_ins_cc_eq:
        return pd->z;
        break;
    case gip_ins_cc_ne:
        return !pd->z;
        break;
    case gip_ins_cc_cs:
        return pd->c;
        break;
    case gip_ins_cc_cc:
        return !pd->c;
        break;
    case gip_ins_cc_mi:
        return pd->n;
        break;
    case gip_ins_cc_pl:
        return !pd->n;
        break;
    case gip_ins_cc_vs:
        return pd->v;
        break;
    case gip_ins_cc_vc:
        return !pd->v;
        break;
    case gip_ins_cc_hi:
        return pd->c && !pd->z;
        break;
    case gip_ins_cc_ls:
        return !pd->c || pd->z;
        break;
    case gip_ins_cc_ge:
        return (!pd->n && !pd->v) || (pd->n && pd->v);
        break;
    case gip_ins_cc_lt:
        return (!pd->n && pd->v) || (pd->n && !pd->v);
        break;
    case gip_ins_cc_gt:
        return ((!pd->n && !pd->v) || (pd->n && pd->v)) && !pd->z;
        break;
    case gip_ins_cc_le:
        return (!pd->n && pd->v) || (pd->n && !pd->v) || pd->z;
        break;
    case gip_ins_cc_always:
        return 1;
        break;
    case gip_ins_cc_cp:
        return pd->cp; // Or in old_cp if the shadow is 'long'
        break;
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
            return ((val>>31)&1) ? 0xfffffff : 0;
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
        return val>>by;
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

/*a Constructors/destructors
 */
/*f c_gip_pipeline_single::c_gip_pipeline_single
 */
c_gip_pipeline_single::c_gip_pipeline_single( c_memory_model *memory ) : c_execution_model_class( memory )
{
    int i;

    printf("This %p pd %p\n", this, &pd );
    fflush(stdout);
    pd = (t_private_data *)malloc(sizeof(t_private_data));
    //memset (pd, 0, sizeof(*pd));
    pd->cycle = 0;
    pd->memory = memory;

    for (i=0; i<8; i++)
    {
        pd->sch_thread_data[i].restart_pc = 0;
        pd->sch_thread_data[i].flag_dependencies = 0;
        pd->sch_thread_data[i].register_map = 0;
        pd->sch_thread_data[i].processor_mode = 0;
        pd->sch_thread_data[i].running = 0;
    }
    pd->sch_semaphores = 1;
    pd->sch_mode = 0;
    pd->sch_thread_data[0].running = 1;
    pd->sch_thread = 0;
    pd->sch_running = 0;

    pd->pc = 0;
    for (i=0; i<MAX_REGISTERS; i++)
    {
        pd->regs[i] = 0;
    }
    pd->acc = 0;
    pd->shf = 0;
    pd->z = 0;
    pd->c = 0;
    pd->v = 0;
    pd->n = 0;
    pd->p = 0;
    pd->cp = 0;

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

/*f c_gip_pipeline_single::~c_gip_pipeline_single
 */
c_gip_pipeline_single::~c_gip_pipeline_single()
{
}

/*a Debug information methods
 */
/*f c_gip_pipeline_single:debug
 */
void c_gip_pipeline_single::debug( int mask )
{
    char buffer[256];
    unsigned int address;
    unsigned int opcode;

    if (mask&8)
    {
        address = pd->pc;
        if (address!=pd->last_address+4)
        {
            pd->last_address = address;
            printf("%d *** 0x%08x:\n", pd->seq_count, address );
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
                pd->regs[0],
                pd->regs[1],
                pd->regs[2],
                pd->regs[3] );
        printf( "\t r4: %08x   r5: %08x   r6: %08x   r7: %08x\n",
                pd->regs[4],
                pd->regs[5],
                pd->regs[6],
                pd->regs[7] );
        printf( "\t r8: %08x   r9: %08x  r10: %08x  r11: %08x\n",
                pd->regs[8],
                pd->regs[9],
                pd->regs[10],
                pd->regs[11] );
        printf( "\tr12: %08x  r13: %08x  r14: %08x  r15: %08x\n",
                pd->regs[12],
                pd->regs[13],
                pd->regs[14],
                pd->regs[15] );
    }
    if (mask&4)
    {
        printf( "\t  acc:%08x shf:%08x c:%d  n:%d  z:%d  v:%d p:%d cp:%d\n",
                pd->acc,
                pd->shf,
                pd->c,
                pd->n,
                pd->z,
                pd->v,
                pd->p,
                pd->cp );
        printf( "\t  alu a:%08x alu b:%08x\n",
                pd->alu_a_in,
                pd->alu_b_in );
    }
    if (mask&1)
    {
        address = pd->pc;
        opcode = pd->memory->read_memory( pd->pc );
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
/*f c_gip_pipeline_single:step
  Returns number of instructions executed
 */
int c_gip_pipeline_single::step( int *reason, int requested_count )
{
    int i;
    unsigned int opcode;
    char buffer[256];

    *reason = 0;

    /*b Loop for requested count
     */
    for (i=0; i<requested_count; i++)
    {
        /*b fetch from pc
         */
        log_reset();
        opcode = pd->memory->read_memory( pd->pc );
        arm_disassemble( pd->pc, opcode, buffer );
        printf( "%08x %08x: %s\n", pd->pc, opcode, buffer );

        log( "address", pd->pc );
        log( "opcode", opcode );

        /*b attempt to execute opcode, trying each instruction coding one at a time till we succeed
         */
        if ( execute_arm_mul( opcode ) ||
             execute_arm_alu( opcode ) ||
             execute_arm_ld_st( opcode ) ||
             execute_arm_ldm_stm( opcode ) ||
             execute_arm_branch( opcode ) ||
             execute_arm_trace( opcode ) ||
             0 )
        {
            //log_display( stdout );
            continue;
        }
        *reason = opcode;
        printf( "failed to execute %x\n", opcode );
        debug(-1);
        return i;
        if (1) 
        {
            pd->pc += 4;
            continue;
        } 
        return i;
    }
    return requested_count;
}

/*f c_gip_pipeline_single::load_code
 */
void c_gip_pipeline_single::load_code( FILE *f, unsigned int base_address )
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

/*f c_gip_pipeline_single::load_code_binary
 */
void c_gip_pipeline_single::load_code_binary( FILE *f, unsigned int base_address )
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

/*f c_gip_pipeline_single::load_symbol_table
 */
void c_gip_pipeline_single::load_symbol_table( char *filename )
{
    symbol_initialize( filename );
}

/*a Internal instruction execution methods
 */
/*f c_gip_pipeline_single::execute_int_alu_instruction
 */
void c_gip_pipeline_single::execute_int_alu_instruction( t_gip_alu_op alu_op, t_gip_alu_op1_src op1_src, t_gip_alu_op2_src op2_src, int a, int s, int p )
{
    /*b Determine inputs to the shifter and ALU
     */
    switch (op1_src)
    {
    case gip_alu_op1_src_a_in:
        pd->alu_op1 = pd->alu_a_in;
        break;
    case gip_alu_op1_src_acc:
        pd->alu_op1 = pd->acc;
        break;
    }
    switch (op2_src)
    {
    case gip_alu_op2_src_b_in:
        pd->alu_op2 = pd->alu_b_in;
        break;
    case gip_alu_op2_src_acc:
        pd->alu_op2 = pd->acc;
        break;
    case gip_alu_op2_src_shf:
        pd->alu_op2 = pd->shf;
        break;
    case gip_alu_op2_src_constant:
        pd->alu_op2 = pd->alu_constant;
        break;
    }

    /*b Perform shifter operation
     */
    switch (alu_op)
    {
    case gip_alu_op_lsl:
        pd->shf_result = barrel_shift( pd->c, shf_type_lsl, pd->alu_a_in, pd->alu_b_in, &pd->shf_carry );
        break;
    case gip_alu_op_lsr:
        pd->shf_result = barrel_shift( pd->c, shf_type_lsr, pd->alu_a_in, pd->alu_b_in, &pd->shf_carry );
        break;
    case gip_alu_op_asr:
        pd->shf_result = barrel_shift( pd->c, shf_type_asr, pd->alu_a_in, pd->alu_b_in, &pd->shf_carry );
        break;
    case gip_alu_op_ror:
        pd->shf_result = barrel_shift( pd->c, shf_type_ror, pd->alu_a_in, pd->alu_b_in, &pd->shf_carry );
        break;
    case gip_alu_op_ror33:
        pd->shf_result = barrel_shift( pd->c, shf_type_rrx, pd->alu_a_in, pd->alu_b_in, &pd->shf_carry );
        break;
    case gip_alu_op_init:
        pd->shf_result = barrel_shift( 0&pd->c, shf_type_lsr, pd->alu_a_in, 0, &pd->shf_carry );
        break;
    case gip_alu_op_mla:
    case gip_alu_op_mlb:
        pd->shf_result = barrel_shift( pd->c, shf_type_lsr, pd->shf, 2, &pd->shf_carry );
        break;
    case gip_alu_op_divst:
        break;
    default:
        break;
    }

    /*b Perform logical operation
     */
    switch (alu_op)
    {
    case gip_alu_op_mov:
        pd->logic_result = pd->alu_op2;
        break;
    case gip_alu_op_mvn:
        pd->logic_result = ~pd->alu_op2;
        break;
    case gip_alu_op_and:
        pd->logic_result = pd->alu_op1 & pd->alu_op2;
        break;
    case gip_alu_op_or:
        pd->logic_result = pd->alu_op1 | pd->alu_op2;
        break;
    case gip_alu_op_xor:
        pd->logic_result = pd->alu_op1 ^ pd->alu_op2;
        break;
    case gip_alu_op_bic:
        pd->logic_result = pd->alu_op1 &~ pd->alu_op2;
        break;
    case gip_alu_op_orn:
        pd->logic_result = pd->alu_op1 |~ pd->alu_op2;
        break;
    case gip_alu_op_init:
    case gip_alu_op_xorcnt:
    case gip_alu_op_mla:
    case gip_alu_op_mlb:
    case gip_alu_op_divst:
        break;
    default:
        break;
    }

    /*b Perform arithmetic operation
     */
    switch (alu_op)
    {
    case gip_alu_op_add:
        pd->arith_result = add_op( pd->alu_op1, pd->alu_op2, 0, &pd->alu_c, &pd->alu_v );
        break;
    case gip_alu_op_adc:
        pd->arith_result = add_op( pd->alu_op1, pd->alu_op2, pd->c, &pd->alu_c, &pd->alu_v );
        break;
    case gip_alu_op_sub:
        pd->arith_result = add_op( pd->alu_op1, ~pd->alu_op2, 1, &pd->alu_c, &pd->alu_v );
        break;
    case gip_alu_op_sbc:
        pd->arith_result = add_op( pd->alu_op1, ~pd->alu_op2, pd->c, &pd->alu_c, &pd->alu_v );
        break;
    case gip_alu_op_rsub:
        pd->arith_result = add_op( ~pd->alu_op1, pd->alu_op2, 1, &pd->alu_c, &pd->alu_v );
        break;
    case gip_alu_op_rsbc:
        pd->arith_result = add_op( ~pd->alu_op1, pd->alu_op2, pd->c, &pd->alu_c, &pd->alu_v );
        break;
    case gip_alu_op_xorcnt:
        break;
    case gip_alu_op_init:
        pd->arith_result = add_op( 0&pd->alu_op1, pd->alu_op2, 0, &pd->alu_c, &pd->alu_v );
        break;
    case gip_alu_op_mla:
    case gip_alu_op_mlb:
        switch ((pd->shf&3)+pd->p)
        {
        case 0:
            pd->arith_result = add_op( pd->alu_op1, 0&pd->alu_op2, 0, &pd->alu_c, &pd->alu_v );
            pd->shf_carry = 0;
            break;
        case 1:
            pd->arith_result = add_op( pd->alu_op1, pd->alu_op2, 0, &pd->alu_c, &pd->alu_v );
            pd->shf_carry = 0;
            break;
        case 2:
            pd->arith_result = add_op( pd->alu_op1, pd->alu_op2<<1, 0, &pd->alu_c, &pd->alu_v );
            pd->shf_carry = 0;
            break;
        case 3:
            pd->arith_result = add_op( pd->alu_op1, ~pd->alu_op2, 1, &pd->alu_c, &pd->alu_v );
            pd->shf_carry = 1;
            break;
        case 4:
            pd->arith_result = add_op( pd->alu_op1, 0&pd->alu_op2, 0, &pd->alu_c, &pd->alu_v );
            pd->shf_carry = 1;
            break;
        }
        break;
    case gip_alu_op_divst:
        break;
    default:
        break;
    }

    /*b Update ALU result, accumulator, shifter and flags
     */
    switch (alu_op)
    {
    case gip_alu_op_mov:
    case gip_alu_op_mvn:
    case gip_alu_op_and:
    case gip_alu_op_or:
    case gip_alu_op_xor:
    case gip_alu_op_bic:
    case gip_alu_op_orn:
        pd->alu_result = pd->logic_result;
        if (p)
        {
            pd->c = pd->p;
        }
        if (s)
        {
            pd->z = (pd->logic_result==0);
            pd->n = ((pd->logic_result&0x80000000)!=0);
        }
        if (a)
        {
            pd->acc = pd->logic_result;
        }
        break;
    case gip_alu_op_add:
    case gip_alu_op_adc:
    case gip_alu_op_sub:
    case gip_alu_op_sbc:
    case gip_alu_op_rsub:
    case gip_alu_op_rsbc:
        pd->alu_result = pd->arith_result;
        if (s)
        {
            pd->z = (pd->arith_result==0);
            pd->n = ((pd->arith_result&0x80000000)!=0);
            pd->c = pd->alu_c;
            pd->v = pd->alu_v;
        }
        if (a)
        {
            pd->acc = pd->arith_result;
        }
        pd->shf = 0;// should only be with the 'C' flag, but have not defined that yet
        break;
    case gip_alu_op_init:
    case gip_alu_op_mla:
    case gip_alu_op_mlb:
        pd->alu_result = pd->arith_result;
        if (s)
        {
            pd->z = (pd->arith_result==0);
            pd->n = ((pd->arith_result&0x80000000)!=0);
            pd->c = pd->alu_c;
            pd->v = pd->alu_v;
        }
        if (a)
        {
            pd->acc = pd->arith_result;
        }
        pd->shf = pd->shf_result;
        pd->p = pd->shf_carry;
        break;
    case gip_alu_op_lsl:
    case gip_alu_op_lsr:
    case gip_alu_op_asr:
    case gip_alu_op_ror:
    case gip_alu_op_ror33:
        pd->shf = pd->shf_result;
        if (s)
        {
            pd->z = (pd->shf_result==0);
            pd->n = ((pd->shf_result&0x80000000)!=0);
            pd->c = pd->shf_carry;
        }
        pd->p = pd->shf_carry;
        break;
    case gip_alu_op_divst:
        break;
    case gip_alu_op_xorcnt:
        break;
    }

    /*b Done
     */
}

/*f c_gip_pipeline_single::execute_int_memory_instruction
 */
void c_gip_pipeline_single::execute_int_memory_instruction( t_gip_mem_op gip_mem_op, unsigned int address, unsigned int data_in )
{
    int offset_in_word;
    unsigned int data;

    switch (gip_mem_op)
    {
    case gip_mem_op_none:
        break;
    case gip_mem_op_store_word:
        pd->memory->write_memory( address, data_in, 0xf );
        break;
    case gip_mem_op_store_half:
        offset_in_word = address&0x2;
        address &= ~2;
        pd->memory->write_memory( address, data_in<<(8*offset_in_word), 3<<offset_in_word );
        break;
    case gip_mem_op_store_byte:
        offset_in_word = address&0x3;
        address &= ~3;
        pd->memory->write_memory( address, data_in<<(8*offset_in_word), 1<<offset_in_word );
        break;
    case gip_mem_op_load_word:
        pd->mem_result = pd->memory->read_memory( address );
        break;
    case gip_mem_op_load_half:
        data = pd->memory->read_memory( address );
        offset_in_word = address&0x2;
        data >>= 8*offset_in_word;
        pd->mem_result = data;
        break;
    case gip_mem_op_load_byte:
        data = pd->memory->read_memory( address );
        offset_in_word = address&0x3;
        data >>= 8*offset_in_word;
        pd->mem_result = data;
        break;
    default:
        break;
    }
}

/*f c_gip_pipeline_single::read_int_register
 */
unsigned int c_gip_pipeline_single::read_int_register( t_gip_instruction *instr, t_gip_ins_rnm r )
{
    if (r.type==gip_ins_rnm_type_register)
    {
        return pd->regs[r.data.r&0x1f];
    }
    return instr->pc;
}

/*f c_gip_pipeline_single::disassemble_int_instruction
 */
void c_gip_pipeline_single::disassemble_int_instruction( t_gip_instruction *inst )
{
    char *op;
    char *cc;
    char *rn, *rm, *rd;
    const char *offset;
    char rn_buffer[8], rm_buffer[16], rd_buffer[8];
    int no_rn;
    char text[128];

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
    case gip_ins_rd_type_internal:
        switch (inst->gip_ins_rd.data.internal)
        {
        case gip_ins_rd_int_none:
            rd = "";
            break;
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
        case gip_ins_rd_int_mi:
            rd = "-> MI";
            break;
        case gip_ins_rd_int_pl:
            rd = "-> PL";
            break;
        case gip_ins_rd_int_vs:
            rd = "-> VS";
            break;
        case gip_ins_rd_int_vc:
            rd = "-> VC";
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
    case gip_ins_rd_type_register:
        rd = rd_buffer;
        sprintf( rd_buffer, "-> R%d", inst->gip_ins_rd.data.r );
        break;
    default:
        rd = rd_buffer;
        sprintf( rd_buffer, "-> S/C%d", inst->gip_ins_rd.data.r );
        break;
    }

    /*b Get text of Rn
     */
    switch (inst->gip_ins_rn.type)
    {
    case gip_ins_rnm_type_internal:
        switch (inst->gip_ins_rn.data.internal)
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
            rn = "<NONE>";
            break;
        }
        break;
    case gip_ins_rnm_type_register:
        rn = rn_buffer;
        sprintf( rn_buffer, "R%d", inst->gip_ins_rn.data.r );
        break;
    default:
        rn = rn_buffer;
        sprintf( rn_buffer, "S/C%d", inst->gip_ins_rn.data.r );
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
        case gip_ins_rnm_type_internal:
            switch (inst->rm_data.gip_ins_rm.data.internal)
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
                rm = "<NONE>";
                break;
            }
            break;
        case gip_ins_rnm_type_register:
            rm = rm_buffer;
            sprintf( rm_buffer, "R%d", inst->rm_data.gip_ins_rm.data.r );
            break;
        default:
            rm = rm_buffer;
            sprintf( rm_buffer, "S/C%d", inst->rm_data.gip_ins_rm.data.r );
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
        default:
            break;
        }
        sprintf( text, "%s%s%s%s%s%s %s%s%s %s",
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
        sprintf( text, "%s%s%s%s%s %s, %s %s",
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
        sprintf( text, "%s%s%s%s %s, %s %s",
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
        sprintf( text, "ILDR%s%s%s%s%s #%d (%s%s, %s%s%s %s",
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
        sprintf( text, "ISTR%s%s%s%s #%d (%s%s%s%s <- %s %s",
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
        sprintf( text, "NO DISASSEMBLY YET" );
        break;
    }

    /*b Display instruction
     */
    printf("%s\n", text );
}

/*f c_gip_pipeline_single::execute_int_instruction
 */
void c_gip_pipeline_single::execute_int_instruction( t_gip_instruction *inst, t_gip_pipeline_results *results )
{
//    debug(6);
    disassemble_int_instruction( inst );
    results->write_pc = 0;
    results->flush = 0;

    /*b Read the register file; but don't write alu_a_in if we have a mlb instruction!
     */
    if ( 0 && (inst->gip_ins_class==gip_ins_class_arith) &&
         (inst->gip_ins_subclass==gip_ins_subclass_arith_mlb) )
    {
        pd->alu_a_in = pd->alu_a_in;
    }
    else
    {
        pd->alu_a_in = read_int_register( inst, inst->gip_ins_rn ); // Read register/special
    }
    if ( (inst->gip_ins_class==gip_ins_class_arith) &&
         (inst->gip_ins_subclass==gip_ins_subclass_arith_mlb) )
    {
        pd->alu_b_in = pd->alu_b_in<<2;// An MLB instruction in RF read stage implies shift left by 2; but only if it moves to the ALU stage, which it does here
    }
    else if (inst->rm_is_imm)
    {
        pd->alu_b_in = inst->rm_data.immediate; // If immediate, pass immediate data in
    }
    else
    {
        pd->alu_b_in = read_int_register( inst, inst->rm_data.gip_ins_rm ); // Else read register/special
    }

    /*b Read the coprocessor read file - supposed to be simultaneous with register file read
     */

    /*b Evaluate condition associated with the instruction - simultaneous with ALU stage, blocks all results from instruction if it fails
     */
    pd->condition_passed = is_condition_met( inst->gip_ins_cc, pd );

    /*b Execute the ALU stage - supposed to be cycle after register file read and coprocessor read
     */
    t_gip_alu_op gip_alu_op;
    results->flush = inst->f;
    pd->alu_rd = gip_ins_rd_none;
    pd->set_zcvn = 0;
    pd->set_p = 0;
    pd->set_acc = inst->a;
    pd->op1_src = gip_alu_op1_src_a_in;
    if ( (inst->gip_ins_rn.type==gip_ins_rnm_type_internal) &&
         (inst->gip_ins_rn.data.internal==gip_ins_rnm_int_acc) )
    {
        pd->op1_src = gip_alu_op1_src_acc;
    }
    if (inst->rm_is_imm)
    {
        pd->op2_src = gip_alu_op2_src_b_in;
    }
    else
    {
        pd->op2_src = gip_alu_op2_src_b_in;
        if ( (inst->rm_data.gip_ins_rm.type==gip_ins_rnm_type_internal) &&
             (inst->rm_data.gip_ins_rm.data.internal==gip_ins_rnm_int_acc) )
        {
            pd->op2_src = gip_alu_op2_src_acc;
        }
        if ( (inst->rm_data.gip_ins_rm.type==gip_ins_rnm_type_internal) &&
             (inst->rm_data.gip_ins_rm.data.internal==gip_ins_rnm_int_shf) )
        {
            pd->op2_src = gip_alu_op2_src_shf;
        }
    }
    switch (inst->gip_ins_class)
    {
    case gip_ins_class_arith:
        pd->alu_rd = inst->gip_ins_rd;
        pd->set_zcvn = inst->gip_ins_opts.alu.s;
        pd->set_p = inst->gip_ins_opts.alu.p;
        switch (inst->gip_ins_subclass)
        {
        case gip_ins_subclass_arith_add:
            gip_alu_op = gip_alu_op_add;
            break;
        case gip_ins_subclass_arith_adc:
            gip_alu_op = gip_alu_op_adc;
            break;
        case gip_ins_subclass_arith_sub:
            gip_alu_op = gip_alu_op_sub;
            break;
        case gip_ins_subclass_arith_sbc:
            gip_alu_op = gip_alu_op_sbc;
            break;
        case gip_ins_subclass_arith_rsb:
            gip_alu_op = gip_alu_op_rsub;
            break;
        case gip_ins_subclass_arith_rsc:
            gip_alu_op = gip_alu_op_rsbc;
            break;
        case gip_ins_subclass_arith_init:
            gip_alu_op = gip_alu_op_init;
            break;
        case gip_ins_subclass_arith_mla:
            gip_alu_op = gip_alu_op_mla;
            break;
        case gip_ins_subclass_arith_mlb:
            gip_alu_op = gip_alu_op_mlb;
            break;
        default:
            break;
        }
        break;
    case gip_ins_class_logic:
        pd->alu_rd = inst->gip_ins_rd;
        pd->set_zcvn = inst->gip_ins_opts.alu.s;
        pd->set_p = inst->gip_ins_opts.alu.p;
        switch (inst->gip_ins_subclass)
        {
        case gip_ins_subclass_logic_and:
            gip_alu_op = gip_alu_op_and;
            break;
        case gip_ins_subclass_logic_or:
            gip_alu_op = gip_alu_op_or;
            break;
        case gip_ins_subclass_logic_xor:
            gip_alu_op = gip_alu_op_xor;
            break;
        case gip_ins_subclass_logic_bic:
            gip_alu_op = gip_alu_op_bic;
            break;
        case gip_ins_subclass_logic_orn:
            gip_alu_op = gip_alu_op_orn;
            break;
        case gip_ins_subclass_logic_mov:
            gip_alu_op = gip_alu_op_mov;
            break;
        case gip_ins_subclass_logic_mvn:
            gip_alu_op = gip_alu_op_mvn;
            break;
        default:
            break;
        }
        break;
    case gip_ins_class_shift:
        pd->alu_rd = gip_ins_rd_none;
        pd->set_zcvn = inst->gip_ins_opts.alu.s;
        pd->set_p = 0;
        switch (inst->gip_ins_subclass)
        {
        case gip_ins_subclass_shift_lsl:
            gip_alu_op = gip_alu_op_lsl;
            break;
        case gip_ins_subclass_shift_lsr:
            gip_alu_op = gip_alu_op_lsr;
            break;
        case gip_ins_subclass_shift_asr:
            gip_alu_op = gip_alu_op_asr;
            break;
        case gip_ins_subclass_shift_ror:
            gip_alu_op = gip_alu_op_ror;
            break;
        case gip_ins_subclass_shift_ror33:
            gip_alu_op = gip_alu_op_ror33;
            break;
        default:
            break;
        }
        break;
    case gip_ins_class_coproc:
        pd->alu_rd = gip_ins_rd_none;
        gip_alu_op = gip_alu_op_mov;
        break;
    case gip_ins_class_load:
        pd->alu_rd = gip_ins_rd_none;
        if ((inst->gip_ins_subclass & gip_ins_subclass_memory_dirn)==gip_ins_subclass_memory_up)
        {
            gip_alu_op = gip_alu_op_add;
        }
        else
        {
            gip_alu_op = gip_alu_op_sub;
        }
        break;
    case gip_ins_class_store:
        pd->alu_rd = inst->gip_ins_rd;
        if ((inst->gip_ins_subclass & gip_ins_subclass_memory_dirn)==gip_ins_subclass_memory_up)
        {
            gip_alu_op = gip_alu_op_add;
        }
        else
        {
            gip_alu_op = gip_alu_op_sub;
        }
        switch (inst->gip_ins_subclass & gip_ins_subclass_memory_size)
        {
        case gip_ins_subclass_memory_word:
            pd->alu_constant = 4;
            break;
        case gip_ins_subclass_memory_half:
            pd->alu_constant = 2;
            break;
        case gip_ins_subclass_memory_byte:
            pd->alu_constant = 1;
            break;
        default:
            pd->alu_constant = 0;
            break;
        }
        if (inst->gip_ins_opts.store.offset_type==1)
        {
            pd->op2_src = gip_alu_op2_src_shf;
        }
        else
        {
            pd->op2_src = gip_alu_op2_src_constant;
        }
        break;
    }
    if (!pd->condition_passed)
    {
        pd->set_acc = 0;
        pd->set_zcvn = 0;
        pd->set_p = 0;
        pd->alu_rd = gip_ins_rd_none;
        results->flush = 0;
    }
    execute_int_alu_instruction( gip_alu_op, pd->op1_src, pd->op2_src, pd->set_acc, pd->set_zcvn, pd->set_p );
    pd->old_cp = pd->cp;
    pd->cp = pd->condition_passed;

    /*b Execute memory read/write - occurs after ALU operation
     */
    t_gip_mem_op gip_mem_op;
    gip_mem_op = gip_mem_op_none;
    pd->mem_rd = gip_ins_rd_none;
    switch (inst->gip_ins_class)
    {
    case gip_ins_class_arith:
    case gip_ins_class_logic:
    case gip_ins_class_shift:
        break;
    case gip_ins_class_coproc:
        break;
    case gip_ins_class_load:
        pd->mem_rd = inst->gip_ins_rd;
        switch (inst->gip_ins_subclass & gip_ins_subclass_memory_size)
        {
        case gip_ins_subclass_memory_word:
            gip_mem_op = gip_mem_op_load_word;
            break;
        case gip_ins_subclass_memory_half:
            gip_mem_op = gip_mem_op_load_half;
            break;
        case gip_ins_subclass_memory_byte:
            gip_mem_op = gip_mem_op_load_byte;
            break;
        default:
            gip_mem_op = gip_mem_op_none;
            break;
        }
        break;
    case gip_ins_class_store:
        pd->mem_rd = gip_ins_rd_none;
        switch (inst->gip_ins_subclass & gip_ins_subclass_memory_size)
        {
        case gip_ins_subclass_memory_word:
            gip_mem_op = gip_mem_op_store_word;
            break;
        case gip_ins_subclass_memory_half:
            gip_mem_op = gip_mem_op_store_half;
            break;
        case gip_ins_subclass_memory_byte:
            gip_mem_op = gip_mem_op_store_byte;
            break;
        default:
            gip_mem_op = gip_mem_op_none;
            break;
        }
        break;
    }
    if (!pd->condition_passed)
    {
        pd->mem_rd = gip_ins_rd_none;
        gip_mem_op = gip_mem_op_none;
    }
    execute_int_memory_instruction( gip_mem_op,
                                    ((inst->gip_ins_subclass & gip_ins_subclass_memory_index)==gip_ins_subclass_memory_preindex)?pd->alu_result:pd->alu_op1,
                                    pd->alu_b_in );

    /*b Execute coprocessor write - occurs after memory read/write
     */

    /*b Register file writeback
     */
    results->rfw_data = pd->alu_result;
    if (pd->alu_rd.type==gip_ins_rd_type_register)
    {
        pd->regs[ pd->alu_rd.data.r&0x1f ] = pd->alu_result;
    }
    if ( (pd->alu_rd.type==gip_ins_rd_type_internal) &&
         (pd->alu_rd.data.internal==gip_ins_rd_int_pc) )
    {
        results->write_pc = 1;
    }
    if (pd->mem_rd.type==gip_ins_rd_type_register)
    {
        results->rfw_data = pd->mem_result;
        pd->regs[ pd->mem_rd.data.r&0x1f ] = pd->mem_result;
    }
    if ( (pd->mem_rd.type==gip_ins_rd_type_internal) &&
         (pd->mem_rd.data.internal==gip_ins_rd_int_pc) )
    {
        results->rfw_data = pd->mem_result;
        results->write_pc = 1;
    }
    //printf("RFW ALU %d MEM %d D %08x\n",pd->alu_rd, pd->mem_rd, results->rfw_data );

    /*b Done
     */
}

/*a Internal instruction building
 */
/*f c_gip_pipeline_single::build_gip_instruction_alu
 */
void c_gip_pipeline_single::build_gip_instruction_alu( t_gip_instruction *gip_instr, t_gip_ins_class gip_ins_class, t_gip_ins_subclass gip_ins_subclass, int a, int s, int p, int f )
{
    gip_instr->gip_ins_class = gip_ins_class;
    gip_instr->gip_ins_subclass = gip_ins_subclass;
    gip_instr->gip_ins_opts.alu.s = s;
    gip_instr->gip_ins_opts.alu.p = p;
    gip_instr->gip_ins_cc = gip_ins_cc_always;
    gip_instr->a = a;
    gip_instr->f = f;
    gip_instr->gip_ins_rn = gip_ins_rnm_acc;
    gip_instr->rm_is_imm = 1;
    gip_instr->rm_data.immediate = 0;
    gip_instr->gip_ins_rd = gip_ins_rd_none;
    gip_instr->pc = pd->pc+8;
}

/*f c_gip_pipeline_single::build_gip_instruction_shift
 */
void c_gip_pipeline_single::build_gip_instruction_shift( t_gip_instruction *gip_instr, t_gip_ins_subclass gip_ins_subclass, int s, int f )
{
    gip_instr->gip_ins_class = gip_ins_class_shift;
    gip_instr->gip_ins_subclass = gip_ins_subclass;
    gip_instr->gip_ins_opts.alu.s = s;
    gip_instr->gip_ins_opts.alu.p = 0;
    gip_instr->gip_ins_cc = gip_ins_cc_always;
    gip_instr->a = 0;
    gip_instr->f = f;
    gip_instr->gip_ins_rn = gip_ins_rnm_acc;
    gip_instr->rm_is_imm = 1;
    gip_instr->rm_data.immediate = 0;
    gip_instr->gip_ins_rd = gip_ins_rd_none;
    gip_instr->pc = pd->pc+8;
}

/*f c_gip_pipeline_single::build_gip_instruction_load
 */
void c_gip_pipeline_single::build_gip_instruction_load( t_gip_instruction *gip_instr, t_gip_ins_subclass gip_ins_subclass, int preindex, int up, int stack, int burst_left, int a, int f )
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
    gip_instr->k = burst_left;
    gip_instr->gip_ins_rn = gip_ins_rnm_acc;
    gip_instr->rm_is_imm = 1;
    gip_instr->rm_data.immediate = 0;
    gip_instr->gip_ins_rd = gip_ins_rd_none;
    gip_instr->pc = pd->pc+8;
}

/*f c_gip_pipeline_single::build_gip_instruction_store
 */
void c_gip_pipeline_single::build_gip_instruction_store( t_gip_instruction *gip_instr, t_gip_ins_subclass gip_ins_subclass, int preindex, int up, int use_shift, int stack, int burst_left, int a, int f )
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
    gip_instr->k = burst_left;
    gip_instr->gip_ins_rn = gip_ins_rnm_acc;
    gip_instr->rm_is_imm = 1;
    gip_instr->rm_data.immediate = 0;
    gip_instr->gip_ins_rd = gip_ins_rd_none;
    gip_instr->pc = pd->pc+8;
}

/*f c_gip_pipeline_single::build_gip_instruction_immediate
 */
void c_gip_pipeline_single::build_gip_instruction_immediate( t_gip_instruction *gip_instr, unsigned int imm_val )
{
    gip_instr->rm_is_imm = 1;
    gip_instr->rm_data.immediate = imm_val;
}

/*f c_gip_pipeline_single::build_gip_instruction_cc
 */
void c_gip_pipeline_single::build_gip_instruction_cc( t_gip_instruction *gip_instr, t_gip_ins_cc gip_ins_cc)
{
    gip_instr->gip_ins_cc = gip_ins_cc;
}

/*f c_gip_pipeline_single::build_gip_instruction_rn
 */
void c_gip_pipeline_single::build_gip_instruction_rn( t_gip_instruction *gip_instr, t_gip_ins_rnm gip_ins_rn )
{
    gip_instr->gip_ins_rn = gip_ins_rn;
}

/*f c_gip_pipeline_single::build_gip_instruction_rm
 */
void c_gip_pipeline_single::build_gip_instruction_rm( t_gip_instruction *gip_instr, t_gip_ins_rnm gip_ins_rm )
{
    gip_instr->rm_is_imm = 0;
    gip_instr->rm_data.gip_ins_rm = gip_ins_rm;
}

/*f c_gip_pipeline_single::build_gip_instruction_rd
 */
void c_gip_pipeline_single::build_gip_instruction_rd( t_gip_instruction *gip_instr, t_gip_ins_rd gip_ins_rd )
{
    gip_instr->gip_ins_rd = gip_ins_rd;
}

/*f c_gip_pipeline_single::map_condition_code
 */
t_gip_ins_cc c_gip_pipeline_single::map_condition_code( int arm_cc )
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

/*f c_gip_pipeline_single::map_source_register
 */
t_gip_ins_rnm c_gip_pipeline_single::map_source_register( int arm_r )
{
    t_gip_ins_rnm result;
    result.type = gip_ins_rnm_type_register;
    result.data.r = arm_r;
    if (arm_r==15)
    {
        result.type = gip_ins_rnm_type_internal;
        result.data.internal = gip_ins_rnm_int_pc;
    }
    return result;
}

/*f c_gip_pipeline_single::map_destination_register
 */
t_gip_ins_rd c_gip_pipeline_single::map_destination_register( int arm_rd )
{
    t_gip_ins_rd result;
    result.type = gip_ins_rd_type_register;
    result.data.r = arm_rd;
    if (arm_rd==15)
    {
        result.type = gip_ins_rd_type_internal;
        result.data.internal = gip_ins_rd_int_pc;
    }
    return result;
}

/*f c_gip_pipeline_single::map_shift
 */
t_gip_ins_subclass c_gip_pipeline_single::map_shift( int shf_how, int imm, int amount )
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


/*a ARM Execution functions
 */
/*f c_gip_pipeline_single::execute_arm_alu
  ALU instructions map to one or more internal instructions
    Those that do not set the PC and are immediate or have a shift of LSL #0 map to one internal ALU instruction
    Those that do not set the PC and have a shift other than LSL #0 map to one internal SHIFT instruction and one internal ALU instruction
    Those that set the PC and have a shift of LSL #0 map to one internal ALU instruction with 'flush' set
    Those that set the PC and have a shift other than LSL #0 map to one internal SHIFT instruction and one internal ALU instruction with 'flush' set
 */
int c_gip_pipeline_single::execute_arm_alu( unsigned int opcode )
{
    int cc, zeros, imm, alu_op, sign, rn, rd, op2;
    int imm_val, imm_rotate;
    int shf_rs, shf_reg_zero, shf_imm_amt, shf_how, shf_by_reg, shf_rm;
    int conditional;
    t_gip_instruction gip_instr;
    t_gip_ins_class gip_ins_class;
    t_gip_ins_subclass gip_ins_subclass;
    t_gip_ins_cc gip_ins_cc;
    t_gip_ins_rnm gip_ins_rn;
    t_gip_ins_rnm gip_ins_rm;
    t_gip_ins_rnm gip_ins_rs;
    t_gip_ins_rd gip_ins_rd;
    t_gip_pipeline_results gip_results;
    int gip_set_flags, gip_set_acc, gip_pass_p;

    /*b Decode ARM instruction
     */
    cc      = (opcode>>28) & 0x0f;
    zeros   = (opcode>>26) & 0x03;
    imm     = (opcode>>25) & 0x01;
    alu_op  = (opcode>>21) & 0x0f;
    sign    = (opcode>>20) & 0x01;
    rn      = (opcode>>16) & 0x0f;
    rd      = (opcode>>12) & 0x0f;
    op2     = (opcode>> 0) & 0xfff;

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
        gip_ins_rd = gip_ins_rd_none;
        break;
    case  9: // teq
        gip_ins_class = gip_ins_class_logic;
        gip_ins_subclass = gip_ins_subclass_logic_xor;
        gip_set_flags = 1;
        gip_pass_p = sign;
        gip_set_acc = 0;
        gip_ins_rd = gip_ins_rd_none;
        break;
    case 10: // cmp
        gip_ins_class = gip_ins_class_arith;
        gip_ins_subclass = gip_ins_subclass_arith_sub;
        gip_set_flags = 1;
        gip_set_acc = 0;
        gip_ins_rd = gip_ins_rd_none;
        break;
    case 11: // cmn
        gip_ins_class = gip_ins_class_arith;
        gip_ins_subclass = gip_ins_subclass_arith_add;
        gip_set_flags = 1;
        gip_set_acc = 0;
        gip_ins_rd = gip_ins_rd_none;
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

    /*b Test for shift of 'LSL #0' or plain immediate
     */
    if (imm)
    {
        gip_pass_p = 0;
        build_gip_instruction_alu( &gip_instr, gip_ins_class, gip_ins_subclass, gip_set_acc, gip_set_flags, gip_pass_p, (rd==15) );
        build_gip_instruction_cc( &gip_instr, gip_ins_cc );
        build_gip_instruction_rn( &gip_instr, gip_ins_rn );
        build_gip_instruction_immediate( &gip_instr, rotate_right(imm_val,imm_rotate*2) );
        build_gip_instruction_rd( &gip_instr, gip_ins_rd );
        execute_int_instruction( &gip_instr, &gip_results );
        pd->pc += 4;
        if (gip_results.write_pc)
        {
            pd->pc = gip_results.rfw_data;
        }
    }
    else if ( (((t_shf_type)shf_how)==shf_type_lsl) &&
              (!shf_by_reg) &&
              (shf_imm_amt==0) )
    {
        gip_pass_p = 0;
        build_gip_instruction_alu( &gip_instr, gip_ins_class, gip_ins_subclass, gip_set_acc, gip_set_flags, gip_pass_p, (rd==15) );
        build_gip_instruction_cc( &gip_instr, gip_ins_cc );
        build_gip_instruction_rn( &gip_instr, gip_ins_rn );
        build_gip_instruction_rm( &gip_instr, gip_ins_rm );
        build_gip_instruction_rd( &gip_instr, gip_ins_rd );
        execute_int_instruction( &gip_instr, &gip_results );
    	pd->pc += 4;
        if (gip_results.write_pc)
        {
            pd->pc = gip_results.rfw_data;
        }
    }
    else if (!shf_by_reg) // Immediate shift, non-zero or non-LSL: ISHF Rm, #imm; IALU{CC}(SP|)[A][F] Rn, SHF -> Rd
    {
        build_gip_instruction_shift( &gip_instr, map_shift( shf_how, 1, shf_imm_amt), 0, 0 );
        build_gip_instruction_rn( &gip_instr, gip_ins_rm );
        build_gip_instruction_immediate( &gip_instr, (shf_imm_amt==0)?32:shf_imm_amt );
        build_gip_instruction_rd( &gip_instr, gip_ins_rd_none );
        execute_int_instruction( &gip_instr, &gip_results );
        if (gip_results.write_pc)
        {
            pd->pc = gip_results.rfw_data;
        }
        if (gip_results.flush)
        {
            return 1;
        }

        build_gip_instruction_alu( &gip_instr, gip_ins_class, gip_ins_subclass, gip_set_acc, gip_set_flags, gip_pass_p, (rd==15) );
        build_gip_instruction_cc( &gip_instr, gip_ins_cc );
        build_gip_instruction_rn( &gip_instr, gip_ins_rn );
        build_gip_instruction_rm( &gip_instr, gip_ins_rnm_shf );
        build_gip_instruction_rd( &gip_instr, gip_ins_rd );
        execute_int_instruction( &gip_instr, &gip_results );
    	pd->pc += 4;
        if (gip_results.write_pc)
        {
            pd->pc = gip_results.rfw_data;
        }
    }
    else // if (shf_by_reg) - must be! ISHF Rm, Rs; IALU{CC}(SP|)[A][F] Rn, SHF -> Rd
    {
        build_gip_instruction_shift( &gip_instr, map_shift(shf_how, 0, 0 ), 0, 0 );
        build_gip_instruction_rn( &gip_instr, gip_ins_rm );
        build_gip_instruction_rm( &gip_instr, gip_ins_rs );
        build_gip_instruction_rd( &gip_instr, gip_ins_rd_none );
        execute_int_instruction( &gip_instr, &gip_results );
        if (gip_results.write_pc)
        {
            pd->pc = gip_results.rfw_data;
        }
        if (gip_results.flush)
        {
            return 1;
        }

        build_gip_instruction_alu( &gip_instr, gip_ins_class, gip_ins_subclass, gip_set_acc, gip_set_flags, gip_pass_p, (rd==15) );
        build_gip_instruction_cc( &gip_instr, gip_ins_cc );
        build_gip_instruction_rn( &gip_instr, gip_ins_rn );
        build_gip_instruction_rm( &gip_instr, gip_ins_rnm_shf );
        build_gip_instruction_rd( &gip_instr, gip_ins_rd );
        execute_int_instruction( &gip_instr, &gip_results );
    	pd->pc += 4;
        if (gip_results.write_pc)
        {
            pd->pc = gip_results.rfw_data;
        }
    }

    return 1;
}

/*f c_gip_pipeline_single::execute_arm_branch
 */
int c_gip_pipeline_single::execute_arm_branch( unsigned int opcode )
{
    int cc, five, link, offset;
    t_gip_instruction gip_instr;
    t_gip_pipeline_results gip_results;

    /*b Decode ARM instruction
     */
    cc      = (opcode>>28) & 0x0f;
    five    = (opcode>>25) & 0x07;
    link    = (opcode>>24) & 0x01;
    offset  = (opcode>> 0) & 0x00ffffff;

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
            pd->pc += 8+((offset<<8)>>6);
        }
        else
        {
            if (offset&0x800000) // backward conditional branch; sub(!cc)f pc, #4 -> pc
            {
                build_gip_instruction_alu( &gip_instr, gip_ins_class_arith, gip_ins_subclass_arith_sub, 0, 0, 0, 1 );
                build_gip_instruction_cc( &gip_instr, map_condition_code( cc^0x1 ) ); // !cc is CC with bottom bit inverted, in ARM
                build_gip_instruction_rn( &gip_instr, gip_ins_rnm_pc );
                build_gip_instruction_immediate( &gip_instr, 4 );
                build_gip_instruction_rd( &gip_instr, gip_ins_rd_pc );
                execute_int_instruction( &gip_instr, &gip_results );
                pd->pc += 8+((offset<<8)>>6);
                if (gip_results.write_pc)
                {
                    pd->pc = gip_results.rfw_data;
                }
            }
            else // forward conditional branch; mov[cc]f #target -> pc
            {
                build_gip_instruction_alu( &gip_instr, gip_ins_class_logic, gip_ins_subclass_logic_mov, 0, 0, 0, 1 );
                build_gip_instruction_cc( &gip_instr, map_condition_code( cc ) );
                build_gip_instruction_immediate( &gip_instr, pd->pc+8+((offset<<8)>>6) );
                build_gip_instruction_rd( &gip_instr, gip_ins_rd_pc );
                execute_int_instruction( &gip_instr, &gip_results );
                pd->pc += 4;
                if (gip_results.write_pc)
                {
                    pd->pc = gip_results.rfw_data;
                }
            }
        }
    }
    else
    {
        if (cc==14) // guaranteed branch with link; sub pc, #4 -> r14
        {
            build_gip_instruction_alu( &gip_instr, gip_ins_class_arith, gip_ins_subclass_arith_sub, 0, 0, 0, 0 );
            build_gip_instruction_cc( &gip_instr, gip_ins_cc_always );
            build_gip_instruction_rn( &gip_instr, gip_ins_rnm_pc );
            build_gip_instruction_immediate( &gip_instr, 4 );
            build_gip_instruction_rd( &gip_instr, gip_ins_rd_r14 );
            execute_int_instruction( &gip_instr, &gip_results );
            pd->pc += 8+((offset<<8)>>6);
        }
        else // conditional branch with link; sub{!cc}f pc, #4 -> pc; sub pc, #4 -> r14
        {
            build_gip_instruction_alu( &gip_instr, gip_ins_class_arith, gip_ins_subclass_arith_sub, 0, 0, 0, 1 );
            build_gip_instruction_cc( &gip_instr, map_condition_code( cc^0x1 ) ); // !cc is CC with bottom bit inverted, in ARM
            build_gip_instruction_rn( &gip_instr, gip_ins_rnm_pc );
            build_gip_instruction_immediate( &gip_instr, 4 );
            build_gip_instruction_rd( &gip_instr, gip_ins_rd_pc );
            execute_int_instruction( &gip_instr, &gip_results );
            if (gip_results.write_pc)
            {
                pd->pc = gip_results.rfw_data;
            }
            if (gip_results.flush)
            {
                return 1;
            }
            build_gip_instruction_alu( &gip_instr, gip_ins_class_arith, gip_ins_subclass_arith_sub, 0, 0, 0, 0 );
            build_gip_instruction_cc( &gip_instr, gip_ins_cc_always );
            build_gip_instruction_rn( &gip_instr, gip_ins_rnm_pc );
            build_gip_instruction_immediate( &gip_instr, 4 );
            build_gip_instruction_rd( &gip_instr, gip_ins_rd_r14 );
            execute_int_instruction( &gip_instr, &gip_results );
            pd->pc += 8+((offset<<8)>>6);
            if (gip_results.write_pc)
            {
                pd->pc = gip_results.rfw_data;
            }
        }
    }
    return 1;
}

/*f c_gip_pipeline_single::execute_arm_ld_st
 */
int c_gip_pipeline_single::execute_arm_ld_st( unsigned int opcode )
{
    int cc, one, not_imm, pre, up, byte, wb, load, rn, rd, offset;
    int imm_val;
    int shf_imm_amt, shf_how, shf_zero, shf_rm;
    t_gip_instruction gip_instr;
    t_gip_pipeline_results gip_results;

    /*b Decode ARM instruction
     */
    cc      = (opcode>>28) & 0x0f;
    one     = (opcode>>26) & 0x03;
    not_imm = (opcode>>25) & 0x01;
    pre     = (opcode>>24) & 0x01;
    up      = (opcode>>23) & 0x01;
    byte    = (opcode>>22) & 0x01;
    wb      = (opcode>>21) & 0x01;
    load    = (opcode>>20) & 0x01;
    rn      = (opcode>>16) & 0x0f;
    rd      = (opcode>>12) & 0x0f;
    offset  = (opcode>> 0) & 0xfff;

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
                build_gip_instruction_load( &gip_instr, gip_ins_subclass_memory_word, 1, up, (rn==13), 0, 1, (rd==15) );
                build_gip_instruction_cc( &gip_instr, map_condition_code( cc ) );
                build_gip_instruction_rn( &gip_instr, map_source_register(rn) );
                if (not_imm)
                {
                    build_gip_instruction_rm( &gip_instr, map_source_register(shf_rm) );
                }
                else
                {
                    build_gip_instruction_immediate( &gip_instr, imm_val );
                }
                build_gip_instruction_rd( &gip_instr, map_destination_register(rd) );
                execute_int_instruction( &gip_instr, &gip_results );
                pd->pc += 4;
                if (gip_results.write_pc)
                {
                    pd->pc = gip_results.rfw_data;
                }
                if (gip_results.flush)
                {
                    return 1;
                }
                return 1;
            }
            /*b Preindexed, writeback
             */
            else if (pre) // preindexed immediate/reg with writeback: IADD[CC]A/ISUB[CC]A Rn, #imm/Rm -> Rn; ILDRCP[F] #0, (Acc) -> Rd
            {
                build_gip_instruction_alu( &gip_instr, gip_ins_class_arith, up?gip_ins_subclass_arith_add:gip_ins_subclass_arith_sub, 1, 0, 0, 0 );
                build_gip_instruction_cc( &gip_instr, map_condition_code( cc ) );
                build_gip_instruction_rn( &gip_instr, map_source_register(rn) );
                if (not_imm)
                {
                    build_gip_instruction_rm( &gip_instr, map_source_register(shf_rm) );
                }
                else
                {
                    build_gip_instruction_immediate( &gip_instr, imm_val );
                }
                build_gip_instruction_rd( &gip_instr, map_destination_register(rn) );
                execute_int_instruction( &gip_instr, &gip_results );
                build_gip_instruction_load( &gip_instr, gip_ins_subclass_memory_word, 0, up, (rn==13), 0, 0, 0 );
                build_gip_instruction_cc( &gip_instr, gip_ins_cc_cp );
                build_gip_instruction_rn( &gip_instr, gip_ins_rnm_acc );
                build_gip_instruction_rd( &gip_instr, map_destination_register( rd ) );
                execute_int_instruction( &gip_instr, &gip_results );
                pd->pc += 4;
                if (gip_results.write_pc)
                {
                    pd->pc = gip_results.rfw_data;
                }
                return 1;
            }
            /*b Postindexed
             */
            else // postindexed immediate/reg: ILDR[CC]A #0, (Rn), +/-Rm/Imm -> Rd; MOVCP[F] Acc -> Rn
            {
                build_gip_instruction_load( &gip_instr,gip_ins_subclass_memory_word, 0, up, (rn==13), 0, 1, 0 );
                build_gip_instruction_cc( &gip_instr, map_condition_code( cc ) );
                build_gip_instruction_rn( &gip_instr, map_source_register(rn) );
                if (not_imm)
                {
                    build_gip_instruction_rm( &gip_instr, map_source_register(shf_rm) );
                }
                else
                {
                    build_gip_instruction_immediate( &gip_instr, imm_val );
                }
                build_gip_instruction_rd( &gip_instr, map_destination_register(rd) );
                execute_int_instruction( &gip_instr, &gip_results );
                if (gip_results.write_pc)
                {
                    pd->pc = gip_results.rfw_data;
                }

                build_gip_instruction_alu( &gip_instr, gip_ins_class_logic, gip_ins_subclass_logic_mov, 0, 0, 0, (rd==15) );
                build_gip_instruction_cc( &gip_instr, gip_ins_cc_cp );
                build_gip_instruction_rm( &gip_instr, gip_ins_rnm_acc );
                build_gip_instruction_rd( &gip_instr, map_destination_register(rn) );
                execute_int_instruction( &gip_instr, &gip_results );
                if (gip_results.flush)
                {
                    return 1;
                }
                pd->pc += 4;
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
                build_gip_instruction_shift( &gip_instr, map_shift(shf_how, 1, shf_imm_amt ), 0, 0 );
                build_gip_instruction_rn( &gip_instr, map_source_register(shf_rm) );
                build_gip_instruction_immediate( &gip_instr, shf_imm_amt );
                build_gip_instruction_rd( &gip_instr, gip_ins_rd_none );
                execute_int_instruction( &gip_instr, &gip_results );
                build_gip_instruction_load( &gip_instr, gip_ins_subclass_memory_word, 1, up, (rn==13), 0, 1, (rd==15) );
                build_gip_instruction_cc( &gip_instr, map_condition_code( cc ) );
                build_gip_instruction_rn( &gip_instr, map_source_register(rn) );
                build_gip_instruction_rm( &gip_instr, gip_ins_rnm_shf );
                build_gip_instruction_rd( &gip_instr, map_destination_register(rd) );
                execute_int_instruction( &gip_instr, &gip_results );
                pd->pc += 4;
                if (gip_results.write_pc)
                {
                    pd->pc = gip_results.rfw_data;
                }
                return 1;
            }
            /*b Preindexed with writeback
             */
            else if (pre) // preindexed reg with shift with writeback: ILSL Rm, #imm; IADD[CC]A/ISUB[CC]A Rn, SHF -> Rn; ILDRCP[F] #0 (Acc) -> Rd
            {
                build_gip_instruction_shift( &gip_instr, map_shift(shf_how, 1, shf_imm_amt ), 0, 0 );
                build_gip_instruction_rn( &gip_instr, map_source_register(shf_rm) );
                build_gip_instruction_immediate( &gip_instr, shf_imm_amt );
                build_gip_instruction_rd( &gip_instr, gip_ins_rd_none );
                execute_int_instruction( &gip_instr, &gip_results );

                build_gip_instruction_alu( &gip_instr, gip_ins_class_arith, up?gip_ins_subclass_arith_add:gip_ins_subclass_arith_sub, 1, 0, 0, 0 );
                build_gip_instruction_cc( &gip_instr, map_condition_code( cc ) );
                build_gip_instruction_rn( &gip_instr, map_source_register(rn) );
                build_gip_instruction_rm( &gip_instr, gip_ins_rnm_shf );
                build_gip_instruction_rd( &gip_instr, map_destination_register(rn) );
                execute_int_instruction( &gip_instr, &gip_results );

                build_gip_instruction_load( &gip_instr, gip_ins_subclass_memory_word, 0, 1, (rn==13), 0, 0, (rd==15) );
                build_gip_instruction_cc( &gip_instr, gip_ins_cc_cp );
                build_gip_instruction_rn( &gip_instr, gip_ins_rnm_acc );
                build_gip_instruction_rd( &gip_instr, map_destination_register(rd) );
                execute_int_instruction( &gip_instr, &gip_results );
                pd->pc += 4;
                if (gip_results.write_pc)
                {
                    pd->pc = gip_results.rfw_data;
                }
                return 1;
            }
            /*b Postindexed
             */
            else // postindexed reg with shift with writeback: ILSL Rm, #imm; ILDR[CC]A #0 (Rn), +/-SHF -> Rd; MOVCP[F] Acc -> Rn
            {
                build_gip_instruction_shift( &gip_instr, map_shift(shf_how, 1, shf_imm_amt ), 0, 0 );
                build_gip_instruction_rn( &gip_instr, map_source_register(shf_rm) );
                build_gip_instruction_immediate( &gip_instr, shf_imm_amt );
                build_gip_instruction_rd( &gip_instr, gip_ins_rd_none );
                execute_int_instruction( &gip_instr, &gip_results );

                build_gip_instruction_load( &gip_instr, gip_ins_subclass_memory_word, 0, up, (rn==13), 0, 1, 0 );
                build_gip_instruction_cc( &gip_instr,map_condition_code(cc) );
                build_gip_instruction_rn( &gip_instr, map_source_register(rn) );
                build_gip_instruction_rm( &gip_instr, gip_ins_rnm_shf );
                build_gip_instruction_rd( &gip_instr, map_destination_register(rd) );
                execute_int_instruction( &gip_instr, &gip_results );
                if (gip_results.write_pc)
                {
                    pd->pc = gip_results.rfw_data;
                }

                build_gip_instruction_alu( &gip_instr, gip_ins_class_logic, gip_ins_subclass_logic_mov, 0, 0, 0, (rd==15) );
                build_gip_instruction_cc( &gip_instr, gip_ins_cc_cp );
                build_gip_instruction_rm( &gip_instr, gip_ins_rnm_acc );
                build_gip_instruction_rd( &gip_instr, map_destination_register(rn) );
                execute_int_instruction( &gip_instr, &gip_results );
                if (gip_results.flush)
                {
                    return 1;
                }
                pd->pc += 4;
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
        build_gip_instruction_store( &gip_instr, gip_ins_subclass_memory_word, 0, 1, 0, (rn==13), 0, 0, 0 );
        build_gip_instruction_cc( &gip_instr, map_condition_code( cc ) );
        build_gip_instruction_rn( &gip_instr, map_source_register(rn) );
        build_gip_instruction_rm( &gip_instr, map_source_register(rd) );
        build_gip_instruction_rd( &gip_instr, gip_ins_rd_none );
        execute_int_instruction( &gip_instr, &gip_results );
        pd->pc += 4;
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
            build_gip_instruction_alu( &gip_instr, gip_ins_class_arith, up?gip_ins_subclass_arith_add:gip_ins_subclass_arith_sub, 1, 0, 0, 0 );
            build_gip_instruction_cc( &gip_instr, map_condition_code(cc) );
            build_gip_instruction_rn( &gip_instr, map_source_register(rn) );
            if (!not_imm)
            {
                build_gip_instruction_immediate( &gip_instr, imm_val );
            }
            else
            {
                build_gip_instruction_rm( &gip_instr, map_source_register(shf_rm) );
            }
            build_gip_instruction_rd( &gip_instr, gip_ins_rd_none );
            execute_int_instruction( &gip_instr, &gip_results );

            build_gip_instruction_store( &gip_instr, gip_ins_subclass_memory_word, 1, up, 1, (rn==13), 0, 1, 0 );
            build_gip_instruction_cc( &gip_instr, gip_ins_cc_cp );
            build_gip_instruction_rn( &gip_instr, gip_ins_rnm_acc );
            build_gip_instruction_rm( &gip_instr, map_source_register(rd) );
            build_gip_instruction_rd( &gip_instr, gip_ins_rd_none );
            if (wb)
            {
                build_gip_instruction_rd( &gip_instr, map_destination_register(rn) );
            }
            execute_int_instruction( &gip_instr, &gip_results );
            pd->pc += 4;
            return 1;
        }
        /*b Preindexed Rm with shift: ISHF[CC] Rm, #imm; ISTRCPA[S] #0, (Rn, +/-SHF) <- Rd [-> Rn]
         */
        else
        {
            build_gip_instruction_shift( &gip_instr, map_shift(shf_how, 1, shf_imm_amt ), 0, 0 );
            build_gip_instruction_cc( &gip_instr, map_condition_code( cc ) );
            build_gip_instruction_rn( &gip_instr, map_source_register(shf_rm) );
            build_gip_instruction_immediate( &gip_instr, shf_imm_amt );
            build_gip_instruction_rd( &gip_instr, gip_ins_rd_none );
            execute_int_instruction( &gip_instr, &gip_results );

            build_gip_instruction_store( &gip_instr, gip_ins_subclass_memory_word, 1, up, 1, (rn==13), 0, 1, 0 );
            build_gip_instruction_cc( &gip_instr, gip_ins_cc_cp );
            build_gip_instruction_rn( &gip_instr, map_source_register(rn) );
            build_gip_instruction_rm( &gip_instr, map_source_register(rd) );
            build_gip_instruction_rd( &gip_instr, gip_ins_rd_none );
            if (wb)
            {
                build_gip_instruction_rd( &gip_instr, map_destination_register(rn) );
            }
            execute_int_instruction( &gip_instr, &gip_results );
            pd->pc += 4;
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
            build_gip_instruction_store( &gip_instr, gip_ins_subclass_memory_word, 0, 0, 0, (rn==13), 0, 0, 0 );
            build_gip_instruction_cc( &gip_instr, map_condition_code(cc) );
            build_gip_instruction_rn( &gip_instr, map_source_register(rn) );
            build_gip_instruction_rm( &gip_instr, map_source_register(rd) );
            build_gip_instruction_rd( &gip_instr, gip_ins_rd_none );
            execute_int_instruction( &gip_instr, &gip_results );

            build_gip_instruction_alu( &gip_instr, gip_ins_class_arith, up?gip_ins_subclass_arith_add:gip_ins_subclass_arith_sub, 1, 0, 0, 0 );
            build_gip_instruction_cc( &gip_instr, gip_ins_cc_cp );
            build_gip_instruction_rn( &gip_instr, map_source_register(rn) );
            if (!not_imm)
            {
                build_gip_instruction_immediate( &gip_instr, imm_val );
            }
            else
            {
                build_gip_instruction_rm( &gip_instr, map_source_register(shf_rm) );
            }
            build_gip_instruction_rd( &gip_instr, map_destination_register(rn) );
            execute_int_instruction( &gip_instr, &gip_results );
            pd->pc += 4;
            return 1;
        }
        /*b Postindexed reg with shift
         */
        else //  ISHF[CC] Rm, #imm; ISTRCPA[S] #0 (Rn), +/-SHF) <-Rd -> Rn
        {
            build_gip_instruction_shift( &gip_instr, map_shift(shf_how, 1, shf_imm_amt ), 0, 0 );
            build_gip_instruction_cc( &gip_instr, map_condition_code( cc ) );
            build_gip_instruction_rn( &gip_instr, map_source_register(shf_rm) );
            build_gip_instruction_immediate( &gip_instr, shf_imm_amt );
            build_gip_instruction_rd( &gip_instr, gip_ins_rd_none );
            execute_int_instruction( &gip_instr, &gip_results );

            build_gip_instruction_store( &gip_instr, gip_ins_subclass_memory_word, 0, up, 1, (rn==13), 0, 1, 0 );
            build_gip_instruction_cc( &gip_instr, gip_ins_cc_cp );
            build_gip_instruction_rn( &gip_instr, map_source_register(rn) );
            build_gip_instruction_rm( &gip_instr, map_source_register(rd) );
            build_gip_instruction_rd( &gip_instr, map_destination_register(rn) );
            execute_int_instruction( &gip_instr, &gip_results );
            pd->pc += 4;
            return 1;
        }
    }
    

    /*b Done
     */
    return 0;
}

/*f c_gip_pipeline_single::execute_arm_ldm_stm
 */
int c_gip_pipeline_single::execute_arm_ldm_stm( unsigned int opcode )
{
    int cc, four, pre, up, psr, wb, load, rn, regs;
    int i, j, first, num_regs;
    t_gip_instruction gip_instr;
    t_gip_pipeline_results gip_results;

    /*b Decode ARM instruction
     */
    cc      = (opcode>>28) & 0x0f;
    four    = (opcode>>25) & 0x07;
    pre     = (opcode>>24) & 0x01;
    up      = (opcode>>23) & 0x01;
    psr     = (opcode>>22) & 0x01;
    wb      = (opcode>>21) & 0x01;
    load    = (opcode>>20) & 0x01;
    rn      = (opcode>>16) & 0x0f;
    regs    = (opcode>> 0) & 0xffff;

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
        first=1;
        if (!up)
        {
            build_gip_instruction_alu( &gip_instr, gip_ins_class_arith, gip_ins_subclass_arith_sub, 1, 0, 0, 0 );
            build_gip_instruction_cc( &gip_instr, map_condition_code(cc) );
            build_gip_instruction_rn( &gip_instr, map_source_register(rn) );
            build_gip_instruction_immediate( &gip_instr, num_regs*4 );
            if (wb)
            {
                build_gip_instruction_rd( &gip_instr, map_destination_register(rn) );
            }
            else
            {
                build_gip_instruction_rd( &gip_instr, gip_ins_rd_none );
            }
            execute_int_instruction( &gip_instr, &gip_results );
            pd->pc += 4;
            first=0;
        }

        /*b Generate num_regs 'ILDR[CC|CP]A[S][F] #i (Rn|Acc), #+4 or preindexed version
         */
        for (i=0; i<num_regs; i++)
        {
            for (j=0; (j<16) && ((regs&(1<<j))==0); j++);
            regs &= ~(1<<j);
            build_gip_instruction_load( &gip_instr, gip_ins_subclass_memory_word, pre^!up, 1, (rn==13), (num_regs-1-i), 1, (j==15)&&!(up&&wb) );
            if (first)
            {
                build_gip_instruction_cc( &gip_instr, map_condition_code(cc) );
                build_gip_instruction_rn( &gip_instr, map_source_register(rn) );
            }
            else
            {
                build_gip_instruction_cc( &gip_instr, gip_ins_cc_cp );
                build_gip_instruction_rn( &gip_instr, gip_ins_rnm_acc );
            }
            build_gip_instruction_immediate( &gip_instr, 4 );
            build_gip_instruction_rd( &gip_instr, map_destination_register(j) );
            execute_int_instruction( &gip_instr, &gip_results );
            if (first)
            {
                first = 0;
                pd->pc += 4;
            }
            if (gip_results.write_pc)
            {
                pd->pc = gip_results.rfw_data;
            }
            if (gip_results.flush)
            {
                return 1;
            }
        }

        /*b If IB/IA with writeback then do final MOVCP[F] Acc -> Rn; F if PC was read in the list
         */
        if (up&&wb)
        {
            build_gip_instruction_alu( &gip_instr, gip_ins_class_logic, gip_ins_subclass_logic_mov, 0, 0, 0, ((opcode&0x8000)!=0) );
            build_gip_instruction_cc( &gip_instr, gip_ins_cc_cp );
            build_gip_instruction_rm( &gip_instr, gip_ins_rnm_acc );
            build_gip_instruction_rd( &gip_instr, map_destination_register(rn) );
            execute_int_instruction( &gip_instr, &gip_results );
            if (gip_results.flush)
            {
                return 1;
            }
        }
        return 1;
    }

    /*b Handle store
     */
    if (!load)
    {
        /*b If DB/DA, do first instruction to generate base address: ISUB[CC]A Rn, #num_regs*4 [-> Rn]
         */
        first=1;
        if (!up)
        {
            build_gip_instruction_alu( &gip_instr, gip_ins_class_arith, gip_ins_subclass_arith_sub, 1, 0, 0, 0 );
            build_gip_instruction_cc( &gip_instr, map_condition_code(cc) );
            build_gip_instruction_rn( &gip_instr, map_source_register(rn) );
            build_gip_instruction_immediate( &gip_instr, num_regs*4 );
            if (wb)
            {
                build_gip_instruction_rd( &gip_instr, map_destination_register(rn) );
            }
            else
            {
                build_gip_instruction_rd( &gip_instr, gip_ins_rd_none );
            }
            execute_int_instruction( &gip_instr, &gip_results );
            first=0;
        }

        /*b Update PC
         */
        pd->pc += 4;

        /*b Generate num_regs 'ISTR[CC|CP]A[S][F] #i (Rn|Acc), #+4 [->Rn] or preindexed version
         */
        for (i=0; i<num_regs; i++)
        {
            for (j=0; (j<16) && ((regs&(1<<j))==0); j++);
            regs &= ~(1<<j);
            build_gip_instruction_store( &gip_instr, gip_ins_subclass_memory_word, pre^!up, 1, 0, (rn==13), (num_regs-1-i), 1, 0 );
            if (first)
            {
                build_gip_instruction_cc( &gip_instr, map_condition_code(cc) );
                build_gip_instruction_rn( &gip_instr, map_source_register(rn) );
            }
            else
            {
                build_gip_instruction_cc( &gip_instr, gip_ins_cc_cp );
                build_gip_instruction_rn( &gip_instr, gip_ins_rnm_acc );
            }
            build_gip_instruction_rm( &gip_instr, map_source_register(j) );
            if ((i==num_regs-1) && (up) && (wb))
            {
                build_gip_instruction_rd( &gip_instr, map_destination_register(rn) );
            }
            else
            {
                build_gip_instruction_rd( &gip_instr, gip_ins_rd_none );
            }
            execute_int_instruction( &gip_instr, &gip_results );
            first = 0;
        }

        return 1;
    }

    /*b Done
     */
    return 0; 
}

/*f c_gip_pipeline_single::execute_arm_mul
 */
int c_gip_pipeline_single::execute_arm_mul( unsigned int opcode )
{
    int cc, zero, nine, accum, sign, rd, rn, rs, rm;
    int i;
    t_gip_instruction gip_instr;
    t_gip_pipeline_results gip_results;

    /*b Decode ARM instruction
     */
    cc      = (opcode>>28) & 0x0f;
    zero    = (opcode>>22) & 0x3f;
    accum   = (opcode>>21) & 0x01;
    sign    = (opcode>>20) & 0x01;
    rd      = (opcode>>16) & 0x0f;
    rn      = (opcode>>12) & 0x0f;
    rs      = (opcode>>8) & 0x0f;
    nine    = (opcode>>4) & 0x0f;
    rm      = (opcode>>0) & 0x0f;

    /*b Validate MUL/MLA instruction
     */
    if ((zero!=0) || (nine!=9) || (cc==15))
        return 0;

    /*b First the INIT instruction
     */
    build_gip_instruction_alu( &gip_instr, gip_ins_class_arith, gip_ins_subclass_arith_init, 1, 0, 0, 0 );
    build_gip_instruction_cc( &gip_instr, map_condition_code(cc) );
    build_gip_instruction_rn( &gip_instr, map_source_register(rm) ); // Note these are
    if (accum)
    {
        build_gip_instruction_rm( &gip_instr, map_source_register(rn) ); //  correctly reversed
    }
    else
    {
        build_gip_instruction_immediate( &gip_instr, 0 );
    }
    build_gip_instruction_rd( &gip_instr, gip_ins_rd_none );
    execute_int_instruction( &gip_instr, &gip_results );

    /*b Then the MLA instruction to get the ALU inputs ready, and do the first step
     */
    build_gip_instruction_alu( &gip_instr, gip_ins_class_arith, gip_ins_subclass_arith_mla, 1, 0, 0, 0 );
    build_gip_instruction_cc( &gip_instr, gip_ins_cc_cp );
    build_gip_instruction_rn( &gip_instr, gip_ins_rnm_acc );
    build_gip_instruction_rm( &gip_instr, map_source_register(rs) );
    build_gip_instruction_rd( &gip_instr, gip_ins_rd_none );
    execute_int_instruction( &gip_instr, &gip_results );

    /*b Then 14 MLB instructions to churn
     */
    for (i=0; i<14; i++)
    {
        build_gip_instruction_alu( &gip_instr, gip_ins_class_arith, gip_ins_subclass_arith_mlb, 1, 0, 0, 0 );
        build_gip_instruction_cc( &gip_instr, gip_ins_cc_cp );
        build_gip_instruction_rn( &gip_instr, gip_ins_rnm_acc );
        build_gip_instruction_immediate( &gip_instr, 0 ); // Not used
        build_gip_instruction_rd( &gip_instr, gip_ins_rd_none );
        execute_int_instruction( &gip_instr, &gip_results );
    }

    /*b Then the last MLB instructions to produce the final results
     */
    build_gip_instruction_alu( &gip_instr, gip_ins_class_arith, gip_ins_subclass_arith_mlb, 1, sign, 0, 0 );
    build_gip_instruction_cc( &gip_instr, gip_ins_cc_cp );
    build_gip_instruction_rn( &gip_instr, gip_ins_rnm_acc );
    build_gip_instruction_immediate( &gip_instr, 0 ); // Not used
    build_gip_instruction_rd( &gip_instr, map_destination_register(rd) );
    execute_int_instruction( &gip_instr, &gip_results );
    pd->pc += 4;

    /*b Done
     */
    return 1; 
}

/*f c_gip_pipeline_single::execute_arm_trace
  Steal top half of SWI space
*/
int c_gip_pipeline_single::execute_arm_trace( unsigned int opcode )
{
    int cc, fifteen, code;

    /*b Decode ARM instruction
     */
    cc      = (opcode>>28) & 0xf;
    fifteen = (opcode>>24) & 0xf;
    code    = (opcode>> 0) & 0xffffff;

//    code=code&0xffff;
//printf ("execute_trace 0x%x (0x%x %d 0x%x)\n", opcode, cc, fifteen, code);

    if (fifteen!=15)
        return 0;

    if (1 || (code & 0x800000))
    {
        //    printf("c_arm:trace_hit:%06d:%06x@0x%08x\n", pd->cycle, code, pd->regs[15] );
    	pd->regs[15] += 4;
//	printf ("SWI %x\n", code);
//        pd->ukernel->handle_swi (code);
    	return 1; 
    }
/*
 *     else
 {
 <<<<<<< c_arm_model.cpp
 const char * codename = code <= noof_call_names ? call_name[code] : "<unknown>";
 printf ("Executed a SWI %x (%s (0x%x,0x%x,0x%x) return to 0x%x)\n", code, codename, pd->regs[0], pd->regs[1], pd->regs[2], pd->regs[15]);
 if (code == 4 && pd->regs[0] == 2)
 {
 printf ("Print to stderr: ");
 unsigned int k;
 for (k = 0; k < pd->regs[2]; k++)
 {
 unsigned long addr = pd->regs[1] + k;
 unsigned long data = pd->memory->read_memory (addr & ~3);
 data >>= 8 * (addr & 3);
 data &= 0xff;
 if (data > 31 && data < 127)
 printf ("%c", (char)data);
 else
 printf ("{%2.2lx}", data);
 }
 printf ("\n");
 }
 =======
//	    const char * codename = code <= noof_call_names ? call_name[code] : "<unknown>";
//	    printf ("Executed a SWI %x (%s (0x%x,0x%x,0x%x) return to 0x%x)\n", code, codename, pd->regs[0], pd->regs[1], pd->regs[2], pd->regs[15]);
>>>>>>> 1.17
pd->swi_code = code;
pd->regs[15] += 4;
do_interrupt (1);
{
void swi_hack(void);
swi_hack();
}

return 1;
}
*/
        }

/*a Debug interface
 */
/*f c_gip_pipeline_single::set_register
 */
void c_gip_pipeline_single::set_register( int r, unsigned int value )
{
    pd->regs[r&0x1f] = value;
}

/*f c_gip_pipeline_single::get_register
 */
unsigned int c_gip_pipeline_single::get_register( int r )
{
    return pd->regs[r&0x1f];
}

/*f c_gip_pipeline_single::set_flags
 */
void c_gip_pipeline_single::set_flags( int value, int mask )
{
    if (mask & gip_flag_mask_z)
        pd->z = ((value & gip_flag_mask_z)!=0);
    if (mask & gip_flag_mask_c)
        pd->c = ((value & gip_flag_mask_c)!=0);
    if (mask & gip_flag_mask_n)
        pd->n = ((value & gip_flag_mask_n)!=0);
    if (mask & gip_flag_mask_v)
        pd->v = ((value & gip_flag_mask_v)!=0);
}

/*f c_gip_pipeline_single::get_flags
 */
int c_gip_pipeline_single::get_flags( void )
{
    int result;
    result = 0;
    if (pd->z)
        result |= gip_flag_mask_z;
    if (pd->c)
        result |= gip_flag_mask_c;
    if (pd->n)
        result |= gip_flag_mask_n;
    if (pd->v)
        result |= gip_flag_mask_v;
    return result;
}

/*f c_gip_pipeline_single::set_breakpoint
 */
int c_gip_pipeline_single::set_breakpoint( unsigned int address )
{
    int bkpt;
    for (bkpt = 0; bkpt < MAX_BREAKPOINTS && (pd->breakpoints_enabled & (1<<bkpt)); bkpt++)
	    ;
    if ((bkpt<0) || (bkpt>=MAX_BREAKPOINTS))
        return 0;
    pd->breakpoint_addresses[bkpt] = address;
    pd->breakpoints_enabled = pd->breakpoints_enabled | (1<<bkpt);
    pd->debugging_enabled = 1;
//    printf("c_gip_pipeline_single::set_breakpoint:%d:%08x\n", bkpt, address );
    return 1;
}

/*f c_gip_pipeline_single::unset_breakpoint
 */
int c_gip_pipeline_single::unset_breakpoint( unsigned int address )
{
    int bkpt;
    for (bkpt = 0; bkpt < MAX_BREAKPOINTS && pd->breakpoint_addresses[bkpt] != address; bkpt++)
	    ;
    if ((bkpt<0) || (bkpt>=MAX_BREAKPOINTS))
        return 0;
    pd->breakpoints_enabled = pd->breakpoints_enabled &~ (1<<bkpt);
    SET_DEBUGGING_ENABLED(pd);
//    printf("c_gip_pipeline_single::unset_breakpoint:%d\n", bkpt );
    return 1;
}

/*f c_gip_pipeline_single::halt_cpu
  If we are currently executing, forces an immediate halt
  This is used by a memory model to abort execution after an unmapped access, for example
*/
void c_gip_pipeline_single::halt_cpu( void )
{
    pd->debugging_enabled = 1;
    pd->halt = 1;
    printf("c_gip_pipeline_single::halt_cpu\n");
}

