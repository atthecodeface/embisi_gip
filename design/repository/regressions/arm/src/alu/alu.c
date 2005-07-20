/*a Includes
 */
#include <stdlib.h>
#include <stdio.h>
#include "../common/wrapper.h"

/*a Tests to do - 4M cycles covers them all
 Base of 50k cycles
  LOGIC      7*20k  = 140k
  LOGIC_ADD  8*35k  = 280k
  RRXS       4*20k  = 80k
  RORS       22*20k = 440k
  LSRS       22*20k = 440k
  ASRS       22*20k = 440k
  LSLS       46*20k = 920k
  SHIFT      14*25k = 350k
  ADD        4*20k  = 80k
  ADDS       9*20k  = 180k
  CMP        9*20k  = 180k
 */
#define TEST_LOGIC
#define TEST_LOGIC_ADD
#define TEST_RRXS
#define TEST_RORS
#define TEST_LSRS
#define TEST_ASRS
#define TEST_LSLS
#define TEST_SHIFT
#define TEST_ADD
#define TEST_ADDS
#define TEST_CMP

/*a Defines
 */
#define joined(a) a
#define join(a,b) joined(a##b)
#define ALU3_ASM(fn,inst) \
            static unsigned int fn ( unsigned int a, unsigned int b ) { register unsigned int result;\
            __asm__ ( #inst " %0, %1, %2" : "=r" (result) : "r" (a), "r" (b) ); return result; }

#define ALU2_ASM(fn,inst) \
            static unsigned int fn ( unsigned int a, unsigned int b ) { register unsigned int result; \
            __asm__ ( #inst " %1, %2" : "=r" (result) : "r" (a), "r" (b) ); return result; }
#define MOV_SHF_ASM(fn,inst,shf) \
            static unsigned int fn ( unsigned int a, unsigned int b ) { register unsigned int result;\
            __asm__ ( #inst " %0, %1, " #shf " %2" : "=r" (result) : "r" (a), "r" (b) ); return result; }


#if (0)
#define SIM_DUMP_VARS   {__asm__ volatile ( ".word 0xf00000a1" : : );}
#define SIM_VERBOSE_ON  {__asm__ volatile ( ".word 0xf00000a2" : : );}
#define SIM_VERBOSE_OFF {__asm__ volatile ( ".word 0xf00000a3" : : );}
#else
#define SIM_DUMP_VARS
#define SIM_VERBOSE_ON
#define SIM_VERBOSE_OFF
#endif

/*a Types
 */
/*t t_alu_fn
 */
typedef unsigned int (*t_alu_fn)(unsigned int a, unsigned int b );

/*t t_test_vector
 */
typedef struct t_test_vector
{
    int zero_in;
    int negative_in;
    int carry_in;
    int overflow_in;
    unsigned int a_in;
    unsigned int b_in;
    t_alu_fn alu_fn;
    int zero_out;
    int negative_out;
    int carry_out;
    int overflow_out;
    unsigned int result;
    char *message;
} t_test_vector;

/*t t_test_vector_result
 */
typedef struct t_test_vector_result
{
    int zero_out;
    int negative_out;
    int carry_out;
    int overflow_out;
    unsigned int result;
} t_test_vector_result;

/*a Test functions
 */
/*f add_inst
 */
static unsigned int add_inst( unsigned int a, unsigned int b )
{
    register unsigned int result;
    __asm__ (
        "add %0, %1, %2"
        : "=r" (result) /* Outputs, starts at %0, commma separated list, specified as =r for assigns to a register */
        : "r" (a), "r" (b) /* Inputs, starting at %n, comma separated list, specified as r for intputs from a register */
         /* : "r8" ... Clobbers list, list of registers that it clobbers; no clobbers list, no : */
        );
    return result;
}

/*f adds_inst
 */
ALU3_ASM(adds_inst,adds)

/*f cmp_inst
 */
ALU2_ASM(cmp_inst,cmp)

/*f rors_inst
 */
MOV_SHF_ASM(rors_inst,movs,ror)

/*f lsrs_inst
 */
MOV_SHF_ASM(lsrs_inst,movs,lsr)

/*f asrs_inst
 */
MOV_SHF_ASM(asrs_inst,movs,asr)

/*f logic - assembler to perform 'XOR' both using EOR and with and, bic and orr
 */
static unsigned int logic( unsigned int a, unsigned int b )
{
    unsigned int result;
    // This test performs result = a^(a^b)
    // The first a^b is performed with an EOR instruction
    // The second ^ of the previous result with a is performed with the logical equivalent: x^y = (x|y) &! (x&y)
    // Basically result should be b
    // Carry, overflow flags not effected
    // Zero set if a==b
    // Negative set if a^b is negative
	__asm__ volatile (
        "eors r8, %1, %2 \n\t orr r9, %1, r8 \n\t and r8, %1, r8 \n\t bic %0, r9, r8"
        : "=r" (result) /* Outputs, starts at %0, commma separated list, specified as =r for assigns to a register */
        : "r" (a), "r" (b) /* Inputs, starting at %n, comma separated list, specified as r for intputs from a register */
        : "r8", "r9" /* Clobbers r8 */
        );
    return result;
}

/*f logic_add - assembler to perform 'ADD' using ADD, SUB, RSB and EOR, AND, OR, LSL
  Basic test is to do a+b with EOR, AND and OR (32 replications, basically mimicking a carry-pass adder)
  Then we have a+b; we want b out at the end, so do:
    (((a+b) - a) - (a+b)) + (a+b)
  To do the initial add we use r8/r10 to contain the EOR of a and b, and r9 to contain the add (carry to next bit up)
  Then r8 = r10^(r9<<1) is the half-add sum, and
       r9 = r10&(r9<<1) is the half-add carry,
  And we just do it 32 times (swapping r8/r10 each time)
 */
static unsigned int logic_add( unsigned int a, unsigned int b )
{
    unsigned int result;
	__asm__ volatile (
        "eor r8, %1, %2 \n\t and r9, %1, %2 \n\t   eor r10, r8, r9, lsl #1 \n\t and r9, r8, r9, lsl #1 \n\t   eor r8, r10, r9, lsl #1 \n\t and r9, r10, r9, lsl #1 \n\t   eor r10, r8, r9, lsl #1 \n\t and r9, r8, r9, lsl #1 \n\t   eor r8, r10, r9, lsl #1 \n\t and r9, r10, r9, lsl #1 \n\t   eor r10, r8, r9, lsl #1 \n\t and r9, r8, r9, lsl #1 \n\t   eor r8, r10, r9, lsl #1 \n\t and r9, r10, r9, lsl #1 \n\t   eor r10, r8, r9, lsl #1 \n\t and r9, r8, r9, lsl #1 \n\t   eor r8, r10, r9, lsl #1 \n\t and r9, r10, r9, lsl #1 \n\t   eor r10, r8, r9, lsl #1 \n\t and r9, r8, r9, lsl #1 \n\t   eor r8, r10, r9, lsl #1 \n\t and r9, r10, r9, lsl #1 \n\t   eor r10, r8, r9, lsl #1 \n\t and r9, r8, r9, lsl #1 \n\t   eor r8, r10, r9, lsl #1 \n\t and r9, r10, r9, lsl #1 \n\t   eor r10, r8, r9, lsl #1 \n\t and r9, r8, r9, lsl #1 \n\t   eor r8, r10, r9, lsl #1 \n\t and r9, r10, r9, lsl #1 \n\t   eor r10, r8, r9, lsl #1 \n\t and r9, r8, r9, lsl #1 \n\t   eor r8, r10, r9, lsl #1 \n\t and r9, r10, r9, lsl #1 \n\t   eor r10, r8, r9, lsl #1 \n\t and r9, r8, r9, lsl #1 \n\t   eor r8, r10, r9, lsl #1 \n\t and r9, r10, r9, lsl #1 \n\t   eor r10, r8, r9, lsl #1 \n\t and r9, r8, r9, lsl #1 \n\t   eor r8, r10, r9, lsl #1 \n\t and r9, r10, r9, lsl #1 \n\t   eor r10, r8, r9, lsl #1 \n\t and r9, r8, r9, lsl #1 \n\t   eor r8, r10, r9, lsl #1 \n\t and r9, r10, r9, lsl #1 \n\t   eor r10, r8, r9, lsl #1 \n\t and r9, r8, r9, lsl #1 \n\t   eor r8, r10, r9, lsl #1 \n\t and r9, r10, r9, lsl #1 \n\t   eor r10, r8, r9, lsl #1 \n\t and r9, r8, r9, lsl #1 \n\t   eor r8, r10, r9, lsl #1 \n\t and r9, r10, r9, lsl #1 \n\t   eor r10, r8, r9, lsl #1 \n\t and r9, r8, r9, lsl #1 \n\t   eor r8, r10, r9, lsl #1 \n\t and r9, r10, r9, lsl #1 \n\t   eor r10, r8, r9, lsl #1 \n\t and r9, r8, r9, lsl #1 \n\t   eor r8, r10, r9, lsl #1 \n\t and r9, r10, r9, lsl #1 \n\t   eor r10, r8, r9, lsl #1 \n\t and r9, r8, r9, lsl #1 \n\t   eor r8, r10, r9, lsl #1 \n\t and r9, r10, r9, lsl #1 \n\t      sub r9, r8, %1 \n\t rsb r9, r8, r9 \n\t add %0, r8, r9"
        : "=r" (result) /* Outputs, starts at %0, commma separated list, specified as =r for assigns to a register */
        : "r" (a), "r" (b) /* Inputs, starting at %n, comma separated list, specified as r for intputs from a register */
        : "r8", "r9", "r10" /* Clobbers r8, r9, r10 */
        );
    return result;
}

/*f lsls - assembler to perform 'MOVS result, a, lsl b'
 */
static unsigned int lsls( unsigned int a, unsigned int b )
{
    unsigned int result;
	__asm__ volatile (
        "movs %0, %1, lsl %2"
        : "=r" (result) /* Outputs, starts at %0, commma separated list, specified as =r for assigns to a register */
        : "r" (a), "r" (b) /* Inputs, starting at %n, comma separated list, specified as r for intputs from a register */
        : "r8", "r9" /* Clobbers r8, r9 */
        );
    return result;
}

/*f rrxs - assembler to perform 'MOVS result, a, rrx'
 */
static unsigned int rrxs_inst( unsigned int a, unsigned int b )
{
    unsigned int result;
	__asm__ volatile (
        "movs %0, %1, rrx"
        : "=r" (result) /* Outputs, starts at %0, commma separated list, specified as =r for assigns to a register */
        : "r" (a), "r" (b) /* Inputs, starting at %n, comma separated list, specified as r for intputs from a register */
        : "r8", "r9" /* Clobbers r8, r9 */
        );
    return result;
}

/*f shift - assembler to perform an movs result, a, ror b, but with lsr, lsl, ror, rrx and asr, add
  the basic concept is to generate a ROR b; b must be 1 to 32; we always put bit 'b-1' of a in the carry, bit 31 in N, and set Z properly.
  so the operation is to use an adds operation on a to do a shift left by one, then an rrx to reverse that
  we then rotate right by 31, and then shift right by b, mask with 1, and then rotate once more to get bit 'b' in to bit 31 and in the carry
  we then asr by 31 to get bit b in every bit
  we then add 0 and the carry to get the carry set correctly, and our register to be zero
  add 256+32 and subtract b to get a '32-b'
  now shift a right by 32-b, shift it left by b, and orr together
  eor with a ror 32-b
  eor with a ror b
  movs r,r to set z and n
 */
static unsigned int shift( unsigned int a, unsigned int b )
{
    unsigned int result;
	__asm__ volatile (
        "adds r8, %1, %1 \n\t mov r8, r8, rrx \n\t mov r10, r8, ror #31 \n\t mov r9, #1 \n\t and r9, r9, r10, asr %2 \n\t movs r9, r9, ror #1 \n\t mov r9, r9, asr #31 \n\t adcs r9, r9, #0 \n\t add r9, r9, #0x120 \n\t rsb r9, %2, r9 \n\t mov r10, r8, lsr r9 \n\t orr r10, r10, r8, lsl %r2 \n\t eor r10, r10, %1, ror r9 \n\t eor %0, r10, %1, ror %2 \n\t movs %0, %0"
        : "=r" (result) /* Outputs, starts at %0, commma separated list, specified as =r for assigns to a register */
        : "r" (a), "r" (b) /* Inputs, starting at %n, comma separated list, specified as r for intputs from a register */
        : "r8", "r9", "r10" /* Clobbers r8, r9, r10 */
        );
    return result;
}

/*a Statics test variables
 */
/*v test_vectors - set of tests to perform
 */
static const t_test_vector test_vectors[] = 
{

    //Z  N  C  V            A           B  fn       Z  N  C  V      Result  Message with A, B, Result as arguments
#ifdef TEST_LOGIC 
   { 0, 0, 0, 0,  0x00000000, 0x00000000, logic,   1, 0, 0, 0, 0x00000000, "Logic %08x %08x result %08x\n" }, // 33k cycles to here, 160k cycles just for tests
   { 1, 0, 1, 0,  0x00000000, 0x11111111, logic,   0, 0, 1, 0, 0x11111111, "Logic %08x %08x result %08x\n" },
   { 1, 0, 0, 1,  0x11111111, 0x11111111, logic,   1, 0, 0, 1, 0x11111111, "Logic %08x %08x result %08x\n" },
   { 0, 1, 1, 0,  0x00000000, 0x5a5a5a5a, logic,   0, 0, 1, 0, 0x5a5a5a5a, "Logic %08x %08x result %08x\n" },
   { 1, 0, 0, 0,  0xaaaaaaaa, 0x55555555, logic,   0, 1, 0, 0, 0x55555555, "Logic %08x %08x result %08x\n" },
   { 0, 0, 0, 0,  0x11111111, 0x00000000, logic,   0, 0, 0, 0, 0x00000000, "Logic %08x %08x result %08x\n" },
   { 1, 0, 1, 1,  0x12481248, 0xffffffff, logic,   0, 1, 1, 1, 0xffffffff, "Logic %08x %08x result %08x\n" }, // 174k cycles to here => 20k cycles for each of these tests
#endif

#ifdef TEST_LOGIC_ADD
    { 0, 0, 0, 0,  0x00000000, 0x00000000, logic_add, 0, 0, 0, 0, 0x00000000, "Logic_add %08x %08x result %08x\n" }, // 35k cycles per, 250k cycles just for tests
    { 1, 0, 1, 0,  0x00000000, 0x11111111, logic_add, 1, 0, 1, 0, 0x11111111, "Logic_add %08x %08x result %08x\n" },
    { 1, 0, 0, 1,  0x11111111, 0x11111111, logic_add, 1, 0, 0, 1, 0x11111111, "Logic_add %08x %08x result %08x\n" },
    { 0, 1, 1, 0,  0x00000000, 0x5a5a5a5a, logic_add, 0, 1, 1, 0, 0x5a5a5a5a, "Logic_add %08x %08x result %08x\n" },
    { 1, 0, 0, 0,  0xaaaaaaaa, 0x55555555, logic_add, 1, 0, 0, 0, 0x55555555, "Logic_add %08x %08x result %08x\n" },
    { 0, 0, 0, 0,  0x11111111, 0x00000000, logic_add, 0, 0, 0, 0, 0x00000000, "Logic_add %08x %08x result %08x\n" },
    { 1, 0, 1, 1,  0x12481248, 0xffffffff, logic_add, 1, 0, 1, 1, 0xffffffff, "Logic_add %08x %08x result %08x\n" },
#endif

#ifdef TEST_RRXS
   { 0, 0, 1, 0,  0x55555555, 0x00000020, rrxs_inst,   0, 1, 1, 0, 0xaaaaaaaa, "RRX inst %08x (by %d) (with sign) result %08x\n" }, // 20k per
    { 0, 0, 0, 0,  0x55555555, 0x00000001, rrxs_inst,   0, 0, 1, 0, 0x2aaaaaaa, "RRX inst %08x (by %d) (with sign) result %08x\n" },
    { 0, 0, 1, 0,  0xaaaaaaaa, 0x00000020, rrxs_inst,   0, 1, 0, 0, 0xd5555555, "RRX inst %08x (by %d) (with sign) result %08x\n" },
    { 0, 0, 0, 0,  0xaaaaaaaa, 0x00000001, rrxs_inst,   0, 0, 0, 0, 0x55555555, "RRX inst %08x (by %d) (with sign) result %08x\n" },
#endif

#ifdef TEST_RORS
   { 0, 0, 0, 0,  0x00000000, 0x00000020, rors_inst,   1, 0, 0, 0, 0x00000000, "ROR inst %08x by %d (with sign) result %08x\n" }, // 18k per
    { 0, 0, 0, 0,  0x00000000, 0x00000001, rors_inst,   1, 0, 0, 0, 0x00000000, "ROR inst %08x by %d (with sign) result %08x\n" },
    { 0, 0, 0, 0,  0x00000000, 0x00000002, rors_inst,   1, 0, 0, 0, 0x00000000, "ROR inst %08x by %d (with sign) result %08x\n" },
    { 0, 0, 0, 0,  0xffffffff, 0x00000020, rors_inst,   0, 1, 1, 0, 0xffffffff, "ROR inst %08x by %d (with sign) result %08x\n" },
    { 0, 0, 0, 0,  0xffffffff, 0x00000001, rors_inst,   0, 1, 1, 0, 0xffffffff, "ROR inst %08x by %d (with sign) result %08x\n" },
    { 0, 0, 0, 0,  0xffffffff, 0x00000002, rors_inst,   0, 1, 1, 0, 0xffffffff, "ROR inst %08x by %d (with sign) result %08x\n" },
    { 0, 0, 0, 0,  0x12345678, 0x00000020, rors_inst,   0, 0, 0, 0, 0x12345678, "ROR inst %08x by %d (with sign) result %08x\n" },
    { 0, 0, 0, 0,  0x12345678, 0x0000001c, rors_inst,   0, 0, 0, 0, 0x23456781, "ROR inst %08x by %d (with sign) result %08x\n" },
    { 0, 0, 0, 0,  0x12345678, 0x00000018, rors_inst,   0, 0, 0, 0, 0x34567812, "ROR inst %08x by %d (with sign) result %08x\n" },
    { 0, 0, 0, 0,  0x12345678, 0x00000014, rors_inst,   0, 0, 0, 0, 0x45678123, "ROR inst %08x by %d (with sign) result %08x\n" },
    { 0, 0, 0, 0,  0x12345678, 0x00000010, rors_inst,   0, 0, 0, 0, 0x56781234, "ROR inst %08x by %d (with sign) result %08x\n" },
    { 0, 0, 0, 0,  0x12345678, 0x0000000c, rors_inst,   0, 0, 0, 0, 0x67812345, "ROR inst %08x by %d (with sign) result %08x\n" },
    { 0, 0, 0, 0,  0x12345678, 0x00000008, rors_inst,   0, 0, 0, 0, 0x78123456, "ROR inst %08x by %d (with sign) result %08x\n" },
    { 0, 0, 0, 0,  0x12345678, 0x00000004, rors_inst,   0, 1, 1, 0, 0x81234567, "ROR inst %08x by %d (with sign) result %08x\n" },
    { 0, 0, 0, 0,  0xf0e1d2c3, 0x00000020, rors_inst,   0, 1, 1, 0, 0xf0e1d2c3, "ROR inst %08x by %d (with sign) result %08x\n" },
    { 0, 0, 0, 0,  0xf0e1d2c3, 0x0000001c, rors_inst,   0, 0, 0, 0, 0x0e1d2c3f, "ROR inst %08x by %d (with sign) result %08x\n" },
    { 0, 0, 0, 0,  0xf0e1d2c3, 0x00000018, rors_inst,   0, 1, 1, 0, 0xe1d2c3f0, "ROR inst %08x by %d (with sign) result %08x\n" },
    { 0, 0, 0, 0,  0xf0e1d2c3, 0x00000014, rors_inst,   0, 0, 0, 0, 0x1d2c3f0e, "ROR inst %08x by %d (with sign) result %08x\n" },
    { 0, 0, 0, 0,  0xf0e1d2c3, 0x00000010, rors_inst,   0, 1, 1, 0, 0xd2c3f0e1, "ROR inst %08x by %d (with sign) result %08x\n" },
    { 0, 0, 0, 0,  0xf0e1d2c3, 0x0000000c, rors_inst,   0, 0, 0, 0, 0x2c3f0e1d, "ROR inst %08x by %d (with sign) result %08x\n" },
    { 0, 0, 0, 0,  0xf0e1d2c3, 0x00000008, rors_inst,   0, 1, 1, 0, 0xc3f0e1d2, "ROR inst %08x by %d (with sign) result %08x\n" },
    { 0, 0, 0, 0,  0xf0e1d2c3, 0x00000004, rors_inst,   0, 0, 0, 0, 0x3f0e1d2c, "ROR inst %08x by %d (with sign) result %08x\n" },
#endif

#ifdef TEST_LSRS
    { 0, 0, 0, 0,  0x00000000, 0x00000020, lsrs_inst,   1, 0, 0, 0, 0x00000000, "LSR inst %08x by %d (with sign) result %08x\n" },
    { 0, 0, 0, 0,  0x00000000, 0x00000001, lsrs_inst,   1, 0, 0, 0, 0x00000000, "LSR inst %08x by %d (with sign) result %08x\n" },
    { 0, 0, 0, 0,  0x00000000, 0x00000002, lsrs_inst,   1, 0, 0, 0, 0x00000000, "LSR inst %08x by %d (with sign) result %08x\n" },
    { 0, 0, 0, 0,  0xffffffff, 0x00000020, lsrs_inst,   1, 0, 1, 0, 0x00000000, "LSR inst %08x by %d (with sign) result %08x\n" },
    { 0, 0, 0, 0,  0xffffffff, 0x00000001, lsrs_inst,   0, 0, 1, 0, 0x7fffffff, "LSR inst %08x by %d (with sign) result %08x\n" },
    { 0, 0, 0, 0,  0xffffffff, 0x00000002, lsrs_inst,   0, 0, 1, 0, 0x3fffffff, "LSR inst %08x by %d (with sign) result %08x\n" },
    { 0, 0, 0, 0,  0x12345678, 0x00000020, lsrs_inst,   1, 0, 0, 0, 0x00000000, "LSR inst %08x by %d (with sign) result %08x\n" },
    { 0, 0, 0, 0,  0x12345678, 0x0000001c, lsrs_inst,   0, 0, 0, 0, 0x00000001, "LSR inst %08x by %d (with sign) result %08x\n" },
    { 0, 0, 0, 0,  0x12345678, 0x00000018, lsrs_inst,   0, 0, 0, 0, 0x00000012, "LSR inst %08x by %d (with sign) result %08x\n" },
    { 0, 0, 0, 0,  0x12345678, 0x00000014, lsrs_inst,   0, 0, 0, 0, 0x00000123, "LSR inst %08x by %d (with sign) result %08x\n" },
    { 0, 0, 0, 0,  0x12345678, 0x00000010, lsrs_inst,   0, 0, 0, 0, 0x00001234, "LSR inst %08x by %d (with sign) result %08x\n" },
    { 0, 0, 0, 0,  0x12345678, 0x0000000c, lsrs_inst,   0, 0, 0, 0, 0x00012345, "LSR inst %08x by %d (with sign) result %08x\n" },
    { 0, 0, 0, 0,  0x12345678, 0x00000008, lsrs_inst,   0, 0, 0, 0, 0x00123456, "LSR inst %08x by %d (with sign) result %08x\n" },
    { 0, 0, 0, 0,  0x12345678, 0x00000004, lsrs_inst,   0, 0, 1, 0, 0x01234567, "LSR inst %08x by %d (with sign) result %08x\n" },
    { 0, 0, 0, 0,  0xf0e1d2c3, 0x00000020, lsrs_inst,   1, 0, 1, 0, 0x00000000, "LSR inst %08x by %d (with sign) result %08x\n" },
    { 0, 0, 0, 0,  0xf0e1d2c3, 0x0000001c, lsrs_inst,   0, 0, 0, 0, 0x0000000f, "LSR inst %08x by %d (with sign) result %08x\n" },
    { 0, 0, 0, 0,  0xf0e1d2c3, 0x00000018, lsrs_inst,   0, 0, 1, 0, 0x000000f0, "LSR inst %08x by %d (with sign) result %08x\n" },
    { 0, 0, 0, 0,  0xf0e1d2c3, 0x00000014, lsrs_inst,   0, 0, 0, 0, 0x00000f0e, "LSR inst %08x by %d (with sign) result %08x\n" },
    { 0, 0, 0, 0,  0xf0e1d2c3, 0x00000010, lsrs_inst,   0, 0, 1, 0, 0x0000f0e1, "LSR inst %08x by %d (with sign) result %08x\n" },
    { 0, 0, 0, 0,  0xf0e1d2c3, 0x0000000c, lsrs_inst,   0, 0, 0, 0, 0x000f0e1d, "LSR inst %08x by %d (with sign) result %08x\n" },
    { 0, 0, 0, 0,  0xf0e1d2c3, 0x00000008, lsrs_inst,   0, 0, 1, 0, 0x00f0e1d2, "LSR inst %08x by %d (with sign) result %08x\n" },
    { 0, 0, 0, 0,  0xf0e1d2c3, 0x00000004, lsrs_inst,   0, 0, 0, 0, 0x0f0e1d2c, "LSR inst %08x by %d (with sign) result %08x\n" },
#endif

#ifdef TEST_ASRS
    { 0, 0, 0, 0,  0x00000000, 0x00000020, asrs_inst,   1, 0, 0, 0, 0x00000000, "ASR inst %08x by %d (with sign) result %08x\n" },
    { 0, 0, 0, 0,  0x00000000, 0x00000001, asrs_inst,   1, 0, 0, 0, 0x00000000, "ASR inst %08x by %d (with sign) result %08x\n" },
    { 0, 0, 0, 0,  0x00000000, 0x00000002, asrs_inst,   1, 0, 0, 0, 0x00000000, "ASR inst %08x by %d (with sign) result %08x\n" },
    { 0, 0, 0, 0,  0xffffffff, 0x00000020, asrs_inst,   0, 1, 1, 0, 0xffffffff, "ASR inst %08x by %d (with sign) result %08x\n" },
    { 0, 0, 0, 0,  0xffffffff, 0x00000001, asrs_inst,   0, 1, 1, 0, 0xffffffff, "ASR inst %08x by %d (with sign) result %08x\n" },
    { 0, 0, 0, 0,  0xffffffff, 0x00000002, asrs_inst,   0, 1, 1, 0, 0xffffffff, "ASR inst %08x by %d (with sign) result %08x\n" },
    { 0, 0, 0, 0,  0x12345678, 0x00000020, asrs_inst,   1, 0, 0, 0, 0x00000000, "ASR inst %08x by %d (with sign) result %08x\n" },
    { 0, 0, 0, 0,  0x12345678, 0x0000001c, asrs_inst,   0, 0, 0, 0, 0x00000001, "ASR inst %08x by %d (with sign) result %08x\n" },
    { 0, 0, 0, 0,  0x12345678, 0x00000018, asrs_inst,   0, 0, 0, 0, 0x00000012, "ASR inst %08x by %d (with sign) result %08x\n" },
    { 0, 0, 0, 0,  0x12345678, 0x00000014, asrs_inst,   0, 0, 0, 0, 0x00000123, "ASR inst %08x by %d (with sign) result %08x\n" },
    { 0, 0, 0, 0,  0x12345678, 0x00000010, asrs_inst,   0, 0, 0, 0, 0x00001234, "ASR inst %08x by %d (with sign) result %08x\n" },
    { 0, 0, 0, 0,  0x12345678, 0x0000000c, asrs_inst,   0, 0, 0, 0, 0x00012345, "ASR inst %08x by %d (with sign) result %08x\n" },
    { 0, 0, 0, 0,  0x12345678, 0x00000008, asrs_inst,   0, 0, 0, 0, 0x00123456, "ASR inst %08x by %d (with sign) result %08x\n" },
    { 0, 0, 0, 0,  0x12345678, 0x00000004, asrs_inst,   0, 0, 1, 0, 0x01234567, "ASR inst %08x by %d (with sign) result %08x\n" },
    { 0, 0, 0, 0,  0xf0e1d2c3, 0x00000020, asrs_inst,   0, 1, 1, 0, 0xffffffff, "ASR inst %08x by %d (with sign) result %08x\n" },
    { 0, 0, 0, 0,  0xf0e1d2c3, 0x0000001c, asrs_inst,   0, 1, 0, 0, 0xffffffff, "ASR inst %08x by %d (with sign) result %08x\n" },
    { 0, 0, 0, 0,  0xf0e1d2c3, 0x00000018, asrs_inst,   0, 1, 1, 0, 0xfffffff0, "ASR inst %08x by %d (with sign) result %08x\n" },
    { 0, 0, 0, 0,  0xf0e1d2c3, 0x00000014, asrs_inst,   0, 1, 0, 0, 0xffffff0e, "ASR inst %08x by %d (with sign) result %08x\n" },
    { 0, 0, 0, 0,  0xf0e1d2c3, 0x00000010, asrs_inst,   0, 1, 1, 0, 0xfffff0e1, "ASR inst %08x by %d (with sign) result %08x\n" },
    { 0, 0, 0, 0,  0xf0e1d2c3, 0x0000000c, asrs_inst,   0, 1, 0, 0, 0xffff0e1d, "ASR inst %08x by %d (with sign) result %08x\n" },
    { 0, 0, 0, 0,  0xf0e1d2c3, 0x00000008, asrs_inst,   0, 1, 1, 0, 0xfff0e1d2, "ASR inst %08x by %d (with sign) result %08x\n" },
    { 0, 0, 0, 0,  0xf0e1d2c3, 0x00000004, asrs_inst,   0, 1, 0, 0, 0xff0e1d2c, "ASR inst %08x by %d (with sign) result %08x\n" },
#endif

#ifdef TEST_SHIFT
    { 0, 0, 0, 0,  0x00000000, 0x00000020, shift,   1, 0, 0, 0, 0x00000000, "ROR %08x by %d (with sign) result %08x\n" },
    { 0, 0, 0, 0,  0x00000000, 0x00000001, shift,   1, 0, 0, 0, 0x00000000, "ROR %08x by %d (with sign) result %08x\n" },
    { 0, 0, 0, 0,  0x00000000, 0x00000002, shift,   1, 0, 0, 0, 0x00000000, "ROR %08x by %d (with sign) result %08x\n" },
    { 0, 0, 0, 0,  0xffffffff, 0x00000020, shift,   0, 1, 1, 0, 0xffffffff, "ROR %08x by %d (with sign) result %08x\n" },
    { 0, 0, 0, 0,  0xffffffff, 0x00000001, shift,   0, 1, 1, 0, 0xffffffff, "ROR %08x by %d (with sign) result %08x\n" },
    { 0, 0, 0, 0,  0xffffffff, 0x00000002, shift,   0, 1, 1, 0, 0xffffffff, "ROR %08x by %d (with sign) result %08x\n" },
    { 0, 0, 0, 0,  0x12345678, 0x00000020, shift,   0, 0, 0, 0, 0x12345678, "ROR %08x by %d (with sign) result %08x\n" },
    { 0, 0, 0, 0,  0x12345678, 0x0000001c, shift,   0, 0, 0, 0, 0x23456781, "ROR %08x by %d (with sign) result %08x\n" },
    { 0, 0, 0, 0,  0x12345678, 0x00000018, shift,   0, 0, 0, 0, 0x34567812, "ROR %08x by %d (with sign) result %08x\n" },
    { 0, 0, 0, 0,  0x12345678, 0x00000014, shift,   0, 0, 0, 0, 0x45678123, "ROR %08x by %d (with sign) result %08x\n" },
    { 0, 0, 0, 0,  0x12345678, 0x00000010, shift,   0, 0, 0, 0, 0x56781234, "ROR %08x by %d (with sign) result %08x\n" },
    { 0, 0, 0, 0,  0x12345678, 0x0000000c, shift,   0, 0, 0, 0, 0x67812345, "ROR %08x by %d (with sign) result %08x\n" },
    { 0, 0, 0, 0,  0x12345678, 0x00000008, shift,   0, 0, 0, 0, 0x78123456, "ROR %08x by %d (with sign) result %08x\n" },
    { 0, 0, 0, 0,  0x12345678, 0x00000004, shift,   0, 1, 1, 0, 0x81234567, "ROR %08x by %d (with sign) result %08x\n" },
#endif

#ifdef TEST_LSLS
    { 0, 0, 0, 0,  0x00000000, 0x00000000, lsls,    1, 0, 0, 0, 0x00000000, "LSL %08x by %02x (with sign) result %08x\n" },
    { 0, 0, 1, 0,  0x00000000, 0x00000000, lsls,    1, 0, 1, 0, 0x00000000, "LSL %08x by %02x (with sign) result %08x\n" },
    { 0, 0, 0, 0,  0x00000000, 0x00000001, lsls,    1, 0, 0, 0, 0x00000000, "LSL %08x by %02x (with sign) result %08x\n" },
    { 0, 0, 1, 0,  0x00000000, 0x00000001, lsls,    1, 0, 0, 0, 0x00000000, "LSL %08x by %02x (with sign) result %08x\n" },
    { 0, 0, 0, 0,  0x00000000, 0xfedcba00, lsls,    1, 0, 0, 0, 0x00000000, "LSL %08x by %02x (with sign) result %08x\n" },
    { 0, 0, 1, 0,  0x00000000, 0xfedcba00, lsls,    1, 0, 1, 0, 0x00000000, "LSL %08x by %02x (with sign) result %08x\n" },
    { 0, 0, 0, 0,  0x00000000, 0x00000020, lsls,    1, 0, 0, 0, 0x00000000, "LSL %08x by %02x (with sign) result %08x\n" },
    { 0, 0, 1, 0,  0x00000000, 0x00000020, lsls,    1, 0, 0, 0, 0x00000000, "LSL %08x by %02x (with sign) result %08x\n" },
    { 0, 0, 0, 0,  0x12345678, 0xfedcba00, lsls,    0, 0, 0, 0, 0x12345678, "LSL %08x by %02x (with sign) result %08x\n" },
    { 0, 0, 1, 0,  0x12345678, 0xfedcba00, lsls,    0, 0, 1, 0, 0x12345678, "LSL %08x by %02x (with sign) result %08x\n" },
    { 0, 0, 0, 0,  0x12345678, 0x00000000, lsls,    0, 0, 0, 0, 0x12345678, "LSL %08x by %02x (with sign) result %08x\n" },
    { 0, 0, 1, 0,  0x12345678, 0x00000000, lsls,    0, 0, 1, 0, 0x12345678, "LSL %08x by %02x (with sign) result %08x\n" }, 
    { 0, 0, 0, 0,  0x12345678, 0x00000001, lsls,    0, 0, 0, 0, 0x2468acf0, "LSL %08x by %02x (with sign) result %08x\n" },
    { 0, 0, 1, 0,  0x12345678, 0x00000001, lsls,    0, 0, 0, 0, 0x2468acf0, "LSL %08x by %02x (with sign) result %08x\n" },
    { 0, 0, 1, 0,  0x12345678, 0x00000002, lsls,    0, 0, 0, 0, 0x48d159e0, "LSL %08x by %02x (with sign) result %08x\n" },
    { 0, 0, 1, 0,  0x12345678, 0x00000003, lsls,    0, 1, 0, 0, 0x91a2b3c0, "LSL %08x by %02x (with sign) result %08x\n" },
    { 0, 0, 1, 0,  0x12345678, 0x00000004, lsls,    0, 0, 1, 0, 0x23456780, "LSL %08x by %02x (with sign) result %08x\n" },
    { 0, 0, 1, 0,  0x12345678, 0x00000005, lsls,    0, 0, 0, 0, 0x468acf00, "LSL %08x by %02x (with sign) result %08x\n" },
    { 0, 0, 1, 0,  0x12345678, 0x00000006, lsls,    0, 1, 0, 0, 0x8d159e00, "LSL %08x by %02x (with sign) result %08x\n" },
    { 0, 0, 1, 0,  0x12345678, 0x00000007, lsls,    0, 0, 1, 0, 0x1a2b3c00, "LSL %08x by %02x (with sign) result %08x\n" },
    { 0, 0, 1, 0,  0x12345678, 0x00000008, lsls,    0, 0, 0, 0, 0x34567800, "LSL %08x by %02x (with sign) result %08x\n" },
    { 0, 0, 1, 0,  0x12345678, 0x00000009, lsls,    0, 0, 0, 0, 0x68acf000, "LSL %08x by %02x (with sign) result %08x\n" },
    { 0, 0, 1, 0,  0x12345678, 0x0000000a, lsls,    0, 1, 0, 0, 0xd159e000, "LSL %08x by %02x (with sign) result %08x\n" },
    { 0, 0, 1, 0,  0x12345678, 0x0000000b, lsls,    0, 1, 1, 0, 0xa2b3c000, "LSL %08x by %02x (with sign) result %08x\n" },
    { 0, 0, 1, 0,  0x12345678, 0x0000000c, lsls,    0, 0, 1, 0, 0x45678000, "LSL %08x by %02x (with sign) result %08x\n" },
    { 0, 0, 0, 0,  0x12345678, 0x0000000d, lsls,    0, 1, 0, 0, 0x8acf0000, "LSL %08x by %02x (with sign) result %08x\n" },
    { 0, 0, 1, 0,  0x12345678, 0x0000000e, lsls,    0, 0, 1, 0, 0x159e0000, "LSL %08x by %02x (with sign) result %08x\n" },
    { 0, 0, 1, 0,  0x12345678, 0x0000000f, lsls,    0, 0, 0, 0, 0x2b3c0000, "LSL %08x by %02x (with sign) result %08x\n" },
    { 0, 0, 1, 0,  0x12345678, 0x00000010, lsls,    0, 0, 0, 0, 0x56780000, "LSL %08x by %02x (with sign) result %08x\n" },
    { 0, 0, 1, 0,  0x12345678, 0x00000011, lsls,    0, 1, 0, 0, 0xacf00000, "LSL %08x by %02x (with sign) result %08x\n" },
    { 0, 0, 1, 0,  0x12345678, 0x00000012, lsls,    0, 0, 1, 0, 0x59e00000, "LSL %08x by %02x (with sign) result %08x\n" },
    { 0, 0, 1, 0,  0x12345678, 0x00000013, lsls,    0, 1, 0, 0, 0xb3c00000, "LSL %08x by %02x (with sign) result %08x\n" },
    { 0, 0, 1, 0,  0x12345678, 0x00000014, lsls,    0, 0, 1, 0, 0x67800000, "LSL %08x by %02x (with sign) result %08x\n" },
    { 0, 0, 1, 0,  0x12345678, 0x00000015, lsls,    0, 1, 0, 0, 0xcf000000, "LSL %08x by %02x (with sign) result %08x\n" },
    { 0, 0, 1, 0,  0x12345678, 0x00000016, lsls,    0, 1, 1, 0, 0x9e000000, "LSL %08x by %02x (with sign) result %08x\n" },
    { 0, 0, 1, 0,  0x12345678, 0x00000017, lsls,    0, 0, 1, 0, 0x3c000000, "LSL %08x by %02x (with sign) result %08x\n" },
    { 0, 0, 1, 0,  0x12345678, 0x00000018, lsls,    0, 0, 0, 0, 0x78000000, "LSL %08x by %02x (with sign) result %08x\n" },
    { 0, 0, 1, 0,  0x12345678, 0x00000019, lsls,    0, 1, 0, 0, 0xf0000000, "LSL %08x by %02x (with sign) result %08x\n" },
    { 0, 0, 1, 0,  0x12345678, 0x0000001a, lsls,    0, 1, 1, 0, 0xe0000000, "LSL %08x by %02x (with sign) result %08x\n" },
    { 0, 0, 1, 0,  0x12345678, 0x0000001b, lsls,    0, 1, 1, 0, 0xc0000000, "LSL %08x by %02x (with sign) result %08x\n" },
    { 0, 0, 0, 0,  0x12345678, 0x0000001c, lsls,    0, 1, 1, 0, 0x80000000, "LSL %08x by %02x (with sign) result %08x\n" },
    { 0, 0, 1, 0,  0x12345678, 0x0000001d, lsls,    1, 0, 1, 0, 0x00000000, "LSL %08x by %02x (with sign) result %08x\n" },
    { 0, 0, 1, 0,  0x12345678, 0x0000001e, lsls,    1, 0, 0, 0, 0x00000000, "LSL %08x by %02x (with sign) result %08x\n" },
    { 0, 0, 1, 0,  0x12345678, 0x0000001f, lsls,    1, 0, 0, 0, 0x00000000, "LSL %08x by %02x (with sign) result %08x\n" },
    { 0, 0, 0, 0,  0x12345678, 0x00000020, lsls,    1, 0, 0, 0, 0x00000000, "LSL %08x by %02x (with sign) result %08x\n" },
    { 0, 0, 1, 0,  0x12345678, 0x00000020, lsls,    1, 0, 0, 0, 0x00000000, "LSL %08x by %02x (with sign) result %08x\n" },
#endif

#ifdef TEST_ADD
    { 0, 0, 0, 0,  0x00000000, 0x00000000, add_inst,     0, 0, 0, 0, 0x00000000, "Add %08x %08x result %08x\n" },
    { 0, 1, 1, 1,  0x00000000, 0x00000000, add_inst,     0, 1, 1, 1, 0x00000000, "Add %08x %08x result %08x\n" },
    { 0, 0, 0, 0,  0x11111111, 0xffffffff, add_inst,     0, 0, 0, 0, 0x11111110, "Add %08x %08x result %08x\n" },
    { 0, 1, 1, 1,  0x11111111, 0xffffffff, add_inst,     0, 1, 1, 1, 0x11111110, "Add %08x %08x result %08x\n" },
#endif

#ifdef TEST_ADDS
    { 0, 0, 0, 0,  0x00000000, 0x00000000, adds_inst,    1, 0, 0, 0, 0x00000000, "Adds %08x %08x result %08x\n" },
    { 0, 1, 1, 1,  0x00000000, 0x00000000, adds_inst,    1, 0, 0, 0, 0x00000000, "Adds %08x %08x result %08x\n" },
    { 0, 0, 0, 0,  0x11111111, 0xffffffff, adds_inst,    0, 0, 1, 0, 0x11111110, "Adds %08x %08x result %08x\n" },
    { 0, 1, 1, 1,  0x11111111, 0xffffffff, adds_inst,    0, 0, 1, 0, 0x11111110, "Adds %08x %08x result %08x\n" },
    { 0, 1, 1, 1,  0x00000001, 0xffffffff, adds_inst,    1, 0, 1, 0, 0x00000000, "Adds %08x %08x result %08x\n" },
    { 0, 1, 1, 1,  0x40000000, 0x40000000, adds_inst,    0, 1, 0, 1, 0x80000000, "Adds %08x %08x result %08x\n" },
    { 0, 1, 1, 1,  0xc0000000, 0xc0000000, adds_inst,    0, 1, 1, 0, 0x80000000, "Adds %08x %08x result %08x\n" },
    { 0, 1, 1, 1,  0xc0000000, 0xc0000001, adds_inst,    0, 1, 1, 0, 0x80000001, "Adds %08x %08x result %08x\n" },
    { 0, 1, 1, 1,  0xc0000000, 0xbfffffff, adds_inst,    0, 0, 1, 1, 0x7fffffff, "Adds %08x %08x result %08x\n" },
#endif

#ifdef TEST_CMP
    { 0, 0, 0, 0,  0x00000000, 0x00000000, cmp_inst,     1, 0, 1, 0, 0x00000000, "Cmp %08x %08x result %08x\n" },
    { 0, 1, 1, 1,  0x00000000, 0x00000000, cmp_inst,     1, 0, 1, 0, 0x00000000, "Cmp %08x %08x result %08x\n" },
    { 0, 0, 0, 0,  0x11111111, 0xffffffff, cmp_inst,     0, 0, 0, 0, 0x11111111, "Cmp %08x %08x result %08x\n" },
    { 0, 1, 1, 1,  0x11111111, 0xffffffff, cmp_inst,     0, 0, 0, 0, 0x11111111, "Cmp %08x %08x result %08x\n" },
    { 0, 1, 1, 1,  0x00000001, 0xffffffff, cmp_inst,     0, 0, 0, 0, 0x00000001, "Cmp %08x %08x result %08x\n" },
    { 0, 1, 1, 1,  0x40000000, 0x40000000, cmp_inst,     1, 0, 1, 0, 0x40000000, "Cmp %08x %08x result %08x\n" },
    { 0, 1, 1, 1,  0xc0000000, 0xc0000000, cmp_inst,     1, 0, 1, 0, 0xc0000000, "Cmp %08x %08x result %08x\n" },
    { 0, 1, 1, 1,  0xc0000000, 0xc0000001, cmp_inst,     0, 1, 0, 0, 0xc0000000, "Cmp %08x %08x result %08x\n" },
    { 0, 1, 1, 1,  0xc0000000, 0xbfffffff, cmp_inst,     0, 0, 1, 0, 0xc0000000, "Cmp %08x %08x result %08x\n" },
#endif

    { 0, 0, 0, 0,  0x00000000, 0x00000000, NULL,    0, 0, 0, 0, 0x00000000, "ZZZ" },
};

/*a Test entry point
 */
/*f external alu_call
 */
extern void alu_call( t_test_vector *test_vector, t_test_vector_result *result );

/*f test_entry_point
 */
extern int test_entry_point()
{
    int i, j;
    t_test_vector_result result;
    int failure;
    failure = 0;
    for (i=0; test_vectors[i].alu_fn; i++)
    {
        dprintf( test_vectors[i].message, test_vectors[i].a_in, test_vectors[i].b_in, test_vectors[i].result );
        alu_call( test_vectors+i, &result );
        SIM_VERBOSE_ON;
        j = 0;
        if (test_vectors[i].zero_out!=result.zero_out)         j |= 1;
        if (test_vectors[i].negative_out!=result.negative_out) j |= 2;
        if (test_vectors[i].carry_out!=result.carry_out)       j |= 4;
        if (test_vectors[i].overflow_out!=result.overflow_out) j |= 8;
        if (test_vectors[i].result!=result.result)             j |= 16;
        SIM_VERBOSE_OFF;
        if (j!=0)
        {
            dprintf( "Test %d failed: result %08x, error type %02x\n\n\n", i, result.result, j );
            if (!failure)
            {
                failure = ((j<<24)|i);
            }
        }
    }
    dprintf( "ALU tests complete, failures %d", failure, 0, 0 );
    return failure;
}
