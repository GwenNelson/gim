#include <malloc.h>
#include <string.h>

#include <buffer.h>
#include <tty.h>

gim_buffer_t* gim_new_buffer() {
	gim_buffer_t* retval = calloc(1,sizeof(gim_buffer_t));
	tty_get_size(&retval->screen_rows, &retval->screen_cols);
        retval->screen_rows--; // save space for status line
	return retval;
}

void gim_delete_buffer(gim_buffer_t* buf) {
     for(int i=0; i < buf->row_count; i++) {
         free(buf->rows[i].chars);
	 free(buf->rows[i].render_str);
     }
     free(buf);
}

static void gim_row_update(gim_buffer_row_t* row) {
     int tabs = 0;
     int j;

     for(j=0; j< row->chars_len; j++) {
         if(row->chars[j] == '\t') tabs++;
     }

     row->render_str = realloc(row->render_str, row->chars_len + tabs*7 + 1);

     int idx = 0;
     for(j=0; j < row->chars_len; j++) {
         if(row->chars[j] == '\t') {
            row->render_str[idx++] = ' ';
	    while(idx % TAB_STOP != 0) row->render_str[idx++] = ' ';
	 } else {
            row->render_str[idx++] = row->chars[j];
	 }
     }
     row->render_str[idx] = '\0';
     row->render_len      = idx;
}

void gim_buffer_insert_row(gim_buffer_t* buf, char* data, int at, size_t len) {
     if(at < 0 || at > buf->row_count) return;
     buf->rows = realloc(buf->rows, sizeof(gim_buffer_row_t) * (buf->row_count + 1));
     memmove(&buf->rows[at + 1], &buf->rows[at], sizeof(gim_buffer_row_t) * (buf->row_count - at));

     buf->rows[at].chars_len = len;
     buf->rows[at].chars     = malloc(len + 1);
     memcpy(buf->rows[at].chars, data, len);
     buf->rows[at].chars[len] = '\0';

     buf->rows[at].render_len = 0;
     buf->rows[at].render_str = NULL;

     buf->row_count++;
     gim_row_update(&buf->rows[at]);
}

void gim_buffer_append_new_row(gim_buffer_t* buf, char* data, size_t len) {
     gim_buffer_insert_row(buf, data, buf->row_count, len);
}

void gim_buffer_insert_nl(gim_buffer_t* buf) {
     if(buf->screen_cur_x==0) {
        gim_buffer_insert_row(buf,"", buf->row_y,0);
     } else {
        gim_buffer_row_t* row = &buf->rows[buf->row_y];
	gim_buffer_insert_row(buf, &row->chars[buf->row_x], buf->row_y + 1, row->chars_len - buf->row_x);
        row = &buf->rows[buf->row_y];
        row->chars_len = buf->row_x;
        row->chars[row->chars_len] = '\0';
	gim_row_update(row);
     }
     gim_buffer_curs_down(buf,1);
     gim_buffer_curs_home(buf);
}

void gim_buffer_delete_row(gim_buffer_t* buf, int at) {
     if (at < 0 || at >= buf->row_count) return;
     free(buf->rows[at].chars);
     free(buf->rows[at].render_str);
     memmove(&buf->rows[at], &buf->rows[at+1], sizeof(gim_buffer_row_t) * (buf->row_count - at - 1));
     buf->row_count--;
}

gim_buffer_row_t* gim_buffer_get_row(gim_buffer_t* buf, int at) {
     if(at <0 || at >= buf->row_count) return NULL;
     return &buf->rows[at];
}

void gim_row_insert_char(gim_buffer_row_t* row, int at, int c) {
     if(at < 0 || at > row->chars_len) at = row->chars_len;
     row->chars = realloc(row->chars, row->chars_len + 2);
     memmove(&row->chars[at + 1], &row->chars[at], row->chars_len - at + 1);
     row->chars_len++;
     row->chars[at] = c;
     gim_row_update(row);
}

void gim_row_delete_char(gim_buffer_row_t* row, int at) {
     if(at < 0 || at >= row->chars_len) return;
     memmove(&row->chars[at], &row->chars[at + 1], row->chars_len - at);
     row->chars_len--;
     gim_row_update(row);
}

void gim_row_append_str(gim_buffer_row_t* row, char* s, size_t len) {
     row->chars = realloc(row->chars, row->chars_len + len + 1);
     memcpy(&row->chars[row->chars_len], s, len);
     row->chars_len += len;
     row->chars[row->chars_len] = '\0';
     gim_row_update(row);
}

void gim_buffer_scroll(gim_buffer_t* buf) {
     buf->row_y = buf->screen_cur_y + buf->row_offset;
     if(buf->screen_cur_x < 0) buf->screen_cur_x = 0;
   
     if(buf->screen_cur_x < 0) buf->screen_cur_x = 0;

     if(buf->screen_cur_y < 0) {
          buf->row_offset--;
          buf->screen_cur_y = 0;
     }
     if(buf->screen_cur_y >= (buf->screen_rows-1)) {
	  buf->row_offset++;
          buf->screen_cur_y = buf->screen_rows-1;
     }
     if(buf->row_offset < 0) buf->row_offset=0;
     buf->row_y = buf->screen_cur_y+buf->row_offset;

     if(buf->row_y >= 0) {
    	if(buf->screen_cur_x >= buf->rows[buf->row_y].chars_len)
	       	buf->screen_cur_x = buf->rows[buf->row_y].chars_len-1;
     } else {
         if(buf->screen_cur_x > (buf->screen_cols-1)) buf->screen_cur_x = buf->screen_cols-1;
     }
     buf->row_x = gim_row_cx_to_rx(&buf->rows[buf->row_y], buf->screen_cur_x);

}

void gim_buffer_curs_up(gim_buffer_t* buf, int count) {
     for(int i=0; i<count; i++) {
         buf->screen_cur_y--;
         buf->row_y = buf->screen_cur_y + buf->row_offset;
         gim_buffer_scroll(buf);
     }
}

void gim_buffer_curs_down(gim_buffer_t* buf, int count) {
     if(buf->row_count == 0) return;
   	for(int i=0; i<count; i++) {
         if(buf->row_y <= buf->row_count) {
            buf->screen_cur_y++;
	    buf->row_y = buf->screen_cur_y + buf->row_offset;
	 } else {
            gim_buffer_curs_up(buf,1);
	 }
	 gim_buffer_scroll(buf);
     }
}

void gim_buffer_curs_left(gim_buffer_t* buf, int count) {
     for(int i=0; i<count; i++) {
         buf->screen_cur_x--;
	 gim_buffer_scroll(buf);
     }
}

void gim_buffer_curs_right(gim_buffer_t* buf, int count) {
     for(int i=0; i<count; i++) {
         buf->screen_cur_x++;
	 gim_buffer_scroll(buf);
     }
}

void gim_buffer_curs_home(gim_buffer_t* buf) {
     buf->screen_cur_x = 0;
     gim_buffer_scroll(buf);
}

void gim_buffer_curs_end(gim_buffer_t* buf) {
     if(buf->row_y >= 0) {
        buf->screen_cur_x = buf->rows[buf->row_y].chars_len-1;
     } else {
        buf->screen_cur_x = buf->screen_cols - 1;
     }
     gim_buffer_scroll(buf);
}

int gim_row_cx_to_rx(gim_buffer_row_t* row, int cx) {
     int rx = 0;
     int j;
     for (j = 0; j < cx; j++) {
          if(row->chars[j] == '\t')
             rx += (TAB_STOP - 1) - (rx % TAB_STOP);
          rx++;
     }
     return rx;
}
