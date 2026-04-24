#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#include "SDL3/SDL.h"
#include "SDL3/SDL_render.h"
#include "GL/glew.h"

#define STB_IMAGE_IMPLEMENTATION
#include "./stb_image.h"

#include <ft2build.h>
#include FT_FREETYPE_H

#include "./la.h"
#include "./font.h"
#include "./head.h"
#include "./editor.h"
#include "./sdl_extra.h"
#include "./tileGlyph.h"
#include "./freeGlyph.h"
#include "./CursorRenderer.h"

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600
#define FPS 60
#define DELTA_TIME (1.0f / FPS)

void render_text(SDL_Renderer *renderer, Font *font, const char *text, Vec2f pos, Uint32 color, float scale)
{
    render_text_sized(renderer, font, text, strlen(text), pos, color, scale);
}

#define BUFFER_CAPACITY 1024

char buffer[BUFFER_CAPACITY];
size_t buffer_size = 0;

#define UNHEX(color)                 \
    ((color) >> (8 * 0)) & 0xFF,     \
        ((color) >> (8 * 1)) & 0xFF, \
        ((color) >> (8 * 2)) & 0xFF, \
        ((color) >> (8 * 3)) & 0xFF

Vec2f Window_size(SDL_Window *window)
{
    int w, h;
    SDL_GetWindowSize(window, &w, &h);
    return (Vec2f){.x = (float)w, .y = (float)h};
}

Vec2f camer_project_point(SDL_Window *window, Vec2f point, Vec2f camer_pos)
{
    return vec2f_add(
        vec2f_sub(point, camer_pos),
        vec2f_mul(Window_size(window), vec2fs(0.5f)));
}

void render_cursor(SDL_Renderer *renderer, SDL_Window *window, const Font *font, const Editor *editor, Vec2f camer_pos)
{
    const Vec2f pos = camer_project_point(window, (Vec2f){.x = editor->cursor_col * FONT_CHAR_WIDTH * FONT_SCALE, .y = editor->cursor_row * FONT_CHAR_HEIGHT * FONT_SCALE}, camer_pos);

    SDL_FRect rect = {
        .x = pos.x,
        .y = pos.y,
        .w = FONT_CHAR_WIDTH * FONT_SCALE,
        .h = FONT_CHAR_HEIGHT * FONT_SCALE};

    scc(SDL_SetRenderDrawColor(renderer, UNHEX(0xFFFFFFFF)));
    scc(SDL_RenderFillRect(renderer, &rect));

    const char *c = editor_char_under_cursor(editor);
    if (c)
    {
        set_texture_color(font->spritesheet, 0xFF000000);
        render_char(renderer, font, *c, pos, FONT_SCALE);
    }
}

void MessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam)
{
    fprintf(stderr, "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
            (type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""),
            type, severity, message);
}

void gl_render_cursor(Tile_Glyph_Buffer *tgb, const Editor *editor)
{
    const char *c = editor_char_under_cursor(editor);
    Vec2i tile = vec2i(editor->cursor_col, -(int)editor->cursor_row);
    tile_glyph_render_line_sized(tgb, c ? c : " ", 1, tile, vec4fs(0.0f), vec4fs(1.0f));
}

// opengl与sdl的渲染分离,开关影响渲染方式
#define GL_DEFINE

Editor editor = {0};
Vec2f camer_pos = {0.0f, 0.0f};
Vec2f camer_vel = {0.0f, 0.0f};

#ifdef GL_DEFINE

// tile glyph 和 free glyph 的渲染分离,开关影响渲染方式
// #define TILE_GLYPH_RENDER

#ifdef TILE_GLYPH_RENDER
static Tile_Glyph_Buffer tgb = {0};
#else
static Free_Glyph_Buffer fgb = {0};
static Cursor_Renderer cursor_renderer = {0};
#endif

void render_editor_into_tgb(SDL_Window *window, Tile_Glyph_Buffer *tgb, const Editor *editor)
{
    {
        const Vec2f cursor_pos = {
            .x = editor->cursor_col * FONT_CHAR_WIDTH * FONT_SCALE,
            .y = -(int)editor->cursor_row * FONT_CHAR_HEIGHT * FONT_SCALE};
        camer_vel = vec2f_mul(vec2f_sub(cursor_pos, camer_pos), vec2fs(2.0f));
        camer_pos = vec2f_add(camer_pos, vec2f_mul(camer_vel, vec2fs(DELTA_TIME)));
        // 修正：摄像机位置取整，避免亚像素采样导致光条
        camer_pos.x = roundf(camer_pos.x);
        camer_pos.y = roundf(camer_pos.y);
    }
    // 视口和分辨率 uniform 更新
    {
        int w, h;
        SDL_GetWindowSize(window, &w, &h);
        glUniform2f(tgb->resolution_uniform, (float)w, (float)h);
    }

    tile_glyph_buffer_clear(tgb);
    for (size_t row = 0; row < editor->size; ++row)
    {
        const Line *line = editor->lines + row;
        tile_glyph_render_line_sized(tgb, line->chars, line->size, vec2i(0, -((int)row)), vec4f(1.0f, 1.0f, 1.0f, 1.0f), vec4fs(0.0f));
    }
    tile_glyph_buffer_sync(tgb);

    glUniform1f(tgb->time_uniform, (float)SDL_GetTicks() / 1000.0f);
    glUniform2f(tgb->camera_uniform, camer_pos.x, camer_pos.y);

    tile_glyph_buffer_draw(tgb);

    tile_glyph_buffer_clear(tgb);
    gl_render_cursor(tgb, editor);
    tile_glyph_buffer_sync(tgb);

    tile_glyph_buffer_draw(tgb);
}

