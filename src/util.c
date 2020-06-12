#include <stdlib.h>
#include <stdio.h>
#include <tty.h>

void die(const char* s) {
     tty_clear_screen();
     tty_set_curpos(1,1);
     perror(s);
     exit(1);
}
