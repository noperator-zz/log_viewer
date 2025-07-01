#version 400 core
layout(location = 0) in float y;
layout(location = 1) in vec4 fg;

layout(std140) uniform Globals {
    mat4 u_proj;
    ivec2 pos_px;
    ivec2 size_px;
    uint width_px;
    uint z_order;
};

out vec4 v_color;

void main() {
    uint vert_idx = uint(gl_VertexID);
    vec2 vert_pos_offsets[4] = vec2[](
        vec2(0, -0.5), vec2(1, -0.5),
        vec2(0, 0.5), vec2(1, 0.5)
    );

    float z = float(z_order) / 255.f;

    vec2 vert_pos_offset_px = vert_pos_offsets[vert_idx] * vec2(size_px.x, width_px);
    vec2 pos_px_ = vec2(pos_px) + vert_pos_offset_px + vec2(0, y * float(size_px.y));
    v_color = fg;

    gl_Position = u_proj * vec4(pos_px_, z, 1.0);
}
