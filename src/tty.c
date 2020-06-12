#include <termios.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

#include <util.h>
#include <tty.h>

static struct termios orig_termios;

void disable_raw_mode() {
     if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios) == -1) die("tcsetattr failed");
}

void enable_raw_mode() {
     if(tcgetattr(STDIN_FILENO, &orig_termios) == -1) die("tcgetattr failed");
     atexit(disable_raw_mode);

     struct termios raw = orig_termios;
     raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
     raw.c_oflag &= ~(OPOST);
     raw.c_cflag |= (CS8);
     raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
     raw.c_cc[VMIN] = 0;
     raw.c_cc[VTIME] = 1;
     if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) die("tcsetattr failed");
}

void tty_echo_on() {
     struct termios t;
     tcgetattr(STDIN_FILENO,&t);
     t.c_lflag |= ECHO;
     tcsetattr(STDIN_FILENO, TCSAFLUSH, &t);
}

void tty_echo_off() {
     struct termios t;
     tcgetattr(STDIN_FILENO,&t);
     t.c_lflag &= ~ECHO;
     tcsetattr(STDIN_FILENO, TCSAFLUSH, &t);

}

int tty_read_key() {
    ssize_t n;
    char   c;
    while((n = read(STDIN_FILENO, &c, 1)) != 1) {
       if(n == -1 && (errno != EAGAIN)) die("read failed");
    }
    if(c=='\x1b') {
      char seq[3];
      if(read(STDIN_FILENO, &seq[0], 1) != 1) return '\x1b';
      if(read(STDIN_FILENO, &seq[1], 1) != 1) return '\x1b';
       
      if(seq[0] == '[') {
          if(seq[1] >= '0' && seq[1] <= '9') {
             if(read(STDIN_FILENO, &seq[2], 1) != 1) return '\x1b';
             if(seq[2] == '~') {
                switch(seq[1]) {
                   case '1': return HOME_KEY;
                   case '2': return INSERT_KEY;
		   case '3': return DEL_KEY;
		   case '4': return END_KEY;
		   case '5': return PAGE_UP;
		   case '6': return PAGE_DOWN;
		   case '7': return HOME_KEY;
		   case '8': return END_KEY;
		}
	     }
	  } else {
             switch(seq[1]) {
                case 'A': return ARROW_UP;
                case 'B': return ARROW_DOWN;
	        case 'C': return ARROW_RIGHT;
	        case 'D': return ARROW_LEFT;
                case 'H': return HOME_KEY;
		case 'F': return END_KEY;
	     }
	  }
      } else if(seq[0] == 'O') {
          switch(seq[1]) {
             case 'H': return HOME_KEY;
	     case 'F': return END_KEY;
	  }
      } else {
         return '\x1b';
      }

    } 
    return c;
    
}

void tty_write_str(char *s) {
     write(STDOUT_FILENO, s, strlen(s));
}

void tty_write_strn(char* s, size_t n) {
     size_t len = strlen(s);
     if(n>=len) {
	tty_write_str(s);
     } else {
        write(STDOUT_FILENO, s, n);
     }
}

size_t isdatatype(char* s) {
       if(strncmp(s,"char "   ,5)==0) return 4;
       if(strncmp(s,"char* "  ,6)==0) return 4;
       if(strncmp(s,"char** " ,7)==0) return 4;
       if(strncmp(s,"void "   ,5)==0) return 4;
       if(strncmp(s,"bool "   ,5)==0) return 4;
       if(strncmp(s,"int "    ,4)==0) return 3;
       if(strncmp(s,"ssize_t ",8)==0) return 7;
       if(strncmp(s,"typedef ",8)==0) return 7;
       if(strncmp(s,"struct " ,7)==0) return 6;
       return 0;
}

size_t iskeyword(char* s) {
       if(strncmp(s,"if( "   ,3)==0) return 2;
       if(strncmp(s,"if ("   ,4)==0) return 2;
       if(strncmp(s,"for ("  ,5)==0) return 3;
       if(strncmp(s,"for("   ,4)==0) return 3;
       if(strncmp(s,"else "  ,5)==0) return 4;
       if(strncmp(s,"switch ",7)==0) return 6;
       if(strncmp(s,"switch(",7)==0) return 6;
       if(strncmp(s,"case "  ,5)==0) return 4;
       if(strncmp(s,"break;" ,6)==0) return 5;
       if(strncmp(s,"return ",7)==0) return 6;
       if(strncmp(s,"return;",7)==0) return 6;
       return 0;
}

