#ifndef __GIP_SUPPORT
#define __GIP_SUPPORT

/*a Includes
 */

/*a Defines
 */
/*b General
 */
#define NOP { __asm__ volatile(" movnv r0, r0"); }
#define NOP_WRINT { NOP; NOP; NOP; }
#define NOP_WREXT { int i; for (i=0; i<60; i++) NOP_WRINT;}
#define GIP_SET_THREAD(thread,pc,data) { unsigned int s_pc=((unsigned int)(pc))|1, s_data=((unsigned int)(data))|0x100; __asm__ volatile (".word 0xec00c84e \n mov r0, %0 \n .word 0xec00c85e \n mov r0, %1 \n .word 0xec00c86e \n mov r0, %2 " : : "r" (thread), "r" (s_pc), "r" (s_data) ); }

/*b Deschedule, block, atomic
 */
#define GIP_DESCHEDULE() {__asm__ volatile (".word 0xec007305" ); NOP; } // this needs to deschedule atomically else preempt can occur before it executes, so atomic
#define GIP_BLOCK_ALL() { __asm__ volatile (".word 0xec007281" ); }
#define GIP_ATOMIC_END() { __asm__ volatile ( ".word 0xec007201" ); }
#define GIP_ATOMIC_MAX() { __asm__ volatile ( ".word 0xec00723f" ); }
#define GIP_ATOMIC_MAX_BLOCK() { __asm__ volatile ( ".word 0xec0072bf" ); }
#define GIP_ATOMIC(n) {__asm__ volatile (".word 0xec007201+((" ##n## ")<<1)" );} // this makes the next 'n' valid instructions atomic with this one - preempt will not occur

/*b Postbus
 */
#define GIP_POST_TXD_0( data ) { __asm__ volatile ( " .word 0xec00c60e \n mov r0, %0 \n" : : "r" (data) ); }
#define GIP_POST_TXC_0( cmd ) { __asm__ volatile ( " .word 0xec00c6ce \n mov r0, %0 \n" : : "r" (cmd) ); }
#define GIP_POST_TXC_0_IO_CMD(route,len,sem,cmd) { GIP_POST_TXC_0( ((route)<<postbus_command_route_start) | ((len)<<postbus_command_source_gip_tx_length_start) | ((sem)<<postbus_command_source_gip_tx_signal_start) | ((cmd)<<postbus_command_target_io_dest_start) | (0<<postbus_command_target_io_dest_type_start) ); }
#define GIP_POST_TXC_0_IO_CFG(route,sem,addr) { GIP_POST_TXC_0( ((route)<<postbus_command_route_start) | ((0)<<postbus_command_source_gip_tx_length_start) | ((sem)<<postbus_command_source_gip_tx_signal_start) | ((addr)<<postbus_command_target_io_dest_start) | (3<<postbus_command_target_io_dest_type_start) ); }
#define GIP_POST_TXC_0_IO_FIFO_DATA(route,len,sem,fifo,ing,cmdstat) { GIP_POST_TXC_0( ((route)<<postbus_command_route_start) | ((len)<<postbus_command_source_gip_tx_length_start) | ((sem)<<postbus_command_source_gip_tx_signal_start) | ((fifo)<<postbus_command_target_io_fifo_start) | ((ing)<<postbus_command_target_io_ingress) | ((cmdstat)<<postbus_command_target_io_cmd_status) | (2<<postbus_command_target_io_dest_type_start) ); }
#define GIP_POSTBUS_COMMAND(r,s,t) ( ((r)<<postbus_command_route_start) | ((s)<<postbus_command_source_start) | ((t)<<postbus_command_target_start) )

 // data == watermark (10;21), size_m_one (10;11), base (11;0)
