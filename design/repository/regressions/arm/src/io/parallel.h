
extern void parallel_frame_capture_init( int slot, int nlines, int init_gap, int pixel_gap, int pixels_per_line );
extern void parallel_frame_capture_start( int slot, unsigned int time );
extern int parallel_frame_capture_buffer( int slot, unsigned int *buffer, int *capture_time, int *num_status );
