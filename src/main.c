#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <malloc.h>
#include <string.h>
#include <stdbool.h>
#include <readline/readline.h>
#include <tty.h>
#include <buffer.h>

bool insert_mode = false;

char status_msg[300];

gim_buffer_t* buf = NULL;

void init_gim() {
     enable_raw_mode();
     tty_echo_off();
     buf = gim_new_buffer();
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
     int i =0;
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
            tty_write_str("~");
	 } else {
            tty_write_strn(buf->rows[y].render_str,buf->screen_cols-1);
	 }
	 if(y < ((buf->screen_rows+buf->row_offset)-1)) {
            tty_write_str("\r\n");
	 }
     } 
}

void proc_cmd(char* cmd) {
	if(cmd[0]=='q') close_gim(); 
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
		if(c==DEL_KEY) gim_buffer_curs_right(buf,1);
		gim_row_delete_char(&buf->rows[buf->row_y], buf->row_x-1);
		gim_buffer_curs_left(buf,1);
		if(buf->rows[buf->row_y].chars_len == 1) gim_buffer_delete_row(buf, buf->row_y);
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
	case 32 ... 126:
            if(insert_mode) {
	       if(buf->row_y == buf->row_count || buf->row_y==-1) {
		    gim_buffer_append_new_row(buf," ",1);
                    if(buf->row_y==-1) buf->row_y=0;
	       }
               gim_row_insert_char(&buf->rows[buf->row_y],buf->screen_cur_x,c); 
               update_cursor(ARROW_RIGHT);
	    } else {
               if(c==':') {
		tty_set_curpos(1,buf->screen_rows+1);
		tty_clear_line();	
		tty_echo_on();
		char* cmd_str = readline(":");
		tty_echo_off();
		proc_cmd(cmd_str);
		free(cmd_str);
	       }
	    }
	break;
	default:
	break;
     }
}



void refresh_screen() {
     tty_disable_cursor();
     tty_clear_screen();
     draw_rows();
     if(insert_mode) {
       snprintf(status_msg,300,"\n\r%d/%d --INSERT--",buf->row_y+1,buf->row_count);
     } else {
       snprintf(status_msg,300,"\n\r%d/%d",buf->row_y+1,buf->row_count);
     }
     tty_write_str(status_msg); 
     tty_set_curpos(buf->row_x+1,buf->screen_cur_y+1);
     tty_enable_cursor();
}

void open_file(char* filename) {
     FILE* fd = fopen(filename, "r");
     size_t line_len = 0;
     char* line = NULL;
     while(getline(&line,&line_len,fd) != -1) {
          char* endline = strrchr(line,'\n');
	  if(endline != NULL) *endline=0;
	  endline = strrchr(line,'\r');
	  if(endline != NULL) *endline=0;
	  gim_buffer_append_new_row(buf,line,strlen(line)+1); 
     }
     free(line);
     buf->row_y = 0;
}

int main(int argc, char** argv) {
    init_gim();
    if(argc==2) open_file(argv[1]);
    for(;;) {
        refresh_screen();
        process_input();
    }
    return 0;
}