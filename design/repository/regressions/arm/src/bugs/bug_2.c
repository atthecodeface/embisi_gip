#include <stdarg.h>
#include "../microkernel/microkernel.h"
typedef int size_t;

/*
This bug stems from the microkernel not running when the SWI is kicked off
It fails the first time after reset, but thereafter it works fine because the kernel is running as thread 1...
Also, at reset the threads are all set to start off there semaphore 0 - so even without a microkernel loaded, thread 1 wants to kick off.
The bug does NOT appear if the simulation is mismanaging the 'deschedule' - this mean that simulation was not mirroring emulation, as 'deschedle' in the decode was combinatorial and this was not expressed properly.
Presumably in these cases 'deschedule' just never happens.
 */
#define utx(a) {__asm__ volatile ("  .word 0xec00c40e \n mov r0, %0" :  : "r" (0x16000096 | (a<<8)) );}
static char printk_buf[1024];

static int vsnprintf(char *buf, size_t size, const char *fmt, va_list args)
{
	char *str, *end;

    __asm__ volatile (" mov lr, #0x11000 ; stmia lr, {r0-r15} ; ldmia lr, {r0-r14} " : : : "lr" );
    utx('+');
	str = buf;
	end = buf + size - 1;

	if (end < buf - 1) {
		end = ((void *) -1);
		size = end - buf + 1;
	}

    utx('#');
	for (; *fmt ; ++fmt) {
//        utx('.'); // this does not seem to help...!

		if (*fmt != '%') {
			if (str <= end)
				*str = *fmt;
			++str;
			continue;
		}
	}
    utx('*');
	if (str <= end)
		*str = '\0';
	else if (size > 0)
		/* don't write out a null byte if the buf size is zero */
		*end = '\0';
	/* the trailing null byte doesn't count towards the total
	* ++str;
	*/
    utx('^');
    __asm__ volatile (" mov lr, #0x11000 ; add lr, lr, #0x40 ; stmia lr, {r0-r15} ; ldmia lr, {r0-r14} ; mov sp, #0 ; mov pc, #0 " : : : "lr" );
	return str-buf;
}

static int zero=0;
static int printk(const char *fmt, ...)
{
	va_list args;
	unsigned long flags;
	int printed_len;

    utx('A');
    if (zero) {
        zero = 1;
        __asm__ volatile( "mov r0, r0 ; mov r1, r1 ; " );
    }
    GIP_ANALYZER_WRITE(0,2); GIP_BLOCK_ALL();// enable analyzer
//    __asm__ volatile( "mov r0, #0x9900 ; ldr r0, [r0] ; .word 0xec007281" : : : "r0" ); // trigger the analyzer
    __asm__ volatile( "mov r0, #0 ; ldr r0, [r0] ; .word 0xec007281" : : : "r0" ); // trigger the analyzer
    utx('B');
	/* This stops the holder of console_sem just where we want him */
//    { { __asm__ volatile ( " mov r0, r0 ; mov %0, r0 " : "=r" (flags) : : "r0" ); } ; flags=!(flags&0x20);} // works on new avnet
//    { { __asm__ volatile ( " swi 0xe00000 ; mov %0, r0 " : "=r" (flags) : : "r0" ); } ; flags=!(flags&0x20);} // works on new avnet
//    { { __asm__ volatile ( " swi 0xef0000 ; mov %0, r0 " : "=r" (flags) : : "r0" ); } ; flags=!(flags&0x20);} // works on new avnet
//    { { __asm__ volatile ( " swi 0x600000 ; mov %0, r0 " : "=r" (flags) : : "r0" ); } ; flags=!(flags&0x20);} // works on new avnet
//    { { __asm__ volatile ( " swi 0xa00000 ; mov %0, r0 " : "=r" (flags) : : "r0" ); } ; flags=!(flags&0x20);} // works on new avnet
    { { __asm__ volatile ( " swi 0x200000 ; mov %0, r0 " : "=r" (flags) : : "r0" ); } ; flags=!(flags&0x20);} // works on new avnet - this failed in regression - yay!
//    { { __asm__ volatile ( " swi 0x260000 ; mov %0, r0 " : "=r" (flags) : : "r0" ); } ; flags=!(flags&0x20);} // works on new avnet
//    { { __asm__ volatile ( " swi 0x200000 ; mov %0, r0 " : "=r" (flags) : : "r0" ); } ; flags=!(flags&0x20);} // did not appear to work

	/* Emit the output into the temporary buffer */
	va_start(args, fmt);
    utx('C');
	printed_len = vsnprintf(printk_buf, sizeof(printk_buf), fmt, args);
    utx('D');
	va_end(args);
    utx('E');
    zero = flags;
    return 0;
}

/*f test_entry_point
 */
static char *text = "hello world\n";
extern int test_entry_point()
{
    int i;
    char *dest;
    __asm__ volatile( "mov sp, #0x80000000 ; orr sp, sp, #0x00310000 ; orr sp, sp, #0x00008000" );
    dest = 0x80220000;
    for (i=0; i<((char *)test_entry_point)-((char *)vsnprintf); i++)
    {
        dest[i] = ((char *)vsnprintf) [i];
    }
    __asm__ volatile( "mov r0, %0 ; mov lr, pc ; mov pc, %1 ; mov pc, #0" : : "r" (text), "r" (0x80220000 + (((char *)printk)-((char *)vsnprintf))) );
    return 0;
}

