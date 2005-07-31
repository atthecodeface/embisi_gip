/*a Includes
 */
#include <stdlib.h>
#include <stdio.h>
#include "../common/wrapper.h"
#include "gip_support.h"

/*a Defines
 */
typedef enum
{
    analyzer_action_idle,                           // H Store nothing, and do not touch counter
    analyzer_action_reset_counter,                  // L Store nothing, and reset counter to 1, stay in current stage
    analyzer_action_store_signal_and_transition,    // M Store signal value and transition immediately
    analyzer_action_store_signal_and_reside,        // H Store signal value and transition if counter matches residence time
    analyzer_action_store_time_and_reside,          // L Store time of occurrence (cycles since trigger_reset went away) and trigger signals, transition if counter matches residence time
    analyzer_action_store_time_and_transition,      // H Store time of occurrence (cycles since trigger_reset went away) and trigger signals and transition
    analyzer_action_store_residence_and_transition, // M Store residence time and transition
    analyzer_action_end,                            // H Store nothing, disable trace
} t_analyzer_action;
#define GIP_ANALYZER_WRITE(t,s) {__asm__ volatile ( " .word 0xec00c20e+" #t "<<4 \n mov r0, %0 \n" : : "r" (s) ); NOP; GIP_BLOCK_ALL(); NOP; }
#define GIP_ANALYZER_READ_CONTROL(s)  {__asm__ volatile ( " .word 0xec00ce02 \n mov %0, r0 \n " : "=r" (s) ); }
#define GIP_ANALYZER_READ_DATA(s)  {__asm__ volatile ( " .word 0xec00ce02 \n mov %0, r1 \n " : "=r" (s) ); }
#define GIP_ANALYZER_TRIGGER_CONTROL(count,stage_if_true,action_if_true,stage_if_false,action_if_false) { GIP_ANALYZER_WRITE(1, ( (count)<<0) | ((stage_if_true)<<16) | ((action_if_true)<<20) | ((stage_if_false)<<24) | ((action_if_false)<<28)); }
#define GIP_ANALYZER_TRIGGER_MASK(mask) {GIP_ANALYZER_WRITE(2,(mask));}
#define GIP_ANALYZER_TRIGGER_COMPARE(mask) {GIP_ANALYZER_WRITE(3,(mask));}

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
        GIP_ANALYZER_READ_CONTROL(s); // read back control - resets should be 1, enables 0, readback 0, valid 0, done 0
        if ((s&0xff)!=0x11) break;
        failure++;

        GIP_ANALYZER_WRITE( 4, 0 ); // set mux control to 0 (gmr, gmw, gmbe[4;0], gma[26;0])
        GIP_ANALYZER_WRITE( 0, 1 | (0<<8) ); // trigger stage 0
        GIP_ANALYZER_TRIGGER_CONTROL( 1, 1, analyzer_action_store_signal_and_reside, 0, analyzer_action_idle ); // capture read of the trigger fn address
        GIP_ANALYZER_TRIGGER_MASK(    0x83ffffff ); // read of address
        GIP_ANALYZER_TRIGGER_COMPARE( 0x80000000 | ((unsigned int)(analyzer_trigger_fn)) );

        GIP_ANALYZER_WRITE( 0, 1 | (1<<8) ); // trigger stage 1
        GIP_ANALYZER_TRIGGER_CONTROL( 64, 2, analyzer_action_store_signal_and_reside, 0, analyzer_action_idle ); // capture 64 more words, then go to stage 2
        GIP_ANALYZER_TRIGGER_MASK(    0 );
        GIP_ANALYZER_TRIGGER_COMPARE( 0 );

        GIP_ANALYZER_WRITE( 0, 1 | (2<<8) ); // trigger stage 2
        GIP_ANALYZER_TRIGGER_CONTROL( 64, 2, analyzer_action_end, 0, analyzer_action_idle ); // end
        GIP_ANALYZER_TRIGGER_MASK(    0 );
        GIP_ANALYZER_TRIGGER_COMPARE( 0 );

        GIP_ANALYZER_WRITE( 0, 0 ); // trigger reset removed
        GIP_ANALYZER_READ_CONTROL(s); // read back control - resets should be 0, enables 0, readback 0, valid 0, done 0
        if ((s&0xff)!=0x00) break;
        failure++;

        GIP_ANALYZER_WRITE( 0, 2 ); // trigger enable
        GIP_ANALYZER_READ_CONTROL(s); // read back control - resets should be 0, enables 1, readback 0, valid 0, done 0
        if ((s&0xff)!=0x22) break;
        failure++;

        analyzer_trigger_fn(0);

        GIP_ANALYZER_READ_CONTROL(s); // read back control - resets should be 0, enables 1, readback 0, valid 0, done 0; trigger stage ought to be 1 (capturing 64 reads)
        if ((s&0xffff)!=0x0122) break;
        failure++;

        { int i; for (i=0; i<10; i++) NOP; } // wait to ensure we capture 64 reads

        GIP_ANALYZER_READ_CONTROL(s); // read back control - resets should be 0, apb_enables 1, trigger_enable 0, readback 0, valid 0, done 1; trigger stage will be 0 because it is no longer enabled
        if ((s&0xffff)!=0x0042) break;
        failure++;

        GIP_ANALYZER_WRITE( 0, 4 ); // trigger readback, clear enable too (its already done)
        GIP_BLOCK_ALL(); NOP ; GIP_BLOCK_ALL(); // this should clear us until the readback has actually occurred
        GIP_ANALYZER_READ_CONTROL(s); // read back control - resets should be 0, apb_enables 1, trigger_enable 0, readback 1, valid 1, done 1; trigger stage will be 0
        if ((s&0xffff)!=0x004c) break;
        failure++;

        // now read back 65 values, starting with analyzer_trigger_fn
        for (i=0; i<65; i++)
        {
            GIP_ANALYZER_READ_CONTROL(s); // read back control - resets should be 0, apb_enables 0, trigger_enable 0, readback 1, valid 1, done 1; trigger stage will be 0
            if ((s&0xffff)!=0x004c) break;
            GIP_ANALYZER_READ_DATA(s);
            if ((i==0) && (s!=(0x80000000 | ((unsigned int)(analyzer_trigger_fn)))))
            {
                break;
            }
        }
        if (i<65) break;
        failure++;

        //Now check its not ready -- there should only have been 65 values
        GIP_ANALYZER_READ_CONTROL(s); // read back control - resets should be 0, apb_enable 0, trigger_enable 0, readback 1, valid 0, done 1; trigger stage will be 0
        if ((s&0xffff)!=0x0044) break;

        failure = 0;
    } while (0);
    return failure;
}
