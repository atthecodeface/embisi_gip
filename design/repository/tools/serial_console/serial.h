extern int open_serial( const char *filename, int baud, int verbose );
extern int poll_fd( int fd, int read, int write, int timeout_s, int timeout_ms );
extern void serial_putchar( int ch );

