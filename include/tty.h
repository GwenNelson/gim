#pragma once
#ifndef __TTY_H_
#define __TTY_H_

void enable_raw_mode();
void disable_raw_mode();

int  tty_read_key();
void tty_write_str(char* s);
void tty_write_strn(char* s, size_t n);

void tty_clear_screen();
void tty_clear_line();

void tty_set_curpos(int x, int y);
void tty_get_curpos(int* rows, int* cols);

void tty_enable_cursor();
void tty_disable_cursor();

void tty_echo_on();
void tty_echo_off();

void tty_get_size(int* rows, int* cols);

enum tty_key {
     ESCAPE_KEY    = 27,
     BACKSPACE_KEY = 127,
     ARROW_LEFT    = 1000,
     ARROW_RIGHT,
     ARROW_UP,
     ARROW_DOWN,
     INSERT_KEY,
     HOME_KEY,
     END_KEY,
     DEL_KEY,
     PAGE_UP,
     PAGE_DOWN
};

#define CTRL_KEY(k) ((k) & 0x1f)

#define ANSI_CLR_SCR      "\x1b[2J"
#define ANSI_SET_CURPOS   "\x1b[%d;%dH"
#define ANSI_GET_CURPOS   "\x1b[6n"
#define ANSI_CUR_FORWARD  "\x1b[%dC"
#define ANSI_CUR_DOWN     "\x1b[%dB"
#define ANSI_ECHO_ON      "\x1b[12h"
#define ANSI_ECHO_OFF     "\x1b[12l"
#define ANSI_CUR_HIDE     "\x1b[?25l"
#define ANSI_CUR_SHOW     "\x1b[?25h"
#define ANSI_CLR_LINE     "\x1b[K"
#define ANSI_RESET_COLOR  "\x1b[0m"
#define ANSI_BLUE_COLOR   "\x1b[34m"
#define ANSI_YELLOW_COLOR "\x1b[33m"

#endif
