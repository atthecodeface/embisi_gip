/*a Includes
 */
#include "cmd.h"
#include "../drivers/flash.h"
#include "flash.h"

/*a Defines
 */

/*a Command functions
 */
/*f command_flash_erase
 */
static int command_flash_erase( void *handle, int argc, unsigned int *args )
{
    if (argc<1) return 1;
    return !flash_erase_block( args[0]<<17 );
}

/*f command_flash_boot
 */
static int command_flash_boot( void *handle, int argc, unsigned int *args )
{
    if (argc<1)
    {
        int i, offset;
        unsigned int csum;
        char buffer[256];
        for (i=0; i<16; i++)
        {
            offset = 0;
            if ( (mon_flash_read_object( i<<17, &csum, buffer, &offset, sizeof(buffer), 1 )>0) &&
                 (buffer[0]==obj_type_description) )
            {
                cmd_result_hex8( handle, i );
                cmd_result_string( handle, " : " );
                cmd_result_string_nl( handle, buffer+1 );
            }
        }
        return 0;
    }
    return !mon_flash_boot( args[0]<<17, 1 );
}

/*f command_flash_download
 */
static int command_flash_download( void *handle, int argc, unsigned int *args )
{
    if (argc!=0)
    {
        return 1;
    }
//    mon_flash_download();
    return 0;
}

/*f command_flash_read_location
 */
static int command_flash_read_location( void *handle, int argc, unsigned int *args )
{
    unsigned char buffer[64];
    int i, j, max;
    if (argc<1)
        return 1;
    max = 8;
    if (argc>1)
    {
        max = args[1];
    }
    max = (max>64)?64:max;

    flash_read_buffer( args[0], buffer, max );

    for (i=j=0; (i<max); i++)
    {
        if (j==0)
        {
            if (i>0)
            {
                cmd_result_nl( handle );
            }
            cmd_result_hex8( handle, args[0]+i );
            cmd_result_string( handle, ":" );
        }
        else
        {
            cmd_result_string( handle, " " );
        }
        cmd_result_hex2( handle, buffer[i] );
        j++;
        if (j==32) j=0;
    }
    return 0;
}

/*f command_flash_write_location
 */
static int command_flash_write_location( void *handle, int argc, unsigned int *args )
{
    unsigned char buffer[64];
    int i;
    if (argc<2)
        return 1;

    for (i=1; i<argc; i++)
    {
        buffer[i-1] = args[i];
    }
    return flash_write_buffer( args[0], buffer, argc-1  );
}

/*a External variables
 */
/*v monitor_cmds_flash
 */
extern const t_command monitor_cmds_flash[];
const t_command monitor_cmds_flash[] =
{
    {"fe", command_flash_erase},
    {"fb", command_flash_boot},
    {"fd", command_flash_download},
    {"fr", command_flash_read_location},
    {"fw", command_flash_write_location},
    {(const char *)0, (t_command_fn *)0},
};

