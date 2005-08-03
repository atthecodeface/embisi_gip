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

/*a Statics test variables
 */

/*a Test entry point
 */
/*f analyzer_trigger_fn
 */
static void analyzer_trigger_fn( int code )
{
    return;
}
static int analyzer_trigger_static;

/*f test_entry_point
 */
extern int test_entry_point()
{
    unsigned int s;
    int i;

    int failure = 1;
    do
    {
        GIP_ANALYZER_WRITE( 0, 1 ); // reset the trigger, disable it, set stage to 0
        GIP_ANALYZER_WRITE( 4, 0 ); // set mux control to 0 (gmr, gmw, gmbe[4;0], gma[26;0])

        GIP_ANALYZER_WRITE( 0, 1 | (0<<8) ); // trigger stage 0
        GIP_ANALYZER_TRIGGER_CONTROL( 16, 1, analyzer_action_store_signal_and_reside, 0, analyzer_action_idle ); // capture read/write of the trigger static address, else
        GIP_ANALYZER_TRIGGER_MASK(    0x03ffffff ); // read/write of our static location
        GIP_ANALYZER_TRIGGER_COMPARE( 0xc0000000 | (((unsigned int)(&analyzer_trigger_static))&0x3ffffff) );

        GIP_ANALYZER_WRITE( 0, 1 | (1<<8) ); // trigger stage 1
        GIP_ANALYZER_TRIGGER_CONTROL( 64, 2, analyzer_action_end, 0, analyzer_action_idle ); // end
        GIP_ANALYZER_TRIGGER_MASK(    0 );
        GIP_ANALYZER_TRIGGER_COMPARE( 0 );

        GIP_ANALYZER_WRITE( 0, 0 ); // trigger reset removed
        GIP_ANALYZER_WRITE( 0, 2 ); // trigger enable
        GIP_ANALYZER_READ_CONTROL(s); // read back control - resets should be 0, enables 1, readback 0, valid 0, done 0
        if ((s&0xff)!=0x22) break;
        failure++;

        // now read
        for (i=0; i<8; i++)
        {
            GIP_ANALYZER_READ_CONTROL(s); // read back control - residence should be 1 for i=0, 3 for i=1, etc
            if ((s>>16)!=1+2*i) break;
            failure++;
            analyzer_trigger_static = 1;
            __asm__ volatile ("" : : : "memory"); // makes memory volatile

            GIP_ANALYZER_READ_CONTROL(s); // read back control - residence should be 2 for i=0, 4 for i=1, etc
            if ((s>>16)!=2+2*i) break;
            failure++;
            __asm__ volatile ("mov %0, %0" : : "r" (analyzer_trigger_static) : "memory" ); // makes memory volatile
        }

        GIP_ANALYZER_READ_CONTROL(s); // read back control - resets should be 0, apb_enables 1, trigger_enable 0, readback 0, valid 0, done 1; trigger stage will be 0 because it is no longer enabled
        if ((s&0xffff)!=0x0042) break;
        failure++;

        GIP_ANALYZER_WRITE( 0, 4 ); // trigger readback, clear enable too (its already done)
        GIP_BLOCK_ALL(); NOP ; GIP_BLOCK_ALL(); // this should clear us until the readback has actually occurred
        GIP_ANALYZER_READ_CONTROL(s); // read back control - resets should be 0, apb_enables 1, trigger_enable 0, readback 1, valid 1, done 1; trigger stage will be 0
        if ((s&0xffff)!=0x004c) break;
        failure++;

        // now read back 16 values, starting with analyzer_trigger_fn
        for (i=0; i<16; i++)
        {
            GIP_ANALYZER_READ_CONTROL(s); // read back control - resets should be 0, apb_enables 0, trigger_enable 0, readback 1, valid 1, done 1; trigger stage will be 0
            if ((s&0xffff)!=0x004c) break;
            failure++;
            GIP_ANALYZER_READ_DATA(s);
            if ( (~i&1) && (s!=(0x7c000000 | (((unsigned int)(&analyzer_trigger_static))&0x3ffffff))) )
            {
                break;
            }
            if ( (i&1) && (s!=(0x80000000 | (((unsigned int)(&analyzer_trigger_static))&0x3ffffff))) )
            {
                break;
            }
            failure++;
        }
        if (i<16) break;
        failure++;

        //Now check its not ready -- there should only have been 65 values
        GIP_ANALYZER_READ_CONTROL(s); // read back control - resets should be 0, apb_enable 0, trigger_enable 0, readback 1, valid 0, done 1; trigger stage will be 0
        if ((s&0xffff)!=0x0044) break;

        failure = 0;
    } while (0);
    dprintf( "Analyzer test 2 complete, failure (0=>pass) %d", failure, 0, 0 );
    return failure;
}
