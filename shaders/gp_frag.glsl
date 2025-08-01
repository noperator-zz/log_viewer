#version 330 core
in vec4 v_color;
in vec2 v_uv;

out vec4 color;

uniform sampler2D atlas;

vec4 blendOver(vec4 top, vec4 bottom) {
    float alpha = top.a + bottom.a * (1.0 - top.a);
    vec3 color = (top.rgb * top.a + bottom.rgb * bottom.a * (1.0 - top.a)) / max(alpha, 0.0001);
    return vec4(color, alpha);
}

void main() {
    vec4 tex_color = texture(atlas, v_uv);
    color = blendOver(tex_color, v_color);
}
