#include "curses.h"

extern WINDOW *command_window;
extern WINDOW *text_window;
extern int mode;
extern int kbd_fd, serial_fd;
extern FILE *serlog;
