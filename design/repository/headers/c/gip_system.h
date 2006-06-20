/*a Notes
  This file lives in the hardware repository under headers/c
  AND in the software repository under include/asm
 */

#ifndef __GIP_SYSTEM
#define __GIP_SYSTEM

/*a Includes
 */

/*a Defines
 */
#define POSTBUS_ROUTE_GIP       (0)
#define POSTBUS_ROUTE_IO_A_SLOT (2)
#define POSTBUS_ROUTE_IO_B_SLOT (3)

#define IO_A_SLOT_ETHERNET_0 (0)
#define IO_A_SLOT_SYNC_SERIAL_0 (1)
#define IO_A_SLOT_PARALLEL_0    (2)
#define IO_A_SLOT_PARALLEL_1    (3)

#define SEM_NUM_MICROKERNEL_TRAP    (4)
#define SEM_NUM_MICROKERNEL_INT_EN  (5)
#define SEM_NUM_MICROKERNEL_HW_INT  (6)
#define SEM_NUM_VFC_STATUS_VNE  (20)
#define SEM_NUM_VFC_RX_DATA_VWM (21)
#define SEM_NUM_VFC_SOFT_IRQ    (22)
#define SEM_NUM_VFC_POSTBUS     (23)
#define SEM_NUM_ETH_STATUS_VNE  (24)
#define SEM_NUM_ETH_TX_DATA_VWM (25)
#define SEM_NUM_ETH_SOFT_IRQ    (26)
#define SEM_NUM_ETH_POSTBUS     (27)
#define SEM_NUM_TIMER       (28)

#define SEM_VFC_STATUS_VNE  (1<<SEM_NUM_VFC_STATUS_VNE)
#define SEM_VFC_RX_DATA_VWM (1<<SEM_NUM_VFC_RX_DATA_VWM)
#define SEM_VFC_SOFT_IRQ    (1<<SEM_NUM_VFC_SOFT_IRQ)
#define SEM_VFC_POSTBUS     (1<<SEM_NUM_VFC_POSTBUS)

#define SEM_ETH_STATUS_VNE  (1<<SEM_NUM_ETH_STATUS_VNE)
#define SEM_ETH_TX_DATA_VWM (1<<SEM_NUM_ETH_TX_DATA_VWM)
#define SEM_ETH_SOFT_IRQ    (1<<SEM_NUM_ETH_SOFT_IRQ)
#define SEM_ETH_POSTBUS     (1<<SEM_NUM_ETH_POSTBUS)

#define VFC_THREAD_NUM (5)
#define ETH_THREAD_NUM (6)
#define TMR_THREAD_NUM (7)

#define MK_INT_BIT_VFC_NUM (2)
#define MK_INT_BIT_VFC (1<<(MK_INT_BIT_VFC_NUM))
#define VFC_PRIVATE_REG_F 27
#define VFC_PRIVATE_REG_R r11

/*a End
 */
#endif  /* __GIP_SYSTEM */
