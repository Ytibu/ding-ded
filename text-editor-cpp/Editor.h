#ifndef __EDITOR_H__
#define __EDITOR_H__

#include <stddef.h>
#include <string>
#include <vector>

class Line
{
public:
    void line_insert_text_before(const char *text, size_t *col);
    void line_backspace(size_t *col);
    void line_delete(size_t *col);

    const char *line_data() const;
    size_t line_size() const;

private:
    std::string chars;
};

class Editor
{
public:
    Editor() : cursor_row(0), cursor_col(0) {}

    void editor_save_to_file(const char *filePath) const;
    void editor_insert_text_before_cursor(const char *text);
    void editor_backspace();
    void editor_delete();
    void editor_insert_new_line();
    void editor_push_new_line();
    const char *editor_char_under_cursor() const;

    size_t editor_line_count() const;
    const char *editor_line_data(size_t row) const;
    size_t editor_line_size(size_t row) const;

    size_t editor_cursor_row() const;
    size_t editor_cursor_col() const;
    void editor_move_up();
    void editor_move_down();
    void editor_move_left();
    void editor_move_right();

private:
    void editor_ensure_editable_line();

private:
    std::vector<Line> lines;
    size_t cursor_row;
    size_t cursor_col;
};

#endif // __EDITOR_H__