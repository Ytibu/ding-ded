#ifndef __SIMPLE_RENDERER_H__
#define __SIMPLE_RENDERER_H__

#include <GL/glew.h>
#include "./la.h"
#include "./head.h"
#include "./sdl_extra.h"

typedef enum {
    UNIFORM_SLOT_TIME = 0,
    UNIFORM_SLOT_RESOLUTION,
    UNIFORM_SLOT_CAMERA_POS,
    UNIFORM_SLOT_CAMERA_SCALE,
    COUNT_UNIFORM_SLOTS,
} Uniform_Slot;

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

typedef enum {
    SHADER_FOR_COLOR = 0,
    SHADER_FOR_IMAGE = 1,
    SHADER_FOR_EPICNESS = 2,
    COUNT_SIMPLE_SHADERS = 3
} Simple_Shader;

#define SIMPLE_TRIANGLES_CAPACITY (1000*640*3)
typedef struct {
    GLuint vao;
    GLuint vbo;
    GLuint program[COUNT_SIMPLE_SHADERS];
    Simple_Shader current_shader;

    GLint uniforms[COUNT_UNIFORM_SLOTS];
    Simple_Vertex vertices[SIMPLE_TRIANGLES_CAPACITY];
    size_t vertices_count;

    Vec2f resolution;
    float time;

    Vec2f camera_pos;
    float camera_scale;
    float camera_scale_vel;
    Vec2f camera_vel;
} Simple_Renderer;

void simple_renderer_init(Simple_Renderer *sr,
    const char *vertex_filePath,
    const char *colorFrag_filePath,
    const char *imageFrag_filePath,
    const char *epicFrag_filePath);
void simple_renderer_use(const Simple_Renderer *sr);
void simple_renderer_vertex(Simple_Renderer *sr, Vec2f position, Vec4f color, Vec2f uv);
void simple_renderer_set_shader(Simple_Renderer *sr, Simple_Shader shader);
void simple_renderer_triangle(Simple_Renderer *sr,
                            Vec2f p0, Vec2f p1, Vec2f p2,
                            Vec4f c0, Vec4f c1, Vec4f c2,
                            Vec2f uv0, Vec2f uv1, Vec2f uv2);
void simple_renderer_quad(Simple_Renderer *sr,
                            Vec2f p0, Vec2f p1, Vec2f p2, Vec2f p3,
                            Vec4f c0, Vec4f c1, Vec4f c2, Vec4f c3,
                            Vec2f uv0, Vec2f uv1, Vec2f uv2, Vec2f uv3);
void simple_renderer_solid_rect(Simple_Renderer *sr, Vec2f pos, Vec2f size, Vec4f color);
void simple_renderer_image_rect(Simple_Renderer *sr, Vec2f pos, Vec2f size, Vec2f uv_pos, Vec2f uv_size);
void simple_renderer_flush(Simple_Renderer *sr);
void simple_renderer_sync(Simple_Renderer *sr);
void simple_renderer_draw(Simple_Renderer *sr);

#endif // __SIMPLE_RENDERER_H__