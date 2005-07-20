/*a Notes
stty /dev/ttyS0 raw
stty /dev/ttyS0 -echo
stty -F /dev/ttyS0 ispeed 2400 ospeed 2400

 */

/*a Includes
 */
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h> /* for errno */

#include <sys/ioctl.h> /* For ioctl() */

#include <sys/types.h> /* For FDs */
#include <sys/stat.h>
#include <fcntl.h>

#include <linux/serial.h> /* For struct serial_struct */
#include <asm/ioctls.h> /* For TIOCGSERIAL, TCGETA, TCSETA */

#include <sys/select.h> /* For select */

#include <sys/time.h> /* For select timeout */

//#include <asm/termios.h> /* For struct termio  - not needed with sys/ioctl.h */
#include <asm/termbits.h> /* For CBAUD, B9600, B19200, B38400, B57600, B115200 */

#include <getopt.h>

#include <curses.h>

#include <string.h>

#include "serial.h"
#include "flash.h"
#include "sl_general.h"
#include "sl_token.h"
#include "sl_exec_file.h"
#include "c_sl_error.h"
#include "sl_mif.h"

/*a Defines
 */
#define CHAR_TIME (1000*1000/120)

/*a Types
 */
/*t window modes
 */
enum 
{
     text_mode,
     cmd_mode
};

/*t t_command_fn
 */
typedef void t_command_fn( char *cmd, char *cmd_end );

/*t t_command
 */
typedef struct
{
    const char *name;
    t_command_fn *fn;
} t_command;

/*t t_obj_type
 */
typedef enum
{
    obj_type_end = 0,
    obj_type_description=1,
    obj_type_data=2,
    obj_type_regs=3
} t_obj_type;

/*a Forward function declarations
 */
static void text_mode_enter( void );
static void command_mode_enter( void );
static void cmd_help( char *cmd, char *cmd_end );
static void cmd_flash_write( char *cmd, char *cmd_end );
static void cmd_exec( char *cmd, char *cmd_end );
static void cmd_rep( char *cmd, char *cmd_end );

/*a Global variables
 */
FILE *serlog;
FILE *outputlog;

/*a Static variables
 */
/*v Statics that just make it easy
 */
WINDOW *command_window;
WINDOW *text_window;
int mode;
int kbd_fd, serial_fd;
static char cmd[256];
static int target_little_endian;

/*v cmds
 */
static t_command cmds[] =
{
    {"?", cmd_help},
    {"help", cmd_help},
    {"fw", cmd_flash_write},
    {"exec", cmd_exec},
    {"rep", cmd_rep},
    {NULL, NULL}
};

/*v long_options
 */
static struct option long_options[] = 
{
     { "filename", 1, NULL, 'f' },
     { "baud", 1, NULL, 'b' },
     { "bigendian", 0, NULL, 'B' },
     { "verbose", 0, NULL, 'v' },
};

/*v ef_cmd_*
 */
enum
{
    ef_cmd_flash_erase = sl_exec_file_cmd_first_external,
    ef_cmd_flash_address,
    ef_cmd_flash_write_string,
    ef_cmd_flash_write_bytes,
    ef_cmd_flash_write_object_string,
    ef_cmd_flash_write_object_regs,
    ef_cmd_flash_write_object_mif,
    ef_cmd_flash_write_object_end,
};

/*v fn_*
 */
enum {
    ef_fn_flash_address=sl_exec_file_fn_first_external
};

/*v ef_cmds
 */
