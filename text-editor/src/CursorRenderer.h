#ifndef __CURSOR_RENDERER_H__
#define __CURSOR_RENDERER_H__

#include <GL/glew.h>

#include "./la.h"
#include "./uniforms.h"

typedef struct{
    GLuint program;

    GLint uniforms[COUNT_UNIFORM_SLOTS];
} Cursor_Renderer;

void cursor_renderer_init(Cursor_Renderer *cursor,
                        const char *vertex_shader_path,
                        const char *fragment_shader_path);
void cursor_renderer_use(const Cursor_Renderer *cursor);
void cursor_renderer_move(const Cursor_Renderer *cursor, Vec2f pos);
void cursor_renderer_draw();

#endif  // __CURSOR_RENDERER_H__