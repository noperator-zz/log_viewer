#version 330 core
flat in uint char_idx;
flat in uint v_style_mask;
flat in float bearingY;
in vec2 v_uv;
in vec4 v_fg;
in vec4 v_bg;
out vec4 color;

layout(std140) uniform Globals {
    mat4 u_proj;
    vec2 glyph_size_px;
    ivec2 scroll_offset_px;
    ivec2 frame_offset_px;
    uint atlas_cols;
    bool is_foreground;
};

uniform sampler2D atlas;

void main() {
    if (!is_foreground) {
        color = v_bg;
        return;
    }

    float a = texture(atlas, v_uv).r;
    color = mix(vec4(0, 0, 0, 0), v_fg, a);
//    color = vec4((v_style_mask & 0xFFU), 0, 0, 1);
//    color = vec4(0, v_uv.r, 0, 1);
//    color = vec4(float(char_idx) / 3., 0.5, 0, 1);
//    color = vec4(float(quad) / 6., 0, 0, 1);
}
