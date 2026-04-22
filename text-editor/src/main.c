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

#define GL_DEFINE
#ifdef GL_DEFINE

void MessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam)
{
    fprintf(stderr, "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
            (type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""),
            type, severity, message);
}

typedef struct{
    Vec2i tile;
    int ch;
    Vec4f fg_color;
    Vec4f bg_color;
} Glyph;

typedef enum{
    GLYPH_ATTR_TILE = 0,
    GLYPH_ATTR_CHAR = 1,
    GLYPH_ATTR_FG_COLOR = 2,
    GLYPH_ATTR_BG_COLOR = 3,
    COUNT_GLYPH_ATTR,
} Glyph_Attr;

typedef struct{
    size_t offset;
    size_t comps;
    GLenum type;
} Glyph_Attr_Def;

const static Glyph_Attr_Def glyph_attr_defs[COUNT_GLYPH_ATTR] = {
    [GLYPH_ATTR_TILE] = {offsetof(Glyph, tile), 2, GL_INT},
    [GLYPH_ATTR_CHAR] = {offsetof(Glyph, ch), 1, GL_INT},
    [GLYPH_ATTR_FG_COLOR] = {offsetof(Glyph, fg_color), 4, GL_FLOAT},
    [GLYPH_ATTR_BG_COLOR] = {offsetof(Glyph, bg_color), 4, GL_FLOAT},
};

static_assert(COUNT_GLYPH_ATTR == 4, "glyph_attr_defs size must match Glyph_Attr count");

#define GLYPH_BUFFER_CAPACITY (1024 * 640)
Glyph glyph_buffer[GLYPH_BUFFER_CAPACITY];
size_t glyph_buffer_count = 0;

void glyph_buffer_clear(void)
{
    glyph_buffer_count = 0;
}

void glyph_buffer_push(Glyph glyph)
{
    assert(glyph_buffer_count < GLYPH_BUFFER_CAPACITY);
    glyph_buffer[glyph_buffer_count++] = glyph;
}

void glyph_buffer_sync(void)
{
    glBufferSubData(GL_ARRAY_BUFFER, 0, glyph_buffer_count * sizeof(Glyph), glyph_buffer);
}

void gl_render_text_sized(const char* text, size_t text_size, Vec2i tile, Vec4f fg_color, Vec4f bg_color)
{
    for(size_t i = 0; i < text_size; ++i){
        const Glyph glyph = {
            .tile = vec2i_add(tile, vec2i(i, 0)),
            .ch = text[i],
            .fg_color = fg_color,
            .bg_color = bg_color,
        };
        glyph_buffer_push(glyph);
    }
}

void gl_render_text(const char* text, Vec2i tile, Vec4f fg_color, Vec4f bg_color)
{
    gl_render_text_sized(text, strlen(text), tile, fg_color, bg_color);
}

