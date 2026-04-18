#include "Editor.h"

#include <algorithm>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void Line::line_insert_text_before(const char *text, size_t *col)
{
    if (text == NULL || col == NULL)
    {
        return;
    }

    if (*col > chars.size())
    {
        *col = chars.size();
    }

    const size_t text_size = strlen(text);
    chars.insert(*col, text, text_size);
    *col += text_size;
}

void Line::line_backspace(size_t *col)
{
    if (col == NULL)
    {
        return;
    }

    if (*col > chars.size())
    {
        *col = chars.size();
    }

    if (*col > 0 && !chars.empty())
    {
        chars.erase(*col - 1, 1);
        *col -= 1;
    }
}

void Line::line_delete(size_t *col)
{
    if (col == NULL)
    {
        return;
    }

    if (*col > chars.size())
    {
        *col = chars.size();
    }

    if (*col < chars.size() && !chars.empty())
    {
        chars.erase(*col, 1);
    }
}

const char *Line::line_data() const
{
    return chars.c_str();
}

size_t Line::line_size() const
{
    return chars.size();
}

void Editor::editor_ensure_editable_line()
{
    if (lines.empty())
    {
        lines.push_back(Line());
        cursor_row = 0;
        cursor_col = 0;
        return;
    }

    if (cursor_row >= lines.size())
    {
        cursor_row = lines.size() - 1;
    }

    const size_t line_len = lines[cursor_row].line_size();
    if (cursor_col > line_len)
    {
        cursor_col = line_len;
    }
}

void Editor::editor_save_to_file(const char *filePath) const
{
    FILE *file = fopen(filePath, "w");
    if (file == NULL)
    {
        fprintf(stdout, "ERROR: can't open file '%s': %s\n", filePath, strerror(errno));
        exit(1);
    }

    for (size_t row = 0; row < lines.size(); ++row)
    {
        fwrite(lines[row].line_data(), 1, lines[row].line_size(), file);
        fputc('\n', file);
    }

    fclose(file);
}

void Editor::editor_insert_text_before_cursor(const char *text)
{
    editor_ensure_editable_line();
    lines[cursor_row].line_insert_text_before(text, &cursor_col);
}

void Editor::editor_backspace()
{
    editor_ensure_editable_line();
    lines[cursor_row].line_backspace(&cursor_col);
}

void Editor::editor_delete()
{
    editor_ensure_editable_line();
    lines[cursor_row].line_delete(&cursor_col);
}

void Editor::editor_insert_new_line()
{
    if (cursor_row >= lines.size())
    {
        editor_push_new_line();
        cursor_row = lines.size() - 1;
        cursor_col = 0;
        return;
    }

    lines.insert(lines.begin() + static_cast<ptrdiff_t>(cursor_row + 1), Line());
    cursor_row += 1;
    cursor_col = 0;
}

void Editor::editor_push_new_line()
{
    lines.push_back(Line());
}

const char *Editor::editor_char_under_cursor() const
{
    if (cursor_row < lines.size())
    {
        const size_t line_len = lines[cursor_row].line_size();
        if (cursor_col < line_len)
        {
            return lines[cursor_row].line_data() + cursor_col;
        }
    }

    return NULL;
}

size_t Editor::editor_line_count() const
{
    return lines.size();
}

const char *Editor::editor_line_data(size_t row) const
{
    if (row < lines.size())
    {
        return lines[row].line_data();
    }

    return NULL;
}

size_t Editor::editor_line_size(size_t row) const
{
    if (row < lines.size())
    {
        return lines[row].line_size();
    }

    return 0;
}

size_t Editor::editor_cursor_row() const
{
    return cursor_row;
}

size_t Editor::editor_cursor_col() const
{
    return cursor_col;
}

void Editor::editor_move_up()
{
    if (cursor_row > 0)
    {
        cursor_row -= 1;
        if (cursor_row < lines.size())
        {
            cursor_col = std::min(cursor_col, lines[cursor_row].line_size());
        }
    }
}

void Editor::editor_move_down()
{
    if (cursor_row + 1 < lines.size())
    {
        cursor_row += 1;
        cursor_col = std::min(cursor_col, lines[cursor_row].line_size());
    }
}

void Editor::editor_move_left()
{
    if (cursor_col > 0)
    {
        cursor_col -= 1;
    }
}

void Editor::editor_move_right()
{
    cursor_col += 1;
}
