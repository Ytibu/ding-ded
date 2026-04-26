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
#include "./freeGlyph.h"
#include "./simple_renderer.h"

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600
#define FPS 60
#define DELTA_TIME (1.0f / FPS)

#define FREE_GLYPH_FONT_SIZE 64
#define ZOOM_OUT_GLYPH_THRESHOLD 30

Editor editor = {0};
Uint32 last_stroke = 0;  // 修正：使用简单的初始化而不是复合字面量
// Vec2f camera_pos = {0};
// float camera_scale = 3.0f;
// float camera_scale_vel = 0.0f;
// Vec2f camera_vel = {0};

static Free_Glyph_Atlas atlas = {0};
static Simple_Renderer sr = {0};

// 错误回调函数，用于输出 OpenGL 错误信息
void MessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam)
{
    (void)source;
    (void)id;
    (void)length;
    (void)userParam;
    fprintf(stderr, "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
            (type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""), type, severity, message);
}

void render_editor(SDL_Window *window, Free_Glyph_Atlas *atlas, Simple_Renderer *sr, Editor *editor)
{
    int w, h;
    SDL_GetWindowSize(window, &w, &h);

    float max_line_len = 0.0f;

    simple_renderer_use(sr);

    sr->resolution = vec2f(w, h);
    sr->time = (float) SDL_GetTicks() / 1000.0f;

    // Render text
    simple_renderer_set_shader(sr, SHADER_FOR_EPICNESS);
    {
        for (size_t row = 0; row < editor->lines.count; ++row) {
            Line_ line = editor->lines.items[row];

            const Vec2f begin = vec2f(0, -(float)row * FREE_GLYPH_FONT_SIZE);
            Vec2f end = begin;
            free_glyph_atlas_render_line_sized(
                atlas, sr, editor->data.items + line.begin, line.end - line.begin,
                &end);
            // TODO: the max_line_len should be calculated based on what's visible on the screen right now
            float line_len = fabsf(end.x - begin.x);
            if (line_len > max_line_len) {
                max_line_len = line_len;
            }
        }

        simple_renderer_flush(sr);
    }

    Vec2f cursor_pos = vec2fs(0.0f);
    {
        size_t cursor_row = editor_cursor_row(editor);
        Line_ line = editor->lines.items[cursor_row];
        size_t cursor_col = editor->cursor - line.begin;
        cursor_pos.y = -(float) cursor_row * FREE_GLYPH_FONT_SIZE;
        cursor_pos.x = free_glyph_atlas_cursor_pos(
                           atlas,
                           editor->data.items + line.begin, line.end - line.begin,
                           vec2f(0.0, cursor_pos.y),
                           cursor_col
                       );
    }

    // Render cursor
    simple_renderer_set_shader(sr, SHADER_FOR_COLOR);
    {
        float CURSOR_WIDTH = 5.0f;
        Uint32 CURSOR_BLINK_THRESHOLD = 500;
        Uint32 CURSOR_BLINK_PERIOD = 1000;
        Uint32 t = SDL_GetTicks() - last_stroke;

        sr->vertices_count = 0;
        if (t < CURSOR_BLINK_THRESHOLD || t/CURSOR_BLINK_PERIOD%2 != 0) {
            simple_renderer_solid_rect(
                sr,
                cursor_pos, vec2f(CURSOR_WIDTH, FREE_GLYPH_FONT_SIZE),
                vec4fs(1));
        }

        simple_renderer_flush(sr);
    }

    // Update camera
    {
        float target_scale = 3.0f;
        if (max_line_len > 0.0f) {
            target_scale = SCREEN_WIDTH / max_line_len;
        }

        if (target_scale > 3.0f) {
            target_scale = 3.0f;
        }

        sr->camera_vel = vec2f_mul(
                             vec2f_sub(cursor_pos, sr->camera_pos),
                             vec2fs(2.0f));
        sr->camera_scale_vel = (target_scale - sr->camera_scale) * 2.0f;

        sr->camera_pos = vec2f_add(sr->camera_pos, vec2f_mul(sr->camera_vel, vec2fs(DELTA_TIME)));
        sr->camera_scale = sr->camera_scale + sr->camera_scale_vel * DELTA_TIME;
    }
}

