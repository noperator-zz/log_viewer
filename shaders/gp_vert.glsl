#version 330 core
layout(location = 0) in ivec3 pos;
layout(location = 1) in vec4 color;
uniform mat4 u_proj;

out vec4 v_color;

void main() {
    gl_Position = u_proj * vec4(pos.x, pos.y, float(pos.z) / 255.f, 1.0);
    v_color = color;
}