size_t ispreproc(char* s) {
       if(strncmp(s,"#include "    ,9)==0) return 8;
       if(strncmp(s,"#define "     ,8)==0) return 7;
       if(strncmp(s,"#pragma once",12)==0) return 12;
       if(strncmp(s,"#ifdef "      ,7)==0) return 6;
       if(strncmp(s,"#ifndef "     ,8)==0) return 7;
       if(strncmp(s,"#endif"       ,6)==0) return 6;
       return 0;
}

void tty_write_strn_hl(char* s, size_t n) {
     char* c = s;
     size_t len = 0;
     size_t keylen = 0;
     while(*c) {
        if(*c == '\"') {
          tty_write_str(ANSI_MAGENTA_COLOR);
          write(STDOUT_FILENO,c,1);
          c++;
	  while(*c) {
             if(*c == '\\') {
                write(STDOUT_FILENO, c, 1);
                c++;
		write(STDOUT_FILENO, c, 1);
	     } else if (*c == '\"') {
                write(STDOUT_FILENO, c, 1);
                c++;
		break;
	     } else {
                write(STDOUT_FILENO, c, 1);
		c++;
	     }
	  }
	  tty_write_str(ANSI_RESET_COLOR);
	} else if(isdigit(*c)) {
          tty_write_str(ANSI_MAGENTA_COLOR);
	  write(STDOUT_FILENO, c, 1);
	  tty_write_str(ANSI_RESET_COLOR);
	  c++;
	} else if((keylen=isdatatype(c)) >0 ) {
          tty_write_str(ANSI_GREEN_COLOR);
          write(STDOUT_FILENO, c, keylen);
          tty_write_str(ANSI_RESET_COLOR);
	  c+=keylen;
        } else if((keylen=iskeyword(c)) >0 ) {
          tty_write_str(ANSI_YELLOW_COLOR);
	  write(STDOUT_FILENO, c, keylen);
	  tty_write_str(ANSI_RESET_COLOR);
	  c+=keylen; 
        } else if((keylen=ispreproc(c)) >0 ) {
          tty_write_str(ANSI_BLUE_COLOR);
	  write(STDOUT_FILENO, c, keylen);
	  tty_write_str(ANSI_RESET_COLOR);
	  c+=keylen;
	} else {
          tty_write_str(ANSI_RESET_COLOR);
	  write(STDOUT_FILENO, c, 1);
	  c++;
	}
        tty_write_str(ANSI_RESET_COLOR);
        len++;
	if(len>n) return;
     }
}

void tty_clear_screen() {
     tty_write_str(ANSI_CLR_SCR);
}

void tty_clear_line() {
     tty_write_str(ANSI_CLR_LINE);
}

void tty_set_curpos(int x, int y) {
     char ansi_str[32];
     snprintf(ansi_str,32,ANSI_SET_CURPOS,y,x);
     tty_write_str(ansi_str);
}

void tty_get_curpos(int* rows, int* cols) {
     char buf[32];
     unsigned int i=0;

     tty_write_str(ANSI_GET_CURPOS);

     while (i < sizeof(buf) - 1) {
         if (read(STDIN_FILENO, &buf[i], 1) != 1) break;
         if (buf[i] == 'R') break;
         i++;
      }
     buf[i] = '\0';

     if (buf[0] != '\x1b' || buf[1] != '[') return;
     if (sscanf(&buf[2], "%d;%d", rows, cols) != 2) return;
}

void tty_get_size(int* rows, int* cols) {
     tty_set_curpos(1,1);
     char ansi_str[32];
     snprintf(ansi_str,32,ANSI_CUR_FORWARD,999);
     tty_write_str(ansi_str);

     snprintf(ansi_str,32,ANSI_CUR_DOWN,999);
     tty_write_str(ansi_str);
     tty_get_curpos(rows, cols);
}

void tty_disable_cursor() {
     tty_write_str(ANSI_CUR_HIDE);
}

void tty_enable_cursor() {
     tty_write_str(ANSI_CUR_SHOW);
}
