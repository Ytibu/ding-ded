#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "SDL3/SDL.h"
#include "SDL3/SDL_render.h"
#include "SDL3_ttf/SDL_ttf.h"

#include "./La.h"

#define STB_IMAGE_IMPLEMENTATION
#include "./stb_image.h"

#include "./Editor.h"

#define FONT_WIDTH 128
#define FONT_HEIGHT 64
#define FONT_COLS 18
#define FONT_ROWS 7
#define FONT_CHAR_WIDTH (FONT_WIDTH / FONT_COLS)
#define FONT_CHAR_HEIGHT (FONT_HEIGHT / FONT_ROWS)
#define FONT_SCALE 5

void scc(int code)
{
    if (code < 0)
    {
        fprintf(stderr, "Error: %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }
}

void *scp(void *ptr)
{
    if (ptr == NULL)
    {
        fprintf(stderr, "Error: %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }
    return ptr;
}

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
    return (SDL_Surface *)scp(SDL_CreateSurfaceFrom(width, height, SDL_PIXELFORMAT_RGBA32, data, pitch));
}

#define ASCII_DISPLAY_LOW 32
#define ASCII_DISPLAY_HIGH 126

typedef struct
{
    SDL_Texture *spritesheet;
    SDL_FRect glyph_table[ASCII_DISPLAY_HIGH - ASCII_DISPLAY_LOW + 1];
} Font;

Font font_load_from_file(SDL_Renderer *renderer, const char *file_path)
{
    Font font = {0};

    SDL_Surface *font_surface = surface_from_file(file_path);
    scc(SDL_SetSurfaceColorKey(font_surface, true, 0xFF000000));
    font.spritesheet = (SDL_Texture *)scp(SDL_CreateTextureFromSurface(renderer, font_surface));

    SDL_DestroySurface(font_surface);

    for (size_t ascii = ASCII_DISPLAY_LOW; ascii <= ASCII_DISPLAY_HIGH; ++ascii)
    {
        const size_t index = ascii - ASCII_DISPLAY_LOW;
        const size_t col = index % FONT_COLS;
        const size_t row = index / FONT_COLS;
        font.glyph_table[index].x = (float)(col * FONT_CHAR_WIDTH);
        font.glyph_table[index].y = (float)(row * FONT_CHAR_HEIGHT);
        font.glyph_table[index].w = (float)FONT_CHAR_WIDTH;
        font.glyph_table[index].h = (float)FONT_CHAR_HEIGHT;
    }

    return font;
}

void render_char(SDL_Renderer *renderer, const Font *font, char c, Vec2f pos, float scale)
{
    const SDL_FRect dst_rect = {
        pos.x,
        pos.y,
        FONT_CHAR_WIDTH * scale,
        FONT_CHAR_HEIGHT * scale,
    };

    assert(c >= ASCII_DISPLAY_LOW && c <= ASCII_DISPLAY_HIGH);
    const size_t index = (size_t)(c - ASCII_DISPLAY_LOW);
    scc(SDL_RenderTexture(renderer, font->spritesheet, &font->glyph_table[index], &dst_rect));
}

void set_texture_color(SDL_Texture *texture, Uint32 color)
{
    scc(SDL_SetTextureColorMod(texture, (color >> 0) & 0xFF, (color >> 8) & 0xFF, (color >> 16) & 0xFF));
    scc(SDL_SetTextureAlphaMod(texture, ((color >> 24) & 0xFF)));
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

    scc(SDL_SetTextureColorMod(font->spritesheet, 255, 255, 255));
    scc(SDL_SetTextureAlphaMod(font->spritesheet, 255));
}

#define UNHEX(color)                 \
    ((color) >> (8 * 0)) & 0xFF,     \
        ((color) >> (8 * 1)) & 0xFF, \
        ((color) >> (8 * 2)) & 0xFF, \
        ((color) >> (8 * 3)) & 0xFF

Editor editor;

void render_cursor(SDL_Renderer *renderer, const Font *font)
{
    Vec2f pos;
    pos.x = (float)(editor.editor_cursor_col() * FONT_CHAR_WIDTH * FONT_SCALE);
    pos.y = (float)(editor.editor_cursor_row() * FONT_CHAR_HEIGHT * FONT_SCALE);

    SDL_FRect rect = {
        pos.x,
        pos.y,
        FONT_CHAR_WIDTH * FONT_SCALE,
        FONT_CHAR_HEIGHT * FONT_SCALE,
    };

    scc(SDL_SetRenderDrawColor(renderer, UNHEX(0xFFFFFFFF)));
    scc(SDL_RenderFillRect(renderer, &rect));

    const char *c = editor.editor_char_under_cursor();
    if (c)
    {
        set_texture_color(font->spritesheet, 0xFF000000);
        render_char(renderer, font, *c, pos, FONT_SCALE);
    }
}

int main(void)
{
    scc(SDL_Init(SDL_INIT_VIDEO));

    SDL_Window *window = (SDL_Window *)scp(SDL_CreateWindow("Text Editor C++", 800, 600, SDL_WINDOW_RESIZABLE));
    SDL_Renderer *renderer = (SDL_Renderer *)scp(SDL_CreateRenderer(window, NULL));

    Font font = font_load_from_file(renderer, "./charmap-oldschool_white.png");

    SDL_StartTextInput(window);

    bool quit = false;
    while (!quit)
    {
        SDL_Event event = {0};
        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
            case SDL_EVENT_QUIT:
                quit = true;
                break;

            case SDL_EVENT_KEY_DOWN:
                switch (event.key.key)
                {
                case SDLK_BACKSPACE:
                    editor.editor_backspace();
                    break;
                case SDLK_RETURN:
                    editor.editor_insert_new_line();
                    break;
                case SDLK_DELETE:
                    editor.editor_delete();
                    break;
                case SDLK_UP:
                    editor.editor_move_up();
                    break;
                case SDLK_DOWN:
                    editor.editor_move_down();
                    break;
                case SDLK_LEFT:
                    editor.editor_move_left();
                    break;
                case SDLK_RIGHT:
                    editor.editor_move_right();
                    break;
                case SDLK_F2:
                    editor.editor_save_to_file("output.txt");
                    break;
                default:
                    break;
                }
                break;

            case SDL_EVENT_TEXT_INPUT:
                editor.editor_insert_text_before_cursor(event.text.text);
                break;

            default:
                break;
            }
        }

        scc(SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0));
        scc(SDL_RenderClear(renderer));

        for (size_t row = 0; row < editor.editor_line_count(); ++row)
        {
            Vec2f pos;
            pos.x = 0.0f;
            pos.y = (float)(row * FONT_CHAR_HEIGHT * FONT_SCALE);
            render_text_sized(renderer,
                              &font,
                              editor.editor_line_data(row),
                              editor.editor_line_size(row),
                              pos,
                              0xFFFFFFFF,
                              FONT_SCALE);
        }

        render_cursor(renderer, &font);
        SDL_RenderPresent(renderer);
    }

    SDL_Quit();
    return 0;
}
