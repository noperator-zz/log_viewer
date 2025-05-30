#version 330 core
layout(location = 0) in uint glyph_idx_;
layout(location = 1) in uint style_mask;
layout(location = 2) in vec4 fg;
layout(location = 3) in vec4 bg;

uniform bool is_foreground;
uniform uint glyph_width;
uniform uint glyph_height;
uniform uint atlas_cols;
uniform int line_idx;
uniform mat4 u_proj;
uniform ivec2 scroll_offset;
uniform ivec2 frame_offset;
uniform sampler2D bearing_table;

flat out uint char_idx;
flat out uint v_style_mask;
flat out float bearingY;
out vec2 v_uv;
out vec4 v_fg;
out vec4 v_bg;

void main() {
    uint corner = uint(gl_VertexID);
    vec2 quad_offsets[4] = vec2[](
        vec2(0, 0), vec2(1, 0),
        vec2(0, 1), vec2(1, 1)
    );

    char_idx = uint(gl_InstanceID);  // Which glyph in the line
    vec2 offset = quad_offsets[corner] * vec2(glyph_width, glyph_height);
    vec2 pos = vec2(char_idx * glyph_width, line_idx * glyph_height) + frame_offset + offset - scroll_offset;
    v_bg = bg;
    if (!is_foreground) {
        gl_Position = u_proj * vec4(pos, 0.0, 1.0);
        return;
    }

    uint glyph_idx = glyph_idx_ & 0xFFU;
    uint style_idx = style_mask & 3U;
    v_style_mask = style_mask;

    bearingY = texelFetch(bearing_table, ivec2(glyph_idx, style_idx), 0).r;
    pos.y += bearingY;

    uint atlas_x = glyph_idx * glyph_width;
    uint atlas_y = style_idx * glyph_height;
    vec2 uv = vec2(atlas_x, atlas_y) + offset;
    v_uv = uv / vec2(glyph_width * atlas_cols, glyph_height * 4U);
    v_fg = fg;

    gl_Position = u_proj * vec4(pos, 0.0, 1.0);
}
