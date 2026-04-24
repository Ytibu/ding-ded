#include "./CursorRenderer.h"

#include <stdio.h>
#include <stdlib.h>

#include "./sdl_extra.h"
#include "./font.h"

void cursor_renderer_init(Cursor_Renderer *cursor, const char *vertex_shader_path, const char *fragment_shader_path)
{
    GLuint shaders[3] = {0};
    // 编译顶点着色器
    if (!compile_shader_file(vertex_shader_path, GL_VERTEX_SHADER, &shaders[0]))
    {
        exit(1);
    }
    if (!compile_shader_file(fragment_shader_path, GL_FRAGMENT_SHADER, &shaders[1]))
    {
        exit(1);
    }
    if (!compile_shader_file("./shaders/camera.vert", GL_VERTEX_SHADER, &shaders[2]))
    {
        exit(1);
    }

    cursor->program = glCreateProgram();
    attach_shaders_to_program(shaders, sizeof(shaders) / sizeof(shaders[0]), cursor->program);
    if (!link_program(cursor->program))
    {
        exit(1);
    }

    glUseProgram(cursor->program);
    get_uniform_location(cursor->program, cursor->uniforms);
}
void cursor_renderer_use(const Cursor_Renderer *cursor)
{
    glUseProgram(cursor->program);
}

void cursor_renderer_move(const Cursor_Renderer *cursor, Vec2f pos)
{
    glUniform2f(cursor->uniforms[UNIFORM_SLOT_CURSOR_POS], pos.x, pos.y);
}

void cursor_renderer_draw()
{
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}