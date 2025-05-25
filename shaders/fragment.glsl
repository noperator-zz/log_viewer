#version 330 core
flat in int char_idx;
in vec2 v_uv;
out vec4 color;
uniform sampler2D atlas;
void main() {
    float a = texture(atlas, v_uv).r;
    color = mix(vec4(0,0,0,1), vec4(1,1,1,1), a);
//    color = vec4(v_uv, 0, 1);
//    color = vec4(float(char_idx) / 3., 0.5, 0, 1);
//    color = vec4(float(quad) / 6., 0, 0, 1);
}
