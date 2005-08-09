extern void sync_serial_wait_and_read_response( int slot, int *time, int *status, int *read_data );
extern void sync_serial_mdio_write( int slot, int clock_divider, unsigned int value );
extern void sync_serial_init( int slot );
