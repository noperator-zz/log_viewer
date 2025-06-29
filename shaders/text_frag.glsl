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
};

uniform sampler2D atlas;

vec4 blendOver(vec4 top, vec4 bottom) {
    float alpha = top.a + bottom.a * (1.0 - top.a);
    vec3 color = (top.rgb * top.a + bottom.rgb * bottom.a * (1.0 - top.a)) / max(alpha, 0.0001);
    return vec4(color, alpha);
}

void main() {
    // TODO convert SRGB to linear before blending, then convert back to SRGB.
    //  There may be a way to get OpenGL to do this automatically by specifying th correct texture format.

    float a = texture(atlas, v_uv).r;
    vec4 fg = mix(v_bg, v_fg, a);
//    color = vec4((v_style_mask & 0xFFU), 0, 0, 1);
//    color = vec4(0, v_uv.r, 0, 1);
//    color = vec4(float(char_idx) / 3., 0.5, 0, 1);
//    color = vec4(float(quad) / 6., 0, 0, 1);

    color = blendOver(fg, v_bg);
}
