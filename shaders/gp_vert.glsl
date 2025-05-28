#version 330 core
layout(location = 0) in ivec2 pos;
layout(location = 1) in vec4 color;
uniform mat4 u_proj;

out vec4 v_color;

void main() {
    gl_Position = u_proj * vec4(vec2(pos), 0.0, 1.0);
    v_color = color;
}