#define FREE_GLYPH_FONT_SIZE 64
void render_editor_into_fgb(SDL_Window *window, Free_Glyph_Buffer *fgb, Cursor_Renderer *cursor_renderer, const Editor *editor)
{
    // 视口和分辨率 uniform 更新

    int w, h;
    SDL_GetWindowSize(window, &w, &h);

    free_glyph_buffer_use(fgb);
    {
        glUniform2f(fgb->resolution_uniform, (float)w, (float)h);
        glUniform1f(fgb->time_uniform, (float)SDL_GetTicks() / 1000.0f);
        glUniform2f(fgb->camera_uniform, camer_pos.x, camer_pos.y);

        free_glyph_buffer_clear(fgb);
        for (size_t row = 0; row < editor->size; ++row)
        {
            const Line *line = editor->lines + row;
            free_glyph_render_line_sized(fgb, line->chars, line->size, vec2f(0, -(float)row * FREE_GLYPH_FONT_SIZE), vec4fs(1.0f), vec4fs(0.0f));
        }
        free_glyph_buffer_sync(fgb);
        free_glyph_buffer_draw(fgb);
    }

    Vec2f cursor_pos = vec2fs(0.0f);
    {
        cursor_pos.y = -(float)editor->cursor_row * FREE_GLYPH_FONT_SIZE;
        cursor_pos.x = 0.0f;
        if (editor->cursor_row < editor->size)
        {
            Line *line = &editor->lines[editor->cursor_row];
            cursor_pos.x = free_glyph_buffer_cursor_pos(fgb, line->chars, line->size, vec2f(0.0, cursor_pos.y), editor->cursor_col);
        }
    }

    cursor_renderer_use(cursor_renderer);
    {
        glUniform2f(cursor_renderer->resolution_uniform, (float)w, (float)h);
        glUniform1f(cursor_renderer->time_uniform, (float)SDL_GetTicks() / 1000.0f);
        glUniform2f(cursor_renderer->camera_uniform, camer_pos.x, camer_pos.y);
        glUniform1f(cursor_renderer->height_uniform, (float)FREE_GLYPH_FONT_SIZE);

        cursor_renderer_move(cursor_renderer, cursor_pos);
        cursor_renderer_draw();
    }

    {
        const Vec2f cursor_pos = {
            .x = editor->cursor_col * FONT_CHAR_WIDTH * FONT_SCALE,
            .y = -(int)editor->cursor_row * FONT_CHAR_HEIGHT * FONT_SCALE};
        camer_vel = vec2f_mul(vec2f_sub(cursor_pos, camer_pos), vec2fs(2.0f));
        camer_pos = vec2f_add(camer_pos, vec2f_mul(camer_vel, vec2fs(DELTA_TIME)));
        // 修正：摄像机位置取整，避免亚像素采样导致光条
        camer_pos.x = roundf(camer_pos.x);
        camer_pos.y = roundf(camer_pos.y);
    }
}

