#define t_gip_vfc__si_slot                  (0*4)
#define t_gip_vfc__si_action_array          (1*4)
#define t_gip_vfc__si_action_array_length   (2*4)
#define t_gip_vfc__rxd_buffer_store         (3*4)
#define t_gip_vfc__rxd_buffer_length        (4*4)
#define t_gip_vfc__rxd_data_length          (5*4)
#define t_gip_vfc__regs                     (6*4)

#ifdef GIP_INCLUDE_FROM_C
typedef struct
{
    unsigned int si_slot;
    unsigned int *si_action_array;
    unsigned int si_action_array_length;
    unsigned char *rxd_buffer_store;
    unsigned int rxd_buffer_length;
    unsigned int rxd_data_length;
    unsigned int regs[16];

    // from here on the items are NOT accessible from the hardware thread
    unsigned char *buffer_presented; // if rxd_buffer_store is NULL and the hardware thread kicks the ISR, then this buffer should be full - it is set by the client to the value placed in rxd_buffer_store at the 'go'
    unsigned int actions_prepared;
} t_gip_vfc;
#endif // GIP_INCLUDE_FROM_C


/*a External functions - in asm version
 */
#ifdef GIP_INCLUDE_FROM_C
extern void gip_video_capture_isr_asm( void );
extern void gip_video_capture_hw_thread( void );
#endif // GIP_INCLUDE_FROM_C

/*a External functions in C module
 */
#ifdef GIP_INCLUDE_FROM_C
extern void gip_video_capture_hw_thread_init( int slot );
extern int gip_video_capture_configure( int line_skip, int nlines, int init_gap, int pixel_gap, int pixels_per_line );
extern void gip_video_capture_start( unsigned char *buffer, int buffer_length );
extern int gip_video_capture_poll( void );
#endif // GIP_INCLUDE_FROM_C


