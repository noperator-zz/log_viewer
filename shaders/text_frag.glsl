#version 330 core
in vec2 v_uv;
in vec4 v_fg;
in vec4 v_bg;
out vec4 color;

layout(std140) uniform Globals {
    mat4 u_proj;
    ivec2 glyph_size_px;
    ivec2 scroll_offset_px;
    ivec2 frame_offset_px;
    uint atlas_cols;
    uint z_order;
    bool is_foreground;
};

uniform sampler2D atlas;

void main() {
    if (!is_foreground) {
        color = v_bg;
        return;
    }

    float a = texture(atlas, v_uv).r;
    if (a == 0.0) {
        // Skip transparent foreground pixels. Due to the bearing_y offset, the transparent bottom of a glyph
        //  may overlap the line below it. Due to depth testing, this would cause the top of the next line's glyph
        //  to not be drawn. By discarding transparent pixels, the depth buffer is not updated, and the next line's glyph
        //  passes the depth test.
        discard;
    }
    color = mix(vec4(0, 0, 0, 0), v_fg, a);
//    color = vec4((v_style_mask & 0xFFU), 0, 0, 1);
//    color = vec4(0, v_uv.r, 0, 1);
//    color = vec4(float(char_idx) / 3., 0.5, 0, 1);
//    color = vec4(float(quad) / 6., 0, 0, 1);
}