int main(int argc, char *argv[])
{
    FT_Library library = {0};

    FT_Error error = FT_Init_FreeType(&library);
    if (error)
    {
        fprintf(stderr, "Failed to initialize FreeType library\n");
        exit(EXIT_FAILURE);
    }

    const char *font_path = "./VictorMono-Regular.ttf";

    FT_Face face;
    error = FT_New_Face(library, font_path, 0, &face);
    if (error == FT_Err_Unknown_File_Format)
    {
        fprintf(stderr, "Unsupported font format: %s\n", font_path);
        exit(EXIT_FAILURE);
    }
    else if (error)
    {
        fprintf(stderr, "Failed to load font: %s\n", font_path);
        exit(EXIT_FAILURE);
    }

    FT_UInt font_size = FREE_GLYPH_FONT_SIZE;
    error = FT_Set_Pixel_Sizes(face, 0, font_size);
    if (error)
    {
        fprintf(stderr, "Failed to set font pixel size: %u\n", font_size);
        exit(EXIT_FAILURE);
    }

    const char *filePath = NULL;
    if (argc > 1)
        filePath = argv[1];
    if (filePath)
    {
        FILE *file = fopen(filePath, "r");
        if (file)
        {
            editor_load_from_file(&editor, file);
            fclose(file);
        }
    }

    scc(SDL_Init(SDL_INIT_VIDEO));
    SDL_Window *window = scp(SDL_CreateWindow("Text Editor", SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL));
    SDL_StartTextInput(window);

    // OpenGL context
    {
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        int major, minor;
        SDL_GL_GetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, &major);
        SDL_GL_GetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, &minor);
        printf("OpenGL context version: %d.%d\n", major, minor);
    }

    scp(SDL_GL_CreateContext(window));

    if (glewInit() != GLEW_OK)
    {
        fprintf(stderr, "Failed to initialize GLEW\n");
        exit(EXIT_FAILURE);
    }

    if (!GLEW_ARB_draw_instanced)
    {
        fprintf(stderr, "ARB_draw_instanced is not supported; game may not work properly!!\n");
        exit(1);
    }

    if (!GLEW_ARB_instanced_arrays)
    {
        fprintf(stderr, "ARB_instanced_arrays is not supported; game may not work properly!!\n");
        exit(1);
    }

    glEnable(GL_DEBUG_OUTPUT);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    if (GLEW_ARB_debug_output)
    {
        glDebugMessageCallback(MessageCallback, NULL);
    }
    else
    {
        fprintf(stderr, "GLEW_ARB_debug_output not available\n");
    }

#ifdef TILE_GLYPH_RENDER
    tile_glyph_buffer_init(&tgb,
                           "./charmap-oldschool_white.png",
                           "./shaders/tile_glyph.vert",
                           "./shaders/tile_glyph.frag");
#else
    free_glyph_buffer_init(&fgb,
                           face,
                           "./shaders/free_glyph.vert",
                           "./shaders/free_glyph.frag");
    cursor_renderer_init(&cursor_renderer,
                         "./shaders/cursor.vert",
                         "./shaders/cursor.frag");
#endif

    bool quit = false;
    while (!quit)
    {
        const Uint32 start = SDL_GetTicks();
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
                    editor_backspace(&editor);
                    break;
                case SDLK_RETURN:
                    editor_insert_new_line(&editor);
                    break;
                case SDLK_DELETE:
                    editor_delete(&editor);
                    break;
                case SDLK_UP:
                    if (editor.cursor_row > 0)
                        editor.cursor_row -= 1;
#ifndef TILE_GLYPH_RENDER
                    cursor_renderer_use(&cursor_renderer);
                    glUniform1f(cursor_renderer.last_stroke_uniform, (float)SDL_GetTicks() / 1000.0f);
#endif
                    break;
                case SDLK_DOWN:
                    editor.cursor_row += 1;
#ifndef TILE_GLYPH_RENDER
                    cursor_renderer_use(&cursor_renderer);
                    glUniform1f(cursor_renderer.last_stroke_uniform, (float)SDL_GetTicks() / 1000.0f);
#endif
                    break;
                case SDLK_LEFT:
                    if (editor.cursor_col > 0)
                        editor.cursor_col -= 1;
#ifndef TILE_GLYPH_RENDER
                    cursor_renderer_use(&cursor_renderer);
                    glUniform1f(cursor_renderer.last_stroke_uniform, (float)SDL_GetTicks() / 1000.0f);
#endif
                    break;
                case SDLK_RIGHT:
                    editor.cursor_col += 1;
#ifndef TILE_GLYPH_RENDER
                    cursor_renderer_use(&cursor_renderer);
                    glUniform1f(cursor_renderer.last_stroke_uniform, (float)SDL_GetTicks() / 1000.0f);
#endif
                    break;
                case SDLK_F2:
                    if (filePath)
                        editor_save_to_file(&editor, filePath);
                    break;
                }
                break;
            case SDL_EVENT_TEXT_INPUT:
                editor_insert_text_before_cursor(&editor, event.text.text);
#ifndef TILE_GLYPH_RENDER
                cursor_renderer_use(&cursor_renderer);
                glUniform1f(cursor_renderer.last_stroke_uniform, (float)SDL_GetTicks() / 1000.0f);
