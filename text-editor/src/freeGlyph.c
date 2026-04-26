#include "freeGlyph.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <SDL3/SDL.h>
#include <GL/glew.h>

#include "./sdl_extra.h"
#include "./font.h"
#include "./simple_renderer.h"
#include "./head.h"
#include "./la.h"


void free_glyph_atlas_init(Free_Glyph_Atlas *atlas, FT_Face font_face)
{
    for (FT_UInt i = 32; i < 128; ++i)
    {
        if (!FT_Load_Char(font_face, i, FT_LOAD_RENDER))
        {
            atlas->atlas_width += font_face->glyph->bitmap.width;
            if (font_face->glyph->bitmap.rows > atlas->atlas_height)
            {
                atlas->atlas_height = font_face->glyph->bitmap.rows;
            }

        }
    }

    glActiveTexture(GL_TEXTURE0);
    glGenTextures(1, &atlas->glyphs_texture);
    glBindTexture(GL_TEXTURE_2D, atlas->glyphs_texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    // 先分配纹理空间
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, atlas->atlas_width, atlas->atlas_height, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);

    // 把每个字符贴进 atlas
    int x = 0;
    for (FT_UInt i = 32; i < 128; ++i)
    {
        if (!FT_Load_Char(font_face, i, FT_LOAD_RENDER))
        {
            const FT_Bitmap *bitmap = &font_face->glyph->bitmap;

            atlas->metrics[i].ax = font_face->glyph->advance.x >> 6;
            atlas->metrics[i].ay = font_face->glyph->advance.y >> 6;
            atlas->metrics[i].bw = font_face->glyph->bitmap.width;
            atlas->metrics[i].bh = font_face->glyph->bitmap.rows;
            atlas->metrics[i].bl = font_face->glyph->bitmap_left;
            atlas->metrics[i].bt = font_face->glyph->bitmap_top;
            atlas->metrics[i].tx = (float)x / (float)atlas->atlas_width;
            
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            glTexSubImage2D(GL_TEXTURE_2D, 0, x, 0, bitmap->width, bitmap->rows, GL_RED, GL_UNSIGNED_BYTE, bitmap->buffer);
            x += bitmap->width;
            
        }
    }

}

float free_glyph_atlas_cursor_pos(const Free_Glyph_Atlas *atlas, const char *text, size_t text_size, Vec2f pos, size_t col)
{
    for (size_t i = 0; i < text_size; ++i) {
        if (i == col) {
            return pos.x;
        }

        Glyph_Metric metric = atlas->metrics[(int) text[i]];
        pos.x += metric.ax;
        pos.y += metric.ay;
    }

    return pos.x;
}

void free_glyph_atlas_render_line_sized(Free_Glyph_Atlas *atlas, Simple_Renderer *sr, const char *text, size_t text_size, Vec2f *pos)
{
    for(size_t i = 0; i < text_size; ++i){
        Glyph_Metric metric = atlas->metrics[(int)text[i]];
        float x2 = pos->x + metric.bl;
        float y2 = -pos->y - metric.bt;
        float w = metric.bw;
        float h = metric.bh;

        pos->x += metric.ax;
        pos->y += metric.ay;

        simple_renderer_image_rect(sr, 
            vec2f(x2, -y2),
             vec2f(w, -h),
             vec2f(metric.tx, 0.0f), 
             vec2f(metric.bw / (float) atlas->atlas_width, metric.bh / (float) atlas->atlas_height));
    }
}