#define GIP_POSTBUS_IO_FIFO_CFG( route, data, rx, fifo, base, size_m_one, wm ) { \
    GIP_POST_TXD_0( ((wm)<<21) | ((size_m_one)<<11) | ((base)<<0) ); \
    GIP_POST_TXC_0( ((route)<<postbus_command_route_start) | ((fifo)<<postbus_command_target_io_fifo_start) | ((!(data))<<postbus_command_target_io_cmd_status) | ((rx)<<postbus_command_target_io_ingress) | (1<<postbus_command_target_io_dest_type_start) | (0<<postbus_command_source_gip_tx_length_start)); \
}
#define GIP_POSTIF_CFG(r,v) { __asm__ volatile ( " .word 0xec00c7ee+" #r "<<4 \n mov r0, %0 \n" : : "r" (v) ); }

// read gip_postbus(3) reg 12 (rx status 0)
#define GIP_POST_STATUS_0(s) { __asm__ volatile ( " .word 0xec00ce06 \n mov %0, r12 \n" : "=r" (s) ); }
#define GIP_POST_RXD_0(s) { __asm__ volatile ( " .word 0xec00ce06 \n mov %0, r0 \n" : "=r" (s) ); }
#define GIP_POST_RXCFG_0(s) { __asm__ volatile ( " .word 0xec00ce06 \n mov %0, r14 \n" : "=r" (s) ); }
#define GIP_POST_TXCFG_0(s) { __asm__ volatile ( " .word 0xec00ce06 \n mov %0, r15 \n" : "=r" (s) ); }

/*b Semaphores
 */
#define GIP_READ_AND_CLEAR_SEMAPHORES( s, m ) {  __asm__ volatile ( " mov r0, %1 ; swi 0xfa0000 ; mov %0, r0 " : "=r" (s) : "r" (m) : "r0" ); }
#define GIP_CLEAR_SEMAPHORES( m )      {  __asm__ volatile ( " mov r0, %0 ; swi 0xf20000 " : : "r" (m) : "r0" ); }
#define GIP_READ_AND_SET_SEMAPHORES( s, m )   {  __asm__ volatile ( " mov r0, %1 ; swi 0xea0000 ; mov %0, r0 " : "=r" (s) : "r" (m) : "r0" ); }
#define GIP_SET_SEMAPHORES( m )        {  __asm__ volatile ( " mov r0, %0 ; swi 0xe20000 " : : "r" (m) : "r0" ); }

/*b APB Uart
 */
#define GIP_UART_TX(a) {__asm__ volatile ("  .word 0xec00c40e \n mov r0, %0" :  : "r" (0x16000096 | (a<<8)) );}

/*b APB Extbus
 */
#define GIP_EXTBUS_CONFIG_WRITE(v) { __asm__ volatile (" .word 0xec00c48e \n mov r8, %0" : : "r" (v) ); }
#define GIP_EXTBUS_ADDRESS_WRITE(v) { __asm__ volatile (" .word 0xec00c4ae \n mov r8, %0" : : "r" (v) ); }
#define GIP_EXTBUS_DATA_WRITE(v) { __asm__ volatile (" .word 0xec00c49e \n mov r8, %0" : : "r" (v) ); }

/*b APB extbus - Flash
 */
#define FLASH_CONFIG_READ(v) { __asm__ volatile (" .word 0xec00ce04 \n mov %0, r8" : "=r" (v) ); }
#define FLASH_CONFIG_WRITE(v) { __asm__ volatile (" .word 0xec00c48e \n mov r8, %0" : : "r" (v) ); NOP_WRINT; }
#define FLASH_ADDRESS_READ(v) { __asm__ volatile (" .word 0xec00ce04 \n mov %0, r10" : "=r" (v) ); }
#define FLASH_ADDRESS_WRITE(v) { __asm__ volatile (" .word 0xec00c4ae \n mov r8, %0" : : "r" (v) ); NOP_WRINT; }
#define FLASH_DATA_READ(v) { __asm__ volatile (" .word 0xec00ce04 \n mov %0, r9" : "=r" (v) ); }
#define FLASH_DATA_WRITE( v ) { __asm__ volatile (" .word 0xec00c49e \n mov r8, %0" : : "r" (v) ); NOP_WREXT; }

