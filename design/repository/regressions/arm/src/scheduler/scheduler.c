/*a Includes
 */
#include "../common/wrapper.h"

/*a Defines
 */
#define NOP { __asm__ volatile(" movnv r0, r0"); }
#define NOP_WRINT { NOP; NOP; NOP; }
static void nop_many( void ) { NOP; NOP; NOP; NOP;    NOP; NOP; NOP; NOP;    NOP; NOP; NOP; NOP;    NOP; NOP; NOP; NOP;    NOP; NOP; NOP; NOP;    NOP; NOP; NOP; NOP;    NOP; NOP; NOP; NOP;   NOP; NOP; NOP; NOP; }
#define NOP_WREXT { nop_many(); }
#define FLASH_CONFIG_READ(v) { __asm__ volatile (" .word 0xec00ce04 \n mov %0, r8" : "=r" (v) ); }
#define FLASH_CONFIG_WRITE(v) { __asm__ volatile (" .word 0xec00c48e \n mov r8, %0" : : "r" (v) ); NOP_WRINT; }
#define FLASH_ADDRESS_READ(v) { __asm__ volatile (" .word 0xec00ce04 \n mov %0, r10" : "=r" (v) ); }
#define FLASH_ADDRESS_WRITE(v) { __asm__ volatile (" .word 0xec00c4ae \n mov r8, %0" : : "r" (v) ); NOP_WRINT; }
#define FLASH_DATA_READ(v) { __asm__ volatile (" .word 0xec00ce04 \n mov %0, r9" : "=r" (v) ); }
#define FLASH_DATA_WRITE( v ) { __asm__ volatile (" .word 0xec00c49e \n mov r8, %0" : : "r" (v) ); NOP_WREXT; }

#define SET_THREAD(thread,pc,data) { unsigned int s_pc=((unsigned int)(pc))|1, s_data=((unsigned int)(data))|0x100; __asm__ volatile (".word 0xec00c84e \n mov r0, %0 \n .word 0xec00c85e \n mov r0, %1 \n .word 0xec00c86e \n mov r0, %2 " : : "r" (thread), "r" (s_pc), "r" (s_data) ); }
#define DESCHEDULE() {__asm__ volatile (".word 0xec007401" );}
#define SEM_SET(s) { unsigned int s_s=(unsigned int)(1<<(s)); __asm__ volatile ( " .word 0xec00c81e \n mov r0, %0 \n" : : "r" (s_s) ); }
#define SEM_CLR(s) { unsigned int s_s=(unsigned int)(1<<(s)); __asm__ volatile ( " .word 0xec00c82e \n mov r0, %0 \n" : : "r" (s_s) ); }

/*a Test entry point
 */
static void thread_1_entry( void )
{
    SEM_CLR(7);
    DESCHEDULE();
}

static void thread_2_entry( void )
{
    SEM_CLR(8);
    DESCHEDULE();
}

static void thread_3_entry( void )
{
    SEM_CLR(12);
    DESCHEDULE();
}

static void thread_4_entry( void )
{
    SEM_CLR(16);
    DESCHEDULE();
}

static void thread_5_entry( void )
{
    SEM_CLR(20);
    DESCHEDULE();
}

static void thread_6_entry( void )
{
    SEM_CLR(24);
    DESCHEDULE();
}

static void thread_7_entry( void )
{
    SEM_CLR(28);
    DESCHEDULE();
}

/*f test_entry_point
 */
extern int test_entry_point()
{
    SEM_CLR(0);

    __asm__ volatile (".word 0xec00c83e \n mov r0, %0" : : "r" (0x40001) ); // set special (4) reg gip_config (3) to 0x40001 (cooperative, prioritized, no privilege, 4 arm trap semaphore)
    __asm__ volatile (".word 0xec00c83e \n mov r0, %0" : : "r" (0x40000) ); // set special (4) reg gip_config (3) to 0x40000 (preemptive, prioritized, no privilege, 4 arm trap semaphore)


    SET_THREAD(1,thread_1_entry,0x81); // set thread 1 startup to be ARM, on semaphore 7 set, and the entry point
    SET_THREAD(2,thread_2_entry,0x11); // set thread 2 startup to be ARM, on semaphore 8 set, and the entry point
    SET_THREAD(3,thread_3_entry,0x11); // set thread 3 startup to be ARM, on semaphore 12 set, and the entry point
    SET_THREAD(4,thread_4_entry,0x11); // set thread 4 startup to be ARM, on semaphore 16 set, and the entry point
    SET_THREAD(5,thread_5_entry,0x11); // set thread 5 startup to be ARM, on semaphore 20 set, and the entry point
    SET_THREAD(6,thread_6_entry,0x11); // set thread 6 startup to be ARM, on semaphore 24 set, and the entry point
    SET_THREAD(7,thread_7_entry,0x11); // set thread 7 startup to be ARM, on semaphore 28 set, and the entry point

    SEM_SET(7);
    SEM_SET(8);
    SEM_SET(12);
    SEM_SET(16);
    SEM_SET(20);
    SEM_SET(24);
    SEM_SET(28);

    DESCHEDULE();

}

