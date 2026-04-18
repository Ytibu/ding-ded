#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "SDL3/SDL.h"
#include "SDL3/SDL_render.h"
#include "SDL3_ttf/SDL_ttf.h"

#include "./la.h"

#define STB_IMAGE_IMPLEMENTATION
#include "./stb_image.h"

#include "./editor.h"

#define FONT_WIDTH 128
#define FONT_HEIGHT 64
#define FONT_COLS 18
#define FONT_ROWS 7
#define FONT_CHAR_WIDTH (FONT_WIDTH / FONT_COLS)
#define FONT_CHAR_HEIGHT (FONT_HEIGHT / FONT_ROWS)
#define FONT_SCALE 5

/**
 * scc - SDL Call Check
 * @ok: SDL函数返回的布尔值
 * 检查SDL函数调用是否成功，失败时输出错误信息并退出程序
 */
void scc(int code)
{
    if (code < 0)
    {
        fprintf(stderr, "Error: %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }
}

/**
 * scp - SDL Pointer Check
 * @ptr: 指向内存的指针
 * 检查指针是否为NULL，为NULL时输出错误信息并退出程序
 * Return: 验证后的指针
 */
void *scp(void *ptr)
{
    if (ptr == NULL)
    {
        fprintf(stderr, "Error: %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }
    return ptr;
}

/**
 * surface_from_file - 从图像文件加载表面
 * @file_path: 图像文件路径
 * 使用stb_image库加载PNG/JPG文件，创建SDL_Surface对象
 * Return: 加载后的SDL_Surface指针
 */
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
#define ASCII_DISPLAY_LOW 32
#define ASCII_DISPLAY_HIGH 126

/**
 * Font - 字体结构体
 * @spritesheet: 字体纹理
 * @glyph_table: 字形位置表（存储每个字符在纹理中的位置和大小）
 */
typedef struct
{
    SDL_Texture *spritesheet;
    SDL_FRect glyph_table[ASCII_DISPLAY_HIGH - ASCII_DISPLAY_LOW + 1];
} Font;

/**
 * font_load_from_file - 从图像文件加载字体
 * @renderer: SDL渲染器
 * @file_path: 字体纹理图像路径
 * 将字体图像加载为纹理，并为每个ASCII字符计算其在纹理中的位置
 * Return: 初始化后的Font结构体
 */
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

/**
 * render_char - 渲染单个字符
 * @renderer: SDL渲染器
 * @font: 字体对象指针
 * @c: 要渲染的字符
 * @pos: 字符在屏幕上的位置
 * @scale: 放大倍数
 * 在指定位置和缩放比例下渲染单个字符
 */
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

void set_texture_color(SDL_Texture *texture, Uint32 color)
{
    scc(SDL_SetTextureColorMod(texture, (color >> 0) & 0xFF, (color >> 8) & 0xFF, (color >> 16) & 0xFF));
    scc(SDL_SetTextureAlphaMod(texture, ((color >> 24) & 0xFF)));
}

/**
 * render_text_sized - 渲染指定大小的文本字符串
 * @renderer: SDL渲染器
 * @font: 字体对象指针
 * @text: 要渲染的文本指针
 * @text_size: 文本长度
 * @pos: 文本在屏幕上的起始位置
 * @color: 文本颜色(RGBA格式)
 * @scale: 放大倍数
 * 在指定位置使用指定颜色和缩放比例渲染文本字符串
 */
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

/**
 * render_text - 渲染文本字符串（包装函数）
 * @renderer: SDL渲染器
 * @font: 字体对象指针
 * @text: 要渲染的文本指针（以NULL结尾）
 * @pos: 文本在屏幕上的起始位置
 * @color: 文本颜色(RGBA格式)
 * @scale: 放大倍数
 * 自动计算文本长度，然后调用render_text_sized进行渲染
 */
void render_text(SDL_Renderer *renderer, Font *font, const char *text, Vec2f pos, Uint32 color, float scale)
{
    render_text_sized(renderer, font, text, strlen(text), pos, color, scale);
}

#define BUFFER_CAPACITY 1024

char buffer[BUFFER_CAPACITY];
size_t buffer_size = 0;

Editor editor = {0};

#define UNHEX(color)                 \
    ((color) >> (8 * 0)) & 0xFF,     \
        ((color) >> (8 * 1)) & 0xFF, \
        ((color) >> (8 * 2)) & 0xFF, \
        ((color) >> (8 * 3)) & 0xFF

/**
 * render_cursor - 渲染文本光标
 * @renderer: SDL渲染器
 * @color: 光标颜色(RGBA格式)
 * 在当前光标位置渲染一个矩形光标
 */
void render_cursor(SDL_Renderer *renderer, const Font *font)
{
    const Vec2f pos = {.x = editor.cursor_col * FONT_CHAR_WIDTH * FONT_SCALE,
                       .y = editor.cursor_row * FONT_CHAR_HEIGHT * FONT_SCALE};

    SDL_FRect rect = {
        .x = pos.x,
        .y = pos.y,
        .w = FONT_CHAR_WIDTH * FONT_SCALE,
        .h = FONT_CHAR_HEIGHT * FONT_SCALE};

    scc(SDL_SetRenderDrawColor(renderer, UNHEX(0xFFFFFFFF)));
    scc(SDL_RenderFillRect(renderer, &rect));

    const char *c = editor_char_under_cursor(&editor);
    if (c)
    {
        set_texture_color(font->spritesheet, 0xFF000000);
        render_char(renderer, font, *c, pos, FONT_SCALE);
    }
}

/**
 * main - 主程序入口
 * 初始化SDL和文本编辑器，处理事件循环，管理文本输入和光标移动
 * Return: 程序退出码
 */
int main()
{
    scc(SDL_Init(SDL_INIT_VIDEO));

    SDL_Window *window =
        scp(SDL_CreateWindow("Text Editor", 800, 600, SDL_WINDOW_RESIZABLE));

    SDL_Renderer *renderer =
        scp(SDL_CreateRenderer(window, NULL));

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
            {
                switch (event.key.key)
                {
                case SDLK_BACKSPACE:
                    editor_backspace(&editor);
                    break;
                case SDLK_RETURN:
                    editor_insert_new_line(&editor);
                case SDLK_DELETE:
                    editor_delete(&editor);
                    break;
                case SDLK_UP:
                    if(editor.cursor_row > 0)
                        editor.cursor_row -= 1;
                    break;
                case SDLK_DOWN:
                    editor.cursor_row += 1;
                    break;
                case SDLK_LEFT:
                    if (editor.cursor_col > 0)
                        editor.cursor_col -= 1;
                    break;
                case SDLK_RIGHT:
                    editor.cursor_col += 1;
                    break;
                }
                case SDLK_F2:
                    editor_save_to_file(&editor, "output");
            }
            break;

            case SDL_EVENT_TEXT_INPUT:
                editor_insert_text_before_cursor(&editor, event.text.text);
                break;
            }
        }

        scc(SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0));
        scc(SDL_RenderClear(renderer));

        for (size_t row = 0; row < editor.size; ++row)
        {
            const Line *line = editor.lines + row;
            render_text_sized(renderer, &font, line->chars, line->size, (Vec2f){0.0f, row * FONT_CHAR_HEIGHT * FONT_SCALE}, 0xFFFFFFFF, FONT_SCALE);
        }

        render_cursor(renderer, &font);

        SDL_RenderPresent(renderer);
    }

    SDL_Quit();
    return 0;
}