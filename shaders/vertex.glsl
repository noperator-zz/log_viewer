layout(location = 0) in uint8_t glyph_idx;
layout(location = 1) in uint8_t style_mask;

uniform int glyph_width;
uniform int glyph_height;
uniform int atlas_cols;
uniform int line_height;
uniform int line_idx;
uniform mat4 u_proj;
uniform vec2 scroll_offset;
uniform sampler2D bearing_table;

flat out int char_idx;
flat out float bearingY;
out vec2 v_uv;

void main() {
    int corner = int(gl_VertexID);
    vec2 quad_offsets[4] = vec2[](
        vec2(0, 0), vec2(1, 0),
        vec2(0, 1), vec2(1, 1)
    );

    int style_idx = style_mask & 3;

    char_idx = int(gl_InstanceID);  // Which glyph in the line
    vec2 offset = quad_offsets[corner] * vec2(glyph_width, glyph_height);
    vec2 pos = vec2(char_idx * glyph_width, line_idx * line_height) + offset - scroll_offset;
    bearingY = texelFetch(bearing_table, ivec2(int(glyph_idx), style_idx), 0).r;
    pos.y += bearingY;

    int atlas_x = int(glyph_idx) * glyph_width;
    int atlas_y = int(style_idx) * glyph_height;
    vec2 uv = vec2(atlas_x, 0) + offset;
    v_uv = uv / vec2(glyph_width * atlas_cols, glyph_height * 4);

    gl_Position = u_proj * vec4(pos, 0.0, 1.0);
}
