#version 330 core

#define FONT_WIDTH 128
#define FONT_HEIGHT 64
#define FONT_COLS 18
#define FONT_ROWS 7
#define FONT_CHAR_WIDTH  (FONT_WIDTH / FONT_COLS)
#define FONT_CHAR_HEIGHT (FONT_HEIGHT / FONT_ROWS)
#define FONT_CHAR_WIDTH_UV  (float(FONT_CHAR_WIDTH) / float(FONT_WIDTH))
#define FONT_CHAR_HEIGHT_UV (float(FONT_CHAR_HEIGHT) / float(FONT_HEIGHT))

#define ASCII_DISPLAY_LOW 32
#define ASCII_DISPLAY_HIGH 126

uniform sampler2D font;
uniform float time;

flat in int glyph_ch;
in vec2 uv;
in vec4 glyph_fg_color;
in vec4 glyph_bg_color;

float map01(float x)
{
    return (x+1) / 2.0;
}

vec3 hsl2rgb(vec3 hsl)
{
    vec3 rgb = clamp( abs(mod(hsl.x * 6.0 + vec3(0.0,4.0,2.0), 6.0) - 3.0) - 1.0, 0.0, 1.0 );
    return hsl.z + hsl.y * (rgb - 0.5) * (1.0 - abs(2.0 * hsl.z - 1.0));
}

void main()
{
    int ch = int(glyph_ch);
    if(!(ASCII_DISPLAY_LOW <= ch && ch <= ASCII_DISPLAY_HIGH)){
        ch = 63;
    }

    int index = ch - ASCII_DISPLAY_LOW;

    float x = float(index % FONT_COLS) * FONT_CHAR_WIDTH_UV;
    float y = float(index / FONT_COLS) * FONT_CHAR_HEIGHT_UV;

    vec2 pos = vec2(x, y + FONT_CHAR_HEIGHT_UV);
    vec2 size = vec2(FONT_CHAR_WIDTH_UV, -FONT_CHAR_HEIGHT_UV);
    vec2 t = pos + uv * size;
    vec4 tc = texture(font, t);


    vec4 rainbow = vec4(hsl2rgb(vec3(map01(time * 0.1), 1.0, 0.5)), 1.0);
    gl_FragColor = glyph_bg_color * (1.0 - tc.x) + tc.x * glyph_fg_color * rainbow;
}