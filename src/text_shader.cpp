#include "text_shader.h"

#include "fragment.glsl.h"
#include "vertex.glsl.h"

using namespace glm;

TextShader::TextShader(const Font &font) : shader_(vertex_glsl, fragment_glsl), font_(font) {
}

int TextShader::setup() {
	int ret = shader_.compile();
	if (ret != 0) {
		return ret;
	}
	scroll_offset_loc_ = glGetUniformLocation(shader_.id(), "scroll_offset");
	line_index_loc_ = glGetUniformLocation(shader_.id(), "line_idx");
	line_height_loc_ = glGetUniformLocation(shader_.id(), "line_height");

	set_uniform(1i, shader_, "atlas", 0);
	set_uniform(1i, shader_, "bearing_table", 1);
	set_uniform(1ui, shader_, "glyph_width", font_.size.x);
	set_uniform(1ui, shader_, "glyph_height", font_.size.y);
	set_uniform(1ui, shader_, "atlas_cols", font_.num_glyphs);
	set_uniform(1ui, shader_, "line_height", font_.size.y);
	return 0;
}

void TextShader::create_buffers(GLuint &vao, GLuint &vbo_text, GLuint &vbo_style, size_t total_size) {
	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo_text);
	glGenBuffers(1, &vbo_style);
	glBindVertexArray(vao);

	glBindBuffer(GL_ARRAY_BUFFER, vbo_text);
	glBufferData(GL_ARRAY_BUFFER, total_size / sizeof(CharStyle), nullptr, GL_DYNAMIC_DRAW);
	glVertexAttribIPointer(0, 1, GL_UNSIGNED_INT, 1, (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribDivisor(0, 1);

	glBindBuffer(GL_ARRAY_BUFFER, vbo_style);
	glBufferData(GL_ARRAY_BUFFER, total_size, nullptr, GL_DYNAMIC_DRAW);
	glVertexAttribIPointer(1, 1, GL_UNSIGNED_INT, sizeof(CharStyle), (void*)offsetof(CharStyle, style));
	glEnableVertexAttribArray(1);
	glVertexAttribDivisor(1, 1);

	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(CharStyle), (void*)offsetof(CharStyle, fg));
	glEnableVertexAttribArray(2);
	glVertexAttribDivisor(2, 1);

	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(CharStyle), (void*)offsetof(CharStyle, bg));
	glEnableVertexAttribArray(3);
	glVertexAttribDivisor(3, 1);
}

void TextShader::use(GLuint vao) const {
	shader_.use();
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, font_.tex_atlas());
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, font_.tex_bearing());
}

void TextShader::set_viewport(uvec4 rect) const {
	float ortho[16] = {
		2.0f / rect.z, 0.0f, 0.0f, 0.0f,
		0.0f, -2.0f / rect.w, 0.0f, 0.0f,
		0.0f, 0.0f, -1.0f, 0.0f,
		-1.0f + (float)rect.x / rect.z, 1.0f - (float)rect.y / rect.w, 1.0f, 1.0f
	};
	set_uniform(Matrix4fv, shader_, "u_proj", 1, GL_FALSE, ortho);
}

void TextShader::set_scroll_offset(uvec2 scroll) const {
	glUniform2ui(scroll_offset_loc_, scroll.x, scroll.y);
}

void TextShader::set_line_index(uint line_index) const {
	glUniform1ui(line_index_loc_, line_index);
}

void TextShader::set_line_height(uint line_height) const {
	glUniform1ui(line_height_loc_, line_height);
}
