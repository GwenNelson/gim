#pragma once
#ifndef __BUFFER_H_
#define __BUFFER_H_

#define TAB_STOP 8

typedef struct gim_buffer_row_t {
	char*        chars;
	int          chars_len;
	char*        render_str;
	int          render_len;
} gim_buffer_row_t;

typedef struct gim_buffer_t {
	int screen_cur_x;
	int screen_cur_y;
	int screen_rows;
	int screen_cols;
	int row_offset;
	int col_offset;
	int row_x;
	int row_y;
	int row_count;
	gim_buffer_row_t* rows;
} gim_buffer_t;

gim_buffer_t* gim_new_buffer();
void          gim_delete_buffer(gim_buffer_t* buf);

void          gim_buffer_append_new_row(gim_buffer_t* buf, char* data, size_t len);
void          gim_buffer_insert_row    (gim_buffer_t* buf, char* data, int at, size_t len);
void          gim_buffer_delete_row    (gim_buffer_t* buf, int at);

void          gim_buffer_insert_nl(gim_buffer_t* buf);

void          gim_buffer_curs_home (gim_buffer_t* buf);
void          gim_buffer_curs_end  (gim_buffer_t* buf);

void          gim_buffer_curs_up   (gim_buffer_t* buf, int count);
void          gim_buffer_curs_down (gim_buffer_t* buf, int count);
void          gim_buffer_curs_left (gim_buffer_t* buf, int count);
void          gim_buffer_curs_right(gim_buffer_t* buf, int count);

gim_buffer_row_t* gim_buffer_get_row(gim_buffer_t* buf, int at);

void gim_row_insert_char(gim_buffer_row_t* row, int at, int c);
void gim_row_delete_char(gim_buffer_row_t* row, int at);

void gim_row_append_str(gim_buffer_row_t* row, char* s, size_t len);

int gim_row_cx_to_rx(gim_buffer_row_t* row, int cx);
#endif
