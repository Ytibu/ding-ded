#ifndef __EDITOR_H__
#define __EDITOR_H__

#include <stddef.h>
#include <stdio.h>

typedef struct {
    char *items;
    size_t count;
    size_t capacity;
} Data;

typedef struct{
    size_t begin;
    size_t end;
} Line_;

typedef struct {
    Line_ *items;
    size_t count;
    size_t capacity;
} Lines;

typedef struct {
    size_t capacity;
    size_t size;
    char *chars;
} Line;

typedef struct{
    Data data;
    Lines lines;
    size_t cursor;
} Editor;

#define DATA_INIT_CAP 1024
#define da_append(data_ptr, item) \
    do { \
        if ((data_ptr)->count == (data_ptr)->capacity) { \
            size_t new_capacity = (data_ptr)->capacity == 0 ? DATA_INIT_CAP : (data_ptr)->capacity * 2; \
            (data_ptr)->items = realloc((data_ptr)->items, new_capacity * sizeof(*(data_ptr)->items)); \
            assert((data_ptr)->items != NULL && "Failed to allocate memory"); \
            (data_ptr)->capacity = new_capacity; \
        } \
        (data_ptr)->items[(data_ptr)->count++] = (item); \
    } while(0)

void editor_recompute_lines(Editor *editor);
void editor_save_to_file(const Editor *editor, const char* filePath);
void editor_load_from_file(Editor *editor, FILE *file);
void editor_backspace(Editor *editor);
void editor_delete(Editor *editor);

size_t editor_cursor_row(const Editor *editor);
void editor_move_line_up(Editor *editor);
void editor_move_line_down(Editor *editor);
void editor_move_line_left(Editor *editor);
void editor_move_line_right(Editor *editor);
void editor_insert_char(Editor *editor, char c);

#endif // __EDITOR_H__