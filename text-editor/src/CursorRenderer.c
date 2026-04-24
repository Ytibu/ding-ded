#include "./CursorRenderer.h"

#include <stdio.h>
#include <stdlib.h>

#include "./sdl_extra.h"
#include "./font.h"

void cursor_renderer_init(Cursor_Renderer *cursor, const char *vertex_shader_path, const char *fragment_shader_path)
{
    GLuint vert_shader = 0, frag_shader = 0;
    // 编译顶点着色器
    if (!compile_shader_file(vertex_shader_path, GL_VERTEX_SHADER, &vert_shader))
    {
        fprintf(stderr, "Failed to compile vertex shader\n");
        exit(EXIT_FAILURE);
    }
    // 编译片元着色器
    if (!compile_shader_file(fragment_shader_path, GL_FRAGMENT_SHADER, &frag_shader))
    {
        fprintf(stderr, "Failed to compile fragment shader\n");
        exit(EXIT_FAILURE);
    }
    // 链接着色器程序
    if (!link_program(vert_shader, frag_shader, &cursor->program))
    {
        fprintf(stderr, "Failed to link shader program\n");
        exit(EXIT_FAILURE);
    }

    glUseProgram(cursor->program);

    // 获取 uniform 变量位置
    cursor->time_uniform = glGetUniformLocation(cursor->program, "time");
    cursor->resolution_uniform = glGetUniformLocation(cursor->program, "resolution");
    cursor->camera_uniform = glGetUniformLocation(cursor->program, "camera");
    cursor->pos_uniform = glGetUniformLocation(cursor->program, "pos");
    cursor->height_uniform = glGetUniformLocation(cursor->program, "height");
    cursor->last_stroke_uniform = glGetUniformLocation(cursor->program, "last_stroke");
}
void cursor_renderer_use(const Cursor_Renderer *cursor)
{
    glUseProgram(cursor->program);
}

void cursor_renderer_move(const Cursor_Renderer *cursor, Vec2f pos)
{
    glUniform2f(cursor->pos_uniform, pos.x, pos.y);
}

void cursor_renderer_draw()
{
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}