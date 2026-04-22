#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "SDL3/SDL.h"
#include "SDL3/SDL_render.h"
#include "SDL3_ttf/SDL_ttf.h"
#include "GL/glew.h"

#include "./la.h"
#include "./font.h"
#include "./head.h"
#include "./editor.h"
#include "./sdl_extra.h"

#define STB_IMAGE_IMPLEMENTATION
#include "./stb_image.h"

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

Editor editor = {0};
Vec2f camer_pos = {0.0f, 0.0f};
Vec2f camer_vel = {0.0f, 0.0f};

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

Vec2f camer_project_point(SDL_Window *window, Vec2f point)
{
    return vec2f_add(
        vec2f_sub(point, camer_pos),
        vec2f_mul(Window_size(window), vec2fs(0.5f)));
}

void render_cursor(SDL_Renderer *renderer, SDL_Window *window, const Font *font)
{
    const Vec2f pos = camer_project_point(window, (Vec2f){
                                                      .x = editor.cursor_col * FONT_CHAR_WIDTH * FONT_SCALE,
                                                      .y = editor.cursor_row * FONT_CHAR_HEIGHT * FONT_SCALE});

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

void useage(FILE *stream)
{
    fprintf(stream, "Usage: text-editor [file]\n");
}

#define GL_DEFINE
#ifdef GL_DEFINE

void MessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam)
{
    fprintf(stderr, "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
            (type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""),
            type, severity, message);
}

typedef struct{
    Vec2f pos;
    float scale;
    float ch;
    Vec4f color;
} Glyph;

typedef enum{
    GLYPH_ATTR_POS = 0,
    GLYPH_ATTR_SCALE = 1,
    GLYPH_ATTR_CHAR = 2,
    GLYPH_ATTR_COLOR = 3,
    COUNT_GLYPH_ATTR,
} Glyph_Attr;

typedef struct{
    size_t offset;
    size_t comps;
} Glyph_Attr_Def;

const static Glyph_Attr_Def glyph_attr_defs[COUNT_GLYPH_ATTR] = {
    [GLYPH_ATTR_POS] = {offsetof(Glyph, pos), 2},
    [GLYPH_ATTR_SCALE] = {offsetof(Glyph, scale), 1},
    [GLYPH_ATTR_CHAR] = {offsetof(Glyph, ch), 1},
    [GLYPH_ATTR_COLOR] = {offsetof(Glyph, color), 4},
};

static_assert(COUNT_GLYPH_ATTR == 4, "glyph_attr_defs size must match Glyph_Attr count");

#define GLYPH_BUFFER_CAPACITY 1024
Glyph glyph_buffer[GLYPH_BUFFER_CAPACITY];
size_t glyph_buffer_count = 0;

void glyph_buffer_push(Glyph glyph)
{
    assert(glyph_buffer_count < GLYPH_BUFFER_CAPACITY);
    glyph_buffer[glyph_buffer_count++] = glyph;
}

void glyph_buffer_sync(void)
{
    glBufferSubData(GL_ARRAY_BUFFER, 0, glyph_buffer_count * sizeof(Glyph), glyph_buffer);
}

void gl_render_text(const char* text, size_t text_size, Vec2f pos, float scale, Vec4f color)
{
    for(size_t i = 0; i < text_size; ++i){
        const Vec2f char_size = vec2f(FONT_CHAR_WIDTH, FONT_CHAR_HEIGHT);
        const Glyph glyph = {
            .pos = vec2f_add(pos, vec2f_mul3(char_size,
                                    vec2f((float)i, 0.0f),
                                    vec2fs(scale))),
            .scale = scale,
            .ch = text[i],
            .color = color
        };
        glyph_buffer_push(glyph);
    }
}

int main(int argc, char *argv[])
{
    SDL_Window *window =
        scp(SDL_CreateWindow("Text Editor", SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL));

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

    glEnable(GL_DEBUG_OUTPUT);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    if (GLEW_ARB_debug_output){
        glDebugMessageCallback(MessageCallback, NULL);
    }else{
        fprintf(stderr, "ARB_debug_output not available\n");
    }

    
    GLuint vert_shader = 0, frag_shader = 0, shader_program = 0;
    if (!compile_shader_file("./shaders/font.vert", GL_VERTEX_SHADER, &vert_shader))
    {
        fprintf(stderr, "Failed to compile vertex shader\n");
        exit(EXIT_FAILURE);
    }
    if (!compile_shader_file("./shaders/font.frag", GL_FRAGMENT_SHADER, &frag_shader))
    {
        fprintf(stderr, "Failed to compile fragment shader\n");
        exit(EXIT_FAILURE);
    }
    if (!link_program(vert_shader, frag_shader, &shader_program))
    {
        fprintf(stderr, "Failed to link shader program\n");
        exit(EXIT_FAILURE);
    }

    glUseProgram(shader_program);
    glGetUniformLocation(shader_program, "font");

    GLint resolution_uniform;
    GLint time_uniform = glGetUniformLocation(shader_program, "time");
    resolution_uniform = glGetUniformLocation(shader_program, "resolution");

    glUniform2f(resolution_uniform, SCREEN_WIDTH, SCREEN_HEIGHT);
    

    {
        const char *file_path = "./charmap-oldschool_white.png";
        int width, height, channels;
        unsigned char *data = stbi_load(file_path, &width, &height, &channels, STBI_rgb_alpha);
        if (data == NULL){
            fprintf(stderr, "ERROR: could not load file %s: %s\n", file_path, stbi_failure_reason());
            exit(EXIT_FAILURE);
        }

        glActiveTexture(GL_TEXTURE0);

        GLuint font_texture = 0;
        glGenTextures(1, &font_texture);
        glBindTexture(GL_TEXTURE_2D, font_texture);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    }

    GLuint vao = 0;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    GLuint vbo = 0;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Glyph) * GLYPH_BUFFER_CAPACITY, glyph_buffer, GL_DYNAMIC_DRAW);
    for(Glyph_Attr attr = 0; attr < COUNT_GLYPH_ATTR; ++attr){
        const Glyph_Attr_Def def = glyph_attr_defs[attr];
        glEnableVertexAttribArray(attr);
        glVertexAttribPointer(attr, def.comps, GL_FLOAT, GL_FALSE, sizeof(Glyph), (void*)def.offset);
        glVertexAttribDivisor(attr, 1);
    }

    const char* text = "hello world";
    Vec4f color = vec4f(0.0f, 1.0f, 0.0f, 1.0f);
    gl_render_text(text, strlen(text), vec2fs(0.0f), 5.0f, color);
    glyph_buffer_sync();

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
            }
        }

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glUniform1f(time_uniform, (float)SDL_GetTicks() / 1000.0f);
        glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, (GLsizei)glyph_buffer_count);
        SDL_GL_SwapWindow(window);
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
        FILE *file = fopen(filePath, "r");
        if (file)
        {
            editor_load_from_file(&editor, file);
        }
        fclose(file);
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
            const Vec2f line_pos = camer_project_point(window, vec2f(0.0f, row * FONT_CHAR_HEIGHT * FONT_SCALE));

            render_text_sized(renderer, &font, line->chars, line->size, line_pos, 0xFFFFFFFF, FONT_SCALE);
        }

        render_cursor(renderer, window, &font);

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