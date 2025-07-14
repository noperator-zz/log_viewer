#version 330 core
in vec4 v_color;
in vec2 v_uv;

out vec4 color;

uniform sampler2D atlas;

void main() {
    vec4 tex_color = texture(atlas, v_uv);
    color = mix(v_color, tex_color, tex_color.a);
}
