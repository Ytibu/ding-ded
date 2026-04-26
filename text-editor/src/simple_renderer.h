#ifndef __SIMPLE_RENDERER_H__
#define __SIMPLE_RENDERER_H__

#include <GL/glew.h>

#include "./la.h"
#include "./uniforms.h"

typedef enum {
    SIMPLE_VERTEX_ATTR_POSITION = 0,
    SIMPLE_VERTEX_ATTR_COLOR = 1,
    SIMPLE_VERTEX_ATTR_UV = 2
} Simple_Vertex_Attr;

typedef struct{
    Vec2f position;
    Vec4f color;
    Vec2f uv;
} Simple_Vertex;

typedef struct{
    Simple_Vertex vertices[3];
} Simple_Triangle;

typedef struct{
    Simple_Vertex *items;
    size_t count;
    size_t capacity;
} Simple_Triangles;

#define SIMPLE_TRIANGLES_CAPACITY (1000*640)
typedef struct {
    GLuint vao;
    GLuint vbo;
    GLuint program;

    GLint uniforms[COUNT_UNIFORM_SLOTS];
    Simple_Triangle triangles[SIMPLE_TRIANGLES_CAPACITY];
    size_t triangles_count;
} Simple_Renderer;

void simple_renderer_init(Simple_Renderer *renderer, const char *vertex_filePath, const char *fragment_filePath);
void simple_renderer_use(const Simple_Renderer *renderer);
void simple_renderer_triangle(Simple_Renderer *renderer,
                            Vec2f p0, Vec2f p1, Vec2f p2,
                            Vec4f c0, Vec4f c1, Vec4f c2,
                            Vec2f uv0, Vec2f uv1, Vec2f uv2);
void simple_renderer_sync(Simple_Renderer *renderer);
void simple_renderer_draw(Simple_Renderer *renderer);
void simple_renderer_clear(Simple_Renderer *renderer);

#endif // __SIMPLE_RENDERER_H__