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
#define REG_VALUE_32(pd, r, o) ((r!=15)?pd->regs[r]:(pd->regs[r]+o))
#define MAX_BREAKPOINTS (8)
#define MAX_TRACING (8)
#define MAX_INTERRUPTS (8)
#define SET_DEBUGGING_ENABLED(pd) {pd->debugging_enabled = (pd->breakpoints_enabled != 0) || (pd->halt);}
#define MAX_REGISTERS (32)
#define GIP_REGISTER_MASK (0x1f)
#define GIP_REGISTER_TYPE (0x20)

/*a Types
 */
/*t t_log_data
 */
typedef struct t_log_data
{
    unsigned int address;
    unsigned int opcode;
    int conditional;
    int condition_passed;
    int sequential;
    int branch;
    int rfr;
    int rfw;
    int sign;
} t_log_data;

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

/*t t_gip_pipeline_single_data
 */
typedef struct t_gip_pipeline_single_data
{
    c_memory_model *memory;
    int cycle;
    t_log_data log;

    /*b Configuration of the pipeline
     */
    int sticky_v;
    int sticky_z;
    int sticky_c;
    int sticky_n;

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

    /*b Combinatorials in the pipeline
     */
    int condition_passed; // Requested condition in the ALU stage passed
    t_gip_alu_op1_src op1_src;
    t_gip_alu_op2_src op2_src;
    int set_zcvn;
    int set_p;
    unsigned int alu_op1; // Operand 1 to the ALU logic itself
    unsigned int alu_op2; // Operand 2 to the ALU logic itself
    unsigned int shf_result; // Result of the shifter
    int shf_carry; // Carry out of the shifter
    unsigned int alu_result; // Result of the ALU (arithmetic/logical result)

    /*b Irrelevant stuff for now
     */
    unsigned int last_address;
    int seq_count;
    int debugging_enabled;
    int breakpoints_enabled;
    unsigned int breakpoint_addresses[ MAX_BREAKPOINTS ];
    int tracing_enabled; // If 0, no tracing, else tracing is performed
    FILE *trace_file;
    unsigned int trace_region_starts[ MAX_TRACING ]; // inclusive start address of area to trace - up to MAX_TRACING areas
    unsigned int trace_region_ends[ MAX_TRACING ]; // exclusive end address of area to trace - up to MAX_TRACING areas
    int halt;
    
    int interrupt_handler[MAX_INTERRUPTS];
    int swi_return_addr;
    int swi_sp;
    int swi_code;
    int kernel_sp;
    int super;

    struct microkernel * ukernel;
} t_gip_pipeline_single_data;

/*a Static variables
 */

/*a Support functions
 */
/*f is_condition_met
 */