static t_sl_exec_file_cmd ef_cmds[] =
{
     {ef_cmd_flash_erase,                0,  "flash_erase", "", "flash_erase"},
     {ef_cmd_flash_address,              1,  "flash_address", "i", "flash_address <byte address>"},
     {ef_cmd_flash_write_string,         1,  "flash_write_string", "s", "flash_write_string <string to write>"},
     {ef_cmd_flash_write_bytes,          1,  "flash_write_bytes", "iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii", "flash_write_bytes <bytes to write>"},
     {ef_cmd_flash_write_object_string,  1,  "flash_write_object_string", "s", "flash_write_object_string <string to write>"},
     {ef_cmd_flash_write_object_regs,    12, "flash_write_object_regs", "iiiiiiiiiiii", "flash_write_bytes <bytes to write>"},
     {ef_cmd_flash_write_object_mif,     4,  "flash_write_object_mif", "isii", "flash_write_object_mif <load address> <MIF file to write> <offset into MIF file> <data length>"},
     {ef_cmd_flash_write_object_end,     0,  "flash_write_object_end", "", "flash_write_object_end"},
     {sl_exec_file_cmd_none, 0, NULL, NULL, NULL }
};

/*v ef_fns
 */
static t_sl_exec_file_eval_fn ef_fn_eval_flash_address;
static t_sl_exec_file_fn ef_fns[] =
{
     {ef_fn_flash_address,               "flash_address",         'i', "", "flash_address()", ef_fn_eval_flash_address },
     {sl_exec_file_fn_none, NULL,     0,   NULL, NULL },
};

/*v flash_address - byte address
 */
static unsigned int flash_address;

/*a Command window commands
 */
/*f cmd_help
 */
static void cmd_help( char *cmd, char *cmd_end )
{
    int i;
    waddstr( command_window, "Commands available:\n" );
    for (i=0; cmds[i].name; i++)
    {
        waddstr( command_window, "\t" );
        waddstr( command_window, cmds[i].name );
        waddstr( command_window,  "\n" );
    }
    wrefresh( command_window );

}

/*f cmd_flash_write
 */
static void cmd_flash_write( char *cmd, char *cmd_end )
{
    char *token;
    unsigned int address;
    char buffer[128];
    int length;
    int error;

    token = sl_token_next( 1, cmd, cmd_end );
    error = 1;
    if (token)
    {
        error = !sl_integer_from_token( token, (int *)&address );
        buffer[0] = (address>> 0)&0xff;
        buffer[1] = (address>> 8)&0xff;
        buffer[2] = (address>>16)&0xff;
        buffer[3] = (address>>24)&0xff;
    }
    length = 4;
    token = sl_token_next( 1, token, cmd_end );
    while ( (!error) && (token) && (length<(int)sizeof(buffer)) )
    {
        int data;
        error = !sl_integer_from_token( token, &data );
        buffer[length++] = data;
        token = sl_token_next( 1, token, cmd_end );
    }
    if (!error)
    {
        flash_download_command( 'w', (unsigned char *)buffer, length );
    }
    if (error)
    {
        waddstr( command_window, "Syntax: " );
        waddstr( command_window, cmd );
        waddstr( command_window,  " <byte address> <data bytes>+\n" );
        wrefresh( command_window );
    }

}

/*f ef_fn_eval_flash_address
 */
static int ef_fn_eval_flash_address( void *handle, t_sl_exec_file_data *file_data, t_sl_exec_file_value *args )
{
    return sl_exec_file_eval_fn_set_result( file_data, (int)flash_address );
}

/*f flash_download_command_object
 */
