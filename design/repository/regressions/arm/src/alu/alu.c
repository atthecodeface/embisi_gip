#include <stdlib.h>
#include <stdio.h>

typedef unsigned int (*t_alu_fn)(unsigned int a, unsigned int b );

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
} t_test_vector;

typedef struct t_test_vector_result
{
    int zero_out;
    int negative_out;
    int carry_out;
    int overflow_out;
    unsigned int result;
} t_test_vector_result;

#define joined(a) a
#define join(a,b) joined(a##b)
#define ALU3_ASM(inst) \
            static unsigned int inst ( unsigned int a, unsigned int b ) { register unsigned int result;\
            __asm__ ( #inst " %0, %1, %2" : "=r" (result) : "r" (a), "r" (b) ); return result; }

#define ALU2_ASM(inst) \
            static unsigned int inst ( unsigned int a, unsigned int b ) { register unsigned int result; \
            __asm__ ( #inst " %1, %2" : "=r" (result) : "r" (a), "r" (b) ); return result; }


static unsigned int add( unsigned int a, unsigned int b )
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

ALU3_ASM(adds)
ALU2_ASM(cmp)


static t_test_vector test_vectors[] = 
{
    { 0, 0, 0, 0,  0x00000000, 0x00000000, add,     0, 0, 0, 0, 0x00000000 },
    { 0, 1, 1, 1,  0x00000000, 0x00000000, add,     0, 1, 1, 1, 0x00000000 },
    { 0, 0, 0, 0,  0x11111111, 0xffffffff, add,     0, 0, 0, 0, 0x11111110 },
    { 0, 1, 1, 1,  0x11111111, 0xffffffff, add,     0, 1, 1, 1, 0x11111110 },

    { 0, 0, 0, 0,  0x00000000, 0x00000000, adds,    1, 0, 0, 0, 0x00000000 },
    { 0, 1, 1, 1,  0x00000000, 0x00000000, adds,    1, 0, 0, 0, 0x00000000 },
    { 0, 0, 0, 0,  0x11111111, 0xffffffff, adds,    0, 0, 1, 0, 0x11111110 },
    { 0, 1, 1, 1,  0x11111111, 0xffffffff, adds,    0, 0, 1, 0, 0x11111110 },
    { 0, 1, 1, 1,  0x00000001, 0xffffffff, adds,    1, 0, 1, 0, 0x00000000 },
    { 0, 1, 1, 1,  0x40000000, 0x40000000, adds,    0, 1, 0, 1, 0x80000000 },
    { 0, 1, 1, 1,  0xc0000000, 0xc0000000, adds,    0, 1, 1, 0, 0x80000000 },
    { 0, 1, 1, 1,  0xc0000000, 0xc0000001, adds,    0, 1, 1, 0, 0x80000001 },
    { 0, 1, 1, 1,  0xc0000000, 0xbfffffff, adds,    0, 0, 1, 1, 0x7fffffff },

    { 0, 0, 0, 0,  0x00000000, 0x00000000, cmp,     1, 0, 1, 0, 0x00000000 },
    { 0, 1, 1, 1,  0x00000000, 0x00000000, cmp,     1, 0, 1, 0, 0x00000000 },
    { 0, 0, 0, 0,  0x11111111, 0xffffffff, cmp,     0, 0, 0, 0, 0x11111111 },
    { 0, 1, 1, 1,  0x11111111, 0xffffffff, cmp,     0, 0, 0, 0, 0x11111111 },
    { 0, 1, 1, 1,  0x00000001, 0xffffffff, cmp,     0, 0, 0, 0, 0x00000001 },
    { 0, 1, 1, 1,  0x40000000, 0x40000000, cmp,     1, 0, 1, 0, 0x40000000 },
    { 0, 1, 1, 1,  0xc0000000, 0xc0000000, cmp,     1, 0, 1, 0, 0xc0000000 },
    { 0, 1, 1, 1,  0xc0000000, 0xc0000001, cmp,     0, 1, 0, 0, 0xc0000000 },
    { 0, 1, 1, 1,  0xc0000000, 0xbfffffff, cmp,     0, 0, 1, 0, 0xc0000000 },

    { 0, 0, 0, 0,  0x00000000, 0x00000000, NULL,    0, 0, 0, 0, 0x00000000 },
};

extern void alu_call( t_test_vector *test_vector, t_test_vector_result *result );

extern int test_entry_point()
{
    int i;
    t_test_vector_result result;
    for (i=0; test_vectors[i].alu_fn; i++)
    {
        alu_call( test_vectors+i, &result );
        if (test_vectors[i].zero_out!=result.zero_out)         return ((i<<4)|0x1);
        if (test_vectors[i].negative_out!=result.negative_out) return ((i<<4)|0x2);
        if (test_vectors[i].carry_out!=result.carry_out)       return ((i<<4)|0x3);
        if (test_vectors[i].overflow_out!=result.overflow_out) return ((i<<4)|0x4);
        if (test_vectors[i].result!=result.result)             return ((i<<4)|0x5);
    }
    return 0;
}
