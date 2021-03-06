#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <malloc.h>
#include <string.h>
#include <stdbool.h>
#include <readline/readline.h>
#include <tty.h>
#include <buffer.h>
#include <termios.h>

bool insert_mode = false;
bool line_num    = true;

char status_msg[300];
char *search_str   = NULL;

gim_buffer_t* buf = NULL;

void init_gim() {
     enable_raw_mode();
     tty_echo_off();
     tty_clear_screen();
     tty_set_curpos(1,1);
}

void close_gim() {
     tty_clear_screen();
     tty_set_curpos(1,1);
     disable_raw_mode();
     exit(0);
}

void update_cursor(int key) {
     switch(key) {
        case PAGE_UP:
		gim_buffer_curs_up(buf,buf->screen_rows);
	break;
	case PAGE_DOWN:
		gim_buffer_curs_down(buf,buf->screen_rows);
	break;
	case ARROW_UP:
		gim_buffer_curs_up(buf,1);
	break;
	case ARROW_DOWN:
		gim_buffer_curs_down(buf,1);
	break;
	case ARROW_LEFT:
		gim_buffer_curs_left(buf,1);
	break;
	case ARROW_RIGHT:
		gim_buffer_curs_right(buf,1);
	break;
	case HOME_KEY:
		gim_buffer_curs_home(buf);
	break;
	case END_KEY:
		gim_buffer_curs_end(buf);
	break;
     }
}

void draw_rows() {
     tty_set_curpos(1,1);
     int y;
     for(y=buf->row_offset; y<buf->screen_rows+buf->row_offset; y++) {
         tty_clear_line();
         if( y >= buf->row_count) {
		 tty_write_str(ANSI_BLUE_COLOR);
     		 tty_write_str("~");
		 tty_write_str(ANSI_RESET_COLOR);
	 } else {
	    char lineno_buf[7];
            if(line_num) {
               snprintf(lineno_buf,7,"%4d ",y+1);
	       tty_write_str(ANSI_YELLOW_COLOR);
	       tty_write_str(lineno_buf);
	       tty_write_str(ANSI_RESET_COLOR);
               tty_write_strn_hl(buf->rows[y].render_str,buf->screen_cols-6);
	    } else {
               tty_write_strn_hl(buf->rows[y].render_str,buf->screen_cols-1);
	    }
	 }
	 if(y < ((buf->screen_rows+buf->row_offset)-1)) {
            tty_write_str("\r\n");
	 }
     } 
}

void proc_cmd(char* cmd) {
	if(cmd[0]=='q') close_gim(); 
	if(cmd[0]=='w') gim_save_buffer(buf);
	if(cmd[0]=='l') line_num = !line_num;
	if(cmd[0]=='d') gim_buffer_delete_row(buf, buf->row_y);
}

void insert_char(int c) {
     if(buf->row_y == buf->row_count || buf->row_y==-1) {
        gim_buffer_append_new_row(buf," ",1);
        if(buf->row_y==-1) buf->row_y=0;
     }
     gim_row_insert_char(&buf->rows[buf->row_y],buf->row_x,c); 
     update_cursor(ARROW_RIGHT);
}

void search_next() {
     if(search_str == NULL) return;

     char* match = NULL;
     int   x;

    
     for(int r=buf->row_y; r < buf->row_count-1; r++) {
         if(r==buf->row_y) {
           match = strstr(buf->rows[r].render_str+buf->row_x, search_str);
           if(match)
	      match = strstr(buf->rows[r].render_str+buf->row_x+1, search_str);
	 } else {
           match = strstr(buf->rows[r].render_str, search_str);
         }
         if(match) {
            gim_buffer_curs_down(buf,r-buf->row_y);
            x = gim_row_rx_to_cx(&buf->rows[r], match - buf->rows[r].render_str);
            buf->row_y        = r;
	    buf->screen_cur_y = r - buf->row_offset;
	    gim_buffer_curs_home(buf);
	    gim_buffer_curs_right(buf,x);
	    return;
	 }
     } 
}

void process_input() {
     int c = tty_read_key();

     switch(c) {
        case CTRL_KEY('q'):
	     close_gim();
	break;

        case INSERT_KEY:
             if(!insert_mode) insert_mode = true;
	break;

	case ESCAPE_KEY:
	     if(insert_mode) insert_mode = false;
	break;

	case BACKSPACE_KEY:
	case CTRL_KEY('h'):
	case DEL_KEY:
		if(buf->row_count>0) {
			if(c==DEL_KEY) gim_buffer_curs_right(buf,1);
			gim_row_delete_char(&buf->rows[buf->row_y], buf->screen_cur_x-1);
			gim_buffer_curs_left(buf,1);
			if(buf->rows[buf->row_y].chars_len == 1) gim_buffer_delete_row(buf, buf->row_y);
		}
	break;

        case HOME_KEY:
	case END_KEY:
	case PAGE_UP:
	case PAGE_DOWN:
	case ARROW_UP:
	case ARROW_DOWN:
	case ARROW_LEFT:
	case ARROW_RIGHT:
             update_cursor(c);
	break;
	case '\r':
	    if(insert_mode) {
              gim_buffer_insert_nl(buf);
	    } else {
	      gim_buffer_curs_down(buf,1);
	    }
	break;
	case '\t':
	    if(insert_mode) insert_char(c);
	break;
	case 32 ... 126:
            if(insert_mode) {
		insert_char(c);
	    } else {
		if(c=='i') {
		   insert_mode = true;
	        } else if(c==':') {
		   tty_set_curpos(1,buf->screen_rows+1);
		   tty_clear_line();	
		   tty_echo_on();
	           char* cmd_str = readline(":");
		   tty_echo_off();
		   proc_cmd(cmd_str);
		   free(cmd_str);
	        } else if(c=='/') {
                   tty_set_curpos(1,buf->screen_rows+1);
		   tty_clear_line();
		   tty_echo_on();
		   char* line = readline("/");
		   if(strlen(line)>1) {
                      if(search_str) free(search_str);
		      search_str = line;
		   }
		   tty_echo_off();
		   search_next();
		}
	    }
	break;
	default:
	break;
     }
}



void refresh_screen() {
//     tty_clear_screen();
     tty_disable_cursor();
     draw_rows();
     if(insert_mode) {
       snprintf(status_msg,300,"\n\r%d/%d %s --INSERT--",buf->row_y+1,buf->row_count,buf->filename);
     } else {
       snprintf(status_msg,300,"\n\r%d/%d %s           ",buf->row_y+1,buf->row_count,buf->filename);
     }
     tty_clear_line();
     tty_write_str(status_msg); 
     if(line_num) {
        tty_set_curpos(buf->row_x+6,buf->screen_cur_y+1);
     } else {
        tty_set_curpos(buf->row_x+1,buf->screen_cur_y+1);
     }
     tty_enable_cursor();
}

int main(int argc, char** argv) {
    init_gim();
    if(argc==2) {
	buf = gim_buffer_from_file(argv[1]);
    } else {
	buf = gim_new_buffer();
	buf->filename = strdup("");
    }
    for(;;) {
        refresh_screen();
        process_input();
    }
    return 0;
}