void gl_render_cursor()
{
    const char *c = editor_char_under_cursor(&editor);
    Vec2i tile =  vec2i(editor.cursor_col, -(int)editor.cursor_row);
    gl_render_text_sized(c ? c : " ", 1, tile, vec4fs(0.0f), vec4fs(1.0f));
}

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
            fclose(file);
        }
    }

    scc(SDL_Init(SDL_INIT_VIDEO));
    // 1. 创建 SDL 窗口，启用 OpenGL
    SDL_Window *window =
        scp(SDL_CreateWindow("Text Editor", SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL));

    // 启用文本输入，确保 SDL_EVENT_TEXT_INPUT 能正常响应
    SDL_StartTextInput(window);

    // 2. 设置并检查 OpenGL 上下文版本（3.3 Core Profile）
    {
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

        int major, minor;
        SDL_GL_GetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, &major);
        SDL_GL_GetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, &minor);
        printf("OpenGL context version: %d.%d\n", major, minor);
    }

    // 3. 创建 OpenGL 上下文
    scp(SDL_GL_CreateContext(window));

    // 4. 初始化 GLEW（OpenGL 扩展加载）
    if (glewInit() != GLEW_OK)
    {
        fprintf(stderr, "Failed to initialize GLEW\n");
        exit(EXIT_FAILURE);
    }

    // 5. 启用 OpenGL 调试输出和混合模式
    glEnable(GL_DEBUG_OUTPUT);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    if (GLEW_ARB_debug_output){
        glDebugMessageCallback(MessageCallback, NULL);
    }else{
        fprintf(stderr, "GLEW_ARB_debug_output not available\n");
    }

    // 6. 编译着色器并链接着色器程序，获取 uniform 位置
    GLint resolution_uniform;
    GLint time_uniform;
    GLint scale_uniform;
    GLint camera_uniform;
    {
        GLuint vert_shader = 0, frag_shader = 0, shader_program = 0;
        // 编译顶点着色器
        if (!compile_shader_file("./shaders/font.vert", GL_VERTEX_SHADER, &vert_shader))
        {
            fprintf(stderr, "Failed to compile vertex shader\n");
            exit(EXIT_FAILURE);
        }
        // 编译片元着色器
        if (!compile_shader_file("./shaders/font.frag", GL_FRAGMENT_SHADER, &frag_shader))
        {
            fprintf(stderr, "Failed to compile fragment shader\n");
            exit(EXIT_FAILURE);
        }
        // 链接着色器程序
        if (!link_program(vert_shader, frag_shader, &shader_program))
        {
            fprintf(stderr, "Failed to link shader program\n");
            exit(EXIT_FAILURE);
        }

        glUseProgram(shader_program);
        glGetUniformLocation(shader_program, "font");

        // 获取 uniform 变量位置
        time_uniform = glGetUniformLocation(shader_program, "time");
        resolution_uniform = glGetUniformLocation(shader_program, "resolution");
        scale_uniform = glGetUniformLocation(shader_program, "scale");
        camera_uniform = glGetUniformLocation(shader_program, "camera");
        // 设置缩放 uniform
        glUniform2f(scale_uniform, FONT_SCALE, FONT_SCALE);
    }

    // 7. 加载字体纹理到 OpenGL
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

        // 设置纹理参数
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        // 上传纹理数据
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    }

    // 8. 设置 VAO/VBO 及顶点属性指针
    {
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
            // 根据属性定义选择正确的顶点属性指针类型
            if (def.type == GL_INT) {
                glVertexAttribIPointer(attr, def.comps, def.type, sizeof(Glyph), (void*)def.offset);
            } else {
                glVertexAttribPointer(attr, def.comps, def.type, GL_FALSE, sizeof(Glyph), (void*)def.offset);
            }
            glVertexAttribDivisor(attr, 1);
        }
    }

    // 10. 主循环：事件处理与渲染
    bool quit = false;
    while (!quit)
    {
        // 事件处理
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
            }
            break;
            case SDL_EVENT_TEXT_INPUT:
                editor_insert_text_before_cursor(&editor, event.text.text);
                break;
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
                if(event.button.button == SDL_BUTTON_LEFT){
                    const Vec2f mouse_pos = {.x = (float)event.button.x, .y = (float)event.button.y};
                    const Vec2f world_pos = vec2f_sub(
                        vec2f_add(mouse_pos, camer_pos),
                        vec2f_mul(Window_size(window), vec2fs(0.5f))
                    );
                    const size_t col = (size_t)(world_pos.x / (FONT_CHAR_WIDTH * FONT_SCALE));
                    const size_t row = (size_t)(world_pos.y / (FONT_CHAR_HEIGHT * FONT_SCALE));
                    editor.cursor_col = col;
                    editor.cursor_row = row;
                }
            break;
            }
        }

        {
            const Vec2f cursor_pos = {
                .x = editor.cursor_col * FONT_CHAR_WIDTH * FONT_SCALE,
                .y = -(int)editor.cursor_row * FONT_CHAR_HEIGHT * FONT_SCALE};

            camer_vel = vec2f_mul(vec2f_sub(cursor_pos, camer_pos), vec2fs(2.0f));
            camer_pos = vec2f_add(camer_pos, vec2f_mul(camer_vel, vec2fs(DELTA_TIME)));
        }

        // 视口和分辨率 uniform 更新
        {
            int  w, h;
            SDL_GetWindowSize(window, &w, &h);
            glViewport(0, 0, w, h);
            glUniform2f(resolution_uniform, (float)w, (float) h);
        }

        glyph_buffer_clear();
        for (size_t row = 0; row < editor.size; ++row)
        {
            const Line *line = editor.lines + row;
            gl_render_text_sized(line->chars, line->size, vec2i(0, -((int)row)), vec4f(1.0f, 1.0f, 1.0f, 1.0f), vec4fs(0.0f));
        }
        glyph_buffer_sync();

        // 清屏并渲染一帧
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUniform1f(time_uniform, (float)SDL_GetTicks() / 1000.0f);
        glUniform2f(camera_uniform, camer_pos.x, camer_pos.y);

        glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, (GLsizei)glyph_buffer_count);

        glyph_buffer_clear();
        gl_render_cursor();
        glyph_buffer_sync();
        glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, (GLsizei)glyph_buffer_count);

        SDL_GL_SwapWindow(window);

        const Uint32 duration = (SDL_GetTicks() - start); // 更新SDL内部计时器，确保事件时间戳正确
        const Uint32 frame_time = 1000 / FPS;
        if (duration < frame_time)
        {
            SDL_Delay(frame_time - duration);
        }
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