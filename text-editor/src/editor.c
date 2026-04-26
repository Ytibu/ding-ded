#include "./editor.h"

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "./file.h"

#define SV_IMPLEMENTATION
#include "./sv.h"

void editor_save_to_file(const Editor *editor, const char* filePath)
{
    FILE *file = fopen(filePath, "w");
    if(file == NULL){
        fprintf(stderr, "ERROR: can't open file '%s': %s\n", filePath, strerror(errno));
        exit(EXIT_FAILURE);
    }

    fwrite(editor->data.items, 1, editor->data.count, file);
    fclose(file);
}

static size_t file_size(FILE *file)
{
    // 截取当前位置，跳转到末尾并读取长度，最后恢复位置
    long saved = ftell(file);
    assert(saved >= 0 && "Failed to get file position");
    int err = fseek(file, 0, SEEK_END);
    assert(err == 0 && "Failed to seek to end of file");
    long result = ftell(file);
    assert(result >= 0 && "Failed to get file size");
    err = fseek(file, saved, SEEK_SET);
    assert(err == 0 && "Failed to restore file position");
    return result;
}

void editor_load_from_file(Editor *editor, FILE *file)
{
    editor->data.count = 0;
    size_t size = file_size(file);
    if(editor->data.capacity < size){
        editor->data.capacity = size;
        editor->data.items = realloc(editor->data.items, editor->data.capacity * sizeof(*editor->data.items));
        assert(editor->data.items != NULL && "Failed to allocate memory");
    }

    fread(editor->data.items, 1, size, file);
    editor->data.count = size;

    editor_recompute_lines(editor);
}

size_t editor_cursor_row(const Editor *editor)
{
    if (editor->lines.count == 0) return 0;
    for(size_t row = 0; row < editor->lines.count; ++row){
        Line_ line = editor->lines.items[row];  // 获取当前行的起始和结束位置
        if(line.begin <= editor->cursor && editor->cursor <= line.end){
            return row;
        }
    }

    return editor->lines.count - 1;
}

void editor_move_line_up(Editor *editor)
{
    size_t row = editor_cursor_row(editor);
    size_t col = editor->cursor - editor->lines.items[row].begin;
    if(row > 0){
        Line_ prev_line = editor->lines.items[row - 1];
        size_t prev_line_len = prev_line.end - prev_line.begin;
        if(col > prev_line_len){
            col = prev_line_len;
        }
        editor->cursor = prev_line.begin + col;
    }

}

void editor_move_line_down(Editor *editor)
{
    size_t row = editor_cursor_row(editor);
    size_t col = editor->cursor - editor->lines.items[row].begin;
    if(row + 1 < editor->lines.count){
        Line_ next_line = editor->lines.items[row + 1];
        size_t next_line_len = next_line.end - next_line.begin;
        if(col > next_line_len){
            col = next_line_len;
        }
        editor->cursor = next_line.begin + col;
    }
}

void editor_move_line_left(Editor *editor)
{
    size_t row = editor_cursor_row(editor);
    Line_ line = editor->lines.items[row];
    
    // 如果光标在行首，不能继续向左移动
    if (editor->cursor > line.begin) {
        editor->cursor--;
    }
}

void editor_move_line_right(Editor *editor)
{
    size_t row = editor_cursor_row(editor);
    Line_ line = editor->lines.items[row];
    
    // 如果光标在行尾，不能继续向右移动
    if (editor->cursor < line.end) {
        editor->cursor++;
    }
}

void editor_backspace(Editor *editor)
{
    if(editor->cursor > 0){
        memmove(editor->data.items + editor->cursor - 1,
                editor->data.items + editor->cursor,
                editor->data.count - editor->cursor
        );
        editor->data.count--;
        editor->cursor--;

        editor_recompute_lines(editor);
    }
}

void editor_delete(Editor *editor)
{
    if(editor->cursor < editor->data.count){
        memmove(editor->data.items + editor->cursor,
                editor->data.items + editor->cursor + 1,
                editor->data.count - editor->cursor - 1
        );
        editor->data.count--;

        editor_recompute_lines(editor);
    }
}

void editor_insert_char(Editor *editor, char c)
{
    if(editor->cursor > editor->data.count){
        editor->cursor = editor->data.count;
    }
    da_append(&editor->data, '\0');
    memmove(editor->data.items + editor->cursor + 1,
            editor->data.items + editor->cursor,
            editor->data.count - editor->cursor - 1
    );
    editor->data.items[editor->cursor] = c;
    editor->cursor++;

    editor_recompute_lines(editor);
}

void editor_recompute_lines(Editor *editor)
{
    // 如果 lines 未初始化，自动初始化
    if (editor->lines.items == NULL) {
        editor->lines.capacity = 16;
        editor->lines.items = calloc(editor->lines.capacity, sizeof(*editor->lines.items));
        assert(editor->lines.items != NULL && "Failed to allocate memory for editor lines");
    }
    
    editor->lines.count = 0;

    Line_ line;
    line.begin = 0;

    for(size_t i = 0; i < editor->data.count; ++i){
        if(editor->data.items[i] == '\n'){
            line.end = i;
            da_append(&editor->lines, line);
            line.begin = i + 1;
        }
    }

    line.end = editor->data.count;
    da_append(&editor->lines, line);
}