static int flash_download_command_object( t_obj_type type, unsigned char *buffer, int length, unsigned char *buffer_2, int length_2 )
{
    unsigned char write_buffer[64+16]; // Must be bigger than 64, but less than 128- a bit for expansion later
    int i, j;
    int hdr_reqd;
    int obj_length;
    unsigned int csum;
    int max_size;

    obj_length = (length+length_2+1); // include type in the length
    obj_length = (obj_length+1)&~1; // add padding to next 16-bits

    csum = type;
    for (i=0, j=1; i<length; i++, j++)
    {
        if (j&1) // odd bytes are top 8 bits of 16-bit word
        {
            csum += buffer[i]<<8;
        }
        else // even bytes are bottom 8 bits of 16-bit word
        {
            csum += buffer[i];
        }
    }
    for (i=0; buffer_2 && (i<length_2); i++, j++)
    {
        if (j&1)
        {
            csum += buffer_2[i]<<8;
        }
        else
        {
            csum += buffer_2[i];
        }
    }
    fprintf(serlog,"\n\ncsum %08x, i %d, j %d, obj_length %d\n\n", csum, i, j, obj_length );
    csum = (csum&0xffff)+(csum>>16);
    csum = ((csum >> 16) + csum)&0xffff;

    hdr_reqd = 1;
    for (i=0; i<obj_length;)
    {
        j=0;
        max_size = 64-(flash_address&0x3f);  // this will get us 64-byte aligned after the first write
        write_buffer[j++] = (flash_address>> 0)&0xff;
        write_buffer[j++] = (flash_address>> 8)&0xff;
        write_buffer[j++] = (flash_address>>16)&0xff;
        write_buffer[j++] = (flash_address>>24)&0xff;
        if (hdr_reqd)
        {
            write_buffer[j++] = (obj_length>>0 )&0xff;
            write_buffer[j++] = (obj_length>>8 )&0xff;
            write_buffer[j++] = (csum>>0 )&0xff;
            write_buffer[j++] = (csum>>8 )&0xff;
            write_buffer[j++] = (type>>0 )&0xff; // first byte of actual data is the type
            i++;
        }
        for (; (j<4+max_size) && (i<length+1); i++, j++) // bytes 1 to length inclusive
        {
            write_buffer[j] = buffer[i-1];
        }
        for (; buffer_2 && (j<4+max_size) && (i<length+length_2+1); i++, j++) // bytes length+1 to length+length_2+1 inclusive
        {
            write_buffer[j] = buffer_2[i-length-1];
        }
        for (; (j<4+max_size) && (i<obj_length); j++) // bytes length+length_2+1 to obj_length inclusive
        {
            write_buffer[j] = 0; // padding at end with 0
            i++;
        }
        flash_download_command( 'w', write_buffer, j );
        if (!flash_download_wait()) return 0;
        flash_address += j-4;
        hdr_reqd = 0;
    }
    return 1;
}
static int flash_download_command_object( t_obj_type type, unsigned char *buffer, int length )
{
    return flash_download_command_object( type, buffer, length, NULL, 0 );
}


/*f cmd_exec
 */
