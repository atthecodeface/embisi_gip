/*a Includes
 */
#include "cmd.h"
#include "gip_support.h"

/*a Defines
 */
#define DELAY(a) {int i;for (i=0; i<a; i++) __asm__ volatile("mov r0, r0");}

/*a Test entry point
 */
/*f test_entry_point
 */
extern int test_entry_point()
{
    int i;
    GIP_LED_OUTPUT_CFG_WRITE(0);
    for (i=0; i<100; i++)
    {
        GIP_LED_OUTPUT_CFG_WRITE(0x3000); // set bit 7 to direction (1 is inc), bit 6 to make it go; then clear bit 6
        GIP_BLOCK_ALL();
        GIP_LED_OUTPUT_CFG_WRITE(0); // Clear psen
        GIP_BLOCK_ALL();
        DELAY(10000);
    }
    __asm__ volatile ("mov pc, #0");
    return 0;
}
