#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "stripe_shader.h"

#include "stripe_vert.glsl.h"
#include "stripe_frag.glsl.h"

using namespace glm;

StripeShader::StripeShader() : shader_(stripe_vert_glsl, stripe_frag_glsl) {
}

StripeShader::~StripeShader() {
	// if (inst_) {
	// 	glDeleteBuffers(1, &ubo_globals_);
	// 	inst_.reset();
	// }
}

int StripeShader::init() {
	if (inst_) {
		return -1; // Already initialized
	}
	inst_ = std::unique_ptr<StripeShader>(new StripeShader());
	int ret = inst_->setup();
	if (ret != 0) {
		inst_.reset();
	}
	return ret;
}

int StripeShader::setup() {
	int ret = shader_.compile();
	if (ret != 0) {
		return ret;
	}
	shader_.use();
	globals_idx_ = glGetUniformBlockIndex(shader_.id(), "Globals");
	glUniformBlockBinding(shader_.id(), globals_idx_, 0);

	glGenBuffers(1, &ubo_globals_);
	glBindBuffer(GL_UNIFORM_BUFFER, ubo_globals_);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(UniformGlobals), nullptr, GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_UNIFORM_BUFFER, 0, ubo_globals_);

	return 0;
}

void StripeShader::create_buffers(Buffer &buf, size_t size) {
	buf.size = size;
	glGenVertexArrays(1, &buf.vao);
	glBindVertexArray(buf.vao);

	glGenBuffers(1, &buf.vbo_style);
	glBindBuffer(GL_ARRAY_BUFFER, buf.vbo_style);
	glBufferData(GL_ARRAY_BUFFER, size * sizeof(LineStyle), nullptr, GL_DYNAMIC_DRAW);
	glClearBufferData(GL_ARRAY_BUFFER, GL_R8UI, GL_RED_INTEGER, GL_UNSIGNED_BYTE, nullptr);

	glVertexAttribPointer(0, 1, GL_FLOAT, true, sizeof(LineStyle), (void*)offsetof(LineStyle, y));
	glEnableVertexAttribArray(0);
	glVertexAttribDivisor(0, 1);

	glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(LineStyle), (void*)offsetof(LineStyle, fg));
	glEnableVertexAttribArray(1);
	glVertexAttribDivisor(1, 1);
}

void StripeShader::destroy_buffers(Buffer &buf) {
	glDeleteBuffers(1, &buf.vbo_style);
	glDeleteVertexArrays(1, &buf.vao);
	buf = {};
}

void StripeShader::update_uniforms() {
	// glBindBufferBase(GL_UNIFORM_BUFFER, 0, buf.ubo_globals);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(inst_->globals), &inst_->globals);
}

void StripeShader::use(const Buffer &buf) {
	inst_->shader_.use();

	glBindVertexArray(buf.vao);
	glBindBuffer(GL_ARRAY_BUFFER, buf.vbo_style);
	glBindBuffer(GL_UNIFORM_BUFFER, inst_->ubo_globals_);

	glBindBufferBase(GL_UNIFORM_BUFFER, 0, inst_->ubo_globals_);
}

void StripeShader::update(const Buffer &buf, const LineStyle *data) {
	use(buf);
	glBindBuffer(GL_ARRAY_BUFFER, buf.vbo_style);
	glBufferSubData(GL_ARRAY_BUFFER, 0, buf.size * sizeof(LineStyle), data);
}

void StripeShader::draw(const Buffer &buf, ivec2 pos, ivec2 size, uint8_t width, uint8_t z) {
	use(buf);
	globals.pos_px = pos;
	globals.size_px = size;
	globals.width_px = width;
	globals.z_order = z;
	update_uniforms();
	glDrawArraysInstancedBaseInstance(GL_TRIANGLE_STRIP, 0, 4, buf.size, 0);
}

void StripeShader::UniformGlobals::set_viewport(ivec2 size) {
	u_proj = ortho<float>(0, size.x, size.y, 0, 0, -1);
}