static void cmd_exec( char *cmd, char *cmd_end )
{
    char *token;
    c_sl_error *error;
    t_sl_exec_file_data *exec_file;

    token = sl_token_next( 1, cmd, cmd_end );
    if (!token)
    {
        waddstr( command_window, "Syntax: " );
        waddstr( command_window, cmd );
        waddstr( command_window,  " <filename>\n" );
        wrefresh( command_window );
        return;
    }
    error = new c_sl_error();
    exec_file = NULL;
    sl_exec_file_allocate_and_read_exec_file( error, error, NULL, NULL, "serial_console_exec", token, &exec_file, "command window", ef_cmds, ef_fns );

    flash_address = 0xffffffff;
    if (exec_file)
    {
        int i;
        int ef_cmd;
        t_sl_exec_file_value *ef_args;
        int unset_address_error;
        int download_error;

        sl_exec_file_reset( exec_file );
        i=0;
        while (sl_exec_file_get_next_cmd(exec_file, &ef_cmd, &ef_args))
        {
            unset_address_error = 0;
            download_error = 0;
            switch (ef_cmd)
            {
            case ef_cmd_flash_address:
                flash_address = ef_args[0].integer;
                break;
            case ef_cmd_flash_erase:
                if (flash_address==0xffffffff)
                {
                    unset_address_error = 1;
                }
                else
                {
                    flash_download_command( 'e', (unsigned char *)&flash_address, 4 );
                    download_error = !flash_download_wait();
                }
                break;
            case ef_cmd_flash_write_string: // copy string to flash, byte 0 going in lowest byte in flash
                if (flash_address==0xffffffff)
                {
                    unset_address_error = 1;
                }
                else
                {
                    char buffer[256];
                    buffer[0] = (flash_address>> 0)&0xff;
                    buffer[1] = (flash_address>> 8)&0xff;
                    buffer[2] = (flash_address>>16)&0xff;
                    buffer[3] = (flash_address>>24)&0xff;
                    for (i=4; i<(int)sizeof(buffer); i++)
                    {
                        buffer[i] = ef_args[0].p.string[i-4];
                        if (buffer[i]==0)
                            break;
                    }
                    i++;
                    if (i<(int)sizeof(buffer))
                    {
                        flash_address += i-4;
                        flash_download_command( 'w', (unsigned char *)buffer, i );
                        download_error = !flash_download_wait();
                    }
                    else
                    {
                        download_error = 1;
                    }
                }
                break;
            case ef_cmd_flash_write_bytes: // copy bytes to flash, byte 0 going in lowest byte in flash
                if (flash_address==0xffffffff)
                {
                    unset_address_error = 1;
                }
                else
                {
                    char buffer[256];
                    buffer[0] = (flash_address>> 0)&0xff;
                    buffer[1] = (flash_address>> 8)&0xff;
                    buffer[2] = (flash_address>>16)&0xff;
                    buffer[3] = (flash_address>>24)&0xff;
                    for (i=4; ef_args[i-4].type!=sl_exec_file_value_type_none; i++)
                    {
                        buffer[i] = ef_args[i-4].integer;
                    }
                    flash_address += i-4;
                    flash_download_command( 'w', (unsigned char *)buffer, i );
                    download_error = !flash_download_wait();
                }
                break;
            case ef_cmd_flash_write_object_string: // copy string to flash, byte 0 going in lowest byte in flash
                if (flash_address==0xffffffff)
                {
                    unset_address_error = 1;
                }
                else
                {
                    download_error = !flash_download_command_object( obj_type_description, (unsigned char *)ef_args[0].p.string, strlen(ef_args[0].p.string)+1 );
                }
                break;
            case ef_cmd_flash_write_object_regs: // copy regs: target endianness dependent
                if (flash_address==0xffffffff)
                {
                    unset_address_error = 1;
                }
                else
                {
                    unsigned char buffer[256];
                    for (i=0; i<12; i++)
                    {
                        if (target_little_endian)
                        {
                            buffer[i*4+0] = (ef_args[i].integer>> 0)&0xff;
                            buffer[i*4+1] = (ef_args[i].integer>> 8)&0xff;
                            buffer[i*4+2] = (ef_args[i].integer>>16)&0xff;
                            buffer[i*4+3] = (ef_args[i].integer>>24)&0xff;
                        }
                        else
                        {
                            buffer[i*4+0] = (ef_args[i].integer>>24)&0xff;
                            buffer[i*4+1] = (ef_args[i].integer>>16)&0xff;
                            buffer[i*4+2] = (ef_args[i].integer>> 8)&0xff;
                            buffer[i*4+3] = (ef_args[i].integer>> 0)&0xff;
                        }
                    }
                    download_error = !flash_download_command_object( obj_type_regs, buffer, 48 );
                }
                break;
            case ef_cmd_flash_write_object_mif: // copy words of data from a MIF file: target endianness dependent
                if (flash_address==0xffffffff)
                {
                    unset_address_error = 1;
                }
                else
                {
                    unsigned char *data;
                    unsigned char buffer[4];
                    if (sl_mif_allocate_and_read_mif_file( error, ef_args[1].p.string, "", ef_args[2].integer, ef_args[3].integer, 32, 0, 1, 0, &(int *)data, NULL, NULL )==error_level_okay)
                    {
                        buffer[0] = (ef_args[0].integer>> 0)&0xff; // hmm - the address to put it at, always sent little_endian
                        buffer[1] = (ef_args[0].integer>> 8)&0xff;
                        buffer[2] = (ef_args[0].integer>>16)&0xff;
                        buffer[3] = (ef_args[0].integer>>24)&0xff;
                        if (!target_little_endian) // endian swap the data
                        {
                            int i;
                            unsigned int d;
                            for (i=0; i<ef_args[3].integer/4; i++)
                            {
                                d = ((unsigned int *)data)[i];
                                data[i*4+0] = d>>24;
                                data[i*4+1] = d>>16;
                                data[i*4+2] = d>>8;
                                data[i*4+3] = d>>0;
                            }
                        }
                        flash_download_command_object( obj_type_data, buffer, 4, data, ef_args[3].integer );
                        download_error = !flash_download_wait();
                    }
                }
                break;
            case ef_cmd_flash_write_object_end:
                if (flash_address==0xffffffff)
                {
                    unset_address_error = 1;
                }
                else
                {
                    unsigned char buffer[4];
                    buffer[0] = 0;
                    buffer[1] = 0;
                    download_error = !flash_download_command_object( obj_type_end, buffer, 2 );
                }
                break;
            }
            if (unset_address_error)
            {
                error->add_error( (void *)"", error_level_fatal, error_number_general_error_s, 0, error_arg_type_const_string, "Flash address not set", error_arg_type_malloc_filename, token, error_arg_type_line_number, sl_exec_file_line_number( exec_file )+1, error_arg_type_none );
                break;
            }
            if (download_error)
            {
                error->add_error( (void *)"", error_level_fatal, error_number_general_error_s, 0, error_arg_type_const_string, "Flash command download failed", error_arg_type_malloc_filename, token, error_arg_type_line_number, sl_exec_file_line_number( exec_file )+1, error_arg_type_none );
                break;
            }
            i++;
#define MAX_EF_RUN_CMDS (10000)
            if (i>MAX_EF_RUN_CMDS) break;
        }
    }
//    if (error->get_error_count( error_level_okay )>0)
    {
        char buffer[256];
        void *error_handle;
        error_handle = NULL;
        while ((error_handle = error->get_next_error( error_handle, error_level_okay ))!=NULL)
        {
            if (error->generate_error_message( error_handle, buffer, sizeof(buffer), 1, NULL ))
            {
                waddstr( command_window, buffer );
                wechochar( command_window, '\n' );
            }
        }
        wrefresh( command_window );
    }
    if (exec_file)
    {
        sl_exec_file_free( exec_file );
    }
    delete error;
}

