/*a Includes
 */
#include <stdlib.h>
#include <stdio.h>
#include "../common/wrapper.h"

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


#define uart_tx(a) { __asm__ volatile (" mov r8, %0, lsl #8 \n orr r8, r8, #0xa6 \n .word 0xec00c40e \n mov r8, r8" :  : "r" (a) : "r8" ); } // extrdrm (c): cddm rd is (3.5 type 2 reg 0) (m is top 3.1 - type of 7 for no override)

#if (0)
#define SIM_DUMP_VARS   {__asm__ volatile ( ".word 0xf00000a1" : : );}
#define SIM_VERBOSE_ON  {__asm__ volatile ( ".word 0xf00000a2" : : );}
#define SIM_VERBOSE_OFF {__asm__ volatile ( ".word 0xf00000a3" : : );}
#else
#define SIM_DUMP_VARS
#define SIM_VERBOSE_ON
#define SIM_VERBOSE_OFF
#endif

/*a Test entry point
 */
/*f test_entry_point
 */
extern int test_entry_point()
{
    int i;
    static const char text[] = "A test message from the UART\n";

    for (i=0; text[i]; i++)
    {
        uart_tx( text[i] );
    }
    uart_tx( 0 );
    uart_tx( 0 );

    for (i=0; i<1000; i++);

}
