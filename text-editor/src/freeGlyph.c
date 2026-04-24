#include "freeGlyph.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <SDL3/SDL.h>
#include <GL/glew.h>

#include "./sdl_extra.h"
#include "./font.h"

typedef struct
{
    size_t offset;
    GLint comps;
    GLenum type;
} Attr_Def;

const static Attr_Def glyph_attr_defs[COUNT_FREE_GLYPH_ATTRS] = {
    [FREE_GLYPH_ATTR_POS] = {offsetof(Free_Glyph, pos), 2, GL_FLOAT},
    [FREE_GLYPH_ATTR_SIZE] = {offsetof(Free_Glyph, size), 2, GL_FLOAT},
    [FREE_GLYPH_ATTR_UV_POS] = {offsetof(Free_Glyph, uv_pos), 2, GL_FLOAT},
    [FREE_GLYPH_ATTR_UV_SIZE] = {offsetof(Free_Glyph, uv_size), 2, GL_FLOAT},
    [FREE_GLYPH_ATTR_FG_COLOR] = {offsetof(Free_Glyph, fg_color), 4, GL_FLOAT},
    [FREE_GLYPH_ATTR_BG_COLOR] = {offsetof(Free_Glyph, bg_color), 4, GL_FLOAT},
};
static_assert(COUNT_FREE_GLYPH_ATTRS == 6, "free_glyph_attr_defs size must match Free_Glyph_Attr count");

void free_glyph_buffer_init(Free_Glyph_Buffer *fgb, FT_Face font_face,
                            const char *vertex_shader_path, const char *fragment_shader_path)
{
    glGenVertexArrays(1, &fgb->vao);
    glBindVertexArray(fgb->vao);

    glGenBuffers(1, &fgb->vbo);
    glBindBuffer(GL_ARRAY_BUFFER, fgb->vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(fgb->glyphs), fgb->glyphs, GL_DYNAMIC_DRAW);

    for (Free_Glyph_Attr attr = 0; attr < COUNT_FREE_GLYPH_ATTRS; ++attr)
    {
        const Attr_Def def = glyph_attr_defs[attr];
        glEnableVertexAttribArray(attr);
        // 根据属性定义选择正确的顶点属性指针类型
        if (def.type == GL_INT)
        {
            glVertexAttribIPointer(attr, def.comps, def.type, sizeof(Free_Glyph), (void *)def.offset);
        }
        else
        {
            glVertexAttribPointer(attr, def.comps, def.type, GL_FALSE, sizeof(Free_Glyph), (void *)def.offset);
        }
        glVertexAttribDivisor(attr, 1);
    }

    /////////////////////////////////////////////////
    /////////////////////////////////////////////////

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

    // 获取 uniform 变量位置
    fgb->time_uniform = glGetUniformLocation(shader_program, "time");
    fgb->resolution_uniform = glGetUniformLocation(shader_program, "resolution");
    fgb->camera_uniform = glGetUniformLocation(shader_program, "camera");
    
    ////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////

    for (FT_UInt i = 32; i < 128; ++i)
    {
        if (!FT_Load_Char(font_face, i, FT_LOAD_RENDER))
        {
            fgb->atlas_width += font_face->glyph->bitmap.width;
            if (font_face->glyph->bitmap.rows > fgb->atlas_height)
            {
                fgb->atlas_height = font_face->glyph->bitmap.rows;
            }

        }
    }

    glActiveTexture(GL_TEXTURE0);
    glGenTextures(1, &fgb->glyphs_texture);
    glBindTexture(GL_TEXTURE_2D, fgb->glyphs_texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    // 先分配纹理空间
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, fgb->atlas_width, fgb->atlas_height, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);

    

    // 把每个字符贴进 atlas
    int x = 0;
    for (FT_UInt i = 32; i < 128; ++i)
    {
        if (!FT_Load_Char(font_face, i, FT_LOAD_RENDER))
        {
            const FT_Bitmap *bitmap = &font_face->glyph->bitmap;

            fgb->metrics[i].ax = font_face->glyph->advance.x >> 6;
            fgb->metrics[i].ay = font_face->glyph->advance.y >> 6;
            fgb->metrics[i].bw = font_face->glyph->bitmap.width;
            fgb->metrics[i].bh = font_face->glyph->bitmap.rows;
            fgb->metrics[i].bl = font_face->glyph->bitmap_left;
            fgb->metrics[i].bt = font_face->glyph->bitmap_top;
            fgb->metrics[i].tx = (float)x / fgb->atlas_width;
            

            glTexSubImage2D(GL_TEXTURE_2D, 0, x, 0, bitmap->width, bitmap->rows, GL_RED, GL_UNSIGNED_BYTE, bitmap->buffer);
            x += bitmap->width;
            
        }
    }

}

void free_glyph_buffer_clear(Free_Glyph_Buffer *fgb)
{
    fgb->glyphs_count = 0;
}
void free_glyph_buffer_push(Free_Glyph_Buffer *fgb, Free_Glyph glyph)
{
    assert(fgb->glyphs_count < FREE_GLYPH_BUFFER_CAPACITY);
    fgb->glyphs[fgb->glyphs_count++] = glyph;
}
void free_glyph_buffer_sync(Free_Glyph_Buffer *fgb)
{
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Free_Glyph) * fgb->glyphs_count, fgb->glyphs);
}
void free_glyph_buffer_draw(Free_Glyph_Buffer *fgb)
{
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, (GLsizei)fgb->glyphs_count);
}

void free_glyph_render_line_sized(Free_Glyph_Buffer *fgb, const char *text, size_t text_size, Vec2f pos, Vec4f fg_color, Vec4f bg_color)
{
    for(size_t i = 0; i < text_size; ++i){
        Glyph_Metric metric = fgb->metrics[(int)text[i]];
        float x2 = pos.x + metric.bl;
        float y2 = -pos.y - metric.bt;
        float w = metric.bw;
        float h = metric.bh;

        pos.x += metric.ax;
        pos.y += metric.ay;

        Free_Glyph glyph = {
            .pos = vec2f(x2, -y2),
            .size = vec2f(w, -h),
            .uv_pos = vec2f(metric.tx, 0.0f),
            .uv_size = vec2f(metric.bw / (float) fgb->atlas_width, metric.bh / (float) fgb->atlas_height),
            .fg_color = fg_color,
            .bg_color = bg_color,
        };
        free_glyph_buffer_push(fgb, glyph);
    }
}

void free_glyph_render_line(Free_Glyph_Buffer *fgb, const char* text, Vec2f pos, Vec4f fg_color, Vec4f bg_color)
{
    free_glyph_render_line_sized(fgb, text, strlen(text), pos, fg_color, bg_color);
}