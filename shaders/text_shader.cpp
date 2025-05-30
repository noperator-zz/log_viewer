#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "text_shader.h"

#include "text_vert.glsl.h"
#include "text_frag.glsl.h"

using namespace glm;

TextShader::TextShader(const Font &font) : shader_(text_vert_glsl, text_frag_glsl), font_(font) {
}

int TextShader::init(const Font &font) {
	if (inst_) {
		return -1; // Already initialized
	}
	inst_ = std::unique_ptr<TextShader>(new TextShader(font));
	int ret = inst_->setup();
	if (ret != 0) {
		inst_.reset();
	}
	return ret;
}

int TextShader::setup() {
	int ret = shader_.compile();
	if (ret != 0) {
		return ret;
	}
	shader_.use();
	frame_offset_loc_ = glGetUniformLocation(shader_.id(), "frame_offset");
	scroll_offset_loc_ = glGetUniformLocation(shader_.id(), "scroll_offset");
	line_index_loc_ = glGetUniformLocation(shader_.id(), "line_idx");
	is_foreground_loc_ = glGetUniformLocation(shader_.id(), "is_foreground");

	set_uniform(1i, shader_, "atlas", 0);
	set_uniform(1i, shader_, "bearing_table", 1);
	set_uniform(1ui, shader_, "glyph_width", font_.size.x);
	set_uniform(1ui, shader_, "glyph_height", font_.size.y);
	set_uniform(1ui, shader_, "atlas_cols", font_.num_glyphs);
	set_scroll_offset({0, 0});
	set_line_index(0);
	set_is_foreground(false);

	return 0;
}

void TextShader::create_buffers(GLuint &vao, GLuint &vbo_text, GLuint &vbo_style, size_t total_size) {
	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo_text);
	glGenBuffers(1, &vbo_style);
	glBindVertexArray(vao);

	glBindBuffer(GL_ARRAY_BUFFER, vbo_text);
	glBufferData(GL_ARRAY_BUFFER, total_size / sizeof(CharStyle), nullptr, GL_DYNAMIC_DRAW);
	glClearBufferData(GL_SHADER_STORAGE_BUFFER, GL_R8UI, GL_RED_INTEGER, GL_UNSIGNED_BYTE, nullptr);

	glVertexAttribIPointer(0, 1, GL_UNSIGNED_BYTE, 1, (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribDivisor(0, 1);

	glBindBuffer(GL_ARRAY_BUFFER, vbo_style);
	glBufferData(GL_ARRAY_BUFFER, total_size, nullptr, GL_DYNAMIC_DRAW);
	glClearBufferData(GL_SHADER_STORAGE_BUFFER, GL_R8UI, GL_RED_INTEGER, GL_UNSIGNED_BYTE, nullptr);

	glVertexAttribIPointer(1, 1, GL_UNSIGNED_BYTE, sizeof(CharStyle), (void*)offsetof(CharStyle, style));
	glEnableVertexAttribArray(1);
	glVertexAttribDivisor(1, 1);

	glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(CharStyle), (void*)offsetof(CharStyle, fg));
	glEnableVertexAttribArray(2);
	glVertexAttribDivisor(2, 1);

	glVertexAttribPointer(3, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(CharStyle), (void*)offsetof(CharStyle, bg));
	glEnableVertexAttribArray(3);
	glVertexAttribDivisor(3, 1);
}

void TextShader::use() {
	inst_->shader_.use();
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, inst_->font_.tex_atlas());
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, inst_->font_.tex_bearing());
}

void TextShader::set_viewport(ivec2 pos, ivec2 size) {
	auto mat = ortho<float>(
		pos.x, pos.x + size.x,
		pos.y + size.y, pos.y,
		-1.0f, 1.0f
	);
	inst_->shader_.use();
	set_uniform(Matrix4fv, inst_->shader_, "u_proj", 1, GL_FALSE, value_ptr(mat));
}

void TextShader::set_frame_offset(ivec2 offset) {
	inst_->shader_.use();
	glUniform2i(inst_->frame_offset_loc_, offset.x, offset.y);
}

void TextShader::set_scroll_offset(ivec2 offset) {
	inst_->shader_.use();
	glUniform2i(inst_->scroll_offset_loc_, offset.x, offset.y);
}

void TextShader::set_line_index(int line_index) {
	inst_->shader_.use();
	glUniform1i(inst_->line_index_loc_, line_index);
}

void TextShader::set_is_foreground(bool is_foreground) {
	inst_->shader_.use();
	glUniform1i(inst_->is_foreground_loc_, is_foreground ? 1 : 0);
}

const Font &TextShader::font() {
	return inst_->font_;
}
