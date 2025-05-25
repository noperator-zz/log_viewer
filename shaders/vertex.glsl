//#version 330 compatibility
layout(location = 0) in uint8_t glyph_idx;
layout(location = 1) in uint8_t style_mask;
layout(location = 2) in vec3 fg;
layout(location = 3) in vec3 bg;

uniform uint glyph_width;
uniform uint glyph_height;
uniform uint atlas_cols;
uniform uint line_height;
uniform uint line_idx;
uniform mat4 u_proj;
uniform uvec2 scroll_offset;
uniform sampler2D bearing_table;

flat out uint char_idx;
flat out float bearingY;
out vec2 v_uv;
out vec3 v_fg;
out vec3 v_bg;

void main() {
    uint corner = uint(gl_VertexID);
    vec2 quad_offsets[4] = vec2[](
        vec2(0, 0), vec2(1, 0),
        vec2(0, 1), vec2(1, 1)
    );

    uint style_idx = style_mask & 3U;

    char_idx = uint(gl_InstanceID);  // Which glyph in the line
    vec2 offset = quad_offsets[corner] * vec2(glyph_width, glyph_height);
    vec2 pos = vec2(char_idx * glyph_width, line_idx * line_height) + offset - scroll_offset;
    bearingY = texelFetch(bearing_table, ivec2(glyph_idx, style_idx), 0).r;
    pos.y += bearingY;

    uint atlas_x = glyph_idx * glyph_width;
    uint atlas_y = style_idx * glyph_height;
    vec2 uv = vec2(atlas_x, 0) + offset;
    v_uv = uv / vec2(glyph_width * atlas_cols, glyph_height * 4U);

    gl_Position = u_proj * vec4(pos, 0.0, 1.0);
}
