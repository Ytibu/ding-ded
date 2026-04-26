#include "simple_renderer.h"

#include <stdio.h>
#include <assert.h>

#include "./head.h"
#include "./sdl_extra.h"

void simple_renderer_init(Simple_Renderer *renderer, const char *vertex_filePath, const char *fragment_filePath)
{
    {
        glGenVertexArrays(1, &renderer->vao);
        glBindVertexArray(renderer->vao);

        glGenBuffers(1, &renderer->vbo);
        glBindBuffer(GL_ARRAY_BUFFER, renderer->vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(renderer->triangles), renderer->triangles, GL_STREAM_DRAW);

        // position
        glEnableVertexAttribArray(SIMPLE_VERTEX_ATTR_POSITION);
        glVertexAttribPointer(SIMPLE_VERTEX_ATTR_POSITION, 2, GL_FLOAT, GL_FALSE, sizeof(Simple_Vertex), (GLvoid *)offsetof(Simple_Vertex, position));

        // color
        glEnableVertexAttribArray(SIMPLE_VERTEX_ATTR_COLOR);
        glVertexAttribPointer(SIMPLE_VERTEX_ATTR_COLOR, 4, GL_FLOAT, GL_FALSE, sizeof(Simple_Vertex), (GLvoid *)offsetof(Simple_Vertex, color));

        // uv
        glEnableVertexAttribArray(SIMPLE_VERTEX_ATTR_UV);
        glVertexAttribPointer(SIMPLE_VERTEX_ATTR_UV, 2, GL_FLOAT, GL_FALSE, sizeof(Simple_Vertex), (GLvoid *)offsetof(Simple_Vertex, uv));
    }

    GLuint shaders[2] = {0};
    if (!compile_shader_file(vertex_filePath, GL_VERTEX_SHADER, &shaders[0]) || !compile_shader_file(fragment_filePath, GL_FRAGMENT_SHADER, &shaders[1]))
    {
        ERROR("Failed to compile shader files");
    }

    renderer->program = glCreateProgram();
    attach_shaders_to_program(shaders, sizeof(shaders) / sizeof(shaders[0]), renderer->program);
    if (!link_program(renderer->program))
    {
        ERROR("Failed to link shader program");
    }

    glUseProgram(renderer->program);

    get_uniform_location(renderer->program, renderer->uniforms);
}

void simple_renderer_use(const Simple_Renderer *renderer)
{
    glBindVertexArray(renderer->vao);
    glBindBuffer(GL_ARRAY_BUFFER, renderer->vbo);
    glUseProgram(renderer->program);
}

void simple_renderer_triangle(Simple_Renderer *renderer,
                              Vec2f p0, Vec2f p1, Vec2f p2,
                              Vec4f c0, Vec4f c1, Vec4f c2,
                              Vec2f uv0, Vec2f uv1, Vec2f uv2)
{
    assert(renderer->triangles_count < SIMPLE_TRIANGLES_CAPACITY);
    Simple_Triangle *last = &renderer->triangles[renderer->triangles_count];
    last->vertices[0].position = p0;
    last->vertices[1].position = p1;
    last->vertices[2].position = p2;
    last->vertices[0].color = c0;
    last->vertices[1].color = c1;
    last->vertices[2].color = c2;
    last->vertices[0].uv = uv0;
    last->vertices[1].uv = uv1;
    last->vertices[2].uv = uv2;

    renderer->triangles_count++;
}

void simple_renderer_sync(Simple_Renderer *renderer)
{
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Simple_Triangle) * renderer->triangles_count, renderer->triangles);
}

void simple_renderer_draw(Simple_Renderer *renderer)
{
    glDrawArrays(GL_TRIANGLES, 0, (renderer->triangles_count * 3));
}
void simple_renderer_clear(Simple_Renderer *renderer)
{
    renderer->triangles_count = 0;
}