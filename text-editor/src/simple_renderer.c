#include "simple_renderer.h"

#include <stdio.h>
#include <assert.h>


typedef struct {
    Uniform_Slot slot;
    const char *name;
} Uniform_Def;

static_assert(COUNT_UNIFORM_SLOTS == 4, "The amount of the shader uniforms has changed. Please update the definition table accordingly");

static const Uniform_Def uniform_defs[COUNT_UNIFORM_SLOTS] = {
    [UNIFORM_SLOT_TIME] = {
        .slot = UNIFORM_SLOT_TIME,
        .name = "time",
    },
    [UNIFORM_SLOT_RESOLUTION] = {
        .slot = UNIFORM_SLOT_RESOLUTION,
        .name = "resolution",
    },
    [UNIFORM_SLOT_CAMERA_POS] = {
        .slot = UNIFORM_SLOT_CAMERA_POS,
        .name = "camera_pos",
    },
    [UNIFORM_SLOT_CAMERA_SCALE] = {
        .slot = UNIFORM_SLOT_CAMERA_SCALE,
        .name = "camera_scale",
    },
};

void get_uniform_location(GLuint program, GLint locations[COUNT_UNIFORM_SLOTS])
{
    for (Uniform_Slot slot = 0; slot < COUNT_UNIFORM_SLOTS; ++slot) {
        locations[slot] = glGetUniformLocation(program, uniform_defs[slot].name);
    }
}


