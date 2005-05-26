enum
{
    obj_type_end = 0,
    obj_type_description=1,
    obj_type_data=2,
    obj_type_regs=3
};

extern int mon_flash_read_object( unsigned int address, unsigned int *csum, char *buffer, int *offset, int max_length, int verbose );
extern int mon_flash_boot( unsigned int address, int verbose );
