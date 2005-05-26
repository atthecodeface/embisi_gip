extern int flash_erase_block( unsigned int address ); // address is byte-based, and includes chip selects etc (it is an extbus address)
extern int flash_write_buffer( unsigned int address, unsigned char *buffer, int length ); // address is byte-based (extbus address), arbitrary length, buffer with any alignment: return 1 for success, 0 for failure
extern int flash_read_buffer( unsigned int address, unsigned char *buffer, int length  ); // address is byte-based (extbus address), arbitrary length, buffer with any alignment: return 1 for success, 0 for failure

