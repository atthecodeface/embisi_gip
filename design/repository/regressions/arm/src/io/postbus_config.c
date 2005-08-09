/*a Includes
 */
#include <stdlib.h> // for NULL
#include "gip_support.h"
#include "postbus.h"

/*a Defines
 */

/*a Types
 */

/*a External functions
 */
/*f postbus_config
  set postbus fifos up
  for the gip RF split with rx_0, tx_0, rx_1 and tx_1
  if tx_0<=0 then share it with rx_0
  ditto for tx_1
 */
extern void postbus_config( int rx_0, int tx_0, int rx_1, int tx_1 )
{
    int n;

    // GIP postbus RF split as 16 for rx/tx fifo 0, 16 for rx/tx fifo 1 (which does not yet exist due to Xilinx tool issues)
    n = 0;
    GIP_POSTIF_CFG( 0, (n<<0) | ((n+rx_0)<<8)); // Rx fifo 0 config base 0, end 8, read 0, write 0 - can overlap with tx as we use one at a time
    if (tx_0>0)
    {
        n += rx_0;
    }
    else
    {
        tx_0 = rx_0;
    }
    GIP_POSTIF_CFG( 1, (n<<0) | ((n+tx_0)<<8) ); // Tx fifo 0 config base 8, end 16, read 0, write 0
    n += tx_0;

    // egress fifo sram (2kx32) split as etxcmd 32, sscmd 32, ssdata 32, gap, etx 1920 (rest)
    GIP_POSTBUS_IO_FIFO_CFG( 0, 0, 0,  0/2,   15, 3 );  // eth tx cmd - unused - note status size and base are in entries, not words
    GIP_POSTBUS_IO_FIFO_CFG( 0, 0, 1, 32/2,   15, 3 );  // ss cmd
    GIP_POSTBUS_IO_FIFO_CFG( 0, 0, 2,  0/2,   15, 3 );  // unused
    GIP_POSTBUS_IO_FIFO_CFG( 0, 0, 3,  0/2,   15, 3 );  // unused
    GIP_POSTBUS_IO_FIFO_CFG( 1, 0, 0,  128, 1919, 3 );  // eth tx data
    GIP_POSTBUS_IO_FIFO_CFG( 1, 0, 1,   64,   31, 3 );  // ss txd
    GIP_POSTBUS_IO_FIFO_CFG( 1, 0, 2,    0,   31, 3 );  // unused
    GIP_POSTBUS_IO_FIFO_CFG( 1, 0, 3,    0,   31, 3 );  // unused

    // ingress fifo sram (2kx32) split as erx status 32, etx status 32, ss status 32, ss rxd 32, erx 1920 (rest)
    GIP_POSTBUS_IO_FIFO_CFG( 0, 1, 0,  0/2,   15, 3 ); // eth status - note status size and base are in entries, not words
    GIP_POSTBUS_IO_FIFO_CFG( 0, 1, 1, 32/2,   15, 3 ); // ss status
    GIP_POSTBUS_IO_FIFO_CFG( 0, 1, 2,  0/2,   15, 3 ); // unused
    GIP_POSTBUS_IO_FIFO_CFG( 0, 1, 3,  0/2,   15, 3 ); // unused
    GIP_POSTBUS_IO_FIFO_CFG( 1, 1, 0,  128, 1919, 3 ); // erx data
    GIP_POSTBUS_IO_FIFO_CFG( 1, 1, 1,   96,   31, 3 ); // ss rx data
    GIP_POSTBUS_IO_FIFO_CFG( 1, 1, 2,    0,   31, 3 ); // etx data - unused
    GIP_POSTBUS_IO_FIFO_CFG( 1, 1, 3,    0,   31, 3 ); // unused

}

