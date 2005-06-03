/*a Includes
 */

/*a Defines
 */
#define GIP_SET_THREAD(thread,pc,data) { unsigned int s_pc=((unsigned int)(pc))|1, s_data=((unsigned int)(data))|0x100; __asm__ volatile (".word 0xec00c84e \n mov r0, %0 \n .word 0xec00c85e \n mov r0, %1 \n .word 0xec00c86e \n mov r0, %2 " : : "r" (thread), "r" (s_pc), "r" (s_data) ); }
#define GIP_DESCHEDULE() {__asm__ volatile (".word 0xec007401" );}
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
#define GIP_POST_TXD_0( data ) { __asm__ volatile ( " .word 0xec00c60e \n mov r0, %0 \n" : : "r" (data) ); }
#define GIP_POST_TXC_0( cmd ) { __asm__ volatile ( " .word 0xec00c6ce \n mov r0, %0 \n" : : "r" (cmd) ); }
#define GIP_POST_TXC_0_IO_CMD(route,len,sem,cmd) { GIP_POST_TXC_0( ((route)<<postbus_command_route_start) | ((len)<<postbus_command_source_gip_tx_length_start) | ((sem)<<postbus_command_source_gip_tx_signal_start) | ((cmd)<<postbus_command_target_io_dest_start) | (0<<postbus_command_target_io_dest_type_start) ); }
#define GIP_POST_TXC_0_IO_FIFO_DATA(route,len,sem,fifo,ing,cmdstat) { GIP_POST_TXC_0( ((route)<<postbus_command_route_start) | ((len)<<postbus_command_source_gip_tx_length_start) | ((sem)<<postbus_command_source_gip_tx_signal_start) | ((fifo)<<postbus_command_target_io_fifo_start) | ((ing)<<postbus_command_target_io_ingress) | ((cmdstat)<<postbus_command_target_io_cmd_status) | (2<<postbus_command_target_io_dest_type_start) ); }
#define GIP_POSTBUS_COMMAND(r,s,t) ( ((r)<<postbus_command_route_start) | ((s)<<postbus_command_source_start) | ((t)<<postbus_command_target_start) )

 // data == watermark (10;21), size_m_one (10;11), base (11;0)
#define GIP_POSTBUS_IO_FIFO_CFG( data, rx, fifo, base, size_m_one, wm ) { \
    GIP_POST_TXD_0( ((wm)<<21) | ((size_m_one)<<11) | ((base)<<0) ); \
    GIP_POST_TXC_0( ((fifo)<<postbus_command_target_io_fifo_start) | ((!(data))<<postbus_command_target_io_cmd_status) | ((rx)<<postbus_command_target_io_ingress) | (1<<postbus_command_target_io_dest_type_start) | (0<<postbus_command_source_gip_tx_length_start)); \
}
#define GIP_POSTIF_CFG(r,v) { __asm__ volatile ( " .word 0xec00c7ee+" #r "<<4 \n mov r0, %0 \n" : : "r" (v) ); }

#define GIP_READ_AND_CLEAR_SEMAPHORES( s, m ) { __asm__ volatile ( " .word 0xec00c80e \n mov r0, %0 \n" : : "r" (m) ); NOP; NOP; __asm__ volatile ( " .word 0xec00de08 \n mov %0, r0 \n" : "=r" (s) ); NOP;  }
#define GIP_READ_AND_SET_SEMAPHORES( s, m )   { __asm__ volatile ( " .word 0xec00c80e \n mov r0, %0 \n" : : "r" (m) ); NOP; NOP; __asm__ volatile ( " .word 0xec00de08 \n mov %0, r1 \n" : "=r" (s) ); NOP; }
// read gip_postbus(3) reg 12 (rx status 0)
#define GIP_POST_STATUS_0(s) { __asm__ volatile ( " .word 0xec00de06 \n mov %0, r12 \n" : "=r" (s) ); }
#define GIP_POST_RXD_0(s) { __asm__ volatile ( " .word 0xec00de06 \n mov %0, r0 \n" : "=r" (s) ); }
#define GIP_POST_RXCFG_0(s) { __asm__ volatile ( " .word 0xec00de06 \n mov %0, r14 \n" : "=r" (s) ); }
#define GIP_POST_TXCFG_0(s) { __asm__ volatile ( " .word 0xec00de06 \n mov %0, r15 \n" : "=r" (s) ); }

#define GIP_TIMER_WRITE(t,s) {__asm__ volatile ( " .word 0xec00c58e+" #t "<<4 \n mov r0, %0 \n" : : "r" (s) ); NOP; NOP; }
#define GIP_TIMER_READ_0(s) {__asm__ volatile ( " .word 0xec00ce05 \n mov %0, r8 \n " : "=r" (s) ); }
#define GIP_TIMER_READ_1(s) {__asm__ volatile ( " .word 0xec00ce05 \n mov %0, r9 \n " : "=r" (s) ); }
#define GIP_TIMER_READ_2(s) {__asm__ volatile ( " .word 0xec00ce05 \n mov %0, r10 \n " : "=r" (s) ); }
#define GIP_TIMER_READ_3(s) {__asm__ volatile ( " .word 0xec00ce05 \n mov %0, r11 \n " : "=r" (s) ); }
#define GIP_TIMER_ENABLE() {__asm__ volatile ( " .word 0xec00c58e \n mov r0, #0<<31 \n" ); NOP; NOP; }
#define GIP_SET_LOCAL_EVENTS_CFG(s) {__asm__ volatile ( " .word 0xec00c82e \n mov r0, %0 \n" : : "r" (s) ); NOP; NOP; }
#define GIP_LED_INPUT_STATUS(s) {__asm__ volatile ( " .word 0xec00ce05 \n mov %0, r0 \n " : "=r" (s) ); }
#define GIP_LED_INPUT_CFG_READ(s) {__asm__ volatile ( " .word 0xec00ce05 \n mov %0, r1 \n " : "=r" (s) ); }
#define GIP_LED_OUTPUT_CFG_READ(s) {__asm__ volatile ( " .word 0xec00ce05 \n mov %0, r2 \n " : "=r" (s) ); }
#define GIP_LED_OUTPUT_CFG_WRITE(s) {__asm__ volatile ( " .word 0xec00c52e \n mov r0, %0 \n " : : "r" (s) ); }