/*f cmd_rep
 */
static void cmd_rep( char *cmd, char *cmd_end )
{
    char *token;
    int count;
    int error;

    token = sl_token_next( 1, cmd, cmd_end );
    cmd = cmd+strlen(cmd);
    
    error = 1;
    if (token)
    {
        error = !sl_integer_from_token( token, (int *)&count );
    }

    token = token+strlen(token)+1;
    if ((!error) && (token<cmd_end))
    {
        int i, j;
        for (i=0; i<count; i++)
        {
            for (j=0; token[j]; j++)
            {
                serial_putchar(token[j]);
            }
            serial_putchar('\n');
            while (1)
            {
                char buffer[4];
                j = poll_fd(serial_fd, 1, 0, 0, 0);
                if (j==2)
                {
                    read( serial_fd, buffer, 1 );
                }
                else
                {
                    break;
                }
            }
        }
        return;
    }
    if (error)
    {
        waddstr( command_window, "Syntax: " );
        waddstr( command_window, cmd );
        waddstr( command_window,  " <byte address> <data bytes>+\n" );
        wrefresh( command_window );
    }

}

/*a Command window functions
 */
/*f command_obey
 */
static void command_obey( char *cmd )
{
    int i;
    char *cmd_end;
    char *token;

    i = 0;
//     flash_download_command( 'e', (unsigned char *)&i, 4 );
//     if (!flash_download_wait()) return;
//     flash_download_command( 'w', boot_desc, sizeof(boot_desc) );
//     if (!flash_download_wait()) return;
//     flash_download_command( 'w', boot_desc, sizeof(boot_desc) );
//     if (!flash_download_wait()) return;
//     flash_download_command( 'w', boot_desc, sizeof(boot_desc) );
//     if (!flash_download_wait()) return;

    cmd_end = cmd+strlen(cmd);
    token = sl_token_next( 0, cmd, cmd_end );
    for (i=0; token && cmds[i].name; i++)
    {
        if (!strcmp( token, cmds[i].name ))
        {
            cmds[i].fn( cmd, cmd_end );
        }
    }
}

