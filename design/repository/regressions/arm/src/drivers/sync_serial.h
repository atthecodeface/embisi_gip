extern void sync_serial_wait_and_read_response( int postbus_route, int slot, int *time, int *status, int *read_data );
extern void sync_serial_mdio_write( int postbus_route, int slot, int clock_divider, unsigned int value );
extern void sync_serial_mdio_read( int postbus_route, int slot, int clock_divider, unsigned int value );
extern void sync_serial_init( int postbus_route, int slot );