/*b APB timer
 */
#define GIP_TIMER_WRITE(t,s) {__asm__ volatile ( " .word 0xec00c58e+" #t "<<4 \n mov r0, %0 \n" : : "r" (s) ); NOP; NOP; }
#define GIP_TIMER_READ_0(s) {__asm__ volatile ( " .word 0xec00ce05 \n mov %0, r8 \n " : "=r" (s) ); }
#define GIP_TIMER_READ_1(s) {__asm__ volatile ( " .word 0xec00ce05 \n mov %0, r9 \n " : "=r" (s) ); }
#define GIP_TIMER_READ_2(s) {__asm__ volatile ( " .word 0xec00ce05 \n mov %0, r10 \n " : "=r" (s) ); }
#define GIP_TIMER_READ_3(s) {__asm__ volatile ( " .word 0xec00ce05 \n mov %0, r11 \n " : "=r" (s) ); }
#define GIP_TIMER_ENABLE() {__asm__ volatile ( " .word 0xec00c58e \n mov r0, #0<<31 \n" ); NOP; NOP; }
#define GIP_TIMER_DISABLE() {__asm__ volatile ( " .word 0xec00c58e \n mov r0, #1<<31 \n" ); NOP; NOP; }

#define GIP_SET_LOCAL_EVENTS_CFG(s) {__asm__ volatile ( " .word 0xec00c82e \n mov r0, %0 \n" : : "r" (s) ); NOP; NOP; }
#define GIP_SET_SCHED_CFG(s) { __asm__ volatile (".word 0xec00c83e \n mov r0, %0" : : "r" (s) ); }

/*b APB GPIO
 */
#define GIP_GPIO_INPUT_STATUS(s) {__asm__ volatile ( " .word 0xec00ce05 \n mov %0, r0 \n " : "=r" (s) ); }
#define GIP_LED_INPUT_CFG_READ(s) {__asm__ volatile ( " .word 0xec00ce05 \n mov %0, r1 \n " : "=r" (s) ); }
#define GIP_LED_OUTPUT_CFG_READ(s) {__asm__ volatile ( " .word 0xec00ce05 \n mov %0, r2 \n " : "=r" (s) ); }
#define GIP_LED_OUTPUT_CFG_WRITE(s) {__asm__ volatile ( " .word 0xec00c52e \n mov r0, %0 \n " : : "r" (s) ); }

/*b Global APB analyzer
 */
#define GIP_ANALYZER_WRITE(t,s) {__asm__ volatile ( " .word 0xec00c20e+" #t "<<4 \n mov r0, %0 \n" : : "r" (s) ); NOP; GIP_BLOCK_ALL(); NOP; }
#define GIP_ANALYZER_READ_CONTROL(s)  {__asm__ volatile ( " .word 0xec00ce02 \n mov %0, r0 \n " : "=r" (s) ); }
#define GIP_ANALYZER_READ_DATA(s)  {__asm__ volatile ( " .word 0xec00ce02 \n mov %0, r1 \n " : "=r" (s) ); }
#define GIP_ANALYZER_TRIGGER_CONTROL(count,stage_if_true,action_if_true,stage_if_false,action_if_false) { GIP_ANALYZER_WRITE(1, ( (count)<<0) | ((stage_if_true)<<16) | ((action_if_true)<<20) | ((stage_if_false)<<24) | ((action_if_false)<<28)); }
#define GIP_ANALYZER_TRIGGER_MASK(mask) {GIP_ANALYZER_WRITE(2,(mask));}
#define GIP_ANALYZER_TRIGGER_COMPARE(mask) {GIP_ANALYZER_WRITE(3,(mask));}

/*a Types
 */
/*t t_analyzer_action
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

/*a End
 */
#endif  /* __GIP_SUPPORT */
