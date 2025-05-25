#include "text_shader.h"

#include "fragment.glsl.h"
#include "vertex.glsl.h"

TextShader::TextShader(Font &font) : shader_(vertex_glsl, fragment_glsl), font_(font) {
}

int TextShader::setup() {
	int ret = shader_.compile();
	if (ret != 0) {
		return ret;
	}
	scroll_offset_loc_ = glGetUniformLocation(shader_.id(), "scroll_offset");
	line_index_loc_ = glGetUniformLocation(shader_.id(), "line_idx");

	set_uniform(1i, shader_, "atlas", 0);
	set_uniform(1i, shader_, "bearing_table", 1);
	set_uniform(1ui, shader_, "glyph_width", font_.size);
	set_uniform(1ui, shader_, "glyph_height", font_.size);
	set_uniform(1ui, shader_, "atlas_cols", font_.num_glyphs);
	set_uniform(1ui, shader_, "line_height", font_.size);
	return 0;
}

void TextShader::create_buffers(GLuint &vao, GLuint &vbo_text, GLuint &vbo_style) {
	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo_text);
	glGenBuffers(1, &vbo_style);
	glBindVertexArray(vao);

	glBindBuffer(GL_ARRAY_BUFFER, vbo_text);
	glVertexAttribIPointer(0, 1, GL_UNSIGNED_INT, 1, (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribDivisor(0, 1);

	glBindBuffer(GL_ARRAY_BUFFER, vbo_style);
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

void TextShader::set_viewport(int x, int y, int w, int h) const {
	float ortho[16] = {
		2.0f / w, 0.0f, 0.0f, 0.0f,
		0.0f, -2.0f / h, 0.0f, 0.0f,
		0.0f, 0.0f, -1.0f, 0.0f,
		-1.0f + (float)x / w, 1.0f - (float)y / h, 1.0f, 1.0f
	};
	set_uniform(Matrix4fv, shader_, "u_proj", 1, GL_FALSE, ortho);
}

void TextShader::set_scroll_offset(float x, float y) const {
	glUniform2ui(scroll_offset_loc_, x, y);
}

void TextShader::set_line_index(int line_index) const {
	glUniform1ui(line_index_loc_, line_index);
}
