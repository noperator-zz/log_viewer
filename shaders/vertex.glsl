#version 330 core
layout(location = 0) in vec2 pos;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec4 fg;
layout(location = 3) in vec4 bg;
out vec2 v_uv;
out vec4 v_fg, v_bg;
void main() {
    v_uv = uv; v_fg = fg; v_bg = bg;
    gl_Position = vec4(pos, 0, 1);
}
