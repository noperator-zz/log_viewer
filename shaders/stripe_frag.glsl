#version 330 core

layout(std140) uniform Globals {
    mat4 u_proj;
    ivec2 pos_px;
    ivec2 size_px;
    uint width_px;
    uint z_order;
};

in vec4 v_color;
out vec4 color;

void main() {
    color = v_color;
}
