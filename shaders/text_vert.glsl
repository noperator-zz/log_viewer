#version 400 core
layout(location = 0) in uvec2 char_pos;
layout(location = 1) in vec4 fg;
layout(location = 2) in vec4 bg;
layout(location = 3) in uint style_mask;
layout(location = 4) in uint glyph_idx_;
layout(location = 5) in uint _padding;

layout(std140) uniform Globals {
    mat4 u_proj;
    ivec2 glyph_size_px;
    ivec2 scroll_offset_px;
    ivec2 frame_offset_px;
    uint atlas_cols;
    uint z_order;
};

uniform sampler2D atlas;

out vec2 v_uv;
out vec4 v_fg;
out vec4 v_bg;

void main() {
    uint corner = uint(gl_VertexID);
    vec2 quad_offsets[4] = vec2[](
        vec2(0, 0), vec2(1, 0),
        vec2(0, 1), vec2(1, 1)
    );

    float z = float(z_order) / 255.f;

//    char_idx = uint(gl_InstanceID);  // Which glyph in the line
    vec2 corner_quant = quad_offsets[corner];
    // Pre-calculate the line offset using integer arithmetic to acheive a greater scroll range compared to float (31-bit vs 24-bit mantissa).
    ivec2 line_offset_px = ivec2(0, char_pos.y) * glyph_size_px.y - ivec2(scroll_offset_px);
    vec2 pos_px = (vec2(char_pos.x, 0) + corner_quant) * glyph_size_px + frame_offset_px + line_offset_px;
    v_bg = bg;

    uint glyph_idx = glyph_idx_ & 0xFFU;
    uint style_idx = style_mask & 3U;

    v_uv = (vec2(glyph_idx, style_idx) + corner_quant) / vec2(atlas_cols, 4U);
    v_fg = fg;

    gl_Position = u_proj * vec4(pos_px, z, 1.0);
}
