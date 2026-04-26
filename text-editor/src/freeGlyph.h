#ifndef __FREE_GLYPH_H__
#define __FREE_GLYPH_H__

#include <stdio.h>

#include <SDL3/SDL.h>
#include <GL/glew.h>

#include <ft2build.h>
#include FT_FREETYPE_H


#include "./simple_renderer.h"

typedef struct{
    float ax;
    float ay;

    float bw;
    float bh;

    float bl;
    float bt;

    float tx;
} Glyph_Metric;

typedef struct{
    FT_UInt atlas_width;
    FT_UInt atlas_height;
    GLuint glyphs_texture;
    Glyph_Metric metrics[128];
} Free_Glyph_Atlas;

void free_glyph_atlas_init(Free_Glyph_Atlas *atlas, FT_Face font_face);

float free_glyph_atlas_cursor_pos(const Free_Glyph_Atlas *atlas, const char *text, size_t text_size, Vec2f pos, size_t col);
void free_glyph_atlas_render_line_sized(Free_Glyph_Atlas *atlas, Simple_Renderer *sr, const char *text, size_t text_size,Vec2f *pos);

#endif // __FREE_GLYPH_H__