/*f command_mode_enter
 */
static void command_mode_enter( void )
{
    fprintf(serlog,"\ncmd_mode_enter\n");
     mode = cmd_mode;
     cmd[0] = 0;
     waddstr( command_window, "Command: (ctl-g to return) > ");
     wrefresh( command_window );
}

/*f command_mode_key
 */
static void command_mode_key( char c )
{
     if (c==7)
     {
          text_mode_enter();
     }
     else if ((c>=32) && (c<127))
     {
          int l;

          l = strlen(cmd);
          if (l<(int)sizeof(cmd)-1)
          {
               cmd[l] = c;
               cmd[l+1] = 0;
               wechochar( command_window, c );
          }
     }
     else if (c==127)
     {
          int l;
          if (cmd[0])
          {
               l = strlen(cmd);
               cmd[l-1] = 0;
               wechochar( command_window, 8 );
               wechochar( command_window, 32 );
               wechochar( command_window, 8 );
          }
     }
     else if (c==13)
     {
          wechochar( command_window, '\n' );
          command_obey( cmd );
          command_mode_enter();
     }
     else if (c==7)
     {
          text_mode_enter();
     }
     else
     {
//          fprintf(stderr,"%d\n", c );
     }
}

/*a Text window functions
 */
/*f text_mode_enter
 */
static void text_mode_enter( void )
{
     mode = text_mode;
     wechochar( command_window, '\r' );
//     wechochar( command_window, '\n' );
     wrefresh( text_window );
     fprintf(serlog,"\ntext_mode_enter\n");
}

/*f text_mode_key
 */
static void text_mode_key( char c )
{
     if (c=='!')
     {
          command_mode_enter();
     }
     else if (c==127)
     {
          c = 8;
          serial_putchar( c );
     }
     else
     {
          serial_putchar( c );
     }
}

/*a Main
 */
/*f main
 */
extern int main (int argc, char **argv)
{
     char *filename;
     int verbose, baud;
     char buffer[256];
     int done;

     serlog = fopen("/tmp/serial.log", "w");
     outputlog = fopen("/tmp/output.log", "w");
     filename = "/dev/ttyS0";
     verbose = 0;
     baud = 9600;
     target_little_endian = 1;
     done = 0;
     while (!done)
     {
          switch (getopt_long( argc, argv, "", long_options, NULL ))
          {
          case -1:
               done = 1;
               break;
          case 'f':
               filename = optarg;
               break;
          case 'b':
               sscanf( optarg, "%d", &baud );
               break;
          case 'B':
               target_little_endian = 0;
               break;
          case 'v':
               verbose = 1;
               break;
          }

     }

     kbd_fd = 0;
     serial_fd = open_serial( filename, baud, verbose );
     if (serial_fd<0) exit(4);

     initscr();
     cbreak();
     text_window = newwin( 40, 0, 0, 0 );
     scrollok(text_window, 1);
     command_window = newwin( 10, 0, 40, 0 );
     scrollok(command_window, 1);

     text_mode_enter();
     serial_putchar( '\n' );
     while (1)
     {
          int i;
          if (poll_fd(kbd_fd, 1, 0, 0, 0)==2)
          {
               read( kbd_fd, buffer, 1 );
               switch (mode)
               {
               case text_mode:
                    text_mode_key( buffer[0] );
                    break;
               case cmd_mode:
                    command_mode_key( buffer[0] );
                    break;
               }
          }
          i = poll_fd(serial_fd, 1, 0, 0, 0);
          if ((i!=0) && (i!=2))
          {
               fprintf(stderr, "poll %d\n", i);
          }
          if (i==2)
          {
               read( serial_fd, buffer, 1 );
               if (buffer[0]!='\r')
               {
                    wechochar(text_window, buffer[0]);
                    fprintf(outputlog,"%c",buffer[0]);
               }
          }
     }
     close(serial_fd);
     return 0;
}
/*
 stty -F /dev/ttyS0 ispeed 19200 ospeed 19200
*/
     
