#include "tileGlyph.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "./stb_image.h"

#include "./font.h"
#include "./sdl_extra.h"

const static Glyph_Attr_Def glyph_attr_defs[COUNT_TILE_GLYPH_ATTRS] = {
    [TILE_GLYPH_ATTR_TILE] = {offsetof(Tile_Glyph, tile), 2, GL_INT},
    [TILE_GLYPH_ATTR_CH] = {offsetof(Tile_Glyph, ch), 1, GL_INT},
    [TILE_GLYPH_ATTR_FG_COLOR] = {offsetof(Tile_Glyph, fg_color), 4, GL_FLOAT},
    [TILE_GLYPH_ATTR_BG_COLOR] = {offsetof(Tile_Glyph, bg_color), 4, GL_FLOAT},
};

static_assert(COUNT_TILE_GLYPH_ATTRS == 4, "tile_glyph_attr_defs size must match Tile_Glyph_Attr count");

void tile_glyph_buffer_init(Tile_Glyph_Buffer *tgb, const char *texture_atlas_path,
                            const char *vertex_shader_path, const char *fragment_shader_path)
{
    glGenVertexArrays(1, &tgb->vao);
    glBindVertexArray(tgb->vao);

    glGenBuffers(1, &tgb->vbo);
    glBindBuffer(GL_ARRAY_BUFFER, tgb->vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(tgb->glyphs), tgb->glyphs, GL_DYNAMIC_DRAW);

    for (Tile_Glyph_Attr attr = 0; attr < COUNT_TILE_GLYPH_ATTRS; ++attr)
    {
        const Glyph_Attr_Def def = glyph_attr_defs[attr];
        glEnableVertexAttribArray(attr);
        // 根据属性定义选择正确的顶点属性指针类型
        if (def.type == GL_INT)
        {
            glVertexAttribIPointer(attr, def.comps, def.type, sizeof(Tile_Glyph), (void *)def.offset);
        }
        else
        {
            glVertexAttribPointer(attr, def.comps, def.type, GL_FALSE, sizeof(Tile_Glyph), (void *)def.offset);
        }
        glVertexAttribDivisor(attr, 1);
    }

    //////////////////////////
    GLuint vert_shader = 0, frag_shader = 0, shader_program = 0;
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
    if (!link_program(vert_shader, frag_shader, &shader_program))
    {
        fprintf(stderr, "Failed to link shader program\n");
        exit(EXIT_FAILURE);
    }

    glUseProgram(shader_program);
    GLint font_uniform = glGetUniformLocation(shader_program, "font");
    if (font_uniform != -1)
    {
        glUniform1i(font_uniform, 0);
    }

    // 获取 uniform 变量位置
    tgb->time_uniform = glGetUniformLocation(shader_program, "time");
    tgb->resolution_uniform = glGetUniformLocation(shader_program, "resolution");
    tgb->scale_uniform = glGetUniformLocation(shader_program, "scale");
    tgb->camera_uniform = glGetUniformLocation(shader_program, "camera");
    // 设置缩放 uniform
    glUniform2f(tgb->scale_uniform, FONT_SCALE, FONT_SCALE);


    //////////////////////////////////////////////////
    int width, height, channels;
    unsigned char *data = stbi_load(texture_atlas_path, &width, &height, &channels, STBI_rgb_alpha);
    if (data == NULL)
    {
        fprintf(stderr, "ERROR: could not load file %s: %s\n", texture_atlas_path, stbi_failure_reason());
        exit(EXIT_FAILURE);
    }

    glActiveTexture(GL_TEXTURE0);

    GLuint font_texture = 0;
    glGenTextures(1, &font_texture);
    glBindTexture(GL_TEXTURE_2D, font_texture);

    // 设置纹理参数
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // 上传纹理数据
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

    stbi_image_free(data);
}

void tile_glyph_buffer_clear(Tile_Glyph_Buffer *tgb)
{
    tgb->glyphs_count = 0;
}

void tile_glyph_buffer_push(Tile_Glyph_Buffer *tgb, Tile_Glyph glyph)
{
    assert(tgb->glyphs_count < TILE_GLYPH_BUFFER_CAPACITY);
    tgb->glyphs[tgb->glyphs_count++] = glyph;
}

void tile_glyph_buffer_sync(Tile_Glyph_Buffer *tgb)
{
    glBufferSubData(GL_ARRAY_BUFFER, 0, tgb->glyphs_count * sizeof(Tile_Glyph), tgb->glyphs);
}

void tile_glyph_buffer_draw(Tile_Glyph_Buffer *tgb)
{
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, (GLsizei)tgb->glyphs_count);
}

void tile_glyph_render_line_sized(Tile_Glyph_Buffer *tgb, const char *text, size_t text_size, Vec2i tile, Vec4f fg_color, Vec4f bg_color)
{
    for (size_t i = 0; i < text_size; ++i)
    {
        const Tile_Glyph glyph = {
            .tile = vec2i_add(tile, vec2i(i, 0)),
            .ch = text[i],
            .fg_color = fg_color,
            .bg_color = bg_color,
        };
        tile_glyph_buffer_push(tgb, glyph);
    }
}

void tile_glyph_render_line(Tile_Glyph_Buffer *tgb, const char *text, Vec2i tile, Vec4f fg_color, Vec4f bg_color)
{
    tile_glyph_render_line_sized(tgb, text, strlen(text), tile, fg_color, bg_color);
}