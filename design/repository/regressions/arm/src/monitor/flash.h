enum
{
    obj_type_end = 0,
    obj_type_description=1,
    obj_type_data=2,
    obj_type_regs=3
};

extern int flash_erase_block( unsigned int address );
extern int flash_write_buffer( unsigned int address, unsigned char *buffer, int length  );
extern int flash_read_object( unsigned int address, unsigned int *csum, char *buffer, int *offset, int max_length  );
extern int flash_download( void );
extern int flash_boot( unsigned int address );

#ifndef LINUX
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
#define FLASH_EXEC( v ) { __asm__ volatile (" mov r0, %0 \n mov lr, pc \n ldmia r0, {r0-r10,pc} \n movnv r0, r0" : : "r" (v) ); }
#else

#define NOP

extern void flash_model_config_write( unsigned int v );
extern void flash_model_address_write( unsigned int v );
extern void flash_model_data_write( unsigned int v );

extern unsigned int flash_model_config_read( void );
extern unsigned int flash_model_address_read( void );
extern unsigned int flash_model_data_read( void );

#define FLASH_CONFIG_READ(v)  { v=flash_model_config_read(); }
#define FLASH_ADDRESS_READ(v) { v=flash_model_address_read(); }
#define FLASH_DATA_READ(v)    { v=flash_model_data_read(); }
#define FLASH_CONFIG_WRITE(v)  { flash_model_config_write(v); }
#define FLASH_ADDRESS_WRITE(v) { flash_model_address_write(v); }
#define FLASH_DATA_WRITE(v)    { flash_model_data_write(v); }
#define FLASH_EXEC(v) {fprintf(stderr,"Would call %08x with r0-r10 as:\n%08x %08x %08x %08x\n%08x %08x %08x %08x\n%08x %08x %08x\n", v[11], v[0], v[1], v[2], v[3], v[4], v[5], v[6], v[7], v[8], v[9], v[10]);}
#endif