// 字体加载
FT_Face init_Font(const char *const font_file_path)
{
    FT_Library library = {0};

    FT_Error error = FT_Init_FreeType(&library);
    if (error)
    {
        fprintf(stderr, "ERROR: could not initialize FreeType2 library\n");
        exit(1);
    }

    FT_Face face;
    error = FT_New_Face(library, font_file_path, 0, &face);
    if (error == FT_Err_Unknown_File_Format)
    {
        fprintf(stderr, "ERROR: `%s` has an unknown format\n", font_file_path);
        exit(1);
    }
    else if (error)
    {
        fprintf(stderr, "ERROR: could not load file `%s`\n", font_file_path);
        exit(1);
    }

    FT_UInt pixel_size = FREE_GLYPH_FONT_SIZE;
    error = FT_Set_Pixel_Sizes(face, 0, pixel_size);
    if (error)
    {
        fprintf(stderr, "ERROR: could not set pixel size to %u\n", pixel_size);
        exit(1);
    }

    return face;
}

// 编辑区加载
void init_editor(int argc, char **argv, const char *file_path)
{
    if (argc > 1)
    {
        file_path = argv[1];
    }

    if (file_path)
    {
        FILE *file = fopen(file_path, "r");
        if (file != NULL)
        {
            editor_load_from_file(&editor, file);
            fclose(file);
        }
    }
    else
    {
        editor_recompute_lines(&editor);
    }
}

int main(int argc, char **argv)
{
    // 加载字体
    FT_Face face = init_Font("./fonts/VictorMono-Regular.ttf");

    // 加载编辑区内容
    const char *file_path = NULL;
    init_editor(argc, argv, file_path);

    scc(SDL_Init(SDL_INIT_VIDEO));
    SDL_Window *window =
        scp(SDL_CreateWindow("Glyph",
                             800, 600,
                             SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL));
    SDL_StartTextInput(window);

    {
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

        int major;
        int minor;
        SDL_GL_GetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, &major);
        SDL_GL_GetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, &minor);
        printf("GL version %d.%d\n", major, minor);
    }

    scp(SDL_GL_CreateContext(window));

    if (GLEW_OK != glewInit())
    {
        ERROR("Could not initialize GLEW!");
    }

    if (!GLEW_ARB_draw_instanced)
    {
        ERROR("ARB_draw_instanced is not supported; game may not work properly!!");
    }

    if (!GLEW_ARB_instanced_arrays)
    {
        ERROR("ARB_instanced_arrays is not supported; game may not work properly!!");
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    if (GLEW_ARB_debug_output)
    {
        glEnable(GL_DEBUG_OUTPUT);
        INFO("No OpenGL debug output will be printed because GLEW_ARB_debug_output is not supported");
    }
    else
    {
        INFO("No OpenGL debug output will be printed (see previous line)");
    }

    simple_renderer_init(&sr,
                         "./shaders/simple.vert",
                         "./shaders/simple_color.frag",
                         "./shaders/simple_image.frag",
                         "./shaders/simple_epic.frag");
    free_glyph_atlas_init(&atlas, face);

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
            {
                quit = true;
            }
            break;
            case SDL_EVENT_KEY_DOWN:
            {
                switch (event.key.key)
                {
                case SDLK_BACKSPACE:
                {
                    editor_backspace(&editor);
                    last_stroke = SDL_GetTicks();
                }
                break;

                case SDLK_F2:
                {
                    if (file_path)
                    {
                        editor_save_to_file(&editor, file_path);
                    }
                }
                break;

                case SDLK_RETURN:
                {
                    editor_insert_char(&editor, '\n');
                    last_stroke = SDL_GetTicks();
                }
                break;

                case SDLK_DELETE:
                {
                    editor_delete(&editor);
                    last_stroke = SDL_GetTicks();
                }
                break;

                case SDLK_UP:
                {
                    editor_move_line_up(&editor);
                    last_stroke = SDL_GetTicks();
                }
                break;

                case SDLK_DOWN:
                {
                    editor_move_line_down(&editor);
                    last_stroke = SDL_GetTicks();
                }
                break;

                case SDLK_LEFT:
                {
                    editor_move_line_left(&editor);
                    last_stroke = SDL_GetTicks();
                }
                break;

                case SDLK_RIGHT:
                {
                    editor_move_line_right(&editor);
                    last_stroke = SDL_GetTicks();
                }
                break;
                }
            }
            break;

            case SDL_EVENT_TEXT_INPUT:
            {
                const char *text = event.text.text;
                size_t text_len = strlen(text);
                for (size_t i = 0; i < text_len; ++i)
                {
                    editor_insert_char(&editor, text[i]);
                }
                last_stroke = SDL_GetTicks();
            }
            break;
            }
        }

        int w, h;
        SDL_GetWindowSize(window, &w, &h);
        glViewport(0, 0, w, h);

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        render_editor(window, &atlas, &sr, &editor);

        SDL_GL_SwapWindow(window);

        const Uint32 duration = SDL_GetTicks() - start;
        const Uint32 delta_time_ms = 1000 / FPS;
        if (duration < delta_time_ms)
        {
            SDL_Delay(delta_time_ms - duration);
        }
    }

    return 0;
}