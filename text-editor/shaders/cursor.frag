#version 330 core

#define BLINK_PERIOD 1.0        // 1秒闪烁周期
#define INPUT_HOLD 0.5          // 输入后0.5秒常亮

uniform float time;
uniform float last_stroke;

void main() {
    float t = time - last_stroke;
    float show = 0.0;
    if (t < INPUT_HOLD) {
        show = 1.0; // 输入后常亮
    } else {
        // 闪烁：每BLINK_PERIOD秒切换一次
        show = mod(floor((t - INPUT_HOLD) / (BLINK_PERIOD / 2.0)), 2.0) < 1.0 ? 1.0 : 0.0;
    }
    gl_FragColor = vec4(1.0) * show;
}
