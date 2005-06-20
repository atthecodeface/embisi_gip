/*a Includes
 */
#include <stdlib.h>
#include <stdio.h>
#include "../common/wrapper.h"
#include "gip_support.h"

/*a Types
 */

/*a Statics
 */

/*a Main test entry
 */
static void dummy( void )
{
    __asm__ volatile (".word 0 ; .word 1 ; .word 2");
}

/*f test_entry_point
 */
extern int test_entry_point()
{
    int i;
    unsigned int *run_count = (unsigned int *)dummy;
    (*run_count)++;
    for (i=0; i<300; i++) NOP; // 150 steps takes us to 64ns
    if ((*run_count)==2)
    {
        return 0;
    }
    return (*run_count)+1000;
}