#endif // TILE_GLYPH_RENDER
                break;
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
                if (event.button.button == SDL_BUTTON_LEFT)
                {
                    const Vec2f mouse_pos = {.x = (float)event.button.x, .y = (float)event.button.y};
                    const Vec2f world_pos = vec2f_sub(
                        vec2f_add(mouse_pos, camer_pos),
                        vec2f_mul(Window_size(window), vec2fs(0.5f)));
                    const size_t col = (size_t)(world_pos.x / (FONT_CHAR_WIDTH * FONT_SCALE));
                    const size_t row = (size_t)(world_pos.y / (FONT_CHAR_HEIGHT * FONT_SCALE));
                    editor.cursor_col = col;
                    editor.cursor_row = row;
#ifndef TILE_GLYPH_RENDER
                    cursor_renderer_use(&cursor_renderer);
                    glUniform1f(cursor_renderer.last_stroke_uniform, (float)SDL_GetTicks() / 1000.0f);
#endif
                }
                break;
            }
        }

        {
            int w, h;
            SDL_GetWindowSize(window, &w, &h);
            glViewport(0, 0, w, h);
        }

        // 摄像机跟随光标
        {
            const Vec2f cursor_pos = {
                .x = editor.cursor_col * FONT_CHAR_WIDTH * FONT_SCALE,
                .y = -(int)editor.cursor_row * FONT_CHAR_HEIGHT * FONT_SCALE};
            camer_vel = vec2f_mul(vec2f_sub(cursor_pos, camer_pos), vec2fs(2.0f));
            camer_pos = vec2f_add(camer_pos, vec2f_mul(camer_vel, vec2fs(DELTA_TIME)));
            // 修正：摄像机位置取整，避免亚像素采样导致光条
            camer_pos.x = roundf(camer_pos.x);
            camer_pos.y = roundf(camer_pos.y);
        }

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

#ifdef TILE_GLYPH_RENDER
        render_editor_into_tgb(window, &tgb, &editor);
#else
        render_editor_into_fgb(window, &fgb, &cursor_renderer, &editor);
#endif // TILE_GLYPH_RENDER

        SDL_GL_SwapWindow(window);

        const Uint32 duration = (SDL_GetTicks() - start);
        const Uint32 frame_time = 1000 / FPS;
        if (duration < frame_time)
        {
            SDL_Delay(frame_time - duration);
        }
    }

    if (face)
    {
        FT_Done_Face(face);
    }
    if (library)
    {
        FT_Done_FreeType(library);
    }

    return 0;
}

#else
int main(int argc, char *argv[])
{
    const char *filePath = NULL;
    if (argc > 1)
    {
        filePath = argv[1];
    }
    if (filePath)
    {
        FILE *file = fopen(filePath, "rb");
        if (file)
        {
            editor_load_from_file(&editor, file);
            fclose(file);
        }
    }

    scc(SDL_Init(SDL_INIT_VIDEO));

    SDL_Window *window =
        scp(SDL_CreateWindow("Text Editor", SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_RESIZABLE));

    SDL_Renderer *renderer =
        scp(SDL_CreateRenderer(window, NULL));

    Font font = font_load_from_file(renderer, "./charmap-oldschool_white.png");

    SDL_StartTextInput(window);

    bool quit = false;
    while (!quit)
    {
        const Uint32 start = SDL_GetTicks(); // 更新SDL内部计时器，确保事件时间戳正确
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
                    break;
                case SDLK_DELETE:
                    editor_delete(&editor);
                    break;
                case SDLK_UP:
                    if (editor.cursor_row > 0)
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
                case SDLK_F2:
                    if (filePath)
                        editor_save_to_file(&editor, filePath);
                    break;
                }
                break;
            }
            break;
            case SDL_EVENT_TEXT_INPUT:
                editor_insert_text_before_cursor(&editor, event.text.text);
                break;
            }
        }

        {
            const Vec2f cursor_pos = {
                .x = editor.cursor_col * FONT_CHAR_WIDTH * FONT_SCALE,
                .y = editor.cursor_row * FONT_CHAR_HEIGHT * FONT_SCALE};

            camer_vel = vec2f_mul(vec2f_sub(cursor_pos, camer_pos), vec2fs(2.0f));
            camer_pos = vec2f_add(camer_pos, vec2f_mul(camer_vel, vec2fs(DELTA_TIME)));
        }

        scc(SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0));
        scc(SDL_RenderClear(renderer));

        for (size_t row = 0; row < editor.size; ++row)
        {
            const Line *line = editor.lines + row;
            const Vec2f line_pos = camer_project_point(window, (Vec2f){.x = 0.0f, .y = row * FONT_CHAR_HEIGHT * FONT_SCALE}, camer_pos);

            render_text_sized(renderer, &font, line->chars, line->size, line_pos, 0xFFFFFFFF, FONT_SCALE);
        }

        render_cursor(renderer, window, &font, &editor, camer_pos);

        SDL_RenderPresent(renderer);

        const Uint32 duration = (SDL_GetTicks() - start); // 更新SDL内部计时器，确保事件时间戳正确
        const Uint32 frame_time = 1000 / FPS;
        if (duration < frame_time)
        {
            SDL_Delay(frame_time - duration);
        }
    }

    SDL_Quit();
    return 0;
}
#endif // GL_DEFINE