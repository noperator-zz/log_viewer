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
	globals_idx_ = glGetUniformBlockIndex(shader_.id(), "Globals");
	glUniformBlockBinding(shader_.id(), globals_idx_, 0);

	set_uniform(1i, shader_, "atlas", 0);
	set_uniform(1i, shader_, "bearing_table", 1);

	globals = {
		.u_proj={},
		.glyph_size_px=inst_->font_.size,
		.scroll_offset_px={},
		.frame_offset_px={},
		.atlas_cols=inst_->font_.num_glyphs,
		.z_order={},
	};

	return 0;
}

void TextShader::create_buffers(Buffer &buf, size_t total_size) {
	glGenVertexArrays(1, &buf.vao);
	glBindVertexArray(buf.vao);

	glGenBuffers(1, &buf.vbo_style);
	glBindBuffer(GL_ARRAY_BUFFER, buf.vbo_style);
	glBufferData(GL_ARRAY_BUFFER, total_size, nullptr, GL_DYNAMIC_DRAW);
	// glClearBufferData(GL_ARRAY_BUFFER, GL_R8UI, GL_RED_INTEGER, GL_UNSIGNED_BYTE, nullptr);

	glVertexAttribIPointer(0, 2, GL_UNSIGNED_INT, sizeof(CharStyle), (void*)offsetof(CharStyle, char_pos));
	glEnableVertexAttribArray(0);
	glVertexAttribDivisor(0, 1);

	glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(CharStyle), (void*)offsetof(CharStyle, fg));
	glEnableVertexAttribArray(1);
	glVertexAttribDivisor(1, 1);

	glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(CharStyle), (void*)offsetof(CharStyle, bg));
	glEnableVertexAttribArray(2);
	glVertexAttribDivisor(2, 1);

	glVertexAttribIPointer(3, 1, GL_UNSIGNED_BYTE, sizeof(CharStyle), (void*)offsetof(CharStyle, style));
	glEnableVertexAttribArray(3);
	glVertexAttribDivisor(3, 1);

	glVertexAttribIPointer(4, 1, GL_UNSIGNED_BYTE, sizeof(CharStyle), (void*)offsetof(CharStyle, glyph));
	glEnableVertexAttribArray(4);
	glVertexAttribDivisor(4, 1);

	glVertexAttribIPointer(5, 2, GL_UNSIGNED_BYTE, sizeof(CharStyle), (void*)offsetof(CharStyle, _padding));
	glEnableVertexAttribArray(5);
	glVertexAttribDivisor(5, 1);

	glGenBuffers(1, &buf.ubo_globals);
	glBindBuffer(GL_UNIFORM_BUFFER, buf.ubo_globals);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(UniformGlobals), nullptr, GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_UNIFORM_BUFFER, 0, buf.ubo_globals);
}

void TextShader::destroy_buffers(Buffer &buf) {
	glDeleteBuffers(1, &buf.vbo_style);
	glDeleteBuffers(1, &buf.ubo_globals);
	glDeleteVertexArrays(1, &buf.vao);
	buf = {};
}

void TextShader::update_uniforms() {
	// glBindBufferBase(GL_UNIFORM_BUFFER, 0, buf.ubo_globals);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(inst_->globals), &inst_->globals);
}

void TextShader::use(const Buffer &buf) {
	inst_->shader_.use();
	// TODO can remove these?
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, inst_->font_.tex_atlas());

	glBindVertexArray(buf.vao);
	glBindBuffer(GL_ARRAY_BUFFER, buf.vbo_style);
	glBindBuffer(GL_UNIFORM_BUFFER, buf.ubo_globals);

	glBindBufferBase(GL_UNIFORM_BUFFER, 0, buf.ubo_globals);
}

void TextShader::render(const Buffer &buf, const std::string_view text, const CharStyle &style) {
	std::vector<size_t> indices;
	std::vector<ivec2> coords;
	render(buf, text, style, indices, coords);
}

void TextShader::render(const Buffer &buf, const std::string_view text, const CharStyle &style, const std::vector<size_t> &indices, std::vector<ivec2> &coords) {
	auto styles = std::make_unique<CharStyle[]>(text.size());

	uvec2 pos {};
	auto it = indices.begin();
	for (size_t i = 0; true; ++i) {
		if (it != indices.end() && *it == i) {
			coords.push_back(pos);
			++it;
		}

		if (i == text.size()) {
			break;
		}

		styles[i] = style;
		styles[i].char_pos = pos;
		styles[i].glyph = text[i];
		++pos.x;

		if (text[i] == '\n') {
			pos.x = 0;
			++pos.y;
		}
	}

	assert(indices.size() == coords.size());

	glBindBuffer(GL_ARRAY_BUFFER, buf.vbo_style);
	glBufferSubData(GL_ARRAY_BUFFER, 0, text.size() * sizeof(CharStyle), styles.get());
}

void TextShader::draw(ivec2 frame_offset, ivec2 scroll_offset, size_t start, size_t count, uint8_t z) {
	globals.frame_offset_px = frame_offset;
	globals.scroll_offset_px = scroll_offset;

	globals.z_order = z;
	update_uniforms();
	glDrawArraysInstancedBaseInstance(GL_TRIANGLE_STRIP, 0, 4, count, start);
}

void TextShader::UniformGlobals::set_viewport(ivec2 size) {
	u_proj = ortho<float>(0, size.x, size.y, 0, 0, -1);
}

const Font &TextShader::font() {
	return inst_->font_;
}
