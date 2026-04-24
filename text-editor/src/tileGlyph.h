#ifndef __TILE_GLYPH_H__
#define __TILE_GLYPH_H__

#include <SDL3/SDL.h>
#include <GL/glew.h>

#include "./la.h"

typedef struct{
    Vec2i tile;
    int ch;
    Vec4f fg_color;
    Vec4f bg_color;
} Tile_Glyph;

typedef enum{
    TILE_GLYPH_ATTR_TILE = 0,
    TILE_GLYPH_ATTR_CH = 1,
    TILE_GLYPH_ATTR_FG_COLOR = 2,
    TILE_GLYPH_ATTR_BG_COLOR = 3,
    COUNT_TILE_GLYPH_ATTRS,
} Tile_Glyph_Attr;

typedef struct{
    size_t offset;
    size_t comps;
    GLenum type;
} Glyph_Attr_Def;

#define TILE_GLYPH_BUFFER_CAPACITY (1024 * 640)
typedef struct{
    GLuint vao;
    GLuint vbo;

    GLuint font_texture;

    GLint resolution_uniform;
    GLint time_uniform;
    GLint scale_uniform;
    GLint camera_uniform;
    
    Tile_Glyph glyphs[TILE_GLYPH_BUFFER_CAPACITY];
    size_t glyphs_count;
} Tile_Glyph_Buffer;

void tile_glyph_buffer_init(Tile_Glyph_Buffer *tgb, 
    const char* texture_atlas_path, const char* vertex_shader_path, const char* fragment_shader_path);

void tile_glyph_buffer_clear(Tile_Glyph_Buffer *tgb);
void tile_glyph_buffer_push(Tile_Glyph_Buffer *tgb, Tile_Glyph glyph);
void tile_glyph_buffer_sync(Tile_Glyph_Buffer *tgb);
void tile_glyph_buffer_draw(Tile_Glyph_Buffer *tgb);

void tile_glyph_render_line_sized(Tile_Glyph_Buffer *tgb, const char* text,
     size_t text_size, Vec2i tile, Vec4f fg_color, Vec4f bg_color);
void tile_glyph_render_line(Tile_Glyph_Buffer *tgb, const char* text,
     Vec2i tile, Vec4f fg_color, Vec4f bg_color);

#endif // __TILE_GLYPH_H__