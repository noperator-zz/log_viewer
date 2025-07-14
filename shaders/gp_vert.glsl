#version 330 core
layout(location = 0) in ivec3 pos;
layout(location = 1) in vec4 color;
layout(location = 2) in vec2 uv;

uniform mat4 u_proj;
uniform sampler2D atlas;
uniform int num_tex;

out vec4 v_color;
out vec2 v_uv;

void main() {
    gl_Position = u_proj * vec4(pos.x, pos.y, float(pos.z) / 255.f, 1.0);
    v_color = color;
    v_uv = uv;
}
