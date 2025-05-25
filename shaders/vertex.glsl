layout(location = 0) in uint8_t glyph_idx;
layout(location = 1) in uint8_t line_idx;

uniform int glyph_width;
uniform int glyph_height;
uniform int atlas_cols;
uniform vec2 origin;
uniform mat4 u_proj;

flat out int char_idx;
out vec2 v_uv;

void main() {
    int corner = int(gl_VertexID);
    vec2 quad_offsets[4] = vec2[](
        vec2(0, 0), vec2(1, 0),
        vec2(0, 1), vec2(1, 1)
    );

    char_idx = int(gl_InstanceID);  // Which glyph in the line
    vec2 offset = quad_offsets[corner] * vec2(glyph_width, glyph_height);
    vec2 pos = origin + vec2(char_idx * glyph_width, line_idx * glyph_height) + offset;

    int atlas_x = int(glyph_idx) * glyph_width;
    vec2 uv = vec2(atlas_x, 0) + offset;
    v_uv = uv / vec2(glyph_width * atlas_cols, glyph_height);

    gl_Position = u_proj * vec4(pos, 0.0, 1.0);
}