void simple_renderer_init(Simple_Renderer *sr, const char *vertex_filePath, const char *colorFrag_filePath, const char *imageFrag_filePath, const char *epicFrag_filePath)
{
    sr->camera_scale = 3.0f;
    {
        glGenVertexArrays(1, &sr->vao);
        glBindVertexArray(sr->vao);

        glGenBuffers(1, &sr->vbo);
        glBindBuffer(GL_ARRAY_BUFFER, sr->vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(sr->vertices), sr->vertices, GL_STREAM_DRAW);

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
    if (!compile_shader_file(vertex_filePath, GL_VERTEX_SHADER, &shaders[0]))
    {
        ERROR("Failed to compile shader files");
    }

    // color shader
    {
        if (!compile_shader_file(colorFrag_filePath, GL_FRAGMENT_SHADER, &shaders[1]))
        {
            ERROR("Failed to compile color fragment shader");
        }
        sr->program[SHADER_FOR_COLOR] = glCreateProgram();
        attach_shaders_to_program(shaders, sizeof(shaders) / sizeof(shaders[0]), sr->program[SHADER_FOR_COLOR]);
        if (!link_program(sr->program[SHADER_FOR_COLOR]))
        {
            ERROR("Failed to link shader program for color");
        }
    }

    // image shader
    {
        if (!compile_shader_file(imageFrag_filePath, GL_FRAGMENT_SHADER, &shaders[1]))
        {
            ERROR("Failed to compile image fragment shader");
        }
        sr->program[SHADER_FOR_IMAGE] = glCreateProgram();
        attach_shaders_to_program(shaders, sizeof(shaders) / sizeof(shaders[0]), sr->program[SHADER_FOR_IMAGE]);
        if (!link_program(sr->program[SHADER_FOR_IMAGE]))
        {
            ERROR("Failed to link shader program for image");
        }
    }
    
    // epic shader
    {
        if (!compile_shader_file(epicFrag_filePath, GL_FRAGMENT_SHADER, &shaders[1]))
        {
            ERROR("Failed to compile epic fragment shader");
        }
        sr->program[SHADER_FOR_EPICNESS] = glCreateProgram();
        attach_shaders_to_program(shaders, sizeof(shaders) / sizeof(shaders[0]), sr->program[SHADER_FOR_EPICNESS]);
        if (!link_program(sr->program[SHADER_FOR_EPICNESS]))
        {
            ERROR("Failed to link shader program for epic");
        }
    }
}

void simple_renderer_use(const Simple_Renderer *sr)
{
    glBindVertexArray(sr->vao);
    glBindBuffer(GL_ARRAY_BUFFER, sr->vbo);
}
void simple_renderer_vertex(Simple_Renderer *sr, Vec2f position, Vec4f color, Vec2f uv)
{
    assert(sr->vertices_count < SIMPLE_TRIANGLES_CAPACITY);
    Simple_Vertex *last = &sr->vertices[sr->vertices_count];
    last->position = position;
    last->color = color;
    last->uv = uv;

    sr->vertices_count += 1;
}
void simple_renderer_set_shader(Simple_Renderer *sr, Simple_Shader shader)
{
    sr->current_shader = shader;
    glUseProgram(sr->program[shader]);
    get_uniform_location(sr->program[shader], sr->uniforms);
    glUniform2f(sr->uniforms[UNIFORM_SLOT_RESOLUTION], sr->resolution.x, sr->resolution.y);
    glUniform1f(sr->uniforms[UNIFORM_SLOT_TIME], sr->time);
    glUniform2f(sr->uniforms[UNIFORM_SLOT_CAMERA_POS], sr->camera_pos.x, sr->camera_pos.y);
    glUniform1f(sr->uniforms[UNIFORM_SLOT_CAMERA_SCALE], sr->camera_scale);
}

void simple_renderer_quad(Simple_Renderer *sr,
                          Vec2f p0, Vec2f p1, Vec2f p2, Vec2f p3,
                          Vec4f c0, Vec4f c1, Vec4f c2, Vec4f c3,
                          Vec2f uv0, Vec2f uv1, Vec2f uv2, Vec2f uv3)
{
    simple_renderer_triangle(sr, p0, p1, p2, c0, c1, c2, uv0, uv1, uv2);
    simple_renderer_triangle(sr, p1, p2, p3, c1, c2, c3, uv1, uv2, uv3);
}

void simple_renderer_triangle(Simple_Renderer *sr,
                              Vec2f p0, Vec2f p1, Vec2f p2,
                              Vec4f c0, Vec4f c1, Vec4f c2,
                              Vec2f uv0, Vec2f uv1, Vec2f uv2)
{
    simple_renderer_vertex(sr, p0, c0, uv0);
    simple_renderer_vertex(sr, p1, c1, uv1);
    simple_renderer_vertex(sr, p2, c2, uv2);
}

void simple_renderer_image_rect(Simple_Renderer *sr, Vec2f pos, Vec2f size, Vec2f uv_pos, Vec2f uv_size)
{
    Vec4f uv = vec4fs(0.0f);
    simple_renderer_quad(
        sr,
        pos, vec2f_add(pos, vec2f(size.x, 0.0f)), vec2f_add(pos, vec2f(0.0f, size.y)), vec2f_add(pos, size),
        uv, uv, uv, uv,
        uv_pos, vec2f_add(uv_pos, vec2f(uv_size.x, 0.0f)), vec2f_add(uv_pos, vec2f(0.0f, uv_size.y)), vec2f_add(uv_pos, uv_size));
}

void simple_renderer_solid_rect(Simple_Renderer *sr, Vec2f pos, Vec2f size, Vec4f color)
{
    Vec2f uv = vec2f(0.0f, 0.0f);
    simple_renderer_quad(sr,
            pos, vec2f_add(pos, vec2f(size.x, 0.0f)), vec2f_add(pos, vec2f(0.0f, size.y)), vec2f_add(pos, size),
            color, color, color, color,
            uv, uv, uv, uv);
}

void simple_renderer_sync(Simple_Renderer *sr)
{
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Simple_Vertex) * sr->vertices_count, sr->vertices);
}

void simple_renderer_draw(Simple_Renderer *sr)
{
    glDrawArrays(GL_TRIANGLES, 0, sr->vertices_count);
}

void simple_renderer_flush(Simple_Renderer *sr)
{
    simple_renderer_sync(sr);
    simple_renderer_draw(sr);
    sr->vertices_count = 0;
}