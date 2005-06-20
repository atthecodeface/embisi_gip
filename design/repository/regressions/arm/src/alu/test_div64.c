/*a Includes
 */
#include <stdlib.h>
#include <stdio.h>
#include "../common/wrapper.h"
#include "gip_support.h"

/*a Defines
 */

/*a Types
 */

/*a Test entry point
 */
/*f external div64
 */
extern void div64( void );

/* Taken from Linux source...
   We're not 64-bit, but... */
#define do_div(n,base)                                          \
({                                                              \
        register int __res asm("r2") = base;                    \
        register unsigned long long __n asm("r0") = n;          \
        asm("bl div64"                                       \
                : "=r" (__n), "=r" (__res)                      \
                : "0" (__n), "1" (__res)                        \
                : "r3", "ip", "lr", "cc");                      \
        n = __n;                                                \
        __res;                                                  \
})

static int do_test( unsigned long long x, unsigned int base )
{
    unsigned int result;

    GIP_LED_OUTPUT_CFG_WRITE( 0xaaff );

    GIP_EXTBUS_DATA_WRITE( ((unsigned int *)(&x))[0] ); NOP; NOP; NOP; NOP; NOP; NOP;
    GIP_EXTBUS_DATA_WRITE( ((unsigned int *)(&x))[1] ); NOP; NOP; NOP; NOP; NOP; NOP;
    GIP_EXTBUS_DATA_WRITE( base ); NOP; NOP; NOP; NOP; NOP; NOP;

    result = do_div( x, base );

    GIP_EXTBUS_DATA_WRITE( ((unsigned int *)(&x))[0] ); NOP; NOP; NOP; NOP; NOP; NOP;
    GIP_EXTBUS_DATA_WRITE( ((unsigned int *)(&x))[1] ); NOP; NOP; NOP; NOP; NOP; NOP;
    GIP_EXTBUS_DATA_WRITE( result ); NOP; NOP; NOP; NOP; NOP; NOP;

    GIP_LED_OUTPUT_CFG_WRITE( 0xaaaa );

    return 1;
}

/*f test_entry_point
 */
extern int test_entry_point()
{
    int failures;
    GIP_EXTBUS_CONFIG_WRITE( 0x111 );
    NOP;NOP;NOP;NOP;
    GIP_EXTBUS_ADDRESS_WRITE( 0xc0000002 ); // Debug display address
    NOP;NOP;NOP;NOP;

//    failures += !do_test( 0x123, 1 );
//    failures += !do_test( 0x123, 2 );
//    failures += !do_test( 0x123, 3 );
//    failures += !do_test( 0x123000000LL, 0x600000 );
    failures += !do_test( 0x7fffffff, 0x1234567 );

    return 0;
}
