#ifndef __GIP_SYSTEM
#define __GIP_SYSTEM

/*a Includes
 */

/*a Defines
 */
#define IO_A_SLOT_ETHERNET_0 (0)
#define IO_A_SLOT_SYNC_SERIAL_0 (1)
#define IO_A_SLOT_PARALLEL_0    (3)

#define SEM_NUM_MICROKERNEL_TRAP    (4)
#define SEM_NUM_MICROKERNEL_INT_EN  (5)
#define SEM_NUM_MICROKERNEL_HW_INT  (6)
#define SEM_NUM_STATUS_VNE  (24)
#define SEM_NUM_TX_DATA_VWM (25)
#define SEM_NUM_SOFT_IRQ    (26)
#define SEM_NUM_POSTBUS     (27)
#define SEM_NUM_TIMER       (28)

#define SEM_STATUS_VNE  (1<<SEM_NUM_STATUS_VNE)
#define SEM_TX_DATA_VWM (1<<SEM_NUM_TX_DATA_VWM)
#define SEM_SOFT_IRQ    (1<<SEM_NUM_SOFT_IRQ)
#define SEM_POSTBUS     (1<<SEM_NUM_POSTBUS)
        
/*a End
 */
#endif  /* __GIP_SYSTEM */
