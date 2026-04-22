#ifndef __FONT_H__
#define __FONT_H__

#include <SDL3/SDL.h>

#include "./la.h"

#define FONT_WIDTH 128
#define FONT_HEIGHT 64
#define FONT_COLS 18
#define FONT_ROWS 7
#define FONT_CHAR_WIDTH (FONT_WIDTH / FONT_COLS)
#define FONT_CHAR_HEIGHT (FONT_HEIGHT / FONT_ROWS)
#define FONT_SCALE 5

#define ASCII_DISPLAY_LOW 32
#define ASCII_DISPLAY_HIGH 126

typedef struct {
    SDL_Texture *spritesheet;
    SDL_FRect glyph_table[ASCII_DISPLAY_HIGH - ASCII_DISPLAY_LOW + 1];
} Font;

Font font_load_from_file(SDL_Renderer *renderer, const char *file_path);
void render_char(SDL_Renderer *renderer, const Font *font, char c, Vec2f pos, float scale);
void render_text_sized(SDL_Renderer *renderer, Font *font, const char *text, size_t text_size, Vec2f pos, Uint32 color, float scale);
void set_texture_color(SDL_Texture *texture, Uint32 color);
#endif // __FONT_H__