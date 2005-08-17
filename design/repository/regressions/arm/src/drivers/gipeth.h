/*a Defines
 */
#define GIPETH_BUFFER_LENGTH (2048)
#define GIPETH_BUFFER_LENGTH_INC_TIME (GIPETH_BUFFER_LENGTH+4)
#define NUM_TX_BUFFERS (4)
#define NUM_RX_BUFFERS (4)

#define t_gipeth__rx_next_buffer_to_reuse (0*4)
#define t_gipeth__rx_buffer_in_hand       (1*4)
#define t_gipeth__rx_length_so_far        (2*4)
#define t_gipeth__tx_next_buffer_to_tx    (3*4)
#define t_gipeth__tx_buffer_in_hand       (4*4)
#define t_gipeth__tx_length               (5*4)
#define t_gipeth__regs                    (6*4)

#define t_gipeth_buffer__next_in_list     (0*4)
#define t_gipeth_buffer__data             (1*4)
#define t_gipeth_buffer__length           (2*4)
#define t_gipeth_buffer__ready            (3*4)
#define t_gipeth_buffer__done             (4*4)

/*a Types
 */
#ifdef GIP_INCLUDE_FROM_C
/*t t_gipeth_buffer - this must match the assembler version
 */
typedef struct t_gipeth_buffer
{
    struct t_gipeth_buffer *next_in_list;
    unsigned int *data; // pointer to data start in the buffer - there needs to be one word before this pointer for the time the first block of data was received
    int length; // length in bytes - updated on receive by hardware thread, set on transmit by client
    int ready; // for rx indicates buffer is full and length set, client to read data and set done
    // for tx indicates buffer is full and length set, driver to transmit and set done
    int done; // for rx indicates buffer is available for driver to fill, client to clear upon handling of ready buffer
    // for tx indicates driver has transmitted
} t_gipeth_buffer;

/*t t_gipeth
 */
typedef struct t_gipeth
{
    t_gipeth_buffer *rx_next_buffer_to_reuse; // owned by hardware thread, indicates next buffer to receive into - not used by client
    unsigned int *rx_buffer_in_hand;    // owned by the hardware thread, buffer being received into (NULL if none)
    int rx_length_so_far;               // owned by the hardware thread, length received so far in buffer
    t_gipeth_buffer *tx_next_buffer_to_tx; // owned by the hardware thread, indicates next buffer to transmit from (if ready) - not used by client
    unsigned int *tx_buffer_in_hand;       // owned by the hardware thread, next data to be transmitted
    int tx_length;                         // owned by the hardware thread, indicates amount of data at tx_buffer_in_hand

    unsigned int regs[16]; // place to store registers when in hardware thread

    // rest is not accessible from hardware thread

    t_gipeth_buffer *rx_buffers; // circular linked chain of rx buffers
    t_gipeth_buffer *tx_buffers; // circular linked chain of tx buffers

    t_gipeth_buffer *rx_next_buffer_rxed; // owned by client, indicates next buffer to check for 'ready' in - not used by driver
    t_gipeth_buffer *tx_next_buffer_done; // owned by client, indicates next buffer to check for 'done' in - not used by driver

} t_gipeth;
#endif

/*a External functions - in asm version
 */
#ifdef GIP_INCLUDE_FROM_C
extern void gipeth_isr_asm( void );
extern void gipeth_hw_thread( void );
#endif // GIP_INCLUDE_FROM_C

/*a External functions in C module
 */
#ifdef GIP_INCLUDE_FROM_C
#ifdef REGRESSION
extern void gipeth_setup(void);
extern int gipeth_xmit( unsigned int *data, int length );
#endif // REGRESSION
#endif // GIP_INCLUDE_FROM_C
