#include "font.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#include "./head.h"

#include "SDL3/SDL.h"
#include "./stb_image.h"

SDL_Surface *surface_from_file(const char *file_path)
{
    int width, height, channels;
    unsigned char *data = stbi_load(file_path, &width, &height, &channels, STBI_rgb_alpha);

    if (data == NULL)
    {
        fprintf(stderr, "ERROR: could not load file %s: %s\n", file_path, stbi_failure_reason());
        exit(EXIT_FAILURE);
    }

    const int pitch = 4 * width;
    return scp(SDL_CreateSurfaceFrom(width, height, SDL_PIXELFORMAT_RGBA32, data, pitch));
}

void set_texture_color(SDL_Texture *texture, Uint32 color)
{
    scc(SDL_SetTextureColorMod(texture, (color >> 0) & 0xFF, (color >> 8) & 0xFF, (color >> 16) & 0xFF));
    scc(SDL_SetTextureAlphaMod(texture, ((color >> 24) & 0xFF)));
}

Font font_load_from_file(SDL_Renderer *renderer, const char *file_path)
{
    Font font = {0};

    SDL_Surface *font_surface = surface_from_file(file_path);
    scc(SDL_SetSurfaceColorKey(font_surface, true, 0xFF000000)); //
    font.spritesheet = scp(SDL_CreateTextureFromSurface(renderer, font_surface));

    SDL_DestroySurface(font_surface);

    for (size_t ascii = ASCII_DISPLAY_LOW; ascii <= ASCII_DISPLAY_HIGH; ++ascii)
    {
        const size_t index = ascii - ASCII_DISPLAY_LOW;
        const size_t col = index % FONT_COLS;
        const size_t row = index / FONT_COLS;
        font.glyph_table[index] = (SDL_FRect){
            .x = (float)(col * FONT_CHAR_WIDTH),
            .y = (float)(row * FONT_CHAR_HEIGHT),
            .w = (float)FONT_CHAR_WIDTH,
            .h = (float)FONT_CHAR_HEIGHT};
    }

    return font;
}

void render_char(SDL_Renderer *renderer, const Font *font, char c, Vec2f pos, float scale)
{
    const SDL_FRect dst_rect = {
        .x = pos.x,
        .y = pos.y,
        .w = FONT_CHAR_WIDTH * scale,
        .h = FONT_CHAR_HEIGHT * scale};

    assert(c >= ASCII_DISPLAY_LOW && c <= ASCII_DISPLAY_HIGH);
    const size_t index = c - ASCII_DISPLAY_LOW;
    scc(SDL_RenderTexture(renderer, font->spritesheet, &font->glyph_table[index], &dst_rect));
}

void render_text_sized(SDL_Renderer *renderer, Font *font, const char *text, size_t text_size, Vec2f pos, Uint32 color, float scale)
{
    set_texture_color(font->spritesheet, color);

    Vec2f pen = pos;
    for (size_t i = 0; i < text_size; ++i)
    {
        render_char(renderer, font, text[i], pen, scale);
        pen.x += FONT_CHAR_WIDTH * scale;
    }

    // 恢复纹理颜色模式
    scc(SDL_SetTextureColorMod(font->spritesheet, 255, 255, 255));
    scc(SDL_SetTextureAlphaMod(font->spritesheet, 255));
}