static int is_condition_met( int cc, t_gip_pipeline_single_data *data )
{
    switch (cc)
    {
    case 0: return data->z;
    case 1: return !data->z;
    case 2: return data->c;
    case 3: return !data->c;
    case 4: return data->n;
    case 5: return !data->n;
    case 6: return data->v;
    case 7: return !data->v;
    case 8: return !data->z && data->c;
    case 9: return !data->c || data->z;
    case 10: return (data->n && data->v) || (!data->n && !data->v);
    case 11: return (data->n && !data->v) || (!data->n && data->v);
    case 12: return !data->z && ((data->n && data->v) || (!data->n && !data->v));
    case 13: return data->z || (data->n && !data->v) || (!data->n && data->v);
    case 14: return 1;
    case 15: return 0;
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
unsigned int barrel_shift( int c_in, t_shf_type type, unsigned int val, int by, int *c_out )
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

/*a Log methods
 */
/*f c_gip_pipeline_single::log
 */
void c_gip_pipeline_single::log( char *reason, unsigned int arg )
{
    if (!strcmp(reason, "address"))
    {
        pd->log.address = arg;
    }
    else if (!strcmp(reason, "opcode"))
    {
        pd->log.opcode = arg;
    }
    else if (!strcmp(reason, "conditional"))
    {
        pd->log.conditional = arg;
    }
    else if (!strcmp(reason, "condition_passed"))
    {
        pd->log.condition_passed = arg;
    }
    else if (!strcmp(reason, "branch"))
    {
        pd->log.branch = arg;
    }
    else if (!strcmp(reason, "rfr"))
    {
        pd->log.rfr |= (1<<arg);
    }
    else if (!strcmp(reason, "rfw"))
    {
        pd->log.rfw |= (1<<arg);
    }
    else if (!strcmp(reason, "sign"))
    {
        pd->log.sign = arg;
    }
}

/*f c_gip_pipeline_single::log_display
 */
void c_gip_pipeline_single::log_display( FILE *f )
{
    fprintf( f, "%08x %08x %1d %1d %1d %04x %04x %1d",
             pd->log.address,
             pd->log.opcode,
             pd->log.sequential,
             pd->log.conditional,
             pd->log.condition_passed,
             pd->log.rfr,
             pd->log.rfw,
             pd->log.sign
        );
    fprintf( f, "\n" );
}

/*f c_gip_pipeline_single::log_reset
 */
void c_gip_pipeline_single::log_reset( void )
{
    pd->log.sequential = !pd->log.branch;
    pd->log.conditional = 0;
    pd->log.condition_passed = 1;
    pd->log.branch = 0;
    pd->log.rfr = 0;
    pd->log.rfw = 0;
    pd->log.sign = 0;
}

/*a Constructors/destructors
 */
/*f c_gip_pipeline_single::c_gip_pipeline_single
 */
c_gip_pipeline_single::c_gip_pipeline_single( c_memory_model *memory )
{
    int i;

    pd = (t_gip_pipeline_single_data *)malloc(sizeof(t_gip_pipeline_single_data));
    //memset (pd, 0, sizeof(*pd));
    pd->cycle = 0;
    pd->memory = memory;

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
      pd->trace_file = NULL;
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
c_gip_pipeline_single::~c_gip_pipeline_single( void )
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
            printf("%d\n0x%08x:", pd->seq_count, address );
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
        printf( "\t  c:%d  n:%d  z:%d  v:%d p:%d cp:%d\n",
                pd->c,
                pd->n,
                pd->z,
                pd->v,
                pd->p,
                pd->cp );
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
/*f c_gip_pipeline_single:arm_step
  Returns 1 for success, 0 for failure
 */
int c_gip_pipeline_single::arm_step( int *reason )
{
    unsigned int opcode;

    /*b fetch from pc
     */
    log_reset();
    opcode = pd->memory->read_memory( pd->pc );
    log( "address", pd->pc );
    log( "opcode", opcode );

    /*b attempt to execute opcode, trying each instruction coding one at a time till we succeed
     */
    if ( execute_arm_alu( opcode ) ||
         execute_arm_ld_st( opcode ) ||
         execute_arm_ldm_stm( opcode ) ||
         execute_arm_branch( opcode ) ||
         execute_arm_mul( opcode ) ||
         execute_arm_trace( opcode ) ||
         0 )
    {
        //log_display( stdout );
        return 1;
    }
    *reason = opcode;
    printf( "failed_to_execute %x\n", opcode );
    debug(-1);
    if (1) 
    {
        pd->pc += 4;
        return 1;
    } 
    return 0;
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

/*a Execution methods
 */
/*f c_gip_pipeline_single::execute_int_alu_instruction
 */
void c_gip_pipeline_single::execute_int_alu_instruction( t_gip_alu_op alu_op, t_gip_alu_op1_src op1_src, t_gip_alu_op2_src op2_src, int s, int p )
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
    }

    /*b Perform shifter operation
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
    case gip_alu_op_xorcnt:
    case gip_alu_op_add:
    case gip_alu_op_adc:
    case gip_alu_op_sub:
    case gip_alu_op_sbc:
    case gip_alu_op_rsub:
    case gip_alu_op_rsbc:
        break;
    case gip_alu_op_lsl:
        barrel_shift( pd->c, shf_type_lsl, pd->alu_a_in, pd->alu_b_in, &pd->p );
        break;
    case gip_alu_op_lsr:
        barrel_shift( pd->c, shf_type_lsr, pd->alu_a_in, pd->alu_b_in, &pd->p );
        break;
    case gip_alu_op_asr:
        barrel_shift( pd->c, shf_type_asr, pd->alu_a_in, pd->alu_b_in, &pd->p );
        break;
    case gip_alu_op_ror:
        barrel_shift( pd->c, shf_type_ror, pd->alu_a_in, pd->alu_b_in, &pd->p );
        break;
    case gip_alu_op_ror33:
        barrel_shift( pd->c, shf_type_rrx, pd->alu_a_in, pd->alu_b_in, &pd->p );
        break;
    case gip_alu_op_init:
        break;
    case gip_alu_op_mulst:
        break;
    case gip_alu_op_divst:
        break;
    }

    /*b Perform arithmetic operation
     */
    switch (alu_op)
    {
    case gip_alu_op_mov:
        break;
    case gip_alu_op_mvn:
        break;
    case gip_alu_op_and:
        break;
    case gip_alu_op_or:
    case gip_alu_op_xor:
    case gip_alu_op_bic:
    case gip_alu_op_orn:
    case gip_alu_op_xorcnt:
    case gip_alu_op_lsl:
    case gip_alu_op_lsr:
    case gip_alu_op_asr:
    case gip_alu_op_ror:
    case gip_alu_op_ror33:
    case gip_alu_op_add:
    case gip_alu_op_adc:
    case gip_alu_op_sub:
    case gip_alu_op_sbc:
    case gip_alu_op_rsub:
    case gip_alu_op_rsbc:
    case gip_alu_op_init:
    case gip_alu_op_mulst:
    case gip_alu_op_divst:
        break;
    }

    /*b Perform logical operation
     */
    switch (alu_op)
    {
    case gip_alu_op_mov:
        break;
    case gip_alu_op_mvn:
        break;
    case gip_alu_op_and:
        break;
    case gip_alu_op_or:
    case gip_alu_op_xor:
    case gip_alu_op_bic:
    case gip_alu_op_orn:
    case gip_alu_op_xorcnt:
    case gip_alu_op_lsl:
    case gip_alu_op_lsr:
    case gip_alu_op_asr:
    case gip_alu_op_ror:
    case gip_alu_op_ror33:
    case gip_alu_op_add:
    case gip_alu_op_adc:
    case gip_alu_op_sub:
    case gip_alu_op_sbc:
    case gip_alu_op_rsub:
    case gip_alu_op_rsbc:
    case gip_alu_op_init:
    case gip_alu_op_mulst:
    case gip_alu_op_divst:
        break;
    }
}

/*f c_gip_pipeline_single::read_int_register
 */
unsigned int c_gip_pipeline_single::read_int_register( int r )
{
    int index, type;
    unsigned int result;

    index = r&GIP_REGISTER_MASK;
    type = r&GIP_REGISTER_TYPE;
    result = pd->regs[index];
    if (type)
    {
        switch ( (t_gip_register_encode)index)
        {
        case gip_register_encode_acc:
            break;
        case gip_register_encode_shf:
            break;
        }
    }
    return result;
}

/*f c_gip_pipeline_single::execute_int_instruction
 */
void c_gip_pipeline_single::execute_int_instruction( t_gip_instruction *inst, t_gip_pipeline_results *results )
{
    /*b Read the register file
     */
    pd->alu_a_in = read_int_register( inst->rn ); // Read register/special
    if (inst->rm_type)
    {
        pd->alu_b_in = inst->rm_data; // If immediate, pass immediate data in
    }
    else
    {
        pd->alu_b_in = read_int_register( (int)(inst->rm_data) ); // Else read register/special
    }

    /*b Read the coprocessor read file - supposed to be simultaneous with register file read
     */

    /*b Evaluate condition associated with the instruction
     */

    /*b Execute the ALU stage - supposed to be cycle after register file read and coprocessor read
     */
    t_gip_alu_op gip_alu_op;
    switch (inst->op)
    {
    case gip_ins_op_iadd:
        gip_alu_op = gip_alu_op_add;
        break;
    case gip_ins_op_iadc:
        gip_alu_op = gip_alu_op_adc;
        break;
    case gip_ins_op_isub:
        gip_alu_op = gip_alu_op_sub;
        break;
    case gip_ins_op_isbc:
        gip_alu_op = gip_alu_op_sbc;
        break;
    case gip_ins_op_irsb:
        gip_alu_op = gip_alu_op_rsub;
        break;
    case gip_ins_op_irsc:
        gip_alu_op = gip_alu_op_rsbc;
        break;
    case gip_ins_op_iand:
        gip_alu_op = gip_alu_op_and;
        break;
    case gip_ins_op_iorr:
        gip_alu_op = gip_alu_op_or;
        break;
    case gip_ins_op_ixor:
        gip_alu_op = gip_alu_op_xor;
        break;
    case gip_ins_op_ibic:
        gip_alu_op = gip_alu_op_bic;
        break;
    case gip_ins_op_iorn:
        gip_alu_op = gip_alu_op_orn;
        break;
    case gip_ins_op_imov:
        gip_alu_op = gip_alu_op_mov;
        break;
    case gip_ins_op_imvn:
        gip_alu_op = gip_alu_op_mvn;
        break;
    case gip_ins_op_ilsl:
        gip_alu_op = gip_alu_op_lsl;
        break;
    case gip_ins_op_ilsr:
        gip_alu_op = gip_alu_op_lsr;
        break;
    case gip_ins_op_iasr:
        gip_alu_op = gip_alu_op_asr;
        break;
    case gip_ins_op_iror:
        gip_alu_op = gip_alu_op_ror;
        break;
    case gip_ins_op_iror33:
        gip_alu_op = gip_alu_op_ror33;
        break;
    case gip_ins_op_icprd:
        gip_alu_op = gip_alu_op_mov;
        break;
    case gip_ins_op_icpwr:
        gip_alu_op = gip_alu_op_mov;
        break;
    case gip_ins_op_icpcmd:
        gip_alu_op = gip_alu_op_mov;
        break;
    case gip_ins_op_ildrpre:
    case gip_ins_op_ildrpost:
    case gip_ins_op_istr:
        if (inst->opts.memory.add)
        {
            gip_alu_op = gip_alu_op_add;
        }
        else
        {
            gip_alu_op = gip_alu_op_sub;
        }
        break;
    }
    execute_int_alu_instruction( gip_alu_op, pd->op1_src, pd->op2_src, pd->set_zcvn, pd->set_p );

    /*b Execute memory read/write - occurs after ALU operation
     */

    /*b Execute coprocessor write - occurs after memory read/write
     */

    /*b Register file writeback
     */

    /*b Done
     */
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
    unsigned int rn_val, rm_val, rs_val;
    unsigned int shf_out;
    int shf_carry;
    unsigned int alu_out;
    int alu_c, alu_z, alu_v, alu_n;
    int dummy;

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
    imm_rotate = (op2>>8) & 0x0f;
    imm_val    = (op2>>0) & 0xff;

    shf_rs       = (op2>> 8) & 0x0f;
    shf_reg_zero = (op2>> 7) & 0x01;
    shf_imm_amt  = (op2>> 7) & 0x1f;
    shf_how      = (op2>> 5) & 0x03;
    shf_by_reg   = (op2>> 4) & 0x01;
    shf_rm       = (op2>> 0) & 0x0f;

    //if (!imm && shf_by_reg && shf_reg_zero)
    //{
    //if (debug_slow) printf("Slow:ALU:Abort - imm zero, shf by reg 1, shf_reg_zero not zero\n");
    //  return 0;
    //}

    /*b Map condition code
     */

    /*b Test for shift of 'LSL #0' or plain immediate
     */
    if ( (imm) ||
         ( (!shf_by_reg) && (((t_shf_type)shf_how)==shf_type_lsl) && (shf_imm_amt==0) )
    {
        build_gip_instruction( op, a=1, flush=(rd==pc), rn, rm_type, rm_data, rdw );
    }

    /*b Get carry and operate barrel shifter
     */
    shf_carry = pd->c;
    if (imm)
    {
        log("immediate", 1 );
        shf_out = barrel_shift( 0, shf_type_ror, imm_val, imm_rotate*2, &dummy );
    }
    else if (shf_by_reg)
    {
        log("rfr", shf_rs );
        log("rfr", shf_rm );
        log("shf", shf_how );
        if (shf_reg_zero)
        {
            if (debug_slow) printf( "Slow:ALU:Abort shf_reg_zero nonzero\n");
            return 0;
        }
        shf_out = barrel_shift( pd->c, (t_shf_type) shf_how, rm_val, rs_val&0xff, &shf_carry );
    }
    else if ((shf_how==0) && (shf_imm_amt==0))
    {
        log("rfr", shf_rm );
        shf_out = rm_val; // lsl #0 is no shift, no carry fussing
    }
    else if ((shf_how==3) && (shf_imm_amt==0)) // ror #0 is rrx
    {
        log("rfr", shf_rm );
        log("shf", shf_type_rrx );
        shf_out = barrel_shift( pd->c, shf_type_rrx, rm_val, 1, &shf_carry );
    }
    else if ((shf_how==1) && (shf_imm_amt==0)) // lsr #0 is lsr #32
    {
        log("rfr", shf_rm );
        log("shf", shf_how );
        shf_out = barrel_shift( pd->c, (t_shf_type) shf_how, rm_val, 32, &shf_carry );
    }
    else if ((shf_how==2) && (shf_imm_amt==0)) // asr #0 is asr #32
    {
        log("rfr", shf_rm );
        log("shf", shf_how );
        shf_out = barrel_shift( pd->c, (t_shf_type) shf_how, rm_val, 32, &shf_carry );
    }
    else
    {
        log("rfr", shf_rm );
        log("shf", shf_how );
        shf_out = barrel_shift( pd->c, (t_shf_type) shf_how, rm_val, shf_imm_amt, &shf_carry );
    }

    /*b Now run the ALU
     */
    log("rfr", rn );
    alu_v = pd->v;
    switch (alu_op)
    {
    case  0: // and
        alu_c = shf_carry;
        alu_out = rn_val & shf_out;
        break;
    case  1: // eor
        alu_c = shf_carry;
        alu_out = rn_val ^ shf_out;
        break;
    case  2: // sub
        alu_out = add_op( rn_val, ~shf_out, 1, &alu_c, &alu_v );
        break;
    case  3: // rsb
        alu_out = add_op( ~rn_val, shf_out, 1, &alu_c, &alu_v );
        break;
    case  4: // add
        alu_out = add_op( rn_val, shf_out, 0, &alu_c, &alu_v );
        break;
    case  5: // adc
        alu_out = add_op( rn_val, shf_out, pd->c, &alu_c, &alu_v );
        break;
    case  6: // sbc
        alu_out = add_op( rn_val, ~shf_out, pd->c, &alu_c, &alu_v );
        break;
    case  7: // rsc
        alu_out = add_op( ~rn_val, shf_out, pd->c, &alu_c, &alu_v );
        break;
    case  8: // tst
        alu_c = shf_carry;
        alu_out = rn_val & shf_out;
        break;
    case  9: // teq
        alu_c = shf_carry;
        alu_out = rn_val ^ shf_out;
        break;
    case 10: // cmp
        alu_out = add_op( rn_val, ~shf_out, 1, &alu_c, &alu_v );
        break;
    case 11: // cmn
        alu_out = add_op( rn_val, ~-shf_out, 1, &alu_c, &alu_v );
        break;
    case 12: // orr
        alu_c = shf_carry;
        alu_out = rn_val | shf_out;
        break;
    case 13: // mov
        alu_c = shf_carry;
        alu_out = shf_out;
        break;
    case 14: // bic
        alu_c = shf_carry;
        alu_out = rn_val &~ shf_out;
        break;
    case 15: // mvn
        alu_c = shf_carry;
        alu_out = ~shf_out;
        break;
    default:
        alu_out = 0;
        break;
    }
    alu_n = ((alu_out&0x80000000)!=0);
    alu_z = (alu_out==0);

    if (sign)
    {
        pd->c = alu_c;
        pd->z = alu_z;
        pd->v = alu_v;
        pd->n = alu_n;
        log("sign", 1 );
    }

    if ( (alu_op == 0x8) ||
         (alu_op == 0x9) ||
         (alu_op == 0xa) ||
         (alu_op == 0xb) ) // cmp, cmn, tst, teq
    {
        if (rd==15)
        {
            if (debug_slow) printf("Slow: TEQP/TSTP\n");
        }
        if (!sign)
        {
            if (debug_slow) printf("Slow: msr/mrs?\n");
            pd->regs[15] += 4;
            return 1;
        }
    }
    else
    {
        log("rfw", rd );
        pd->regs[ rd ] = alu_out;
        if ((rd==15) && (sign))
        {
            if (debug_slow) printf("Slow:ns - ALUS pc!!\n");
        }
        if (rd==15)
        {
            return 1;
        }
    }
    pd->regs[15] += 4;
    return 1;
}

/*f c_gip_pipeline_single::execute_arm_branch
 */
int c_gip_pipeline_single::execute_arm_branch( unsigned int opcode )
{
    int cc, five, link, offset;

    cc      = (opcode>>28) & 0x0f;
    five    = (opcode>>25) & 0x07;
    link    = (opcode>>24) & 0x01;
    offset  = (opcode>> 0) & 0x00ffffff;

    if (five != 5)
        return 0;


    /*b Condition code not met?
     */
    if (cc!=14) log("conditional", 1);
    if (!is_condition_met( cc, pd))
    {
        log("condition_passed", 0 );
        pd->regs[15] += 4;
        return 1;
    }

    /*b Branch
     */
    log( "branch", 1 );
    if (link)
    {
        log( "link", 1 );
        pd->regs[14] = register_value( pd, 15, 0, 4 );
    }
    pd->regs[15] += 8+((offset<<8)>>6);
    log( "branch_address", pd->regs[15] );
    return 1;
}

/*f c_gip_pipeline_single::execute_arm_ld_st
 */
int c_gip_pipeline_single::execute_arm_ld_st( unsigned int opcode )
{
    int cc, one, not_imm, pre, up, byte, wb, load, rn, rd, offset;
    int imm_val;
    int shf_imm_amt, shf_how, shf_zero, shf_rm;

    unsigned int shf_val;
    int dummy;
    unsigned int rn_val, rm_val, rd_val;
    unsigned int memory_address;

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

    if (one != 1)
        return 0;

    imm_val    = (offset>>0) & 0xfff;

    shf_imm_amt  = (offset>> 7) & 0x1f;
    shf_how      = (offset>> 5) & 0x01;
    shf_zero     = (offset>> 4) & 0x01;
    shf_rm       = (offset>> 0) & 0x0f;

    if (not_imm && shf_zero)
        return 0;

    /*b Condition code not met?
     */
    if (cc!=14) log("conditional", 1);
    if (!is_condition_met( cc, pd))
    {
        log("condition_passed", 0 );
        pd->regs[15] += 4;
        return 1;
    }

    /*b Read registers
     */
    rn_val = register_value( pd, rn, 0, 8 );
    rm_val = register_value( pd, shf_rm, 1, 8 );
    rd_val = register_value( pd, rd, 1, 12 );

    if (!not_imm)
    {
        shf_val = imm_val;
    }
    else if ((shf_how==0) && (shf_imm_amt==0))
    {
        shf_val = rm_val;
    }
    else if ((shf_how==3) && (shf_imm_amt==0))
    {
        shf_val = barrel_shift( 0, shf_type_rrx, rm_val, 1, &dummy );
    }
    else
    {
        shf_val = barrel_shift( 0, (t_shf_type) shf_how, rm_val, shf_imm_amt, &dummy );
    }

    memory_address = rn_val;

    if (pre)
    {
        memory_address += (up ? shf_val : -shf_val );
    }
    if (wb || !pre)
    {
        pd->regs[rn] = rn_val + (up?shf_val:-shf_val);
    }

    if (load)
    {
        unsigned int data;
//      read to rd, with rotate as necessary, possibly a byte
        data = pd->memory->read_memory( memory_address    );
        if (byte)
        {
            int offset_in_word = memory_address & 3;
            data >>= 8 * offset_in_word;
            data &= 0xff;
        }
//      printf("Read memory %08x gives %08x\n", memory_address, data);
        pd->regs[rd] = data;
    }
    else
    {
//      write rd, with rotate as necessary, possibly a byte
//      printf("Write memory %08x with %08x\n", memory_address, rd_val);
        if (byte)
        {
            int offset_in_word = memory_address & 3;
            memory_address &= ~3;
            int mask = 1 << offset_in_word; // is this endian-specific ?
            rd_val <<= 8 * offset_in_word;
            pd->memory->write_memory( memory_address, rd_val, mask );
        }
        else
        {
            pd->memory->write_memory( memory_address, rd_val, 0xf );
        }
    }

    if (byte)
    {
//      append_string( &buffer, "b" );
    }
    if ( (rd!=15) || !load)
    {
        pd->regs[15] += 4;
    }
    return 1; 
}

/*f c_gip_pipeline_single::execute_arm_ldm_stm
 */
int c_gip_pipeline_single::execute_arm_ldm_stm( unsigned int opcode )
{
    int cc, four, pre, up, psr, wb, load, rn, regs;
    int i, j;
    unsigned int rn_val;
    int num_regs;
    unsigned int memory_address;
    unsigned int data;

    cc      = (opcode>>28) & 0x0f;
    four    = (opcode>>25) & 0x07;
    pre     = (opcode>>24) & 0x01;
    up      = (opcode>>23) & 0x01;
    psr     = (opcode>>22) & 0x01;
    wb      = (opcode>>21) & 0x01;
    load    = (opcode>>20) & 0x01;
    rn      = (opcode>>16) & 0x0f;
    regs    = (opcode>> 0) & 0xffff;

    if (four != 4)
        return 0;

    /*b Condition code not met?
     */
    if (cc!=14) log("conditional", 1);
    if (!is_condition_met( cc, pd))
    {
        log("condition_passed", 0 );
        pd->regs[15] += 4;
        return 1;
    }

    /*b Empty registers to transfer is just a return
     */
    if (regs==0)
    {
        pd->regs[15] += 4;
        return 1;
    }

    /*b Read base register
     */
    rn_val = register_value( pd, rn, 0, 8 );

    // Note - ARM does not allow writeback of r15 as base - just assumes wb is 0
    // Note - ARM reads with writeback use the read data for rn if it is in the list
    // Note - ARM store with writeback of first register in store writes unchanged value, of second or later writes changed value
    // Note - ARM store of PC is PC+12
    // Note - ARM documentation did not say what to use as base (PC+8 or PC+12) if PC is base - we'll use 8

    /*b Calculate memory address and writeback value
     */
    for (i=regs, num_regs=0; i; i>>=1)
    {
        if (i&1)
        {
            num_regs++;
        }
    }
    memory_address = rn_val;
    if (up)
    {
        if (pre)
        {
            memory_address+=4;
        }
        rn_val += num_regs*4;
    }
    else
    {
        if (pre)
        {
            memory_address -= num_regs*4;
        }
        else
        {
            memory_address -= (num_regs-1)*4;
        }
        rn_val -= num_regs*4;
    }

    j = 0;
    for (i=0; i<16; i++)
    {
        if (regs&(1<<i))
        {
            if (!load)
            {
//                 printf("Write memory (stm) %08x with %08x\n", memory_address, pd->regs[i]);
                pd->memory->write_memory( memory_address, pd->regs[i], 0xf );
            }
            if (wb && (j==0) && (rn!=15))
            {
                pd->regs[rn] = rn_val;
            }
            if (load)
            {
                data = pd->memory->read_memory( memory_address );
//                 printf("Read memory (ldm) %08x gives %08x\n", memory_address, data);
                pd->regs[i] = data;
            }
            memory_address += 4;
            j++;
        }
    }

    if ( ((regs&0x8000)==0) || !load )
    {
        pd->regs[15] += 4;
    }
    return 1; 
}

/*f c_gip_pipeline_single::execute_arm_mul
 */
int c_gip_pipeline_single::execute_arm_mul( unsigned int opcode )
{
    int cc, zero, nine, accum, sign, rd, rn, rs, rm;
    unsigned int rn_val, rs_val, rm_val, result;

    cc      = (opcode>>28) & 0x0f;
    zero    = (opcode>>22) & 0x3f;
    accum   = (opcode>>21) & 0x01;
    sign    = (opcode>>20) & 0x01;
    rd      = (opcode>>16) & 0x0f;
    rn      = (opcode>>12) & 0x0f;
    rs      = (opcode>>8) & 0x0f;
    nine    = (opcode>>4) & 0x0f;
    rm      = (opcode>>0) & 0x0f;

    if ((zero!=0) || (nine!=9))
        return 0;

    rs_val = register_value( pd, rs, 1, 8 );
    rn_val = register_value( pd, rn, 1, 8 );
    rm_val = register_value( pd, rm, 1, 8 );
    result = 0;
    if (accum)
    {
        result = rn_val;
    }
    result += rs_val*rm_val;
    if (sign)
    {
        pd->c = 0;
        pd->z = (result==0);
        pd->n = (result&0x80000000)?1:0;
    }
    pd->regs[ rd ] = result;
    pd->regs[15] += 4;
//    printf("Mul(a%d) %d*%d+%d=%d\n", accum, rs_val,rm_val,rn_val,result);
//    printf("Mul(a%d) %08x*%08x+%08x=%08x\n", accum,rs_val,rm_val,rn_val,result);
    return 1; 
}

/*f c_gip_pipeline_single::execute_arm_trace
  Steal top half of SWI space
*/
int c_gip_pipeline_single::execute_arm_trace( unsigned int opcode )
{
    int cc, fifteen, code;

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
        pd->ukernel->handle_swi (code);
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

/*a Old ARM interrupt functions
 */
/*f c_arm_model::do_interrupt
  Execute either a software or hardware interrupt
*/
void c_arm_model::do_interrupt (int vector)
{
	if (pd->interrupt_handler [vector])
	{
//		printf ("on entry, pc=%x, sp=%x\n", pd->regs[15], pd->regs[13]);
		if (vector == 2)
			printf ("on entry, pc=%x, sp=%x\n", pd->regs[15], pd->regs[13]);
		pd->swi_return_addr = pd->regs[15];
		pd->swi_sp = pd->regs[13];
		pd->regs[15] = pd->interrupt_handler[vector];
		if (!pd->super)
			pd->regs[13] = pd->kernel_sp;
	}
}

/*f c_arm_model:register_microkernel
 */
void c_arm_model::register_microkernel (microkernel * ukernel)
{
	pd->ukernel = ukernel;
}

/*f c_arm_model::write_memory
 */
void c_arm_model::write_memory (unsigned int address, unsigned int data, unsigned int bits)
{
	pd->memory->write_memory (address, data, bits);
}

/*f c_arm_model::read_memory
 */
unsigned int c_arm_model::read_memory (unsigned int address)
{
	return pd->memory->read_memory (address);
}

/*a Public level methods
 */
/*f c_arm_model:debug
 */
void c_arm_model::debug( int mask )
{
    char buffer[256];
    unsigned int address;
    unsigned int opcode;

    if (mask&8)
    {
        address = pd->regs[15];
        if (address!=pd->last_address+4)
        {
            pd->last_address = address;
            printf("%d\n0x%08x:", pd->seq_count, address );
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
        printf( "\t  c:%d  n:%d  z:%d  v:%d\n",
                pd->c,
                pd->n,
                pd->z,
                pd->v );
    }
    if (mask&1)
    {
        address = pd->regs[15];
        opcode = pd->memory->read_memory( pd->regs[15] );
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

/*f c_arm_model:step
 */
int c_arm_model::step( int *reason )
{
    unsigned int opcode;

    /*b fetch from pc, which is regs[15]
     */
    log_reset();
    opcode = pd->memory->read_memory( pd->regs[15] );
    log( "address", pd->regs[15] );
    log( "opcode", opcode );

    /*b attempt to execute opcode, trying each instruction coding one at a time till we succeed
     */
    if ( execute_alu( opcode ) ||
         execute_ld_st( opcode ) ||
         execute_ldm_stm( opcode ) ||
         execute_branch( opcode ) ||
         execute_mul( opcode ) ||
         execute_trace( opcode ) ||
         0 )
    {
        //log_display( stdout );
        return 1;
    }
    *reason = opcode;
    printf( "failed_to_execute %x\n", opcode );
    debug(-1);
    if (1) 
    {
        pd->regs[15] += 4;
        return 1;
    } 
    return 0;
}

/*f c_arm_model::load_code
 */
void c_arm_model::load_code( FILE *f, unsigned int base_address )
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
    pd->regs[15] = 0;
}

/*f c_arm_model::load_code_binary
 */
void c_arm_model::load_code_binary( FILE *f, unsigned int base_address )
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
    pd->regs[15] = 0;
}

/*f c_arm_model::load_symbol_table
 */
void c_arm_model::load_symbol_table( char *filename )
{
    symbol_initialize( filename );
}

/*f c_arm_model:get_swi_return_addr
 */
int c_arm_model::get_swi_return_addr (void)
{
	return pd->swi_return_addr;
}

/*f c_arm_model::get_swi_code
 */
int c_arm_model::get_swi_code (void)
{
	return pd->swi_code;
}

/*f c_arm_model::set_interrupt_vector
 */
void c_arm_model::set_interrupt_vector (int vector, int handler)
{
    if ((vector<0) || (vector>=MAX_INTERRUPTS))
    {
        fprintf(stderr, "Attempt to set interrupt vector %d, max %d\n", vector, MAX_INTERRUPTS );
        exit(4);
    }
	pd->interrupt_handler[vector] = handler;
}

/*f c_arm_model::get_swi_sp
 */
int c_arm_model::get_swi_sp (void)
{
	return pd->swi_sp;
}

/*f c_arm_model::set_kernel_sp
 */
void c_arm_model::set_kernel_sp (int sp)
{
	pd->kernel_sp = sp;
}

/*f c_arm_model::get_kernel_sp
 */
int c_arm_model::get_kernel_sp (void)
{
	return pd->kernel_sp;
}

/*a Debug interface
 */
/*f c_arm_model::set_register
 */
void c_arm_model::set_register( int r, unsigned int value )
{
    pd->regs[r&0xf] = value;
}

/*f c_arm_model::get_register
 */
unsigned int c_arm_model::get_register( int r )
{
    return pd->regs[r&0xf];
}

/*f c_arm_model::set_flags
 */
void c_arm_model::set_flags( int value, int mask )
{
    if (mask & am_flag_z)
        pd->z = ((value & am_flag_z)!=0);
    if (mask & am_flag_c)
        pd->c = ((value & am_flag_c)!=0);
    if (mask & am_flag_n)
        pd->n = ((value & am_flag_n)!=0);
    if (mask & am_flag_v)
        pd->v = ((value & am_flag_v)!=0);
    if (mask & am_flag_super)
        pd->super = ((value & am_flag_super)!=0);
}

/*f c_arm_model::get_flags
 */
int c_arm_model::get_flags( void )
{
    int result;
    result = 0;
    if (pd->z)
        result |= am_flag_z;
    if (pd->c)
        result |= am_flag_c;
    if (pd->n)
        result |= am_flag_n;
    if (pd->v)
        result |= am_flag_v;
    if (pd->super)
        result |= am_flag_super;
    return result;
}

/*f c_arm_model::set_breakpoint
 */
int c_arm_model::set_breakpoint( unsigned int address )
{
    int bkpt;
    for (bkpt = 0; bkpt < MAX_BREAKPOINTS && (pd->breakpoints_enabled & (1<<bkpt)); bkpt++)
	    ;
    if ((bkpt<0) || (bkpt>=MAX_BREAKPOINTS))
        return 0;
    pd->breakpoint_addresses[bkpt] = address;
    pd->breakpoints_enabled = pd->breakpoints_enabled | (1<<bkpt);
    pd->debugging_enabled = 1;
//    printf("c_arm_model::set_breakpoint:%d:%08x\n", bkpt, address );
    return 1;
}

/*f c_arm_model::unset_breakpoint
 */
int c_arm_model::unset_breakpoint( unsigned int address )
{
    int bkpt;
    for (bkpt = 0; bkpt < MAX_BREAKPOINTS && pd->breakpoint_addresses[bkpt] != address; bkpt++)
	    ;
    if ((bkpt<0) || (bkpt>=MAX_BREAKPOINTS))
        return 0;
    pd->breakpoints_enabled = pd->breakpoints_enabled &~ (1<<bkpt);
    SET_DEBUGGING_ENABLED(pd);
//    printf("c_arm_model::unset_breakpoint:%d\n", bkpt );
    return 1;
}

/*f c_arm_model::halt_cpu
  If we are currently executing, forces an immediate halt
  This is used by a memory model to abort execution after an unmapped access, for example
*/
void c_arm_model::halt_cpu( void )
{
    pd->debugging_enabled = 1;
    pd->halt = 1;
    printf("c_arm_model::halt_cpu\n");
}

/*f c_arm_model::interrupt_emulator
 */
void c_arm_model::interrupt_emulator (void)
{
	pd->halt = 1;
}

/*a Trace interface
 */
/*f c_arm_model::trace_output
 */
void c_arm_model::trace_output( char *format, ... )
{
    va_list args;
    va_start( args, format );

    if (pd->trace_file)
    {
        vfprintf( pd->trace_file, format, args );
        fflush (pd->trace_file);
    }
    else
    {
        vprintf( format, args );
    }

    va_end(args);
}

/*f c_arm_model::trace_set_file
 */
int c_arm_model::trace_set_file( char *filename )
{
    if (pd->trace_file)
    {
        fclose( pd->trace_file );
        pd->trace_file = NULL;
    }
    if (filename)
    {
        pd->trace_file = fopen( filename, "w+" );
    }
    return (pd->trace_file!=NULL);
}

/*f c_arm_model::trace_region
 */
int c_arm_model::trace_region( int region, unsigned int start_address, unsigned int end_address )
{
    if ((region<0) || (region>=MAX_TRACING))
        return 0;
    pd->trace_region_starts[region] = start_address;
    pd->trace_region_ends[region] = end_address;
    pd->tracing_enabled = 1;
    printf ("Tracing enabled from %x to %x\n", start_address, end_address);
    return 1;
}

/*f c_arm_model::trace_region_stop
 */
int c_arm_model::trace_region_stop( int region )
{
    if ((region<0) || (region>=MAX_TRACING))
        return 0;
    return trace_restart();
}

/*f c_arm_model::trace_all_stop
 */
int c_arm_model::trace_all_stop( void )
{
    pd->tracing_enabled = 0;
    return 1;
}

/*f c_arm_model::trace_restart
 */
int c_arm_model::trace_restart( void )
{
    int i;

    pd->tracing_enabled = 0;
    for (i=0; i<MAX_TRACING; i++)
    {
        if (pd->trace_region_starts[i] != pd->trace_region_ends[i])
        {
            pd->tracing_enabled = 1;
        }
    }
    return 1;
}

