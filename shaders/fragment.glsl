#version 330 core
in vec2 v_uv;
in vec4 v_fg, v_bg;
out vec4 color;
uniform sampler2D atlas;
void main() {
    float a = texture(atlas, v_uv).r;
    color = mix(v_bg, v_fg, a);
}
